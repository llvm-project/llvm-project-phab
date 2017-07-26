//===- DiagnosticHandler.h - DiagnosticHandler class for LLVM -------------===//
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
#include "llvm/IR/DiagnosticHandler.h"
#include "llvm/Support/CommandLine.h"
#include <string>
#include <iostream>
using namespace llvm;

namespace {

/// \brief Regular expression corresponding to the value given in one of the
/// -pass-remarks* command line flags. Passes whose name matches this regexp
/// will emit a diagnostic when calling the associated diagnostic function
/// (emitOptimizationRemark, emitOptimizationRemarkMissed or
/// emitOptimizationRemarkAnalysis).
struct PassRemarksOpt {
  std::shared_ptr<Regex> Pattern;

  void operator=(const std::string &Val) {
    // Create a regexp object to match pass names for emitOptimizationRemark.
    if (!Val.empty()) {
      Pattern = std::make_shared<Regex>(Val);
      std::string RegexError;
      if (!Pattern->isValid(RegexError))
        report_fatal_error("Invalid regular expression '" + Val +
                               "' in -pass-remarks: " + RegexError,
                           false);
    }
  }
};

static PassRemarksOpt PassRemarksPassedOptLoc;
static PassRemarksOpt PassRemarksMissedOptLoc;
static PassRemarksOpt PassRemarksAnalysisOptLoc;

// -pass-remarks
//    Command line flag to enable emitOptimizationRemark()
static cl::opt<PassRemarksOpt, true, cl::parser<std::string>>
PassRemarks("pass-remarks", cl::value_desc("pattern"),
            cl::desc("Enable optimization remarks from passes whose name match "
                     "the given regular expression"),
            cl::Hidden, cl::location(PassRemarksPassedOptLoc), cl::ValueRequired,
            cl::ZeroOrMore);

// -pass-remarks-missed
//    Command line flag to enable emitOptimizationRemarkMissed()
static cl::opt<PassRemarksOpt, true, cl::parser<std::string>> PassRemarksMissed(
    "pass-remarks-missed", cl::value_desc("pattern"),
    cl::desc("Enable missed optimization remarks from passes whose name match "
             "the given regular expression"),
    cl::Hidden, cl::location(PassRemarksMissedOptLoc), cl::ValueRequired,
    cl::ZeroOrMore);

// -pass-remarks-analysis
//    Command line flag to enable emitOptimizationRemarkAnalysis()
static cl::opt<PassRemarksOpt, true, cl::parser<std::string>>
PassRemarksAnalysis(
    "pass-remarks-analysis", cl::value_desc("pattern"),
    cl::desc(
        "Enable optimization analysis remarks from passes whose name match "
        "the given regular expression"),
    cl::Hidden, cl::location(PassRemarksAnalysisOptLoc), cl::ValueRequired,
    cl::ZeroOrMore);
}

void DiagnosticHandler::setAnalysisRemarkPassNamePattern(std::shared_ptr<Regex> AnalysisRemark) {
    PassRemarksAnalysisOptLoc.Pattern = AnalysisRemark;
}

void DiagnosticHandler::setPassedRemarkPassNamePattern(std::shared_ptr<Regex> PassedOptRemark) {
    PassRemarksPassedOptLoc.Pattern = PassedOptRemark;
}

void DiagnosticHandler::setMissedRemarkPassNamePattern(std::shared_ptr<Regex> MissedOptRemark) {
    PassRemarksMissedOptLoc.Pattern = MissedOptRemark;
}

RemarkInfo DiagnosticHandler::isRemarkEnable(const std::string &PassName) {
	if (PassRemarksMissedOptLoc.Pattern && PassRemarksMissedOptLoc.Pattern->match(PassName))
    return RemarkInfo::MissedOptRemarkEnable;

	if (PassRemarksPassedOptLoc.Pattern && PassRemarksPassedOptLoc.Pattern->match(PassName))
    return RemarkInfo::PassedOptRemarkEnable;

  if (PassRemarksAnalysisOptLoc.Pattern && PassRemarksAnalysisOptLoc.Pattern->match(PassName))
    return RemarkInfo::AnalysisRemarkEnable;

	return RemarkInfo::RemarkNotEnabled;
}

bool DiagnosticHandler::isAnalysisRemarkEnable(const std::string &PassName) {
  if (PassRemarksAnalysisOptLoc.Pattern && PassRemarksAnalysisOptLoc.Pattern->match(PassName))
    return true;
  return false;
}
bool DiagnosticHandler::isMissedOptRemarkEnable(const std::string &PassName) {
	if (PassRemarksMissedOptLoc.Pattern && PassRemarksMissedOptLoc.Pattern->match(PassName))
    return true;
  return false;
}
bool DiagnosticHandler::isPassedOptRemarkEnable(const std::string &PassName) {
  if (PassRemarksPassedOptLoc.Pattern && PassRemarksPassedOptLoc.Pattern->match(PassName))
    return true;
  return false;
}

void  DiagnosticHandler::HandleDiagnostics(const DiagnosticInfo &DI, void *Context) {
    if(DiagHandlerCallback)
      DiagHandlerCallback(DI, Context);
}