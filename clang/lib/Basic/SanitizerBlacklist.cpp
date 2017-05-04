//===--- SanitizerBlacklist.cpp - Blacklist for sanitizers ----------------===//
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
#include "clang/Basic/SanitizerBlacklist.h"

using namespace clang;

SanitizerBlacklistInfo::SanitizerBlacklistInfo(
    ArrayRef<SanitizerBlacklist> Blacklists, SourceManager &SM)
    : SM(SM) {
  for (const auto &SB : Blacklists)
    BLs.emplace_back(SB.Sanitizers,
                     llvm::SpecialCaseList::createOrDie({SB.Path}));
}

bool SanitizerBlacklistInfo::isBlacklisted(StringRef SectionName,
                                           StringRef Ident, SanitizerMask Kind,
                                           StringRef Category) const {
  for (const auto &BL : BLs)
    if ((BL.Sanitizers & Kind) &&
        BL.SCL->inSection(SectionName, Ident, Category))
      return true;
  return false;
}

bool SanitizerBlacklistInfo::isBlacklistedGlobal(StringRef GlobalName,
                                                 SanitizerMask Kind,
                                                 StringRef Category) const {
  return isBlacklisted("global", GlobalName, Kind, Category);
}

bool SanitizerBlacklistInfo::isBlacklistedType(StringRef MangledTypeName,
                                               SanitizerMask Kind,
                                               StringRef Category) const {
  return isBlacklisted("type", MangledTypeName, Kind, Category);
}

bool SanitizerBlacklistInfo::isBlacklistedFunction(StringRef FunctionName,
                                                   SanitizerMask Kind) const {
  return isBlacklisted("fun", FunctionName, Kind, StringRef());
}

bool SanitizerBlacklistInfo::isBlacklistedFile(StringRef FileName,
                                               SanitizerMask Kind,
                                               StringRef Category) const {
  return isBlacklisted("src", FileName, Kind, Category);
}

bool SanitizerBlacklistInfo::isBlacklistedLocation(SourceLocation Loc,
                                                   SanitizerMask Kind,
                                                   StringRef Category) const {
  return Loc.isValid() &&
         isBlacklistedFile(SM.getFilename(SM.getFileLoc(Loc)), Kind, Category);
}
