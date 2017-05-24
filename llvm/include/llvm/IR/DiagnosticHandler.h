//===- DiagnosticHandler.cpp - DiagnosticHandler class for LLVM -*- C++ -*-===//
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

#include "llvm/Support/Regex.h"

namespace llvm {
enum class RemarkInfo {
	RemarkNotEnabled,
	AnalysisRemarkEnable,
	MissedOptRemarkEnable,
	OptRemarkEnable
};
class DiagnosticInfo;
class DiagnosticHandler {
public:
  using DiagnosticHandlerTy = void (*)(const DiagnosticInfo &DI, void *Context);
  DiagnosticHandlerTy DiagnosticHandler;
  RemarkInfo isRemarkEnable(const std::string &PassName);
  std::shared_ptr<Regex> PassRemarkAnalysisClang;
	std::shared_ptr<Regex> PassRemarkOptClang;
	std::shared_ptr<Regex> PassRemarkMissedOptClang;
};
}
