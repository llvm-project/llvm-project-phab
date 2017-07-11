//===-- llvm/Support/PluginLoader.h - Plugin Loader for Tools ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_PLUGIN_H
#define LLVM_SUPPORT_PLUGIN_H

#include <cstdint>
#include <functional>

namespace llvm {
class PassBuilder;

#define LLVM_PLUGIN_API_VERSION 1

#ifdef WIN32
#define LLVM_PLUGIN_EXPORT __declspec(dllexport)
#else
#define LLVM_PLUGIN_EXPORT
#endif

extern "C" {
struct PluginInfo {
  uint32_t APIVersion;
  const char *PluginName;
  const char *PluginVersion;

  void (*RegisterPassBuilderCallbacks)(PassBuilder &);
};

PluginInfo LLVM_PLUGIN_EXPORT llvmPluginGetInfo();
}
}

#define LLVM_PLUGIN(NAME, PLUGIN_VERSION, REGISTER_CALLBACKS)                  \
  extern "C" {                                                                 \
  ::llvm::PluginInfo LLVM_PLUGIN_EXPORT llvmPluginGetInfo() {                  \
    return {LLVM_PLUGIN_API_VERSION, NAME, PLUGIN_VERSION,                     \
            REGISTER_CALLBACKS};                                               \
  }                                                                            \
  }

#endif /* LLVM_SUPPORT_PLUGIN_H */
