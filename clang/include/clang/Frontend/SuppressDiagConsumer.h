//===--- SuppressDiagConsumer.h - SuppressDiagononstic header data-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef SUPPRESSDIAGCONSUMER_H
#define SUPPRESSDIAGCONSUMER_H
#include "clang/Basic/Diagnostic.h"
#include "clang/Lex/Preprocessor.h"
#include <vector>
#include <string>
#include <map>

namespace clang {

class SuppressDiagConsumer;

typedef std::map<SourceLocation, std::vector<std::string> > IgnoredReports_t;

class UserSuppressions {
  IgnoredReports_t IgnoredReports;
public:
  friend class SuppressDiagConsumer;
  const IgnoredReports_t &getIgnoredReports() {
    return IgnoredReports;
  }
};

class SuppressDiagConsumer : public DiagnosticConsumer,
                             public CommentHandler {
  const Preprocessor *CurrentPreprocessor;
  UserSuppressions *AnalyzerUserSuppressions;
public:
  SuppressDiagConsumer(UserSuppressions *US)
   : AnalyzerUserSuppressions(US)
  { }

  DiagnosticConsumer *clone(DiagnosticsEngine &Diags) const {
    return new SuppressDiagConsumer(AnalyzerUserSuppressions);
  }

  void BeginSourceFile(const LangOptions &LangOpts,
                       const Preprocessor *PP);
  void EndSourceFile();
  bool HandleComment(Preprocessor &PP, SourceRange Comment);
  void HandleDiagnostic(DiagnosticsEngine::Level DiagLevel,
                        const Diagnostic &Info) {}
  UserSuppressions *getUserSuppressions() {
    return AnalyzerUserSuppressions;
  }
};

} // end namespace clang
#endif // SUPPRESSDIAGCONSUMER_H
