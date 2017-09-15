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
  /// Limit of 25 is the "upstream"'s default.
  static constexpr unsigned DefaultLimit = 25U;

  /// Any increment is based on some combination of reasons.
  /// For details you can look at the Specification at
  /// https://www.sonarsource.com/docs/CognitiveComplexity.pdf
  /// or user-facing docs at
  /// http://clang.llvm.org/extra/clang-tidy/checks/readability-function-cognitive-complexity.html
  /// Here are all the possible reasons:
  enum Criteria : unsigned char {
    None = 0U,

    /// B1, increases cognitive complexity (by 1)
    /// What causes it:
    /// * if, else if, else, ConditionalOperator (not BinaryConditionalOperator)
    /// * SwitchStmt
    /// * ForStmt, CXXForRangeStmt
    /// * WhileStmt, DoStmt
    /// * CXXCatchStmt
    /// * GotoStmt (but not BreakStmt, ContinueStmt)
    /// * sequences of binary logical operators (BinOpLAnd, BinOpLOr)
    /// * each method in a recursion cycle (not implemented)
    Increment = 1U << 0,

    /// B2, increases current nesting level (by 1)
    /// What causes it:
    /// * if, else if, else, ConditionalOperator (not BinaryConditionalOperator)
    /// * SwitchStmt
    /// * ForStmt, CXXForRangeStmt
    /// * WhileStmt, DoStmt
    /// * CXXCatchStmt
    /// * nested CXXConstructor, CXXDestructor, CXXMethod (incl. C++11 Lambda)
    IncrementNesting = 1U << 1,

    /// B3, increases cognitive complexity by the current nesting level
    /// Applied before IncrementNesting
    /// What causes it:
    /// * IfStmt, ConditionalOperator (not BinaryConditionalOperator)
    /// * SwitchStmt
    /// * ForStmt, CXXForRangeStmt
    /// * WhileStmt, DoStmt
    /// * CXXCatchStmt
    PenalizeNesting = 1U << 2,

    All = Increment | PenalizeNesting | IncrementNesting,
  };

  /// All the possible messages that can be outputed. The choice of the message
  /// to use is based of the combination of the Criterias
  static const std::array<const char *const, 4> Msgs;

  /// The helper struct used to record one increment occurence, with all the
  /// details nessesary
  struct Detail final {
    const SourceLocation Loc;     /// What caused the increment?
    const unsigned short Nesting; /// How deeply nested is Loc located?
    const Criteria C : 3;         /// The criteria of the increment

    /// Criteria C is a bitfield. Even though Criteria is an unsigned char; and
    /// only using 3 bits will still result in padding, the fact that it is a
    /// bitfield is a reminder that it is important to min(sizeof(Detail))

    Detail(SourceLocation SLoc, unsigned short CurrentNesting, Criteria Crit)
        : Loc(SLoc), Nesting(CurrentNesting), C(Crit) {}

    /// To minimize the sizeof(Detail), we only store the minimal info there.
    /// This function is used to convert from the stored info into the usable
    /// information - what message to output, how much of an increment did this
    /// occurence actually result in
    std::pair<const char *, unsigned short> process() const {
      assert(C != Criteria::None && "invalid criteria");

      const char *Msg;          /// The message to output
      unsigned short Increment; /// How much of an increment?

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
        Increment = 0; /// Unused in this message.
        Msg = Msgs[3];
      } else
        llvm_unreachable("should not get to here.");

      return std::make_pair(Msg, Increment);
    }
  };
  // static_assert(sizeof(Detail) <= 8, "it's best to keep the size minimal");

  /// Based on the publicly-avaliable numbers for some big open-source projects
  /// https://sonarcloud.io/projects?languages=c%2Ccpp&size=5   we can estimate:
  /// value ~20 would result in no allocs for 98% of functions, ~12 for 96%, ~10
  /// for 91%, ~8 for 88%, ~6 for 84%, ~4 for 77%, ~2 for 64%, and ~1 for 37%
  SmallVector<Detail, DefaultLimit> Details; // 25 elements is 200 bytes.
  /// Yes, 25 is a magic number. This is the seemingly-sane default for the
  /// upper limit for function cognitive complexity. Thus it would make sense
  /// to avoid allocations for any function that does not violate the limit.

  /// The grand total Cognitive Complexity of the function
  unsigned Total = 0;

  /// The function used to store new increment, calculate the total complexity
  void account(SourceLocation Loc, unsigned short Nesting, Criteria C);
};

/// Refer to the enum Criteria for detailed explaination.
const std::array<const char *const, 4> CognitiveComplexity::Msgs = {{
    /// B1 + B2 + B3
    "+%0, including nesting penalty of %1, nesting level increased to %2",

    /// B1 + B2
    "+%0, nesting level increased to %2",

    /// B1
    "+%0",

    /// B2
    "nesting level increased to %2",
}};

/// Criteria is a bitset, thus a few helpers are needed
CognitiveComplexity::Criteria operator|(CognitiveComplexity::Criteria LHS,
                                        CognitiveComplexity::Criteria RHS) {
  return static_cast<CognitiveComplexity::Criteria>(
      static_cast<std::underlying_type<CognitiveComplexity::Criteria>::type>(
          LHS) |
      static_cast<std::underlying_type<CognitiveComplexity::Criteria>::type>(
          RHS));
}
CognitiveComplexity::Criteria operator&(CognitiveComplexity::Criteria LHS,
                                        CognitiveComplexity::Criteria RHS) {
  return static_cast<CognitiveComplexity::Criteria>(
      static_cast<std::underlying_type<CognitiveComplexity::Criteria>::type>(
          LHS) &
      static_cast<std::underlying_type<CognitiveComplexity::Criteria>::type>(
          RHS));
}
CognitiveComplexity::Criteria &operator|=(CognitiveComplexity::Criteria &LHS,
                                          CognitiveComplexity::Criteria RHS) {
  LHS = operator|(LHS, RHS);
  return LHS;
}
CognitiveComplexity::Criteria &operator&=(CognitiveComplexity::Criteria &LHS,
                                          CognitiveComplexity::Criteria RHS) {
  LHS = operator&(LHS, RHS);
  return LHS;
}

void CognitiveComplexity::account(SourceLocation Loc, unsigned short Nesting,
                                  Criteria C) {
  C &= Criteria::All;
  assert(C != Criteria::None && "invalid criteria");

  Details.emplace_back(Loc, Nesting, C);
  const Detail &D(Details.back());

  const std::pair<const char *, unsigned short> Reasoning(D.process());
  Total += Reasoning.second;
}

class FunctionASTVisitor final
    : public RecursiveASTVisitor<FunctionASTVisitor> {
  using Base = RecursiveASTVisitor<FunctionASTVisitor>;

  /// The current nesting level (increased by Criteria::IncrementNesting)
  unsigned short CurrentNestingLevel = 0;

  /// Used to efficiently know the last type of binary sequence operator that
  /// was encountered. It would make sense for the function call to start the
  /// new sequence, thus it is a stack.
  std::stack</*********/ Optional<BinaryOperator::Opcode>,
             SmallVector<Optional<BinaryOperator::Opcode>, 4>>
      BinaryOperatorsStack;

public:
  /// Important piece of the information: the Nesting level is increased only
  /// for the body of whatever statement, not it's init/cond/inc, thus the
  /// common repetitive code is factored here.
#define TraverseWithIncreasedNestingLevel(CLASS, Node)                         \
  ++CurrentNestingLevel;                                                       \
  ShouldContinue = Base::Traverse##CLASS(Node);                                \
  --CurrentNestingLevel;

#define IncreasesNestingLevel(CLASS)                                           \
  bool Traverse##CLASS(CLASS *Node) {                                          \
    if (!Node)                                                                 \
      return Base::Traverse##CLASS(Node);                                      \
                                                                               \
    bool ShouldContinue;                                                       \
    TraverseWithIncreasedNestingLevel(CLASS, Node);                            \
                                                                               \
    return ShouldContinue;                                                     \
  }

  /// B2. Nesting level
  /// The following structures increment the nesting level:
  IncreasesNestingLevel(DefaultStmt);
  IncreasesNestingLevel(CXXCatchStmt);
  /// Other instances are more complex, and need standalone functions

#undef IncreasesNestingLevel

  bool TraverseIfStmt(IfStmt *Node, bool InElseIf = false) {
    if (!Node)
      return Base::TraverseIfStmt(Node);

    {
      CognitiveComplexity::Criteria Reasons =
          CognitiveComplexity::Criteria::None;

      /// if   increases cognitive complexity
      Reasons |= CognitiveComplexity::Criteria::Increment;
      /// if   increases nesting level
      Reasons |= CognitiveComplexity::Criteria::IncrementNesting;

      /// if   receives a nesting increment commensurate with it's nested depth
      /// but only if it is not part of  else if
      if (!InElseIf)
        Reasons |= CognitiveComplexity::Criteria::PenalizeNesting;

      CC.account(Node->getIfLoc(), CurrentNestingLevel, Reasons);
    }

    bool ShouldContinue = TraverseStmt(Node->getInit());
    if (!ShouldContinue)
      return ShouldContinue;

    ShouldContinue = TraverseStmt(Node->getCond());
    if (!ShouldContinue)
      return ShouldContinue;

    /// if   increases nesting level
    TraverseWithIncreasedNestingLevel(Stmt, Node->getThen());

    if (!ShouldContinue || !Node->getElse())
      return ShouldContinue;

    /// else  OR  else if   increases cognitive complexity and nesting level
    /// but  else if  does NOT result in double increase !
    if (isa<IfStmt>(Node->getElse()))
      return TraverseIfStmt(dyn_cast<IfStmt>(Node->getElse()), true);

    {
      CognitiveComplexity::Criteria Reasons =
          CognitiveComplexity::Criteria::None;

      /// else   increases cognitive complexity
      Reasons |= CognitiveComplexity::Criteria::Increment;
      /// else   increases nesting level
      Reasons |= CognitiveComplexity::Criteria::IncrementNesting;
      /// else   DOES NOT receive a nesting increment commensurate with it's
      /// nested depth

      CC.account(Node->getElseLoc(), CurrentNestingLevel, Reasons);
    }

    TraverseWithIncreasedNestingLevel(Stmt, Node->getElse());

    return ShouldContinue;
  }

  bool TraverseConditionalOperator(ConditionalOperator *Node) {
    if (!Node)
      return Base::TraverseConditionalOperator(Node);

    bool ShouldContinue = TraverseStmt(Node->getCond());
    if (!ShouldContinue)
      return ShouldContinue;

    TraverseWithIncreasedNestingLevel(Stmt, Node->getLHS());

    if (!ShouldContinue)
      return ShouldContinue;

    TraverseWithIncreasedNestingLevel(Stmt, Node->getRHS());

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

    TraverseWithIncreasedNestingLevel(Stmt, Node->getSubStmt());

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

    TraverseWithIncreasedNestingLevel(Stmt, Node->getBody());

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

    TraverseWithIncreasedNestingLevel(Stmt, Node->getBody());

    return ShouldContinue;
  }

  bool TraverseWhileStmt(WhileStmt *Node) {
    if (!Node)
      return Base::TraverseWhileStmt(Node);

    bool ShouldContinue = TraverseStmt(Node->getCond());
    if (!ShouldContinue)
      return ShouldContinue;

    TraverseWithIncreasedNestingLevel(Stmt, Node->getBody());

    return ShouldContinue;
  }

  bool TraverseDoStmt(DoStmt *Node) {
    if (!Node)
      return Base::TraverseDoStmt(Node);

    bool ShouldContinue = TraverseStmt(Node->getCond());
    if (!ShouldContinue)
      return ShouldContinue;

    TraverseWithIncreasedNestingLevel(Stmt, Node->getBody());

    return ShouldContinue;
  }

/// The currently-being-processed stack entry, which is always the top
#define CurrentBinaryOperator BinaryOperatorsStack.top()

#define TraverseBinOp(CLASS)                                                   \
  bool TraverseBin##CLASS(BinaryOperator *Op) {                                \
    if (!Op)                                                                   \
      return Base::TraverseBin##CLASS(Op);                                     \
                                                                               \
    /** Make sure that there is always at least one frame in the stack */      \
    if (BinaryOperatorsStack.empty())                                          \
      BinaryOperatorsStack.emplace();                                          \
                                                                               \
    /** If this is the first binary operator that we are processing, or the    \
     ** previous binary operator was different, there is an increment */       \
    if (!CurrentBinaryOperator || Op->getOpcode() != CurrentBinaryOperator)    \
      CC.account(Op->getOperatorLoc(), CurrentNestingLevel,                    \
                 CognitiveComplexity::Criteria::Increment);                    \
                                                                               \
    /** We might encounter function call, which starts a new sequence, thus    \
     ** we need to save the current previous binary operator */                \
    const Optional<BinaryOperator::Opcode> BinOpCopy(CurrentBinaryOperator);   \
                                                                               \
    /** Record the operator that we are currently processing and traverse it   \
     */                                                                        \
    CurrentBinaryOperator = Op->getOpcode();                                   \
    const bool ShouldContinue = Base::TraverseBin##CLASS(Op);                  \
    /** And restore the previous binary operator, which might be nonexistant   \
     */                                                                        \
    CurrentBinaryOperator = BinOpCopy;                                         \
                                                                               \
    return ShouldContinue;                                                     \
  }

  /// In a sequence of binary logical operators, if new operator is different
  /// from the previous-one, then the cognitive complexity is increased.
  TraverseBinOp(LAnd);
  TraverseBinOp(LOr);

#define TraverseCallExpr_(CLASS)                                               \
  bool Traverse##CLASS(CLASS *Call) {                                          \
    /** If we are not currently processing any binary operator sequence, then  \
     ** no special-handling is needed */                                       \
    if (!Call || BinaryOperatorsStack.empty() || !CurrentBinaryOperator)       \
      return Base::Traverse##CLASS(Call);                                      \
                                                                               \
    /** Else, do add [uninitialized] frame to the stack, and traverse call */  \
    BinaryOperatorsStack.emplace();                                            \
    const bool ShouldContinue = Base::Traverse##CLASS(Call);                   \
    /** And remove the top frame */                                            \
    BinaryOperatorsStack.pop();                                                \
                                                                               \
    return ShouldContinue;                                                     \
  }

  /// It would make sense for the function call to start the new binary
  /// operator sequence, thus let's make sure that it creates new stack frame
  TraverseCallExpr_(CallExpr);
  TraverseCallExpr_(CXXMemberCallExpr);
  TraverseCallExpr_(CXXOperatorCallExpr);

  bool TraverseStmt(Stmt *Node) {
    if (!Node)
      return Base::TraverseStmt(Node);

    /// Three following switch()'es have huge duplication, but it is better to
    /// keep them separate, to simplify comparing them with the Specification

    CognitiveComplexity::Criteria Reasons = CognitiveComplexity::Criteria::None;
    SourceLocation Location = Node->getLocStart();

    /// B1. Increments
    /// There is an increment for each of the following:
    switch (Node->getStmtClass()) {
    /// if, else if, else  are handled in TraverseIfStmt()
    /// FIXME: each method in a recursion cycle
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
      /// break LABEL, continue LABEL increase cognitive complexity,
      /// but they are not supported in C++ or C.
      /// regular break/continue do not increase cognitive complexity
      break;
    }

    /// B2. Nesting level
    /// The following structures increment the nesting level:
    switch (Node->getStmtClass()) {
    /// if, else if, else  are handled in TraverseIfStmt()
    /// nested methods and such are handled in TraverseDecl
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

    /// B3. Nesting increments
    /// The following structures receive a nesting increment
    /// commensurate with their nested depth inside B2 structures:
    switch (Node->getStmtClass()) {
    /// if, else if, else  are handled in TraverseIfStmt()
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

    if (Node->getStmtClass() == Stmt::ConditionalOperatorClass) {
      /// A little beautification.
      /// For conditional operator "cond ? true : false" point at the "?"
      /// symbol.
      ConditionalOperator *COp = dyn_cast<ConditionalOperator>(Node);
      Location = COp->getQuestionLoc();
    }

    /// if we have found any reasons, let's account it.
    if (Reasons & CognitiveComplexity::Criteria::All)
      CC.account(Location, CurrentNestingLevel, Reasons);

    return Base::TraverseStmt(Node);
  }

  /// The second non-standard parameter MainAnalyzedFunction is needed to
  /// differentiate between the cases where TraverseDecl() is the entry point
  /// from FunctionCognitiveComplexityCheck::check() and the cases where it was
  /// called from the FunctionASTVisitor itself.
  /// Explaination: if we get a function definition (e.g. constructor,
  /// destructor, method), the Cognitive Complexity specification states that
  /// the Nesting level shall be increased. But if this function is the entry
  /// point, then the Nesting level should not be increased. Thus that parameter
  /// is there and is used to fall-through directly to traversing if this is
  /// the main function that is being analyzed.
  bool TraverseDecl(Decl *Node, bool MainAnalyzedFunction = false) {
    if (!Node || MainAnalyzedFunction)
      return Base::TraverseDecl(Node);

    /// B2. Nesting level
    /// The following structures increment the nesting level:
    switch (Node->getKind()) {
    /// lambda is handled in TraverseLambdaExpr().
    /// can't have nested functions, so not accounted for
    case Decl::CXXConstructor:
    case Decl::CXXDestructor:
    case Decl::CXXMethod:
      break;
    default:
      return Base::TraverseDecl(Node);
      break;
    }

    CC.account(Node->getLocStart(), CurrentNestingLevel,
               CognitiveComplexity::Criteria::IncrementNesting);

    bool ShouldContinue;
    TraverseWithIncreasedNestingLevel(Decl, Node);

    return ShouldContinue;
  }

  bool TraverseLambdaExpr(LambdaExpr *Node) {
    if (!Node)
      return Base::TraverseLambdaExpr(Node);

    CC.account(Node->getLocStart(), CurrentNestingLevel,
               CognitiveComplexity::Criteria::IncrementNesting);

    bool ShouldContinue;
    TraverseWithIncreasedNestingLevel(LambdaExpr, Node);

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
    const char *IncreaseMessage = nullptr;
    unsigned short Increase = 0;
    std::tie(IncreaseMessage, Increase) = Detail.process();
    assert(IncreaseMessage && "the code should always provide a msg");
    /// Increase, on the other hand, can be 0, if only nesting is increased.

    diag(Detail.Loc, IncreaseMessage, DiagnosticIDs::Note)
        << Increase << Detail.Nesting << 1 + Detail.Nesting;
  }
}

} // namespace readability
} // namespace tidy
} // namespace clang
