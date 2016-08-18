//===-- AAPTargetInfo.cpp - AAP Target Implementation ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/Module.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

namespace llvm {
Target TheAAPTarget;
}

extern "C" void LLVMInitializeAAPTargetInfo() {
  RegisterTarget<Triple::aap> X(TheAAPTarget, "aap", "AAP [experimental]");
}

// Placeholder. This will be removed when the MC support has been
// introduced
extern "C" void LLVMInitializeAAPTargetMC() {
}
