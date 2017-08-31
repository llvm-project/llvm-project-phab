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
	DiagnosticHandler (void *DiagContext);
	DiagnosticHandler() = default;
	virtual ~DiagnosticHandler() = default;
	using DiagnosticHandlerTy = void (*)(const DiagnosticInfo &DI, void *Context);
	// DiagHandlerCallback is settable from the C API and base implimentation
	// of DiagnosticHandler will call it from handleDiagnostics(). Any derived
	// class of DiagnosticHandler should not use callback but
	// implement handleDiagnostics().  
  DiagnosticHandlerTy DiagHandlerCallback = nullptr;
  bool isRemarkEnable(const StringRef &PassName);
	virtual bool handleDiagnostics(const DiagnosticInfo &DI);
	virtual bool isAnalysisRemarkEnable(const StringRef &PassName);
	virtual bool isMissedOptRemarkEnable(const StringRef &PassName);
	virtual bool isPassedOptRemarkEnable(const StringRef &PassName);
};
}
