//===-- NVPTXLowerSharedFrameIndicesPass.cpp - NVPTX lowering  ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a copy of the generic LLVM PrologEpilogInserter pass, modified
// to remove unneeded functionality and to handle virtual registers. This pass
// lowers the frame indices to the shared framed index wherever needed.
//
//===----------------------------------------------------------------------===//

#include "NVPTX.h"
#include "NVPTXUtilities.h"
#include "NVPTXRegisterInfo.h"
#include "NVPTXSubtarget.h"
#include "NVPTXTargetMachine.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/MC/MachineLocation.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetFrameLowering.h"
#include "llvm/Target/TargetRegisterInfo.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#include "llvm/Target/TargetInstrInfo.h"

using namespace llvm;

#define DEBUG_TYPE "nvptx-lower-shared-frame-indices"

namespace {
class NVPTXLowerSharedFrameIndicesPass : public MachineFunctionPass {
public:
  static char ID;
  NVPTXLowerSharedFrameIndicesPass() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override;

private:
  void calculateSharedFrameObjectOffsets(MachineFunction &Fn);
};
}

MachineFunctionPass *llvm::createNVPTXLowerSharedFrameIndicesPass() {
  return new NVPTXLowerSharedFrameIndicesPass();
}

char NVPTXLowerSharedFrameIndicesPass::ID = 0;

static bool isSharedFrame(
      MachineBasicBlock::iterator II,
      MachineFunction &MF) {
  MachineInstr &currentMI = *II;

  if (!currentMI.getOperand(0).isReg())
    return false;;

  bool useSharedFrame = false;
  unsigned AllocRegisterNumber = currentMI.getOperand(0).getReg();

  for (MachineBasicBlock &MBB : MF) {
    for (MachineInstr &MI : MBB) {
      if (MI.getOpcode() == NVPTX::cvta_to_shared_yes_64 ||
          MI.getOpcode() == NVPTX::cvta_to_shared_yes) {
        if (AllocRegisterNumber == MI.getOperand(1).getReg()) {
          useSharedFrame = true;
          break;
        }
      }
    }
  }
  return useSharedFrame;
}

bool NVPTXLowerSharedFrameIndicesPass::runOnMachineFunction(MachineFunction &MF) {
  bool Modified = false;
  bool IsKernel = isKernelFunction(*MF.getFunction());

  SmallVector<int, 16> SharedFrameIndices;

  calculateSharedFrameObjectOffsets(MF);

  for (MachineBasicBlock &MBB : MF) {
    for (MachineInstr &MI : MBB) {
      for (unsigned i = 0, e = MI.getNumOperands(); i != e; ++i) {
        if (!MI.getOperand(i).isFI())
          continue;

        if (i + 1 >= MI.getNumOperands())
          continue;

        if (IsKernel) {
          bool IsSharedFrame = false;
          int FrameIndex = MI.getOperand(i).getIndex();

          for(int SFI : SharedFrameIndices)
            if (FrameIndex == SFI)
              IsSharedFrame = true;

          if (!IsSharedFrame && isSharedFrame(MI, MF)) {
            SharedFrameIndices.push_back(FrameIndex);
            IsSharedFrame = true;
          }

          if (IsSharedFrame) {
            // Change Frame index to use shared stack.
            MachineFunction &MF = *MI.getParent()->getParent();
            int Offset = MF.getFrameInfo().getObjectOffset(FrameIndex) +
                         MI.getOperand(i + 1).getImm();

            // Using I0 as the frame pointer
            // For shared data use the appropriate virtual register: VRShared
            MI.getOperand(i).ChangeToRegister(NVPTX::VRShared, false);
            MI.getOperand(i + 1).ChangeToImmediate(Offset);
          }
        }
        Modified = true;
      }
    }
  }

  return Modified;
}

/// AdjustStackOffset - Helper function used to adjust the stack frame offset.
static inline void
AdjustStackOffset(MachineFrameInfo &MFI, int FrameIdx,
                  bool StackGrowsDown, int64_t &Offset,
                  unsigned &MaxAlign) {
  // If the stack grows down, add the object size to find the lowest address.
  if (StackGrowsDown)
    Offset += MFI.getObjectSize(FrameIdx);

  unsigned Align = MFI.getObjectAlignment(FrameIdx);

  // If the alignment of this object is greater than that of the stack, then
  // increase the stack alignment to match.
  MaxAlign = std::max(MaxAlign, Align);

  // Adjust to alignment boundary.
  Offset = (Offset + Align - 1) / Align * Align;

  if (StackGrowsDown) {
    DEBUG(dbgs() << "alloc FI(" << FrameIdx << ") at SP[" << -Offset << "]\n");
    MFI.setObjectOffset(FrameIdx, -Offset); // Set the computed offset
  } else {
    DEBUG(dbgs() << "alloc FI(" << FrameIdx << ") at SP[" << Offset << "]\n");
    MFI.setObjectOffset(FrameIdx, Offset);
    Offset += MFI.getObjectSize(FrameIdx);
  }
}

/// This function computes the offset inside the shared stack.
///
/// TODO: For simplicity, currently, the offsets conincide with
/// the local stack frame offsets - the local and stack frame
/// offsets are the same length.
void
NVPTXLowerSharedFrameIndicesPass::calculateSharedFrameObjectOffsets(
      MachineFunction &Fn) {
  const TargetFrameLowering &TFI = *Fn.getSubtarget().getFrameLowering();
  const TargetRegisterInfo *RegInfo = Fn.getSubtarget().getRegisterInfo();

  bool StackGrowsDown =
    TFI.getStackGrowthDirection() == TargetFrameLowering::StackGrowsDown;

  // Loop over all of the stack objects, assigning sequential addresses...
  MachineFrameInfo &MFI = Fn.getFrameInfo();

  // Start at the beginning of the local area.
  // The Offset is the distance from the stack top in the direction
  // of stack growth -- so it's always nonnegative.
  int LocalAreaOffset = TFI.getOffsetOfLocalArea();
  if (StackGrowsDown)
    LocalAreaOffset = -LocalAreaOffset;
  assert(LocalAreaOffset >= 0
         && "Local area offset should be in direction of stack growth");
  int64_t Offset = LocalAreaOffset;

  // If there are fixed sized objects that are preallocated in the local area,
  // non-fixed objects can't be allocated right at the start of local area.
  // We currently don't support filling in holes in between fixed sized
  // objects, so we adjust 'Offset' to point to the end of last fixed sized
  // preallocated object.
  for (int i = MFI.getObjectIndexBegin(); i != 0; ++i) {
    int64_t FixedOff;
    if (StackGrowsDown) {
      // The maximum distance from the stack pointer is at lower address of
      // the object -- which is given by offset. For down growing stack
      // the offset is negative, so we negate the offset to get the distance.
      FixedOff = -MFI.getObjectOffset(i);
    } else {
      // The maximum distance from the start pointer is at the upper
      // address of the object.
      FixedOff = MFI.getObjectOffset(i) + MFI.getObjectSize(i);
    }
    if (FixedOff > Offset) Offset = FixedOff;
  }

  // NOTE: We do not have a call stack

  unsigned MaxAlign = MFI.getMaxAlignment();

  // No scavenger

  // FIXME: Once this is working, then enable flag will change to a target
  // check for whether the frame is large enough to want to use virtual
  // frame index registers. Functions which don't want/need this optimization
  // will continue to use the existing code path.
  if (MFI.getUseLocalStackAllocationBlock()) {
    unsigned Align = MFI.getLocalFrameMaxAlign();

    // Adjust to alignment boundary.
    Offset = (Offset + Align - 1) / Align * Align;

    DEBUG(dbgs() << "Local frame base offset: " << Offset << "\n");

    // Resolve offsets for objects in the local block.
    for (unsigned i = 0, e = MFI.getLocalFrameObjectCount(); i != e; ++i) {
      std::pair<int, int64_t> Entry = MFI.getLocalFrameObjectMap(i);
      int64_t FIOffset = (StackGrowsDown ? -Offset : Offset) + Entry.second;
      DEBUG(dbgs() << "alloc FI(" << Entry.first << ") at SP[" <<
            FIOffset << "]\n");
      MFI.setObjectOffset(Entry.first, FIOffset);
    }
    // Allocate the local block
    Offset += MFI.getLocalFrameSize();

    MaxAlign = std::max(Align, MaxAlign);
  }

  // No stack protector

  // Then assign frame offsets to stack objects that are not used to spill
  // callee saved registers.
  for (unsigned i = 0, e = MFI.getObjectIndexEnd(); i != e; ++i) {
    if (MFI.isObjectPreAllocated(i) &&
        MFI.getUseLocalStackAllocationBlock())
      continue;
    if (MFI.isDeadObjectIndex(i))
      continue;

    AdjustStackOffset(MFI, i, StackGrowsDown, Offset, MaxAlign);
  }

  // No scavenger

  if (!TFI.targetHandlesStackFrameRounding()) {
    // If we have reserved argument space for call sites in the function
    // immediately on entry to the current function, count it as part of the
    // overall stack size.
    if (MFI.adjustsStack() && TFI.hasReservedCallFrame(Fn))
      Offset += MFI.getMaxCallFrameSize();

    // Round up the size to a multiple of the alignment.  If the function has
    // any calls or alloca's, align to the target's StackAlignment value to
    // ensure that the callee's frame or the alloca data is suitably aligned;
    // otherwise, for leaf functions, align to the TransientStackAlignment
    // value.
    unsigned StackAlign;
    if (MFI.adjustsStack() || MFI.hasVarSizedObjects() ||
        (RegInfo->needsStackRealignment(Fn) && MFI.getObjectIndexEnd() != 0))
      StackAlign = TFI.getStackAlignment();
    else
      StackAlign = TFI.getTransientStackAlignment();

    // If the frame pointer is eliminated, all frame offsets will be relative to
    // SP not FP. Align to MaxAlign so this works.
    StackAlign = std::max(StackAlign, MaxAlign);
    unsigned AlignMask = StackAlign - 1;
    Offset = (Offset + AlignMask) & ~uint64_t(AlignMask);
  }

  // Update frame info to pretend that this is part of the stack...
  int64_t StackSize = Offset - LocalAreaOffset;
  MFI.setStackSize(StackSize);
}
