//===- ScopedNoAliasAA.cpp - Scoped No-Alias Alias Analysis ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the ScopedNoAlias alias-analysis pass, which implements
// metadata-based scoped no-alias support.
//
// Alias-analysis scopes are defined by an id (which can be a string or some
// other metadata node), a domain node, and an optional descriptive string.
// A domain is defined by an id (which can be a string or some other metadata
// node), and an optional descriptive string.
//
// !dom0 =   metadata !{ metadata !"domain of foo()" }
// !scope1 = metadata !{ metadata !scope1, metadata !dom0, metadata !"scope 1" }
// !scope2 = metadata !{ metadata !scope2, metadata !dom0, metadata !"scope 2" }
//
// Loads and stores can be tagged with an alias-analysis scope, and also, with
// a noalias tag for a specific scope:
//
// ... = load %ptr1, !alias.scope !{ !scope1 }
// ... = load %ptr2, !alias.scope !{ !scope1, !scope2 }, !noalias !{ !scope1 }
//
// When evaluating an aliasing query, if one of the instructions is associated
// has a set of noalias scopes in some domain that is superset of the alias
// scopes in that domain of some other instruction, then the two memory
// accesses are assumed not to alias.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/CaptureTracking.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
using namespace llvm;

#define DEBUG_TYPE "scoped-noalias"

// A handy option for disabling scoped no-alias functionality. The same effect
// can also be achieved by stripping the associated metadata tags from IR, but
// this option is sometimes more convenient.
static cl::opt<bool>
EnableScopedNoAlias("enable-scoped-noalias", cl::init(true), cl::Hidden,
  cl::desc("Enable use of scoped-noalias metadata"));

static cl::opt<int>
MaxNoAliasDepth("scoped-noalias-max-depth", cl::init(12), cl::Hidden,
  cl::desc("Maximum depth for noalias intrinsic search"));

namespace {
/// AliasScopeNode - This is a simple wrapper around an MDNode which provides
/// a higher-level interface by hiding the details of how alias analysis
/// information is encoded in its operands.
class AliasScopeNode {
  const MDNode *Node;

public:
  AliasScopeNode() : Node(0) {}
  explicit AliasScopeNode(const MDNode *N) : Node(N) {}

  /// getNode - Get the MDNode for this AliasScopeNode.
  const MDNode *getNode() const { return Node; }

  /// getDomain - Get the MDNode for this AliasScopeNode's domain.
  const MDNode *getDomain() const {
    if (Node->getNumOperands() < 2)
      return nullptr;
    return dyn_cast_or_null<MDNode>(Node->getOperand(1));
  }
};

/// ScopedNoAliasAA - This is a simple alias analysis
/// implementation that uses scoped-noalias metadata to answer queries.
class ScopedNoAliasAA : public ImmutablePass, public AliasAnalysis {
public:
  static char ID; // Class identification, replacement for typeinfo
  ScopedNoAliasAA() : ImmutablePass(ID) {
    initializeScopedNoAliasAAPass(*PassRegistry::getPassRegistry());
  }

  bool doInitialization(Module &M) override;

  /// getAdjustedAnalysisPointer - This method is used when a pass implements
  /// an analysis interface through multiple inheritance.  If needed, it
  /// should override this to adjust the this pointer as needed for the
  /// specified pass info.
  void *getAdjustedAnalysisPointer(const void *PI) override {
    if (PI == &AliasAnalysis::ID)
      return (AliasAnalysis*)this;
    return this;
  }

protected:
  bool mayAliasInScopes(const MDNode *Scopes, const MDNode *NoAlias) const;
  void collectMDInDomain(const MDNode *List, const MDNode *Domain,
                         SmallPtrSetImpl<const MDNode *> &Nodes) const;

  bool findCompatibleNoAlias(const Value *P,
                             const MDNode *ANoAlias, const MDNode *BNoAlias,
                             const DataLayout &DL,
                             SmallPtrSetImpl<const Value *> &Visited,
                             SmallVectorImpl<Instruction *> &CompatibleSet,
                             int Depth = 0);
  bool noAliasByIntrinsic(const MDNode *ANoAlias,
                          const Value *APtr,
                          const MDNode *BNoAlias,
                          const Value *BPtr,
                          ImmutableCallSite CSA = ImmutableCallSite(),
                          ImmutableCallSite CSB = ImmutableCallSite());

private:
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  AliasResult alias(const Location &LocA, const Location &LocB) override;
  bool pointsToConstantMemory(const Location &Loc, bool OrLocal) override;
  ModRefBehavior getModRefBehavior(ImmutableCallSite CS) override;
  ModRefBehavior getModRefBehavior(const Function *F) override;
  ModRefResult getModRefInfo(ImmutableCallSite CS,
                             const Location &Loc) override;
  ModRefResult getModRefInfo(ImmutableCallSite CS1,
                             ImmutableCallSite CS2) override;
};
}  // End of anonymous namespace

// Register this pass...
char ScopedNoAliasAA::ID = 0;
INITIALIZE_AG_PASS(ScopedNoAliasAA, AliasAnalysis, "scoped-noalias",
                   "Scoped NoAlias Alias Analysis", false, true, false)

ImmutablePass *llvm::createScopedNoAliasAAPass() {
  return new ScopedNoAliasAA();
}

bool ScopedNoAliasAA::doInitialization(Module &M) {
  InitializeAliasAnalysis(this, &M.getDataLayout());
  return true;
}

void
ScopedNoAliasAA::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AliasAnalysis::getAnalysisUsage(AU);
}

void
ScopedNoAliasAA::collectMDInDomain(const MDNode *List, const MDNode *Domain,
                   SmallPtrSetImpl<const MDNode *> &Nodes) const {
  for (unsigned i = 0, ie = List->getNumOperands(); i != ie; ++i)
    if (const MDNode *MD = dyn_cast<MDNode>(List->getOperand(i)))
      if (AliasScopeNode(MD).getDomain() == Domain)
        Nodes.insert(MD);
}

bool
ScopedNoAliasAA::mayAliasInScopes(const MDNode *Scopes,
                                  const MDNode *NoAlias) const {
  if (!Scopes || !NoAlias)
    return true;

  // Collect the set of scope domains relevant to the noalias scopes.
  SmallPtrSet<const MDNode *, 16> Domains;
  for (unsigned i = 0, ie = NoAlias->getNumOperands(); i != ie; ++i)
    if (const MDNode *NAMD = dyn_cast<MDNode>(NoAlias->getOperand(i)))
      if (const MDNode *Domain = AliasScopeNode(NAMD).getDomain())
        Domains.insert(Domain);

  // We alias unless, for some domain, the set of noalias scopes in that domain
  // is a superset of the set of alias scopes in that domain.
  for (const MDNode *Domain : Domains) {
    SmallPtrSet<const MDNode *, 16> NANodes, ScopeNodes;
    collectMDInDomain(NoAlias, Domain, NANodes);
    collectMDInDomain(Scopes, Domain, ScopeNodes);
    if (!ScopeNodes.size())
      continue;

    // To not alias, all of the nodes in ScopeNodes must be in NANodes.
    bool FoundAll = true;
    for (const MDNode *SMD : ScopeNodes)
      if (!NANodes.count(SMD)) {
        FoundAll = false;
        break;
      }

    if (FoundAll)
      return false;
  }

  return true;
}

bool ScopedNoAliasAA::findCompatibleNoAlias(const Value *P,
                        const MDNode *ANoAlias, const MDNode *BNoAlias,
                        const DataLayout &DL,
                        SmallPtrSetImpl<const Value *> &Visited,
                        SmallVectorImpl<Instruction *> &CompatibleSet,
                        int Depth) {
  // When a pointer is derived from multiple noalias calls, there are two
  // potential reasons:
  //   1. The path of derivation is uncertain (because of a select, PHI, etc.).
  //   2. Some noalias calls are derived from other noalias calls.
  // Logically, we need to treat (1) as an "and" and (2) as an "or" when
  // checking for scope compatibility. If we don't know from which noalias call
  // a pointer is derived, then we need to require compatibility with all of
  // them. If we're derived from a noalias call that is derived from another
  // noalias call, then we need the ability to effectively ignore the inner one
  // in favor of the outer one (thus, we only need compatibility with one or
  // the other).
  //
  // Scope compatibility means that, as with the noalias metadata, within each
  // domain, the set of noalias intrinsic scopes is a subset of the noalias
  // scopes.
  //
  // Given this, we check compatibility of the relevant sets of noalias calls
  // from which LocA.Ptr might derive with both LocA.AATags.NoAlias and
  // LocB.AATags.NoAlias, and LocB.Ptr does not derive from any of the noalias
  // calls in some set, then we can conclude NoAlias.
  //
  // So if we have:
  //   noalias1  noalias3
  //      |         |
  //   noalias2  noalias4
  //      |         |
  //       \       /
  //        \     /
  //         \   /
  //          \ /
  //         select
  //           |
  //        noalias5
  //           |
  //        noalias6
  //           |
  //          PtrA
  //
  //  - If PtrA is compatible with noalias6, and PtrB is also compatible,
  //    but does not derive from noalias6, then NoAlias.
  //  - If PtrA is compatible with noalias5, and PtrB is also compatible,
  //    but does not derive from noalias5, then NoAlias.
  //  - If PtrA is compatible with noalias2 and noalias4, and PtrB is also
  //    compatible, but does not derive from either, then NoAlias.
  //  - If PtrA is compatible with noalias2 and noalias3, and PtrB is also
  //    compatible, but does not derive from either, then NoAlias.
  //  - If PtrA is compatible with noalias1 and noalias4, and PtrB is also
  //    compatible, but does not derive from either, then NoAlias.
  //  - If PtrA is compatible with noalias1 and noalias3, and PtrB is also
  //    compatible, but does not derive from either, then NoAlias.
  //
  //  We don't need, or want, to explicitly build N! sets to check for scope
  //  compatibility. Instead, recurse through the tree of underlying objects.

  SmallVector<Instruction *, 8> NoAliasCalls;
  P = GetUnderlyingObject(P, DL, 0, &NoAliasCalls);

  // If we've already visited this underlying value (likely because this is a
  // PHI that depends on itself, directly or indirectly), we must not have
  // returned false the first time, so don't do so this time either.
  if (!Visited.insert(P).second)
    return true;

  // Our pointer is derived from P, with NoAliasCalls along the way.
  // Compatibility with any of them is fine.
  auto NAI =
    std::find_if(NoAliasCalls.begin(), NoAliasCalls.end(),
                 [&](Instruction *A) {
                  return !mayAliasInScopes(
                             dyn_cast<MDNode>(
                               cast<MetadataAsValue>(A->getOperand(1))->
                                 getMetadata()), ANoAlias) &&
                         !mayAliasInScopes(
                             dyn_cast<MDNode>(
                               cast<MetadataAsValue>(A->getOperand(1))->
                                 getMetadata()), BNoAlias);
                 });
  if (NAI != NoAliasCalls.end()) {
    CompatibleSet.push_back(*NAI);
    return true;
  }

  // We've not found a compatible noalias call, but we might be able to keep
  // looking. If this underlying object is really a PHI or a select, we can
  // check the incoming values. They all need to be compatible, and if so, we
  // can take the union of all of the compatible noalias calls as the set to
  // return for further validation.
  SmallVector<const Value *, 8> Children;
  if (const SelectInst *SI = dyn_cast<SelectInst>(P)) {
    Children.push_back(SI->getTrueValue());
    Children.push_back(SI->getFalseValue());
  } else if (const PHINode *PN = dyn_cast<PHINode>(P)) {
    for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i)
      Children.push_back(PN->getIncomingValue(i));
  }

  if (Children.empty() || Depth == MaxNoAliasDepth)
    return false;

  for (auto &C : Children) {
    SmallPtrSet<const Value *, 16> ChildVisited(Visited.begin(), Visited.end());
    ChildVisited.insert(P);

    SmallVector<Instruction *, 8> ChildCompatSet;
    if (!findCompatibleNoAlias(C, ANoAlias, BNoAlias, DL, ChildVisited,
                               ChildCompatSet, Depth+1))
      return false;

    CompatibleSet.insert(CompatibleSet.end(),
                         ChildCompatSet.begin(), ChildCompatSet.end());
  }

  // All children were compatible, and we've added them to CompatibleSet.
  return true;
}

bool ScopedNoAliasAA::noAliasByIntrinsic(const MDNode *ANoAlias,
                                         const Value *APtr,
                                         const MDNode *BNoAlias,
                                         const Value *BPtr,
                                         ImmutableCallSite CSA,
                                         ImmutableCallSite CSB) {
  if (!ANoAlias || !BNoAlias)
    return false;

  if (CSA) {
    // We're querying a callsite against something else, where we want to know
    // if the callsite (CSA) is derived from some noalias call(s) and the other
    // thing is not derived from those noalias call(s). This can be determined
    // only if CSA only accesses memory through its arguments.
    AliasAnalysis::ModRefBehavior MRB = getModRefBehavior(CSA);
    if (MRB != AliasAnalysis::OnlyAccessesArgumentPointees &&
        MRB != AliasAnalysis::OnlyReadsArgumentPointees)
      return false;

    DEBUG(dbgs() << "SNA: CSA: " << *CSA.getInstruction() << "\n");
    // Since the memory-access behavior of CSA is determined only by its
    // arguments, we can answer this query in the affirmative if we can prove a
    // lack of aliasing for all pointer arguments.
    for (auto AI = CSA.arg_begin(), AE = CSA.arg_end(); AI != AE; ++AI) {
      if (!(*AI)->getType()->isPointerTy())
        continue;

      if (!noAliasByIntrinsic(ANoAlias, *AI, BNoAlias, BPtr,
                              ImmutableCallSite(), CSB)) {
        DEBUG(dbgs() << "SNA: CSA: noalias fail for arg: " << *(*AI) << "\n");
        return false;
      }
    }

    return true;
  }

  const Instruction *AInst = dyn_cast<Instruction>(APtr);
  if (!AInst || !AInst->getParent())
    return false;
  const DataLayout &DL = AInst->getParent()->getModule()->getDataLayout();

  if (!CSB && !BPtr)
    return false;

  DEBUG(dbgs() << "SNA: A: " << *APtr << "\n");
  DEBUG(dbgs() << "SNA: "; if (CSA) dbgs() << "CSB: " << *CSB.getInstruction();
                           else dbgs() << "B: " << *BPtr; dbgs() << "\n");

  SmallPtrSet<const Value *, 8> Visited;
  SmallVector<Instruction *, 8> CompatibleSet;
  if (!findCompatibleNoAlias(APtr, ANoAlias, BNoAlias, DL, Visited,
                             CompatibleSet))
    return false;

  assert(!CompatibleSet.empty() &&
         "Fould an empty set of compatible intrinsics?");

  DEBUG(dbgs() << "SNA: Found a compatible set!\n");
#ifndef NDEBUG
  for (auto &C : CompatibleSet)
    DEBUG(dbgs() << "\t" << *C << "\n");
  DEBUG(dbgs() << "\n");
#endif

  // We have a set of compatible noalias calls (compatible with the scopes from
  // both LocA and LocB) from which LocA.Ptr potentially derives. We now need
  // to make sure that LocB.Ptr does not derive from any in that set. For
  // correctness, there cannot be a depth limit here (if a pointer is derived
  // from a noalias call, we must know).
  SmallVector<Value *, 8> BObjs;
  SmallVector<Instruction *, 8> BNoAliasCalls;
  if (CSB) {
    for (auto AI = CSB.arg_begin(), AE = CSB.arg_end(); AI != AE; ++AI)
      GetUnderlyingObjects(*AI, BObjs, DL, nullptr, 0, &BNoAliasCalls);
  } else {
    GetUnderlyingObjects(const_cast<Value*>(BPtr), BObjs, DL, nullptr, 0,
                         &BNoAliasCalls);
  }

  DEBUG(dbgs() << "SNA: B/CSB noalias:\n");
#ifndef NDEBUG
  for (auto &B : BNoAliasCalls)
    DEBUG(dbgs() << "\t" << *B << "\n");
  DEBUG(dbgs() << "\n");
#endif

  // The noalias scope from the compatible intrinsics are really identified by
  // their scope argument, and we need to make sure that LocB.Ptr is not only
  // not derived from the calls currently in CompatibleSet, but also from any
  // other intrinsic with the same scope. We can't just search the list of
  // noalias intrinsics in BNoAliasCalls because we care not just about those
  // direct dependence, but also dependence through capturing. Metadata
  // do not have use lists, but MetadataAsValue objects do (and they are
  // uniqued), so we can search their use list. As a result, however,
  // correctness demands that the scope list has only one element (so that we
  // can find all uses of that scope by noalias intrinsics by looking at the
  // use list of the associated scope list).
  SmallPtrSet<Instruction *, 8> CompatibleSetMembers(CompatibleSet.begin(),
                                                     CompatibleSet.end());
  SmallVector<MetadataAsValue *, 8> CompatibleSetMVs;
  for (auto &C : CompatibleSet)
    CompatibleSetMVs.push_back(cast<MetadataAsValue>(C->getOperand(1)));
  for (auto &MV : CompatibleSetMVs)
    for (Use &U : MV->uses())
      if (Instruction *UI = dyn_cast<Instruction>(U))
        if (CompatibleSetMembers.insert(UI).second) {
          CompatibleSet.push_back(UI);
          DEBUG(dbgs() << "SNA: Adding to compatible set based on MD use: " <<
                          *UI << "\n");
        }

  if (std::find_first_of(CompatibleSet.begin(), CompatibleSet.end(),
                         BNoAliasCalls.begin(), BNoAliasCalls.end()) !=
        CompatibleSet.end())
    return false;

  DEBUG(dbgs() << "SNA: B does not derive from the compatible set!\n");

  DominatorTreeWrapperPass *DTWP =
      getAnalysisIfAvailable<DominatorTreeWrapperPass>();
  DominatorTree *DT = DTWP ? &DTWP->getDomTree() : nullptr;

  DEBUG(dbgs() << "SNA: DT is " << (DT ? "available" : "unavailable") << "\n");

  // We now know that LocB.Ptr does not derive from any of the noalias calls in
  // CompatibleSet directly. We do, however, need to make sure that it cannot
  // derive from them by capture.
  for (auto &V  : BObjs) {
    // If the underlying object is not an instruction, then it can't be
    // capturing the output value of an instruction (specifically, the noalias
    // intrinsic call), and we can ignore it.
    Instruction *I = dyn_cast<Instruction>(V);
    if (!I)
      continue;
    if (isIdentifiedFunctionLocal(I))
      continue;

    DEBUG(dbgs() << "SNA: Capture check for B/CSB UO: " << *I << "\n");

    // If the value from the noalias intrinsic has been captured prior to the
    // instruction defining the underlying object, then LocB.Ptr might yet be
    // derived from the return value of the noalias intrinsic, and we cannot
    // conclude anything about the aliasing.
    for (auto &C : CompatibleSet)
      if (PointerMayBeCapturedBefore(C, /* ReturnCaptures */false,
                                        /* StoreCaptures */false, I, DT)) {
        DEBUG(dbgs() << "SNA: Pointer " << *C << " might be captured!\n");
        return false;
      }
  }

  if (CSB) {
    AliasAnalysis::ModRefBehavior MRB = getModRefBehavior(CSB);
    if (MRB != AliasAnalysis::OnlyAccessesArgumentPointees &&
        MRB != AliasAnalysis::OnlyReadsArgumentPointees) {
      // If we're querying against a callsite, and it might read from memory
      // not based on its arguments, then we need to check whether or not the
      // relevant noalias results have been captured prior to the callsite.
      for (auto &C : CompatibleSet)
        if (PointerMayBeCapturedBefore(C, /* ReturnCaptures */false,
                                          /* StoreCaptures */false,
                                          CSB.getInstruction(), DT)) {
          DEBUG(dbgs() << "SNA: CSB: Pointer " << *C <<
                          " might be captured!\n");
          return false;
        }
    }
  }

  DEBUG(dbgs() << " SNA: noalias!\n");
  return true;
}

AliasAnalysis::AliasResult
ScopedNoAliasAA::alias(const Location &LocA, const Location &LocB) {
  if (!EnableScopedNoAlias)
    return AliasAnalysis::alias(LocA, LocB);

  // Get the attached MDNodes.
  const MDNode *AScopes = LocA.AATags.Scope,
               *BScopes = LocB.AATags.Scope;

  const MDNode *ANoAlias = LocA.AATags.NoAlias,
               *BNoAlias = LocB.AATags.NoAlias;

  if (!mayAliasInScopes(AScopes, BNoAlias))
    return NoAlias;

  if (!mayAliasInScopes(BScopes, ANoAlias))
    return NoAlias;

  if (noAliasByIntrinsic(ANoAlias, LocA.Ptr, BNoAlias, LocB.Ptr))
    return NoAlias;

  if (noAliasByIntrinsic(BNoAlias, LocB.Ptr, ANoAlias, LocA.Ptr))
    return NoAlias;

  // If they may alias, chain to the next AliasAnalysis.
  return AliasAnalysis::alias(LocA, LocB);
}

bool ScopedNoAliasAA::pointsToConstantMemory(const Location &Loc,
                                             bool OrLocal) {
  return AliasAnalysis::pointsToConstantMemory(Loc, OrLocal);
}

AliasAnalysis::ModRefBehavior
ScopedNoAliasAA::getModRefBehavior(ImmutableCallSite CS) {
  return AliasAnalysis::getModRefBehavior(CS);
}

AliasAnalysis::ModRefBehavior
ScopedNoAliasAA::getModRefBehavior(const Function *F) {
  return AliasAnalysis::getModRefBehavior(F);
}

AliasAnalysis::ModRefResult
ScopedNoAliasAA::getModRefInfo(ImmutableCallSite CS, const Location &Loc) {
  if (!EnableScopedNoAlias)
    return AliasAnalysis::getModRefInfo(CS, Loc);

  const MDNode *CSNoAlias =
    CS.getInstruction()->getMetadata(LLVMContext::MD_noalias);
  if (!mayAliasInScopes(Loc.AATags.Scope, CSNoAlias))
    return NoModRef;

  const MDNode *CSScopes =
    CS.getInstruction()->getMetadata(LLVMContext::MD_alias_scope);
  if (!mayAliasInScopes(CSScopes, Loc.AATags.NoAlias))
    return NoModRef;

  if (noAliasByIntrinsic(Loc.AATags.NoAlias, Loc.Ptr, CSNoAlias,
                         nullptr, ImmutableCallSite(), CS))
    return NoModRef;

  if (noAliasByIntrinsic(CSNoAlias, nullptr, Loc.AATags.NoAlias, Loc.Ptr, CS))
    return NoModRef;

  return AliasAnalysis::getModRefInfo(CS, Loc);
}

AliasAnalysis::ModRefResult
ScopedNoAliasAA::getModRefInfo(ImmutableCallSite CS1, ImmutableCallSite CS2) {
  if (!EnableScopedNoAlias)
    return AliasAnalysis::getModRefInfo(CS1, CS2);

  const MDNode *CS1Scopes =
    CS1.getInstruction()->getMetadata(LLVMContext::MD_alias_scope);
  const MDNode *CS2Scopes =
    CS2.getInstruction()->getMetadata(LLVMContext::MD_alias_scope);
  const MDNode *CS1NoAlias =
    CS1.getInstruction()->getMetadata(LLVMContext::MD_noalias);
  const MDNode *CS2NoAlias =
    CS2.getInstruction()->getMetadata(LLVMContext::MD_noalias);
  if (!mayAliasInScopes(CS1Scopes,
          CS2.getInstruction()->getMetadata(LLVMContext::MD_noalias)))
    return NoModRef;

  if (!mayAliasInScopes(CS2Scopes,
          CS1.getInstruction()->getMetadata(LLVMContext::MD_noalias)))
    return NoModRef;

  if (noAliasByIntrinsic(CS1NoAlias, nullptr, CS2NoAlias, nullptr, CS1, CS2))
    return NoModRef;

  if (noAliasByIntrinsic(CS2NoAlias, nullptr, CS1NoAlias, nullptr, CS2, CS1))
    return NoModRef;

  return AliasAnalysis::getModRefInfo(CS1, CS2);
}

