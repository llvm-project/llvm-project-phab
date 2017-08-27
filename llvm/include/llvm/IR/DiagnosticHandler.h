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
// enum class RemarkInfo {
// 	RemarkNotEnabled,
// 	AnalysisRemarkEnable,
// 	MissedOptRemarkEnable,
// 	PassedOptRemarkEnable
// };
struct RemarkInfo {
	unsigned char AnalysisRemarkEnable : 1;
	unsigned char MissedOptRemarkEnable : 1;
	unsigned char PassedOptRemarkEnable : 1;
};

class DiagnosticInfo;
class DiagnosticHandler {
public:
	virtual ~DiagnosticHandler() = default;
  using DiagnosticHandlerTy = void (*)(const DiagnosticInfo &DI, void *Context);
  DiagnosticHandlerTy DiagHandlerCallback;
  void isRemarkEnable(const std::string &PassName, RemarkInfo &Output);
	// void setAnalysisRemarkPassNamePattern(std::shared_ptr<Regex> AnalysisRemark);
	// void setPassedRemarkPassNamePattern(std::shared_ptr<Regex> PassedOptRemark);
	// void setMissedRemarkPassNamePattern(std::shared_ptr<Regex> MissedOptRemark);
	virtual bool handleDiagnostics(const DiagnosticInfo &DI, void *Context);
	virtual bool isAnalysisRemarkEnable(const std::string &PassName);
	virtual bool isMissedOptRemarkEnable(const std::string &PassName);
	virtual bool isPassedOptRemarkEnable(const std::string &PassName);
};
}
