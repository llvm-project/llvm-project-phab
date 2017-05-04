//===--- SanitizerArgs.h - Arguments for sanitizer tools  -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_CLANG_DRIVER_SANITIZERARGS_H
#define LLVM_CLANG_DRIVER_SANITIZERARGS_H

#include "clang/Basic/Sanitizers.h"
#include "clang/Basic/SanitizerBlacklist.h"
#include "clang/Driver/Types.h"
#include "llvm/ADT/Optional.h"
#include "llvm/Option/Arg.h"
#include "llvm/Option/ArgList.h"
#include <string>
#include <vector>

namespace clang {
namespace driver {

class Driver;
class ToolChain;

class SanitizerArgs {
  SanitizerSet Sanitizers;
  SanitizerSet RecoverableSanitizers;
  SanitizerSet TrapSanitizers;

  int CoverageFeatures = 0;
  int MsanTrackOrigins = 0;
  bool MsanUseAfterDtor = false;
  bool CfiCrossDso = false;
  int AsanFieldPadding = 0;
  bool AsanSharedRuntime = false;
  bool AsanUseAfterScope = true;
  bool LinkCXXRuntimes = false;
  bool NeedPIE = false;
  bool Stats = false;
  bool TsanMemoryAccess = true;
  bool TsanFuncEntryExit = true;
  bool TsanAtomics = true;

  /// Blacklists which each apply to specific sanitizers.
  std::vector<SanitizerBlacklist> SanitizerBlacklists;

  /// Paths to all sanitizer blacklist files.
  std::vector<std::string> SanitizerBlacklistFiles;

  /// Paths to sanitizer blacklist files which need depfile entries.
  std::vector<StringRef> ExtraDeps;

  /// Collect all default blacklists for the sanitizers enabled in \p Kinds.
  void collectDefaultBlacklists(const Driver &D, SanitizerMask Kinds);

 public:
  /// Parses the sanitizer arguments from an argument list.
  SanitizerArgs(const ToolChain &TC, const llvm::opt::ArgList &Args);

  bool needsAsanRt() const { return Sanitizers.has(SanitizerKind::Address); }
  bool needsSharedAsanRt() const { return AsanSharedRuntime; }
  bool needsTsanRt() const { return Sanitizers.has(SanitizerKind::Thread); }
  bool needsMsanRt() const { return Sanitizers.has(SanitizerKind::Memory); }
  bool needsFuzzer() const { return Sanitizers.has(SanitizerKind::Fuzzer); }
  bool needsLsanRt() const {
    return Sanitizers.has(SanitizerKind::Leak) &&
           !Sanitizers.has(SanitizerKind::Address);
  }
  bool needsUbsanRt() const;
  bool needsDfsanRt() const { return Sanitizers.has(SanitizerKind::DataFlow); }
  bool needsSafeStackRt() const {
    return Sanitizers.has(SanitizerKind::SafeStack);
  }
  bool needsCfiRt() const;
  bool needsCfiDiagRt() const;
  bool needsStatsRt() const { return Stats; }
  bool needsEsanRt() const {
    return Sanitizers.hasOneOf(SanitizerKind::Efficiency);
  }

  bool requiresPIE() const;
  bool needsUnwindTables() const;
  bool linkCXXRuntimes() const { return LinkCXXRuntimes; }
  bool hasCrossDsoCfi() const { return CfiCrossDso; }
  void addArgs(const ToolChain &TC, const llvm::opt::ArgList &Args,
               llvm::opt::ArgStringList &CmdArgs, types::ID InputType) const;

  /// Encode a sanitizer blacklist argument as a string.
  static std::string encodeBlacklistArg(const SanitizerBlacklist &SB);

  /// Decode a sanitizer blacklist argument. Returns true on success.
  static bool decodeBlacklistArg(const std::string &Arg, SanitizerMask &Kinds,
                                 std::string &Path);
};

}  // namespace driver
}  // namespace clang

#endif
