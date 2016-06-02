//===- llvm/CodeGen/ReachingPhysDefs.cpp --- Reaching definitions -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/CodeGen/ReachingPhysDefs.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include <queue>

using namespace llvm;

#define DEBUG_TYPE "reachingdefs"

static void dumpMIWithBB(const MachineInstr *MI) {
  dbgs() << "MBB#" << MI->getParent()->getNumber()
         << ":"; MI->dump();
}

void SetOfMachineInstr::dump() {
  for (auto *I : *this) {
    dbgs() << "\t"; dumpMIWithBB(I);
  }
  dbgs() << "\n";
}

SetOfMachineInstr &ReachingPhysDefs::
getSet(BlockToSetOfInstrsPerColor &sets, const MachineBasicBlock &MBB,
       unsigned reg) {
  SetOfMachineInstr *result;
  BlockToSetOfInstrsPerColor::iterator it = sets.find(&MBB);
  if (it != sets.end())
    result = it->second.get();
  else
    result = (sets[&MBB] = make_unique<SetOfMachineInstr[]>(nbRegs())).get();

  return result[reg];
}

SetOfMachineInstr &ReachingPhysDefs::getUses(InstrToInstrs *sets, unsigned reg,
                                             MachineInstr &MI) {
  return sets[reg][&MI];
}

const SetOfMachineInstr *ReachingPhysDefs::
getUses(const InstrToInstrs *sets, unsigned reg, MachineInstr &MI) {
  InstrToInstrs::const_iterator Res = sets[reg].find(&MI);
  if (Res != sets[reg].end())
    return &(Res->second);
  return nullptr;
}

SetOfMachineInstr *ReachingPhysDefs::
getUses(unsigned reg, MachineInstr &MI) {
  return &getUses(ColorOpToReachedUses, RegToId.find(reg)->second, MI);
}

ReachingPhysDefs::
ReachingPhysDefs(MachineFunction *MF_, const TargetRegisterInfo *TRI_,
                 bool BBScopeOnly, const Twine &name)
  : MF(MF_), TRI(TRI_), TII(nullptr), Name(name.str()),
    BasicBlockScopeOnly(BBScopeOnly), DummyOp(nullptr), CurRegId(0),
    ColorOpToReachedUses(nullptr), ColorOpToReachingDefs(nullptr)
{
  TII = MF->getSubtarget().getInstrInfo();
}

ReachingPhysDefs::~ReachingPhysDefs() {
  if (ColorOpToReachedUses != nullptr)
    delete[] ColorOpToReachedUses;
  if (ColorOpToReachingDefs != nullptr)
    delete[] ColorOpToReachingDefs;
  if (DummyOp != nullptr)
    MF->DeleteMachineInstr(DummyOp);
}

void ReachingPhysDefs::
pruneMultipleDefs() {
  for (unsigned CurReg = 0; CurReg < nbRegs(); ++CurReg) {
    bool change = true;
    while (change) {
      change = false;
      for (const auto &DefsIt : ColorOpToReachedUses[CurReg]) {
        SetOfMachineInstr toPrune;
        for (MachineInstr *Use : DefsIt.second)
          if (ColorOpToReachingDefs[CurReg][Use].size() > 1) {
            DEBUG(dbgs() << "Found user with multiple defs ("
                  << PrintReg(IdToReg[CurReg], TRI) << "):\n";
                  dumpMIWithBB(Use);
                  ColorOpToReachingDefs[CurReg][Use].dump(););
            toPrune.insert(ColorOpToReachingDefs[CurReg][Use].begin(),
                           ColorOpToReachingDefs[CurReg][Use].end());
            break;
          }

        if (!toPrune.empty()) {
          for (MachineInstr *Def : toPrune) {
            DEBUG(dbgs() << "Pruning reaching def:\n"; Def->dump();
                  ColorOpToReachedUses[CurReg][Def].dump());
            ColorOpToReachedUses[CurReg].erase(Def);
          }
          change = true;
          break;
        }
      }
    }
  }
}

bool ReachingPhysDefs::analyze(unsigned singleReg) {
  if (singleReg)
    addReg(singleReg);
  
  if (!nbRegs())
    return false;

  assert(ColorOpToReachedUses == nullptr && "Don't call analyze twice!");
  ColorOpToReachedUses = new InstrToInstrs[nbRegs()];
  ColorOpToReachingDefs = new InstrToInstrs[nbRegs()];
  if (BasicBlockScopeOnly) {
    // For local analysis, create a dummy operation to record uses
    // that are not local.
    DummyOp = MF->CreateMachineInstr(TII->get(TargetOpcode::COPY), DebugLoc());
  }

  // Compute the reaching defs.
  reachingDef();

  // Translate the definition to uses map into a use to definitions map.
  reachedUsesToDefs();

  return true;
}

// Map registers with dense id in RegToId and vice-versa in
// IdToReg. IdToReg is populated only in DEBUG mode.
void ReachingPhysDefs::addReg(unsigned reg) {
  DEBUG(IdToReg.push_back(reg);
        assert(IdToReg[CurRegId] == reg &&
               "Reg index mismatches insertion index."));
  RegToId[reg] = CurRegId++;
}

// Map registers with dense id in RegToId and vice-versa in
// IdToReg. IdToReg is populated only in DEBUG mode.
void ReachingPhysDefs::addAllTargetRegs() {
  CurRegId = 0;
  unsigned NbReg = TRI->getNumRegs();
  for (; CurRegId < NbReg; ++CurRegId) {
    RegToId[CurRegId] = CurRegId;
    DEBUG(IdToReg.push_back(CurRegId););
  }
}

// Initialize the reaching definition algorithm:
// For each basic block BB in MF, record:
// - its kill set.
// - its reachable uses (uses that are exposed to BB's predecessors).
// - its generated definitions.
void ReachingPhysDefs::initReachingDef() {
  for (MachineBasicBlock &MBB : *MF) {
    auto &BBGen = Gen[&MBB];
    BBGen = make_unique<MachineInstr *[]>(nbRegs());
    std::fill(BBGen.get(), BBGen.get() + nbRegs(), nullptr);

    BitVector &BBKillSet = Kill[&MBB];
    BBKillSet.resize(nbRegs());

    for (MachineInstr &MI : MBB) {
      // Process uses first.
      processMIUses(MI);
      processMIClobbers(MI);
      processMIDefs(MI);
    }

    // If we restrict our analysis to basic block scope,
    // conservatively add a dummy use for each generated value.
    if (recordDummyUses() && DummyOp && !MBB.succ_empty())
      for (unsigned CurReg = 0; CurReg < nbRegs(); ++CurReg)
        if (BBGen[CurReg])
          getUses(ColorOpToReachedUses, CurReg, *BBGen[CurReg]).insert(DummyOp);
  }
}

unsigned ReachingPhysDefs::processMIUseMO(MachineOperand &MO) {
    if (!MO.isReg() || !MO.isUse())
      return 0;
    return MO.getReg();
}

void ReachingPhysDefs::processMIUses(MachineInstr &MI) {
  const MachineBasicBlock *MBB = MI.getParent();
  auto &BBGen = Gen[MBB];
  BitVector &BBKillSet = Kill[MBB];

  for (MachineOperand &MO : MI.operands()) {
    unsigned CurReg = processMIUseMO(MO);
    if (!CurReg)
      continue;
    MapRegToId::const_iterator ItCurRegId = RegToId.find(CurReg);
    if (ItCurRegId == RegToId.end())
      continue;
    CurReg = ItCurRegId->second;

    // if CurReg has not been defined, this use is reachable.
    if (!BBGen[CurReg] && !BBKillSet.test(CurReg))
      getSet(ReachableUses, *MBB, CurReg).insert(&MI);
    // current basic block definition for this color, if any, is in Gen.
    if (BBGen[CurReg])
      getUses(ColorOpToReachedUses, CurReg, *BBGen[CurReg]).insert(&MI);
  }
}

void ReachingPhysDefs::processMIClobbers(MachineInstr &MI) {
  const MachineBasicBlock *MBB = MI.getParent();
  auto &BBGen = Gen[MBB];
  BitVector &BBKillSet = Kill[MBB];

  for (const MachineOperand &MO : MI.operands()) {
    if (!MO.isRegMask())
      continue;
    // Clobbers kill the related colors.
    const uint32_t *PreservedRegs = MO.getRegMask();

    // Set generated regs.
    for (const auto &Entry : RegToId) {
      unsigned Reg = Entry.second;
      // Use the global register ID when querying APIs external to this
      // pass.
      if (MachineOperand::clobbersPhysReg(PreservedRegs, Entry.first)) {
        // Do not register clobbered definition - this definition
        // is not used anyway (otherwise register allocation is
        // wrong).
        BBGen[Reg] = rememberClobberingMI() ? &MI : nullptr;
        BBKillSet.set(Reg);
      }
    }
  }
}

void ReachingPhysDefs::processMIDefs(MachineInstr &MI) {
  const MachineBasicBlock *MBB = MI.getParent();
  auto &BBGen = Gen[MBB];
  BitVector &BBKillSet = Kill[MBB];

  for (const MachineOperand &MO : MI.operands()) {
    if (!MO.isReg() || !MO.isDef())
      continue;
    unsigned CurReg = MO.getReg();
    MapRegToId::const_iterator ItCurRegId = RegToId.find(CurReg);
    if (ItCurRegId == RegToId.end())
      continue;

    for (MCRegAliasIterator AI(CurReg, TRI, true); AI.isValid(); ++AI) {
      MapRegToId::const_iterator ItRegId = RegToId.find(*AI);
      // If this alias has not been recorded, then it is not interesting
      // for the current analysis. XXX True generally?
      // We can end up in this situation because of tuple registers.
      // E.g., Let say we are interested in S1. When we register
      // S1, we will also register its aliases and in particular
      // the tuple Q1_Q2.
      // Now, when we encounter Q1_Q2, we will look through its aliases
      // and will find that S2 is not registered.
      if (ItRegId == RegToId.end())
        continue;

      BBKillSet.set(ItRegId->second);
      BBGen[ItRegId->second] = &MI;
    }
    BBGen[ItCurRegId->second] = &MI;
  }
}

// Reaching def core algorithm:
// While !converged
//    for each bb
//       for each color
//           In[bb][color] = U Out[bb.predecessors][color]
//           insert reachableUses[bb][color] in each in[bb][color]
//                 op.reachedUses
//
//           Out[bb] = Gen[bb] U (In[bb] - Kill[bb])
void ReachingPhysDefs::reachingDefAlgorithm() {
  DenseMap<unsigned int, MachineBasicBlock *> OrderToBB;
  DenseMap<MachineBasicBlock *, unsigned int> BBToOrder;
  std::priority_queue<unsigned int, std::vector<unsigned int>,
                      std::greater<unsigned int>>
      Worklist;
  std::priority_queue<unsigned int, std::vector<unsigned int>,
                      std::greater<unsigned int>>
      Pending;

  ReversePostOrderTraversal<MachineFunction *> RPOT(MF);
  unsigned int RPONumber = 0;
  for (auto RI = RPOT.begin(), RE = RPOT.end(); RI != RE; ++RI) {
    OrderToBB[RPONumber] = *RI;
    BBToOrder[*RI] = RPONumber;
    Worklist.push(RPONumber);
    ++RPONumber;
  }

  // The algorithm has converged when both worklists are empty.
  while (!Worklist.empty() || !Pending.empty()) {
    // We track what is on the pending worklist to avoid inserting the same
    // thing twice.  We could avoid this with a custom priority queue, but this
    // is probably not worth it.
    SmallPtrSet<MachineBasicBlock *, 16> OnPending;
    while (!Worklist.empty()) {
      MachineBasicBlock *MBB = OrderToBB[Worklist.top()];
      Worklist.pop();
      bool OutHasChanged = false;

      ///////// Work for MBB.
      // The Worklist algo was copied from LivedebugValues, and
      // perhaps it could be factored out with a virtual doMBBWork()
      // or similar?
      unsigned CurReg;
      for (CurReg = 0; CurReg < nbRegs(); ++CurReg) {
        SetOfMachineInstr &BBInSet = getSet(In, *MBB, CurReg);
        SetOfMachineInstr &BBReachableUses =
          getSet(ReachableUses, *MBB, CurReg);
        SetOfMachineInstr &BBOutSet = getSet(Out, *MBB, CurReg);
        unsigned Size = BBOutSet.size();
        //   In[bb][color] = U Out[bb.predecessors][color]
        for (const MachineBasicBlock *PredMBB : MBB->predecessors()) {
          SetOfMachineInstr &PredOutSet = getSet(Out, *PredMBB, CurReg);
          BBInSet.insert(PredOutSet.begin(), PredOutSet.end());
        }
        //   insert reachableUses[bb][color] in each in[bb][color] op.reachedses
        for (MachineInstr *MI : BBInSet) {
          SetOfMachineInstr &OpReachedUses =
            getUses(ColorOpToReachedUses, CurReg, *MI);
          OpReachedUses.insert(BBReachableUses.begin(), BBReachableUses.end());
        }
        //           Out[bb] = Gen[bb] U (In[bb] - Kill[bb])
        if (!Kill[MBB].test(CurReg))
          BBOutSet.insert(BBInSet.begin(), BBInSet.end());
        if (Gen[MBB][CurReg])
          BBOutSet.insert(Gen[MBB][CurReg]);
        OutHasChanged |= BBOutSet.size() != Size;
      }
      ///////// End work for MBB.

      // If Out[bb] has changed, put its successors on Pending.
      if (OutHasChanged) {
        for (auto s : MBB->successors())
          if (!OnPending.count(s)) {
            OnPending.insert(s);
            Pending.push(BBToOrder[s]);
          }
      }
    }

    Worklist.swap(Pending);
    // At this point, pending must be empty, since it was just the empty
    // worklist. (Do we still want to check Pending in the outer while loop?)
    assert(Pending.empty() && "Pending should be empty");
  }
}

// Reaching definition algorithm.
// On completion, ColorOpToReachedUses will contain the result of the
// reaching def algorithm.
void ReachingPhysDefs::reachingDef() {

  // Initialize Gen, kill and reachableUses.
  initReachingDef();

  // Algo.
  if (!DummyOp)
    reachingDefAlgorithm();
}

void ReachingPhysDefs::reachedUsesToDefs() {
  for (unsigned CurReg = 0; CurReg < nbRegs(); ++CurReg) {
    for (const auto &DefsIt : ColorOpToReachedUses[CurReg]) {
      MachineInstr *Def = DefsIt.first;
      for (MachineInstr *Use : DefsIt.second) {
        UseToDefs[Use].insert(Def);
        ColorOpToReachingDefs[CurReg][Use].insert(Def);
      }
    }
  }
}

#ifndef NDEBUG
void ReachingPhysDefs::dump() {
  DEBUG(dbgs() << Name << ":\n");
  unsigned CurReg;
  for (CurReg = 0; CurReg < nbRegs(); ++CurReg) {
    if (ColorOpToReachedUses[CurReg].empty())
      continue;
    DEBUG(dbgs() << "Reg " << PrintReg(IdToReg[CurReg], TRI) << " \n");
    for (auto &DefsIt : ColorOpToReachedUses[CurReg]) {
      DEBUG(dumpMIWithBB(DefsIt.first);
            DefsIt.second.dump(););
    }
  }
}
#endif // NDEBUG

