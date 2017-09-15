//===-- MemorySSAAliasSets.h - Alias sets using MemorySSA -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This a a helper class that creates alias sets using MemorySSA, each
// set being marked with properties that enablei/disable promotion.
// It is used for promotion in the Loop Invariant Code Motion Pass.
// It also stores insertion points into MemorySSA for a particular loop.
// These need to be reset if the sets are reused.
//===----------------------------------------------------------------------===//
#ifndef LLVM_ANALYSIS_MEMORYSSA_ALIASSETS_H
#define LLVM_ANALYSIS_MEMORYSSA_ALIASSETS_H

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace llvm {
class MemorySSAAliasSets {
public:
  MemorySSA *MSSA;
  MemorySSAUpdater *MSSAUpdater;

private:
  Loop *L;
  LoopInfo *LI;
  AliasAnalysis *AA;
  DominatorTree *DT;
  SmallVector<MemoryAccess *, 4> MSSAInsertPts;

  class SingleMustAliasVec {
    SmallSetVector<Value *, 8> Vec;
    MemoryLocation Loc;

  public:
    // CanNeverPromote trumps everything else
    // Set is either may-alias, has a volatile store or aliases an instruction
    // witout a MemoryLocation.
    bool CanNeverPromote = false;
    bool LoopInvariantAccess = false;
    bool HasAGoodStore = false;
    SingleMustAliasVec(MemoryLocation loc) : Loc(loc), CanNeverPromote(false) {
      Vec.insert(const_cast<Value *>(Loc.Ptr));
    }
    void addMemoryLocation(Optional<MemoryLocation> MemLoc) {
      if (MemLoc == None)
        return;
      Vec.insert(const_cast<Value *>(MemLoc.getValue().Ptr));
    }
    MemoryLocation getMemoryLocation() { return Loc; }
    SmallSetVector<Value *, 8> *getVecRef() { return &Vec; }
    bool isGoodForPromotion() {
      return !CanNeverPromote && LoopInvariantAccess && HasAGoodStore;
    }
    void setMemoryLocation(Value *LI, Value *Replacement) {
      Loc = MemoryLocation(Replacement, Loc.Size, Loc.AATags);
      Vec.remove(LI);
      Vec.insert(Replacement);
    }
  };

  SmallVector<SingleMustAliasVec *, 4> AllSets;
  DenseMap<const MemoryAccess *, SingleMustAliasVec *> StoredDefs;

  // FIXME: This should not need to query AA, MemorySSA should cache this.
  AliasResult alias(Optional<MemoryLocation> MemLocI, Instruction *InstrI,
                    MemoryLocation &MemLocJ) {
    if (MemLocI == None) {
      ModRefInfo ModRef = AA->getModRefInfo(InstrI, MemLocJ);
      if (bool(ModRef & MRI_Mod)) {
        return MustAlias;
      }
      if (bool(ModRef & MRI_Ref)) {
        return MayAlias;
      }
      return NoAlias;
    } else {
      AliasResult Result = AA->alias(MemLocI.getValue(), MemLocJ);
      return Result;
    }
  }

  void addToAliasSets(const MemoryUse *Use,
                      SmallPtrSetImpl<const MemoryAccess *> &Processed) {
    Processed.insert(Use);
    // Follow-up to the BIG NOTE:
    // Because we allowed some imprecision, it is possible that the
    // definingAccess is not the clobberingAccess. We *really* want the
    // correct aliasing definition here, otherwise we're creating a set with
    // things that don't alias.
    // This is currently visible in "LoopSimplify/ashr-crash.ll"(Puts @a,@c,@d
    // in the same set because its uses all point to the newly promoted Def)
    // We *have to* use getClobberingAccess here for correctness!
    // To avoid incurring the performance penalty of complete info, we can
    // either:
    // a) Create sets in BB order, without walking MemoryAccess uses
    // recursively. Then, we'll need to check alias information with all
    // existing sets and cap the number of sets created, much like the
    // AliasSetTracker :/ Working version available, but, well, yuck! b) Figure
    // out why the MemorySSA uses are not updated to be accurate after a
    // promotion. The problem here is that when MemorySSA reaches the
    // "memssa-check-limit", results will still be incorrect.
    // c) Add another way to cap this in MemorySSA.
    // e.g. Query MemorySSA on how many calls to getClobberingMemoryAccess have
    // passed the "memssa-check-limit", and cap that.

    // MemoryDef* DefiningAcc =
    // dyn_cast_or_null<MemoryDef>(Use->getDefiningAccess());
    MemoryDef *DefiningAcc = dyn_cast_or_null<MemoryDef>(
        MSSA->getWalker()->getClobberingMemoryAccess(
            const_cast<MemoryUse *>(Use)));
    if (!DefiningAcc || !DefiningAcc->getMemoryInst())
      return;

    SingleMustAliasVec *OneAliasSet = StoredDefs[DefiningAcc];
    if (!OneAliasSet)
      return;

    Instruction *InstrI = Use->getMemoryInst();
    Optional<MemoryLocation> MemLocI = MemoryLocation::getOrNone(InstrI);
    MemoryLocation MemLocJ = OneAliasSet->getMemoryLocation();
    AliasResult Result = alias(MemLocI, InstrI, MemLocJ);
    if (Result == MayAlias) {
      OneAliasSet->CanNeverPromote = true;
    }
    OneAliasSet->addMemoryLocation(MemLocI);
  }

  void addDefsHelper(MemoryLocation &MemLocJ,
                     const MemoryAccess *DefOrUseOfADef,
                     SingleMustAliasVec *newEntry,
                     SmallPtrSetImpl<const MemoryAccess *> &Processed,
                     SmallPtrSetImpl<const MemoryAccess *> &Phis,
                     bool skipUses) {
    for (const User *U : DefOrUseOfADef->users()) {
      /// No need ot look at ProcessedPhis; need to looks at ProcessedDefs!
      if (Phis.count(dyn_cast<MemoryAccess>(U)))
        continue;

      /// Process MemoryPhis
      const MemoryPhi *Phi = dyn_cast<MemoryPhi>(U);
      if (Phi && L->contains(Phi->getBlock())) {
        Phis.insert(Phi);
        addDefsHelper(MemLocJ, Phi, newEntry, Processed, Phis, true);
        continue;
      }

      const MemoryUseOrDef *UseOrDef = dyn_cast<MemoryUseOrDef>(U);
      if (!UseOrDef || !UseOrDef->getMemoryInst()) {
        continue; /*FIXME: What does this mean? U=unknown value.*/
      }
      Instruction *InstrI = UseOrDef->getMemoryInst();
      if (!InstrI || !L->contains(InstrI))
        continue;
      Optional<MemoryLocation> MemLocI = MemoryLocation::getOrNone(InstrI);
      bool LoopInvariantAccess =
          ((MemLocI != None) && L->isLoopInvariant(MemLocI.getValue().Ptr));

      /// Process MemoryDefs
      const MemoryDef *UserDef = dyn_cast_or_null<MemoryDef>(UseOrDef);
      if (UserDef) {
        AliasResult Result = alias(MemLocI, InstrI, MemLocJ);
        if (Result == MayAlias || Result == MustAlias) {
          Processed.insert(UseOrDef);
          if (MemLocI != None)
            newEntry->addMemoryLocation(MemLocI.getValue());
          else // Cannot promote stuff with no memory locations
            newEntry->CanNeverPromote = true;
          StoredDefs[UseOrDef] = newEntry;

          StoreInst *USIn = dyn_cast_or_null<StoreInst>(InstrI);

          // TODO: minor optimization: only check if !CanNeverPromote
          if (Result == MayAlias || (USIn && USIn->isVolatile()))
            newEntry->CanNeverPromote = true;
          if (USIn && !USIn->isVolatile())
            newEntry->HasAGoodStore = true;
          newEntry->LoopInvariantAccess |= LoopInvariantAccess;
        } else {
          addDefsHelper(MemLocJ, UserDef, newEntry, Processed, Phis, true);
        }
        continue;
      }

      if (skipUses)
        continue;

      /// Process MemoryUses
      const MemoryUse *UserUse = dyn_cast_or_null<MemoryUse>(UseOrDef);
      if (UserUse) {
        Processed.insert(UseOrDef);
        if (MemLocI != None)
          newEntry->addMemoryLocation(MemLocI.getValue());

        if (!newEntry->CanNeverPromote) {
          AliasResult Result = alias(MemLocI, InstrI, MemLocJ);
          if (Result == MayAlias)
            newEntry->CanNeverPromote = true;
          newEntry->LoopInvariantAccess |= LoopInvariantAccess;
        }
      }
    }
  }

  void addToAliasSets(const MemoryDef *Def,
                      SmallPtrSetImpl<const MemoryAccess *> &Processed) {
    Processed.insert(Def);
    Instruction *InstrJ = Def->getMemoryInst();
    Optional<MemoryLocation> OptMemLocJ = MemoryLocation::getOrNone(InstrJ);
    if (OptMemLocJ == None)
      return;
    MemoryLocation MemLocJ = OptMemLocJ.getValue();

    StoreInst *SIn = dyn_cast_or_null<StoreInst>(InstrJ);
    SingleMustAliasVec *newEntry = new SingleMustAliasVec(MemLocJ);
    newEntry->CanNeverPromote = SIn && SIn->isVolatile();
    newEntry->LoopInvariantAccess = L->isLoopInvariant(MemLocJ.Ptr);
    newEntry->HasAGoodStore = SIn && !SIn->isVolatile();

    SmallPtrSet<const MemoryAccess *, 32> Phis;
    addDefsHelper(MemLocJ, Def, newEntry, Processed, Phis, false);
    AllSets.push_back(newEntry);
  }

public:
  MemorySSAAliasSets(MemorySSA *mssa, MemorySSAUpdater *mssaupdater, Loop *l,
                     LoopInfo *li, AliasAnalysis *aa, DominatorTree *dt,
                     SmallVectorImpl<BasicBlock *> &ExitBlocks)
      : MSSA(mssa), MSSAUpdater(mssaupdater), L(l), LI(li), AA(aa), DT(dt),
        AllSets() {
    resetMSSAInsertPts(ExitBlocks);
  }

  void resetMSSAInsertPts(SmallVectorImpl<BasicBlock *> &ExitBlocks) {
    // TODO: Better way to fill this in.
    MSSAInsertPts.reserve(ExitBlocks.size());
    for (unsigned I = 0; I < ExitBlocks.size(); ++I) {
      MSSAInsertPts.push_back(nullptr);
    }
  }

  // Creates all alias sets in L
  void createSets() {
    SmallPtrSet<const MemoryAccess *, 32> Processed;
    addToSets(DT->getNode(L->getHeader()), Processed);
  }

  void addToSets(DomTreeNode *N,
                 SmallPtrSetImpl<const MemoryAccess *> &Processed) {
    // We want to visit parents before children. We will enque all the parents
    // before their children in the worklist and process the worklist in order.
    SmallVector<DomTreeNode *, 16> Worklist = collectChildrenInLoop(N, L);

    for (DomTreeNode *DTN : Worklist) {
      BasicBlock *BB = DTN->getBlock();
      auto *Accesses = MSSA->getBlockAccesses(BB);
      if (Accesses)
        for (auto &Acc : make_range(Accesses->begin(), Accesses->end())) {
          if (Processed.count(&Acc))
            continue;
          // Process uses
          const MemoryUse *Use = dyn_cast_or_null<MemoryUse>(&Acc);
          if (Use)
            addToAliasSets(Use, Processed);
          // Process defs
          const MemoryDef *Def = dyn_cast_or_null<MemoryDef>(&Acc);
          if (Def)
            addToAliasSets(Def, Processed);
        }
    }
  }

  SmallSetVector<Value *, 8> *getSetIfPromotableOrNull(unsigned I) {
    if (AllSets[I]->isGoodForPromotion()) {
      SmallSetVector<Value *, 8> *MustAliasPtrs = AllSets[I]->getVecRef();
      assert(MustAliasPtrs->size() > 0 && "Alias set cannot be empty!!!\n");
      return MustAliasPtrs;
    }
    return nullptr;
  }

  unsigned size() { return AllSets.size(); }

  void replace(Value *LoadI, Value *Replacement) {
    for (auto OneAliasSet : AllSets) {
      MemoryLocation MemLocJ = OneAliasSet->getMemoryLocation();
      if (MemLocJ.Ptr == LoadI) {
        OneAliasSet->setMemoryLocation(LoadI, Replacement);
        if (!OneAliasSet->CanNeverPromote && OneAliasSet->HasAGoodStore &&
            !OneAliasSet->LoopInvariantAccess &&
            L->isLoopInvariant(Replacement)) {
          OneAliasSet->LoopInvariantAccess = true;
        }
      }
    }
  }

  MemoryAccess *getInsertPoint(int I) { return MSSAInsertPts[I]; }
  void setInsertPoint(MemoryAccess *Acc, int I) { MSSAInsertPts[I] = Acc; }
};
} // end namespace llvm

#endif // LLVM_ANALYSIS_MEMORYSSA_ALIASSETS_H
