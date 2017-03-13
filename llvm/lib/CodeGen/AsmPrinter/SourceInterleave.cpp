//===-- llvm/lib/CodeGen/AsmPrinter/SourceInterleave.cpp --*- C++ -*--===//
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

#include "SourceInterleave.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Support/LineIterator.h"
#include "llvm/Support/Path.h"

namespace llvm {

void SourceInterleave::indexBuffer(SourceCodeInfo *SI) {
  SI->LineIndex.reserve(128);
  SI->LineIndex.insert(SI->LineIndex.end(),
                       line_iterator(*SI->MemBuff, /* SkipBlanks */ false),
                       line_iterator());
}

SourceInterleave::SourceCodeInfo *
SourceInterleave::getOrCreateSourceCodeInfo(StringRef FullPathname) {
  auto it = BufferMap.find(FullPathname);
  if (it == BufferMap.end()) {
    std::unique_ptr<SourceCodeInfo> SI(new SourceCodeInfo);
    auto ErrorMemBuffer = MemoryBuffer::getFile(FullPathname);
    if (ErrorMemBuffer) {
      SI->MemBuff = std::move(*ErrorMemBuffer);
      indexBuffer(SI.get());
    }
    it = BufferMap.insert({FullPathname, std::move(SI)}).first;
  }
  return it->second.get();
}

std::string SourceInterleave::getFullPathname(StringRef Dir, StringRef Fn) {
  SmallString<128> FullPathname;
  llvm::sys::path::append(FullPathname, Dir, Fn);

  return FullPathname.str();
}

std::string SourceInterleave::getFullPathname(const DebugLoc &DL) {
  std::string Dir;
  std::string Fn;
  if (DIScope *S = DL->getScope()) {
    Dir = S->getDirectory();
    Fn = S->getFilename();
  }

  return getFullPathname(Dir, Fn);
}

void SourceInterleave::endModule() {}

void SourceInterleave::beginFunction(const MachineFunction *MF) {
  const Function *F = MF->getFunction();
  DISubprogram *DS = F->getSubprogram();
  if (!DS)
    return;

  if (!DS->getFile())
    return;

  std::string FullPathname = getFullPathname(DS->getFile()->getDirectory(),
                                             DS->getFile()->getFilename());
  SourceCodeInfo *SI = getOrCreateSourceCodeInfo(FullPathname);
  assert(SI);

  // If we could not load the file, give up.
  if (SI->MemBuff == nullptr)
    return;

  // While unlikely, it might happen that we do not really know where this
  // function starts.
  if (DS->getLine() > 0)
    SI->LastLineSeen = DS->getLine() - 1;
}

void SourceInterleave::endFunction(const MachineFunction *MF) {}

void SourceInterleave::beginInstruction(const MachineInstr *MI) {
  const DebugLoc &DL = MI->getDebugLoc();
  if (!DL)
    return;

  std::string FullPathname = getFullPathname(DL);
  SourceCodeInfo *SI = getOrCreateSourceCodeInfo(FullPathname);
  assert(SI);

  // If we could not load the file, give up.
  if (SI->MemBuff == nullptr)
    return;

  int CurrentLine = DL->getLine();
  if (CurrentLine == 0)
    return;

  // If we have no idea where this function started, use the current line
  // as a sensible starting point.
  if (SI->LastLineSeen == SourceCodeInfo::UnknownFunctionStart)
    SI->LastLineSeen = CurrentLine - 1;
  // If we're not moving or going backwards, ignore this.
  else if (CurrentLine - SI->LastLineSeen < 0)
    return;

  for (int Line = SI->LastLineSeen + 1; Line <= CurrentLine; Line++) {
    if (Line <= 0 || (unsigned)Line > SI->LineIndex.size())
      break;

    SmallString<128> Str;
    raw_svector_ostream OS(Str);

    OS << ' ' << FullPathname << ':' << Line << ":  ";
    OS << SI->LineIndex[Line - 1].str();

    AP->OutStreamer->emitRawComment(OS.str(), false);
  }

  SI->LastLineSeen = CurrentLine;
}

void SourceInterleave::endInstruction() {}
}
