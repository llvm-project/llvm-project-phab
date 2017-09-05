//===- DiagnosticHandler.cpp - DiagnosticHandler class for LLVM -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Base DiagnosticHandler class declaration. Derive from this class to provide
// custom diagnostic reporting.
//===----------------------------------------------------------------------===//

#include "llvm/Support/Regex.h"

namespace llvm {
class DiagnosticInfo;

struct DiagnosticHandler {
public:
  void *DiagnosticContext = nullptr;
  DiagnosticHandler(void *DiagContext = nullptr)
      : DiagnosticContext(DiagContext) {}
  virtual ~DiagnosticHandler() = default;

  using DiagnosticHandlerTy = void (*)(const DiagnosticInfo &DI, void *Context);

  /// DiagHandlerCallback is settable from the C API and base implimentation
  /// of DiagnosticHandler will call it from handleDiagnostics(). Any derived
  /// class of DiagnosticHandler should not use callback but
  /// implement handleDiagnostics().
  DiagnosticHandlerTy DiagHandlerCallback = nullptr;

  /// Override handleDiagnostics to provide custom implementation.
  /// Return true if it handles diagnostics reporting properly otherwise
  /// return false to make LLVMContext::diagnose() to print the message
  /// with a prefix based on the severity.
  virtual bool handleDiagnostics(const DiagnosticInfo &DI) {
    if (DiagHandlerCallback) {
      DiagHandlerCallback(DI, DiagnosticContext);
      return true;
    }
    return false;
  }

  /// checks if remark requested with -pass-remarks-analysis, override
  /// to provide different implementation
  virtual bool isAnalysisRemarkEnabled(StringRef &&PassName) const;

  /// checks if remarks requested with -pass-remarks-missed, override
  /// to provide different implementation
  virtual bool isMissedOptRemarkEnabled(StringRef &&PassName) const;

  /// checks if remarks requsted with -pass-remarks, override to provide
  /// different implementation
  virtual bool isPassedOptRemarkEnabled(StringRef &&PassName) const;

  /// checks if remark requested with -pass-remarks{-missed/-analysis}
  bool isAnyRemarkEnabled(StringRef &&PassName) const {
    return (isMissedOptRemarkEnabled(std::move(PassName)) ||
            isPassedOptRemarkEnabled(std::move(PassName)) ||
            isAnalysisRemarkEnabled(std::move(PassName)));
  }
};
}
