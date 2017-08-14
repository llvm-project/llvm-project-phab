//===- LoopVectorizePred.cpp - A Pass to be run before Loop Vectorizer
//------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This is the LLVM pass which is intended to be run before loop vectorizer.
// This pass checks if there is a backward dependence between instructions, and
// if so, then it tries to reorder instructions, so as to convert
// non-vectorizable loop into vectorizable form. The pass uses
// LoopAccessAnalysis and MemorySSA analysis to check for dependences, in order
// to reorder the instructions.
// For example, the pass reorders the code below, in order to convert it into
// vectorizable form.

// Before applying the pass
// for (int i = 0; i < n; i++) {
//   a[i] = b[i];
//   c[i] = a[i + 1];
// }
//
// After applying the pass
// for (int i = 0; i < n; i++) {
//   c[i] = a[i + 1];
//   a[i] = b[i];
// }

//===----------------------------------------------------------------
#include "llvm/Transforms/Vectorize/LoopVectorizePred.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopIterator.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include "llvm/Analysis/ScalarEvolutionExpander.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/LoopSimplify.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Vectorize.h"
#include "llvm/Transforms/Utils/OrderedInstructions.h"
#include "queue"

using namespace llvm;

#define LV_NAME "loop-vectorize-pred"
#define DEBUG_TYPE LV_NAME

STATISTIC(LoopsAnalyzed, "Number of loops analyzed ");

namespace {

// Forward declarations.

class LoopInstructionsReordering;

/// LoopInstructionsReordering checks if there is a backward dependency between
/// instructions, such that the instructions can be reordered so as to convert a
/// non-vectorizable loop into vectorizable form. It first finds out the store
/// instructions which aliases with a load instruction, and then uses MemorySSA
/// Analysis to check if it is legal to move a load instruction above the store
/// instructions. If it is legal, then it checks for all the dependencies of the
/// corresponding Load Instruction which also needs to be moved above the Store
/// Instruction
class LoopInstructionsReordering {
public:
  LoopInstructionsReordering(
      Loop *L, PredicatedScalarEvolution &PSE, DominatorTree *DT,
      TargetLibraryInfo *TLI, AliasAnalysis *AA, Function *F,
      const TargetTransformInfo *TTI,
      std::function<const LoopAccessInfo &(Loop &)> *GetLAA, LoopInfo *LI,
      MemorySSA *MSSA)
      : TheLoop(L), PSE(PSE), TLI(TLI), TTI(TTI), DT(DT), MSSA(MSSA),
        GetLAA(GetLAA), LAI(nullptr) {}

  /// Returns true if the instructions have been reordered
  bool canbeReordered();

  const LoopAccessInfo *getLAI() const { return LAI; }

private:
  /// Gets the backward-dependent store and load instructions and checks if it
  /// is legal to reorder them using MemorySSA results
  bool reorderInstructions();

  /// Shifts a load instructions and all its dependences above the corresponding
  /// backward-dependent store instruction
  bool checkDepAndReorder(StoreInst *StInst,
                          Instruction *LdInst);

  /// The loop that we evaluate.
  Loop *TheLoop;
  /// A wrapper around ScalarEvolution used to add runtime SCEV checks.
  /// Applies dynamic knowledge to simplify SCEV expressions in the context
  /// of existing SCEV assumptions. The analysis will also add a minimal set
  /// of new predicates if this is required to enable vectorization and
  /// unrolling.
  PredicatedScalarEvolution &PSE;
  /// Target Library Info.
  TargetLibraryInfo *TLI;
  /// Target Transform Info
  const TargetTransformInfo *TTI;
  /// Dominator Tree.
  DominatorTree *DT;
  /// MemorySSA
  MemorySSA *MSSA;
  // LoopAccess analysis.
  std::function<const LoopAccessInfo &(Loop &)> *GetLAA;
  // And the loop-accesses info corresponding to this loop.  This pointer is
  // null until canVectorizeMemory sets it up.
  const LoopAccessInfo *LAI;

};

/// Moves load instructions and all its dependences above the corresponding
/// backward-dependent store instruction
bool LoopInstructionsReordering::checkDepAndReorder(StoreInst *StInst,
                                                    Instruction *LdInst) {
  std::queue<Instruction *> InstQueue;
  SmallVector<Instruction *, 16> InstStack;
  OrderedInstructions OI(&*DT);
  InstQueue.push(LdInst);

  Instruction *CurrInst;
  while (!InstQueue.empty()) {
    CurrInst = InstQueue.front();
    InstQueue.pop();
    // Pushing all the instructions to be moved before the store instruction
    InstStack.push_back(CurrInst);

    // Get all the operands of the current instruction, and if the operands
    // don't dominate the store instruction, then they will be enqueued
    for (User::op_iterator OpIter = CurrInst->op_begin();
         OpIter != CurrInst->op_end(); ++OpIter) {
      Instruction *OpInst = dyn_cast<Instruction>(*OpIter);
      if (OpInst) {
        if (!OI.dominates(OpInst, StInst))
          InstQueue.push(OpInst);
      }
    }
  }

  // Moving the load instruction and all its operands before the store
  // instruction
  while (!InstStack.empty()) {
    CurrInst = InstStack.pop_back_val();
    CurrInst->moveBefore(StInst);
  }

  return true;
}


/// Gets the backward-dependent store and load instructions and checks
/// if it is legal to reorder them using MemorySSA results
bool LoopInstructionsReordering::reorderInstructions() {
  bool Reordered = false;
  OrderedInstructions OI(&*DT);

  auto *Deps = LAI->getDepChecker().getDependences();
  for (auto Dep : *Deps) {
    if ((Dep.Type == MemoryDepChecker::Dependence::DepType::Backward)) {

      StoreInst *St = dyn_cast<StoreInst>(Dep.getSource(*LAI));
      LoadInst *Ld = dyn_cast<LoadInst>(Dep.getDestination(*LAI));
      // If the backward dependence is between load/store and not between
      // two loads or two stores
      if (Ld && St) {
        // If the Definition of the Load Instruction is live on Entry,
        // then then load instruction can be moved above the store instruction
        MemoryAccess *MemAcc = MSSA->getMemoryAccess(Ld);
        MemoryUseOrDef *NewMemAcc = cast<MemoryUseOrDef>(MemAcc);
        MemoryAccess *D = NewMemAcc->getDefiningAccess();
        if (!D)
          return Reordered;

        if (MSSA->isLiveOnEntryDef(D))
          Reordered |= checkDepAndReorder(St, Ld);
        else if (Instruction *DefInst = dyn_cast<Instruction>(D)) {
          // If the defining access is not contained in the loop, then the load
          // instruction can be moved above the store instruction
          if (!TheLoop->contains(DefInst))
            Reordered |= checkDepAndReorder(St, Ld);
          else {
            // If the defining access is MemoryPhi, then it means that load
            // instruction is before defining access, so if the store
            // instruction is between phiNode and load instruction, then the
            // instructions can be reordered
            MemoryPhi *DefPhi = dyn_cast<MemoryPhi>(D);
            if (DefPhi) {
              if (OI.dominates(St, Ld))
                Reordered |= checkDepAndReorder(St, Ld);
            } else {
              // If the defining access is MemoryUsOrDef, then it means that
              // load Instruction is after defining access, so if the store
              // instruction is between defining access and load instruction
              // then, the instructions can be reordered
              if (!OI.dominates(St, DefInst) && OI.dominates(St, Ld))
                Reordered |= checkDepAndReorder(St, Ld);
            }
          }
        }
      }
    }
  }
  return Reordered;
}


#ifndef NDEBUG
/// \return string containing a file name and a line # for the given loop.
static std::string getDebugLocString(const Loop *L) {
  std::string Result;
  if (L) {
    raw_string_ostream OS(Result);
    if (const DebugLoc LoopDbgLoc = L->getStartLoc())
      LoopDbgLoc.print(OS);
    else
      // Just print the module name.
      OS << L->getHeader()->getParent()->getParent()->getModuleIdentifier();
    OS.flush();
  }
  return Result;
}
#endif

/// Returns true if the given loop body has a cycle, excluding the loop
/// itself.
static bool hasCyclesInLoopBody(const Loop &L) {
  if (!L.empty())
    return true;

  for (const auto &SCC :
       make_range(scc_iterator<Loop, LoopBodyTraits>::begin(L),
                  scc_iterator<Loop, LoopBodyTraits>::end(L))) {
    if (SCC.size() > 1) {
      DEBUG(dbgs() << "LVL: Detected a cycle in the loop body:\n");
      DEBUG(L.dump());
      return true;
    }
  }
  return false;
}

/// Detect innermost loops which do not contain cycles
static void addAcyclicInnerLoop(Loop &L, SmallVectorImpl<Loop *> &V) {
  if (L.empty()) {
    if (!hasCyclesInLoopBody(L))
      V.push_back(&L);
    return;
  }
  for (Loop *InnerL : L)
    addAcyclicInnerLoop(*InnerL, V);
}

/// The LoopVectorize Pass.
struct LoopVectorizePred : public FunctionPass {
  /// Pass identification, replacement for typeid
  static char ID;

  explicit LoopVectorizePred(bool NoUnrolling = false,
                             bool AlwaysVectorize = true)
      : FunctionPass(ID) {
    Impl.DisableUnrolling = NoUnrolling;
    Impl.AlwaysVectorize = AlwaysVectorize;
    initializeLoopVectorizePredPass(*PassRegistry::getPassRegistry());
  }

  LoopVectorizePredPass Impl;

  bool runOnFunction(Function &F) override {
    if (skipFunction(F))
      return false;

    auto *SE = &getAnalysis<ScalarEvolutionWrapperPass>().getSE();
    auto *LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    auto *TTI = &getAnalysis<TargetTransformInfoWrapperPass>().getTTI(F);
    auto *DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    auto *TLIP = getAnalysisIfAvailable<TargetLibraryInfoWrapperPass>();
    auto *TLI = TLIP ? &TLIP->getTLI() : nullptr;
    auto *AA = &getAnalysis<AAResultsWrapperPass>().getAAResults();
    auto *AC = &getAnalysis<AssumptionCacheTracker>().getAssumptionCache(F);
    auto *LAA = &getAnalysis<LoopAccessLegacyAnalysis>();
    auto *MSSA = &getAnalysis<MemorySSAWrapperPass>().getMSSA();

    std::function<const LoopAccessInfo &(Loop &)> GetLAA =
        [&](Loop &L) -> const LoopAccessInfo & { return LAA->getInfo(&L); };

    return Impl.runImpl(F, *SE, *LI, *TTI, *DT, TLI, *AA, *AC, GetLAA, *MSSA);
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<AssumptionCacheTracker>();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<ScalarEvolutionWrapperPass>();
    AU.addRequired<TargetTransformInfoWrapperPass>();
    AU.addRequired<AAResultsWrapperPass>();
    AU.addRequired<LoopAccessLegacyAnalysis>();
    AU.addPreserved<LoopInfoWrapperPass>();
    AU.addPreserved<DominatorTreeWrapperPass>();
    AU.addPreserved<BasicAAWrapperPass>();
    AU.addPreserved<GlobalsAAWrapperPass>();
    AU.addRequired<MemorySSAWrapperPass>();
  }
};
} // namespace

char LoopVectorizePred::ID = 0;
static const char lv_name[] = "Loop Vectorization Predecessor";
INITIALIZE_PASS_BEGIN(LoopVectorizePred, LV_NAME, lv_name, false, false)
INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(BasicAAWrapperPass)
INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass)
INITIALIZE_PASS_DEPENDENCY(GlobalsAAWrapperPass)
INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(LoopAccessLegacyAnalysis)
INITIALIZE_PASS_DEPENDENCY(MemorySSAWrapperPass)
INITIALIZE_PASS_END(LoopVectorizePred, LV_NAME, lv_name, false, false)

namespace llvm {
Pass *createLoopVectorizePredPass(bool NoUnrolling, bool AlwaysVectorize) {
  return new LoopVectorizePred(NoUnrolling, AlwaysVectorize);
}
} // namespace llvm

bool LoopInstructionsReordering::canbeReordered() {
  LAI = &(*GetLAA)(*TheLoop);
  auto *Deps = LAI->getDepChecker().getDependences();

  for (auto Dep : *Deps) {
    if ((Dep.Type == MemoryDepChecker::Dependence::DepType::Backward)) {
      bool Reordered = reorderInstructions();
      if (Reordered)
        DEBUG(dbgs() << "\nLV: Instructions in loop have been reordered."
                     << "\n");
      return Reordered;
    }
  }

  return false;
}

bool LoopVectorizePredPass::processLoop(Loop *L) {
  assert(L->empty() && "Only process inner loops.");

#ifndef NDEBUG
  const std::string DebugLocStr = getDebugLocString(L);
#endif /* NDEBUG */

  DEBUG(dbgs() << "\nLV: Checking a loop in \""
               << L->getHeader()->getParent()->getName() << "\" from "
               << DebugLocStr << "\n");

  PredicatedScalarEvolution PSE(*SE, *L);
  Function *F = L->getHeader()->getParent();

  LoopInstructionsReordering LVL(L, PSE, DT, TLI, AA, F, TTI, GetLAA, LI, MSSA);
  bool Reordered = LVL.canbeReordered();

  return Reordered;
}

bool LoopVectorizePredPass::runImpl(
    Function &F, ScalarEvolution &SE_, LoopInfo &LI_, TargetTransformInfo &TTI_,
    DominatorTree &DT_, TargetLibraryInfo *TLI_, AliasAnalysis &AA_,
    AssumptionCache &AC_,
    std::function<const LoopAccessInfo &(Loop &)> &GetLAA_, MemorySSA &MSSA_) {

  SE = &SE_;
  LI = &LI_;
  TTI = &TTI_;
  DT = &DT_;
  TLI = TLI_;
  AA = &AA_;
  AC = &AC_;
  GetLAA = &GetLAA_;
  MSSA = &MSSA_;

  bool Changed = false;

  // The vectorizer requires loops to be in simplified form.
  // Since simplification may add new inner loops, it has to run before the
  // legality and profitability checks. This means running the loop vectorizer
  // will simplify all loops, regardless of whether anything end up being
  // vectorized.
  for (auto &L : *LI)
    Changed |= simplifyLoop(L, DT, LI, SE, AC, false /* PreserveLCSSA */);

  // Build up a worklist of inner-loops to vectorize. This is necessary as
  // the act of vectorizing or partially unrolling a loop creates new loops
  // and can invalidate iterators across the loops.
  SmallVector<Loop *, 8> Worklist;

  for (Loop *L : *LI)
    addAcyclicInnerLoop(*L, Worklist);

  LoopsAnalyzed += Worklist.size();

  // Now walk the identified inner loops.
  while (!Worklist.empty()) {
    Loop *L = Worklist.pop_back_val();

    // For the inner loops we actually process, form LCSSA to simplify the
    // transform.
    Changed |= formLCSSARecursively(*L, *DT, LI, SE);

    Changed |= processLoop(L);
  }

  // Process each loop nest in the function.
  return Changed;
}

PreservedAnalyses LoopVectorizePredPass::run(Function &F,
                                             FunctionAnalysisManager &AM) {
  auto &SE = AM.getResult<ScalarEvolutionAnalysis>(F);
  auto &LI = AM.getResult<LoopAnalysis>(F);
  auto &TTI = AM.getResult<TargetIRAnalysis>(F);
  auto &DT = AM.getResult<DominatorTreeAnalysis>(F);
  auto &TLI = AM.getResult<TargetLibraryAnalysis>(F);
  auto &AA = AM.getResult<AAManager>(F);
  auto &AC = AM.getResult<AssumptionAnalysis>(F);
  auto &MSSA = AM.getResult<MemorySSAAnalysis>(F).getMSSA();

  auto &LAM = AM.getResult<LoopAnalysisManagerFunctionProxy>(F).getManager();
  std::function<const LoopAccessInfo &(Loop &)> GetLAA =
      [&](Loop &L) -> const LoopAccessInfo & {
    LoopStandardAnalysisResults AR = {AA, AC, DT, LI, SE, TLI, TTI};
    return LAM.getResult<LoopAccessAnalysis>(L, AR);
  };
  bool Changed = runImpl(F, SE, LI, TTI, DT, &TLI, AA, AC, GetLAA, MSSA);
  if (!Changed)
    return PreservedAnalyses::all();
  PreservedAnalyses PA;
  PA.preserve<LoopAnalysis>();
  PA.preserve<DominatorTreeAnalysis>();
  PA.preserve<BasicAA>();
  PA.preserve<GlobalsAA>();
  return PA;
}
