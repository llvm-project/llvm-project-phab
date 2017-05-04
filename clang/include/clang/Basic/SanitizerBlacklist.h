//===--- SanitizerBlacklist.h - Blacklist for sanitizers --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// User-provided blacklist used to disable/alter instrumentation done in
// sanitizers.
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_CLANG_BASIC_SANITIZERBLACKLIST_H
#define LLVM_CLANG_BASIC_SANITIZERBLACKLIST_H

#include "clang/Basic/LLVM.h"
#include "clang/Basic/Sanitizers.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/SpecialCaseList.h"
#include <memory>

namespace clang {

struct SanitizerBlacklist {
  /// The set of sanitizers this blacklist applies to.
  SanitizerMask Sanitizers;

  /// The path to the blacklist file.
  StringRef Path;

  SanitizerBlacklist(SanitizerMask SM, StringRef P) : Sanitizers(SM), Path(P) {}
};

class SanitizerBlacklistInfo {
  struct Blacklist {
    SanitizerMask Sanitizers;
    std::unique_ptr<llvm::SpecialCaseList> SCL;

    Blacklist(SanitizerMask Sanitizers,
              std::unique_ptr<llvm::SpecialCaseList> SCL)
        : Sanitizers(Sanitizers), SCL(std::move(SCL)) {}
  };

  SmallVector<Blacklist, 2> BLs;
  SourceManager &SM;

  bool isBlacklisted(StringRef SectionName, StringRef Ident, SanitizerMask Kind,
                     StringRef Category) const;

public:
  SanitizerBlacklistInfo(ArrayRef<SanitizerBlacklist> Blacklists,
                         SourceManager &SM);
  bool isBlacklistedGlobal(StringRef GlobalName, SanitizerMask Kind,
                           StringRef Category = StringRef()) const;
  bool isBlacklistedType(StringRef MangledTypeName, SanitizerMask Kind,
                         StringRef Category = StringRef()) const;
  bool isBlacklistedFunction(StringRef FunctionName, SanitizerMask Kind) const;
  bool isBlacklistedFile(StringRef FileName, SanitizerMask Kind,
                         StringRef Category = StringRef()) const;
  bool isBlacklistedLocation(SourceLocation Loc, SanitizerMask Kind,
                             StringRef Category = StringRef()) const;
};

}  // end namespace clang

#endif
