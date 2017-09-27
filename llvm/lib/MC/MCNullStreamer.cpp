//===- lib/MC/MCNullStreamer.cpp - Dummy Streamer Implementation ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSectionMachO.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"

using namespace llvm;

namespace {

  class MCNullStreamer : public MCStreamer {
  public:
    MCNullStreamer(MCContext &Context) : MCStreamer(Context) {}

    /// @name MCStreamer Interface
    /// @{

    bool EmitSymbolAttribute(MCSymbol *Symbol,
                             MCSymbolAttr Attribute) override {
      return true;
    }

    void EmitCommonSymbol(MCSymbol *Symbol, uint64_t Size,
                          uint64_t ByteAlignment) override {}
    void EmitZerofill(MCSection *Section, MCSymbol *Symbol = nullptr,
                      uint64_t Size = 0, uint64_t ByteAlignment = 0) override {}
    void EmitGPRel32Value(const MCExpr *Value) override {}
    void BeginCOFFSymbolDef(const MCSymbol *Symbol) override {}
    void EmitCOFFSymbolStorageClass(int StorageClass) override {}
    void EmitCOFFSymbolType(int Type) override {}
    void EndCOFFSymbolDef() override {}
  };

}

MCStreamer *llvm::createNullStreamer(MCContext &Context) {
  return new MCNullStreamer(Context);
}
