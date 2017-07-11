//===-- PluginLoader.cpp - Implement -load command line option ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the -load <plugin> command line option handler.
//
//===----------------------------------------------------------------------===//

#define DONT_GET_PLUGIN_LOADER_OPTION
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Mutex.h"
#include "llvm/Support/Plugin.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>
using namespace llvm;

class Plugin {
public:
  static Optional<Plugin> Load(const std::string &Filename,
                               const sys::DynamicLibrary &Library);
  std::string &getFilename() { return Filename; }
  const PluginInfo &getInfo() const { return Info; }

private:
  Plugin(const std::string &Filename, const sys::DynamicLibrary &Library)
      : Filename(Filename), Library(Library) {}
  std::string Filename;
  sys::DynamicLibrary Library;
  PluginInfo Info;
};

static ManagedStatic<std::vector<Plugin>> Plugins;
static ManagedStatic<sys::SmartMutex<true>> PluginsLock;

Optional<Plugin> Plugin::Load(const std::string &Filename,
                              const sys::DynamicLibrary &Library) {
  Plugin P{Filename, Library};
  auto *getDetailsFn = Library.SearchForAddressOfSymbol("llvmPluginGetInfo");

  if (!getDetailsFn)
    // If the symbol isn't found, this is probably a legacy plugin. Just treat
    // it as such.
    return P;

  P.Info = reinterpret_cast<decltype(llvmPluginGetInfo) *>(getDetailsFn)();

  if (P.Info.APIVersion != LLVM_PLUGIN_API_VERSION) {
    errs() << "Wrong API version on plugin '" << Filename << "'. Got version "
           << P.Info.APIVersion << ", supported version is "
           << LLVM_PLUGIN_API_VERSION << ".\n";
    return None;
  }

  return P;
}

void PluginLoader::operator=(const std::string &Filename) {
  sys::SmartScopedLock<true> Lock(*PluginsLock);
  std::string Error;
  auto Library =
      sys::DynamicLibrary::getPermanentLibrary(Filename.c_str(), &Error);
  if (!Library.isValid()) {
    errs() << "Error opening '" << Filename << "': " << Error
           << "\n  -load request ignored.\n";
    return;
  }

  auto P = Plugin::Load(Filename, Library);
  if (!P) {
    errs() << "Failed to load plugin '" << Filename << "'\n";
    return;
  }

  Plugins->emplace_back(std::move(*P));
}

unsigned PluginLoader::getNumPlugins() {
  sys::SmartScopedLock<true> Lock(*PluginsLock);
  return Plugins.isConstructed() ? Plugins->size() : 0;
}

std::string &PluginLoader::getPlugin(unsigned num) {
  sys::SmartScopedLock<true> Lock(*PluginsLock);
  assert(Plugins.isConstructed() && num < Plugins->size() &&
         "Asking for an out of bounds plugin");
  return (*Plugins)[num].getFilename();
}
const PluginInfo &PluginLoader::getPluginInfo(unsigned num) {
  sys::SmartScopedLock<true> Lock(*PluginsLock);
  assert(Plugins.isConstructed() && num < Plugins->size() &&
         "Asking for an out of bounds plugin");
  return (*Plugins)[num].getInfo();
}
