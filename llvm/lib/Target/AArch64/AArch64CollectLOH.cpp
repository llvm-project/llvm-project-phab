//===---------- AArch64CollectLOH.cpp - AArch64 collect LOH pass --*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a pass that collect the Linker Optimization Hint (LOH).
// This pass should be run at the very end of the compilation flow, just before
// assembly printer.
// To be useful for the linker, the LOH must be printed into the assembly file.
//
// A LOH describes a sequence of instructions that may be optimized by the
// linker.
// This same sequence cannot be optimized by the compiler because some of
// the information will be known at link time.
// For instance, consider the following sequence:
//     L1: adrp xA, sym@PAGE
//     L2: add xB, xA, sym@PAGEOFF
//     L3: ldr xC, [xB, #imm]
// This sequence can be turned into:
// A literal load if sym@PAGE + sym@PAGEOFF + #imm - address(L3) is < 1MB:
//     L3: ldr xC, sym+#imm
// It may also be turned into either the following more efficient
// code sequences:
// - If sym@PAGEOFF + #imm fits the encoding space of L3.
//     L1: adrp xA, sym@PAGE
//     L3: ldr xC, [xB, sym@PAGEOFF + #imm]
// - If sym@PAGE + sym@PAGEOFF - address(L1) < 1MB:
//     L1: adr xA, sym
//     L3: ldr xC, [xB, #imm]
//
// To be valid a LOH must meet all the requirements needed by all the related
// possible linker transformations.
// For instance, using the running example, the constraints to emit
// ".loh AdrpAddLdr" are:
// - L1, L2, and L3 instructions are of the expected type, i.e.,
//   respectively ADRP, ADD (immediate), and LD.
// - The result of L1 is used only by L2.
// - The register argument (xA) used in the ADD instruction is defined
//   only by L1.
// - The result of L2 is used only by L3.
// - The base address (xB) in L3 is defined only L2.
// - The ADRP in L1 and the ADD in L2 must reference the same symbol using
//   @PAGE/@PAGEOFF with no additional constants
//
// Currently supported LOHs are:
// * So called non-ADRP-related:
//   - .loh AdrpAddLdr L1, L2, L3:
//     L1: adrp xA, sym@PAGE
//     L2: add xB, xA, sym@PAGEOFF
//     L3: ldr xC, [xB, #imm]
//   - .loh AdrpLdrGotLdr L1, L2, L3:
//     L1: adrp xA, sym@GOTPAGE
//     L2: ldr xB, [xA, sym@GOTPAGEOFF]
//     L3: ldr xC, [xB, #imm]
//   - .loh AdrpLdr L1, L3:
//     L1: adrp xA, sym@PAGE
//     L3: ldr xC, [xA, sym@PAGEOFF]
//   - .loh AdrpAddStr L1, L2, L3:
//     L1: adrp xA, sym@PAGE
//     L2: add xB, xA, sym@PAGEOFF
//     L3: str xC, [xB, #imm]
//   - .loh AdrpLdrGotStr L1, L2, L3:
//     L1: adrp xA, sym@GOTPAGE
//     L2: ldr xB, [xA, sym@GOTPAGEOFF]
//     L3: str xC, [xB, #imm]
//   - .loh AdrpAdd L1, L2:
//     L1: adrp xA, sym@PAGE
//     L2: add xB, xA, sym@PAGEOFF
//   For all these LOHs, L1, L2, L3 form a simple chain:
//   L1 result is used only by L2 and L2 result by L3.
//   L3 LOH-related argument is defined only by L2 and L2 LOH-related argument
//   by L1.
// All these LOHs aim at using more efficient load/store patterns by folding
// some instructions used to compute the address directly into the load/store.
//
// * So called ADRP-related:
//  - .loh AdrpAdrp L2, L1:
//    L2: ADRP xA, sym1@PAGE
//    L1: ADRP xA, sym2@PAGE
//    L2 dominates L1 and xA is not redifined between L2 and L1
// This LOH aims at getting rid of redundant ADRP instructions.
//
// The overall design for emitting the LOHs is:
// 1. AArch64CollectLOH (this pass) records the LOHs in the AArch64FunctionInfo.
// 2. AArch64AsmPrinter reads the LOHs from AArch64FunctionInfo and it:
//     1. Associates them a label.
//     2. Emits them in a MCStreamer (EmitLOHDirective).
//         - The MCMachOStreamer records them into the MCAssembler.
//         - The MCAsmStreamer prints them.
//         - Other MCStreamers ignore them.
//     3. Closes the MCStreamer:
//         - The MachObjectWriter gets them from the MCAssembler and writes
//           them in the object file.
//         - Other ObjectWriters ignore them.
//===----------------------------------------------------------------------===//

#include "AArch64.h"
#include "AArch64InstrInfo.h"
#include "AArch64MachineFunctionInfo.h"
#include "AArch64Subtarget.h"
#include "MCTargetDesc/AArch64AddressingModes.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/ReachingPhysDefs.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetRegisterInfo.h"
using namespace llvm;

#define DEBUG_TYPE "reachingdefs"

static cl::opt<bool>
PreCollectRegister("aarch64-collect-loh-pre-collect-register", cl::Hidden,
                   cl::desc("Restrict analysis to registers invovled"
                            " in LOHs"),
                   cl::init(true));

static cl::opt<bool>
BasicBlockScopeOnly("aarch64-collect-loh-bb-only", cl::Hidden,
                    cl::desc("Restrict analysis at basic block scope"),
                    cl::init(true));

STATISTIC(NumADRPSimpleCandidate,
          "Number of simplifiable ADRP dominate by another");
STATISTIC(NumADRPComplexCandidate2,
          "Number of simplifiable ADRP reachable by 2 defs");
STATISTIC(NumADRPComplexCandidate3,
          "Number of simplifiable ADRP reachable by 3 defs");
STATISTIC(NumADRPComplexCandidateOther,
          "Number of simplifiable ADRP reachable by 4 or more defs");
STATISTIC(NumADDToSTRWithImm,
          "Number of simplifiable STR with imm reachable by ADD");
STATISTIC(NumLDRToSTRWithImm,
          "Number of simplifiable STR with imm reachable by LDR");
STATISTIC(NumADDToSTR, "Number of simplifiable STR reachable by ADD");
STATISTIC(NumLDRToSTR, "Number of simplifiable STR reachable by LDR");
STATISTIC(NumADDToLDRWithImm,
          "Number of simplifiable LDR with imm reachable by ADD");
STATISTIC(NumLDRToLDRWithImm,
          "Number of simplifiable LDR with imm reachable by LDR");
STATISTIC(NumADDToLDR, "Number of simplifiable LDR reachable by ADD");
STATISTIC(NumLDRToLDR, "Number of simplifiable LDR reachable by LDR");
STATISTIC(NumADRPToLDR, "Number of simplifiable LDR reachable by ADRP");
STATISTIC(NumCplxLvl1, "Number of complex case of level 1");
STATISTIC(NumTooCplxLvl1, "Number of too complex case of level 1");
STATISTIC(NumCplxLvl2, "Number of complex case of level 2");
STATISTIC(NumTooCplxLvl2, "Number of too complex case of level 2");
STATISTIC(NumADRSimpleCandidate, "Number of simplifiable ADRP + ADD");
STATISTIC(NumADRComplexCandidate, "Number of too complex ADRP + ADD");

namespace llvm {
void initializeAArch64CollectLOHPass(PassRegistry &);
}

#define AARCH64_COLLECT_LOH_NAME "AArch64 Collect Linker Optimization Hint (LOH)"

namespace {
struct AArch64CollectLOH : public MachineFunctionPass {
  static char ID;
  AArch64CollectLOH() : MachineFunctionPass(ID) {
    initializeAArch64CollectLOHPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

  MachineFunctionProperties getRequiredProperties() const override {
    return MachineFunctionProperties().set(
        MachineFunctionProperties::Property::AllVRegsAllocated);
  }

  const char *getPassName() const override {
    return AARCH64_COLLECT_LOH_NAME;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
    MachineFunctionPass::getAnalysisUsage(AU);
    AU.addRequired<MachineDominatorTree>();
  }

private:
};


} // end anonymous namespace.

char AArch64CollectLOH::ID = 0;

INITIALIZE_PASS_BEGIN(AArch64CollectLOH, "aarch64-collect-loh",
                      AARCH64_COLLECT_LOH_NAME, false, false)
INITIALIZE_PASS_DEPENDENCY(MachineDominatorTree)
INITIALIZE_PASS_END(AArch64CollectLOH, "aarch64-collect-loh",
                    AARCH64_COLLECT_LOH_NAME, false, false)


// A class to compute the reaching def in ADRP mode, meaning ADRP
// definitions are first considered as uses.
class ReachingPhysDefs_ADRP : public ReachingPhysDefs {
  unsigned processMIUseMO(MachineOperand &MO) override;
  bool rememberClobberingMI() override { return true; }
  void reachedUsesToDefs() override;
  bool recordDummyUses() override { return false; }

public:
  ReachingPhysDefs_ADRP(MachineFunction *MF_, const TargetRegisterInfo *TRI_,
                        bool BasicBlockScopeOnly)
    : ReachingPhysDefs(MF_, TRI_, BasicBlockScopeOnly,
                       "ADRP reaching defs") {}
  void addRegs();
  InstrToInstrs &getUseToDefs() { return UseToDefs; }
};

class ReachingPhysDefs_Others : public ReachingPhysDefs {
  void reachedUsesToDefs() override;

public:
  ReachingPhysDefs_Others(MachineFunction *MF_, const TargetRegisterInfo *TRI_,
                        bool BasicBlockScopeOnly)
    : ReachingPhysDefs(MF_, TRI_, BasicBlockScopeOnly,
                       "All reaching defs") {}
  void addRegs();

  InstrToInstrs &getUseToDefs() { return UseToDefs; }

  /// Returns the first definition in UseToDefs for MI.
  MachineInstr *getDef(MachineInstr *MI) {
    return *UseToDefs.find(MI)->second.begin();
  }
};

/// Answer the following question: Can Def be one of the definition
/// involved in a part of a LOH?
static bool canDefBePartOfLOH(const MachineInstr *Def) {
  unsigned Opc = Def->getOpcode();
  // Accept ADRP, ADDLow and LOADGot.
  switch (Opc) {
  default:
    return false;
  case AArch64::ADRP:
    return true;
  case AArch64::ADDXri:
    // Check immediate to see if the immediate is an address.
    switch (Def->getOperand(2).getType()) {
    default:
      return false;
    case MachineOperand::MO_GlobalAddress:
    case MachineOperand::MO_JumpTableIndex:
    case MachineOperand::MO_ConstantPoolIndex:
    case MachineOperand::MO_BlockAddress:
      return true;
    }
  case AArch64::LDRXui:
    // Check immediate to see if the immediate is an address.
    switch (Def->getOperand(2).getType()) {
    default:
      return false;
    case MachineOperand::MO_GlobalAddress:
      return true;
    }
  }
  // Unreachable.
  return false;
}

/// Check whether the given instruction can be the end of a LOH chain
/// involving a load.
static bool isCandidateLoad(const MachineInstr *Instr) {
  switch (Instr->getOpcode()) {
  default:
    return false;
  case AArch64::LDRSBWui:
  case AArch64::LDRSBXui:
  case AArch64::LDRSHWui:
  case AArch64::LDRSHXui:
  case AArch64::LDRSWui:
  case AArch64::LDRBui:
  case AArch64::LDRHui:
  case AArch64::LDRWui:
  case AArch64::LDRXui:
  case AArch64::LDRSui:
  case AArch64::LDRDui:
  case AArch64::LDRQui:
    if (Instr->getOperand(2).getTargetFlags() & AArch64II::MO_GOT)
      return false;
    return true;
  }
  // Unreachable.
  return false;
}

/// Check whether the given instruction can the end of a LOH chain involving a
/// store.
static bool isCandidateStore(const MachineInstr *Instr) {
  switch (Instr->getOpcode()) {
  default:
    return false;
  case AArch64::STRBBui:
  case AArch64::STRHHui:
  case AArch64::STRBui:
  case AArch64::STRHui:
  case AArch64::STRWui:
  case AArch64::STRXui:
  case AArch64::STRSui:
  case AArch64::STRDui:
  case AArch64::STRQui:
    // In case we have str xA, [xA, #imm], this is two different uses
    // of xA and we cannot fold, otherwise the xA stored may be wrong,
    // even if #imm == 0.
    if (Instr->getOperand(0).getReg() != Instr->getOperand(1).getReg())
      return true;
  }
  return false;
}

/// Look for every register defined by potential LOHs candidates.
static void collectLOHInvolvedRegs(ReachingPhysDefs &ReachingDefs,
                                   MachineFunction *MF) {
  const TargetRegisterInfo *TRI = MF->getSubtarget().getRegisterInfo();

  DEBUG(dbgs() << "** Collect Involved Register\n");
  for (const auto &MBB : *MF) {
    for (const MachineInstr &MI : MBB) {
      if (!canDefBePartOfLOH(&MI) &&
          !isCandidateLoad(&MI) && !isCandidateStore(&MI))
        continue;

      // Process defs
      for (MachineInstr::const_mop_iterator IO = MI.operands_begin(),
                                            IOEnd = MI.operands_end();
           IO != IOEnd; ++IO) {
        if (!IO->isReg() || !IO->isDef())
          continue;
        unsigned CurReg = IO->getReg();
        for (MCRegAliasIterator AI(CurReg, TRI, true);
             AI.isValid(); ++AI)
          if (!ReachingDefs.isRegInMaps(*AI)) {
            ReachingDefs.addReg(*AI);
            DEBUG(dbgs() << "Register: " << PrintReg(*AI, TRI)
                  << '\n');
          }
      }
    }
  }
}

void ReachingPhysDefs_ADRP::addRegs() {
  if (!PreCollectRegister) {
    ReachingPhysDefs::addAllTargetRegs();
    return;
  }

  collectLOHInvolvedRegs(*this, MF);
}

unsigned ReachingPhysDefs_ADRP::processMIUseMO(MachineOperand &MO) {
  // Only handle ADRP instructions, and treat ADRP def as use, as the
  // goal of the analysis is to find ADRP defs reached by other ADRP
  // defs.
  if (MO.getParent()->getOpcode() != AArch64::ADRP || !MO.isReg() || !MO.isDef())
    return 0;
  return MO.getReg();
}

void ReachingPhysDefs_ADRP::reachedUsesToDefs() {
  SetOfMachineInstr NotCandidate;
  MapRegToId::const_iterator EndIt = RegToId.end();
  for (unsigned CurReg = 0; CurReg < nbRegs(); ++CurReg) {
    // If this color is never defined, continue.
    if (ColorOpToReachedUses[CurReg].empty())
      continue;

    for (const auto &DefsIt : ColorOpToReachedUses[CurReg]) {
      for (MachineInstr *MI : DefsIt.second) {
        const MachineInstr *Def = DefsIt.first;
        MapRegToId::const_iterator It;
        // if all the reaching defs are not adrp, this use will not be
        // simplifiable.
        if (Def->getOpcode() != AArch64::ADRP) {
          NotCandidate.insert(MI);
          continue;
        }
        // Do not consider self reaching as a simplifiable case for ADRP.
        if (MI != DefsIt.first)
          UseToDefs[MI].insert(DefsIt.first);
      }
    }
  }
  for (MachineInstr *Elem : NotCandidate) {
    DEBUG(dbgs() << "Too many reaching defs: " << *Elem << "\n");
    // It would have been better if we could just remove the entry
    // from the map.  Because of that, we have to filter the garbage
    // (second.empty) in the subsequence analysis.
    UseToDefs[Elem].clear();
  }
}

void ReachingPhysDefs_Others::addRegs() {
  if (!PreCollectRegister) {
    ReachingPhysDefs::addAllTargetRegs();
    return;
  }

  collectLOHInvolvedRegs(*this, MF);
}

void ReachingPhysDefs_Others::reachedUsesToDefs() {
  SetOfMachineInstr NotCandidate;
  unsigned NbReg = RegToId.size();
  MapRegToId::const_iterator EndIt = RegToId.end();
  for (unsigned CurReg = 0; CurReg < NbReg; ++CurReg) {
    // If this color is never defined, continue.
    if (ColorOpToReachedUses[CurReg].empty())
      continue;

    for (const auto &DefsIt : ColorOpToReachedUses[CurReg]) {
      for (MachineInstr *MI : DefsIt.second) {
        MachineInstr *Def = DefsIt.first;
        MapRegToId::const_iterator It;
        // if all the reaching defs are not adrp, this use will not be
        // simplifiable.
        if (!canDefBePartOfLOH(Def) ||
            (isCandidateStore(MI) &&
             // store are LOH candidate iff the end of the chain is used as
             // base.
             ((It = RegToId.find((MI)->getOperand(1).getReg())) == EndIt ||
              It->second != CurReg))) {
          NotCandidate.insert(MI);
          continue;
        }
        UseToDefs[MI].insert(DefsIt.first);
        // If UsesIt has several reaching definitions, it is not
        // candidate for simplificaton in non-ADRPMode.
        if (UseToDefs[MI].size() > 1)
          NotCandidate.insert(MI);
      }
    }
  }
  for (MachineInstr *Elem : NotCandidate) {
    DEBUG(dbgs() << "Too many reaching defs: " << *Elem << "\n");
    // It would have been better if we could just remove the entry
    // from the map.  Because of that, we have to filter the garbage
    // (second.empty) in the subsequence analysis.
    UseToDefs[Elem].clear();
  }
}



/// Based on the use to defs information (in ADRPMode), compute the
/// opportunities of LOH ADRP-related.
static void computeADRP(ReachingPhysDefs_ADRP &ReachingDefs_ADRP,
                        AArch64FunctionInfo &AArch64FI,
                        const MachineDominatorTree *MDT) {
  DEBUG(dbgs() << "*** Compute LOH for ADRP\n");
  for (const auto &Entry : ReachingDefs_ADRP.getUseToDefs()) {
    unsigned Size = Entry.second.size();
    if (Size == 0)
      continue;
    if (Size == 1) {
      const MachineInstr *L2 = *Entry.second.begin();
      const MachineInstr *L1 = Entry.first;
      if (!MDT->dominates(L2, L1)) {
        DEBUG(dbgs() << "Dominance check failed:\n" << *L2 << '\n' << *L1
                     << '\n');
        continue;
      }
      DEBUG(dbgs() << "Record AdrpAdrp:\n" << *L2 << '\n' << *L1 << '\n');
      SmallVector<const MachineInstr *, 2> Args;
      Args.push_back(L2);
      Args.push_back(L1);
      AArch64FI.addLOHDirective(MCLOH_AdrpAdrp, Args);
      ++NumADRPSimpleCandidate;
    }
#ifdef DEBUG
    else if (Size == 2)
      ++NumADRPComplexCandidate2;
    else if (Size == 3)
      ++NumADRPComplexCandidate3;
    else
      ++NumADRPComplexCandidateOther;
#endif
    // if Size < 1, the use should have been removed from the candidates
    assert(Size >= 1 && "No reaching defs for that use!");
  }
}

/// Check whether the given instruction can load a litteral.
static bool supportLoadFromLiteral(const MachineInstr *Instr) {
  switch (Instr->getOpcode()) {
  default:
    return false;
  case AArch64::LDRSWui:
  case AArch64::LDRWui:
  case AArch64::LDRXui:
  case AArch64::LDRSui:
  case AArch64::LDRDui:
  case AArch64::LDRQui:
    return true;
  }
  // Unreachable.
  return false;
}

/// Check whether the given instruction is a LOH candidate.
/// \param UseToDefs is used to check that Instr is at the end of LOH supported
/// chain.
/// \pre UseToDefs contains only on def per use, i.e., obvious non candidate are
/// already been filtered out.
static bool isCandidate(MachineInstr *Instr,
                        const ReachingPhysDefs::InstrToInstrs &UseToDefs,
                        const MachineDominatorTree *MDT) {
  if (!isCandidateLoad(Instr) && !isCandidateStore(Instr))
    return false;

  MachineInstr *Def = *UseToDefs.find(Instr)->second.begin();
  if (Def->getOpcode() != AArch64::ADRP) {
    // At this point, Def is ADDXri or LDRXui of the right type of
    // symbol, because we filtered out the uses that were not defined
    // by these kind of instructions (+ ADRP).

    // Check if this forms a simple chain: each intermediate node must
    // dominates the next one.
    if (!MDT->dominates(Def, Instr))
      return false;
    // Move one node up in the simple chain.
    if (UseToDefs.find(Def) ==
            UseToDefs.end()
            // The map may contain garbage we have to ignore.
        ||
        UseToDefs.find(Def)->second.empty())
      return false;
    Instr = Def;
    Def = *UseToDefs.find(Def)->second.begin();
  }
  // Check if we reached the top of the simple chain:
  // - top is ADRP.
  // - check the simple chain property: each intermediate node must
  // dominates the next one.
  if (Def->getOpcode() == AArch64::ADRP)
    return MDT->dominates(Def, Instr);
  return false;
}

static bool registerADRCandidate(MachineInstr &Use,
                                 ReachingPhysDefs_Others &ReachingDefs,
                                 AArch64FunctionInfo &AArch64FI,
                                 SetOfMachineInstr *InvolvedInLOHs) {
  // Look for opportunities to turn ADRP -> ADD or
  // ADRP -> LDR GOTPAGEOFF into ADR.
  // If ADRP has more than one use. Give up.
  if (Use.getOpcode() != AArch64::ADDXri &&
      (Use.getOpcode() != AArch64::LDRXui ||
       !(Use.getOperand(2).getTargetFlags() & AArch64II::MO_GOT)))
    return false;
  ReachingPhysDefs::InstrToInstrs::const_iterator
    It = ReachingDefs.getUseToDefs().find(&Use);

  // The map may contain garbage that we need to ignore.
  if (It == ReachingDefs.getUseToDefs().end() || It->second.empty())
    return false;
  MachineInstr &Def = **It->second.begin();
  if (Def.getOpcode() != AArch64::ADRP)
    return false;
  // Check the number of users of ADRP.
  const SetOfMachineInstr *Users =
    ReachingDefs.getUses(Def.getOperand(0).getReg(), Def);
  if (Users->size() > 1) {
    ++NumADRComplexCandidate;
    return false;
  }
  ++NumADRSimpleCandidate;
  assert((!InvolvedInLOHs || InvolvedInLOHs->insert(&Def)) &&
         "ADRP already involved in LOH.");
  assert((!InvolvedInLOHs || InvolvedInLOHs->insert(&Use)) &&
         "ADD already involved in LOH.");
  DEBUG(dbgs() << "Record AdrpAdd\n" << Def << '\n' << Use << '\n');

  SmallVector<const MachineInstr *, 2> Args;
  Args.push_back(&Def);
  Args.push_back(&Use);

  AArch64FI.addLOHDirective(Use.getOpcode() == AArch64::ADDXri ? MCLOH_AdrpAdd
                                                           : MCLOH_AdrpLdrGot,
                          Args);
  return true;
}

/// Based on the use to defs information (in non-ADRPMode), compute the
/// opportunities of LOH non-ADRP-related
static void computeOthers(ReachingPhysDefs_Others &ReachingDefs,
                          AArch64FunctionInfo &AArch64FI,
                          const MachineDominatorTree *MDT) {
  SetOfMachineInstr *InvolvedInLOHs = nullptr;
#ifdef DEBUG
  SetOfMachineInstr InvolvedInLOHsStorage;
  InvolvedInLOHs = &InvolvedInLOHsStorage;
#endif // DEBUG
  DEBUG(dbgs() << "*** Compute LOH for Others\n");
  // ADRP -> ADD/LDR -> LDR/STR pattern.
  // Fall back to ADRP -> ADD pattern if we fail to catch the bigger pattern.

  // FIXME: When the statistics are not important,
  // This initial filtering loop can be merged into the next loop.
  // Currently, we didn't do it to have the same code for both DEBUG and
  // NDEBUG builds. Indeed, the iterator of the second loop would need
  // to be changed.
  SetOfMachineInstr PotentialCandidates;
  SetOfMachineInstr PotentialADROpportunities;
  for (auto &Use : ReachingDefs.getUseToDefs()) {
    // If no definition is available, this is a non candidate.
    if (Use.second.empty())
      continue;
    // Keep only instructions that are load or store and at the end of
    // a ADRP -> ADD/LDR/Nothing chain.
    // We already filtered out the no-chain cases.
    if (!isCandidate(Use.first, ReachingDefs.getUseToDefs(), MDT)) {
      PotentialADROpportunities.insert(Use.first);
      continue;
    }
    PotentialCandidates.insert(Use.first);
  }

  // Make the following distinctions for statistics as the linker does
  // know how to decode instructions:
  // - ADD/LDR/Nothing make there different patterns.
  // - LDR/STR make two different patterns.
  // Hence, 6 - 1 base patterns.
  // (because ADRP-> Nothing -> STR is not simplifiable)

  // The linker is only able to have a simple semantic, i.e., if pattern A
  // do B.
  // However, we want to see the opportunity we may miss if we were able to
  // catch more complex cases.

  // PotentialCandidates are result of a chain ADRP -> ADD/LDR ->
  // A potential candidate becomes a candidate, if its current immediate
  // operand is zero and all nodes of the chain have respectively only one user
#ifdef DEBUG
  SetOfMachineInstr DefsOfPotentialCandidates;
#endif
  for (MachineInstr *Candidate : PotentialCandidates) {
    // Get the definition of the candidate i.e., ADD or LDR.
    MachineInstr *Def = ReachingDefs.getDef(Candidate);
    // Record the elements of the chain.
    MachineInstr *L1 = Def;
    MachineInstr *L2 = nullptr;
    unsigned ImmediateDefOpc = Def->getOpcode();
    if (Def->getOpcode() != AArch64::ADRP) {
      // Check the number of users of this node.
      const SetOfMachineInstr *Users =
        ReachingDefs.getUses(Def->getOperand(0).getReg(), *Def);
      if (Users->size() > 1) {
#ifdef DEBUG
        // if all the uses of this def are in potential candidate, this is
        // a complex candidate of level 2.
        bool IsLevel2 = true;
        for (MachineInstr *MI : *Users) {
          if (!PotentialCandidates.count(MI)) {
            ++NumTooCplxLvl2;
            IsLevel2 = false;
            break;
          }
        }
        if (IsLevel2)
          ++NumCplxLvl2;
#endif // DEBUG
        PotentialADROpportunities.insert(Def);
        continue;
      }
      L2 = Def;
      Def = *ReachingDefs.getUseToDefs().find(Def)->second.begin();
      L1 = Def;
    } // else the element in the middle of the chain is nothing, thus
      // Def already contains the first element of the chain.

    // Check the number of users of the first node in the chain, i.e., ADRP
    const SetOfMachineInstr *Users =
      ReachingDefs.getUses(Def->getOperand(0).getReg(), *Def);
    if (Users->size() > 1) {
#ifdef DEBUG
      // if all the uses of this def are in the defs of the potential candidate,
      // this is a complex candidate of level 1
      if (DefsOfPotentialCandidates.empty()) {
        // lazy init
        DefsOfPotentialCandidates = PotentialCandidates;
        for (MachineInstr *Candidate : PotentialCandidates) {
          if (!ReachingDefs.getUseToDefs().find(Candidate)->second.empty())
            DefsOfPotentialCandidates.insert(
                *ReachingDefs.getUseToDefs().find(Candidate)->second.begin());
        }
      }
      bool Found = false;
      for (auto &Use : *Users) {
        if (!DefsOfPotentialCandidates.count(Use)) {
          ++NumTooCplxLvl1;
          Found = true;
          break;
        }
      }
      if (!Found)
        ++NumCplxLvl1;
#endif // DEBUG
      continue;
    }

    bool IsL2Add = (ImmediateDefOpc == AArch64::ADDXri);
    // If the chain is three instructions long and ldr is the second element,
    // then this ldr must load form GOT, otherwise this is not a correct chain.
    if (L2 && !IsL2Add &&
        !(L2->getOperand(2).getTargetFlags() & AArch64II::MO_GOT))
      continue;
    SmallVector<const MachineInstr *, 3> Args;
    MCLOHType Kind;
    if (isCandidateLoad(Candidate)) {
      if (!L2) {
        // At this point, the candidate LOH indicates that the ldr instruction
        // may use a direct access to the symbol. There is not such encoding
        // for loads of byte and half.
        if (!supportLoadFromLiteral(Candidate))
          continue;

        DEBUG(dbgs() << "Record AdrpLdr:\n" << *L1 << '\n' << *Candidate
                     << '\n');
        Kind = MCLOH_AdrpLdr;
        Args.push_back(L1);
        Args.push_back(Candidate);
        assert((!InvolvedInLOHs || InvolvedInLOHs->insert(L1)) &&
               "L1 already involved in LOH.");
        assert((!InvolvedInLOHs || InvolvedInLOHs->insert(Candidate)) &&
               "Candidate already involved in LOH.");
        ++NumADRPToLDR;
      } else {
        DEBUG(dbgs() << "Record Adrp" << (IsL2Add ? "Add" : "LdrGot")
                     << "Ldr:\n" << *L1 << '\n' << *L2 << '\n' << *Candidate
                     << '\n');

        Kind = IsL2Add ? MCLOH_AdrpAddLdr : MCLOH_AdrpLdrGotLdr;
        Args.push_back(L1);
        Args.push_back(L2);
        Args.push_back(Candidate);

        PotentialADROpportunities.remove(L2);
        assert((!InvolvedInLOHs || InvolvedInLOHs->insert(L1)) &&
               "L1 already involved in LOH.");
        assert((!InvolvedInLOHs || InvolvedInLOHs->insert(L2)) &&
               "L2 already involved in LOH.");
        assert((!InvolvedInLOHs || InvolvedInLOHs->insert(Candidate)) &&
               "Candidate already involved in LOH.");
#ifdef DEBUG
        // get the immediate of the load
        if (Candidate->getOperand(2).getImm() == 0)
          if (ImmediateDefOpc == AArch64::ADDXri)
            ++NumADDToLDR;
          else
            ++NumLDRToLDR;
        else if (ImmediateDefOpc == AArch64::ADDXri)
          ++NumADDToLDRWithImm;
        else
          ++NumLDRToLDRWithImm;
#endif // DEBUG
      }
    } else {
      if (ImmediateDefOpc == AArch64::ADRP)
        continue;
      else {

        DEBUG(dbgs() << "Record Adrp" << (IsL2Add ? "Add" : "LdrGot")
                     << "Str:\n" << *L1 << '\n' << *L2 << '\n' << *Candidate
                     << '\n');

        Kind = IsL2Add ? MCLOH_AdrpAddStr : MCLOH_AdrpLdrGotStr;
        Args.push_back(L1);
        Args.push_back(L2);
        Args.push_back(Candidate);

        PotentialADROpportunities.remove(L2);
        assert((!InvolvedInLOHs || InvolvedInLOHs->insert(L1)) &&
               "L1 already involved in LOH.");
        assert((!InvolvedInLOHs || InvolvedInLOHs->insert(L2)) &&
               "L2 already involved in LOH.");
        assert((!InvolvedInLOHs || InvolvedInLOHs->insert(Candidate)) &&
               "Candidate already involved in LOH.");
#ifdef DEBUG
        // get the immediate of the store
        if (Candidate->getOperand(2).getImm() == 0)
          if (ImmediateDefOpc == AArch64::ADDXri)
            ++NumADDToSTR;
          else
            ++NumLDRToSTR;
        else if (ImmediateDefOpc == AArch64::ADDXri)
          ++NumADDToSTRWithImm;
        else
          ++NumLDRToSTRWithImm;
#endif // DEBUG
      }
    }
    AArch64FI.addLOHDirective(Kind, Args);
  }

  // Now, we grabbed all the big patterns, check ADR opportunities.
  for (MachineInstr *Candidate : PotentialADROpportunities)
    registerADRCandidate(*Candidate, ReachingDefs, AArch64FI, InvolvedInLOHs);
}

bool AArch64CollectLOH::runOnMachineFunction(MachineFunction &MF) {
  if (skipFunction(*MF.getFunction()))
    return false;

  const TargetRegisterInfo *TRI = MF.getSubtarget().getRegisterInfo();
  const MachineDominatorTree *MDT = &getAnalysis<MachineDominatorTree>();

  AArch64FunctionInfo *AArch64FI = MF.getInfo<AArch64FunctionInfo>();
  assert(AArch64FI && "No MachineFunctionInfo for this function!");

  DEBUG(dbgs() << "Looking for LOH in " << MF.getName() << '\n');

  bool Modified = false;

  // Start with ADRP.
  ReachingPhysDefs_ADRP ReachingDefs_ADRP(&MF, TRI, BasicBlockScopeOnly);
  ReachingDefs_ADRP.addRegs();
  if (!ReachingDefs_ADRP.analyze())
    return false;
  DEBUG(ReachingDefs_ADRP.dump(););

  // Compute LOH for ADRP.
  computeADRP(ReachingDefs_ADRP, *AArch64FI, MDT);

  // Continue with general ADRP -> ADD/LDR -> LDR/STR pattern.
  ReachingPhysDefs_Others ReachingDefs(&MF, TRI, BasicBlockScopeOnly);
  ReachingDefs.addRegs();
  ReachingDefs.analyze();
  DEBUG(ReachingDefs.dump(););

  // Compute other than AdrpAdrp LOH.
  computeOthers(ReachingDefs, *AArch64FI, MDT);

  return Modified;
}

/// createAArch64CollectLOHPass - returns an instance of the Statistic for
/// linker optimization pass.
FunctionPass *llvm::createAArch64CollectLOHPass() {
  return new AArch64CollectLOH();
}
