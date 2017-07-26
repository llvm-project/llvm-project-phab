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
	PassedOptRemarkEnable
};
class DiagnosticInfo;
class DiagnosticHandler {
public:
	virtual ~DiagnosticHandler() = default;
  using DiagnosticHandlerTy = void (*)(const DiagnosticInfo &DI, void *Context);
  DiagnosticHandlerTy DiagHandlerCallback;
  RemarkInfo isRemarkEnable(const std::string &PassName);
	void setAnalysisRemarkPassNamePattern(std::shared_ptr<Regex> AnalysisRemark);
	void setPassedRemarkPassNamePattern(std::shared_ptr<Regex> PassedOptRemark);
	void setMissedRemarkPassNamePattern(std::shared_ptr<Regex> MissedOptRemark);
	virtual void HandleDiagnostics(const DiagnosticInfo &DI, void *Context);
	static bool isAnalysisRemarkEnable(const std::string &PassName);
	static bool isMissedOptRemarkEnable(const std::string &PassName);
	static bool isPassedOptRemarkEnable(const std::string &PassName);
};
}
