//===--- FunctionCognitiveComplexityCheck.cpp - clang-tidy-----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "FunctionCognitiveComplexityCheck.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace readability {
namespace {

struct CognitiveComplexity final {
  // Limit of 25 is the "upstream"'s default.
  static constexpr const unsigned DefaultLimit = 25U;

  // Any increment is based on some combination of reasons.
  // Here are all the possible reasons:
  enum Criteria : unsigned char {
    None = 0,

    // B1, increases cognitive complexity (by 1)
    Increment = 1 << 0,

    // B2, increases current nesting level (by 1)
    IncrementNesting = 1 << 1,

    // B3, increases cognitive complexity by the current nesting level
    // Applied before IncrementNesting
    PenalizeNesting = 1 << 2,

    All = Increment | PenalizeNesting | IncrementNesting,
  };

  // All the possible messages that can be outputed. The choice of the message
  // to use is based of the combination of the Criterias
  static const std::array<const char *const, 4> Msgs;

  // The helper struct used to record one increment occurence, with all the
  // details nessesary
  struct Detail final {
    const SourceLocation Loc; // what caused the increment?
    const unsigned short Nesting; // how deeply nestedly is Loc located?
    const Criteria C : 3; // the criteria of the increment

    Detail(SourceLocation SLoc, unsigned short CurrentNesting, Criteria Crit)
        : Loc(SLoc), Nesting(CurrentNesting), C(Crit) {}

    // To minimize the the sizeof(Detail), we only store the minimal info there.
    // This function is used to convert from the stored info into the usable
    // information - what message to output, how much of an increment did this
    // occurence actually result in
    std::pair<const char *, unsigned short> Process() const {
      assert(C != Criteria::None && "invalid criteria");

      const char *Msg;
      unsigned short Increment;

      if (C == Criteria::All) {
        Increment = 1 + Nesting;
        Msg = Msgs[0];
      } else if (C == (Criteria::Increment | Criteria::IncrementNesting)) {
        Increment = 1;
        Msg = Msgs[1];
      } else if (C == Criteria::Increment) {
        Increment = 1;
        Msg = Msgs[2];
      } else if (C == Criteria::IncrementNesting) {
        Msg = Msgs[3];
      } else
        llvm_unreachable("should not get to here.");

      return std::make_pair(Msg, Increment);
    }
  };
  // static_assert(sizeof(Detail) <= 8, "it's best to keep the size minimal");

  // Based on the publicly-avaliable numbers for some big open-source projects
  // https://sonarcloud.io/projects?languages=c%2Ccpp&size=5   we can estimate:
  // value ~20 would result in no allocations for 98% of functions, ~12 for 96%,
  // ~10 for 91%, ~8 for 88%, ~6 for 84%, ~4 for 77%, ~2 for 64%, and ~1 for 37%
  SmallVector<Detail, DefaultLimit> Details; // 25 elements is 200 bytes.
  // Yes, 25 is a magic number. This is the seemingly-sane default for the
  // upper limit for function cognitive complexity. Thus it would make sense
  // to avoid allocations for any function that does not violate the limit.

  // The grand total Cognitive Complexity of the function
  unsigned Total = 0;

  // The function used to store new increment, calculate the total complexity
  void Account(SourceLocation Loc, unsigned short Nesting, Criteria C);
};

const std::array<const char *const, 4> CognitiveComplexity::Msgs = {{
    // B1 + B2 + B3
    "+%0, including nesting penalty of %1, nesting level increased to %2",

    // B1 + B2
    "+%0, nesting level increased to %2",

    // B1
    "+%0",

    // B2
    "nesting level increased to %2",
}};

// Criteria is a bitset, thus a few helpers are needed
static CognitiveComplexity::Criteria
operator|(CognitiveComplexity::Criteria lhs,
          CognitiveComplexity::Criteria rhs) {
  return static_cast<CognitiveComplexity::Criteria>(
      static_cast<std::underlying_type<CognitiveComplexity::Criteria>::type>(
          lhs) |
      static_cast<std::underlying_type<CognitiveComplexity::Criteria>::type>(
          rhs));
}
static CognitiveComplexity::Criteria
operator&(CognitiveComplexity::Criteria lhs,
          CognitiveComplexity::Criteria rhs) {
  return static_cast<CognitiveComplexity::Criteria>(
      static_cast<std::underlying_type<CognitiveComplexity::Criteria>::type>(
          lhs) &
      static_cast<std::underlying_type<CognitiveComplexity::Criteria>::type>(
          rhs));
}
static CognitiveComplexity::Criteria &
operator|=(CognitiveComplexity::Criteria &lhs,
           CognitiveComplexity::Criteria rhs) {
  lhs = operator|(lhs, rhs);
  return lhs;
}
static CognitiveComplexity::Criteria &
operator&=(CognitiveComplexity::Criteria &lhs,
           CognitiveComplexity::Criteria rhs) {
  lhs = operator&(lhs, rhs);
  return lhs;
}

void CognitiveComplexity::Account(SourceLocation Loc, unsigned short Nesting,
                                  Criteria C) {
  C &= Criteria::All;
  assert(C != Criteria::None && "invalid criteria");

  Details.emplace_back(Loc, Nesting, C);
  const Detail &d(Details.back());

  const std::pair<const char *, unsigned short> Reasoning(d.Process());
  Total += Reasoning.second;
}

class FunctionASTVisitor final
    : public RecursiveASTVisitor<FunctionASTVisitor> {
  using Base = RecursiveASTVisitor<FunctionASTVisitor>;

  // Count the current nesting level (increased by Criteria::IncrementNesting)
  unsigned short CurrentNestingLevel = 0;

  // Used to efficiently know the last type of binary sequence operator that
  // was encoutered. It would make sense for the function call to start the
  // new sequence, thus it is a stack.
  std::stack<            Optional<BinaryOperator::Opcode>,
             SmallVector<Optional<BinaryOperator::Opcode>, 4>>
      BinaryOperatorsStack;

public:

  // Important piece of the information: nesting level is only increased for the
  // body, but not the init/cond/inc !

#define IncreasesNestingLevel(CLASS)                                           \
  bool Traverse##CLASS(CLASS *Node) {                                          \
    if (!Node)                                                                 \
      return Base::Traverse##CLASS(Node);                                      \
                                                                               \
    ++CurrentNestingLevel;                                                     \
    const bool ShouldContinue = Base::Traverse##CLASS(Node);                   \
    --CurrentNestingLevel;                                                     \
                                                                               \
    return ShouldContinue;                                                     \
  }

  // B2. Nesting level
  // The following structures increment the nesting level:
  IncreasesNestingLevel(DefaultStmt);
  IncreasesNestingLevel(CXXCatchStmt);
  // Other instances are more complex, and need standalone functions

#undef IncreasesNestingLevel

  bool TraverseIfStmt(IfStmt *Node, bool InElseIf = false) {
    if (!Node)
      return Base::TraverseIfStmt(Node);

    {
      CognitiveComplexity::Criteria Reasons =
          CognitiveComplexity::Criteria::None;

      // if   increases cognitive complexity
      Reasons |= CognitiveComplexity::Criteria::Increment;
      // if   increases nesting level
      Reasons |= CognitiveComplexity::Criteria::IncrementNesting;

      // if   receives a nesting increment commensurate with it's nested depth
      // but only if it is not part of  else if
      if (!InElseIf)
        Reasons |= CognitiveComplexity::Criteria::PenalizeNesting;

      CC.Account(Node->getIfLoc(), CurrentNestingLevel, Reasons);
    }

    bool ShouldContinue = TraverseStmt(Node->getInit());
    if (!ShouldContinue)
      return ShouldContinue;

    ShouldContinue = TraverseStmt(Node->getCond());
    if (!ShouldContinue)
      return ShouldContinue;

    // if   increases nesting level
    ++CurrentNestingLevel;
    ShouldContinue = TraverseStmt(Node->getThen());
    --CurrentNestingLevel;

    if (!ShouldContinue || !Node->getElse())
      return ShouldContinue;

    // else  OR  else if   increases cognitive complexity and nesting level
    // but  else if  does NOT result in double increase !
    if (isa<IfStmt>(Node->getElse()))
      return TraverseIfStmt(dyn_cast<IfStmt>(Node->getElse()), true);

    {
      CognitiveComplexity::Criteria Reasons =
          CognitiveComplexity::Criteria::None;

      // else   increases cognitive complexity
      Reasons |= CognitiveComplexity::Criteria::Increment;
      // else   increases nesting level
      Reasons |= CognitiveComplexity::Criteria::IncrementNesting;
      // else   DOES NOT receive a nesting increment commensurate with it's
      // nested depth

      CC.Account(Node->getElseLoc(), CurrentNestingLevel, Reasons);
    }

    ++CurrentNestingLevel;
    ShouldContinue = TraverseStmt(Node->getElse());
    --CurrentNestingLevel;

    return ShouldContinue;
  }

  bool TraverseConditionalOperator(ConditionalOperator *Node) {
    if (!Node)
      return Base::TraverseConditionalOperator(Node);

    bool ShouldContinue = TraverseStmt(Node->getCond());
    if (!ShouldContinue)
      return ShouldContinue;

    ++CurrentNestingLevel;
    ShouldContinue = TraverseStmt(Node->getLHS());
    --CurrentNestingLevel;

    if (!ShouldContinue)
      return ShouldContinue;

    ++CurrentNestingLevel;
    ShouldContinue = TraverseStmt(Node->getRHS());
    --CurrentNestingLevel;

    return ShouldContinue;
  }

  bool TraverseCaseStmt(CaseStmt *Node) {
    if (!Node)
      return Base::TraverseCaseStmt(Node);

    bool ShouldContinue = TraverseStmt(Node->getLHS());
    if (!ShouldContinue)
      return ShouldContinue;

    ShouldContinue = TraverseStmt(Node->getRHS());
    if (!ShouldContinue)
      return ShouldContinue;

    ++CurrentNestingLevel;
    ShouldContinue = TraverseStmt(Node->getSubStmt());
    --CurrentNestingLevel;

    return ShouldContinue;
  }

  bool TraverseForStmt(ForStmt *Node) {
    if (!Node)
      return Base::TraverseForStmt(Node);

    bool ShouldContinue = TraverseStmt(Node->getInit());
    if (!ShouldContinue)
      return ShouldContinue;

    ShouldContinue = TraverseStmt(Node->getCond());
    if (!ShouldContinue)
      return ShouldContinue;

    ShouldContinue = TraverseStmt(Node->getInc());
    if (!ShouldContinue)
      return ShouldContinue;

    ++CurrentNestingLevel;
    ShouldContinue = TraverseStmt(Node->getBody());
    --CurrentNestingLevel;

    return ShouldContinue;
  }

  bool TraverseCXXForRangeStmt(CXXForRangeStmt *Node) {
    if (!Node)
      return Base::TraverseCXXForRangeStmt(Node);

    bool ShouldContinue = TraverseDecl(Node->getLoopVariable());
    if (!ShouldContinue)
      return ShouldContinue;

    ShouldContinue = TraverseStmt(Node->getRangeInit());
    if (!ShouldContinue)
      return ShouldContinue;

    ++CurrentNestingLevel;
    ShouldContinue = TraverseStmt(Node->getBody());
    --CurrentNestingLevel;

    return ShouldContinue;
  }

  bool TraverseWhileStmt(WhileStmt *Node) {
    if (!Node)
      return Base::TraverseWhileStmt(Node);

    bool ShouldContinue = TraverseStmt(Node->getCond());
    if (!ShouldContinue)
      return ShouldContinue;

    ++CurrentNestingLevel;
    ShouldContinue = TraverseStmt(Node->getBody());
    --CurrentNestingLevel;

    return ShouldContinue;
  }

  bool TraverseDoStmt(DoStmt *Node) {
    if (!Node)
      return Base::TraverseDoStmt(Node);

    bool ShouldContinue = TraverseStmt(Node->getCond());
    if (!ShouldContinue)
      return ShouldContinue;

    ++CurrentNestingLevel;
    ShouldContinue = TraverseStmt(Node->getBody());
    --CurrentNestingLevel;

    return ShouldContinue;
  }

// The currently-being-processed stack entry, which is always the top
#define CurrentBinaryOperator BinaryOperatorsStack.top()

#define TraverseBinOp(CLASS)                                                   \
  bool TraverseBin##CLASS(BinaryOperator *Op) {                                \
    if (!Op)                                                                   \
      return Base::TraverseBin##CLASS(Op);                                     \
                                                                               \
    /* Make sure that there is always at least one frame in the stack */       \
    if (BinaryOperatorsStack.empty())                                          \
      BinaryOperatorsStack.emplace();                                          \
                                                                               \
    /* If this is the first binary operator that we are processing, or the     \
     * previous binary operator was different, there is an increment */        \
    if (!CurrentBinaryOperator || Op->getOpcode() != CurrentBinaryOperator)    \
      CC.Account(Op->getOperatorLoc(), CurrentNestingLevel,                    \
                 CognitiveComplexity::Criteria::Increment);                    \
                                                                               \
    /* We might encounter function call, which starts a new sequence, thus     \
     * we need to save the current previous binary operator */                 \
    const Optional<BinaryOperator::Opcode> BinOpCopy(CurrentBinaryOperator);   \
                                                                               \
    /* Record the operator that we are currently processing and traverse it */ \
    CurrentBinaryOperator = Op->getOpcode();                                   \
    const bool ShouldContinue = Base::TraverseBin##CLASS(Op);                  \
    /* And restore the previous binary operator, which might be nonexistant */ \
    CurrentBinaryOperator = BinOpCopy;                                         \
                                                                               \
    return ShouldContinue;                                                     \
  }

  // In a sequence of binary logical operators, if new operator is different
  // from the previous-one, then the cognitive complexity is increased.
  TraverseBinOp(LAnd);
  TraverseBinOp(LOr);

#define TraverseCallExpr_(CLASS)                                                \
  bool Traverse##CLASS(CLASS *Call) {                                          \
    /* If we are not currently processing any binary operator sequence, then   \
     * no special-handling is needed */                                        \
    if (!Call || BinaryOperatorsStack.empty() || !CurrentBinaryOperator)       \
      return Base::Traverse##CLASS(Call);                                      \
                                                                               \
    /* Else, do add [uninitialized] frame to the stack, and traverse call */   \
    BinaryOperatorsStack.emplace();                                            \
    const bool ShouldContinue = Base::Traverse##CLASS(Call);                   \
    /* And remove the top frame */                                             \
    BinaryOperatorsStack.pop();                                                \
                                                                               \
    return ShouldContinue;                                                     \
  }

  // It would make sense for the function call to start the new binary operator
  // sequence, thus let's make sure that it creates new stack frame
  TraverseCallExpr_(CallExpr);
  TraverseCallExpr_(CXXMemberCallExpr);
  TraverseCallExpr_(CXXOperatorCallExpr);

  bool TraverseStmt(Stmt *Node) {
    if (!Node)
      return Base::TraverseStmt(Node);

    // three following switch()'es have huge duplication, but it is better to
    // keep them separate, to simplify comparing them with the Specification

    CognitiveComplexity::Criteria Reasons = CognitiveComplexity::Criteria::None;

    // B1. Increments
    // There is an increment for each of the following:
    switch (Node->getStmtClass()) {
    // if, else if, else  are handled in TraverseIfStmt()
    // FIXME: each method in a recursion cycle
    case Stmt::ConditionalOperatorClass:
    case Stmt::SwitchStmtClass:
    case Stmt::ForStmtClass:
    case Stmt::CXXForRangeStmtClass:
    case Stmt::WhileStmtClass:
    case Stmt::DoStmtClass:
    case Stmt::CXXCatchStmtClass:
    case Stmt::GotoStmtClass:
      Reasons |= CognitiveComplexity::Criteria::Increment;
      break;
    default:
      // break LABEL, continue LABEL increase cognitive complexity,
      // but they are not supported in C++ or C.
      // regular break/continue do not increase cognitive complexity
      break;
    }

    // B2. Nesting level
    // The following structures increment the nesting level:
    switch (Node->getStmtClass()) {
    // if, else if, else  are handled in TraverseIfStmt()
    // nested methods and such are handled in TraverseDecl
    case Stmt::ConditionalOperatorClass:
    case Stmt::SwitchStmtClass:
    case Stmt::ForStmtClass:
    case Stmt::CXXForRangeStmtClass:
    case Stmt::WhileStmtClass:
    case Stmt::DoStmtClass:
    case Stmt::CXXCatchStmtClass:
      Reasons |= CognitiveComplexity::Criteria::IncrementNesting;
      break;
    default:
      break;
    }

    // B3. Nesting increments
    // The following structures receive a nesting increment
    // commensurate with their nested depth inside B2 structures:
    switch (Node->getStmtClass()) {
    // if, else if, else  are handled in TraverseIfStmt()
    case Stmt::ConditionalOperatorClass:
    case Stmt::SwitchStmtClass:
    case Stmt::ForStmtClass:
    case Stmt::CXXForRangeStmtClass:
    case Stmt::WhileStmtClass:
    case Stmt::DoStmtClass:
    case Stmt::CXXCatchStmtClass:
      Reasons |= CognitiveComplexity::Criteria::PenalizeNesting;
      break;
    default:
      break;
    }

    // if we have found any reasons, let's account it.
    if (Reasons & CognitiveComplexity::Criteria::All)
      CC.Account(Node->getLocStart(), CurrentNestingLevel, Reasons);

    return Base::TraverseStmt(Node);
  }

  bool TraverseDecl(Decl *Node, bool MainAnalyzedFunction = false) {
    if (!Node || MainAnalyzedFunction)
      return Base::TraverseDecl(Node);

    // B2. Nesting level
    // The following structures increment the nesting level:
    switch (Node->getKind()) {
    // lambda is handled in TraverseLambdaExpr().
    // can't have nested functions, so not accounted for
    case Decl::CXXConstructor:
    case Decl::CXXDestructor:
    case Decl::CXXMethod:
      break;
    default:
      return Base::TraverseDecl(Node);
      break;
    }

    CC.Account(Node->getLocStart(), CurrentNestingLevel,
               CognitiveComplexity::Criteria::IncrementNesting);

    ++CurrentNestingLevel;
    const bool ShouldContinue = Base::TraverseDecl(Node);
    --CurrentNestingLevel;

    return ShouldContinue;
  }

  bool TraverseLambdaExpr(LambdaExpr *Node) {
    if (!Node)
      return Base::TraverseLambdaExpr(Node);

    CC.Account(Node->getLocStart(), CurrentNestingLevel,
               CognitiveComplexity::Criteria::IncrementNesting);

    ++CurrentNestingLevel;
    const bool ShouldContinue = Base::TraverseLambdaExpr(Node);
    --CurrentNestingLevel;

    return ShouldContinue;
  }

  CognitiveComplexity CC;
};
} // namespace

FunctionCognitiveComplexityCheck::FunctionCognitiveComplexityCheck(
    StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context),
      Threshold(Options.get("Threshold", CognitiveComplexity::DefaultLimit)) {}

void FunctionCognitiveComplexityCheck::storeOptions(
    ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "Threshold", Threshold);
}

void FunctionCognitiveComplexityCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      functionDecl(unless(anyOf(isInstantiated(), isImplicit()))).bind("func"),
      this);
}

void FunctionCognitiveComplexityCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *Func = Result.Nodes.getNodeAs<FunctionDecl>("func");

  FunctionASTVisitor Visitor;
  Visitor.TraverseDecl(const_cast<FunctionDecl *>(Func), true);

  if (Visitor.CC.Total <= Threshold)
    return;

  diag(Func->getLocation(),
       "function %0 has cognitive complexity of %1 (threshold %2)")
      << Func << Visitor.CC.Total << Threshold;

  for (const auto &Detail : Visitor.CC.Details) {
    const std::pair<const char *, unsigned short> Reasoning(Detail.Process());
    diag(Detail.Loc, Reasoning.first, DiagnosticIDs::Note)
        << Reasoning.second << Detail.Nesting << 1 + Detail.Nesting;
  }
}

} // namespace readability
} // namespace tidy
} // namespace clang
