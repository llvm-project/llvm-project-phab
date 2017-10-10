//===--------- AVRMCELFStreamer.cpp - AVR subclass of MCELFStreamer -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a stub that parses a MCInst bundle and passes the
// instructions on to the real streamer.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "avrmcelfstreamer"

#include "MCTargetDesc/AVRMCELFStreamer.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCSymbol.h"

using namespace llvm;

void AVRMCELFStreamer::EmitValue(const MCSymbol *Sym, unsigned SizeInBytes,
                                 AVRMCExpr::VariantKind ModifierKind,
                                 SMLoc Loc) {
  MCSymbolRefExpr::VariantKind Kind = MCSymbolRefExpr::VK_AVR_NONE;
  if (ModifierKind == AVRMCExpr::VK_AVR_LO8)
    Kind = MCSymbolRefExpr::VK_AVR_LO8;
  else if (ModifierKind == AVRMCExpr::VK_AVR_HI8)
    Kind = MCSymbolRefExpr::VK_AVR_HI8;
  else if (ModifierKind == AVRMCExpr::VK_AVR_HH8)
    Kind = MCSymbolRefExpr::VK_AVR_HLO8;
  MCELFStreamer::EmitValue(MCSymbolRefExpr::create(Sym, Kind, getContext()),
                           SizeInBytes, Loc);
}


namespace llvm {
  MCStreamer *createAVRELFStreamer(Triple const &TT, MCContext &Context,
                                   MCAsmBackend &MAB,
                                   raw_pwrite_stream &OS, MCCodeEmitter *CE) {
    return new AVRMCELFStreamer(Context, MAB, OS, CE);
  }

} // end namespace llvm
