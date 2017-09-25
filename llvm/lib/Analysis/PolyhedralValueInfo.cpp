//===-- PolyhedralValueInfo.cpp  - Polyhedral value analysis ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "llvm/Analysis/PolyhedralValueInfo.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>

using namespace llvm;

#define DEBUG_TYPE "polyhedral-value-info"

STATISTIC(NUM_PARAMETERS, "Number of parameters created");
STATISTIC(NUM_DOMAINS, "Number of domains created");
STATISTIC(NUM_EXPRESSIONS, "Number of expressions created");
STATISTIC(COMPLEX_DOMAIN, "Number of domains to complex");

raw_ostream &llvm::operator<<(raw_ostream &OS, PEXP::ExpressionKind Kind) {
  switch (Kind) {
  case PEXP::EK_NONE:
    return OS << "NONE";
  case PEXP::EK_INTEGER:
    return OS << "INTEGER";
  case PEXP::EK_DOMAIN:
    return OS << "DOMAIN";
  case PEXP::EK_UNKNOWN_VALUE:
    return OS << "UNKNOWN";
  case PEXP::EK_NON_AFFINE:
    return OS << "NON AFFINE";
  default:
    llvm_unreachable("Unknown polyhedral expression kind");
  }
}

PEXP *PEXP::setDomain(const PVSet &Domain, bool Overwrite) {
  assert((!PWA || Overwrite) && "PWA already initialized");
  DEBUG(dbgs() << "SetDomain: " << Domain << " for " << Val->getName() << "\n");
  if (Domain.isComplex()) {
    DEBUG(dbgs() << "Domain too complex!\n");
    return invalidate();
  }

  setKind(PEXP::EK_DOMAIN);
  PWA = PVAff(Domain, 1);
  PWA.dropUnusedParameters();
  if (!InvalidDomain)
    InvalidDomain = PVSet::empty(PWA);
  if (!KnownDomain)
    KnownDomain = PVSet::universe(PWA);
  else
    PWA.simplify(KnownDomain);
  return this;
}

void PEXP::print(raw_ostream &OS) const {
  OS << PWA << " [" << getValue()->getName() << "] [" << getKind()
     << "] [Scope: " << (getScope() ? getScope()->getName() : "<max>") << "]";
  if (!InvalidDomain.isEmpty())
    OS << " [ID: " << InvalidDomain << "]";
  if (!KnownDomain.isUniverse())
    OS << " [KD: " << KnownDomain << "]";
}
void PEXP::dump() const { print(dbgs()); }

raw_ostream &llvm::operator<<(raw_ostream &OS, const PEXP *PE) {
  if (PE)
    OS << *PE;
  else
    OS << "<null>";
  return OS;
}

raw_ostream &llvm::operator<<(raw_ostream &OS, const PEXP &PE) {
  PE.print(OS);
  return OS;
}

PEXP &PEXP::operator=(const PEXP &PE) {
  Kind = PE.Kind;
  PWA = PE.getPWA();
  InvalidDomain = PE.getInvalidDomain();
  KnownDomain = PE.getKnownDomain();

  return *this;
}

PEXP &PEXP::operator=(PEXP &&PE) {
  std::swap(Kind, PE.Kind);
  std::swap(PWA, PE.PWA);
  std::swap(InvalidDomain, PE.InvalidDomain);
  std::swap(KnownDomain, PE.KnownDomain);

  return *this;
}

void PEXP::addInvalidDomain(const PVSet &ID) { InvalidDomain.unify(ID); }

void PEXP::addKnownDomain(const PVSet &KD) {
  KnownDomain.intersect(KD);
  PWA.simplify(KnownDomain);
}

PEXP *PEXP::assign(const PEXP *PE0, const PEXP *PE1,
                   PVAff::IslCombinatorFn Combinator) {
  return assign(PE0, PE1, PVAff::getCombinatorFn(Combinator));
}

PEXP *PEXP::assign(const PEXP *PE0, const PEXP *PE1,
                   PVAff::CombinatorFn Combinator) {
  DEBUG(dbgs() << "Assign " << this << " [" << PE0 << "][" << PE1 << "]\n");
  assert(!isInitialized() &&
         "Cannot assign to an initialized polyhedral expression");
  assert(PE0 && PE0->isInitialized() && PE1 && PE1->isInitialized() &&
         "Cannot assign from an uninitialized polyhedral expression");

  // Merge the kinds and exit if the result is non-affine.
  Kind = std::max(PE0->Kind, PE1->Kind);
  if (Kind == PEXP::EK_NON_AFFINE)
    return invalidate();

  auto &PWA0 = PE0->getPWA();
  auto &PWA1 = PE1->getPWA();
  PWA = Combinator(PWA0, PWA1);

  InvalidDomain =
      PVSet::unify(PE0->getInvalidDomain(), PE1->getInvalidDomain());
  KnownDomain = PVSet::intersect(PE0->getKnownDomain(), PE1->getKnownDomain());

  return this;
}

PEXP *PEXP::combine(const PEXP *PE) {
  Kind = std::max(Kind, PE->Kind);

  InvalidDomain.unify(PE->getInvalidDomain());
  KnownDomain.intersect(PE->getKnownDomain());
  return this;
}

PEXP *PEXP::combine(const PEXP *PE, PVAff::IslCombinatorFn Combinator,
                    const PVSet *Domain) {
  return combine(PE, PVAff::getCombinatorFn(Combinator), Domain);
}

PEXP *PEXP::combine(const PEXP *PE, PVAff::CombinatorFn Combinator,
                    const PVSet *Domain) {
  assert(isInitialized() && PE && PE->isInitialized() && Combinator &&
         "Can only combine initialized polyhedral expressions");

  combine(PE);
  if (Kind == PEXP::EK_NON_AFFINE)
    return invalidate();

  PVAff PEPWA = PE->getPWA();
  if (Domain)
    PEPWA.intersectDomain(*Domain);

  PWA = PWA ? Combinator(PWA, PEPWA) : PEPWA;

  return this;
}

PEXP *PEXP::invalidate() {
  Kind = PEXP::EK_NON_AFFINE;
  PWA = PVAff();
  return this;
}

void PEXP::adjustInvalidAndKnownDomain() {
  auto *ITy = cast<IntegerType>(getValue()->getType());
  unsigned BitWidth = ITy->getBitWidth();
  assert(BitWidth > 0 && BitWidth <= 64);
  int64_t LowerBound = -1 * (1 << (BitWidth - 1));
  int64_t UpperBound = (1 << (BitWidth - 1)) - 1;

  PVAff LowerPWA(getDomain(), LowerBound);
  PVAff UpperPWA(getDomain(), UpperBound);

  auto *OVBinOp = cast<OverflowingBinaryOperator>(getValue());
  bool HasNSW = OVBinOp->hasNoSignedWrap();

  const PVAff &PWA = getPWA();
  if (HasNSW) {
    PVSet BoundedDomain = PWA.getGreaterEqualDomain(LowerPWA).intersect(
        PWA.getLessEqualDomain(UpperPWA));
    KnownDomain.intersect(BoundedDomain);
  } else {
    PVSet BoundedDomain = LowerPWA.getGreaterEqualDomain(PWA).unify(
        UpperPWA.getLessEqualDomain(PWA));

    InvalidDomain.unify(BoundedDomain);
  }
}

// ------------------------------------------------------------------------- //

void PolyhedralValueInfoCache::releaseMemory() {
  DeleteContainerSeconds(ValueMap);
  DeleteContainerSeconds(DomainMap);
  ParameterMap.clear();
}

PVId PolyhedralValueInfoCache::getParameterId(Value &V, const PVCtx &Ctx) {
  PVId &Id = ParameterMap[&V];
  if (Id)
    return Id;

  std::string ParameterName;
  ParameterName = V.hasName() ? V.getName().str()
                              : "p" + std::to_string(ParameterMap.size());
  ParameterName = PVBase::getIslCompatibleName("", ParameterName, "");
  DEBUG(dbgs() << "NEW PARAM: " << V << " ::: " << ParameterName << "\n";);
  Id = PVId(Ctx, ParameterName, &V);

  return Id;
}

// ------------------------------------------------------------------------- //

class llvm::PolyhedralExpressionBuilder
    : public InstVisitor<PolyhedralExpressionBuilder, PEXP *> {

  Loop *Scope;

  PolyhedralValueInfo &PI;
  PolyhedralValueInfoCache PIC;

  PEXP *visit(Constant &I);
  PEXP *visit(ConstantInt &I);
  PEXP *visitParameter(Value &V);
  PEXP *createParameter(PEXP *PE);

  PVAff getZero(const PVAff &RefPWA) { return PVAff(RefPWA.getDomain(), 0); }

  PVAff getOne(const PVAff &RefPWA) { return PVAff(RefPWA.getDomain(), 1); }

  PEXP *getOrCreatePEXP(Value &V) {
    Loop *UsedScope = Scope;
    if (!Scope || !isa<Instruction>(V))
      UsedScope = nullptr;

    PEXP *PE = PIC.getOrCreatePEXP(V, UsedScope);
    assert(PE && PIC.lookup(V, UsedScope));
    assert(PE->getScope() == UsedScope && PE->getValue() == &V);
    return PE;
  }

  PEXP *getOrCreateDomain(BasicBlock &BB) {
    return PIC.getOrCreateDomain(BB, Scope);
  }

  unsigned getRelativeLoopDepth(BasicBlock *BB);

public:
  PolyhedralExpressionBuilder(PolyhedralValueInfo &PI)
      : Scope(nullptr), PI(PI) {}

  PVSet buildNotEqualDomain(const PEXP *VI, ArrayRef<Constant *> CIs);
  PVSet buildEqualDomain(const PEXP *VI, Constant &CI);

  void setScope(Loop *NewScope) { Scope = NewScope; }

  PEXP *getDomain(BasicBlock &BB);

  PVId getParameterId(Value &V) { return PIC.getParameterId(V, PI.getCtx()); }

  void setEdgeCondition(PEXP *PE, BasicBlock &PredBB, BasicBlock &BB);
  PEXP *getTerminatorPEXP(BasicBlock &BB);

  PEXP *getDomainOnEdge(BasicBlock &PredBB, BasicBlock &BB);
  PEXP *getDomainOnEdge(const PEXP &PredDomPE, BasicBlock &BB);

  PEXP *visitOperand(Value &Op, Instruction &I);
  PEXP *visit(Value &V);
  PEXP *visit(Instruction &I);

  PEXP *visitBinaryOperator(BinaryOperator &I);
  PEXP *visitCallInst(CallInst &I);
  PEXP *visitCastInst(CastInst &I);
  PEXP *visitFCmpInst(FCmpInst &I);
  PEXP *visitGetElementPtrInst(GetElementPtrInst &I);
  PEXP *visitICmpInst(ICmpInst &I);
  PEXP *visitInvokeInst(InvokeInst &I);
  PEXP *visitLoadInst(LoadInst &I);
  PEXP *visitSelectInst(SelectInst &I);
  PEXP *visitConditionalPHINode(PHINode &I);
  PEXP *visitPHINode(PHINode &I);
  PEXP *visitAllocaInst(AllocaInst &I);

  PEXP *visitInstruction(Instruction &I);
};

void PolyhedralExpressionBuilder::setEdgeCondition(PEXP *PE, BasicBlock &PredBB,
                                                   BasicBlock &BB) {
  assert(PE && !PE->isInitialized());

  auto &TI = *PredBB.getTerminator();
  if (TI.getNumSuccessors() == 1) {
    assert(&BB == TI.getSuccessor(0));
    PE->setDomain(PVSet::universe(PI.getCtx()));
    return;
  }

  auto *TermPE = getTerminatorPEXP(PredBB);
  if (!TermPE) {
    PE->invalidate();
    return;
  }
  DEBUG(dbgs() << "TERMPE: " << TermPE << "\n");

  PVSet EdgeCond;
  auto *Int64Ty = Type::getInt64Ty(TI.getContext());
  if (isa<BranchInst>(TI)) {
    if (TI.getSuccessor(0) == &BB)
      EdgeCond = buildEqualDomain(TermPE, *ConstantInt::get(Int64Ty, 1));
    if (TI.getSuccessor(1) == &BB)
      EdgeCond.unify(buildEqualDomain(TermPE, *ConstantInt::get(Int64Ty, 0)));
  } else if (auto *SI = dyn_cast<SwitchInst>(&TI)) {
    bool IsDefaultBlock = (SI->getDefaultDest() == &BB);
    SmallVector<Constant *, 8> OtherCaseValues;
    for (auto &Case : SI->cases()) {
      if (Case.getCaseSuccessor() == &BB)
        EdgeCond.unify(buildEqualDomain(TermPE, *Case.getCaseValue()));
      else if (IsDefaultBlock)
        OtherCaseValues.push_back(Case.getCaseValue());
    }

    if (IsDefaultBlock && !OtherCaseValues.empty())
      EdgeCond.unify(buildNotEqualDomain(TermPE, OtherCaseValues));
  }

  PE->combine(TermPE);
  PE->setDomain(EdgeCond);
  if (PE->PWA.isComplex()) {
    DEBUG(dbgs() << "Too complex edge condition!\n";);
    COMPLEX_DOMAIN++;
    PE->invalidate();
  }
}

PEXP *PolyhedralExpressionBuilder::getDomain(BasicBlock &BB) {
  PEXP *PE = getOrCreateDomain(BB);
  assert(PE);

  if (PE->isInitialized())
    return PE;

  DEBUG(dbgs() << "Get domain of: " << BB.getName() << "\n";);

  auto *L = PI.LI.getLoopFor(&BB);
  bool IsLoopHeader = L && L->getHeader() == &BB;

  if (&BB.getParent()->getEntryBlock() == &BB) {
    DEBUG(dbgs() << "Universe domain for entry [" << BB.getName() << "]\n");
    NUM_DOMAINS++;
    return PE->setDomain(PVSet::universe(PI.getCtx()));
  }

  if (Scope && (Scope == L || !Scope->contains(&BB))) {
    DEBUG(dbgs() << "Universe domain for outside block for [" << BB.getName()
                 << "] [" << (Scope ? Scope->getName() : "<max>") << "]\n");
    NUM_DOMAINS++;
    return PE->setDomain(PVSet::universe(PI.getCtx()));
  }

  if (L) {
    if (!IsLoopHeader) {
      DEBUG(dbgs() << "recurse for loop header [" << L->getHeader()->getName()
                   << "] first!\n");
      getDomain(*L->getHeader());
    } else if (auto *PL = L->getParentLoop()) {
      DEBUG(dbgs() << "recurse for parent loop header ["
                   << PL->getHeader()->getName() << "] first!\n");
      getDomain(*PL->getHeader());
    }

    // After the recursion we have to update PE.
    PE = getOrCreateDomain(BB);
  }

  DEBUG(dbgs() << "-- PE: " << PE << " : " << (void *)PE << " [L: " << L
               << "][LH: " << IsLoopHeader << "]\n";);

  // While we created the domain of the loop header we did produce partial
  // results for other blocks in the loop. While we could update these results
  // we will simply forget them for now and recreate them if necessary.
  auto ForgetDomainsInLoop = [&](Loop &L) {
    if (!IsLoopHeader)
      return;
    for (auto *BB : L.blocks())
      if (BB != L.getHeader())
        PIC.forget(*BB, Scope);
  };

  unsigned LD = getRelativeLoopDepth(&BB);
  PVSet Domain = PVSet::empty(PI.getCtx(), LD);
  for (auto *PredBB : predecessors(&BB)) {
    DEBUG(dbgs() << "\t Pred: " << PredBB->getName() << "\n");
    if (IsLoopHeader && L->contains(PredBB)) {
      DEBUG(dbgs() << "Skip backedge from " << PredBB->getName() << "\n");
      continue;
    }

    PEXP *PredDomainPE = getDomainOnEdge(*PredBB, BB);
    DEBUG(dbgs() << "Domain on edge " << PredBB->getName() << " => "
                 << BB.getName() << ": " << PredDomainPE << "\n");
    if (!PredDomainPE || PredDomainPE->PWA.isComplex()) {
      ForgetDomainsInLoop(*L);
      return PE->invalidate();
    }

    PE->combine(PredDomainPE);

    PVSet PredDomain = PredDomainPE->getDomain();

    auto *PredL = PI.LI.getLoopFor(PredBB);
    if (PredL && L && PredL != L) {
      if (getRelativeLoopDepth(PredBB) == LD)
        PredDomain.dropDimsFrom(LD - 1);
      else
        PredDomain.dropDimsFrom(LD);
    } else if (PredL != L) {
      PredDomain.dropDimsFrom(LD);
    }
    DEBUG(dbgs() << "Domain on edge " << PredBB->getName() << " => "
                 << BB.getName() << ": " << PredDomain << "\n");

    Domain.unify(PredDomain);
    delete PredDomainPE;
    if (Domain.isComplex()) {
      DEBUG(dbgs() << "Too complex domain on edge!\n";);
      COMPLEX_DOMAIN++;
      ForgetDomainsInLoop(*L);
      return PE->invalidate();
    }
  }

  DEBUG(dbgs() << "PE: " << PE << "\n");
  DEBUG(dbgs() << "DOmain: " << Domain << "\n");
  PVSet ParameterRange = PVSet::universe(Domain);

  SmallVector<PVId, 4> Parameters;
  Domain.getParameters(Parameters);
  for (const PVId &PId : Parameters) {
    auto *Parameter = PId.getPayloadAs<Value *>();
    if (!Parameter->getType()->isIntegerTy(1)) {
      DEBUG(dbgs() << "TODO simplify parameter of non boolean type: "
                   << *Parameter << "\n");
      continue;
    }

    ParameterRange.intersect(PVSet::createParameterRange(PId, 0, 1));
  }
  Domain.simplifyParameters(ParameterRange);

  if (!IsLoopHeader) {
    PE->setDomain(Domain, true);
    NUM_DOMAINS++;
    return PE;
  }

  PVSet NonExitDom = Domain.setInputLowerBound(LD - 1, 0);
  DEBUG(dbgs() << "NonExitDom :" << NonExitDom << "\n");
  PE->setDomain(NonExitDom, true);
  SmallVector<BasicBlock *, 4> ExitingBlocks;
  L->getExitingBlocks(ExitingBlocks);
  for (auto *ExitingBB : ExitingBlocks) {
    DEBUG(dbgs() << "ExitingBB: " << ExitingBB->getName() << "\n");
    const PEXP *ExitingBBDomainPE =
        ExitingBB == &BB ? PE : getDomain(*ExitingBB);
    if (!ExitingBBDomainPE || PI.isNonAffine(ExitingBBDomainPE)) {
      DEBUG(dbgs() << "TODO: Fix exiting bb domain hack for loop domains!");
      ForgetDomainsInLoop(*L);
      return PE->invalidate();
    }
    DEBUG(dbgs() << "NonExitDom :" << NonExitDom << "\n");
    PVSet ExitingBBDom = ExitingBBDomainPE->getDomain();
    DEBUG(dbgs() << "ExitingDom: " << ExitingBBDom << "\n");
    PVSet ExitCond;
    for (auto *SuccBB : successors(ExitingBB))
      if (!L->contains(SuccBB)) {
        PEXP *ExitingPE = getDomainOnEdge(*ExitingBBDomainPE, *SuccBB);
        if (!ExitingPE || ExitingPE->PWA.isComplex()) {
          ForgetDomainsInLoop(*L);
          return PE->invalidate();
        }
        PE->combine(ExitingPE);
        ExitCond.unify(ExitingPE->getDomain());
        delete ExitingPE;
      }
    DEBUG(dbgs() << "ExitCond: " << ExitCond << "\n");
    ExitingBBDom.intersect(ExitCond);
    ExitingBBDom.dropDimsFrom(LD);
    DEBUG(dbgs() << "ExitingDom: " << ExitingBBDom << "\n");
    if (ExitingBBDom.getNumDimensions() >= LD)
      ExitingBBDom.getNextIterations(LD - 1);
    DEBUG(dbgs() << "ExitingDom: " << ExitingBBDom << "\n");
    NonExitDom.subtract(ExitingBBDom);
    DEBUG(dbgs() << "NonExitDom :" << NonExitDom << "\n");
    if (NonExitDom.isComplex()) {
      ForgetDomainsInLoop(*L);
      return PE->invalidate();
    }
  }

  DEBUG(dbgs() << "NonExitDom :" << NonExitDom << "\nDom :" << Domain << "\n");
  Domain.fixInputDim(LD - 1, 0);
  DEBUG(dbgs() << "Dom :" << Domain << "\n");
  Domain.unify(NonExitDom);
  DEBUG(dbgs() << "Dom :" << Domain << "\n");

  PE->setDomain(Domain, true);

  ForgetDomainsInLoop(*L);

  NUM_DOMAINS++;
  return PE;
}

PEXP *PolyhedralExpressionBuilder::getDomainOnEdge(BasicBlock &PredBB,
                                                   BasicBlock &BB) {
  PEXP *PredDomPE = getDomain(PredBB);
  assert(PredDomPE);
  if (PI.isNonAffine(PredDomPE))
    return nullptr;
  return getDomainOnEdge(*PredDomPE, BB);
}

PEXP *PolyhedralExpressionBuilder::getDomainOnEdge(const PEXP &PredDomPE,
                                                   BasicBlock &BB) {
  assert(PredDomPE.getKind() == PEXP::EK_DOMAIN);
  auto &PredBB = *cast<BasicBlock>(PredDomPE.getValue());
  DEBUG(dbgs() << "getDomOnEdge: " << PredBB.getName() << " => " << BB.getName()
               << "\n");
  PEXP *EdgeDomain = new PEXP(&BB, Scope);
  setEdgeCondition(EdgeDomain, PredBB, BB);
  DEBUG(dbgs() << "EdgeCond: " << EdgeDomain << " "
               << EdgeDomain->isInitialized() << "\n");
  assert(EdgeDomain->isInitialized());

  EdgeDomain->combine(&PredDomPE);

  PVSet PredDomain = PredDomPE.getDomain();
  DEBUG(dbgs() << "Pred: " << PredBB.getName() << "    BB: " << BB.getName()
               << "\n"
               << "Pred Dom: " << PredDomain << "\n"
               << "EdgeDomain: " << EdgeDomain << "\n");

  return EdgeDomain->setDomain(EdgeDomain->getDomain().intersect(PredDomain),
                               true);
}

PEXP *PolyhedralExpressionBuilder::visitOperand(Value &Op, Instruction &I) {
  PEXP *PE = visit(Op);

  Instruction *OpI = dyn_cast<Instruction>(&Op);
  if (!OpI)
    return PE;

  Loop *OpIL = PI.LI.getLoopFor(OpI->getParent());
  unsigned NumDims = PE->getPWA().getNumInputDimensions();
  unsigned NumLeftLoops = 0;
  while (OpIL && NumDims && !OpIL->contains(&I)) {
    NumLeftLoops++;
    NumDims--;
    OpIL = OpIL->getParentLoop();
  }

  if (NumLeftLoops) {
    PEXP *OpIDomPE = getDomain(*OpI->getParent());
    if (PI.isNonAffine(OpIDomPE))
      PE->getPWA().dropLastInputDims(NumLeftLoops);
    else {
      PVSet OpIDom = OpIDomPE->getDomain();
      OpIDom.maxInLastInputDims(NumLeftLoops);
      PE->getPWA().intersectDomain(OpIDom);
      PE->getPWA().dropLastInputDims(NumLeftLoops);
    }
  }

  return PE;
}

PEXP *PolyhedralExpressionBuilder::visit(Value &V) {

  PEXP *PE = PIC.lookup(V, Scope);
  if (PE && PE->isInitialized()) {
    return PE;
  }

  DEBUG(dbgs() << "Visit V: " << V << " [" << Scope << "]\n");
  if (!V.getType()->isIntegerTy() && !V.getType()->isPointerTy())
    PE = visitParameter(V);
  else if (auto *I = dyn_cast<Instruction>(&V))
    PE = visit(*I);
  else if (auto *CI = dyn_cast<ConstantInt>(&V))
    PE = visit(*CI);
  else if (auto *C = dyn_cast<Constant>(&V))
    PE = visit(*C);
  else
    PE = visitParameter(V);

  assert(PE && PE->isInitialized());

  if (PI.isAffine(PE))
    NUM_EXPRESSIONS++;

  return PE;
}

PEXP *PolyhedralExpressionBuilder::visit(Constant &I) {
  DEBUG(dbgs() << "Visit C: " << I << "\n";);
  if (I.isNullValue())
    return visit(*ConstantInt::get(Type::getInt64Ty(I.getContext()), 0));
  return visitParameter(I);
}

PEXP *PolyhedralExpressionBuilder::visit(ConstantInt &I) {
  DEBUG(dbgs() << "Visit CI: " << I << "\n";);

  auto *PE = getOrCreatePEXP(I);
  if (PE->isInitialized())
    return PE;

  int64_t ConstVal;
  if (auto *CI = dyn_cast<ConstantInt>(&I))
    ConstVal = CI->isOne() ? 1 : CI->getSExtValue();
  else if (I.isNullValue())
    ConstVal = 0;
  else
    llvm_unreachable("Unhandled ConstantInt!");

  PE->PWA = PVAff(PI.getCtx(), ConstVal);
  PE->setKind(PEXP::EK_INTEGER);

  return PE;
}

PEXP *PolyhedralExpressionBuilder::createParameter(PEXP *PE) {
  PE->PWA = PVAff(PI.getParameterId(*PE->getValue()));
  PE->setKind(PEXP::EK_UNKNOWN_VALUE);

  NUM_PARAMETERS++;
  return PE;
}

PEXP *PolyhedralExpressionBuilder::visitParameter(Value &V) {
  DEBUG(dbgs() << "PESIT Par: " << V << "\n");
  auto *PE = getOrCreatePEXP(V);
  return createParameter(PE);
}

unsigned PolyhedralExpressionBuilder::getRelativeLoopDepth(BasicBlock *BB) {
  Loop *L = PI.LI.getLoopFor(BB);
  if (!L)
    return 0;
  if (!Scope)
    return L->getLoopDepth();
  if (!Scope->contains(L))
    return 0;
  return L->getLoopDepth() - Scope->getLoopDepth();
}

PEXP *PolyhedralExpressionBuilder::visit(Instruction &I) {
  if (!I.getType()->isIntegerTy() && !I.getType()->isPointerTy())
    return visitParameter(I);

  DEBUG(dbgs() << "Visit I: " << I << "\n");
  auto *PE = InstVisitor::visit(I);

  unsigned RelLD = getRelativeLoopDepth(I.getParent());
  unsigned NumDims = PE->PWA.getNumInputDimensions();
  DEBUG(dbgs() << "RelLD: " << RelLD << " NumDims " << NumDims << "\n\t => "
               << PE << "\n");
  assert(NumDims <= RelLD);
  PE->PWA.addInputDims(RelLD - NumDims);
  assert(PE->PWA.getNumInputDimensions() == RelLD);

  DEBUG(dbgs() << "Visited I: " << I << "\n\t => " << PE << "\n");
  return PE;
}

PEXP *PolyhedralExpressionBuilder::getTerminatorPEXP(BasicBlock &BB) {
  auto *Term = BB.getTerminator();

  switch (Term->getOpcode()) {
  case Instruction::Br: {
    auto *BI = cast<BranchInst>(Term);
    if (BI->isUnconditional())
      return nullptr;
    else
      return visitOperand(*BI->getCondition(), *Term);
  }
  case Instruction::Switch:
    return visitOperand(*cast<SwitchInst>(Term)->getCondition(), *Term);
  case Instruction::Ret:
  case Instruction::Unreachable:
    return nullptr;
  case Instruction::IndirectBr:
    /// @TODO This can be over-approximated
    return nullptr;
  case Instruction::Invoke:
  case Instruction::Resume:
  case Instruction::CleanupRet:
  case Instruction::CatchRet:
  case Instruction::CatchSwitch:
    return nullptr;
  default:
    return nullptr;
  }

  llvm_unreachable("unknown terminator");
  return nullptr;
}

// ------------------------------------------------------------------------- //

PEXP *PolyhedralExpressionBuilder::visitICmpInst(ICmpInst &I) {

  auto *PE = getOrCreatePEXP(I);
  auto *LPE = visitOperand(*I.getOperand(0), I);
  if (PI.isNonAffine(LPE))
    return visitParameter(I);
  auto *RPE = visitOperand(*I.getOperand(1), I);
  if (PI.isNonAffine(RPE))
    return visitParameter(I);

  DEBUG(dbgs() << "ICMP: " << I << "\n");
  DEBUG(dbgs() << "LPE: " << LPE << "\n");
  DEBUG(dbgs() << "RPE: " << RPE << "\n");

  auto Pred = I.getPredicate();
  auto IPred = I.getInversePredicate();
  auto TrueDomain = PVAff::buildConditionSet(Pred, LPE->PWA, RPE->PWA);
  DEBUG(dbgs() << "TD: " << TrueDomain << "\n");
  if (TrueDomain.isComplex()) {
    DEBUG(dbgs() << "Too complex true domain!\n";);
    COMPLEX_DOMAIN++;
    return visitParameter(I);
  }

  auto FalseDomain = PVAff::buildConditionSet(IPred, LPE->PWA, RPE->PWA);
  DEBUG(dbgs() << "FD: " << FalseDomain << "\n");
  if (FalseDomain.isComplex()) {
    DEBUG(dbgs() << "Too complex false domain!\n";);
    COMPLEX_DOMAIN++;
    return visitParameter(I);
  }

  PE->PWA = PVAff(FalseDomain, 0);
  PE->PWA.union_add(PVAff(TrueDomain, 1));
  PE->combine(LPE);
  PE->combine(RPE);
  PE->Kind = PEXP::EK_INTEGER;
  DEBUG(dbgs() << PE << "\n");
  return PE;
}

PEXP *PolyhedralExpressionBuilder::visitFCmpInst(FCmpInst &I) {
  return visitParameter(I);
}

PEXP *PolyhedralExpressionBuilder::visitLoadInst(LoadInst &I) {
  return visitParameter(I);
}

PEXP *
PolyhedralExpressionBuilder::visitGetElementPtrInst(GetElementPtrInst &I) {
  auto &DL = I.getModule()->getDataLayout();

  auto *PtrPE = visitOperand(*I.getPointerOperand(), I);
  if (PI.isNonAffine(PtrPE))
    return visitParameter(I);

  auto *PE = getOrCreatePEXP(I);
  *PE = *PtrPE;

  auto *Ty = I.getPointerOperandType();
  for (auto &Op : make_range(I.idx_begin(), I.idx_end())) {
    auto *PEOp = visitOperand(*Op, I);
    if (PI.isNonAffine(PEOp))
      return visitParameter(I);

    if (Ty->isStructTy()) {
      // TODO: Struct
      DEBUG(dbgs() << "TODO: Struct ty " << *Ty << " for " << I << "\n");
      return visitParameter(I);
    }

    uint64_t Size = 0;
    if (Ty->isPointerTy()) {
      Ty = Ty->getPointerElementType();
      Size = DL.getTypeAllocSize(Ty);
    } else if (Ty->isArrayTy()) {
      Ty = Ty->getArrayElementType();
      Size = DL.getTypeAllocSize(Ty);
    } else {
      DEBUG(dbgs() << "TODO: Unknown ty " << *Ty << " for " << I << "\n");
      return visitParameter(I);
    }
    DEBUG(dbgs() << "Ty: " << *Ty << " Size: " << Size << "\n");
    DEBUG(dbgs() << "GepPE: " << PE << "\n");

    PE->combine(PEOp);

    PVAff ScaledPWA(PEOp->getDomain(), Size);
    ScaledPWA.multiply(PEOp->getPWA());
    PE->PWA.add(ScaledPWA);
    DEBUG(dbgs() << "GepPE: " << PE << "\n");
  }

  return PE;
}

PEXP *PolyhedralExpressionBuilder::visitCastInst(CastInst &I) {
  PEXP *PE = getOrCreatePEXP(I);
  *PE = *visitOperand(*I.getOperand(0), I);

  switch (I.getOpcode()) {
  case Instruction::Trunc: {
    // Handle changed values.
    unsigned TypeWidth =
        I.getModule()->getDataLayout().getTypeSizeInBits(I.getType());
    if (TypeWidth >= 64)
      return visitParameter(I);
    uint64_t ExpVal = ((uint64_t)1) << TypeWidth;
    PE->addInvalidDomain(
        PE->getPWA().getGreaterEqualDomain(PVAff(PE->getPWA(), ExpVal)));
    PE->addInvalidDomain(
        PE->getPWA().getLessEqualDomain(PVAff(PE->getPWA(), -ExpVal)));
    break;
  }
  case Instruction::ZExt:
    // Handle negative values.
    PE->addInvalidDomain(
        PE->getPWA().getLessThanDomain(PVAff(PE->getPWA(), 0)));
    break;
  case Instruction::SExt:
  case Instruction::FPToSI:
  case Instruction::FPToUI:
  case Instruction::PtrToInt:
  case Instruction::IntToPtr:
  case Instruction::BitCast:
  case Instruction::AddrSpaceCast:
    // No-op
    break;
  default:
    llvm_unreachable("Unhandled cast operation!\n");
  }

  return PE;
}

PEXP *PolyhedralExpressionBuilder::visitSelectInst(SelectInst &I) {
  auto *CondPE = visitOperand(*I.getCondition(), I);
  if (PI.isNonAffine(CondPE))
    return visitParameter(I);

  auto *OpTrue = visitOperand(*I.getTrueValue(), I);
  if (PI.isNonAffine(OpTrue))
    return visitParameter(I);

  auto *OpFalse = visitOperand(*I.getFalseValue(), I);
  if (PI.isNonAffine(OpFalse))
    return visitParameter(I);

  auto CondZero = CondPE->getPWA().zeroSet();
  auto CondOne = CondPE->getPWA().nonZeroSet();

  auto *PE = getOrCreatePEXP(I);
  if (!PI.isNonAffine(OpTrue))
    PE->combine(OpTrue);

  if (!PI.isNonAffine(OpFalse))
    PE->combine(OpFalse);

  if (PI.isNonAffine(OpTrue)) {
    PE->InvalidDomain.unify(CondOne);
    PE->PWA = OpFalse->getPWA();
    PE->setKind(OpFalse->getKind());
    return PE;
  }

  if (PI.isNonAffine(OpFalse)) {
    PE->InvalidDomain.unify(CondZero);
    PE->PWA = OpTrue->getPWA();
    PE->setKind(OpTrue->getKind());
    return PE;
  }

  PE->PWA = PVAff::createSelect(CondPE->getPWA(), OpTrue->getPWA(),
                                OpFalse->getPWA());
  return PE;
}

PEXP *PolyhedralExpressionBuilder::visitInvokeInst(InvokeInst &I) {
  return visitParameter(I);
}

PEXP *PolyhedralExpressionBuilder::visitCallInst(CallInst &I) {
  return visitParameter(I);
}

PEXP *PolyhedralExpressionBuilder::visitConditionalPHINode(PHINode &I) {
  bool InvalidOtherwise = false;

  unsigned RelLD = getRelativeLoopDepth(I.getParent());
  DEBUG(dbgs() << "\nCondPHI: " << I << " [RelLD: " << RelLD << "]\n");
  auto *PE = getOrCreatePEXP(I);
  PE->setKind(PEXP::EK_NONE);

  for (unsigned u = 0, e = I.getNumIncomingValues(); u < e; u++) {
    PEXP *PredEdgePE = getDomainOnEdge(*I.getIncomingBlock(u), *I.getParent());
    if (!PredEdgePE) {
      DEBUG(dbgs() << "PHI op " << u << " (" << *I.getIncomingValue(u)
                   << ") no domain on edge\n");
      InvalidOtherwise = true;
      continue;
    }

    auto *PredOpPE = visit(*I.getIncomingValue(u));
    assert(!PI.isNonAffine(PredOpPE));

    DEBUG(dbgs() << "Op: " << PredOpPE << " " << u
                 << "\n On edge: " << PredEdgePE << "\n";);
    PE->combine(PredEdgePE);
    PVSet PredDomain = PredEdgePE->getDomain();
    unsigned NumDims = PredDomain.getNumDimensions();
    assert(RelLD <= NumDims);
    if (RelLD < NumDims)
      PredDomain.maxInLastInputDims(NumDims - RelLD);
    DEBUG(dbgs() << "PredDom: " << PredDomain << "\n");
    PE->combine(PredOpPE, PVAff::createUnionAdd, &PredDomain);
    delete PredEdgePE;
  }

  if (!PE->isInitialized())
    return visitParameter(I);

  DEBUG(dbgs() << "\nCONDITIONAL PHI: " << I << "\n" << PE << "\n";);
  assert(PE->PWA.getNumInputDimensions() >= RelLD);

  PE->PWA.dropLastInputDims(PE->PWA.getNumInputDimensions() - RelLD);
  if (InvalidOtherwise) {
    assert(PE->PWA);
    PVSet Dom = PE->getDomain();
    PE->InvalidDomain.unify(Dom.complement());
  }

  DEBUG(dbgs() << "\nCONDITIONAL PHI: " << I << "\n" << PE << "\n";);
  return PE;
}

PEXP *PolyhedralExpressionBuilder::visitPHINode(PHINode &I) {
  auto &BB = *I.getParent();
  Loop *L = PI.LI.getLoopFor(&BB);
  bool IsLoopHeader = L && L->getHeader() == &BB;

  if (!IsLoopHeader)
    return visitConditionalPHINode(I);

  PEXP *ParamPE = PIC.getOrCreatePEXP(I, L);
  assert(ParamPE);

  PVId Id = PI.getParameterId(I);
  if (!ParamPE->isInitialized()) {
    ParamPE->PWA = PVAff(Id);
    ParamPE->Kind = PEXP::EK_UNKNOWN_VALUE;
  }
  if (Scope == L) {
    DEBUG(dbgs() << "PHI loop is scope. Parametric value is sufficent!\n");
    return ParamPE;
  }

  PEXP *PE = getOrCreatePEXP(I);
  assert(PE && !PE->isInitialized());
  if (Scope && !Scope->contains(&I)) {
    DEBUG(dbgs() << "PHI not in scope. Parametric value is sufficent!\n");
    *PE = *ParamPE;
    return PE;
  }

  unsigned LoopDim = getRelativeLoopDepth(L->getHeader());

  auto OldScope = Scope;
  setScope(L);

  bool ConstantStride = true;
  PVAff BackEdgeOp;
  unsigned NumLatches = L->getNumBackEdges();
  for (unsigned u = 0, e = I.getNumIncomingValues(); u != e; u++) {
    auto *OpBB = I.getIncomingBlock(u);
    if (!L->contains(OpBB))
      continue;

    auto *OpVal = I.getIncomingValue(u);
    auto *OpPE = visit(*OpVal);
    PVAff OpAff = OpPE->getPWA();
    OpAff.dropParameter(Id);
    if (!OpAff.isConstant()) {
      DEBUG(dbgs() << "PHI has non constant stride: " << OpPE << "\n\tfor "
                   << *OpVal << "\n");
      if (OpPE->getPWA() != OpAff) {
        DEBUG(dbgs() << "  Operand involves PHI! Invalid!\n");
        setScope(OldScope);
        return visitParameter(I);
      }

      ConstantStride = false;
      if (OldScope != Scope) {
        setScope(OldScope);
        OpPE = visit(*OpVal);
        setScope(L);
      }
    }

    OpAff = OpPE->getPWA();
    if (NumLatches > 1 || !ConstantStride) {
      PEXP *EdgeDomPE = getDomainOnEdge(*OpBB, BB);
      if (!EdgeDomPE) {
        DEBUG(dbgs() << "PHI back edge has unknown domain!\n");
        setScope(OldScope);
        return visitParameter(I);
      }

      PVSet EdgeDom = EdgeDomPE->getDomain();
      DEBUG(dbgs() << "EdgeDom: " << EdgeDom << " [LD: " << LoopDim << "]\n");
      EdgeDom.setInputLowerBound(LoopDim - 1, 1);
      DEBUG(dbgs() << "EdgeDom: " << EdgeDom << " [LD: " << LoopDim << "]\n");
      OpAff.intersectDomain(EdgeDom);
      delete EdgeDomPE;
    }

    DEBUG(dbgs() << "Back edge Op: " << OpAff << "\n");
    BackEdgeOp.union_add(OpAff);
  }

  DEBUG(dbgs() << "Final Back edge Op: " << BackEdgeOp << "\n");
  if (ConstantStride) {
    BackEdgeOp = BackEdgeOp.perPiecePHIEvolution(Id, LoopDim - 1);
    if (!BackEdgeOp) {
      DEBUG(dbgs() << "TODO: non constant back edge operand value!\n");
      setScope(OldScope);
      return visitParameter(I);
    }
  }

  DEBUG(dbgs() << "BackEdgeOp: " << BackEdgeOp << "\n");

  PE->PWA = PVAff();
  PE->PWA.union_add(BackEdgeOp);
  PE->PWA.dropParameter(Id);
  setScope(OldScope);

  for (unsigned u = 0, e = I.getNumIncomingValues(); u != e; u++) {
    auto *OpBB = I.getIncomingBlock(u);
    if (L->contains(OpBB))
      continue;

    auto *OpVal = I.getIncomingValue(u);
    auto *OpPE = visit(*OpVal);
    if (PI.isNonAffine(OpPE)) {
      return visitParameter(I);
    }

    PVAff OpAff = OpPE->getPWA();
    assert(e > NumLatches);
    if (e - NumLatches > 1 || !ConstantStride) {
      PEXP *EdgeDomPE = getDomainOnEdge(*OpBB, BB);
      if (!EdgeDomPE) {
        return visitParameter(I);
      }

      PVSet EdgeDom = EdgeDomPE->getDomain();
      EdgeDom.fixInputDim(LoopDim - 1, 0);
      OpAff.intersectDomain(EdgeDom);
      delete EdgeDomPE;
    }

    DEBUG(dbgs() << "Init Op: " << OpAff << "\n");
    PE->PWA.union_add(OpAff);
    PE->combine(OpPE);
  }

  PE->Kind = PEXP::EK_UNKNOWN_VALUE;
  DEBUG(dbgs() << "PHI: " << PE->PWA << "\n");

  return PE;
}

PEXP *PolyhedralExpressionBuilder::visitBinaryOperator(BinaryOperator &I) {

  Value *Op0 = I.getOperand(0);
  auto *PEOp0 = visitOperand(*Op0, I);
  if (PI.isNonAffine(PEOp0))
    return visitParameter(I);

  Value *Op1 = I.getOperand(1);
  auto *PEOp1 = visitOperand(*Op1, I);
  if (PI.isNonAffine(PEOp1))
    return visitParameter(I);

  auto *PE = getOrCreatePEXP(I);
  switch (I.getOpcode()) {
  case Instruction::Add:
    PE->assign(PEOp0, PEOp1, PVAff::createAdd);
    PE->adjustInvalidAndKnownDomain();
    return PE;
  case Instruction::Sub:
    PE->assign(PEOp0, PEOp1, PVAff::createSub);
    PE->adjustInvalidAndKnownDomain();
    return PE;

  case Instruction::Mul:
    if (PEOp0->Kind != PEXP::EK_INTEGER && PEOp1->Kind != PEXP::EK_INTEGER)
      return visitParameter(I);
    PE->assign(PEOp0, PEOp1, PVAff::createMultiply);
    PE->adjustInvalidAndKnownDomain();
    return PE;

  case Instruction::SRem:
// TODO: This is not yet compatible with the PHI handling which assumes
// monotonicity!
#if 0
    if (PEOp1->Kind == PEXP::EK_INTEGER) {
      auto NZ = PEOp1->PWA.nonZeroSet();
      PEOp1->PWA.intersectDomain(NZ);
      return PE->assign(PEOp0, PEOp1, isl_pw_aff_tdiv_r);
    }
#endif
    return visitParameter(I);
  case Instruction::SDiv:
    if (PEOp1->Kind == PEXP::EK_INTEGER) {
      auto NZ = PEOp1->PWA.nonZeroSet();
      PEOp1->PWA.intersectDomain(NZ);
      return PE->assign(PEOp0, PEOp1, PVAff::createSDiv);
    }
    return visitParameter(I);
  case Instruction::Shl:
    if (PEOp1->Kind == PEXP::EK_INTEGER) {
      return PE->assign(PEOp0, PEOp1, PVAff::createShiftLeft);
    }
    return visitParameter(I);
  case Instruction::UDiv:
  case Instruction::AShr:
  case Instruction::LShr:
  case Instruction::URem:
    // TODO
    return visitParameter(I);

  // Bit operations
  case Instruction::And:
    if (I.getType()->isIntegerTy(1)) {
      PE->assign(PEOp0, PEOp1, PVAff::createMultiply);
      PE->PWA.select(getOne(PE->PWA), getZero(PE->PWA));
      return PE;
    }
    return visitParameter(I);
  case Instruction::Or:
    if (I.getType()->isIntegerTy(1)) {
      PE->assign(PEOp0, PEOp1, PVAff::createAdd);
      PE->PWA.select(getOne(PE->PWA), getZero(PE->PWA));
      return PE;
    }
    return visitParameter(I);
  case Instruction::Xor:
    if (I.getType()->isIntegerTy(1)) {
      auto OneBitXOR = [this](const PVAff &PWA0, const PVAff &PWA1) {
        auto OnePWA = getOne(PWA0);
        auto ZeroPWA = getZero(PWA0);
        auto Mul = PVAff::createMultiply(PWA0, PWA1);
        auto MulZero = Mul.zeroSet();
        auto Add = PVAff::createAdd(PWA0, PWA1);
        auto AddNonZero = Add.nonZeroSet();
        auto TrueSet = MulZero.intersect(AddNonZero);
        return PVAff(TrueSet).select(OnePWA, ZeroPWA);
      };
      return PE->assign(PEOp0, PEOp1, OneBitXOR);
    }
    if (auto *COp0 = dyn_cast<ConstantInt>(Op0))
      if (COp0->isMinusOne()) {
        PE->assign(PEOp0, PEOp1, PVAff::createMultiply);
        PE->combine(PEOp0, PVAff::createAdd);
      }
    if (auto *COp1 = dyn_cast<ConstantInt>(Op1))
      if (COp1->isMinusOne()) {
        PE->assign(PEOp0, PEOp1, PVAff::createMultiply);
        PE->combine(PEOp1, PVAff::createAdd);
      }

    // TODO
    return visitParameter(I);
  case Instruction::FAdd:
  case Instruction::FSub:
  case Instruction::FMul:
  case Instruction::FDiv:
  case Instruction::FRem:
  case Instruction::BinaryOpsEnd:
    break;
  }

  llvm_unreachable("Invalid Binary Operation");
}

PEXP *PolyhedralExpressionBuilder::visitAllocaInst(AllocaInst &I) {
  return visitParameter(I);
}

PEXP *PolyhedralExpressionBuilder::visitInstruction(Instruction &I) {
  DEBUG(dbgs() << "UNKNOWN INST " << I << "\n";);
  assert(!I.getType()->isVoidTy());
  return visitParameter(I);
}

PVSet PolyhedralExpressionBuilder::buildNotEqualDomain(
    const PEXP *PE, ArrayRef<Constant *> CIs) {
  assert(PE->Kind != PEXP::EK_NON_AFFINE && PE->PWA);

  PVSet NotEqualDomain;
  for (auto *CI : CIs) {
    auto *CPE = visit(*static_cast<Value *>(CI));
    assert(CPE->Kind == PEXP::EK_INTEGER && CPE->PWA && !CPE->InvalidDomain);

    NotEqualDomain.intersect(
        PVAff::buildConditionSet(ICmpInst::ICMP_NE, PE->PWA, CPE->PWA));
  }

  return NotEqualDomain;
}

PVSet PolyhedralExpressionBuilder::buildEqualDomain(const PEXP *PE,
                                                    Constant &CI) {
  assert(PE->Kind != PEXP::EK_NON_AFFINE && PE->PWA);

  auto *CPE = visit(static_cast<Value &>(CI));
  assert(CPE->Kind == PEXP::EK_INTEGER && CPE->PWA && !CPE->InvalidDomain);

  return PVAff::buildConditionSet(ICmpInst::ICMP_EQ, PE->PWA, CPE->PWA);
}

// ------------------------------------------------------------------------- //

PolyhedralValueInfo::PolyhedralValueInfo(PVCtx Ctx, LoopInfo &LI)
    : Ctx(Ctx), LI(LI), PEBuilder(new PolyhedralExpressionBuilder(*this)) {
}

PolyhedralValueInfo::~PolyhedralValueInfo() { delete PEBuilder; }

const PEXP *PolyhedralValueInfo::getDomainFor(BasicBlock *BB, Loop *Scope) {
  PEBuilder->setScope(Scope);
  return PEBuilder->getDomain(*BB);
}

const PEXP *PolyhedralValueInfo::getPEXP(Value *V, Loop *Scope) {
  PEBuilder->setScope(Scope);
  return PEBuilder->visit(*V);
}

PVId PolyhedralValueInfo::getParameterId(Value &V) {
  return PEBuilder->getParameterId(V);
}

bool PolyhedralValueInfo::isUnknown(const PEXP *PE) const {
  return PE->Kind == PEXP::EK_UNKNOWN_VALUE;
}

bool PolyhedralValueInfo::isInteger(const PEXP *PE) const {
  return PE->Kind == PEXP::EK_INTEGER;
}

bool PolyhedralValueInfo::isConstant(const PEXP *PE) const {
  return isInteger(PE) && PE->PWA.getNumPieces() == 1;
}

bool PolyhedralValueInfo::isAffine(const PEXP *PE) const {
  return PE->Kind != PEXP::EK_NON_AFFINE && PE->isInitialized();
}

bool PolyhedralValueInfo::isNonAffine(const PEXP *PE) const {
  return PE->Kind == PEXP::EK_NON_AFFINE;
}

bool PolyhedralValueInfo::hasScope(const PEXP *PE, Loop *Scope) const {
  SmallVector<Value *, 4> Values;
  getParameters(PE, Values);
  for (Value *V : Values) {
    auto *I = dyn_cast<Instruction>(V);
    if (!I)
      continue;
    if (!Scope || Scope->contains(I))
      return false;
  }
  return true;
}

bool PolyhedralValueInfo::hasFunctionScope(const PEXP *PE) const {
  return hasScope(PE, nullptr);
}

void PolyhedralValueInfo::getParameters(const PEXP *PE,
                                        SmallVectorImpl<PVId> &Values) const {
  const PVAff &PWA = PE->getPWA();
  PWA.getParameters(Values);
}

void PolyhedralValueInfo::getParameters(
    const PEXP *PE, SmallVectorImpl<Value *> &Values) const {
  const PVAff &PWA = PE->getPWA();
  PWA.getParameters(Values);
}

bool PolyhedralValueInfo::isKnownToHold(Value *LHS, Value *RHS,
                                        ICmpInst::Predicate Pred,
                                        Instruction *IP, Loop *Scope) {
  const PEXP *LHSPE = getPEXP(LHS, Scope);
  if (isNonAffine(LHSPE))
    return false;

  const PEXP *RHSPE = getPEXP(RHS, Scope);
  if (isNonAffine(RHSPE))
    return false;

  const PEXP *IPDomPE = IP ? getDomainFor(IP->getParent(), Scope) : nullptr;
  if (IP && (isNonAffine(IPDomPE) || !IPDomPE->getInvalidDomain().isEmpty()))
    return false;

  PVSet LHSInvDom = LHSPE->getInvalidDomain();
  PVSet RHSInvDom = RHSPE->getInvalidDomain();
  if (IPDomPE) {
    LHSInvDom.intersect(IPDomPE->getDomain());
    RHSInvDom.intersect(IPDomPE->getDomain());
  }

  if (!LHSInvDom.isEmpty() || !RHSInvDom.isEmpty())
    return false;

  PVAff LHSAff = LHSPE->getPWA();
  PVAff RHSAff = RHSPE->getPWA();

  if (IPDomPE) {
    LHSAff.intersectDomain(IPDomPE->getDomain());
    RHSAff.intersectDomain(IPDomPE->getDomain());
  }

  auto FalseDomain = PVAff::buildConditionSet(
      ICmpInst::getInversePredicate(Pred), LHSAff, RHSAff);
  return FalseDomain.isEmpty();
}

void PolyhedralValueInfo::print(raw_ostream &OS) const {}

// ------------------------------------------------------------------------- //

char PolyhedralValueInfoWrapperPass::ID = 0;

void PolyhedralValueInfoWrapperPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<LoopInfoWrapperPass>();
  AU.setPreservesAll();
}

void PolyhedralValueInfoWrapperPass::releaseMemory() {
  F = nullptr;
  delete PI;
  PI = nullptr;
}

bool PolyhedralValueInfoWrapperPass::runOnFunction(Function &F) {

  auto &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

  delete PI;
  PI = new PolyhedralValueInfo(Ctx, LI);

  this->F = &F;

  return false;
}

void PolyhedralValueInfoWrapperPass::print(raw_ostream &OS,
                                           const Module *) const {
  PI->print(OS);

  if (!F)
    return;

  PolyhedralValueInfoWrapperPass &PIWP =
      *const_cast<PolyhedralValueInfoWrapperPass *>(this);
  LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

  for (auto &BB : *PIWP.F) {
    Loop *Scope = LI.getLoopFor(&BB);
    do {
      PIWP.PI->getDomainFor(&BB, Scope);
      for (auto &Inst : BB)
        if (!Inst.getType()->isVoidTy())
          PIWP.PI->getPEXP(&Inst, Scope);
      if (!Scope)
        break;
      Scope = Scope->getParentLoop();
    } while (true);
  }

  for (auto &BB : *PIWP.F) {
    Loop *Scope = LI.getLoopFor(&BB);
    do {
      OS << "Scope: " << (Scope ? Scope->getName() : "<none>") << "\n";
      const PEXP *PE = PIWP.PI->getDomainFor(&BB, Scope);
      OS << "Domain of " << BB.getName() << ":\n";
      OS << "\t => " << PE << "\n";
      for (auto &Inst : BB) {
        if (Inst.getType()->isVoidTy()) {
          OS << "\tValue of " << Inst << ":\n";
          OS << "\t\t => void type!\n";
          continue;
        }
        const PEXP *PE = PIWP.PI->getPEXP(&Inst, Scope);
        OS << "\tValue of " << Inst << ":\n";
        OS << "\t\t => " << PE << "\n";
        SmallVector<Value *, 4> Values;
        PIWP.PI->getParameters(PE, Values);
        if (Values.empty())
          continue;
        OS << "\t\t\tParams:\n";
        for (Value *Val : Values)
          OS << "\t\t\t - " << *Val << "\n";
      }

      if (!Scope)
        break;
      Scope = Scope->getParentLoop();
    } while (true);
  }
}

FunctionPass *llvm::createPolyhedralValueInfoWrapperPass() {
  return new PolyhedralValueInfoWrapperPass();
}

void PolyhedralValueInfoWrapperPass::dump() const {
  return print(dbgs(), nullptr);
}

AnalysisKey PolyhedralValueInfoAnalysis::Key;

PolyhedralValueInfo
PolyhedralValueInfoAnalysis::run(Function &F, FunctionAnalysisManager &AM) {
  auto &LI = AM.getResult<LoopAnalysis>(F);
  return PolyhedralValueInfo(Ctx, LI);
}

INITIALIZE_PASS_BEGIN(PolyhedralValueInfoWrapperPass, "polyhedral-value-info",
                      "Polyhedral value analysis", false, true);
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass);
INITIALIZE_PASS_END(PolyhedralValueInfoWrapperPass, "polyhedral-value-info",
                    "Polyhedral value analysis", false, true)
