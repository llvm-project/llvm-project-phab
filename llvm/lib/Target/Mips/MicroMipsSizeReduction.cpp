//=== MicroMipsSizeReduction.cpp - MicroMips size reduction pass --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///\file
/// This pass is used to reduce the size of instructions where applicable.
///
/// TODO: Implement microMIPS64 support.
//===----------------------------------------------------------------------===//
#include "Mips.h"
#include "MipsInstrInfo.h"
#include "MipsSubtarget.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/Support/Debug.h"

using namespace llvm;

#define DEBUG_TYPE "micromips-reduce-size"

STATISTIC(NumReduced, "Number of instructions reduced (32-bit to 16-bit ones, "
                      "or two instructions into one");

namespace {

/// Order of operands to transfer
// TODO: Will be extended when additional optimizations are added
enum OperandTransfer {
  OT_NA,          ///< Not applicable
  OT_OperandsAll, ///< Transfer all operands
  OT_Operands02,  ///< Transfer operands 0 and 2
  OT_Operand2,    ///< Transfer just operand 2
  OT_OperandsXOR, ///< Transfer operands for XOR16
  OT_OperandsXwp, ///< Transfer operands for LWP/SWP
};

/// Reduction type
// TODO: Will be extended when additional optimizations are added
enum ReduceType {
  RT_TwoInstr, ///< Reduce two instructions into one instruction
  RT_OneInstr  ///< Reduce one instruction into a smaller instruction
};

// Information about immediate field restrictions
struct ImmField {
  ImmField() : ImmFieldOperand(-1), Shift(0), LBound(0), HBound(0) {}
  ImmField(uint8_t Shift, int16_t LBound, int16_t HBound,
           int8_t ImmFieldOperand)
      : ImmFieldOperand(ImmFieldOperand), Shift(Shift), LBound(LBound),
        HBound(HBound) {}
  int8_t ImmFieldOperand; // Immediate operand, -1 if it does not exist
  uint8_t Shift;          // Shift value
  int16_t LBound;         // Low bound of the immediate operand
  int16_t HBound;         // High bound of the immediate operand
};

/// Information about operands
// TODO: Will be extended when additional optimizations are added
struct OpInfo {
  OpInfo(enum OperandTransfer TransferOperands)
      : TransferOperands(TransferOperands) {}
  OpInfo() : TransferOperands(OT_NA) {}

  enum OperandTransfer
      TransferOperands; ///< Operands to transfer to the new instruction
};

// Information about opcodes
struct OpCodes {
  OpCodes(unsigned WideOpc, unsigned NarrowOpc)
      : WideOpc(WideOpc), NarrowOpc(NarrowOpc) {}

  unsigned WideOpc;   ///< Wide opcode
  unsigned NarrowOpc; ///< Narrow opcode
};

/// ReduceTable - A static table with information on mapping from wide
/// opcodes to narrow
struct ReduceEntry {

  enum ReduceType eRType;                ///< Reduction type
  bool (*ReduceFunction)(void *FunArgs); ///< Pointer to reduce function
  struct OpCodes Ops;                    ///< All relevant OpCodes
  struct OpInfo OpInf;                   ///< Characteristics of operands
  struct ImmField Imm;                   ///< Characteristics of immediate field

  ReduceEntry(enum ReduceType RType, struct OpCodes Op,
              bool (*F)(void *FunArgs), struct OpInfo OpInf,
              struct ImmField Imm)
      : eRType(RType), ReduceFunction(F), Ops(Op), OpInf(OpInf), Imm(Imm) {}

  unsigned NarrowOpc() const { return Ops.NarrowOpc; }
  unsigned WideOpc() const { return Ops.WideOpc; }
  int16_t LBound() const { return Imm.LBound; }
  int16_t HBound() const { return Imm.HBound; }
  uint8_t Shift() const { return Imm.Shift; }
  int8_t ImmField() const { return Imm.ImmFieldOperand; }
  enum OperandTransfer TransferOperands() const {
    return OpInf.TransferOperands;
  }
  enum ReduceType RType() const { return eRType; }

  // operator used by std::equal_range
  bool operator<(const unsigned int r) const { return (WideOpc() < r); }

  // operator used by std::equal_range
  friend bool operator<(const unsigned int r, const struct ReduceEntry &re) {
    return (r < re.WideOpc());
  }
};

// Function arguments for ReduceFunction
struct ReduceEntryFunArgs {
  MachineInstr *MI;         // Instruction
  const ReduceEntry &Entry; // Entry field
  MachineBasicBlock::instr_iterator
      &NextMII; // Iterator to next instruction in block
  const MachineBasicBlock::instr_iterator &E; // End iterator

  ReduceEntryFunArgs(MachineInstr *argMI, const ReduceEntry &argEntry,
                     MachineBasicBlock::instr_iterator &argNextMII,
                     const MachineBasicBlock::instr_iterator &argE)
      : MI(argMI), Entry(argEntry), NextMII(argNextMII), E(argE) {}
};

typedef llvm::SmallVector<ReduceEntry, 32> ReduceEntryVector;

class MicroMipsSizeReduce : public MachineFunctionPass {
public:
  static char ID;
  MicroMipsSizeReduce();

  static const MipsInstrInfo *MipsII;
  const MipsSubtarget *Subtarget;

  bool runOnMachineFunction(MachineFunction &MF) override;

  llvm::StringRef getPassName() const override {
    return "microMIPS instruction size reduction pass";
  }

private:
  /// Reduces width of instructions in the specified basic block.
  bool ReduceMBB(MachineBasicBlock &MBB);

  /// Attempts to reduce MI, returns true on success.
  bool ReduceMI(const MachineBasicBlock::instr_iterator &MII,
                MachineBasicBlock::instr_iterator &NextMII,
                const MachineBasicBlock::instr_iterator &E);

  // Attempts to reduce LW/SW instruction into LWSP/SWSP,
  // returns true on success.
  static bool ReduceXWtoXWSP(void *FunArgs);

  // Attempts to reduce two LW/SW instructions into LWP/SWP instruction,
  // returns true on success.
  static bool ReduceXWtoXWP(void *FunArgs);

  // Attempts to reduce LBU/LHU instruction into LBU16/LHU16,
  // returns true on success.
  static bool ReduceLXUtoLXU16(void *FunArgs);

  // Attempts to reduce SB/SH instruction into SB16/SH16,
  // returns true on success.
  static bool ReduceSXtoSX16(void *FunArgs);

  // Attempts to reduce arithmetic instructions, returns true on success.
  static bool ReduceArithmeticInstructions(void *FunArgs);

  // Attempts to reduce ADDIU into ADDIUSP instruction,
  // returns true on success.
  static bool ReduceADDIUToADDIUSP(void *FunArgs);

  // Attempts to reduce ADDIU into ADDIUR1SP instruction,
  // returns true on success.
  static bool ReduceADDIUToADDIUR1SP(void *FunArgs);

  // Attempts to reduce XOR into XOR16 instruction,
  // returns true on success.
  static bool ReduceXORtoXOR16(void *FunArgs);

  // Changes opcode of an instruction.
  static bool ReplaceInstruction(MachineInstr *MI, const ReduceEntry &Entry,
                                 MachineInstr *MI2 = nullptr,
                                 bool flag = false);

  // Table with transformation rules for each instruction.
  static ReduceEntryVector ReduceTable;
};

char MicroMipsSizeReduce::ID = 0;
const MipsInstrInfo *MicroMipsSizeReduce::MipsII;

// This table must be sorted by WideOpc as a main criterion and
// ReduceType as a sub-criterion (when wide opcodes are the same).
ReduceEntryVector MicroMipsSizeReduce::ReduceTable = {

    // ReduceType, OpCodes, ReduceFunction,
    // OpInfo(TransferOperands),
    // ImmField(Shift, LBound, HBound, ImmFieldPosition)
    {RT_OneInstr, OpCodes(Mips::ADDiu, Mips::ADDIUR1SP_MM),
     ReduceADDIUToADDIUR1SP, OpInfo(OT_Operands02), ImmField(2, 0, 64, 2)},
    {RT_OneInstr, OpCodes(Mips::ADDiu, Mips::ADDIUSP_MM), ReduceADDIUToADDIUSP,
     OpInfo(OT_Operand2), ImmField(0, 0, 0, 2)},
    {RT_OneInstr, OpCodes(Mips::ADDiu_MM, Mips::ADDIUR1SP_MM),
     ReduceADDIUToADDIUR1SP, OpInfo(OT_Operands02), ImmField(2, 0, 64, 2)},
    {RT_OneInstr, OpCodes(Mips::ADDiu_MM, Mips::ADDIUSP_MM),
     ReduceADDIUToADDIUSP, OpInfo(OT_Operand2), ImmField(0, 0, 0, 2)},
    {RT_OneInstr, OpCodes(Mips::ADDu, Mips::ADDU16_MM),
     ReduceArithmeticInstructions, OpInfo(OT_OperandsAll),
     ImmField(0, 0, 0, -1)},
    {RT_OneInstr, OpCodes(Mips::ADDu_MM, Mips::ADDU16_MM),
     ReduceArithmeticInstructions, OpInfo(OT_OperandsAll),
     ImmField(0, 0, 0, -1)},
    {RT_OneInstr, OpCodes(Mips::LBu, Mips::LBU16_MM), ReduceLXUtoLXU16,
     OpInfo(OT_OperandsAll), ImmField(0, -1, 15, 2)},
    {RT_OneInstr, OpCodes(Mips::LBu_MM, Mips::LBU16_MM), ReduceLXUtoLXU16,
     OpInfo(OT_OperandsAll), ImmField(0, -1, 15, 2)},
    {RT_OneInstr, OpCodes(Mips::LEA_ADDiu, Mips::ADDIUR1SP_MM),
     ReduceADDIUToADDIUR1SP, OpInfo(OT_Operands02), ImmField(2, 0, 64, 2)},
    {RT_OneInstr, OpCodes(Mips::LHu, Mips::LHU16_MM), ReduceLXUtoLXU16,
     OpInfo(OT_OperandsAll), ImmField(1, 0, 16, 2)},
    {RT_OneInstr, OpCodes(Mips::LHu_MM, Mips::LHU16_MM), ReduceLXUtoLXU16,
     OpInfo(OT_OperandsAll), ImmField(1, 0, 16, 2)},
    {RT_TwoInstr, OpCodes(Mips::LW, Mips::LWP_MM), ReduceXWtoXWP,
     OpInfo(OT_OperandsXwp), ImmField(0, -2048, 2048, 2)},
    {RT_OneInstr, OpCodes(Mips::LW, Mips::LWSP_MM), ReduceXWtoXWSP,
     OpInfo(OT_OperandsAll), ImmField(2, 0, 32, 2)},
    {RT_TwoInstr, OpCodes(Mips::LW_MM, Mips::LWP_MM), ReduceXWtoXWP,
     OpInfo(OT_OperandsXwp), ImmField(0, -2048, 2048, 2)},
    {RT_OneInstr, OpCodes(Mips::LW_MM, Mips::LWSP_MM), ReduceXWtoXWSP,
     OpInfo(OT_OperandsAll), ImmField(2, 0, 32, 2)},
    {RT_OneInstr, OpCodes(Mips::SB, Mips::SB16_MM), ReduceSXtoSX16,
     OpInfo(OT_OperandsAll), ImmField(0, 0, 16, 2)},
    {RT_OneInstr, OpCodes(Mips::SB_MM, Mips::SB16_MM), ReduceSXtoSX16,
     OpInfo(OT_OperandsAll), ImmField(0, 0, 16, 2)},
    {RT_OneInstr, OpCodes(Mips::SH, Mips::SH16_MM), ReduceSXtoSX16,
     OpInfo(OT_OperandsAll), ImmField(1, 0, 16, 2)},
    {RT_OneInstr, OpCodes(Mips::SH_MM, Mips::SH16_MM), ReduceSXtoSX16,
     OpInfo(OT_OperandsAll), ImmField(1, 0, 16, 2)},
    {RT_OneInstr, OpCodes(Mips::SUBu, Mips::SUBU16_MM),
     ReduceArithmeticInstructions, OpInfo(OT_OperandsAll),
     ImmField(0, 0, 0, -1)},
    {RT_OneInstr, OpCodes(Mips::SUBu_MM, Mips::SUBU16_MM),
     ReduceArithmeticInstructions, OpInfo(OT_OperandsAll),
     ImmField(0, 0, 0, -1)},
    {RT_TwoInstr, OpCodes(Mips::SW, Mips::SWP_MM), ReduceXWtoXWP,
     OpInfo(OT_OperandsXwp), ImmField(0, -2048, 2048, 2)},
    {RT_OneInstr, OpCodes(Mips::SW, Mips::SWSP_MM), ReduceXWtoXWSP,
     OpInfo(OT_OperandsAll), ImmField(2, 0, 32, 2)},
    {RT_TwoInstr, OpCodes(Mips::SW_MM, Mips::SWP_MM), ReduceXWtoXWP,
     OpInfo(OT_OperandsXwp), ImmField(0, -2048, 2048, 2)},
    {RT_OneInstr, OpCodes(Mips::SW_MM, Mips::SWSP_MM), ReduceXWtoXWSP,
     OpInfo(OT_OperandsAll), ImmField(2, 0, 32, 2)},
    {RT_OneInstr, OpCodes(Mips::XOR, Mips::XOR16_MM), ReduceXORtoXOR16,
     OpInfo(OT_OperandsXOR), ImmField(0, 0, 0, -1)},
    {RT_OneInstr, OpCodes(Mips::XOR_MM, Mips::XOR16_MM), ReduceXORtoXOR16,
     OpInfo(OT_OperandsXOR), ImmField(0, 0, 0, -1)}};
} // namespace

// Returns true if the machine operand MO is register SP.
static bool IsSP(const MachineOperand &MO) {
  if (MO.isReg() && ((MO.getReg() == Mips::SP)))
    return true;
  return false;
}

// Returns true if the machine operand MO is register $16, $17, or $2-$7.
static bool isMMThreeBitGPRegister(const MachineOperand &MO) {
  if (MO.isReg() && Mips::GPRMM16RegClass.contains(MO.getReg()))
    return true;
  return false;
}

// Returns true if the machine operand MO is register $0, $17, or $2-$7.
static bool isMMSourceRegister(const MachineOperand &MO) {
  if (MO.isReg() && Mips::GPRMM16ZeroRegClass.contains(MO.getReg()))
    return true;
  return false;
}

// Returns true if the operand Op is an immediate value
// and writes the immediate value into variable Imm.
static bool GetImm(MachineInstr *MI, unsigned Op, int64_t &Imm) {

  if (!MI->getOperand(Op).isImm())
    return false;
  Imm = MI->getOperand(Op).getImm();
  return true;
}

// Returns true if the value is a valid immediate for ADDIUSP.
static bool AddiuspImmValue(int64_t Value) {
  int64_t Value2 = Value >> 2;
  if (((Value & (int64_t)maskTrailingZeros<uint64_t>(2)) == Value) &&
      ((Value2 >= 2 && Value2 <= 257) || (Value2 >= -258 && Value2 <= -3)))
    return true;
  return false;
}

// Returns true if the variable Value has the number of least-significant zero
// bits equal to Shift and if the shifted value is between the bounds.
static bool InRange(int64_t Value, unsigned short Shift, int LBound,
                    int HBound) {
  int64_t Value2 = Value >> Shift;
  if (((Value & (int64_t)maskTrailingZeros<uint64_t>(Shift)) == Value) &&
      (Value2 >= LBound) && (Value2 < HBound))
    return true;
  return false;
}

// Returns true if immediate operand is in range.
static bool ImmInRange(MachineInstr *MI, const ReduceEntry &Entry) {

  int64_t offset;

  if (!GetImm(MI, Entry.ImmField(), offset))
    return false;

  if (!InRange(offset, Entry.Shift(), Entry.LBound(), Entry.HBound()))
    return false;

  return true;
}

// Returns true if MI can be reduced to lwp/swp instruciton
static bool CheckXWPInstr(MachineInstr *MI, bool ReduceToLwp,
                          const ReduceEntry &Entry) {

  if (ReduceToLwp &&
      !(MI->getOpcode() == Mips::LW || MI->getOpcode() == Mips::LW_MM))
    return false;

  if (!ReduceToLwp &&
      !(MI->getOpcode() == Mips::SW || MI->getOpcode() == Mips::SW_MM))
    return false;

  unsigned reg = MI->getOperand(0).getReg();
  if (reg == Mips::RA)
    return false;

  if (!ImmInRange(MI, Entry))
    return false;

  if (ReduceToLwp && (MI->getOperand(0).getReg() == MI->getOperand(1).getReg()))
    return false;

  return true;
}

// Returns true if the registers Reg1 and Reg2 are consecutive
static bool ConsecutiveRegisters(unsigned Reg1, unsigned Reg2) {
  static SmallVector<unsigned, 31> Registers = {
      Mips::AT, Mips::V0, Mips::V1, Mips::A0, Mips::A1, Mips::A2, Mips::A3,
      Mips::T0, Mips::T1, Mips::T2, Mips::T3, Mips::T4, Mips::T5, Mips::T6,
      Mips::T7, Mips::S0, Mips::S1, Mips::S2, Mips::S3, Mips::S4, Mips::S5,
      Mips::S6, Mips::S7, Mips::T8, Mips::T9, Mips::K0, Mips::K1, Mips::GP,
      Mips::SP, Mips::FP, Mips::RA};

  for (uint8_t i = 0; i < Registers.size() - 1; i++) {
    if (Registers[i] == Reg1) {
      if (Registers[i + 1] == Reg2)
        return true;
      else
        return false;
    }
  }
  return false;
}

// Returns true if registers and offsets are consecutive
static bool ConsecutiveInstr(MachineInstr *MI1, MachineInstr *MI2) {

  int64_t Offset1, Offset2;
  if (!GetImm(MI1, 2, Offset1))
    return false;
  if (!GetImm(MI2, 2, Offset2))
    return false;

  unsigned Reg1 = MI1->getOperand(0).getReg();
  unsigned Reg2 = MI2->getOperand(0).getReg();

  return ((Offset1 == (Offset2 - 4)) && (ConsecutiveRegisters(Reg1, Reg2)));
}

MicroMipsSizeReduce::MicroMipsSizeReduce() : MachineFunctionPass(ID) {}

bool MicroMipsSizeReduce::ReduceMI(const MachineBasicBlock::instr_iterator &MII,
                                   MachineBasicBlock::instr_iterator &NextMII,
                                   const MachineBasicBlock::instr_iterator &E) {

  MachineInstr *MI = &*MII;
  unsigned Opcode = MI->getOpcode();

  // Search the table.
  ReduceEntryVector::const_iterator Start = std::begin(ReduceTable);
  ReduceEntryVector::const_iterator End = std::end(ReduceTable);

  std::pair<ReduceEntryVector::const_iterator,
            ReduceEntryVector::const_iterator>
      Range = std::equal_range(Start, End, Opcode);

  if (Range.first == Range.second)
    return false;

  for (ReduceEntryVector::const_iterator Entry = Range.first;
       Entry != Range.second; ++Entry) {
    struct ReduceEntryFunArgs FunArgs(&(*MII), *Entry, NextMII, E);
    if (((*Entry).ReduceFunction)(&FunArgs))
      return true;
  }
  return false;
}

bool MicroMipsSizeReduce::ReduceXWtoXWSP(void *FunArgs) {

  ReduceEntryFunArgs Arguments = *(ReduceEntryFunArgs *)FunArgs;
  MachineInstr *MI = Arguments.MI;
  const ReduceEntry &Entry = Arguments.Entry;

  if (!ImmInRange(MI, Entry))
    return false;

  if (!IsSP(MI->getOperand(1)))
    return false;

  return ReplaceInstruction(MI, Entry);
}

bool MicroMipsSizeReduce::ReduceXWtoXWP(void *FunArgs) {

  ReduceEntryFunArgs Arguments = *(ReduceEntryFunArgs *)FunArgs;
  const ReduceEntry &Entry = Arguments.Entry;
  const MachineBasicBlock::instr_iterator &E = Arguments.E;
  MachineBasicBlock::instr_iterator &NextMII = Arguments.NextMII;

  MachineInstr *MI1 = Arguments.MI;
  MachineInstr *MI2 = nullptr;

  if (NextMII == E)
    return false;

  // ReduceToLwp = true/false - reduce to LWP/SWP instruction
  bool ReduceToLwp =
      (MI1->getOpcode() == Mips::LW) || (MI1->getOpcode() == Mips::LW_MM);

  if (!CheckXWPInstr(MI1, ReduceToLwp, Entry))
    return false;

  MI2 = &*NextMII;
  if (!CheckXWPInstr(MI2, ReduceToLwp, Entry))
    return false;

  bool Reduce = false;
  bool ConsecutiveForward = false;
  bool ConsecutiveBackward = false;

  unsigned Reg1 = MI1->getOperand(1).getReg();
  unsigned Reg2 = MI2->getOperand(1).getReg();

  if (Reg1 == Reg2) {
    ConsecutiveForward = ConsecutiveInstr(MI1, MI2);
    ConsecutiveBackward = ConsecutiveInstr(MI2, MI1);
    Reduce = ConsecutiveForward || ConsecutiveBackward;
  }

  if (!Reduce)
    return false;

  NextMII = std::next(NextMII);
  return ReplaceInstruction(MI1, Entry, MI2, ConsecutiveForward);
}

bool MicroMipsSizeReduce::ReduceArithmeticInstructions(void *FunArgs) {

  ReduceEntryFunArgs Arguments = *(ReduceEntryFunArgs *)FunArgs;
  MachineInstr *MI = Arguments.MI;
  const ReduceEntry &Entry = Arguments.Entry;

  if (!isMMThreeBitGPRegister(MI->getOperand(0)) ||
      !isMMThreeBitGPRegister(MI->getOperand(1)) ||
      !isMMThreeBitGPRegister(MI->getOperand(2)))
    return false;

  return ReplaceInstruction(MI, Entry);
}

bool MicroMipsSizeReduce::ReduceADDIUToADDIUR1SP(void *FunArgs) {

  ReduceEntryFunArgs Arguments = *(ReduceEntryFunArgs *)FunArgs;
  MachineInstr *MI = Arguments.MI;
  const ReduceEntry &Entry = Arguments.Entry;

  if (!ImmInRange(MI, Entry))
    return false;

  if (!isMMThreeBitGPRegister(MI->getOperand(0)) || !IsSP(MI->getOperand(1)))
    return false;

  return ReplaceInstruction(MI, Entry);
}

bool MicroMipsSizeReduce::ReduceADDIUToADDIUSP(void *FunArgs) {

  ReduceEntryFunArgs Arguments = *(ReduceEntryFunArgs *)FunArgs;
  MachineInstr *MI = Arguments.MI;
  const ReduceEntry &Entry = Arguments.Entry;

  int64_t ImmValue;
  if (!GetImm(MI, Entry.ImmField(), ImmValue))
    return false;

  if (!AddiuspImmValue(ImmValue))
    return false;

  if (!IsSP(MI->getOperand(0)) || !IsSP(MI->getOperand(1)))
    return false;

  return ReplaceInstruction(MI, Entry);
}

bool MicroMipsSizeReduce::ReduceLXUtoLXU16(void *FunArgs) {

  ReduceEntryFunArgs Arguments = *(ReduceEntryFunArgs *)FunArgs;
  MachineInstr *MI = Arguments.MI;
  const ReduceEntry &Entry = Arguments.Entry;

  if (!ImmInRange(MI, Entry))
    return false;

  if (!isMMThreeBitGPRegister(MI->getOperand(0)) ||
      !isMMThreeBitGPRegister(MI->getOperand(1)))
    return false;

  return ReplaceInstruction(MI, Entry);
}

bool MicroMipsSizeReduce::ReduceSXtoSX16(void *FunArgs) {

  ReduceEntryFunArgs Arguments = *(ReduceEntryFunArgs *)FunArgs;
  MachineInstr *MI = Arguments.MI;
  const ReduceEntry &Entry = Arguments.Entry;

  if (!ImmInRange(MI, Entry))
    return false;

  if (!isMMSourceRegister(MI->getOperand(0)) ||
      !isMMThreeBitGPRegister(MI->getOperand(1)))
    return false;

  return ReplaceInstruction(MI, Entry);
}

bool MicroMipsSizeReduce::ReduceXORtoXOR16(void *FunArgs) {
  ReduceEntryFunArgs Arguments = *(ReduceEntryFunArgs *)FunArgs;
  MachineInstr *MI = Arguments.MI;
  const ReduceEntry &Entry = Arguments.Entry;

  if (!isMMThreeBitGPRegister(MI->getOperand(0)) ||
      !isMMThreeBitGPRegister(MI->getOperand(1)) ||
      !isMMThreeBitGPRegister(MI->getOperand(2)))
    return false;

  if (!(MI->getOperand(0).getReg() == MI->getOperand(2).getReg()) &&
      !(MI->getOperand(0).getReg() == MI->getOperand(1).getReg()))
    return false;

  return ReplaceInstruction(MI, Entry);
}

bool MicroMipsSizeReduce::ReduceMBB(MachineBasicBlock &MBB) {
  bool Modified = false;
  MachineBasicBlock::instr_iterator MII = MBB.instr_begin(),
                                    E = MBB.instr_end();
  MachineBasicBlock::instr_iterator NextMII;

  // Iterate through the instructions in the basic block
  for (; MII != E; MII = NextMII) {
    NextMII = std::next(MII);
    MachineInstr *MI = &*MII;

    // Don't reduce bundled instructions or pseudo operations
    if (MI->isBundle() || MI->isTransient())
      continue;

    // Try to reduce 32-bit instruction into 16-bit instruction
    Modified |= ReduceMI(MII, NextMII, E);
  }

  return Modified;
}

bool MicroMipsSizeReduce::ReplaceInstruction(MachineInstr *MI,
                                             const ReduceEntry &Entry,
                                             MachineInstr *MI2, bool flag) {

  enum OperandTransfer OpTransfer = Entry.TransferOperands();

  DEBUG(dbgs() << "Converting 32-bit: " << *MI);
  ++NumReduced;

  if (OpTransfer == OT_OperandsAll) {
    MI->setDesc(MipsII->get(Entry.NarrowOpc()));
    DEBUG(dbgs() << "       to 16-bit: " << *MI);
    return true;
  } else {
    MachineBasicBlock &MBB = *MI->getParent();
    const MCInstrDesc &NewMCID = MipsII->get(Entry.NarrowOpc());
    DebugLoc dl = MI->getDebugLoc();
    MachineInstrBuilder MIB = BuildMI(MBB, MI, dl, NewMCID);
    switch (OpTransfer) {
    case OT_Operand2:
      MIB.add(MI->getOperand(2));
      break;
    case OT_Operands02: {
      MIB.add(MI->getOperand(0));
      MIB.add(MI->getOperand(2));
      break;
    }
    case OT_OperandsXOR: {
      if (MI->getOperand(0).getReg() == MI->getOperand(2).getReg()) {
        MIB.add(MI->getOperand(0));
        MIB.add(MI->getOperand(1));
        MIB.add(MI->getOperand(2));
      } else {
        MIB.add(MI->getOperand(0));
        MIB.add(MI->getOperand(2));
        MIB.add(MI->getOperand(1));
      }
      break;
    }
    case OT_OperandsXwp: {
      if (flag) {
        MIB.add(MI->getOperand(0));
        // FIXME: This should be MIB.add(MI2->getOperand(0));
        // However, TabGen counts regpair as one output operand,
        // and that introduces machine verfier error for lwp instruction.
        // Setting the second register as undef, bypasses this error.
        // This should not introduce bugs as this pass is run
        // immediately before machine code is emitted.
        MIB.addReg(MI2->getOperand(0).getReg(), RegState::Undef);
        MIB.add(MI->getOperand(1));
        MIB.add(MI->getOperand(2));
      } else { // consecutive backward
        MIB.add(MI2->getOperand(0));
        // FIXME: This should be MIB.add(MI->getOperand(0));
        // See the comment above.
        MIB.addReg(MI->getOperand(0).getReg(), RegState::Undef);
        MIB.add(MI2->getOperand(1));
        MIB.add(MI2->getOperand(2));
      }

      DEBUG(dbgs() << "and converting 32-bit: " << *MI2
                   << "       to: " << *MIB);

      MBB.erase_instr(MI);
      MBB.erase_instr(MI2);
      return true;
    }

    default:
      llvm_unreachable("Unknown operand transfer!");
    }

    // Transfer MI flags.
    MIB.setMIFlags(MI->getFlags());

    DEBUG(dbgs() << "       to 16-bit: " << *MIB);
    MBB.erase_instr(MI);
    return true;
  }
  return false;
}

bool MicroMipsSizeReduce::runOnMachineFunction(MachineFunction &MF) {

  Subtarget = &static_cast<const MipsSubtarget &>(MF.getSubtarget());

  // TODO: Add support for other subtargets:
  // microMIPS32r6
  if (!Subtarget->inMicroMipsMode() || !Subtarget->hasMips32r2() ||
      Subtarget->hasMips32r6())
    return false;

  MipsII = static_cast<const MipsInstrInfo *>(Subtarget->getInstrInfo());

  bool Modified = false;
  MachineFunction::iterator I = MF.begin(), E = MF.end();

  for (; I != E; ++I)
    Modified |= ReduceMBB(*I);
  return Modified;
}

/// Returns an instance of the MicroMips size reduction pass.
FunctionPass *llvm::createMicroMipsSizeReductionPass() {
  return new MicroMipsSizeReduce();
}
