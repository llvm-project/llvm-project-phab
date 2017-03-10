//===-- ShrinkWrap2.cpp - Compute safe point for prolog/epilog insertion --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This pass is an improvement of the current shrink-wrapping pass, based, this
// time, on the dataflow analysis described in "Minimizing Register Usage
// Penalty at Procedure Calls - Fred C. Chow" [1], and improved in "Post
// Register Allocation Spill Code Optimization - Christopher Lupo, Kent D.
// Wilken" [2]. The aim of this improvement is to remove the restriction that
// the current shrink-wrapping pass is having, which is having only one save /
// restore point.
// FIXME: ShrinkWrap2: Random thoughts:
// - r193749 removed an old pass that was an implementation of [1].
// - Cost model: use MachineBlockFrequency and some instruction cost model?
// - Avoid RegionInfo since it's expensive to build?
// - Split critical edges on demand?
// - Statistics: get a number from block frequency and number of save / restores
// - Statistics: mesure code size
// - Include loops based on the cost model?
// - Eliminate trivial cases where most of the CSRs are used in the entry block?
//===----------------------------------------------------------------------===//

// Pass initialization.
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/Passes.h"

// Data structures.
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/DenseMap.h"
#include <set>
#include <limits>

// Codegen basics.
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

// Determine maximal SESE regions for save / restore blocks placement.
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachinePostDominators.h"
#include "llvm/CodeGen/MachineRegionInfo.h"

// Detect and avoid saves / restores inside loops.
#include "llvm/CodeGen/MachineLoopInfo.h"

// Target-specific information.
#include "llvm/Target/TargetFrameLowering.h"
#include "llvm/Target/TargetRegisterInfo.h"
#include "llvm/Target/TargetSubtargetInfo.h"

// Debug.
#include "llvm/Support/raw_ostream.h"

// FIXME: ShrinkWrap2: Fix name.
#define DEBUG_TYPE "shrink-wrap2"

#define VERBOSE_DEBUG(X)                                                       \
  do {                                                                         \
    if (VerboseDebug)                                                          \
      DEBUG(X);                                                                \
  } while (0);

// FIXME: ShrinkWrap2: Add iterators to BitVector?
#define FOREACH_BIT(Var, BitVector)                                            \
  for (int Var = BitVector.find_first(); Var > 0;                              \
       Var = BitVector.find_next(Var))

using namespace llvm;

// FIXME: ShrinkWrap2: Statistics.

// FIXME: ShrinkWrap2: Fix name.
static cl::opt<cl::boolOrDefault>
    EnableShrinkWrap2Opt("enable-shrink-wrap2", cl::Hidden,
                         cl::desc("enable the shrink-wrapping 2 pass"));

#define UseRegions cl::BOU_FALSE
//static cl::opt<cl::boolOrDefault>
//    UseRegions("shrink-wrap-use-regions", cl::Hidden,
//               cl::desc("push saves / restores at region boundaries"));
//
static cl::opt<cl::boolOrDefault>
    VerboseDebug("shrink-wrap-verbose", cl::Hidden,
                 cl::desc("verbose debug output"));

// FIXME: ShrinkWrap2: Remove, debug.
static cl::opt<cl::boolOrDefault> ViewCFGDebug("shrink-wrap-view", cl::Hidden,
                                               cl::desc("view cfg"));

namespace {
// FIXME: ShrinkWrap2: Fix name.
class ShrinkWrap2 : public MachineFunctionPass {
  // FIXME: ShrinkWrap2: Use SmallSet<unsigned> with MBB number.
  using BBSet = std::set<MachineBasicBlock *>;
  using BBRegSetMap = DenseMap<MachineBasicBlock *, BitVector>;

  /// Store callee-saved-altering instructions.
  // FIXME: ShrinkWrap2: Should be merged with APP.
  // FIXME: ShrinkWrap2: Use SmallVector<<BitVector>, MF.size()>, use MBB number
  // as index.
  DenseMap<const MachineBasicBlock *, BitVector> UsedCSR;

  /// All the CSR used in the function. This is used for quicker iteration over
  /// the registers.
  /// FIXME: ShrinkWrap2: PEI needs this, instead of calling TII->determineCalleeSaved.
  BitVector Regs;

  /// The dataflow attributes needed to compute shrink-wrapping locations.
  struct DataflowAttributes {
    /// The MachineFunction we're analysing.
    MachineFunction &MF;

    // Associate Reg -> [MBB -> bool]
    // FIXME: ShrinkWrap2: Update to MBB -> BitVector<Reg>.
    using AttributeMap = DenseMap<const MachineBasicBlock *, bool>;
    using RegAttributeMap = DenseMap<unsigned, AttributeMap>;

    // FIXME: ShrinkWrap2: Explain anticipated / available and how the
    // properties are used.

    /// Is the register used ?
    RegAttributeMap APP;
    /// Is the register anticipated at the end of this basic block?
    RegAttributeMap ANTOUT;
    /// Is the register anticipated at the beginning of this basic block?
    RegAttributeMap ANTIN;
    /// Is the register available at the beginning of this basic block?
    RegAttributeMap AVIN;
    /// Is the register available at the end of this basic block?
    RegAttributeMap AVOUT;

    DataflowAttributes(MachineFunction &TheFunction) : MF{TheFunction} {}

    /// Populate the attribute maps with trivial properties from the used
    /// registers.
    void populate(const DenseMap<const MachineBasicBlock *, BitVector> &Used);
    /// Compute the attributes for one register.
    // FIXME: ShrinkWrap2: Don't do this per register.
    void compute(unsigned Reg);
    /// Save the results for this particular register.
    // FIXME: ShrinkWrap2: Don't do this per register.
    void results(unsigned Reg, BBRegSetMap &Saves, BBRegSetMap &Restores);
    /// Dump the contents of the attributes.
    // FIXME: ShrinkWrap2: Don't do this per register.
    void dump(unsigned Reg) const;
  };

  /// Final results.
  BBRegSetMap Saves;
  BBRegSetMap Restores;

  /// Detect loops to avoid placing saves / restores in a loop.
  MachineLoopInfo *MLI;

  // REGIONS ===================================================================

  /// (Post)Dominator trees used for getting maximal SESE regions.
  MachineDominatorTree *MDT;
  MachinePostDominatorTree *MPDT;

  /// SESE region information used by Christopher Lupo and Kent D. Wilken's
  /// paper.
  MachineRegionInfo *RegionInfo;
  /// Hold all the maximal regions.
  // FIXME: ShrinkWrap2: Use something better than std::set?
  std::set<MachineRegion *> MaximalRegions;

  /// Determine all the calee saved register used in this function.
  /// This fills out the Regs set, containing all the CSR used in the entire
  /// function, and fills the UsedCSR map, containing all the CSR used per
  /// basic block.
  /// We don't use the TargetFrameLowering::determineCalleeSaves function
  /// because we need to associate each usage of a CSR to the corresponding
  /// basic block.
  // FIXME: ShrinkWrap2: Target hook might add other callee saves.
  // FIXME: ShrinkWrap2: Should we add the registers added by the target in the
  // entry / exit block(s) ?
  void determineCalleeSaves(const MachineFunction &MF);
  void removeUsesOnUnreachablePaths(MachineFunction &MF);
  void dumpUsedCSR(const MachineFunction &MF) const;

  /// This algorithm relies on the fact that there are no critical edges.
  // FIXME: ShrinkWrap2: Get rid of this.
  bool splitCriticalEdges(MachineFunction &MF);

  /// Mark all the basic blocks related to a loop (inside, entry, exit) as used,
  /// if there is an usage of a CSR inside a loop. We want to avoid any save /
  /// restore operations in a loop.
  // FIXME: ShrinkWrap2: Should this be updated with the cost model?
  void markUsesInsideLoops(MachineFunction &MF);

  /// Populate MaximalRegions with all the regions in the function.
  // FIXME: ShrinkWrap2: iterate on depth_first(Top) should work?
  // FIXME: ShrinkWrap2: use a lambda inside computeMaximalRegions?
  void gatherAllRegions(MachineRegion *Top) {
    MaximalRegions.insert(Top);
    for (const std::unique_ptr<MachineRegion> &MR : *Top)
      gatherAllRegions(MR.get());
  }

  /// The SESE region information used by Christopher Lupo and Kent D. Wilken's
  /// paper is based on _maximal_ SESE regions which are described like this:
  /// A SESE region (a, b) is _maximal_ provided:
  /// * b post-dominates b' for any SESE region (a, b')
  /// * a dominates a' for any SESE region (a', b)
  // FIXME: ShrinkWrap2: Somehow, we have RegionInfoBase::getMaxRegionExit that
  // returns the exit of the maximal refined region starting at a basic block.
  // Using this doesn't return the blocks expected by the paper's definition of
  // _maximal_ regions.
  // FIXME: ShrinkWrap2: Same here, he have RegionBase::getExpandedRegion that
  // returns a bigger region starting at the same entry point.
  // FIXME: ShrinkWrap2: Merge with gatherAllRegions?
  void computeMaximalRegions(MachineRegion *MRI);
  void removeMaximalRegion(MachineRegion *Parent, MachineRegion *Child);
  void mergeMaximalRegions(MachineRegion *Entry, MachineRegion *Exit);

  /// Move all the save / restore points at the boundaries of a maximal region.
  /// This is where [2] is an improvement of [1].
  // FIXME: ShrinkWrap2: Don't ALWAYS move. Use cost model.
  void moveAtRegionBoundaries(MachineFunction &MF);

  /// * Verify if the results are better than obvious results, like:
  ///   * CSR used in a single MBB: save at MBB.entry, restore at MBB.exit.
  /// * Remove empty entries from the Saves / Restores maps.
  // FIXME: ShrinkWrap2: This shouldn't happen, we better fix the algorithm
  // first.
  void postProcessResults(MachineFunction &MF);

  /// Return the save / restore points to MachineFrameInfo, to be used by PEI.
  void returnToMachineFrame(MachineFunction &MF);

  /// Verify save / restore points by walking the CFG.
  /// This asserts if anything went wrong.
  // FIXME: ShrinkWrap2: Should we add a special flag for this?
  // FIXME: ShrinkWrap2: Expensive checks?
  void verifySavesRestores(MachineFunction &MF) const;

  /// Dump the final shrink-wrapping results.
  void dumpResults(MachineFunction &MF) const;

  /// \brief Initialize the pass for \p MF.
  void init(MachineFunction &MF) {
    MLI = &getAnalysis<MachineLoopInfo>();
    if (UseRegions != cl::BOU_FALSE) {
      MDT = &getAnalysis<MachineDominatorTree>();
      MPDT = &getAnalysis<MachinePostDominatorTree>();
      RegionInfo = &getAnalysis<MachineRegionInfoPass>().getRegionInfo();
    }
  }

  /// Clear the function's state.
  void clear() {
    UsedCSR.clear();
    Regs.clear();
    Saves.clear();
    Restores.clear();
    if (UseRegions != cl::BOU_FALSE)
      MaximalRegions.clear();
  }

public:
  static char ID;

  ShrinkWrap2() : MachineFunctionPass(ID) {
    initializeShrinkWrap2Pass(*PassRegistry::getPassRegistry());
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
    AU.addRequired<MachineLoopInfo>();
    if (UseRegions != cl::BOU_FALSE) {
      AU.addRequired<MachineDominatorTree>();
      AU.addRequired<MachinePostDominatorTree>();
      AU.addRequired<MachineRegionInfoPass>();
    }
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  StringRef getPassName() const override { return "Shrink Wrapping analysis"; }

  /// \brief Perform the shrink-wrapping analysis and update
  /// the MachineFrameInfo attached to \p MF with the results.
  bool runOnMachineFunction(MachineFunction &MF) override;
};
} // End anonymous namespace.

char ShrinkWrap2::ID = 0;
char &llvm::ShrinkWrap2ID = ShrinkWrap2::ID;

// FIXME: ShrinkWrap2: Fix name.
INITIALIZE_PASS_BEGIN(ShrinkWrap2, "shrink-wrap2", "Shrink Wrap Pass", false,
                      false)
INITIALIZE_PASS_DEPENDENCY(MachineLoopInfo)
INITIALIZE_PASS_DEPENDENCY(MachineDominatorTree)
INITIALIZE_PASS_DEPENDENCY(MachinePostDominatorTree)
INITIALIZE_PASS_DEPENDENCY(MachineRegionInfoPass)
// FIXME: ShrinkWrap2: Fix name.
INITIALIZE_PASS_END(ShrinkWrap2, "shrink-wrap2", "Shrink Wrap Pass", false,
                    false)

void ShrinkWrap2::DataflowAttributes::populate(
    const DenseMap<const MachineBasicBlock *, BitVector> &Used) {
  for (auto &KV : Used) {
    const MachineBasicBlock *MBB = KV.first;
    const BitVector &Regs = KV.second;
    FOREACH_BIT(Reg, Regs) {
      APP[Reg][MBB] = true;
      // Setting APP also affects ANTIN and AVOUT.
      // ANTIN = APP || ANTOUT
      ANTIN[Reg][MBB] = true;
      // AVOUT = APP || AVIN
      AVOUT[Reg][MBB] = true;
    }
  }
}

void ShrinkWrap2::DataflowAttributes::compute(unsigned Reg) {
  // FIXME: ShrinkWrap2: Reverse DF for ANT? DF for AV?
  // FIXME: ShrinkWrap2: Use a work list, don't compute attributes that we're
  // sure will never change.
  bool Changed = true;
  auto AssignIfDifferent = [&](bool &Original, const bool &New) {
    if (Original != New) {
      Original = New;
      Changed = true;
    }
  };

  AttributeMap &APPr = APP[Reg];
  AttributeMap &ANTOUTr = ANTOUT[Reg];
  AttributeMap &ANTINr = ANTIN[Reg];
  AttributeMap &AVINr = AVIN[Reg];
  AttributeMap &AVOUTr = AVOUT[Reg];

  while (Changed) {
    Changed = false;
    for (MachineBasicBlock &MBB : MF) {
      // If there is an use of this register on *all* the paths starting from
      // this basic block, the register is anticipated at the end of this block
      // (propagate the IN attribute of successors to possibly merge saves)
      //           -
      //          | *false*             if no successor.
      // ANTOUT = |
      //          | && ANTIN(succ[i])   otherwise.
      //           -
      bool &ANTOUTi = ANTOUTr[&MBB];
      if (MBB.succ_empty())
        AssignIfDifferent(ANTOUTi, false);
      else {
        bool A = std::all_of(MBB.succ_begin(), MBB.succ_end(),
                             [&](MachineBasicBlock *S) {
                               if (S == &MBB) // Ignore self.
                                 return true;
                               return ANTINr[S];
                             });
        AssignIfDifferent(ANTOUTi, A);
      }

      // If the register is used in the block, or if it is anticipated in all
      // successors it is also anticipated at the beginning, since we consider
      // entire blocks.
      //          -
      // ANTIN = | APP || ANTOUT
      //          -
      bool &ANTINi = ANTINr[&MBB];
      bool NewANTIN = APPr[&MBB] || ANTOUTr[&MBB];
      AssignIfDifferent(ANTINi, NewANTIN);

      // If there is an use of this register on *all* the paths arriving in this
      // block, then the register is available in this block (propagate the out
      // attribute of predecessors to possibly merge restores).
      //         -
      //        | *false*             if no predecessor.
      // AVIN = |
      //        | && AVOUT(pred[i])   otherwise.
      //         -
      bool &AVINi = AVINr[&MBB];
      if (MBB.pred_empty())
        AssignIfDifferent(AVINi, false);
      else {
        bool A = std::all_of(MBB.pred_begin(), MBB.pred_end(),
                             [&](MachineBasicBlock *P) {
                               if (P == &MBB) // Ignore self.
                                 return true;
                               return AVOUTr[P];
                             });
        AssignIfDifferent(AVINi, A);
      }

      // If the register is used in the block, or if it is always available in
      // all predecessors , it is also available on exit, since we consider
      // entire blocks.
      //          -
      // AVOUT = | APP || AVIN
      //          -
      bool &AVOUTi = AVOUTr[&MBB];
      bool NewAVOUT = APPr[&MBB] || AVINr[&MBB];
      AssignIfDifferent(AVOUTi, NewAVOUT);
    }

    VERBOSE_DEBUG(dump(Reg));
  }
}

void ShrinkWrap2::DataflowAttributes::results(unsigned Reg, BBRegSetMap &Saves,
                                              BBRegSetMap &Restores) {
  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();
  AttributeMap &ANTINr = ANTIN[Reg];
  AttributeMap &AVOUTr = AVOUT[Reg];

  for (MachineBasicBlock &MBB : MF) {
    // If the register uses are anticipated on *all* the paths leaving this
    // block, and if the register is not available at the entrance of this block
    // (if it is, then it means it has been saved already, but not restored),
    // and if *none* of the predecessors anticipates this register on their
    // output (we want to get the "highest" block), then we can identify a save
    // point for the function.
    //
    // SAVE = ANTIN && !AVIN && !ANTIN(pred[i])
    //
    bool NS = std::none_of(MBB.pred_begin(), MBB.pred_end(),
                           [&](MachineBasicBlock *P) {
                             if (P == &MBB) // Ignore self.
                               return false;
                             return ANTINr[P];
                           });
    if (ANTINr[&MBB] && !AVIN[Reg][&MBB] && NS) {
      BitVector &Save = Saves[&MBB];
      if (Save.empty())
        Save.resize(TRI.getNumRegs());
      Save.set(Reg);
    }

    // If the register uses are available on *all* the paths leading to this
    // block, and if the register is not anticipated at the exit of this block
    // (if it is, then it means it has been restored already), and if *none* of
    // the successors make the register available (we want to cover the deepest
    // use), then we can identify a restrore point for the function.
    //
    // RESTORE = AVOUT && !ANTOUT && !AVOUT(succ[i])
    //
    bool NR = std::none_of(MBB.succ_begin(), MBB.succ_end(),
                           [&](MachineBasicBlock *S) {
                             if (S == &MBB) // Ignore self.
                               return false;
                             return AVOUTr[S];
                           });
    if (AVOUTr[&MBB] && !ANTOUT[Reg][&MBB] && NR) {
      BitVector &Restore = Restores[&MBB];
      if (Restore.empty())
        Restore.resize(TRI.getNumRegs());
      Restore.set(Reg);
    }
  }
}

void ShrinkWrap2::DataflowAttributes::dump(unsigned Reg) const {
  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();
  for (MachineBasicBlock &MBB : MF) {
    dbgs() << "BB#" << MBB.getNumber() << "<" << PrintReg(Reg, &TRI) << ">"
           << ":\n\tANTOUT : " << ANTOUT.lookup(Reg).lookup(&MBB) << '\n'
           << "\tANTIN : " << ANTIN.lookup(Reg).lookup(&MBB) << '\n'
           << "\tAVIN : " << AVIN.lookup(Reg).lookup(&MBB) << '\n'
           << "\tAVOUT : " << AVOUT.lookup(Reg).lookup(&MBB) << '\n';
  }
}

// FIXME: ShrinkWrap2: Target hook might add other callee saves.
void ShrinkWrap2::determineCalleeSaves(const MachineFunction &MF) {
  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();
  const MachineRegisterInfo &MRI = MF.getRegInfo();

  // Walk all the uses of each callee-saved register, and map them to their
  // basic blocks.
  const MCPhysReg *CSRegs = TRI.getCalleeSavedRegs(&MF);

  // FIXME: ShrinkWrap2: Naked functions.
  // FIXME: ShrinkWrap2: __builtin_unwind_init.
  // FIXME: ShrinkWrap2: RegMasks. In theory this no callee saves should be
  // clobbered, but if we call a different ABI function, we have to check.

  // Test all the target's callee saved registers.
  for (unsigned i = 0; CSRegs[i]; ++i) {
    unsigned Reg = CSRegs[i];
    // If at least one of the aliases is used, mark the original register as
    // used.
    for (MCRegAliasIterator AliasReg(Reg, &TRI, true); AliasReg.isValid();
         ++AliasReg) {
      // Walk all the uses, excepting for debug instructions.
      for (auto MOI = MRI.reg_nodbg_begin(*AliasReg), e = MRI.reg_nodbg_end();
           MOI != e; ++MOI) {
        // Get or create the registers used for the BB.
        BitVector &Used = UsedCSR[MOI->getParent()->getParent()];
        // Resize if it's the first time used.
        if (Used.empty())
          Used.resize(TRI.getNumRegs());
        Used.set(Reg);
      }
      // Look for any live-ins in basic blocks.
      for (const MachineBasicBlock &MBB : MF) {
        if (MBB.isLiveIn(Reg)) {
          BitVector &Used = UsedCSR[&MBB];
          // Resize if it's the first time used.
          if (Used.empty())
            Used.resize(TRI.getNumRegs());
          Used.set(Reg);
        }
      }
    }
  }
}

void ShrinkWrap2::removeUsesOnUnreachablePaths(MachineFunction &MF) {
  std::set<const MachineBasicBlock *> ToRemove;
  for (auto &KV : UsedCSR) {
    const MachineBasicBlock *MBB = KV.first;
    if (![&] {
          for (const MachineBasicBlock *Block : depth_first(MBB))
            if (Block->isReturnBlock())
              return true;
          return false;
        }())
      ToRemove.insert(MBB);
  }
  for (const MachineBasicBlock *MBB : ToRemove) {
    DEBUG(dbgs() << "Remove uses from unterminable BB#" << MBB->getNumber()
                 << '\n');
    UsedCSR.erase(MBB);
  }
}

void ShrinkWrap2::dumpUsedCSR(const MachineFunction &MF) const {
  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();

  // Use std::map for deterministic output.
  std::map<const MachineBasicBlock *, BitVector> UsedCSRSorted{UsedCSR.begin(),
                                                               UsedCSR.end()};

  for (auto &KV : UsedCSRSorted) {
    dbgs() << "BB#" << KV.first->getNumber() << " uses : ";
    int Reg = KV.second.find_first();
    if (Reg > 0)
      dbgs() << PrintReg(Reg, &TRI);
    for (Reg = KV.second.find_next(Reg); Reg > 0;
         Reg = KV.second.find_next(Reg))
      dbgs() << ", " << PrintReg(Reg, &TRI);
    dbgs() << '\n';
  }
}

void ShrinkWrap2::computeMaximalRegions(MachineRegion *Top) {
  using RegionPair = std::pair<MachineRegion *, MachineRegion *>;
  bool Changed = true;

  while (Changed) {
    Changed = false;

    // Merge consecutive regions. If Exit(A) == Entry(B), merge A and B.
    bool MergeChanged = true;
    while (MergeChanged) {
      MergeChanged = false;

      // FIXME: ShrinkWrap2: Don't search all of them.
      for (MachineRegion *MR : MaximalRegions) {
        auto Found = std::find_if(
            MaximalRegions.begin(), MaximalRegions.end(),
            [MR](MachineRegion *M) { return MR->getExit() == M->getEntry(); });
        if (Found != MaximalRegions.end()) {
          MergeChanged = true;
          Changed = true;
          mergeMaximalRegions(MR, *Found);
          break;
        }
      }
    }

    SmallVector<RegionPair, 4> ToRemove;
    // Look for sub-regions that are post dominated by the exit and dominated by
    // the entry of a maximal region, and remove them in order to have only
    // maximal regions.
    // FIXME: ShrinkWrap2: Avoid double pass. DF?
    DEBUG(RegionInfo->dump());
    for (MachineRegion *Parent : MaximalRegions) {
      for (const std::unique_ptr<MachineRegion> &Child : *Parent) {
        if (Child->getEntry() == Parent->getEntry() &&
            (MPDT->dominates(Parent->getExit(), Child->getExit()) ||
             Parent->isTopLevelRegion())) {
          DEBUG(dbgs() << "(MPDT) Schedule for erasing : "
                       << Child->getNameStr() << '\n');
          ToRemove.emplace_back(Parent, Child.get());
        }
        if (Child->getExit() == Parent->getExit() &&
            MDT->dominates(Parent->getEntry(), Child->getEntry())) {
          DEBUG(dbgs() << "(MDT) Schedule for erasing : " << Child->getNameStr()
                       << '\n');
          ToRemove.emplace_back(Parent, Child.get());
        }
      }
    }

    // We need to remove this after the loop so that we don't invalidate
    // iterators.
    Changed |= !ToRemove.empty();
    // Sort by depth. Start with the deepest regions, to preserve the outer
    // regions.
    std::sort(ToRemove.begin(), ToRemove.end(), [](RegionPair A, RegionPair B) {
      return A.second->getDepth() > B.second->getDepth();
    });

    // Remove the regions and transfer all children.
    for (auto &KV : ToRemove) {
      MachineRegion *Parent = KV.first;
      MachineRegion *Child = KV.second;
      removeMaximalRegion(Parent, Child);
    }
  }

  // Reconstruct the MRegion -> MBB association inside the RegionInfo.
  // FIXME: ShrinkWrap2: Quick and dirty.
  for (MachineBasicBlock &MBB : *Top->getEntry()->getParent()) {
    int Depth = std::numeric_limits<int>::min();
    MachineRegion *Final = nullptr;
    for (MachineRegion *MR : MaximalRegions) {
      if (MR->contains(&MBB) || MR->getExit() == &MBB) {
        if (static_cast<int>(MR->getDepth()) > Depth) {
          Depth = MR->getDepth();
          Final = MR;
        }
      }
    }
    assert(Depth >= 0 && "Minimal depth is the top level region, 0.");
    RegionInfo->setRegionFor(&MBB, Final);
  }

  DEBUG(for (MachineRegion *R : MaximalRegions) R->print(dbgs(), false););
}

void ShrinkWrap2::removeMaximalRegion(MachineRegion *Parent,
                                      MachineRegion *Child) {
  DEBUG(dbgs() << "Erasing region: " << Child->getNameStr() << '\n');
  MaximalRegions.erase(Child);
  Child->transferChildrenTo(Parent);
  Parent->removeSubRegion(Child);
}

void ShrinkWrap2::mergeMaximalRegions(MachineRegion *Entry,
                                      MachineRegion *Exit) {
  assert(Entry->getExit() == Exit->getEntry() &&
         "Only merging of regions that share entry(Exit) & exit(Entry) block!");

  DEBUG(dbgs() << "Merging: " << Entry->getNameStr() << " with "
               << Exit->getNameStr());

  // Create a region starting at the entry of the first block, ending at the
  // exit of the last block.
  // FIXME: ShrinkWrap2: Leak.
  // FIXME: ShrinkWrap2: Use MachineRegionInfo->create?
  MachineRegion *NewRegion =
      new MachineRegion(Entry->getEntry(), Exit->getExit(), RegionInfo, MDT);
  MaximalRegions.insert(NewRegion);
  // Attach this region to the common parent of both Entry and Exit
  // blocks.
  MachineRegion *NewParent = RegionInfo->getCommonRegion(Entry, Exit);
  NewParent->addSubRegion(NewRegion);

  // Remove the subregions that were merged.
  removeMaximalRegion(Entry->getParent(), Entry);
  removeMaximalRegion(Exit->getParent(), Exit);
}

void ShrinkWrap2::postProcessResults(MachineFunction &MF) {
  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();
  // If there is only one use of the register, and multiple saves / restores,
  // remove them and place the save / restore at the used MBB.
  FOREACH_BIT(Reg, Regs) {
    unsigned Uses =
        std::count_if(UsedCSR.begin(), UsedCSR.end(),
                      [&](const decltype(UsedCSR)::value_type &CSR) {
                        return CSR.second.test(Reg);
                      });
    if (Uses == 1) {
      // Gather all the saves.
      SmallVector<BBRegSetMap::iterator, 4> SavesReg;
      for (auto It = Saves.begin(), End = Saves.end(); It != End; ++It)
        if (It->second.test(Reg))
          SavesReg.push_back(It);

      // Gather all the restores.
      SmallVector<BBRegSetMap::iterator, 4> RestoresReg;
      for (auto It = Restores.begin(), End = Restores.end(); It != End; ++It)
        if (It->second.test(Reg))
          RestoresReg.push_back(It);

      if (SavesReg.size() == 1 && RestoresReg.size() == 1)
        continue;

      // Remove the register from the sets.
      for (auto *Set : {&SavesReg, &RestoresReg})
        for (BBRegSetMap::iterator It : *Set)
          It->second.reset(Reg);

      auto It = std::find_if(UsedCSR.begin(), UsedCSR.end(),
                             [&](decltype(UsedCSR)::value_type &CSR) {
                               return CSR.second.test(Reg);
                             });
      // FIXME: ShrinkWrap2: const_cast.
      MachineBasicBlock *MBB = const_cast<MachineBasicBlock *>(It->first);

      for (auto *Map : {&Saves, &Restores}) {
        BitVector &Regs = (*Map)[MBB];
        if (Regs.empty())
          Regs.resize(TRI.getNumRegs());
        Regs.set(Reg);
      }
    }
  }

  // Remove all the empty entries from the Saves / Restores maps.
  // FIXME: ShrinkWrap2: Should we even have empty entries?
  SmallVector<BBRegSetMap::iterator, 4> ToRemove;
  for (auto *Map : {&Saves, &Restores}) {
    for (auto It = Map->begin(), End = Map->end(); It != End; ++It)
      if (It->second.count() == 0)
        ToRemove.push_back(It);
    for (auto It : ToRemove)
      Map->erase(It);
    ToRemove.clear();
  }
}

void ShrinkWrap2::moveAtRegionBoundaries(MachineFunction &MF) {
  BBSet ToRemove;
  std::set<std::pair<MachineBasicBlock *, const BitVector *>> ToInsert;

  auto FlushInsertRemoveQueue = [&](BBRegSetMap &Map) {
    // Move registers to the new basic block.
    for (auto &KV : ToInsert) {
      MachineBasicBlock *MBB = KV.first;
      const BitVector &Regs = *KV.second;
      Map[MBB] |= Regs;
    }
    ToInsert.clear();

    // Remove the basic blocks.
    for (MachineBasicBlock *MBB : ToRemove)
      Map.erase(MBB);
    ToRemove.clear();
  };

  // For each saving block, find the smallest region that contains it, and move
  // the saves at the entry of the region.
  // FIXME: ShrinkWrap2: Cost model.
  for (auto &KV : Saves) {
    MachineBasicBlock *MBB = KV.first;
    const BitVector &Regs = KV.second;

    MachineRegion *MR = RegionInfo->getRegionFor(MBB);
    assert(MR);
    if (MBB == MR->getEntry())
      continue;
    DEBUG(dbgs() << "Moving saves from " << MBB->getNumber() << " to "
                 << MR->getEntry()->getNumber() << '\n');
    ToInsert.emplace(MR->getEntry(), &Regs);
    ToRemove.insert(MBB);
    // If we're moving to top level, and there is no restore related to this
    // save, add a restore at return blocks. This can happen in the following
    // case:
    //      0 ---> 2
    //      |
    //      |
    //      v ->
    //      1    3 // infinite loop
    //        <-
    // if there is an use of a CSR for the basic block 1.
    if (MR->isTopLevelRegion()) {
      FOREACH_BIT(Reg, Regs) {
        unsigned NumSaves = std::count_if(
            Saves.begin(), Saves.end(),
            [&](const std::pair<MachineBasicBlock *, BitVector> &KV) {
              return KV.second.test(Reg);
            });
        bool HasRestores =
            std::find_if(
                Restores.begin(), Restores.end(),
                [&](const std::pair<MachineBasicBlock *, BitVector> &KV) {
                  return KV.second.test(Reg);
                }) != Restores.end();
        if (NumSaves == 1 && !HasRestores) {
          // Restore in all the return blocks.
          for (MachineBasicBlock &MBB : MF) {
            if (MBB.isReturnBlock()) {
              BitVector &RestoresMBB = Restores[&MBB];
              if (RestoresMBB.empty()) {
                const TargetRegisterInfo &TRI =
                    *MF.getSubtarget().getRegisterInfo();
                RestoresMBB.resize(TRI.getNumRegs());
              }
              RestoresMBB.set(Reg);
            }
          }
        }
      }
    }
  }

  // Actually apply the insert / removes to the saves.
  FlushInsertRemoveQueue(Saves);

  // For each restoring block, find the smallest region that contains it and
  // move the saves at the entry of the region.
  // FIXME: ShrinkWrap2: Cost model.
  for (auto &KV : Restores) {
    MachineBasicBlock *MBB = KV.first;
    BitVector &Regs = KV.second;

    MachineRegion *MR = RegionInfo->getRegionFor(MBB);
    assert(MR);
    // Corner case of the top level region. The restores have to be added on all
    // the terminator blocks.
    if (MR->isTopLevelRegion()) {
      // FIXME: ShrinkWrap2: Better way to do this?
      for (MachineBasicBlock &Block : MF) {
        if (Block.isReturnBlock()) {
          if (MBB == &Block)
            continue;
          DEBUG(dbgs() << "Moving restores from " << MBB->getNumber() << " to "
                       << Block.getNumber() << '\n');
          ToInsert.emplace(&Block, &Regs);
          if (!MBB->isReturnBlock())
            ToRemove.insert(MBB);
        }
      }
    } else {
      if (MBB == MR->getExit())
        continue;
      DEBUG(dbgs() << "Moving restores from " << MBB->getNumber() << " to "
                   << MR->getExit()->getNumber() << '\n');
      ToInsert.emplace(MR->getExit(), &Regs);
      ToRemove.insert(MBB);
    }
  }

  // Actually apply the insert / removes to the restores.
  FlushInsertRemoveQueue(Restores);

  // Eliminate nested saves / restores.
  // FIXME: ShrinkWrap2: Is this really needed? How do we end up here?
  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();
  auto EliminateNested = [&TRI, this](
      BBRegSetMap &Map, MachineBasicBlock *(MachineRegion::*Member)() const) {
    for (auto &KV : Map) {
      MachineBasicBlock *MBB = KV.first;
      BitVector &Regs = KV.second;

      const MachineRegion *Parent = RegionInfo->getRegionFor(MBB);
      DEBUG(dbgs() << "Region for BB#" << MBB->getNumber() << " : "
                   << Parent->getNameStr() << '\n');
      for (const std::unique_ptr<MachineRegion> &Child : *Parent) {
        auto ChildToo = Map.find((Child.get()->*Member)());
        if (ChildToo != Map.end()) {
          BitVector &ChildRegs = ChildToo->second;
          if (&Regs == &ChildRegs)
            continue;
          // For each saved register in the parent, remove them from the
          // children.

          FOREACH_BIT(Reg, Regs) {
            if (ChildRegs[Reg]) {
              DEBUG(dbgs() << "Both " << (Child.get()->*Member)()->getNumber()
                           << " and " << MBB->getNumber() << " save / restore "
                           << PrintReg(Reg, &TRI) << '\n');
              ChildRegs[Reg] = false;
              if (ChildRegs.empty())
                Map.erase(ChildToo);
            }
          }
        }
      }
    }
  };

  EliminateNested(Saves, &MachineRegion::getEntry);
  EliminateNested(Restores, &MachineRegion::getExit);
}

void ShrinkWrap2::returnToMachineFrame(MachineFunction &MF) {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  auto Transform = [](BBRegSetMap &Src, MachineFrameInfo::CalleeSavedMap &Dst) {
    for (auto &KV : Src) {
      MachineBasicBlock *MBB = KV.first;
      BitVector &Regs = KV.second;
      std::vector<CalleeSavedInfo> &CSI = Dst[MBB];

      FOREACH_BIT(Reg, Regs)
      CSI.emplace_back(Reg);
    }
  };
  Transform(Saves, MFI.getSaves());
  Transform(Restores, MFI.getRestores());
}

void ShrinkWrap2::verifySavesRestores(MachineFunction &MF) const {
  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();

  auto HasReg = [&](const BBRegSetMap &Map, unsigned Reg) {
    return std::find_if(
               Map.begin(), Map.end(),
               [&](const std::pair<MachineBasicBlock *, BitVector> &KV) {
                 return KV.second[Reg];
               }) != Map.end();

  };

  auto RestoresReg = [&](unsigned Reg) { return HasReg(Restores, Reg); };
  auto SavesReg = [&](unsigned Reg) { return HasReg(Saves, Reg); };

  // Check that all the CSRs used in the function are saved at least once.
  FOREACH_BIT(Reg, Regs) {
    assert(SavesReg(Reg) || RestoresReg(Reg) && "Used CSR is never saved!");
  }

  // Check that there are no saves / restores in a loop.
  for (const BBRegSetMap *Map : {&Saves, &Restores}) {
    for (auto &KV : *Map) {
      MachineBasicBlock *MBB = KV.first;
      assert(!MLI->getLoopFor(MBB) && "Save / restore in a loop.");
    }
  }

  // Keep track of the currently saved registers.
  BitVector Saved{TRI.getNumRegs()};
  // Cache the state of each call, to avoid redundant checks.
  DenseMap<MachineBasicBlock *, SmallVector<BitVector, 2>> Cache;

  // Verify if:
  // * All the saves are restored.
  // * All the restores are related to a store.
  // * There are no nested stores.
  std::function<void(MachineBasicBlock *)> verifySavesRestoresRec = [&](
      MachineBasicBlock *MBB) {
    // Don't even check unreachable blocks.
    if (MBB->succ_empty() && !MBB->isReturnBlock()) {
      VERBOSE_DEBUG(dbgs() << "IN: BB#" << MBB->getNumber()
                           << " is an unreachable\n");
      return;
    }

    SmallVectorImpl<BitVector> &State = Cache[MBB];
    if (std::find(State.begin(), State.end(), Saved) != State.end()) {
      VERBOSE_DEBUG(dbgs() << "IN: BB#" << MBB->getNumber()
                           << " already visited.\n");
      return;
    } else
      State.push_back(Saved);

    VERBOSE_DEBUG(
        dbgs() << "IN: BB#" << MBB->getNumber() << ": ";
        FOREACH_BIT(Reg, Saved) { dbgs() << PrintReg(Reg, &TRI) << " "; } dbgs()
        << '\n');

    const BitVector &SavesMBB = Saves.lookup(MBB);
    const BitVector &RestoresMBB = Restores.lookup(MBB);

    // Get the intersection of the currently saved registers and the
    // registers to be saved for this basic block. If the intersection is
    // not empty, it means we have nested saves for the same register.
    BitVector Intersection{SavesMBB};
    Intersection &= Saved;

    FOREACH_BIT(Reg, Intersection) {
      DEBUG(dbgs() << PrintReg(Reg, &TRI) << " is saved twice.\n");
    }

    assert(Intersection.count() == 0 && "Nested saves for the same register.");
    Intersection.clear();

    // Save the registers to be saved.
    FOREACH_BIT(Reg, SavesMBB) {
      Saved.set(Reg);
      VERBOSE_DEBUG(dbgs() << "IN: BB#" << MBB->getNumber() << ": Save "
                           << PrintReg(Reg, &TRI) << ".\n");
    }

    // If the intersection of the currently saved registers and the
    // registers to be restored for this basic block is not equal to the
    // restores, it means we are trying to restore something that is not
    // saved.
    Intersection = RestoresMBB;
    Intersection &= Saved;

    assert(Intersection.count() == RestoresMBB.count() &&
           "Not all restores are saved.");

    // Restore the registers to be restored.
    FOREACH_BIT(Reg, RestoresMBB) {
      Saved.reset(Reg);
      VERBOSE_DEBUG(dbgs() << "IN: BB#" << MBB->getNumber() << ": Restore "
                           << PrintReg(Reg, &TRI) << ".\n");
    }

    if (MBB->succ_empty() && Saved.count() != 0)
      llvm_unreachable("Not all saves are restored.");

    // Using the current set of saved registers, walk all the successors
    // recursively.
    for (MachineBasicBlock *Succ : MBB->successors())
      verifySavesRestoresRec(Succ);

    // Restore the state prior of the function exit.
    FOREACH_BIT(Reg, RestoresMBB) {
      Saved.set(Reg);
      VERBOSE_DEBUG(dbgs() << "OUT: BB#" << MBB->getNumber() << ": Save "
                           << PrintReg(Reg, &TRI) << ".\n");
    }
    FOREACH_BIT(Reg, SavesMBB) {
      Saved.reset(Reg);
      VERBOSE_DEBUG(dbgs() << "OUT: BB#" << MBB->getNumber() << ": Restore "
                           << PrintReg(Reg, &TRI) << ".\n");
    }
  };

  verifySavesRestoresRec(&MF.front());
}

void ShrinkWrap2::dumpResults(MachineFunction &MF) const {
  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();

  for (MachineBasicBlock &MBB : MF) {
    if (Saves.count(&MBB) || Restores.count(&MBB)) {
      DEBUG(dbgs() << "BB#" << MBB.getNumber() << ": Saves: ");
      FOREACH_BIT(Reg, Saves.lookup(&MBB)) {
        DEBUG(dbgs() << PrintReg(Reg, &TRI) << ", ");
      }
      DEBUG(dbgs() << "| Restores: ");
      FOREACH_BIT(Reg, Restores.lookup(&MBB)) {
        DEBUG(dbgs() << PrintReg(Reg, &TRI) << ", ");
      }

      DEBUG(dbgs() << '\n');
    }
  }
}

bool ShrinkWrap2::splitCriticalEdges(MachineFunction &MF) {
  auto IsCriticalEdge = [&](MachineBasicBlock *Src, MachineBasicBlock *Dst) {
    return Src->succ_size() > 1 && Dst->pred_size() > 1;
  };

  bool Changed = true;
  while (Changed) {
    Changed = false;
    SmallVector<std::pair<MachineBasicBlock *, MachineBasicBlock *>, 4> ToSplit;
    for (MachineBasicBlock &MBB : MF) {
      for (MachineBasicBlock *Succ : MBB.successors()) {
        if (IsCriticalEdge(&MBB, Succ)) {
          ToSplit.push_back({&MBB, Succ});
          VERBOSE_DEBUG(dbgs() << "Critical edge detected. Split.\n");
        }
      }
    }

    for (std::pair<MachineBasicBlock *, MachineBasicBlock *> Split : ToSplit)
      if (Split.first->SplitCriticalEdge(Split.second, *this))
        Changed = true;
  }
  return false;
}

void ShrinkWrap2::markUsesInsideLoops(MachineFunction &MF) {
  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();

  // Keep track of the registers to attach to a basic block.
  DenseMap<MachineBasicBlock *, BitVector> ToInsert;
  for (auto It = UsedCSR.begin(), End = UsedCSR.end(); It != End; ++It) {
    const MachineBasicBlock *MBB = It->first;
    BitVector &Regs = It->second;

    if (MachineLoop *Loop = MLI->getLoopFor(MBB)) {
      DEBUG(dbgs() << "Loop for CSR: BB#" << MBB->getNumber() << '\n');

      // Get the most top loop.
      while (Loop->getParentLoop())
        Loop = Loop->getParentLoop();

      VERBOSE_DEBUG(Loop->dump());

      auto Mark = [&](MachineBasicBlock *Block) {
        if (ToInsert[Block].empty())
          ToInsert[Block].resize(TRI.getNumRegs());
        ToInsert[Block] |= Regs;
        VERBOSE_DEBUG(dbgs() << "Mark: BB#" << Block->getNumber() << '\n');
      };

      // Mark all around the top block.
      MachineBasicBlock *Top = Loop->getTopBlock();
      for (auto &Around : {Top->successors(), Top->predecessors()})
        for (MachineBasicBlock *Block : Around)
          Mark(Block);

      // Mark all the blocks of the loop.
      for (MachineBasicBlock *Block : Loop->getBlocks())
        Mark(Block);

      // Mark all the exit blocks of the loop.
      SmallVector<MachineBasicBlock *, 4> ExitBlocks;
      Loop->getExitBlocks(ExitBlocks);
      for (MachineBasicBlock *Exit : ExitBlocks)
        Mark(Exit);
    }
  }

  for (auto &Pair : ToInsert)
    UsedCSR[Pair.first] |= Pair.second;
}

static bool shouldShrinkWrap(MachineFunction &MF) {
  // No exceptions.
  if (MF.empty() || MF.hasEHFunclets() || MF.callsUnwindInit() ||
      MF.callsEHReturn())
    return false;

  for (MachineBasicBlock &MBB : MF) {
    // No exceptions.
    if (MBB.isEHPad() || MBB.isEHFuncletEntry())
      return false;

    // No indirect branches. This is required in order to be able to split
    // critical edges.
    for (MachineInstr &MI : MBB)
      if (MI.isIndirectBranch())
        return false;
  }
  return true;
}

bool ShrinkWrap2::runOnMachineFunction(MachineFunction &MF) {
  if (skipFunction(*MF.getFunction()) ||
      EnableShrinkWrap2Opt == cl::BOU_FALSE || !shouldShrinkWrap(MF))
    return false;

  DEBUG(dbgs() << "**** Analysing " << MF.getName() << '\n');

  if (ViewCFGDebug == cl::BOU_TRUE)
    VERBOSE_DEBUG(MF.viewCFGOnly());

  // Initialize the dependencies.
  init(MF);

  // FIXME: ShrinkWrap2: Sadly, I we have to split critical edges before looking
  // for all the used CSRs, since liveness analysis is impacted. This means that
  // even for programs with no CSRs used, we have to split all the critical
  // edges.
  splitCriticalEdges(MF);

  // Determine all the used callee saved registers. If there are none, avoid
  // running this pass.
  determineCalleeSaves(MF);
  DEBUG(dumpUsedCSR(MF));

  // Don't bother saving if we know we're never going to return.
  removeUsesOnUnreachablePaths(MF);
  DEBUG(dumpUsedCSR(MF));
  if (UsedCSR.empty())
    return false;

  const TargetRegisterInfo &TRI = *MF.getSubtarget().getRegisterInfo();
  // Collect all the CSRs.
  Regs.resize(TRI.getNumRegs());
  for (auto &KV : UsedCSR)
    Regs |= KV.second;


  if (ViewCFGDebug == cl::BOU_TRUE)
    DEBUG(MF.viewCFGOnly());

  DEBUG(dumpUsedCSR(MF));

  // If there are any loops, mark the uses inside.
  if (!MLI->empty())
    markUsesInsideLoops(MF);

  VERBOSE_DEBUG(dumpUsedCSR(MF));

  // Use this scope to get rid of the dataflow attributes after the computation.
  {
    // Compute the dataflow attributes described by Fred C. Chow.
    DataflowAttributes Attr{MF};
    Attr.populate(UsedCSR);
    // For each register, compute the data flow attributes.
    FOREACH_BIT(Reg, Regs) {
      // FIXME: ShrinkWrap2: Avoid recomputing all the saves / restores.
      for (auto *Map : {&Saves, &Restores}) {
        for (auto &KV : *Map) {
          BitVector &Regs = KV.second;
          Regs.reset(Reg);
        }
      }
      // Compute the attributes.
      Attr.compute(Reg);
      // Gather the results.
      Attr.results(Reg, Saves, Restores);
      VERBOSE_DEBUG(dumpResults(MF));
    }
  }

  DEBUG(dbgs() << "******** Before regions\n";);
  DEBUG(dumpResults(MF));

  if (UseRegions != cl::BOU_FALSE) {
    DEBUG(dbgs() << "******** Optimize using regions\n";);
    gatherAllRegions(RegionInfo->getTopLevelRegion());
    computeMaximalRegions(RegionInfo->getTopLevelRegion());
    DEBUG(RegionInfo->dump());
    moveAtRegionBoundaries(MF);
    DEBUG(dbgs() << "******** After regions\n";);
    DEBUG(dumpResults(MF));
  }

  postProcessResults(MF);

  DEBUG(dbgs() << "**** Shrink-wrapping results ****\n");
  DEBUG(dumpResults(MF));

  // FIXME: ShrinkWrap2: Enable EXPENSIVE_CHECKS.
  //#ifdef EXPENSIVE_CHECKS
  verifySavesRestores(MF);
  //#endif // EXPENSIVE_CHECKS

  // FIXME: ShrinkWrap2: Should merge the behaviour in PEI?
  // If there is only one save block, and it's the first one, don't forward
  // anything to the MachineFrameInfo.
  if (!(Saves.size() == 1 && Saves.begin()->first == &MF.front()))
    returnToMachineFrame(MF);
  else
    DEBUG(dbgs() << "No shrink-wrapping results.\n");

  clear();

  return false;
}
