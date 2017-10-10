//===--------- AVRMCELFStreamer.h - AVR subclass of MCElfStreamer ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AVR_MCTARGETDESC_AVRMCELFSTREAMER_H
#define LLVM_LIB_TARGET_AVR_MCTARGETDESC_AVRMCELFSTREAMER_H

#include "MCTargetDesc/AVRMCTargetDesc.h"
#include "MCTargetDesc/AVRMCExpr.h"
#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCInstrInfo.h"
#include <cstdint>
#include <memory>

namespace llvm {

class AVRMCELFStreamer : public MCELFStreamer {
  std::unique_ptr<MCInstrInfo> MCII;

public:
  AVRMCELFStreamer(MCContext &Context, MCAsmBackend &TAB,
                   raw_pwrite_stream &OS, MCCodeEmitter *Emitter)
      : MCELFStreamer(Context, TAB, OS, Emitter),
        MCII(createAVRMCInstrInfo()) {}

  AVRMCELFStreamer(MCContext &Context,
                   MCAsmBackend &TAB,
                   raw_pwrite_stream &OS, MCCodeEmitter *Emitter,
                   MCAssembler *Assembler) :
  MCELFStreamer(Context, TAB, OS, Emitter),
  MCII(createAVRMCInstrInfo()) {}

  void EmitValue(const MCExpr *Value, unsigned SizeInBytes,
                 AVRMCExpr::VariantKind ModifierKind, SMLoc Loc = SMLoc());
};

MCStreamer *createAVRELFStreamer(Triple const &TT, MCContext &Context,
                                 MCAsmBackend &MAB, raw_pwrite_stream &OS,
                                 MCCodeEmitter *CE);

} // end namespace llvm

#endif // LLVM_LIB_TARGET_AVR_MCTARGETDESC_AVRMCELFSTREAMER_H
