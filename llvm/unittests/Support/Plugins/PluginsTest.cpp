//===- llvm/unittest/Support/Plugins/PluginsTest.cpp --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define DONT_GET_PLUGIN_LOADER_OPTION
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Config/config.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Plugin.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "gtest/gtest.h"

#include "TestPlugin.h"

using namespace llvm;

void anchor() {}

static std::string LibPath(const std::string Name = "TestPlugin") {
  const std::vector<testing::internal::string> &Argvs =
      testing::internal::GetArgvs();
  const char *Argv0 = Argvs.size() > 0 ? Argvs[0].c_str() : "PluginsTests";
  void *Ptr = (void *)anchor;
  std::string Path = sys::fs::getMainExecutable(Argv0, Ptr);
  llvm::SmallString<256> Buf{sys::path::parent_path(Path)};
  sys::path::append(Buf, (Name + ".so").c_str());
  return Buf.str();
}

static std::string StdString(const char *c_str) { return c_str ? c_str : ""; }

TEST(PluginsTests, LoadPlugin) {
  ASSERT_EQ(0u, PluginLoader::getNumPlugins());

  auto PluginPath = LibPath();
  ASSERT_NE("", PluginPath);

  PluginLoader() = PluginPath;
  ASSERT_EQ(1u, PluginLoader::getNumPlugins()) << "Plugin path: " << PluginPath;

  ASSERT_EQ(PluginPath, PluginLoader::getPlugin(0));
  const auto &Info = PluginLoader::getPluginInfo(0);

  ASSERT_EQ(TEST_PLUGIN_NAME, StdString(Info.PluginName));
  ASSERT_EQ(TEST_PLUGIN_VERSION, StdString(Info.PluginVersion));
  ASSERT_NE(nullptr, Info.RegisterPassBuilderCallbacks);

  PassBuilder PB;
  ModulePassManager PM;
  ASSERT_FALSE(PB.parsePassPipeline(PM, "plugin-pass"));

  Info.RegisterPassBuilderCallbacks(PB);
  ASSERT_TRUE(PB.parsePassPipeline(PM, "plugin-pass"));
}
