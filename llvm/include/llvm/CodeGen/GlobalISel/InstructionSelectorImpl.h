//==-- llvm/CodeGen/GlobalISel/InstructionSelectorImpl.h ---------*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file This file declares the API for the instruction selector.
/// This class is responsible for selecting machine instructions.
/// It's implemented by the target. It's used by the InstructionSelect pass.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CODEGEN_GLOBALISEL_INSTRUCTIONSELECTORIMPL_H
#define LLVM_CODEGEN_GLOBALISEL_INSTRUCTIONSELECTORIMPL_H

namespace llvm {
template <class TgtInstructionSelector, class PredicateBitset,
          class ComplexMatcherMemFn>
bool InstructionSelector::executeMatchTable(
    TgtInstructionSelector &ISel, RecordedMIVector &MIs,
    RecordedMIVector &MIStack, std::vector<ComplexRendererFn> &Renderers,
    const LLT TypeObjects[], const PredicateBitset FeatureBitsets[],
    const std::vector<ComplexMatcherMemFn> &ComplexPredicates,
    const int64_t *MatchTable, MachineRegisterInfo &MRI,
    const TargetRegisterInfo &TRI, const RegisterBankInfo &RBI,
    const PredicateBitset &AvailableFeatures) const {
  const int64_t *Command = MatchTable;
  while (true) {
    switch (*Command++) {
    case GIM_RecordInsn: {
      int64_t NewInsnID = *Command++;
      assert((size_t)NewInsnID == MIs.size() && "Expected to store MIs in order");
      MIs.push_back(MIStack.back());
      DEBUG(dbgs() << "MIs[" << NewInsnID << "] = GIM_RecordInsn()\n");
      break;
    }
    case GIM_PushInsnOpDef: {
      int64_t InsnID = *Command++;
      int64_t OpIdx = *Command++;
      DEBUG(dbgs() << "GIM_PushInsnOpDef(" << InsnID << ", " << OpIdx << ")\n");
      if (!MIs[InsnID]->getOperand(OpIdx).isReg()) {
        DEBUG(dbgs() << "Rejected (not a register)\n");
        return false;
      }
      if (TRI.isPhysicalRegister(MIs[InsnID]->getOperand(OpIdx).getReg())) {
        DEBUG(dbgs() << "Rejected (is a physical register)\n");
        return false;
      }
      MIStack.push_back(
          MRI.getVRegDef(MIs[InsnID]->getOperand(OpIdx).getReg()));
      break;
    }
    case GIM_PopInsn: {
      DEBUG(dbgs() << "GIM_PopInsn()\n");
      MIStack.pop_back();
      break;
    }

    case GIM_CheckFeatures: {
      int64_t ExpectedBitsetID = *Command++;
      DEBUG(dbgs() << "GIM_CheckFeatures(ExpectedBitsetID=" << ExpectedBitsetID
                   << ")\n");
      if ((AvailableFeatures & FeatureBitsets[ExpectedBitsetID]) !=
          FeatureBitsets[ExpectedBitsetID]) {
        DEBUG(dbgs() << "Rejected\n");
        return false;
      }
      break;
    }

    case GIM_CheckOpcode: {
      int64_t InsnID = *Command++;
      int64_t Expected = *Command++;
      DEBUG(dbgs() << "GIM_CheckOpcode(MIs[" << InsnID
                   << "], ExpectedOpcode=" << Expected << ")\n");
      assert(MIs[InsnID] != nullptr && "Used insn before defined");
      if (MIs[InsnID]->getOpcode() != Expected)
        return false;
      break;
    }
    case GIM_CheckNumOperands: {
      int64_t InsnID = *Command++;
      int64_t Expected = *Command++;
      DEBUG(dbgs() << "GIM_CheckNumOperands(MIs[" << InsnID
                   << "], Expected=" << Expected << ")\n");
      assert(MIs[InsnID] != nullptr && "Used insn before defined");
      if (MIs[InsnID]->getNumOperands() != Expected)
        return false;
      break;
    }

    case GIM_CheckType: {
      int64_t InsnID = *Command++;
      int64_t OpIdx = *Command++;
      int64_t TypeID = *Command++;
      DEBUG(dbgs() << "GIM_CheckType(MIs[" << InsnID << "]->getOperand("
                   << OpIdx << "), TypeID=" << TypeID << ")\n");
      assert(MIs[InsnID] != nullptr && "Used insn before defined");
      if (MRI.getType(MIs[InsnID]->getOperand(OpIdx).getReg()) !=
          TypeObjects[TypeID])
        return false;
      break;
    }
    case GIM_CheckRegBankForClass: {
      int64_t InsnID = *Command++;
      int64_t OpIdx = *Command++;
      int64_t RCEnum = *Command++;
      DEBUG(dbgs() << "GIM_CheckRegBankForClass(MIs[" << InsnID
                   << "]->getOperand(" << OpIdx << "), RCEnum=" << RCEnum
                   << ")\n");
      assert(MIs[InsnID] != nullptr && "Used insn before defined");
      if (&RBI.getRegBankFromRegClass(*TRI.getRegClass(RCEnum)) !=
          RBI.getRegBank(MIs[InsnID]->getOperand(OpIdx).getReg(), MRI, TRI))
        return false;
      break;
    }
    case GIM_CheckComplexPattern: {
      int64_t InsnID = *Command++;
      int64_t OpIdx = *Command++;
      int64_t RendererID = *Command++;
      int64_t ComplexPredicateID = *Command++;
      DEBUG(dbgs() << "Renderers[" << RendererID
                   << "] = GIM_CheckComplexPattern(MIs[" << InsnID
                   << "]->getOperand(" << OpIdx
                   << "), ComplexPredicateID=" << ComplexPredicateID << ")\n");
      assert(MIs[InsnID] != nullptr && "Used insn before defined");
      // FIXME: Use std::invoke() when it's available.
      if (!(Renderers[RendererID] =
                (ISel.*ComplexPredicates[ComplexPredicateID])(
                    MIs[InsnID]->getOperand(OpIdx))))
        return false;
      break;
    }
    case GIM_CheckInt: {
      int64_t InsnID = *Command++;
      int64_t OpIdx = *Command++;
      int64_t Value = *Command++;
      DEBUG(dbgs() << "GIM_CheckInt(MIs[" << InsnID << "]->getOperand(" << OpIdx
                   << "), Value=" << Value << ")\n");
      assert(MIs[InsnID] != nullptr && "Used insn before defined");
      if (!isOperandImmEqual(MIs[InsnID]->getOperand(OpIdx), Value, MRI))
        return false;
      break;
    }
    case GIM_CheckIsMBB: {
      int64_t InsnID = *Command++;
      int64_t OpIdx = *Command++;
      DEBUG(dbgs() << "GIM_CheckIsMBB(MIs[" << InsnID << "]->getOperand("
                   << OpIdx << "))\n");
      assert(MIs[InsnID] != nullptr && "Used insn before defined");
      if (!MIs[InsnID]->getOperand(OpIdx).isMBB())
        return false;
      break;
    }

    case GIM_Accept:
      DEBUG(dbgs() << "GIM_Accept");
      return true;
    default:
      llvm_unreachable("Unexpected command");
    }
  }
}
} // end namespace llvm

#endif // LLVM_CODEGEN_GLOBALISEL_INSTRUCTIONSELECTORIMPL_H
