//===--- AssertionCountCheck.cpp - clang-tidy------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "AssertionCountCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {
namespace {

// checks whether second SourceRange is fully within the first SourceRange
// this does check both the line numbers and the columns
// FIXME: this function appears to be rather convoluted...
bool SourceRangeContains(const SourceManager &SM, const SourceRange &outer,
                         const SourceRange &inner) {
  assert(SM.isWrittenInSameFile(outer.getBegin(), outer.getEnd()) &&
         SM.isWrittenInSameFile(inner.getBegin(), inner.getEnd()) &&
         SM.isWrittenInSameFile(outer.getBegin(), inner.getEnd()) &&
         "don't know how this could work for more than one file");

  assert((SM.getSpellingLineNumber(outer.getBegin()) <=
          SM.getSpellingLineNumber(outer.getEnd())) &&
         (SM.getSpellingLineNumber(inner.getBegin()) <=
          SM.getSpellingLineNumber(inner.getEnd())) &&
         "why are the ranges all messed up?");

  // the thought is that if the "inner" block occupies subset of the lines of
  // "outer" block, then the inner block is indeed lies within the outer block.

  // handle lines first. much easier

  // if the "inner" block starts first, then it is not inner.
  if (SM.getSpellingLineNumber(outer.getBegin()) >
      SM.getSpellingLineNumber(inner.getBegin()))
    return false;

  // if the "outer" block ends first, then it is not outer.
  if (SM.getSpellingLineNumber(outer.getEnd()) <
      SM.getSpellingLineNumber(inner.getEnd()))
    return false;

  // at this point we know that the outer block begins not after the inner ends
  // and that the outer block ends not before the inner block stars
  assert((SM.getSpellingLineNumber(outer.getBegin()) <=
          SM.getSpellingLineNumber(inner.getEnd())) &&
         (SM.getSpellingLineNumber(outer.getEnd()) >=
          SM.getSpellingLineNumber(inner.getBegin())));

  // okay, as far as linenumbers are concerned, blocks are ok.

  // if the blocks are properly multi-line then we are all set
  // i.e. outer and inner blocks start on different lines and
  //      outer and inner blocks end   on different lines
  if ((SM.getSpellingLineNumber(outer.getBegin()) <
       SM.getSpellingLineNumber(inner.getBegin())) &&
      (SM.getSpellingLineNumber(outer.getEnd()) >
       SM.getSpellingLineNumber(inner.getEnd()))) {
    // and the outer block starts on different line than inner ends
    assert(SM.getSpellingLineNumber(outer.getBegin()) <
           SM.getSpellingLineNumber(inner.getEnd()));
    return true;
  }

  // blocks share line[s] where they start and/or end
  // will have to check columns

  assert((SM.getSpellingLineNumber(outer.getBegin()) ==
          SM.getSpellingLineNumber(inner.getBegin())) ||
         (SM.getSpellingLineNumber(outer.getEnd()) ==
          SM.getSpellingLineNumber(inner.getEnd())));

  // if both the blocks begin at the same line, check that "outer" block
  // is started earlier in the line
  if ((SM.getSpellingLineNumber(outer.getBegin()) ==
       SM.getSpellingLineNumber(inner.getBegin())) &&
      (SM.getSpellingColumnNumber(outer.getBegin()) >
       SM.getSpellingColumnNumber(inner.getBegin())))
    return false;

  // if both the blocks end at the same line, check that "outer" block
  // ends later in the line
  if ((SM.getSpellingLineNumber(outer.getEnd()) ==
       SM.getSpellingLineNumber(inner.getEnd())) &&
      (SM.getSpellingColumnNumber(outer.getEnd()) <
       SM.getSpellingColumnNumber(inner.getEnd())))
    return false;

  // looks ok.
  return true;
}

// post-processes the results of the first part, AssertionMacrosCallbacks.
unsigned CountMacroAssertions(const std::vector<SourceRange> &FoundAssertions,
                              const SourceManager *SM, const Stmt *Body) {
  unsigned Count = 0;

  // let's count how many of the found macro instantiations lie in this function
  for (const auto &MacroExpansion : FoundAssertions) {
    if (!SM->isWrittenInSameFile(MacroExpansion.getBegin(),
                                 MacroExpansion.getEnd()))
      continue;

    if (!SM->isWrittenInSameFile(Body->getLocStart(),
                                 MacroExpansion.getBegin()))
      continue;

    // is this assertion in this function?
    if (!SourceRangeContains(*SM, Body->getSourceRange(), MacroExpansion))
      continue;

    // got valid assertion
    ++Count;
  }

  return Count;
}

// part One of the assertion detection.
// a callback that will be triggered each time a macro is expanded,
// and it will record the expansion position (range). it is important to record
// the positions (as in, not just the count), because at this stage, it is
// impossible to know in which function declaration the expansion happens.
class AssertionMacrosCallbacks : public PPCallbacks {
  const SmallVector<StringRef, 5> &AssertNames;
  std::vector<SourceRange> *FoundAssertions;

public:
  AssertionMacrosCallbacks(const SmallVector<StringRef, 5> &Names,
                           std::vector<SourceRange> *Found)
      : AssertNames(Names), FoundAssertions(Found) {
    assert(FoundAssertions);
  }

  void MacroExpands(const Token &MacroNameTok, const MacroDefinition &MD,
                    SourceRange Range, const MacroArgs *Args) override {
    const auto *II = MacroNameTok.getIdentifierInfo();

    if (!II)
      return;

    // if given macro [expansion] matches one of the configured assert names,
    // record this position. the check itself will later sieve all the matches,
    // and find (count) the ones in the function in question.
    if (std::find(AssertNames.begin(), AssertNames.end(), II->getName()) !=
        AssertNames.end())
      FoundAssertions->emplace_back(Range);
  }
};

struct FunctionInfo {
  unsigned Lines = 0;
  unsigned Assertions = 0;
};

// part Two of the assertion detection.
// after the results of the first part (AssertionMacrosCallbacks) is processed
// (via CountMacroAssertions), this can be called to walk through the whole AST
// of a given function declaration, and count function-like assertions.
class FunctionASTVisitor : public RecursiveASTVisitor<FunctionASTVisitor> {
  using Base = RecursiveASTVisitor<FunctionASTVisitor>;

  const SmallVector<StringRef, 5> &AssertNames;
  const std::vector<SourceRange> &FoundAssertions;
  const SourceManager *SM = nullptr;
  const LangOptions LangOpts;
  FunctionInfo *FI = nullptr;

  void HandleStmt(const Stmt *Node) {
    // let's check if this found interesting node was *NOT*
    // introduced as expansion of one of the macros that we care about.

    SourceLocation Loc = Node->getLocStart();
    while (Loc.isValid() && Loc.isMacroID()) {
      StringRef MacroName = Lexer::getImmediateMacroName(Loc, *SM, LangOpts);

      // Check if this macro is an assert that we care about.
      if (std::find(AssertNames.begin(), AssertNames.end(), MacroName) !=
          AssertNames.end()) {
        return;
      }
      Loc = SM->getImmediateMacroCallerLoc(Loc);
    }

    // this call was not expanded from one of the 'cared' macros, count it.
    ++FI->Assertions;
  }

  void HandleFunctionCall(Stmt *S, const CallExpr *CExpr) {
    const auto *FuncDecl = CExpr->getDirectCallee();

    if (!FuncDecl)
      return;

    if (!FuncDecl->getDeclName().isIdentifier())
      return;

    if (std::find(AssertNames.begin(), AssertNames.end(),
                  FuncDecl->getName()) == AssertNames.end())
      return;

    // cool. found a call to a function with one of the configured names
    HandleStmt(S);
  }

  void HandleLambdaCall(Stmt *S, const CXXOperatorCallExpr *OCall) {
    // FIXME
  }

public:
  FunctionASTVisitor(const SmallVector<StringRef, 5> &Names,
                     const std::vector<SourceRange> &MacroAssertions,
                     ASTContext *Context, FunctionInfo *Info)
      : AssertNames(Names), FoundAssertions(MacroAssertions),
        SM(&(Context->getSourceManager())), LangOpts(Context->getLangOpts()),
        FI(Info) {}

  // do not traverse into any declarations.
  bool TraverseDeclStmt(DeclStmt *DS) {
    if (!(DS->isSingleDecl() && isa<StaticAssertDecl>(DS->getSingleDecl()))) {
      // all the assertions located within declarations do NOT contribute
      // to the func! so subtract them.

      FI->Assertions -= CountMacroAssertions(FoundAssertions, SM, DS);

      // do not traverse into any declarations.
      return true;
    }

    // found a static_assert().
    HandleStmt(DS);

    return true;
  }

  bool VisitStmt(Stmt *S) {
    if (const auto *OCall = dyn_cast<CXXOperatorCallExpr>(S))
      HandleLambdaCall(S, OCall);
    else if (const auto *CExpr = dyn_cast<CallExpr>(S))
      HandleFunctionCall(S, CExpr);

    return true;
  }
};

} // anonymous namespace

AssertionCountCheck::AssertionCountCheck(StringRef Name,
                                         ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context),
      LineThreshold(Options.get("LineThreshold", -1U)),
      AssertsThreshold(Options.get("AssertsThreshold", 0U)),
      LinesStep(Options.get("LinesStep", 0U)),
      AssertsStep(Options.get("AssertsStep", 0U)),
      RawAssertList(Options.get("AssertNames", "assert")) {
  StringRef(RawAssertList).split(AssertNames, ",", -1, false);
  // NOTE: default parameters should NOT produce any warnings. isNOP() == true
}

void AssertionCountCheck::storeOptions(ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "LineThreshold", LineThreshold);
  Options.store(Opts, "AssertsThreshold", AssertsThreshold);
  Options.store(Opts, "LinesStep", LinesStep);
  Options.store(Opts, "AssertsStep", AssertsStep);
  Options.store(Opts, "AssertNames", RawAssertList);
}

bool AssertionCountCheck::isNOP() const {
  // is there any configured assertion limit?
  // if no limits are imposed, then no function can violate them.

  // see unittest for the logic behind these checks.

  // if there is no base limit for the asserts count, and no either/both of the
  // steps according to which the assertion count expectation would increase,
  // then the expected minimal assertion count is always zero.

  return AssertsThreshold == 0 && (LinesStep == 0 || AssertsStep == 0);
}

void AssertionCountCheck::registerPPCallbacks(CompilerInstance &Compiler) {
  // if the check will never complain, then there is no point in bothering.
  if (isNOP())
    return;

  Compiler.getPreprocessor().addPPCallbacks(
      llvm::make_unique<AssertionMacrosCallbacks>(this->AssertNames,
                                                  &(this->FoundAssertions)));
}

void AssertionCountCheck::registerMatchers(MatchFinder *Finder) {
  // if the check will never complain, then there is no point in bothering.
  if (isNOP())
    return;

  Finder->addMatcher(functionDecl(unless(isImplicit())).bind("func"), this);
}

unsigned AssertionCountCheck::expectedAssertionsMin(unsigned Lines) const {
  // can NOT check isNOP() here, unittest uses this function!

  // if the function is shorter than the threshold, impose no limits
  if (Lines < LineThreshold)
    return 0U;

  assert(Lines >= LineThreshold);

  // if the function is no less than threshold lines, it must have at least
  // this much assertions.
  unsigned AssertionsMin = AssertsThreshold;

  // if the function has exactly the threshold lines, nothing more to do
  if (Lines == LineThreshold)
    return AssertionsMin;

  // if there are no scales, all functions above threshold should contain
  // no less than that ^ much assertions, regardless of their linelength
  // NOTE: and if AssertsThreshold is zero, then isNOP() is true
  if (LinesStep == 0 || AssertsStep == 0)
    return AssertionsMin;

  assert(LinesStep != 0);
  assert(AssertsStep != 0);

  // how many more lines does the function have, above the threshold?
  Lines -= LineThreshold;
  assert(Lines > 0);

  // how many multiples of the LinesStep is that?
  // NOTE: this is floor division! any fractional part is ignored!
  const unsigned LinesMultiple = Lines / LinesStep;
  assert(LinesStep * LinesMultiple <= Lines);

  // are there any multiples?
  if (LinesMultiple == 0)
    return AssertionsMin;

  // additionally increase expected assertion count
  assert(AssertsStep * LinesMultiple > 0);
  AssertionsMin += AssertsStep * LinesMultiple;

  return AssertionsMin;
}

void AssertionCountCheck::check(const MatchFinder::MatchResult &Result) {
  assert(!isNOP() && "if config params are NOP, then we should not be here");

  const auto *Func = Result.Nodes.getNodeAs<FunctionDecl>("func");
  assert(Func);

  FunctionInfo FI;

  const SourceManager *SM = Result.SourceManager;
  const Stmt *Body = Func->getBody();

  if (!Body)
    return;

  if (!SM->isWrittenInSameFile(Body->getLocStart(), Body->getLocEnd()))
    return;

  // Count the lines including whitespace and comments. Really simple.
  FI.Lines = SM->getSpellingLineNumber(Body->getLocEnd()) -
             SM->getSpellingLineNumber(Body->getLocStart());

  // if the function has less lines than threshold, no point in checking
  if (FI.Lines < LineThreshold)
    return;

  assert(FI.Lines >= LineThreshold);
  assert(expectedAssertionsMin(FI.Lines) > 0);

  // out of all the macro expansions, count only the ones in the current func.
  FI.Assertions = CountMacroAssertions(FoundAssertions, SM, Body);

  // now, let's chech the AST. e.g. because we need to ignore all the assertions
  // that are locate within the lambda functions.

  FunctionASTVisitor Visitor(AssertNames, FoundAssertions, Result.Context, &FI);
  Visitor.TraverseDecl(const_cast<FunctionDecl *>(Func));

  if (FI.Assertions >= expectedAssertionsMin(FI.Lines)) {
    // good, have enough assertions.
    return;
  }

  // very well, let's complain then

  diag(Func->getLocation(),
       "function %0 falls behind recommended lines-per-assertion thresholds")
      << Func;

  diag(Func->getLocation(),
       "%0 lines including whitespace and comments (threshold %1)",
       DiagnosticIDs::Note)
      << FI.Lines << LineThreshold;

  diag(Func->getLocation(), "%0 assertions (recommended %1)",
       DiagnosticIDs::Note)
      << FI.Assertions << expectedAssertionsMin(FI.Lines);
}

} // namespace misc
} // namespace tidy
} // namespace clang
