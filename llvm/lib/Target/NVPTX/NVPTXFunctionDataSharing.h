//===--- NVPTXFrameLowering.h - Define frame lowering for NVPTX -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_NVPTX_NVPTXFUNCTIONDATASHARING_H
#define LLVM_LIB_TARGET_NVPTX_NVPTXFUNCTIONDATASHARING_H

namespace llvm {

class NVPTXFunctionDataSharing : public FunctionPass {
  bool runOnFunction(Function &F) override;
  bool runOnKernelFunction(Function &F);
  bool runOnDeviceFunction(Function &F);

public:
  static char ID; // Pass identification, replacement for typeid
  NVPTXFunctionDataSharing(const NVPTXTargetMachine *TM = nullptr)
      : FunctionPass(ID), TM(TM) {}
  StringRef getPassName() const override {
    return "Function level data sharing pass.";
  }

private:
  const NVPTXTargetMachine *TM;
};
} // End llvm namespace

#endif