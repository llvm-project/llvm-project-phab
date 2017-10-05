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
  // Yes, this member variable would be better off being static. But then
  // either StringRef does not have constexpr constructor (because strlen is not
  // constexpr), which is needed for static constexpr variable; or if char* is
  // used, there is a linking problem (Msgs is missing).
  // So either static out-of-line or non-static in-line.
  const std::array<const StringRef, 4> Msgs = {{
      // FIXME: these messages somehow trigger an assertion:
      // Fix conflicts with existing fix! The new replacement overlaps with an
      // existing replacement.
      // New replacement: /build/llvm-build-Clang-release/tools/clang/tools/extra/test/clang-tidy/Output/readability-function-cognitive-complexity.cpp.tmp.cpp: 3484:+5:""
      // Existing replacement: /build/llvm-build-Clang-release/tools/clang/tools/extra/test/clang-tidy/Output/readability-function-cognitive-complexity.cpp.tmp.cpp: 3485:+2:"&"
      // clang-tidy: /build/clang-tools-extra/clang-tidy/ClangTidyDiagnosticConsumer.cpp:86: virtual void (anonymous namespace)::ClangTidyDiagnosticRenderer::emitCodeContext(clang::FullSourceLoc, DiagnosticsEngine::Level, SmallVectorImpl<clang::CharSourceRange> &, ArrayRef<clang::FixItHint>): Assertion `false && "Fix conflicts with existing fix!"' failed.

      /// B1 + B2 + B3
      "+%0, including nesting penalty of %1, nesting level increased to %2",

      /// B1 + B2
      "+%0, nesting level increased to %2",

      /// B1
      "+%0",

      /// B2
      "nesting level increased to %2",
  }};

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
    std::pair<unsigned, unsigned short> process() const {
      assert(C != Criteria::None && "invalid criteria");

      unsigned MsgId;           /// The id of the message to output
      unsigned short Increment; /// How much of an increment?

      if (C == Criteria::All) {
        Increment = 1 + Nesting;
        MsgId = 0;
      } else if (C == (Criteria::Increment | Criteria::IncrementNesting)) {
        Increment = 1;
        MsgId = 1;
      } else if (C == Criteria::Increment) {
        Increment = 1;
        MsgId = 2;
      } else if (C == Criteria::IncrementNesting) {
        Increment = 0; /// Unused in this message.
        MsgId = 3;
      } else
        llvm_unreachable("should not get to here.");

      return std::make_pair(MsgId, Increment);
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

  unsigned MsgId;
  unsigned short Increase;
  std::tie(MsgId, Increase) = D.process();

  Total += Increase;
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
#define TraverseWithIncreasedNestingLevel(CLASS, Node)                         \
  ++CurrentNestingLevel;                                                       \
  ShouldContinue = Base::Traverse##CLASS(Node);                                \
  --CurrentNestingLevel;

  bool TraverseStmtWithIncreasedNestingLevel(Stmt *Node) {
    bool ShouldContinue;
    TraverseWithIncreasedNestingLevel(Stmt, Node);
    return ShouldContinue;
  }

  bool TraverseDeclWithIncreasedNestingLevel(Decl *Node) {
    bool ShouldContinue;
    TraverseWithIncreasedNestingLevel(Decl, Node);
    return ShouldContinue;
  }

#undef TraverseWithIncreasedNestingLevel

  bool TraverseIfStmt(IfStmt *Node, bool InElseIf = false) {
    if (!Node)
      return Base::TraverseIfStmt(Node);

    {
      CognitiveComplexity::Criteria Reasons;

      Reasons = CognitiveComplexity::Criteria::None;

      /// "If" increases cognitive complexity
      Reasons |= CognitiveComplexity::Criteria::Increment;
      /// "If" increases nesting level
      Reasons |= CognitiveComplexity::Criteria::IncrementNesting;

      if (!InElseIf) {
        /// "If" receives a nesting increment commensurate with it's nested
        /// depth, if it is not part of "else if"
        Reasons |= CognitiveComplexity::Criteria::PenalizeNesting;
      }

      CC.account(Node->getIfLoc(), CurrentNestingLevel, Reasons);
    }

    /// If this IfStmt is *NOT* "else if", then only the body (i.e. "Then" and
    /// "Else" are traversed with increased Nesting level.
    /// However if this IfStmt *IS* "else if", then Nesting level is increased
    /// for the whole IfStmt (i.e. for "Init", "Cond", "Then" and "Else")
    if (!InElseIf) {
      if (!TraverseStmt(Node->getInit()))
        return false;

      if (!TraverseStmt(Node->getCond()))
        return false;
    } else {
      if (!TraverseStmtWithIncreasedNestingLevel(Node->getInit()))
        return false;

      if (!TraverseStmtWithIncreasedNestingLevel(Node->getCond()))
        return false;
    }

    /// "Then" always increases nesting level.
    if (!TraverseStmtWithIncreasedNestingLevel(Node->getThen()))
      return false;

    if (!Node->getElse())
      return true;

    if (isa<IfStmt>(Node->getElse()))
      return TraverseIfStmt(dyn_cast<IfStmt>(Node->getElse()), true);

    {
      CognitiveComplexity::Criteria Reasons;

      Reasons = CognitiveComplexity::Criteria::None;

      /// "Else" increases cognitive complexity
      Reasons |= CognitiveComplexity::Criteria::Increment;
      /// "Else" increases nesting level
      Reasons |= CognitiveComplexity::Criteria::IncrementNesting;
      /// "Else" DOES NOT receive a nesting increment commensurate with it's
      /// nested depth

      CC.account(Node->getElseLoc(), CurrentNestingLevel, Reasons);
    }

    /// "Else" always increases nesting level.
    return TraverseStmtWithIncreasedNestingLevel(Node->getElse());
  }

/// The currently-being-processed stack entry, which is always the top
#define CurrentBinaryOperator BinaryOperatorsStack.top()

  // FIXME: how to define  bool TraverseBinaryOperator(BinaryOperator *Op)  ?

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

  /// It would make sense for the function call to start the new binary
  /// operator sequence, thus let's make sure that it creates new stack frame
  bool TraverseCallExpr(CallExpr *Node) {
    /// If we are not currently processing any binary operator sequence, then
    /// no Node-handling is needed
    if (!Node || BinaryOperatorsStack.empty() || !CurrentBinaryOperator)
      return Base::TraverseCallExpr(Node);

    /// Else, do add [uninitialized] frame to the stack, and traverse call
    BinaryOperatorsStack.emplace();
    const bool ShouldContinue = Base::TraverseCallExpr(Node);
    /// And remove the top frame
    BinaryOperatorsStack.pop();

    return ShouldContinue;
  }

#undef CurrentBinaryOperator

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
    case Stmt::LambdaExprClass:
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

    /// Did we decide that the nesting level should be increased?
    if (!(Reasons & CognitiveComplexity::Criteria::IncrementNesting))
      return Base::TraverseStmt(Node);

    return TraverseStmtWithIncreasedNestingLevel(Node);
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
    case Decl::Function:
    case Decl::CXXMethod:
    case Decl::CXXConstructor:
    case Decl::CXXDestructor:
      break;
    default:
      /// If this is something else, we use early return!
      return Base::TraverseDecl(Node);
      break;
    }

    CC.account(Node->getLocStart(), CurrentNestingLevel,
               CognitiveComplexity::Criteria::IncrementNesting);

    return TraverseDeclWithIncreasedNestingLevel(Node);
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

  // Now, output all the basic increments, of which this Cognitive Complexity
  // consists of
  for (const auto &Detail : Visitor.CC.Details) {
    unsigned MsgId;          /// The id of the message to output
    unsigned short Increase; /// How much of an increment?
    std::tie(MsgId, Increase) = Detail.process();
    assert(MsgId < Visitor.CC.Msgs.size() && "MsgId should always be valid");
    // Increase on the other hand, can be 0.

    diag(Detail.Loc, Visitor.CC.Msgs[MsgId], DiagnosticIDs::Note)
        << Increase << Detail.Nesting << 1 + Detail.Nesting;
  }
}

} // namespace readability
} // namespace tidy
} // namespace clang
