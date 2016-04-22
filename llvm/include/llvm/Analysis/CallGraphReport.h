//===- CallGraphReport.h Implement call graph report ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes a class CallGraphReport which contains pure virtual 
// functions that should be redefined for each subclass.  These functions 
// indicate how the reports in each subclass should be modified when a  
// major transformation to the call graph occurs. 
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_CALLGRAPHREPORT_H
#define LLVM_ANALYSIS_CALLGRAPHREPORT_H

namespace llvm {

class Function; 

// \brief Class describing a Report that describes major transformations 
// on the call graph. 
class CallGraphReport { 
public: 
  
  virtual void replaceFunctionWithFunction(Function* OldFunction, 
    Function* NewFunction) = 0; 

}; 

}

#endif
