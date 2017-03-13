//===-- llvm/lib/CodeGen/AsmPrinter/SourceInterleave.h --*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains support for interleaving source code with assembler.
//
//===----------------------------------------------------------------------===//

#include "AsmPrinterHandler.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/DebugLoc.h"
#include <memory>

namespace llvm {

class SourceInterleave : public AsmPrinterHandler {
  struct SourceCodeInfo {
    enum { UnknownFunctionStart = -2 };
    int LastLineSeen = 0;
    std::unique_ptr<MemoryBuffer> MemBuff;
    std::vector<StringRef> LineIndex;
  };

  llvm::StringMap<std::unique_ptr<SourceCodeInfo>> BufferMap;

  static void indexBuffer(SourceCodeInfo *SI);
  SourceCodeInfo *getOrCreateSourceCodeInfo(StringRef FullPath);

  static std::string getFullPathname(StringRef Dir, StringRef Fn);
  static std::string getFullPathname(const DebugLoc &DL);
  AsmPrinter *AP;

public:
  SourceInterleave(AsmPrinter *ap) : AP(ap) {}

  void setSymbolSize(const MCSymbol *Sym, uint64_t Size) override {}

  void endModule() override;

  void beginFunction(const MachineFunction *MF) override;
  void endFunction(const MachineFunction *MF) override;

  void beginInstruction(const MachineInstr *MI) override;
  void endInstruction() override;
};
}
