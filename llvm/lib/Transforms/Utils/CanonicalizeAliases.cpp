//===- CanonicalizeAliases.cpp - ThinLTO Support: Canonicalize Aliases ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements alias canonicalization, so that all aliasees
// are private anonymous values. E.g.
//  @a = alias i8, i8 *@g
//  @g = global i8 0
//
// will be converted to:
//  @0 = private global
//  @a = alias i8, i8* @0
//  @g = alias i8, i8* @0
//
// This simplifies optimization and ThinLTO linking of the original symbols.
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/CanonicalizeAliases.h"

#include "llvm/IR/Operator.h"
#include "llvm/IR/ValueHandle.h"

using namespace llvm;

/// Check if the given user is an alias (looking through bit casts).
static bool userIsAlias(Value *V) {
  if (isa<GlobalAlias>(V))
    return true;
  if (BitCastOperator *BC = dyn_cast<BitCastOperator>(V)) {
    for (Use &U : BC->materialized_uses()) {
      return userIsAlias(U.getUser());
    }
  }
  return false;
}

/// Like replaceAllUsesWith but doesn't replace any uses that are
/// aliases, so that we can just replace uses of the original value
/// with the new private aliasee, but keep the original value name
/// as an alias to it.
static void replaceAllUsesWithExceptAliases(Value *Old, Value *New) {
  assert(New && "Value::replaceAllUsesWith(<null>) is invalid!");
  assert(New->getType() == Old->getType() &&
         "replaceAllUses of value with new value of different type!");

  // Notify all ValueHandles (if present) that this value is going away.
  if (Old->hasValueHandle())
    ValueHandleBase::ValueIsRAUWd(Old, New);
  if (Old->isUsedByMetadata())
    ValueAsMetadata::handleRAUW(Old, New);

  for (Use &U : Old->materialized_uses()) {
    if (userIsAlias(U.getUser()))
      continue;
    // Must handle Constants specially, we cannot call replaceUsesOfWith on a
    // constant because they are uniqued.
    if (auto *C = dyn_cast<Constant>(U.getUser())) {
      if (!isa<GlobalValue>(C)) {
        C->handleOperandChange(Old, New);
        continue;
      }
    }

    U.set(New);
  }
}

namespace llvm {
/// Convert aliases to canonical form.
bool canonicalizeAliases(Module &M) {
  bool Changed = false;
  for (auto &GA : M.aliases()) {
    // Check if the aliasee is private already
    GlobalObject *GO = GA.getBaseObject();
    if (!GO || GO->hasPrivateLinkage())
      continue;

    // Don't make malformed IR (will be caught by verifier) worse.
    if (GO->isDeclarationForLinker())
      continue;

    // Create a new alias to aliasee that keeps the original aliasee name
    // and linkage.
    auto *NewGA = GlobalAlias::create("", GO);
    NewGA->takeName(GO);

    // Make the original aliasee anonymous
    GO->setName("");

    // Change linkage of renamed original aliasee to private
    GO->setLinkage(GlobalValue::PrivateLinkage);

    // RAUW all uses of original aliasee with the new alias.
    replaceAllUsesWithExceptAliases(GO, NewGA);

    Changed = true;
  }
  return Changed;
}
}

namespace {

// Legacy pass that canonicalizes aliases.
class CanonicalizeAliasesLegacyPass : public ModulePass {

public:
  /// Pass identification, replacement for typeid
  static char ID;

  /// Specify pass name for debug output
  StringRef getPassName() const override { return "Canonicalize Aliases"; }

  explicit CanonicalizeAliasesLegacyPass() : ModulePass(ID) {}

  bool runOnModule(Module &M) override { return canonicalizeAliases(M); }
};
char CanonicalizeAliasesLegacyPass::ID = 0;

} // anonymous namespace

PreservedAnalyses CanonicalizeAliases::run(Module &M,
                                           ModuleAnalysisManager &AM) {
  if (!canonicalizeAliases(M))
    return PreservedAnalyses::all();

  return PreservedAnalyses::none();
}

INITIALIZE_PASS_BEGIN(CanonicalizeAliasesLegacyPass, "canonicalize-aliases",
                      "Canonicalize aliases", false, false)
INITIALIZE_PASS_END(CanonicalizeAliasesLegacyPass, "canonicalize-aliases",
                    "Canonicalize aliases", false, false)

namespace llvm {
ModulePass *createCanonicalizeAliasesPass() {
  return new CanonicalizeAliasesLegacyPass();
}
}
