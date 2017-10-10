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
#include "llvm/ADT/StringRef.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCObjectStreamer.h"
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCSymbolELF.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include <cassert>
#include <cstdint>

using namespace llvm;

void AVRMCELFStreamer::EmitValue(const MCExpr *Value, unsigned SizeInBytes,
                                 AVRMCExpr::VariantKind ModifierKind,
                                 SMLoc Loc) {
  MCSymbol *Sym = getContext().createTempSymbol();
  if (!Sym)
    return;
  VariantKind Kind = MCSymbolRefExpr::VK_AVR_NONE;
  if (ModifierKind == AVRMCExpr::VK_AVR_LO8)
    Kind = MCSymbolRefExpr::VK_AVR_LO8;
  else if (ModifierKind == AVRMCExpr::VK_AVR_HI8)
    Kind = MCSymbolRefExpr::VK_AVR_HI8;
  else if (ModifierKind == AVRMCExpr::VK_AVR_HH8)
    Kind = MCSymbolRefExpr::VK_AVR_HLO8;
  const MCSymbolRefExpr *Ref =
      MCSymbolRefExpr::create(Sym, Kind, getContext());
  EmitValue(Ref, SizeInBytes);
}


namespace llvm {
  MCStreamer *createAVRELFStreamer(Triple const &TT, MCContext &Context,
                                   MCAsmBackend &MAB,
                                   raw_pwrite_stream &OS, MCCodeEmitter *CE) {
    return new AVRMCELFStreamer(Context, MAB, OS, CE);
  }

} // end namespace llvm
