//=- AArch64SIMDInstrOpt.cpp - AArch64 vector by element inst opt pass =//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a pass that performs optimization on SIMD instructions
// with high latency by splitting them into more efficient series of
// instructions.
//
// 1. Rewrite certain SIMD instructions with vector element due to their
// inefficiency on some targets.
// Example:
//    fmla v0.4s, v1.4s, v2.s[1]
//    is rewritten into
//    dup v3.4s, v2.s[1]
//    fmla v0.4s, v1.4s, v3.4s
//
// 2. Rewrite Interleaved memory access instructions due to their
// inefficiency on some targets.
// Example:
//    st2 {v0.4s, v1.4s}, addr
//    is rewritten into
//    zip1 v2.4s, v0.4s, v1.4s
//    zip2 v3.4s, v0.4s, v1.4s
//    stp  q2, q3,  addr
//
//===----------------------------------------------------------------------===//

#include "AArch64InstrInfo.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetSchedule.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/MCSchedule.h"
#include "llvm/Pass.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#include <map>
#include <string>

using namespace llvm;

#define DEBUG_TYPE "aarch64-simdinstr-opt"

STATISTIC(NumModifiedInstr,
          "Number of SIMD instructions modified");

#define AARCH64_VECTOR_BY_ELEMENT_OPT_NAME                                     \
  "AArch64 SIMD instructions optimization pass"

namespace {

struct AArch64SIMDInstrOpt : public MachineFunctionPass {
  static char ID;

  const TargetInstrInfo *TII;
  MachineRegisterInfo *MRI;
  TargetSchedModel SchedModel;

  // This is used to cache instruction replacement decisions within function
  // units and across function units.
  std::map<std::pair<unsigned, std::string>, bool> SIMDInstrTable;

  typedef enum {
    VectorElem,
    Interleave
  } Subpass;

  typedef enum {
    TwoVectors,
    FourVectors
  } AccessType;

  struct InstReplInfo {
    unsigned OrigOpc;
    unsigned ReplOpc[3];
    TargetRegisterClass RC;
    AccessType AccT;
  };

#define Rules(Opc1, Opc2, Opc3, Opc4, RC, ACCT) \
  {Opc1, {Opc2, Opc3, Opc4}, RC, ACCT}

  // The maximum number of instruction replacement kind.
  static const unsigned MaxInstrKind = 14;
  InstReplInfo IRI [MaxInstrKind] = {
    Rules(AArch64::ST2Twov2d, AArch64::ZIP1v2i64, AArch64::ZIP2v2i64, AArch64::STPQi, AArch64::FPR128RegClass, TwoVectors),
    Rules(AArch64::ST2Twov4s, AArch64::ZIP1v4i32, AArch64::ZIP2v4i32, AArch64::STPQi,  AArch64::FPR128RegClass, TwoVectors),
    Rules(AArch64::ST2Twov2s, AArch64::ZIP1v2i32, AArch64::ZIP2v2i32, AArch64::STPDi,  AArch64::FPR64RegClass, TwoVectors),
    Rules(AArch64::ST2Twov8h, AArch64::ZIP1v8i16, AArch64::ZIP2v8i16, AArch64::STPQi,  AArch64::FPR128RegClass, TwoVectors),
    Rules(AArch64::ST2Twov4h, AArch64::ZIP1v4i16, AArch64::ZIP2v4i16, AArch64::STPDi,  AArch64::FPR64RegClass, TwoVectors),
    Rules(AArch64::ST2Twov16b, AArch64::ZIP1v16i8, AArch64::ZIP2v16i8, AArch64::STPQi,  AArch64::FPR128RegClass, TwoVectors),
    Rules(AArch64::ST2Twov8b, AArch64::ZIP1v8i8, AArch64::ZIP2v8i8, AArch64::STPDi,  AArch64::FPR64RegClass, TwoVectors),
    Rules(AArch64::ST4Fourv2d, AArch64::ZIP1v2i64, AArch64::ZIP2v2i64, AArch64::STPQi,  AArch64::FPR128RegClass, FourVectors),
    Rules(AArch64::ST4Fourv4s, AArch64::ZIP1v4i32, AArch64::ZIP2v4i32, AArch64::STPQi,  AArch64::FPR128RegClass, FourVectors),
    Rules(AArch64::ST4Fourv2s, AArch64::ZIP1v2i32, AArch64::ZIP2v2i32, AArch64::STPDi,  AArch64::FPR64RegClass, FourVectors),
    Rules(AArch64::ST4Fourv8h, AArch64::ZIP1v8i16, AArch64::ZIP2v8i16, AArch64::STPQi,  AArch64::FPR128RegClass, FourVectors),
    Rules(AArch64::ST4Fourv4h, AArch64::ZIP1v4i16, AArch64::ZIP2v4i16, AArch64::STPDi,  AArch64::FPR64RegClass, FourVectors),
    Rules(AArch64::ST4Fourv16b, AArch64::ZIP1v16i8, AArch64::ZIP2v16i8, AArch64::STPQi,  AArch64::FPR128RegClass, FourVectors),
    Rules(AArch64::ST4Fourv8b, AArch64::ZIP1v8i8, AArch64::ZIP2v8i8, AArch64::STPQi,  AArch64::FPR64RegClass, FourVectors)
  };

  // A costly instruction is replaced in this work by N efficient instructions
  // The maximum of N is curently 10 and it is for ST4 case.
  static const unsigned MaxNumRepl = 10;

  AArch64SIMDInstrOpt() : MachineFunctionPass(ID) {
    initializeAArch64SIMDInstrOptPass(*PassRegistry::getPassRegistry());
  }

  /// Based only on latency of instructions, determine if it is cost efficient
  /// to replace the instruction InstDesc by the instructions stored in the
  /// array InstDescRepl.
  /// Return true if replacement is expected to be faster.
  bool shouldReplaceInst(MachineFunction *MF, const MCInstrDesc *InstDesc,
                         SmallVectorImpl<const MCInstrDesc*> &ReplInstrMCID);

  /// Determine if we need to exit the instruction replacement optimization
  /// subpasses early. This makes sure that Targets with no need for this
  /// optimization do not spend any compile time on this subpass other than the
  /// simple check performed here. This simple check is done by comparing the
  /// latency of the original instruction to the latency of the replacement
  /// instructions. We only check for a representative instruction in the class
  /// of instructions and not all concerned instructions. For the VectorElem
  /// subpass, we check for the FMLA instruction while for the interleave subpass
  /// we check for the st2.4s instruction.
  /// Return true if early exit of the subpass is recommended.
  bool shouldExitEarly(MachineFunction *MF, Subpass SP);

  /// Check whether an equivalent DUP instruction has already been
  /// created or not.
  /// Return true when the dup instruction already exists. In this case,
  /// DestReg will point to the destination of the already created DUP.
  bool reuseDUP(MachineInstr &MI, unsigned DupOpcode, unsigned SrcReg,
                unsigned LaneNumber, unsigned *DestReg) const;

  /// Certain SIMD instructions with vector element operand are not efficient.
  /// Rewrite them into SIMD instructions with vector operands. This rewrite
  /// is driven by the latency of the instructions.
  /// Return true if the SIMD instruction is modified.
  bool optimizeVectElement(MachineInstr &MI);

  /// Process The REG_SEQUENCE instruction, and extract the source
  /// operands of the st2/4 instruction from it.
  /// Example of such instructions.
  ///    %dest = REG_SEQUENCE %st2_src1, dsub0, %st2_src2, dsub1;
  /// Return true when the instruction is processed successfully.
  bool processSeqRegInst(MachineInstr *DefiningMI, unsigned* StReg,
                         unsigned* StRegKill, unsigned NumArg) const;

  /// Prepare the parameters needed to build the replacement statements.
  void prepareStmtParam(unsigned Opcode1, unsigned Opcode2,
                        const TargetRegisterClass *RC,
                        SmallVectorImpl<const MCInstrDesc*> &ReplInstrMCID,
                        SmallVectorImpl<unsigned> &ZipDest, AccessType AccT)
                        const;

  /// Load/Store Interleaving instructions are not always beneficial.
  /// Replace them by zip instructionand classical load/store.
  /// Return true if the SIMD instruction is modified.
  bool optimizeLdStInterleave(MachineInstr &MI);

  bool runOnMachineFunction(MachineFunction &Fn) override;

  StringRef getPassName() const override {
    return AARCH64_VECTOR_BY_ELEMENT_OPT_NAME;
  }
};

char AArch64SIMDInstrOpt::ID = 0;

} // end anonymous namespace

INITIALIZE_PASS(AArch64SIMDInstrOpt, "aarch64-simdinstr-opt",
                AARCH64_VECTOR_BY_ELEMENT_OPT_NAME, false, false)

/// Based only on latency of instructions, determine if it is cost efficient
/// to replace the instruction InstDesc by the instructions stored in the
/// array InstDescRepl.
/// Return true if replacement is expected to be faster.
bool AArch64SIMDInstrOpt::
shouldReplaceInst(MachineFunction *MF, const MCInstrDesc *InstDesc,
                  SmallVectorImpl<const MCInstrDesc*> &InstDescRepl) {
  // Check if replacement decision is already available in the cached table.
  // if so, return it.
  std::string Subtarget = SchedModel.getSubtargetInfo()->getCPU();
	std::pair<unsigned, std::string> InstID = std::make_pair(InstDesc->getOpcode(), Subtarget);
  if (!SIMDInstrTable.empty() &&
      SIMDInstrTable.find(InstID) != SIMDInstrTable.end())
    return SIMDInstrTable[InstID];

  unsigned SCIdx = InstDesc->getSchedClass();
  const MCSchedClassDesc *SCDesc =
    SchedModel.getMCSchedModel()->getSchedClassDesc(SCIdx);

  // If a subtarget does not define resources for the instructions
  // of interest, then return false for no replacement.
  const MCSchedClassDesc *SCDescRepl;
  if (!SCDesc->isValid() || SCDesc->isVariant())
  {
    SIMDInstrTable[InstID] = false;
    return false;
  }
  for (auto IDesc : InstDescRepl)
  {
    SCDescRepl = SchedModel.getMCSchedModel()->getSchedClassDesc(
      IDesc->getSchedClass());
    if (!SCDescRepl->isValid() || SCDescRepl->isVariant())
    {
      SIMDInstrTable[InstID] = false;
      return false;
    }
  }

  // Replacement cost.
  unsigned ReplCost = 0;
  for (auto IDesc :InstDescRepl)
    ReplCost += SchedModel.computeInstrLatency(IDesc->getOpcode());

  if (SchedModel.computeInstrLatency(InstDesc->getOpcode()) > ReplCost)
  {
    SIMDInstrTable[InstID] = true;
    return true;
  }
  else
  {
    SIMDInstrTable[InstID] = false;
    return false;
  }
}

/// Determine if we need to exit the instruction replacement optimization
/// subpasses early. This makes sure that Targets with no need for this
/// optimization do not spend any compile time on this subpass other than the
/// simple check performed here. This simple check is done by comparing the
/// latency of the original instruction to the latency of the replacement
/// instructions. We only check for a representative instruction in the class of
/// instructions and not all concerned instructions. For the VectorElem subpass,
/// we check for the FMLA instruction while for the interleave subpass we check
/// for the st2.4s instruction.
/// Return true if early exit of the subpass is recommended.
bool AArch64SIMDInstrOpt::shouldExitEarly(MachineFunction *MF, Subpass SP) {
  const MCInstrDesc* OriginalMCID;
  SmallVector<const MCInstrDesc*, MaxNumRepl> ReplInstrMCID;

  switch (SP) {
  case VectorElem:
    OriginalMCID = &TII->get(AArch64::FMLAv4i32_indexed);
    ReplInstrMCID.push_back(&TII->get(AArch64::DUPv4i32lane));
    ReplInstrMCID.push_back(&TII->get(AArch64::FMULv4f32));
    if (shouldReplaceInst(MF, OriginalMCID, ReplInstrMCID))
      return false;
    break;
  case Interleave:
    for (unsigned i=0; i<MaxInstrKind; i++)
    {
      OriginalMCID = &TII->get(IRI[i].OrigOpc);
      if (IRI[i].AccT == TwoVectors) {
        ReplInstrMCID.push_back(&TII->get(IRI[i].ReplOpc[0]));
        ReplInstrMCID.push_back(&TII->get(IRI[i].ReplOpc[1]));
        ReplInstrMCID.push_back(&TII->get(IRI[i].ReplOpc[2]));
      }
			else { // FourVectors
        for (unsigned j=0; j<4; j++) {
          ReplInstrMCID.push_back(&TII->get(IRI[i].ReplOpc[0]));
          ReplInstrMCID.push_back(&TII->get(IRI[i].ReplOpc[1]));
        }
        ReplInstrMCID.push_back(&TII->get(IRI[i].ReplOpc[2]));
        ReplInstrMCID.push_back(&TII->get(IRI[i].ReplOpc[2]));
			}
      if (shouldReplaceInst(MF, OriginalMCID, ReplInstrMCID))
        return false;
      ReplInstrMCID.clear();
		}
    break;
  }

  return true;
}

/// Check whether an equivalent DUP instruction has already been
/// created or not.
/// Return true when the dup instruction already exists. In this case,
/// DestReg will point to the destination of the already created DUP.
bool AArch64SIMDInstrOpt::reuseDUP(MachineInstr &MI, unsigned DupOpcode,
                                         unsigned SrcReg, unsigned LaneNumber,
                                         unsigned *DestReg) const {
  for (MachineBasicBlock::iterator MII = MI, MIE = MI.getParent()->begin();
       MII != MIE;) {
    MII--;
    MachineInstr *CurrentMI = &*MII;

    if (CurrentMI->getOpcode() == DupOpcode &&
        CurrentMI->getNumOperands() == 3 &&
        CurrentMI->getOperand(1).getReg() == SrcReg &&
        CurrentMI->getOperand(2).getImm() == LaneNumber) {
      *DestReg = CurrentMI->getOperand(0).getReg();
      return true;
    }
  }

  return false;
}

/// Certain SIMD instructions with vector element operand are not efficient.
/// Rewrite them into SIMD instructions with vector operands. This rewrite
/// is driven by the latency of the instructions.
/// The instruction of concerns are for the time being fmla, fmls, fmul,
/// and fmulx and hence they are hardcoded.
///
/// Example:
///    fmla v0.4s, v1.4s, v2.s[1]
///    is rewritten into
///    dup v3.4s, v2.s[1]           // dup not necessary if redundant
///    fmla v0.4s, v1.4s, v3.4s
/// Return true if the SIMD instruction is modified.
bool AArch64SIMDInstrOpt::optimizeVectElement(MachineInstr &MI) {
  const MCInstrDesc *MulMCID, *DupMCID;
  const TargetRegisterClass *RC = &AArch64::FPR128RegClass;

  switch (MI.getOpcode()) {
  default:
    return false;

  // 4X32 instructions
  case AArch64::FMLAv4i32_indexed:
    DupMCID = &TII->get(AArch64::DUPv4i32lane);
    MulMCID = &TII->get(AArch64::FMLAv4f32);
    break;
  case AArch64::FMLSv4i32_indexed:
    DupMCID = &TII->get(AArch64::DUPv4i32lane);
    MulMCID = &TII->get(AArch64::FMLSv4f32);
    break;
  case AArch64::FMULXv4i32_indexed:
    DupMCID = &TII->get(AArch64::DUPv4i32lane);
    MulMCID = &TII->get(AArch64::FMULXv4f32);
    break;
  case AArch64::FMULv4i32_indexed:
    DupMCID = &TII->get(AArch64::DUPv4i32lane);
    MulMCID = &TII->get(AArch64::FMULv4f32);
    break;

  // 2X64 instructions
  case AArch64::FMLAv2i64_indexed:
    DupMCID = &TII->get(AArch64::DUPv2i64lane);
    MulMCID = &TII->get(AArch64::FMLAv2f64);
    break;
  case AArch64::FMLSv2i64_indexed:
    DupMCID = &TII->get(AArch64::DUPv2i64lane);
    MulMCID = &TII->get(AArch64::FMLSv2f64);
    break;
  case AArch64::FMULXv2i64_indexed:
    DupMCID = &TII->get(AArch64::DUPv2i64lane);
    MulMCID = &TII->get(AArch64::FMULXv2f64);
    break;
  case AArch64::FMULv2i64_indexed:
    DupMCID = &TII->get(AArch64::DUPv2i64lane);
    MulMCID = &TII->get(AArch64::FMULv2f64);
    break;

  // 2X32 instructions
  case AArch64::FMLAv2i32_indexed:
    RC = &AArch64::FPR64RegClass;
    DupMCID = &TII->get(AArch64::DUPv2i32lane);
    MulMCID = &TII->get(AArch64::FMLAv2f32);
    break;
  case AArch64::FMLSv2i32_indexed:
    RC = &AArch64::FPR64RegClass;
    DupMCID = &TII->get(AArch64::DUPv2i32lane);
    MulMCID = &TII->get(AArch64::FMLSv2f32);
    break;
  case AArch64::FMULXv2i32_indexed:
    RC = &AArch64::FPR64RegClass;
    DupMCID = &TII->get(AArch64::DUPv2i32lane);
    MulMCID = &TII->get(AArch64::FMULXv2f32);
    break;
  case AArch64::FMULv2i32_indexed:
    RC = &AArch64::FPR64RegClass;
    DupMCID = &TII->get(AArch64::DUPv2i32lane);
    MulMCID = &TII->get(AArch64::FMULv2f32);
    break;
  }

  SmallVector<const MCInstrDesc*, 2> ReplInstrMCID;
  ReplInstrMCID.push_back(DupMCID);
  ReplInstrMCID.push_back(MulMCID);
  if (!shouldReplaceInst(MI.getParent()->getParent(), &TII->get(MI.getOpcode()),
                         ReplInstrMCID))
    return false;

  const DebugLoc &DL = MI.getDebugLoc();
  MachineBasicBlock &MBB = *MI.getParent();
  MachineRegisterInfo &MRI = MBB.getParent()->getRegInfo();

  // get the operands of the current SIMD arithmetic instruction.
  unsigned MulDest = MI.getOperand(0).getReg();
  unsigned SrcReg0 = MI.getOperand(1).getReg();
  unsigned Src0IsKill = getKillRegState(MI.getOperand(1).isKill());
  unsigned SrcReg1 = MI.getOperand(2).getReg();
  unsigned Src1IsKill = getKillRegState(MI.getOperand(2).isKill());
  unsigned DupDest;

  // Instructions of interest have either 4 or 5 operands.
  if (MI.getNumOperands() == 5) {
    unsigned SrcReg2 = MI.getOperand(3).getReg();
    unsigned Src2IsKill = getKillRegState(MI.getOperand(3).isKill());
    unsigned LaneNumber = MI.getOperand(4).getImm();
    // Create a new DUP instruction. Note that if an equivalent DUP instruction
    // has already been created before, then use that one instread of creating
    // a new one.
    if (!reuseDUP(MI, DupMCID->getOpcode(), SrcReg2, LaneNumber, &DupDest)) {
      DupDest = MRI.createVirtualRegister(RC);
      BuildMI(MBB, MI, DL, *DupMCID, DupDest)
          .addReg(SrcReg2, Src2IsKill)
          .addImm(LaneNumber);
    }
    BuildMI(MBB, MI, DL, *MulMCID, MulDest)
        .addReg(SrcReg0, Src0IsKill)
        .addReg(SrcReg1, Src1IsKill)
        .addReg(DupDest, Src2IsKill);
  } else if (MI.getNumOperands() == 4) {
    unsigned LaneNumber = MI.getOperand(3).getImm();
    if (!reuseDUP(MI, DupMCID->getOpcode(), SrcReg1, LaneNumber, &DupDest)) {
      DupDest = MRI.createVirtualRegister(RC);
      BuildMI(MBB, MI, DL, *DupMCID, DupDest)
          .addReg(SrcReg1, Src1IsKill)
          .addImm(LaneNumber);
    }
    BuildMI(MBB, MI, DL, *MulMCID, MulDest)
        .addReg(SrcReg0, Src0IsKill)
        .addReg(DupDest, Src1IsKill);
  } else {
    return false;
  }

  ++NumModifiedInstr;
  return true;
}

/// Load/Store Interleaving instructions are not always beneficial.
/// Replace them by zip instructions and classical load/store.
///
/// Example:
///    st2 {v0.4s, v1.4s}, addr
///    is rewritten into
///    zip1 v2.4s, v0.4s, v1.4s
///    zip2 v3.4s, v0.4s, v1.4s
///    stp  q2, q3, addr
//
/// Example:
///    st4 {v0.4s, v1.4s, v2.4s, v3.4s}, addr
///    is rewritten into
///    zip1 v4.4s, v0.4s, v2.4s
///    zip2 v5.4s, v0.4s, v2.4s
///    zip1 v6.4s, v1.4s, v3.4s
///    zip2 v7.4s, v1.4s, v3.4s
///    zip1 v8.4s, v4.4s, v6.4s
///    zip2 v9.4s, v4.4s, v6.4s
///    zip1 v10.4s, v5.4s, v7.4s
///    zip2 v11.4s, v5.4s, v7.4s
///    stp  q8, q9, addr
///    stp  q10, q11, addr+32
/// Currently only instructions related to st2 and st4 are considered.
/// Other may be added later.
/// Return true if the SIMD instruction is modified.
bool AArch64SIMDInstrOpt::optimizeLdStInterleave(MachineInstr &MI) {

  unsigned SeqReg, AddrReg;
  unsigned StReg[4], StRegKill[4];
  const TargetRegisterClass *RC128 = &AArch64::FPR128RegClass;
  const TargetRegisterClass *RC64  = &AArch64::FPR64RegClass;
  MachineInstr *DefiningMI;
  const DebugLoc &DL = MI.getDebugLoc();
  MachineBasicBlock &MBB = *MI.getParent();
  SmallVector<unsigned, MaxNumRepl> ZipDest;
  SmallVector<const MCInstrDesc*, MaxNumRepl> ReplInstrMCID;

  // Common code among all instructions.
  switch (MI.getOpcode()) {
  default:
    return false;
  case AArch64::ST2Twov16b:
  case AArch64::ST2Twov8b:
  case AArch64::ST2Twov8h:
  case AArch64::ST2Twov4h:
  case AArch64::ST2Twov4s:
  case AArch64::ST2Twov2s:
  case AArch64::ST2Twov2d:
    SeqReg  = MI.getOperand(0).getReg();
    AddrReg = MI.getOperand(1).getReg();
    DefiningMI = MRI->getUniqueVRegDef(SeqReg);
    if (!processSeqRegInst(DefiningMI, StReg, StRegKill, 2))
      return false;
    break;
  case AArch64::ST4Fourv16b:
  case AArch64::ST4Fourv8b:
  case AArch64::ST4Fourv8h:
  case AArch64::ST4Fourv4h:
  case AArch64::ST4Fourv4s:
  case AArch64::ST4Fourv2s:
  case AArch64::ST4Fourv2d:
    SeqReg  = MI.getOperand(0).getReg();
    AddrReg = MI.getOperand(1).getReg();
    DefiningMI = MRI->getUniqueVRegDef(SeqReg);
    if (!processSeqRegInst(DefiningMI, StReg, StRegKill, 4))
      return false;
    break;
  }

  // Specialized code
  unsigned Opcode1, Opcode2;
  const TargetRegisterClass *RC;
  AccessType AccT;
  switch (MI.getOpcode()) {
  default:
    return false;
  // ST2 case
  case AArch64::ST2Twov2d:
    Opcode1 = AArch64::ZIP1v2i64; Opcode2 = AArch64::ZIP2v2i64;
    RC = RC128; AccT = TwoVectors;
    break;
  case AArch64::ST2Twov4s:
    Opcode1 = AArch64::ZIP1v4i32; Opcode2 = AArch64::ZIP2v4i32;
    RC = RC128; AccT = TwoVectors;
    break;
  case AArch64::ST2Twov2s:
    Opcode1 = AArch64::ZIP1v2i32; Opcode2 = AArch64::ZIP2v2i32;
    RC = RC64; AccT = TwoVectors;
    break;
  case AArch64::ST2Twov8h:
    Opcode1 = AArch64::ZIP1v8i16; Opcode2 = AArch64::ZIP2v8i16;
    RC = RC128; AccT = TwoVectors;
    break;
  case AArch64::ST2Twov4h:
    Opcode1 = AArch64::ZIP1v4i16; Opcode2 = AArch64::ZIP2v4i16;
    RC = RC64; AccT = TwoVectors;
    break;
  case AArch64::ST2Twov16b:
    Opcode1 = AArch64::ZIP1v16i8; Opcode2 = AArch64::ZIP2v16i8;
    RC = RC128; AccT = TwoVectors;
    break;
  case AArch64::ST2Twov8b:
    Opcode1 = AArch64::ZIP1v8i8; Opcode2 = AArch64::ZIP2v8i8;
    RC = RC64; AccT = TwoVectors;
    break;

  // St4 case
  case AArch64::ST4Fourv2d:
    Opcode1 = AArch64::ZIP1v2i64; Opcode2 = AArch64::ZIP2v2i64;
    RC = RC128; AccT = FourVectors;
    break;
  case AArch64::ST4Fourv4s:
    Opcode1 = AArch64::ZIP1v4i32; Opcode2 = AArch64::ZIP2v4i32;
    RC = RC128; AccT = FourVectors;
    break;
  case AArch64::ST4Fourv2s:
    Opcode1 = AArch64::ZIP1v2i32; Opcode2 = AArch64::ZIP2v2i32;
    RC = RC64; AccT = FourVectors;
    break;
  case AArch64::ST4Fourv8h:
    Opcode1 = AArch64::ZIP1v8i16; Opcode2 = AArch64::ZIP2v8i16;
    RC = RC128; AccT = FourVectors;
    break;
  case AArch64::ST4Fourv4h:
    Opcode1 = AArch64::ZIP1v4i16; Opcode2 = AArch64::ZIP2v4i16;
    RC = RC64; AccT = FourVectors;
    break;
  case AArch64::ST4Fourv16b:
    Opcode1 = AArch64::ZIP1v16i8; Opcode2 = AArch64::ZIP2v16i8;
    RC = RC128; AccT = FourVectors;
    break;
  case AArch64::ST4Fourv8b:
    Opcode1 = AArch64::ZIP1v8i8; Opcode2 = AArch64::ZIP2v8i8;
    RC = RC64; AccT = FourVectors;
    break;
  }
  prepareStmtParam(Opcode1, Opcode2, RC, ReplInstrMCID, ZipDest, AccT);

  if (!shouldReplaceInst(MI.getParent()->getParent(), &TII->get(MI.getOpcode()),
                         ReplInstrMCID))
    return false;

  // Generate the replacement instructions composed of zip1, zip2, and stp
  switch (MI.getOpcode()) {
  default:
    return false;
  case AArch64::ST2Twov16b:
  case AArch64::ST2Twov8b:
  case AArch64::ST2Twov8h:
  case AArch64::ST2Twov4h:
  case AArch64::ST2Twov4s:
  case AArch64::ST2Twov2s:
  case AArch64::ST2Twov2d:
    // zip instructions
    BuildMI(MBB, MI, DL, *ReplInstrMCID[0], ZipDest[0])
        .addReg(StReg[0])
        .addReg(StReg[1]);
    BuildMI(MBB, MI, DL, *ReplInstrMCID[1], ZipDest[1])
        .addReg(StReg[0], StRegKill[0])
        .addReg(StReg[1], StRegKill[1]);
    // stp instructions
    BuildMI(MBB, MI, DL, *ReplInstrMCID[2])
        .addReg(ZipDest[0])
        .addReg(ZipDest[1])
        .addReg(AddrReg)
        .addImm(0);
    break;
  case AArch64::ST4Fourv16b:
  case AArch64::ST4Fourv8b:
  case AArch64::ST4Fourv8h:
  case AArch64::ST4Fourv4h:
  case AArch64::ST4Fourv4s:
  case AArch64::ST4Fourv2s:
  case AArch64::ST4Fourv2d:
    // zip instructions
    BuildMI(MBB, MI, DL, *ReplInstrMCID[0], ZipDest[0])
        .addReg(StReg[0])
        .addReg(StReg[2]);
    BuildMI(MBB, MI, DL, *ReplInstrMCID[1], ZipDest[1])
        .addReg(StReg[0], StRegKill[0])
        .addReg(StReg[2], StRegKill[2]);
    BuildMI(MBB, MI, DL, *ReplInstrMCID[2], ZipDest[2])
        .addReg(StReg[1])
        .addReg(StReg[3]);
    BuildMI(MBB, MI, DL, *ReplInstrMCID[3], ZipDest[3])
        .addReg(StReg[1], StRegKill[1])
        .addReg(StReg[3], StRegKill[3]);
    BuildMI(MBB, MI, DL, *ReplInstrMCID[4], ZipDest[4])
        .addReg(ZipDest[0])
        .addReg(ZipDest[2]);
    BuildMI(MBB, MI, DL, *ReplInstrMCID[5], ZipDest[5])
        .addReg(ZipDest[0])
        .addReg(ZipDest[2]);
    BuildMI(MBB, MI, DL, *ReplInstrMCID[6], ZipDest[6])
        .addReg(ZipDest[1])
        .addReg(ZipDest[3]);
    BuildMI(MBB, MI, DL, *ReplInstrMCID[7], ZipDest[7])
        .addReg(ZipDest[1])
        .addReg(ZipDest[3]);
    // stp instructions
    BuildMI(MBB, MI, DL, *ReplInstrMCID[8])
        .addReg(ZipDest[4])
        .addReg(ZipDest[5])
        .addReg(AddrReg)
        .addImm(0);
    BuildMI(MBB, MI, DL, *ReplInstrMCID[9])
        .addReg(ZipDest[6])
        .addReg(ZipDest[7])
        .addReg(AddrReg)
        .addImm(2);
    break;
  }

  ++NumModifiedInstr;
  return true;
}

/// Process The REG_SEQUENCE instruction, and extract the source
/// operands of the st2/4 instruction from it.
/// Example of such instruction.
///    %dest = REG_SEQUENCE %st2_src1, dsub0, %st2_src2, dsub1;
/// Return true when the instruction is processed successfully.
bool AArch64SIMDInstrOpt::processSeqRegInst(MachineInstr *DefiningMI,
     unsigned* StReg, unsigned* StRegKill, unsigned NumArg) const {
  assert (DefiningMI != NULL);
  if (DefiningMI->getOpcode() != AArch64::REG_SEQUENCE)
    return false;

  for (unsigned i=0; i<NumArg; i++) {
    StReg[i]     = DefiningMI->getOperand(2*i+1).getReg();
    StRegKill[i] = getKillRegState(DefiningMI->getOperand(2*i+1).isKill());

    // Sanity check for the other arguments.
    if (DefiningMI->getOperand(2*i+2).isImm()) {
      switch (DefiningMI->getOperand(2*i+2).getImm()) {
      default:
        return false;
      case AArch64::dsub0:
      case AArch64::dsub1:
      case AArch64::dsub2:
      case AArch64::dsub3:
      case AArch64::qsub0:
      case AArch64::qsub1:
      case AArch64::qsub2:
      case AArch64::qsub3:
        break;
      }
    }
    else
      return false;
  }
  return true;
}

/// Prepare the parameters needed to build the replacement statements
void AArch64SIMDInstrOpt::prepareStmtParam(unsigned Opcode1,
     unsigned Opcode2, const TargetRegisterClass *RC,
     SmallVectorImpl<const MCInstrDesc*> &ReplInstrMCID,
     SmallVectorImpl<unsigned>& ZipDest, AccessType AccT) const {
  switch(AccT) {
  case TwoVectors:
    ReplInstrMCID.push_back(&TII->get(Opcode1));
    ZipDest.push_back(MRI->createVirtualRegister(RC));
    ReplInstrMCID.push_back(&TII->get(Opcode2));
    ZipDest.push_back(MRI->createVirtualRegister(RC));
    if (RC == &AArch64::FPR128RegClass)
      ReplInstrMCID.push_back(&TII->get(AArch64::STPQi));
    else
      ReplInstrMCID.push_back(&TII->get(AArch64::STPDi));
    break;
  case FourVectors:
    for (int i=0; i<4; i++) {
       ReplInstrMCID.push_back(&TII->get(Opcode1));
       ZipDest.push_back(MRI->createVirtualRegister(RC));
       ReplInstrMCID.push_back(&TII->get(Opcode2));
       ZipDest.push_back(MRI->createVirtualRegister(RC));
    }
    if (RC == &AArch64::FPR128RegClass) {
      ReplInstrMCID.push_back(&TII->get(AArch64::STPQi));
      ReplInstrMCID.push_back(&TII->get(AArch64::STPQi));
    }
    else {
      ReplInstrMCID.push_back(&TII->get(AArch64::STPDi));
      ReplInstrMCID.push_back(&TII->get(AArch64::STPDi));
    }
    break;
  }
}

bool AArch64SIMDInstrOpt::runOnMachineFunction(MachineFunction &MF) {
  if (skipFunction(*MF.getFunction()))
    return false;

  TII = MF.getSubtarget().getInstrInfo();
  MRI = &MF.getRegInfo();
  const TargetSubtargetInfo &ST = MF.getSubtarget();
  const AArch64InstrInfo *AAII =
      static_cast<const AArch64InstrInfo *>(ST.getInstrInfo());
  if (!AAII)
    return false;
  SchedModel.init(ST.getSchedModel(), &ST, AAII);
  if (!SchedModel.hasInstrSchedModel())
    return false;

  bool Changed = false;
  for (auto OptimizationKind : {VectorElem, Interleave}) {
    if (!shouldExitEarly(&MF, OptimizationKind)) {
      SmallVector<MachineInstr *, 8> RemoveMIs;
      for (MachineBasicBlock &MBB : MF) {
        for (MachineBasicBlock::iterator MII = MBB.begin(), MIE = MBB.end();
             MII != MIE;) {
          MachineInstr &MI = *MII;
          bool InstRewrite;
          if (OptimizationKind == VectorElem)
            InstRewrite = optimizeVectElement(MI) ;
          else
            InstRewrite = optimizeLdStInterleave(MI);
          if (InstRewrite) {
            // Add MI to the list of instructions to be removed given that it
            // has been replaced.
            RemoveMIs.push_back(&MI);
            Changed = true;
          }
          ++MII;
        }
      }
      for (MachineInstr *MI : RemoveMIs)
        MI->eraseFromParent();
    }
	}

  return Changed;
}

/// createAArch64SIMDInstrOptPass - returns an instance of the
/// vector by element optimization pass.
FunctionPass *llvm::createAArch64SIMDInstrOptPass() {
  return new AArch64SIMDInstrOpt();
}
