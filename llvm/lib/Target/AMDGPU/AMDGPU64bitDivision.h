#include "AMDGPUISelLowering.h"
#include "AMDGPU.h"
#include "AMDGPUDiagnosticInfoUnsupported.h"
#include "AMDGPUFrameLowering.h"
#include "AMDGPUIntrinsicInfo.h"
#include "AMDGPURegisterInfo.h"
#include "AMDGPUSubtarget.h"
#include "R600MachineFunctionInfo.h"
#include "SIMachineFunctionInfo.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"

namespace llvm
{
	class BinaryOperator;
}

namespace llvm{



bool AMDExpandUDivision(BinaryOperator *Div);

}