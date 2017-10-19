//===--- JSON.h - LLVM customizations of nlohmann-json ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANGD_JSON_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANGD_JSON_H

#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"
#include "nlohmann-json/json.hpp"

namespace clang {
namespace clangd {

// It would be nice to support BumpPtrAllocator, but stateful allocators are not
// supported: https://github.com/nlohmann/json/issues/161
using JSON = nlohmann::basic_json<
    /*ObjectType=*/std::map,      // TODO: StringMap
    /*ArrayType=*/std::vector,    // TODO: SmallVector
    /*StringType=*/std::string>;  // TODO: SmallString

inline JSON parseJSON(nlohmann::detail::input_adapter i) {
  return JSON::parse(std::move(i), /*callback=*/nullptr,
                     /*allow_exceptions=*/false);
}

}  // namespace clangd
}  // namespace clang

#endif
