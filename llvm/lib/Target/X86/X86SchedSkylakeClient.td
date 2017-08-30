//=- X86SchedSkylake.td - X86 Skylake Client Scheduling ------*- tablegen -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the machine model for Skylake Client to support
// instruction scheduling and other instruction cost heuristics.
//
//===----------------------------------------------------------------------===//

def SkylakeClientModel : SchedMachineModel {
  // All x86 instructions are modeled as a single micro-op, and SKylake can
  // decode 6 instructions per cycle.
  let IssueWidth = 6;
  let MicroOpBufferSize = 224; // Based on the reorder buffer.
  let LoadLatency = 5;
  let MispredictPenalty = 14;
  
  // Based on the LSD (loop-stream detector) queue size and benchmarking data.
  let LoopMicroOpBufferSize = 50;

  // This flag is set to allow the scheduler to assign a default model to
  // unrecognized opcodes.
  let CompleteModel = 0;
}

let SchedModel = SkylakeClientModel in {

// Skylake Client can issue micro-ops to 8 different ports in one cycle.

// Ports 0, 1, 5, and 6 handle all computation.
// Port 4 gets the data half of stores. Store data can be available later than
// the store address, but since we don't model the latency of stores, we can
// ignore that.
// Ports 2 and 3 are identical. They handle loads and the address half of
// stores. Port 7 can handle address calculations.
def SKLPort0 : ProcResource<1>;
def SKLPort1 : ProcResource<1>;
def SKLPort2 : ProcResource<1>;
def SKLPort3 : ProcResource<1>;
def SKLPort4 : ProcResource<1>;
def SKLPort5 : ProcResource<1>;
def SKLPort6 : ProcResource<1>;
def SKLPort7 : ProcResource<1>;

// Many micro-ops are capable of issuing on multiple ports.
def SKLPort01  : ProcResGroup<[SKLPort0, SKLPort1]>;
def SKLPort23  : ProcResGroup<[SKLPort2, SKLPort3]>;
def SKLPort237 : ProcResGroup<[SKLPort2, SKLPort3, SKLPort7]>;
def SKLPort04  : ProcResGroup<[SKLPort0, SKLPort4]>;
def SKLPort05  : ProcResGroup<[SKLPort0, SKLPort5]>;
def SKLPort06  : ProcResGroup<[SKLPort0, SKLPort6]>;
def SKLPort15  : ProcResGroup<[SKLPort1, SKLPort5]>;
def SKLPort16  : ProcResGroup<[SKLPort1, SKLPort6]>;
def SKLPort56  : ProcResGroup<[SKLPort5, SKLPort6]>;
def SKLPort015 : ProcResGroup<[SKLPort0, SKLPort1, SKLPort5]>;
def SKLPort056 : ProcResGroup<[SKLPort0, SKLPort5, SKLPort6]>;
def SKLPort0156: ProcResGroup<[SKLPort0, SKLPort1, SKLPort5, SKLPort6]>;

// 60 Entry Unified Scheduler
def SKLPortAny : ProcResGroup<[SKLPort0, SKLPort1, SKLPort2, SKLPort3, SKLPort4,
                              SKLPort5, SKLPort6, SKLPort7]> {
  let BufferSize=60;
}

// Loads are 4 cycles, so ReadAfterLd registers needn't be available until 4
// cycles after the memory operand.
def : ReadAdvance<ReadAfterLd, 4>;

// Many SchedWrites are defined in pairs with and without a folded load.
// Instructions with folded loads are usually micro-fused, so they only appear
// as two micro-ops when queued in the reservation station.
// This multiclass defines the resource usage for variants with and without
// folded loads.
multiclass SKLWriteResPair<X86FoldableSchedWrite SchedRW,
                          ProcResourceKind ExePort,
                          int Lat> {
  // Register variant is using a single cycle on ExePort.
  def : WriteRes<SchedRW, [ExePort]> { let Latency = Lat; }

  // Memory variant also uses a cycle on port 2/3 and adds 4 cycles to the
  // latency.
  def : WriteRes<SchedRW.Folded, [SKLPort23, ExePort]> {
     let Latency = !add(Lat, 4);
  }
}

// A folded store needs a cycle on port 4 for the store data, but it does not
// need an extra port 2/3 cycle to recompute the address.
def : WriteRes<WriteRMW, [SKLPort4]>;

// Arithmetic.
defm : SKLWriteResPair<WriteALU,   SKLPort0156, 1>; // Simple integer ALU op.
defm : SKLWriteResPair<WriteIMul,  SKLPort1,   3>; // Integer multiplication.
def : WriteRes<WriteIMulH, []> { let Latency = 3; } // Integer multiplication, high part.
def SKLDivider : ProcResource<1>; // Integer division issued on port 0.     
def : WriteRes<WriteIDiv, [SKLPort0, SKLDivider]> { // Integer division.
  let Latency = 25;
  let ResourceCycles = [1, 10];
}
def : WriteRes<WriteIDivLd, [SKLPort23, SKLPort0, SKLDivider]> {
  let Latency = 29;
  let ResourceCycles = [1, 1, 10];
}

def : WriteRes<WriteLEA, [SKLPort15]>; // LEA instructions can't fold loads.

// Integer shifts and rotates.
defm : SKLWriteResPair<WriteShift, SKLPort06,  1>;

// Loads, stores, and moves, not folded with other operations.
def : WriteRes<WriteLoad,  [SKLPort23]> { let Latency = 4; }
def : WriteRes<WriteStore, [SKLPort237, SKLPort4]>;
def : WriteRes<WriteMove,  [SKLPort0156]>;

// Idioms that clear a register, like xorps %xmm0, %xmm0.
// These can often bypass execution ports completely.
def : WriteRes<WriteZero,  []>;

// Branches don't produce values, so they have no latency, but they still
// consume resources. Indirect branches can fold loads.
defm : SKLWriteResPair<WriteJump,  SKLPort06,   1>;

// Floating point. This covers both scalar and vector operations.
defm : SKLWriteResPair<WriteFAdd,   SKLPort1, 3>; // Floating point add/sub/compare.
defm : SKLWriteResPair<WriteFMul,   SKLPort0, 5>; // Floating point multiplication.
defm : SKLWriteResPair<WriteFDiv,   SKLPort0, 12>; // 10-14 cycles. // Floating point division.
defm : SKLWriteResPair<WriteFSqrt,  SKLPort0, 15>; // Floating point square root.
defm : SKLWriteResPair<WriteFRcp,   SKLPort0, 5>; // Floating point reciprocal estimate.
defm : SKLWriteResPair<WriteFRsqrt, SKLPort0, 5>; // Floating point reciprocal square root estimate.
// defm WriteFMA    : X86SchedWritePair; // Fused Multiply Add.
defm : SKLWriteResPair<WriteFShuffle,  SKLPort5,  1>; // Floating point vector shuffles.
defm : SKLWriteResPair<WriteFBlend,  SKLPort015,  1>; // Floating point vector blends.
def : WriteRes<WriteFVarBlend, [SKLPort5]> { // Fp vector variable blends.	       
  let Latency = 2;
  let ResourceCycles = [2];
} 
def : WriteRes<WriteFVarBlendLd, [SKLPort5, SKLPort23]> {
  let Latency = 6;
  let ResourceCycles = [2, 1];
}

// FMA Scheduling helper class.
// class FMASC { X86FoldableSchedWrite Sched = WriteFAdd; }

// Vector integer operations.
defm : SKLWriteResPair<WriteVecALU,   SKLPort15,  1>; // Vector integer ALU op, no logicals.
defm : SKLWriteResPair<WriteVecShift, SKLPort0,  1>; // Vector integer shifts.
defm : SKLWriteResPair<WriteVecIMul,  SKLPort0,   5>; // Vector integer multiply.
defm : SKLWriteResPair<WriteShuffle,  SKLPort5,  1>; // Vector shuffles.
defm : SKLWriteResPair<WriteBlend,  SKLPort15,  1>; // Vector blends.

def : WriteRes<WriteVarBlend, [SKLPort5]> { // Vector variable blends.
  let Latency = 2;
  let ResourceCycles = [2];
}
def : WriteRes<WriteVarBlendLd, [SKLPort5, SKLPort23]> {
  let Latency = 6;
  let ResourceCycles = [2, 1];
}

def : WriteRes<WriteMPSAD, [SKLPort0, SKLPort5]> { // Vector MPSAD.     
  let Latency = 6;
  let ResourceCycles = [1, 2];
}
def : WriteRes<WriteMPSADLd, [SKLPort23, SKLPort0, SKLPort5]> {
  let Latency = 6;
  let ResourceCycles = [1, 1, 2];
}

// Vector bitwise operations.
// These are often used on both floating point and integer vectors.
defm : SKLWriteResPair<WriteVecLogic, SKLPort015, 1>; // Vector and/or/xor.

// Conversion between integer and float.
defm : SKLWriteResPair<WriteCvtF2I, SKLPort1, 3>; // Float -> Integer.
defm : SKLWriteResPair<WriteCvtI2F, SKLPort1, 4>; // Integer -> Float.
defm : SKLWriteResPair<WriteCvtF2F, SKLPort1, 3>; // Float -> Float size conversion.

// Strings instructions.
// Packed Compare Implicit Length Strings, Return Mask
// String instructions.
def : WriteRes<WritePCmpIStrM, [SKLPort0]> {
  let Latency = 10;
  let ResourceCycles = [3];
}
def : WriteRes<WritePCmpIStrMLd, [SKLPort0, SKLPort23]> {
  let Latency = 10;
  let ResourceCycles = [3, 1];
} 
// Packed Compare Explicit Length Strings, Return Mask
def : WriteRes<WritePCmpEStrM, [SKLPort0, SKLPort16, SKLPort5]> {
  let Latency = 10;
  let ResourceCycles = [3, 2, 4];
}
def : WriteRes<WritePCmpEStrMLd, [SKLPort05, SKLPort16, SKLPort23]> {
  let Latency = 10;
  let ResourceCycles = [6, 2, 1];
} 
  // Packed Compare Implicit Length Strings, Return Index
def : WriteRes<WritePCmpIStrI, [SKLPort0]> {
  let Latency = 11;
  let ResourceCycles = [3];
}
def : WriteRes<WritePCmpIStrILd, [SKLPort0, SKLPort23]> {
  let Latency = 11;
  let ResourceCycles = [3, 1];
}     
// Packed Compare Explicit Length Strings, Return Index
def : WriteRes<WritePCmpEStrI, [SKLPort05, SKLPort16]> {
  let Latency = 11;
  let ResourceCycles = [6, 2];
}
def : WriteRes<WritePCmpEStrILd, [SKLPort0, SKLPort16, SKLPort5, SKLPort23]> {
  let Latency = 11;
  let ResourceCycles = [3, 2, 2, 1];
}

// AES instructions.
def : WriteRes<WriteAESDecEnc, [SKLPort5]> { // Decryption, encryption.
  let Latency = 7;
  let ResourceCycles = [1];
}
def : WriteRes<WriteAESDecEncLd, [SKLPort5, SKLPort23]> {
  let Latency = 7;
  let ResourceCycles = [1, 1];
}
def : WriteRes<WriteAESIMC, [SKLPort5]> { // InvMixColumn.
  let Latency = 14;
  let ResourceCycles = [2];
}
def : WriteRes<WriteAESIMCLd, [SKLPort5, SKLPort23]> {
  let Latency = 14;
  let ResourceCycles = [2, 1];
}
def : WriteRes<WriteAESKeyGen, [SKLPort0, SKLPort5]> { // Key Generation.
  let Latency = 10;
  let ResourceCycles = [2, 8];
}
def : WriteRes<WriteAESKeyGenLd, [SKLPort0, SKLPort5, SKLPort23]> {
  let Latency = 10;
  let ResourceCycles = [2, 7, 1];
}

// Carry-less multiplication instructions.
def : WriteRes<WriteCLMul, [SKLPort0, SKLPort5]> {
  let Latency = 7;
  let ResourceCycles = [2, 1];
}
def : WriteRes<WriteCLMulLd, [SKLPort0, SKLPort5, SKLPort23]> {
  let Latency = 7;
  let ResourceCycles = [2, 1, 1];
}

// Catch-all for expensive system instructions.
def : WriteRes<WriteSystem,     [SKLPort0156]> { let Latency = 100; } // def WriteSystem : SchedWrite;

// AVX2.
defm : SKLWriteResPair<WriteFShuffle256,  SKLPort5,  3>; // Fp 256-bit width vector shuffles.
defm : SKLWriteResPair<WriteShuffle256,  SKLPort5,  3>;  // 256-bit width vector shuffles.
def : WriteRes<WriteVarVecShift, [SKLPort0, SKLPort5]> { // Variable vector shifts.
  let Latency = 2;
  let ResourceCycles = [2, 1];
}
def : WriteRes<WriteVarVecShiftLd, [SKLPort0, SKLPort5, SKLPort23]> {
  let Latency = 6;
  let ResourceCycles = [2, 1, 1];
}

// Old microcoded instructions that nobody use.
def : WriteRes<WriteMicrocoded, [SKLPort0156]> { let Latency = 100; } // def WriteMicrocoded : SchedWrite;

// Fence instructions.
def : WriteRes<WriteFence,  [SKLPort23, SKLPort4]>;

// Nop, not very useful expect it provides a model for nops!
def : WriteRes<WriteNop, []>;

////////////////////////////////////////////////////////////////////////////////
// Horizontal add/sub  instructions.
////////////////////////////////////////////////////////////////////////////////
// HADD, HSUB PS/PD
// x,x / v,v,v.
def : WriteRes<WriteFHAdd, [SKLPort1]> {
  let Latency = 3;
}

// x,m / v,v,m.
def : WriteRes<WriteFHAddLd, [SKLPort1, SKLPort23]> {
  let Latency = 7;
  let ResourceCycles = [1, 1];
}

// PHADD|PHSUB (S) W/D.
// v <- v,v.
def : WriteRes<WritePHAdd, [SKLPort15]>;

// v <- v,m.
def : WriteRes<WritePHAddLd, [SKLPort15, SKLPort23]> {
  let Latency = 5;
  let ResourceCycles = [1, 1];
}

// Remaining instrs.

def SKLWriteResGroup0 : SchedWriteRes<[SKLPort23]> {
  let Latency = 1;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup0], (instregex "LDDQUrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "LD_F32m")>;
def: InstRW<[SKLWriteResGroup0], (instregex "LD_F64m")>;
def: InstRW<[SKLWriteResGroup0], (instregex "LD_F80m")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MMX_MOVD64from64rm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MMX_MOVD64rm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MMX_MOVD64to64rm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MMX_MOVQ64rm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MOV(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MOV64toPQIrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MOV8rm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MOVAPDrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MOVAPSrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MOVDDUPrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MOVDI2PDIrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MOVDQArm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MOVDQUrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MOVNTDQArm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MOVSHDUPrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MOVSLDUPrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MOVSSrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MOVSX(16|32|64)rm16")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MOVSX(16|32|64)rm32")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MOVSX(16|32|64)rm8")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MOVUPDrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MOVUPSrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MOVZX(16|32|64)rm16")>;
def: InstRW<[SKLWriteResGroup0], (instregex "MOVZX(16|32|64)rm8")>;
def: InstRW<[SKLWriteResGroup0], (instregex "PREFETCHNTA")>;
def: InstRW<[SKLWriteResGroup0], (instregex "PREFETCHT0")>;
def: InstRW<[SKLWriteResGroup0], (instregex "PREFETCHT1")>;
def: InstRW<[SKLWriteResGroup0], (instregex "PREFETCHT2")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTF128")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTF32X2Z256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTF32X4Z256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTF32X4rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTF32X8rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTF64X2Z128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTF64X2rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTF64X4rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTI128")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTI32X2Z128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTI32X2Z256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTI32X4Z256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTI32X4rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTI32X8rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTI64X2Z128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTI64X2rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTI64X4rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTSDYrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTSDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTSSYrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTSSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTSSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VBROADCASTSSrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VLDDQUYrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VLDDQUrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOV64toPQIrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVAPDYrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVAPDrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVAPSYrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVAPSrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVDDUPYrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVDDUPZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVDDUPZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVDDUPrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVDI2PDIrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVDQAYrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVDQArm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVDQUYrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVDQUrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVNTDQAYrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVNTDQAZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVNTDQAZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVNTDQArm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVQI2PQIrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVSDrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVSHDUPYrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVSHDUPZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVSHDUPZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVSHDUPrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVSLDUPYrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVSLDUPZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVSLDUPZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVSLDUPrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVSSrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVUPDYrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVUPDrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVUPSYrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VMOVUPSrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VPBROADCASTDYrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VPBROADCASTDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VPBROADCASTDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VPBROADCASTDrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VPBROADCASTQYrm")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VPBROADCASTQZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VPBROADCASTQZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup0], (instregex "VPBROADCASTQrm")>;

def SKLWriteResGroup1 : SchedWriteRes<[SKLPort4,SKLPort237]> {
  let Latency = 1;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup1], (instregex "FBSTPm")>;
def: InstRW<[SKLWriteResGroup1], (instregex "KMOVBmk")>;
def: InstRW<[SKLWriteResGroup1], (instregex "KMOVDmk")>;
def: InstRW<[SKLWriteResGroup1], (instregex "KMOVQmk")>;
def: InstRW<[SKLWriteResGroup1], (instregex "KMOVWmk")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MMX_MOVD64from64rm")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MMX_MOVD64mr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MMX_MOVNTQmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MMX_MOVQ64mr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOV(16|32|64)mr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOV8mi")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOV8mr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOVAPDmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOVAPSmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOVDQAmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOVDQUmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOVHPDmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOVHPSmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOVLPDmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOVLPSmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOVNTDQmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOVNTI_64mr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOVNTImr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOVNTPDmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOVNTPSmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOVPDI2DImr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOVPQI2QImr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOVPQIto64mr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOVSSmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOVUPDmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "MOVUPSmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "ST_FP32m")>;
def: InstRW<[SKLWriteResGroup1], (instregex "ST_FP64m")>;
def: InstRW<[SKLWriteResGroup1], (instregex "ST_FP80m")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VEXTRACTF128mr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VEXTRACTF32x4Z256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VEXTRACTF64x2Z256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VEXTRACTI128mr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VEXTRACTI32x4Z256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VEXTRACTI64x2Z256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVAPDYmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVAPDZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVAPDZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVAPDmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVAPSYmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVAPSZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVAPSZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVAPSmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVDQA32Z128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVDQA32Z256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVDQA64Z128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVDQA64Z256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVDQAYmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVDQAmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVDQU16Z128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVDQU16Z256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVDQU32Z128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVDQU32Z256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVDQU64Z128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVDQU64Z256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVDQU8Z128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVDQU8Z256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVDQUYmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVDQUmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVHPDZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVHPDmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVHPSZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVHPSmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVLPDZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVLPDmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVLPSZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVLPSmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVNTDQYmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVNTDQZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVNTDQZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVNTDQmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVNTPDYmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVNTPDZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVNTPDZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVNTPDmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVNTPSYmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVNTPSZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVNTPSZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVNTPSmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVPDI2DImr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVPQI2QImr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVPQIto64mr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVSDmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVSSmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVUPDYmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVUPDZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVUPDZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVUPDmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVUPSYmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVUPSZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVUPSZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMOVUPSmr")>;
def: InstRW<[SKLWriteResGroup1], (instregex "VMPTRSTm")>;

def SKLWriteResGroup2 : SchedWriteRes<[SKLPort0]> {
  let Latency = 1;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup2], (instregex "KANDBrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KANDDrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KANDNBrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KANDNDrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KANDNQrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KANDNWrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KANDQrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KANDWrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KMOVBkk")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KMOVDkk")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KMOVQkk")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KMOVWkk")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KNOTBrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KNOTDrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KNOTQrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KNOTWrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KORBrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KORDrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KORQrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KORWrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KXNORBrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KXNORDrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KXNORQrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KXNORWrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KXORBrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KXORDrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KXORQrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "KXORWrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PADDSBirr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PADDSWirr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PADDUSBirr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PADDUSWirr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PAVGBirr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PAVGWirr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PCMPEQBirr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PCMPEQDirr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PCMPEQWirr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PCMPGTBirr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PCMPGTDirr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PCMPGTWirr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PMAXSWirr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PMAXUBirr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PMINSWirr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PMINUBirr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PSLLDri")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PSLLDrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PSLLQri")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PSLLQrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PSLLWri")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PSLLWrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PSRADri")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PSRADrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PSRAWri")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PSRAWrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PSRLDri")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PSRLDrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PSRLQri")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PSRLQrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PSRLWri")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PSRLWrr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PSUBSBirr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PSUBSWirr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PSUBUSBirr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "MMX_PSUBUSWirr")>;
def: InstRW<[SKLWriteResGroup2], (instregex "VPMOVB2MZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup2], (instregex "VPMOVB2MZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup2], (instregex "VPMOVD2MZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup2], (instregex "VPMOVD2MZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup2], (instregex "VPMOVQ2MZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup2], (instregex "VPMOVQ2MZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup2], (instregex "VPMOVW2MZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup2], (instregex "VPMOVW2MZ256rr(b?)(k?)(z?)")>;

def SKLWriteResGroup3 : SchedWriteRes<[SKLPort1]> {
  let Latency = 1;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup3], (instregex "MMX_MASKMOVQ64")>;

def SKLWriteResGroup4 : SchedWriteRes<[SKLPort5]> {
  let Latency = 1;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup4], (instregex "COMP_FST0r")>;
def: InstRW<[SKLWriteResGroup4], (instregex "COM_FST0r")>;
def: InstRW<[SKLWriteResGroup4], (instregex "INSERTPSrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "KMOVBkr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "KMOVDkr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "KMOVQkr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "KMOVWkr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "MMX_MOVD64rr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "MMX_MOVD64to64rr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "MMX_PALIGNR64irr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "MMX_PSHUFBrr64")>;
def: InstRW<[SKLWriteResGroup4], (instregex "MMX_PSHUFWri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "MMX_PUNPCKHBWirr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "MMX_PUNPCKHDQirr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "MMX_PUNPCKHWDirr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "MMX_PUNPCKLBWirr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "MMX_PUNPCKLDQirr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "MMX_PUNPCKLWDirr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "MOV64toPQIrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "MOVDDUPrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "MOVDI2PDIrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "MOVHLPSrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "MOVLHPSrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "MOVSDrr(_REV?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "MOVSHDUPrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "MOVSLDUPrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "MOVUPDrr(_REV?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "MOVUPSrr(_REV?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PACKSSDWrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PACKSSWBrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PACKUSDWrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PACKUSWBrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PALIGNRrri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PBLENDWrri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PMOVSXBDrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PMOVSXBQrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PMOVSXBWrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PMOVSXDQrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PMOVSXWDrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PMOVSXWQrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PMOVZXBDrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PMOVZXBQrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PMOVZXBWrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PMOVZXDQrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PMOVZXWDrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PMOVZXWQrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PSHUFBrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PSHUFDri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PSHUFHWri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PSHUFLWri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PSLLDQri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PSRLDQri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PUNPCKHBWrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PUNPCKHDQrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PUNPCKHQDQrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PUNPCKHWDrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PUNPCKLBWrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PUNPCKLDQrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PUNPCKLQDQrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "PUNPCKLWDrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "SHUFPDrri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "SHUFPSrri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "UCOM_FPr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "UCOM_Fr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "UNPCKHPDrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "UNPCKHPSrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "UNPCKLPDrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "UNPCKLPSrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VBROADCASTI32X2Z128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VBROADCASTSSrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VINSERTPSrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VMOV64toPQIrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VMOVDDUPYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VMOVDDUPZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VMOVDDUPZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VMOVDDUPrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VMOVDI2PDIrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VMOVHLPSrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VMOVLHPSrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VMOVSDrr(_REV?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VMOVSHDUPYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VMOVSHDUPZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VMOVSHDUPZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VMOVSHDUPrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VMOVSLDUPYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VMOVSLDUPZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VMOVSLDUPZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VMOVSLDUPrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VMOVUPDYrr(_REV?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VMOVUPDrr(_REV?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VMOVUPSYrr(_REV?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VMOVUPSrr(_REV?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPACKSSDWYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPACKSSDWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPACKSSDWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPACKSSDWrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPACKSSWBYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPACKSSWBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPACKSSWBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPACKSSWBrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPACKUSDWYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPACKUSDWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPACKUSDWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPACKUSDWrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPACKUSWBYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPACKUSWBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPACKUSWBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPACKUSWBrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPALIGNRYrri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPALIGNRZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPALIGNRZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPALIGNRrri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPBLENDWYrri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPBLENDWrri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPBROADCASTDrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPBROADCASTQrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPERMILPDYri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPERMILPDYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPERMILPDZ128r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPERMILPDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPERMILPDZ256r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPERMILPDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPERMILPDri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPERMILPDrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPERMILPSYri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPERMILPSYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPERMILPSZ128r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPERMILPSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPERMILPSZ256r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPERMILPSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPERMILPSri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPERMILPSrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPMOVSXBDrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPMOVSXBQrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPMOVSXBWrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPMOVSXDQrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPMOVSXWDrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPMOVSXWQrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPMOVZXBDrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPMOVZXBQrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPMOVZXBWrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPMOVZXDQrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPMOVZXWDrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPMOVZXWQrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSHUFBYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSHUFBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSHUFBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSHUFBrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSHUFDYri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSHUFDZ128r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSHUFDZ256r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSHUFDri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSHUFHWYri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSHUFHWZ128r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSHUFHWZ128r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSHUFHWri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSHUFLWYri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSHUFLWZ128r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSHUFLWZ128r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSHUFLWri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSLLDQYri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSLLDQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSLLDQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSLLDQri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSRLDQYri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSRLDQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSRLDQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPSRLDQri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKHBWYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKHBWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKHBWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKHBWrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKHDQYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKHDQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKHDQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKHDQrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKHQDQYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKHQDQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKHQDQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKHQDQrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKHWDYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKHWDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKHWDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKHWDrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKLBWYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKLBWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKLBWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKLBWrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKLDQYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKLDQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKLDQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKLDQrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKLQDQYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKLQDQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKLQDQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKLQDQrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKLWDYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKLWDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKLWDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VPUNPCKLWDrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VSHUFPDYrri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VSHUFPDZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VSHUFPDZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VSHUFPDrri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VSHUFPSYrri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VSHUFPSZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VSHUFPSZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VSHUFPSrri")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VUNPCKHPDYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VUNPCKHPDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VUNPCKHPDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VUNPCKHPDrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VUNPCKHPSYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VUNPCKHPSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VUNPCKHPSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VUNPCKHPSrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VUNPCKLPDYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VUNPCKLPDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VUNPCKLPDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VUNPCKLPDrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VUNPCKLPSYrr")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VUNPCKLPSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VUNPCKLPSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup4], (instregex "VUNPCKLPSrr")>;

def SKLWriteResGroup5 : SchedWriteRes<[SKLPort6]> {
  let Latency = 1;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup5], (instregex "JMP(16|32|64)r")>;

def SKLWriteResGroup6 : SchedWriteRes<[SKLPort01]> {
  let Latency = 1;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup6], (instregex "PABSBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PABSDrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PABSWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PADDSBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PADDSWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PADDUSBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PADDUSWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PAVGBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PAVGWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PCMPEQBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PCMPEQDrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PCMPEQQrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PCMPEQWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PCMPGTBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PCMPGTDrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PCMPGTWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PMAXSBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PMAXSDrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PMAXSWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PMAXUBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PMAXUDrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PMAXUWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PMINSBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PMINSDrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PMINSWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PMINUBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PMINUDrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PMINUWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PSIGNBrr128")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PSIGNDrr128")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PSIGNWrr128")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PSLLDri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PSLLQri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PSLLWri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PSRADri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PSRAWri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PSRLDri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PSRLQri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PSRLWri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PSUBSBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PSUBSWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PSUBUSBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "PSUBUSWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPABSBYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPABSBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPABSBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPABSBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPABSDYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPABSDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPABSDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPABSDrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPABSQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPABSQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPABSWYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPABSWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPABSWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPABSWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDSBYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDSBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDSBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDSBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDSWYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDSWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDSWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDSWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDUSBYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDUSBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDUSBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDUSBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDUSWYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDUSWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDUSWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDUSWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPADDWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPAVGBYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPAVGBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPAVGBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPAVGBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPAVGWYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPAVGWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPAVGWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPAVGWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPCMPEQBYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPCMPEQBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPCMPEQDYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPCMPEQDrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPCMPEQQYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPCMPEQQrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPCMPEQWYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPCMPEQWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPCMPGTBYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPCMPGTBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPCMPGTDYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPCMPGTDrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPCMPGTWYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPCMPGTWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXSBYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXSBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXSBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXSBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXSDYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXSDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXSDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXSDrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXSWYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXSWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXSWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXSWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXUBYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXUBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXUBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXUBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXUDYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXUDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXUDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXUDrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXUWYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXUWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXUWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMAXUWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINSBYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINSBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINSBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINSBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINSDYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINSDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINSDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINSDrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINSWYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINSWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINSWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINSWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINUBYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINUBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINUBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINUBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINUDYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINUDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINUDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINUDrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINUWYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINUWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINUWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPMINUWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPROLDZ128r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPROLDZ256r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPROLQZ128r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPROLQZ256r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPROLVDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPROLVDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPROLVQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPROLVQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPRORDZ128r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPRORDZ256r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPRORQZ128r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPRORQZ256r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPRORVDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPRORVDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPRORVQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPRORVQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSIGNBYrr256")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSIGNBrr128")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSIGNDYrr256")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSIGNDrr128")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSIGNWYrr256")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSIGNWrr128")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLDYri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLDZ128r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLDZ256r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLDri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLQYri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLQZ128r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLQZ256r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLQri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLVDYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLVDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLVDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLVDrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLVQYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLVQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLVQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLVQrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLVWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLVWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLWYri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLWZ128ri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLWZ256ri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSLLWri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRADYri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRADZ128r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRADZ256r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRADri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRAQZ128r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRAQZ256r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRAVDYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRAVDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRAVDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRAVDrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRAVQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRAVQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRAVWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRAVWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRAWYri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRAWZ128ri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRAWZ256ri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRAWri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLDYri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLDZ128r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLDZ256r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLDri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLQYri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLQZ128r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLQZ256r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLQri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLVDYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLVDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLVDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLVDrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLVQYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLVQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLVQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLVQrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLVWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLVWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLWYri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLWZ128ri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLWZ256ri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSRLWri")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBSBYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBSBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBSBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBSBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBSWYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBSWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBSWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBSWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBUSBYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBUSBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBUSBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBUSBrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBUSWYrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBUSWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBUSWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBUSWrr")>;
def: InstRW<[SKLWriteResGroup6], (instregex "VPSUBWZ128rr(b?)(k?)(z?)")>;

def SKLWriteResGroup7 : SchedWriteRes<[SKLPort05]> {
  let Latency = 1;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup7], (instregex "FINCSTP")>;
def: InstRW<[SKLWriteResGroup7], (instregex "FNOP")>;
def: InstRW<[SKLWriteResGroup7], (instregex "MMX_MOVQ64rr(_REV?)")>;
def: InstRW<[SKLWriteResGroup7], (instregex "MMX_PABSBrr64")>;
def: InstRW<[SKLWriteResGroup7], (instregex "MMX_PABSDrr64")>;
def: InstRW<[SKLWriteResGroup7], (instregex "MMX_PABSWrr64")>;
def: InstRW<[SKLWriteResGroup7], (instregex "MMX_PADDBirr")>;
def: InstRW<[SKLWriteResGroup7], (instregex "MMX_PADDDirr")>;
def: InstRW<[SKLWriteResGroup7], (instregex "MMX_PADDQirr")>;
def: InstRW<[SKLWriteResGroup7], (instregex "MMX_PADDWirr")>;
def: InstRW<[SKLWriteResGroup7], (instregex "MMX_PANDNirr")>;
def: InstRW<[SKLWriteResGroup7], (instregex "MMX_PANDirr")>;
def: InstRW<[SKLWriteResGroup7], (instregex "MMX_PORirr")>;
def: InstRW<[SKLWriteResGroup7], (instregex "MMX_PSIGNBrr64")>;
def: InstRW<[SKLWriteResGroup7], (instregex "MMX_PSIGNDrr64")>;
def: InstRW<[SKLWriteResGroup7], (instregex "MMX_PSIGNWrr64")>;
def: InstRW<[SKLWriteResGroup7], (instregex "MMX_PSUBBirr")>;
def: InstRW<[SKLWriteResGroup7], (instregex "MMX_PSUBDirr")>;
def: InstRW<[SKLWriteResGroup7], (instregex "MMX_PSUBQirr")>;
def: InstRW<[SKLWriteResGroup7], (instregex "MMX_PSUBWirr")>;
def: InstRW<[SKLWriteResGroup7], (instregex "MMX_PXORirr")>;

def SKLWriteResGroup8 : SchedWriteRes<[SKLPort06]> {
  let Latency = 1;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup8], (instregex "ADC(16|32|64)ri8")>;
def: InstRW<[SKLWriteResGroup8], (instregex "ADC(16|32|64)rr(_REV?)")>;
def: InstRW<[SKLWriteResGroup8], (instregex "ADC8rr(_REV?)")>;
def: InstRW<[SKLWriteResGroup8], (instregex "ADCX32rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "ADCX64rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "ADOX32rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "ADOX64rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "BT(16|32|64)ri8")>;
def: InstRW<[SKLWriteResGroup8], (instregex "BT(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "BTC(16|32|64)ri8")>;
def: InstRW<[SKLWriteResGroup8], (instregex "BTC(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "BTR(16|32|64)ri8")>;
def: InstRW<[SKLWriteResGroup8], (instregex "BTR(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "BTS(16|32|64)ri8")>;
def: InstRW<[SKLWriteResGroup8], (instregex "BTS(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "CDQ")>;
def: InstRW<[SKLWriteResGroup8], (instregex "CLAC")>;
def: InstRW<[SKLWriteResGroup8], (instregex "CMOVAE(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "CMOVB(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "CMOVE(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "CMOVG(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "CMOVGE(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "CMOVL(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "CMOVLE(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "CMOVNE(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "CMOVNO(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "CMOVNP(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "CMOVNS(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "CMOVO(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "CMOVP(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "CMOVS(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "CQO")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JAE_1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JAE_4")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JA_1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JA_4")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JBE_1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JBE_4")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JB_1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JB_4")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JE_1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JE_4")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JGE_1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JGE_4")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JG_1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JG_4")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JLE_1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JLE_4")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JL_1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JL_4")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JMP_1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JMP_4")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JNE_1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JNE_4")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JNO_1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JNO_4")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JNP_1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JNP_4")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JNS_1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JNS_4")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JO_1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JO_4")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JP_1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JP_4")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JS_1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "JS_4")>;
def: InstRW<[SKLWriteResGroup8], (instregex "RORX32ri")>;
def: InstRW<[SKLWriteResGroup8], (instregex "RORX64ri")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SAR(16|32|64)r1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SAR(16|32|64)ri")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SAR8r1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SAR8ri")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SARX32rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SARX64rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SBB(16|32|64)ri8")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SBB(16|32|64)rr(_REV?)")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SBB8rr(_REV?)")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SETAEr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SETBr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SETEr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SETGEr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SETGr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SETLEr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SETLr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SETNEr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SETNOr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SETNPr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SETNSr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SETOr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SETPr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SETSr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SHL(16|32|64)r1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SHL(16|32|64)ri")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SHL8r1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SHL8ri")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SHLX32rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SHLX64rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SHR(16|32|64)r1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SHR(16|32|64)ri")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SHR8r1")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SHR8ri")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SHRX32rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "SHRX64rr")>;
def: InstRW<[SKLWriteResGroup8], (instregex "STAC")>;

def SKLWriteResGroup9 : SchedWriteRes<[SKLPort15]> {
  let Latency = 1;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup9], (instregex "ANDN32rr")>;
def: InstRW<[SKLWriteResGroup9], (instregex "ANDN64rr")>;
def: InstRW<[SKLWriteResGroup9], (instregex "BLSI32rr")>;
def: InstRW<[SKLWriteResGroup9], (instregex "BLSI64rr")>;
def: InstRW<[SKLWriteResGroup9], (instregex "BLSMSK32rr")>;
def: InstRW<[SKLWriteResGroup9], (instregex "BLSMSK64rr")>;
def: InstRW<[SKLWriteResGroup9], (instregex "BLSR32rr")>;
def: InstRW<[SKLWriteResGroup9], (instregex "BLSR64rr")>;
def: InstRW<[SKLWriteResGroup9], (instregex "BZHI32rr")>;
def: InstRW<[SKLWriteResGroup9], (instregex "BZHI64rr")>;
def: InstRW<[SKLWriteResGroup9], (instregex "LEA(16|32|64)r")>;

def SKLWriteResGroup10 : SchedWriteRes<[SKLPort015]> {
  let Latency = 1;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup10], (instregex "ANDNPDrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "ANDNPSrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "ANDPDrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "ANDPSrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "BLENDPDrri")>;
def: InstRW<[SKLWriteResGroup10], (instregex "BLENDPSrri")>;
def: InstRW<[SKLWriteResGroup10], (instregex "MMX_MOVD64from64rr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "MOVAPDrr(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "MOVAPSrr(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "MOVDQArr(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "MOVDQUrr(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "MOVPQI2QIrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "MOVSSrr(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "ORPDrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "ORPSrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "PADDBrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "PADDDrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "PADDQrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "PADDWrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "PANDNrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "PANDrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "PORrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "PSUBBrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "PSUBDrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "PSUBQrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "PSUBWrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "PXORrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VANDNPDYrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VANDNPDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VANDNPDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VANDNPDrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VANDNPSYrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VANDNPSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VANDNPSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VANDNPSrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VANDPDYrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VANDPDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VANDPDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VANDPDrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VANDPSYrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VANDPSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VANDPSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VANDPSrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VBLENDMPDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VBLENDMPDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VBLENDMPSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VBLENDMPSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VBLENDPDYrri")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VBLENDPDrri")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VBLENDPSYrri")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VBLENDPSrri")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVAPDYrr(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVAPDZ128rr(b?)(k?)(z?)(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVAPDZ256rr(b?)(k?)(z?)(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVAPDrr(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVAPSYrr(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVAPSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVAPSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVAPSrr(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVDQA32Z128rr(b?)(k?)(z?)(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVDQA32Z256rr(b?)(k?)(z?)(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVDQA64Z128rr(b?)(k?)(z?)(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVDQA64Z256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVDQAYrr(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVDQArr(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVDQU16Z128rr(b?)(k?)(z?)(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVDQU16Z256rr(b?)(k?)(z?)(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVDQU32Z128rr(b?)(k?)(z?)(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVDQU32Z256rr(b?)(k?)(z?)(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVDQU64Z128rr(b?)(k?)(z?)(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVDQU64Z256rr(b?)(k?)(z?)(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVDQU8Z128rr(b?)(k?)(z?)(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVDQU8Z256rr(b?)(k?)(z?)(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVDQUYrr(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVDQUrr(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVPQI2QIrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVSSrr(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVUPDZ128rr(b?)(k?)(z?)(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVUPDZ256rr(b?)(k?)(z?)(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVUPSZ128rr(b?)(k?)(z?)(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVUPSZ256rr(b?)(k?)(z?)(_REV?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VMOVZPQILo2PQIrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VORPDYrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VORPDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VORPDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VORPDrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VORPSYrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VORPSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VORPSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VORPSrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPADDBYrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPADDBrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPADDDYrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPADDDrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPADDQYrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPADDQrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPADDWYrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPADDWrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPANDDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPANDDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPANDNDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPANDNDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPANDNQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPANDNQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPANDNYrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPANDNrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPANDQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPANDQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPANDYrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPANDrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPBLENDDYrri")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPBLENDDrri")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPBLENDMBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPBLENDMBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPBLENDMDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPBLENDMDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPBLENDMQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPBLENDMQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPBLENDMWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPBLENDMWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPORDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPORDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPORQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPORQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPORYrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPORrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPSUBBYrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPSUBBrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPSUBDYrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPSUBDrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPSUBQYrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPSUBQrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPSUBWYrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPSUBWrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPTERNLOGDZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPTERNLOGDZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPTERNLOGQZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPTERNLOGQZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPXORDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPXORDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPXORQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPXORQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPXORYrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VPXORrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VXORPDYrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VXORPDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VXORPDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VXORPDrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VXORPSYrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VXORPSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VXORPSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup10], (instregex "VXORPSrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "XORPDrr")>;
def: InstRW<[SKLWriteResGroup10], (instregex "XORPSrr")>;

def SKLWriteResGroup11 : SchedWriteRes<[SKLPort0156]> {
  let Latency = 1;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup11], (instregex "ADD(16|32|64)ri8")>;
def: InstRW<[SKLWriteResGroup11], (instregex "ADD(16|32|64)rr(_REV?)")>;
def: InstRW<[SKLWriteResGroup11], (instregex "ADD8i8")>;
def: InstRW<[SKLWriteResGroup11], (instregex "ADD8ri")>;
def: InstRW<[SKLWriteResGroup11], (instregex "ADD8rr(_REV?)")>;
def: InstRW<[SKLWriteResGroup11], (instregex "AND(16|32|64)ri8")>;
def: InstRW<[SKLWriteResGroup11], (instregex "AND(16|32|64)rr(_REV?)")>;
def: InstRW<[SKLWriteResGroup11], (instregex "AND8i8")>;
def: InstRW<[SKLWriteResGroup11], (instregex "AND8ri")>;
def: InstRW<[SKLWriteResGroup11], (instregex "AND8rr(_REV?)")>;
def: InstRW<[SKLWriteResGroup11], (instregex "CBW")>;
def: InstRW<[SKLWriteResGroup11], (instregex "CLC")>;
def: InstRW<[SKLWriteResGroup11], (instregex "CMC")>;
def: InstRW<[SKLWriteResGroup11], (instregex "CMP(16|32|64)ri8")>;
def: InstRW<[SKLWriteResGroup11], (instregex "CMP(16|32|64)rr(_REV?)")>;
def: InstRW<[SKLWriteResGroup11], (instregex "CMP8i8")>;
def: InstRW<[SKLWriteResGroup11], (instregex "CMP8ri")>;
def: InstRW<[SKLWriteResGroup11], (instregex "CMP8rr(_REV?)")>;
def: InstRW<[SKLWriteResGroup11], (instregex "CWDE")>;
def: InstRW<[SKLWriteResGroup11], (instregex "DEC(16|32|64)r")>;
def: InstRW<[SKLWriteResGroup11], (instregex "DEC8r")>;
def: InstRW<[SKLWriteResGroup11], (instregex "INC(16|32|64)r")>;
def: InstRW<[SKLWriteResGroup11], (instregex "INC8r")>;
def: InstRW<[SKLWriteResGroup11], (instregex "LAHF")>;
def: InstRW<[SKLWriteResGroup11], (instregex "MOV(16|32|64)rr(_REV?)")>;
def: InstRW<[SKLWriteResGroup11], (instregex "MOV8ri(_alt?)")>;
def: InstRW<[SKLWriteResGroup11], (instregex "MOV8ri_alt")>;
def: InstRW<[SKLWriteResGroup11], (instregex "MOV8rr(_REV?)")>;
def: InstRW<[SKLWriteResGroup11], (instregex "MOVSX(16|32|64)rr16")>;
def: InstRW<[SKLWriteResGroup11], (instregex "MOVSX(16|32|64)rr32")>;
def: InstRW<[SKLWriteResGroup11], (instregex "MOVSX(16|32|64)rr8")>;
def: InstRW<[SKLWriteResGroup11], (instregex "MOVZX(16|32|64)rr16")>;
def: InstRW<[SKLWriteResGroup11], (instregex "MOVZX(16|32|64)rr8")>;
def: InstRW<[SKLWriteResGroup11], (instregex "NEG(16|32|64)r")>;
def: InstRW<[SKLWriteResGroup11], (instregex "NEG8r")>;
def: InstRW<[SKLWriteResGroup11], (instregex "NOOP")>;
def: InstRW<[SKLWriteResGroup11], (instregex "NOT(16|32|64)r")>;
def: InstRW<[SKLWriteResGroup11], (instregex "NOT8r")>;
def: InstRW<[SKLWriteResGroup11], (instregex "OR(16|32|64)ri8")>;
def: InstRW<[SKLWriteResGroup11], (instregex "OR(16|32|64)rr(_REV?)")>;
def: InstRW<[SKLWriteResGroup11], (instregex "OR8i8")>;
def: InstRW<[SKLWriteResGroup11], (instregex "OR8ri")>;
def: InstRW<[SKLWriteResGroup11], (instregex "OR8rr(_REV?)")>;
def: InstRW<[SKLWriteResGroup11], (instregex "SAHF")>;
def: InstRW<[SKLWriteResGroup11], (instregex "SGDT64m")>;
def: InstRW<[SKLWriteResGroup11], (instregex "SIDT64m")>;
def: InstRW<[SKLWriteResGroup11], (instregex "SLDT64m")>;
def: InstRW<[SKLWriteResGroup11], (instregex "SMSW16m")>;
def: InstRW<[SKLWriteResGroup11], (instregex "STC")>;
def: InstRW<[SKLWriteResGroup11], (instregex "STRm")>;
def: InstRW<[SKLWriteResGroup11], (instregex "SUB(16|32|64)ri8")>;
def: InstRW<[SKLWriteResGroup11], (instregex "SUB(16|32|64)rr(_REV?)")>;
def: InstRW<[SKLWriteResGroup11], (instregex "SUB8i8")>;
def: InstRW<[SKLWriteResGroup11], (instregex "SUB8ri")>;
def: InstRW<[SKLWriteResGroup11], (instregex "SUB8rr(_REV?)")>;
def: InstRW<[SKLWriteResGroup11], (instregex "SYSCALL")>;
def: InstRW<[SKLWriteResGroup11], (instregex "TEST(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup11], (instregex "TEST8i8")>;
def: InstRW<[SKLWriteResGroup11], (instregex "TEST8ri")>;
def: InstRW<[SKLWriteResGroup11], (instregex "TEST8rr")>;
def: InstRW<[SKLWriteResGroup11], (instregex "XCHG(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup11], (instregex "XOR(16|32|64)ri8")>;
def: InstRW<[SKLWriteResGroup11], (instregex "XOR(16|32|64)rr(_REV?)")>;
def: InstRW<[SKLWriteResGroup11], (instregex "XOR8i8")>;
def: InstRW<[SKLWriteResGroup11], (instregex "XOR8ri")>;
def: InstRW<[SKLWriteResGroup11], (instregex "XOR8rr(_REV?)")>;

def SKLWriteResGroup12 : SchedWriteRes<[SKLPort0,SKLPort23]> {
  let Latency = 1;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PADDSBirm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PADDSWirm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PADDUSBirm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PADDUSWirm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PAVGBirm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PAVGWirm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PCMPEQBirm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PCMPEQDirm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PCMPEQWirm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PCMPGTBirm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PCMPGTDirm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PCMPGTWirm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PMAXSWirm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PMAXUBirm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PMINSWirm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PMINUBirm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PSLLDrm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PSLLQrm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PSLLWrm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PSRADrm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PSRAWrm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PSRLDrm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PSRLQrm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PSRLWrm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PSUBSBirm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PSUBSWirm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PSUBUSBirm")>;
def: InstRW<[SKLWriteResGroup12], (instregex "MMX_PSUBUSWirm")>;

def SKLWriteResGroup13 : SchedWriteRes<[SKLPort0,SKLPort237]> {
  let Latency = 1;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup13], (instregex "MMX_MASKMOVQ64")>;
def: InstRW<[SKLWriteResGroup13], (instregex "VMASKMOVDQU")>;
def: InstRW<[SKLWriteResGroup13], (instregex "VMASKMOVPDYmr")>;
def: InstRW<[SKLWriteResGroup13], (instregex "VMASKMOVPDmr")>;
def: InstRW<[SKLWriteResGroup13], (instregex "VMASKMOVPSYmr")>;
def: InstRW<[SKLWriteResGroup13], (instregex "VMASKMOVPSmr")>;
def: InstRW<[SKLWriteResGroup13], (instregex "VPMASKMOVDYmr")>;
def: InstRW<[SKLWriteResGroup13], (instregex "VPMASKMOVDmr")>;
def: InstRW<[SKLWriteResGroup13], (instregex "VPMASKMOVQYmr")>;
def: InstRW<[SKLWriteResGroup13], (instregex "VPMASKMOVQmr")>;

def SKLWriteResGroup14 : SchedWriteRes<[SKLPort5,SKLPort23]> {
  let Latency = 1;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup14], (instregex "FCOM32m")>;
def: InstRW<[SKLWriteResGroup14], (instregex "FCOM64m")>;
def: InstRW<[SKLWriteResGroup14], (instregex "FCOMP32m")>;
def: InstRW<[SKLWriteResGroup14], (instregex "FCOMP64m")>;
def: InstRW<[SKLWriteResGroup14], (instregex "INSERTPSrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "MMX_PALIGNR64irm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "MMX_PINSRWirmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "MMX_PSHUFBrm64")>;
def: InstRW<[SKLWriteResGroup14], (instregex "MMX_PSHUFWmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "MMX_PUNPCKHBWirm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "MMX_PUNPCKHDQirm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "MMX_PUNPCKHWDirm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "MMX_PUNPCKLBWirm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "MMX_PUNPCKLDQirm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "MMX_PUNPCKLWDirm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "MOVHPDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "MOVHPSrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "MOVLPDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "MOVLPSrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PACKSSDWrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PACKSSWBrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PACKUSDWrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PACKUSWBrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PALIGNRrmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PBLENDWrmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PINSRBrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PINSRDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PINSRQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PINSRWrmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PMOVSXBDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PMOVSXBQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PMOVSXBWrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PMOVSXDQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PMOVSXWDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PMOVSXWQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PMOVZXBDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PMOVZXBQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PMOVZXBWrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PMOVZXDQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PMOVZXWDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PMOVZXWQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PSHUFBrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PSHUFDmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PSHUFHWmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PSHUFLWmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PUNPCKHBWrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PUNPCKHDQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PUNPCKHQDQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PUNPCKHWDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PUNPCKLBWrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PUNPCKLDQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PUNPCKLQDQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "PUNPCKLWDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "SHUFPDrmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "SHUFPSrmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "UNPCKHPDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "UNPCKHPSrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "UNPCKLPDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "UNPCKLPSrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VINSERTPSrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VMOVHPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VMOVHPDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VMOVHPSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VMOVHPSrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VMOVLPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VMOVLPDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VMOVLPSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VMOVLPSrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPACKSSDWYrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPACKSSDWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPACKSSDWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPACKSSDWrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPACKSSWBYrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPACKSSWBZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPACKSSWBZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPACKSSWBrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPACKUSDWYrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPACKUSDWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPACKUSDWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPACKUSDWrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPACKUSWBYrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPACKUSWBZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPACKUSWBZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPACKUSWBrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPALIGNRYrmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPALIGNRZ128rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPALIGNRZ256rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPALIGNRrmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPBLENDWYrmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPBLENDWrmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPBROADCASTBYrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPBROADCASTBZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPBROADCASTBZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPBROADCASTBrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPBROADCASTWYrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPBROADCASTWZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPBROADCASTWZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPBROADCASTWrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPERMILPDYmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPERMILPDYrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPERMILPDZ128m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPERMILPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPERMILPDZ256m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPERMILPDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPERMILPDmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPERMILPDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPERMILPSYmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPERMILPSYrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPERMILPSZ128m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPERMILPSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPERMILPSZ256m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPERMILPSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPERMILPSmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPERMILPSrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPINSRBrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPINSRDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPINSRQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPINSRWrmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPMOVSXBDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPMOVSXBQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPMOVSXBWrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPMOVSXDQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPMOVSXWDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPMOVSXWQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPMOVZXBDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPMOVZXBQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPMOVZXBWrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPMOVZXDQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPMOVZXWDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPMOVZXWQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPSHUFBYrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPSHUFBZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPSHUFBZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPSHUFBrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPSHUFDYmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPSHUFDZ128m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPSHUFDZ256m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPSHUFDmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPSHUFHWYmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPSHUFHWZ128mi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPSHUFHWZ128mi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPSHUFHWmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPSHUFLWYmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPSHUFLWZ128mi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPSHUFLWZ128mi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPSHUFLWmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPSLLDQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPSLLDQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPSRLDQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPSRLDQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKHBWYrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKHBWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKHBWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKHBWrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKHDQYrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKHDQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKHDQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKHDQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKHQDQYrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKHQDQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKHQDQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKHQDQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKHWDYrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKHWDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKHWDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKHWDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKLBWYrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKLBWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKLBWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKLBWrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKLDQYrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKLDQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKLDQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKLDQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKLQDQYrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKLQDQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKLQDQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKLQDQrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKLWDYrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKLWDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKLWDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VPUNPCKLWDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VSHUFPDYrmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VSHUFPDZ128rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VSHUFPDZ256rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VSHUFPDrmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VSHUFPSYrmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VSHUFPSZ128rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VSHUFPSZ256rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VSHUFPSrmi")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VUNPCKHPDYrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VUNPCKHPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VUNPCKHPDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VUNPCKHPDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VUNPCKHPSYrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VUNPCKHPSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VUNPCKHPSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VUNPCKHPSrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VUNPCKLPDYrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VUNPCKLPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VUNPCKLPDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VUNPCKLPDrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VUNPCKLPSYrm")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VUNPCKLPSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VUNPCKLPSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup14], (instregex "VUNPCKLPSrm")>;

def SKLWriteResGroup15 : SchedWriteRes<[SKLPort6,SKLPort23]> {
  let Latency = 1;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup15], (instregex "FARJMP64")>;
def: InstRW<[SKLWriteResGroup15], (instregex "JMP(16|32|64)m")>;

def SKLWriteResGroup16 : SchedWriteRes<[SKLPort01,SKLPort23]> {
  let Latency = 1;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup16], (instregex "PABSBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PABSDrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PABSWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PADDSBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PADDSWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PADDUSBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PADDUSWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PAVGBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PAVGWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PCMPEQBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PCMPEQDrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PCMPEQQrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PCMPEQWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PCMPGTBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PCMPGTDrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PCMPGTWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PMAXSBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PMAXSDrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PMAXSWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PMAXUBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PMAXUDrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PMAXUWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PMINSBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PMINSDrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PMINSWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PMINUBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PMINUDrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PMINUWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PSIGNBrm128")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PSIGNDrm128")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PSIGNWrm128")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PSLLDrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PSLLQrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PSLLWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PSRADrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PSRAWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PSRLDrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PSRLQrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PSRLWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PSUBSBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PSUBSWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PSUBUSBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "PSUBUSWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPABSBYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPABSBZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPABSBZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPABSBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPABSDYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPABSDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPABSDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPABSDrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPABSQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPABSQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPABSWYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPABSWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPABSWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPABSWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDBZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDBZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDSBYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDSBZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDSBZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDSBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDSWYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDSWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDSWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDSWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDUSBYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDUSBZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDUSBZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDUSBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDUSWYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDUSWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDUSWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDUSWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPADDWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPAVGBYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPAVGBZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPAVGBZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPAVGBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPAVGWYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPAVGWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPAVGWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPAVGWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPCMPEQBYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPCMPEQBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPCMPEQDYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPCMPEQDrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPCMPEQQYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPCMPEQQrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPCMPEQWYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPCMPEQWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPCMPGTBYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPCMPGTBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPCMPGTDYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPCMPGTDrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPCMPGTWYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPCMPGTWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXSBYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXSBZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXSBZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXSBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXSDYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXSDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXSDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXSDrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXSWYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXSWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXSWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXSWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXUBYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXUBZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXUBZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXUBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXUDYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXUDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXUDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXUDrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXUWYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXUWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXUWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMAXUWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINSBYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINSBZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINSBZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINSBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINSDYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINSDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINSDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINSDrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINSWYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINSWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINSWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINSWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINUBYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINUBZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINUBZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINUBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINUDYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINUDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINUDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINUDrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINUWYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINUWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINUWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPMINUWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPROLDZ128m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPROLDZ256m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPROLQZ128m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPROLQZ256m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPROLVDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPROLVDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPROLVQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPROLVQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPRORDZ128m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPRORDZ256m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPRORQZ128m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPRORQZ256m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPRORVDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPRORVDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPRORVQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPRORVQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSIGNBYrm256")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSIGNBrm128")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSIGNDYrm256")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSIGNDrm128")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSIGNWYrm256")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSIGNWrm128")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLDYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLDZ128m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLDZ256m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLDrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLQYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLQZ128m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLQZ256m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLQrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLVDYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLVDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLVDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLVDrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLVQYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLVQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLVQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLVQrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLVWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLVWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLWYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLWZ128mi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLWZ256mi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSLLWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRADYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRADZ128m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRADZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRADZ256m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRADZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRADrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRAQZ128m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRAQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRAQZ256m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRAQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRAVDYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRAVDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRAVDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRAVDrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRAVQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRAVQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRAVWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRAVWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRAWYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRAWZ128mi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRAWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRAWZ256mi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRAWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRAWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLDYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLDZ128m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLDZ256m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLDrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLQYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLQZ128m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLQZ256m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLQrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLVDYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLVDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLVDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLVDrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLVQYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLVQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLVQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLVQrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLVWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLVWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLWYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLWZ128mi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLWZ256mi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSRLWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBBZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBBZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBSBYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBSBZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBSBZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBSBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBSWYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBSWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBSWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBSWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBUSBYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBUSBZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBUSBZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBUSBrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBUSWYrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBUSWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBUSWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBUSWrm")>;
def: InstRW<[SKLWriteResGroup16], (instregex "VPSUBWZ128rm(b?)(k?)(z?)")>;

def SKLWriteResGroup17 : SchedWriteRes<[SKLPort23,SKLPort05]> {
  let Latency = 1;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup17], (instregex "MMX_PABSBrm64")>;
def: InstRW<[SKLWriteResGroup17], (instregex "MMX_PABSDrm64")>;
def: InstRW<[SKLWriteResGroup17], (instregex "MMX_PABSWrm64")>;
def: InstRW<[SKLWriteResGroup17], (instregex "MMX_PADDBirm")>;
def: InstRW<[SKLWriteResGroup17], (instregex "MMX_PADDDirm")>;
def: InstRW<[SKLWriteResGroup17], (instregex "MMX_PADDQirm")>;
def: InstRW<[SKLWriteResGroup17], (instregex "MMX_PADDWirm")>;
def: InstRW<[SKLWriteResGroup17], (instregex "MMX_PANDNirm")>;
def: InstRW<[SKLWriteResGroup17], (instregex "MMX_PANDirm")>;
def: InstRW<[SKLWriteResGroup17], (instregex "MMX_PORirm")>;
def: InstRW<[SKLWriteResGroup17], (instregex "MMX_PSIGNBrm64")>;
def: InstRW<[SKLWriteResGroup17], (instregex "MMX_PSIGNDrm64")>;
def: InstRW<[SKLWriteResGroup17], (instregex "MMX_PSIGNWrm64")>;
def: InstRW<[SKLWriteResGroup17], (instregex "MMX_PSUBBirm")>;
def: InstRW<[SKLWriteResGroup17], (instregex "MMX_PSUBDirm")>;
def: InstRW<[SKLWriteResGroup17], (instregex "MMX_PSUBQirm")>;
def: InstRW<[SKLWriteResGroup17], (instregex "MMX_PSUBWirm")>;
def: InstRW<[SKLWriteResGroup17], (instregex "MMX_PXORirm")>;

def SKLWriteResGroup18 : SchedWriteRes<[SKLPort23,SKLPort06]> {
  let Latency = 1;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup18], (instregex "ADC(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "ADC8rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "ADCX32rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "ADCX64rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "ADOX32rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "ADOX64rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "BT(16|32|64)mi8")>;
def: InstRW<[SKLWriteResGroup18], (instregex "CMOVAE(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "CMOVB(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "CMOVE(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "CMOVG(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "CMOVGE(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "CMOVL(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "CMOVLE(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "CMOVNE(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "CMOVNO(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "CMOVNP(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "CMOVNS(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "CMOVO(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "CMOVP(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "CMOVS(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "RORX32mi")>;
def: InstRW<[SKLWriteResGroup18], (instregex "RORX64mi")>;
def: InstRW<[SKLWriteResGroup18], (instregex "SARX32rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "SARX64rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "SBB(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "SBB8rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "SHLX32rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "SHLX64rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "SHRX32rm")>;
def: InstRW<[SKLWriteResGroup18], (instregex "SHRX64rm")>;

def SKLWriteResGroup19 : SchedWriteRes<[SKLPort23,SKLPort15]> {
  let Latency = 1;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup19], (instregex "ANDN32rm")>;
def: InstRW<[SKLWriteResGroup19], (instregex "ANDN64rm")>;
def: InstRW<[SKLWriteResGroup19], (instregex "BLSI32rm")>;
def: InstRW<[SKLWriteResGroup19], (instregex "BLSI64rm")>;
def: InstRW<[SKLWriteResGroup19], (instregex "BLSMSK32rm")>;
def: InstRW<[SKLWriteResGroup19], (instregex "BLSMSK64rm")>;
def: InstRW<[SKLWriteResGroup19], (instregex "BLSR32rm")>;
def: InstRW<[SKLWriteResGroup19], (instregex "BLSR64rm")>;
def: InstRW<[SKLWriteResGroup19], (instregex "BZHI32rm")>;
def: InstRW<[SKLWriteResGroup19], (instregex "BZHI64rm")>;
def: InstRW<[SKLWriteResGroup19], (instregex "MOVBE(16|32|64)rm")>;

def SKLWriteResGroup20 : SchedWriteRes<[SKLPort23,SKLPort015]> {
  let Latency = 1;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup20], (instregex "ANDNPDrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "ANDNPSrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "ANDPDrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "ANDPSrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "BLENDPDrmi")>;
def: InstRW<[SKLWriteResGroup20], (instregex "BLENDPSrmi")>;
def: InstRW<[SKLWriteResGroup20], (instregex "ORPDrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "ORPSrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "PADDBrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "PADDDrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "PADDQrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "PADDWrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "PANDNrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "PANDrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "PORrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "PSUBBrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "PSUBDrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "PSUBQrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "PSUBWrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "PXORrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VANDNPDYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VANDNPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VANDNPDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VANDNPDrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VANDNPSYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VANDNPSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VANDNPSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VANDNPSrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VANDPDYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VANDPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VANDPDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VANDPDrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VANDPSYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VANDPSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VANDPSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VANDPSrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VBLENDMPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VBLENDMPDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VBLENDMPSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VBLENDMPSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VBLENDPDYrmi")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VBLENDPDrmi")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VBLENDPSYrmi")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VBLENDPSrmi")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VINSERTF128rm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VINSERTF32x4Z256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VINSERTF64x2Z256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VINSERTI128rm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VINSERTI32x4Z256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VINSERTI64x2Z256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMASKMOVPDYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMASKMOVPDrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMASKMOVPSYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMASKMOVPSrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMOVAPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMOVAPDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMOVAPSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMOVAPSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMOVDQA32Z128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMOVDQA32Z256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMOVDQA64Z128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMOVDQA64Z256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMOVDQU16Z128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMOVDQU16Z256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMOVDQU32Z128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMOVDQU32Z256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMOVDQU64Z128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMOVDQU64Z256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMOVDQU8Z128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMOVDQU8Z256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMOVUPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMOVUPDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMOVUPSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VMOVUPSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VORPDYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VORPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VORPDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VORPDrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VORPSYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VORPSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VORPSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VORPSrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPADDBYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPADDBrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPADDDYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPADDDrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPADDQYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPADDQrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPADDWYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPADDWrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPANDDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPANDDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPANDNDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPANDNDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPANDNQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPANDNQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPANDNYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPANDNrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPANDQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPANDQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPANDYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPANDrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPBLENDDYrmi")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPBLENDDrmi")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPBLENDMBZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPBLENDMBZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPBLENDMDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPBLENDMDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPBLENDMQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPBLENDMQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPBLENDMWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPBLENDMWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPMASKMOVDYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPMASKMOVDrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPMASKMOVQYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPMASKMOVQrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPORDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPORDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPORQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPORQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPORYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPORrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPSUBBYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPSUBBrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPSUBDYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPSUBDrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPSUBQYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPSUBQrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPSUBWYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPSUBWrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPTERNLOGDZ128rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPTERNLOGDZ256rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPTERNLOGQZ128rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPTERNLOGQZ256rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPXORDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPXORDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPXORQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPXORQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPXORYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VPXORrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VXORPDYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VXORPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VXORPDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VXORPDrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VXORPSYrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VXORPSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VXORPSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup20], (instregex "VXORPSrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "XORPDrm")>;
def: InstRW<[SKLWriteResGroup20], (instregex "XORPSrm")>;

def SKLWriteResGroup21 : SchedWriteRes<[SKLPort23,SKLPort0156]> {
  let Latency = 1;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup21], (instregex "ADD(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup21], (instregex "ADD8rm")>;
def: InstRW<[SKLWriteResGroup21], (instregex "AND(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup21], (instregex "AND8rm")>;
def: InstRW<[SKLWriteResGroup21], (instregex "CMP(16|32|64)mi8")>;
def: InstRW<[SKLWriteResGroup21], (instregex "CMP(16|32|64)mr")>;
def: InstRW<[SKLWriteResGroup21], (instregex "CMP(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup21], (instregex "CMP8mi")>;
def: InstRW<[SKLWriteResGroup21], (instregex "CMP8mr")>;
def: InstRW<[SKLWriteResGroup21], (instregex "CMP8rm")>;
def: InstRW<[SKLWriteResGroup21], (instregex "OR(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup21], (instregex "OR8rm")>;
def: InstRW<[SKLWriteResGroup21], (instregex "POP(16|32|64)r(mr?)")>;
def: InstRW<[SKLWriteResGroup21], (instregex "SUB(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup21], (instregex "SUB8rm")>;
def: InstRW<[SKLWriteResGroup21], (instregex "TEST(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup21], (instregex "TEST8mi")>;
def: InstRW<[SKLWriteResGroup21], (instregex "TEST8rm")>;
def: InstRW<[SKLWriteResGroup21], (instregex "XOR(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup21], (instregex "XOR8rm")>;

def SKLWriteResGroup22 : SchedWriteRes<[SKLPort237,SKLPort0156]> {
  let Latency = 1;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup22], (instregex "SFENCE")>;

def SKLWriteResGroup23 : SchedWriteRes<[SKLPort4,SKLPort5,SKLPort237]> {
  let Latency = 1;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup23], (instregex "EXTRACTPSmr")>;
def: InstRW<[SKLWriteResGroup23], (instregex "PEXTRBmr")>;
def: InstRW<[SKLWriteResGroup23], (instregex "PEXTRDmr")>;
def: InstRW<[SKLWriteResGroup23], (instregex "PEXTRQmr")>;
def: InstRW<[SKLWriteResGroup23], (instregex "PEXTRWmr")>;
def: InstRW<[SKLWriteResGroup23], (instregex "STMXCSR")>;
def: InstRW<[SKLWriteResGroup23], (instregex "VEXTRACTPSmr")>;
def: InstRW<[SKLWriteResGroup23], (instregex "VPEXTRBmr")>;
def: InstRW<[SKLWriteResGroup23], (instregex "VPEXTRDmr")>;
def: InstRW<[SKLWriteResGroup23], (instregex "VPEXTRQmr")>;
def: InstRW<[SKLWriteResGroup23], (instregex "VPEXTRWmr")>;
def: InstRW<[SKLWriteResGroup23], (instregex "VSTMXCSR")>;

def SKLWriteResGroup24 : SchedWriteRes<[SKLPort4,SKLPort6,SKLPort237]> {
  let Latency = 1;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup24], (instregex "FNSTCW16m")>;

def SKLWriteResGroup25 : SchedWriteRes<[SKLPort4,SKLPort237,SKLPort06]> {
  let Latency = 1;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup25], (instregex "SETAEm")>;
def: InstRW<[SKLWriteResGroup25], (instregex "SETBm")>;
def: InstRW<[SKLWriteResGroup25], (instregex "SETEm")>;
def: InstRW<[SKLWriteResGroup25], (instregex "SETGEm")>;
def: InstRW<[SKLWriteResGroup25], (instregex "SETGm")>;
def: InstRW<[SKLWriteResGroup25], (instregex "SETLEm")>;
def: InstRW<[SKLWriteResGroup25], (instregex "SETLm")>;
def: InstRW<[SKLWriteResGroup25], (instregex "SETNEm")>;
def: InstRW<[SKLWriteResGroup25], (instregex "SETNOm")>;
def: InstRW<[SKLWriteResGroup25], (instregex "SETNPm")>;
def: InstRW<[SKLWriteResGroup25], (instregex "SETNSm")>;
def: InstRW<[SKLWriteResGroup25], (instregex "SETOm")>;
def: InstRW<[SKLWriteResGroup25], (instregex "SETPm")>;
def: InstRW<[SKLWriteResGroup25], (instregex "SETSm")>;

def SKLWriteResGroup26 : SchedWriteRes<[SKLPort4,SKLPort237,SKLPort15]> {
  let Latency = 1;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup26], (instregex "MOVBE(16|32|64)mr")>;

def SKLWriteResGroup27 : SchedWriteRes<[SKLPort4,SKLPort237,SKLPort0156]> {
  let Latency = 1;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup27], (instregex "PUSH(16|32|64)r(mr?)")>;
def: InstRW<[SKLWriteResGroup27], (instregex "PUSH64i8")>;
def: InstRW<[SKLWriteResGroup27], (instregex "STOSB")>;
def: InstRW<[SKLWriteResGroup27], (instregex "STOSL")>;
def: InstRW<[SKLWriteResGroup27], (instregex "STOSQ")>;
def: InstRW<[SKLWriteResGroup27], (instregex "STOSW")>;

def SKLWriteResGroup28 : SchedWriteRes<[SKLPort4,SKLPort23,SKLPort237,SKLPort06]> {
  let Latency = 1;
  let NumMicroOps = 4;
  let ResourceCycles = [1,1,1,1];
}
def: InstRW<[SKLWriteResGroup28], (instregex "BTC(16|32|64)mi8")>;
def: InstRW<[SKLWriteResGroup28], (instregex "BTR(16|32|64)mi8")>;
def: InstRW<[SKLWriteResGroup28], (instregex "BTS(16|32|64)mi8")>;
def: InstRW<[SKLWriteResGroup28], (instregex "SAR(16|32|64)m1")>;
def: InstRW<[SKLWriteResGroup28], (instregex "SAR(16|32|64)mi")>;
def: InstRW<[SKLWriteResGroup28], (instregex "SAR8m1")>;
def: InstRW<[SKLWriteResGroup28], (instregex "SAR8mi")>;
def: InstRW<[SKLWriteResGroup28], (instregex "SHL(16|32|64)m1")>;
def: InstRW<[SKLWriteResGroup28], (instregex "SHL(16|32|64)mi")>;
def: InstRW<[SKLWriteResGroup28], (instregex "SHL8m1")>;
def: InstRW<[SKLWriteResGroup28], (instregex "SHL8mi")>;
def: InstRW<[SKLWriteResGroup28], (instregex "SHR(16|32|64)m1")>;
def: InstRW<[SKLWriteResGroup28], (instregex "SHR(16|32|64)mi")>;
def: InstRW<[SKLWriteResGroup28], (instregex "SHR8m1")>;
def: InstRW<[SKLWriteResGroup28], (instregex "SHR8mi")>;

def SKLWriteResGroup29 : SchedWriteRes<[SKLPort4,SKLPort23,SKLPort237,SKLPort0156]> {
  let Latency = 1;
  let NumMicroOps = 4;
  let ResourceCycles = [1,1,1,1];
}
def: InstRW<[SKLWriteResGroup29], (instregex "ADD(16|32|64)mi8")>;
def: InstRW<[SKLWriteResGroup29], (instregex "ADD(16|32|64)mr")>;
def: InstRW<[SKLWriteResGroup29], (instregex "ADD8mi")>;
def: InstRW<[SKLWriteResGroup29], (instregex "ADD8mr")>;
def: InstRW<[SKLWriteResGroup29], (instregex "AND(16|32|64)mi8")>;
def: InstRW<[SKLWriteResGroup29], (instregex "AND(16|32|64)mr")>;
def: InstRW<[SKLWriteResGroup29], (instregex "AND8mi")>;
def: InstRW<[SKLWriteResGroup29], (instregex "AND8mr")>;
def: InstRW<[SKLWriteResGroup29], (instregex "DEC(16|32|64)m")>;
def: InstRW<[SKLWriteResGroup29], (instregex "DEC8m")>;
def: InstRW<[SKLWriteResGroup29], (instregex "INC(16|32|64)m")>;
def: InstRW<[SKLWriteResGroup29], (instregex "INC8m")>;
def: InstRW<[SKLWriteResGroup29], (instregex "NEG(16|32|64)m")>;
def: InstRW<[SKLWriteResGroup29], (instregex "NEG8m")>;
def: InstRW<[SKLWriteResGroup29], (instregex "NOT(16|32|64)m")>;
def: InstRW<[SKLWriteResGroup29], (instregex "NOT8m")>;
def: InstRW<[SKLWriteResGroup29], (instregex "OR(16|32|64)mi8")>;
def: InstRW<[SKLWriteResGroup29], (instregex "OR(16|32|64)mr")>;
def: InstRW<[SKLWriteResGroup29], (instregex "OR8mi")>;
def: InstRW<[SKLWriteResGroup29], (instregex "OR8mr")>;
def: InstRW<[SKLWriteResGroup29], (instregex "POP(16|32|64)rmm")>;
def: InstRW<[SKLWriteResGroup29], (instregex "PUSH(16|32|64)rmm")>;
def: InstRW<[SKLWriteResGroup29], (instregex "SUB(16|32|64)mi8")>;
def: InstRW<[SKLWriteResGroup29], (instregex "SUB(16|32|64)mr")>;
def: InstRW<[SKLWriteResGroup29], (instregex "SUB8mi")>;
def: InstRW<[SKLWriteResGroup29], (instregex "SUB8mr")>;
def: InstRW<[SKLWriteResGroup29], (instregex "XOR(16|32|64)mi8")>;
def: InstRW<[SKLWriteResGroup29], (instregex "XOR(16|32|64)mr")>;
def: InstRW<[SKLWriteResGroup29], (instregex "XOR8mi")>;
def: InstRW<[SKLWriteResGroup29], (instregex "XOR8mr")>;

def SKLWriteResGroup30 : SchedWriteRes<[SKLPort4,SKLPort237,SKLPort15]> {
  let Latency = 1;
  let NumMicroOps = 5;
  let ResourceCycles = [2,2,1];
}

def SKLWriteResGroup31 : SchedWriteRes<[SKLPort0]> {
  let Latency = 2;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup31], (instregex "COMISDrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "COMISSrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "MMX_MOVD64from64rr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "MMX_MOVD64grr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "MMX_PMOVMSKBrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "MOVMSKPDrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "MOVMSKPSrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "MOVPDI2DIrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "MOVPQIto64rr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "PMOVMSKBrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "UCOMISDrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "UCOMISSrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "VCOMISDrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "VCOMISSrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "VMOVMSKPDYrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "VMOVMSKPDrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "VMOVMSKPSYrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "VMOVMSKPSrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "VMOVPDI2DIrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "VMOVPQIto64rr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "VPMOVMSKBYrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "VPMOVMSKBrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "VTESTPDYrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "VTESTPDrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "VTESTPSYrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "VTESTPSrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "VUCOMISDrr")>;
def: InstRW<[SKLWriteResGroup31], (instregex "VUCOMISSrr")>;

def SKLWriteResGroup32 : SchedWriteRes<[SKLPort5]> {
  let Latency = 2;
  let NumMicroOps = 2;
  let ResourceCycles = [2];
}
def: InstRW<[SKLWriteResGroup32], (instregex "MMX_MOVQ2DQrr")>;
def: InstRW<[SKLWriteResGroup32], (instregex "MMX_PINSRWirri")>;
def: InstRW<[SKLWriteResGroup32], (instregex "PINSRBrr")>;
def: InstRW<[SKLWriteResGroup32], (instregex "PINSRDrr")>;
def: InstRW<[SKLWriteResGroup32], (instregex "PINSRQrr")>;
def: InstRW<[SKLWriteResGroup32], (instregex "PINSRWrri")>;
def: InstRW<[SKLWriteResGroup32], (instregex "VPINSRBrr")>;
def: InstRW<[SKLWriteResGroup32], (instregex "VPINSRDrr")>;
def: InstRW<[SKLWriteResGroup32], (instregex "VPINSRQrr")>;
def: InstRW<[SKLWriteResGroup32], (instregex "VPINSRWrri")>;

def SKLWriteResGroup33 : SchedWriteRes<[SKLPort05]> {
  let Latency = 2;
  let NumMicroOps = 2;
  let ResourceCycles = [2];
}
def: InstRW<[SKLWriteResGroup33], (instregex "FDECSTP")>;
def: InstRW<[SKLWriteResGroup33], (instregex "MMX_MOVDQ2Qrr")>;

def SKLWriteResGroup34 : SchedWriteRes<[SKLPort06]> {
  let Latency = 2;
  let NumMicroOps = 2;
  let ResourceCycles = [2];
}
def: InstRW<[SKLWriteResGroup34], (instregex "CMOVA(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup34], (instregex "CMOVBE(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup34], (instregex "ROL(16|32|64)r1")>;
def: InstRW<[SKLWriteResGroup34], (instregex "ROL(16|32|64)ri")>;
def: InstRW<[SKLWriteResGroup34], (instregex "ROL8r1")>;
def: InstRW<[SKLWriteResGroup34], (instregex "ROL8ri")>;
def: InstRW<[SKLWriteResGroup34], (instregex "ROR(16|32|64)r1")>;
def: InstRW<[SKLWriteResGroup34], (instregex "ROR(16|32|64)ri")>;
def: InstRW<[SKLWriteResGroup34], (instregex "ROR8r1")>;
def: InstRW<[SKLWriteResGroup34], (instregex "ROR8ri")>;
def: InstRW<[SKLWriteResGroup34], (instregex "SETAr")>;
def: InstRW<[SKLWriteResGroup34], (instregex "SETBEr")>;

def SKLWriteResGroup35 : SchedWriteRes<[SKLPort015]> {
  let Latency = 2;
  let NumMicroOps = 2;
  let ResourceCycles = [2];
}
def: InstRW<[SKLWriteResGroup35], (instregex "BLENDVPDrr0")>;
def: InstRW<[SKLWriteResGroup35], (instregex "BLENDVPSrr0")>;
def: InstRW<[SKLWriteResGroup35], (instregex "PBLENDVBrr0")>;
def: InstRW<[SKLWriteResGroup35], (instregex "VBLENDVPDYrr")>;
def: InstRW<[SKLWriteResGroup35], (instregex "VBLENDVPDrr")>;
def: InstRW<[SKLWriteResGroup35], (instregex "VBLENDVPSYrr")>;
def: InstRW<[SKLWriteResGroup35], (instregex "VBLENDVPSrr")>;
def: InstRW<[SKLWriteResGroup35], (instregex "VPBLENDVBYrr")>;
def: InstRW<[SKLWriteResGroup35], (instregex "VPBLENDVBrr")>;

def SKLWriteResGroup36 : SchedWriteRes<[SKLPort0156]> {
  let Latency = 2;
  let NumMicroOps = 2;
  let ResourceCycles = [2];
}
def: InstRW<[SKLWriteResGroup36], (instregex "LFENCE")>;
def: InstRW<[SKLWriteResGroup36], (instregex "WAIT")>;
def: InstRW<[SKLWriteResGroup36], (instregex "XGETBV")>;

def SKLWriteResGroup37 : SchedWriteRes<[SKLPort0,SKLPort23]> {
  let Latency = 2;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup37], (instregex "COMISDrm")>;
def: InstRW<[SKLWriteResGroup37], (instregex "COMISSrm")>;
def: InstRW<[SKLWriteResGroup37], (instregex "UCOMISDrm")>;
def: InstRW<[SKLWriteResGroup37], (instregex "UCOMISSrm")>;
def: InstRW<[SKLWriteResGroup37], (instregex "VCOMISDrm")>;
def: InstRW<[SKLWriteResGroup37], (instregex "VCOMISSrm")>;
def: InstRW<[SKLWriteResGroup37], (instregex "VTESTPDYrm")>;
def: InstRW<[SKLWriteResGroup37], (instregex "VTESTPDrm")>;
def: InstRW<[SKLWriteResGroup37], (instregex "VTESTPSYrm")>;
def: InstRW<[SKLWriteResGroup37], (instregex "VTESTPSrm")>;
def: InstRW<[SKLWriteResGroup37], (instregex "VUCOMISDrm")>;
def: InstRW<[SKLWriteResGroup37], (instregex "VUCOMISSrm")>;

def SKLWriteResGroup38 : SchedWriteRes<[SKLPort5,SKLPort01]> {
  let Latency = 2;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup38], (instregex "PSLLDrr")>;
def: InstRW<[SKLWriteResGroup38], (instregex "PSLLQrr")>;
def: InstRW<[SKLWriteResGroup38], (instregex "PSLLWrr")>;
def: InstRW<[SKLWriteResGroup38], (instregex "PSRADrr")>;
def: InstRW<[SKLWriteResGroup38], (instregex "PSRAWrr")>;
def: InstRW<[SKLWriteResGroup38], (instregex "PSRLDrr")>;
def: InstRW<[SKLWriteResGroup38], (instregex "PSRLQrr")>;
def: InstRW<[SKLWriteResGroup38], (instregex "PSRLWrr")>;
def: InstRW<[SKLWriteResGroup38], (instregex "VPSLLDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup38], (instregex "VPSLLDrr")>;
def: InstRW<[SKLWriteResGroup38], (instregex "VPSLLQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup38], (instregex "VPSLLQrr")>;
def: InstRW<[SKLWriteResGroup38], (instregex "VPSLLWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup38], (instregex "VPSLLWrr")>;
def: InstRW<[SKLWriteResGroup38], (instregex "VPSRADZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup38], (instregex "VPSRADrr")>;
def: InstRW<[SKLWriteResGroup38], (instregex "VPSRAQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup38], (instregex "VPSRAWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup38], (instregex "VPSRAWrr")>;
def: InstRW<[SKLWriteResGroup38], (instregex "VPSRLDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup38], (instregex "VPSRLDrr")>;
def: InstRW<[SKLWriteResGroup38], (instregex "VPSRLQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup38], (instregex "VPSRLQrr")>;
def: InstRW<[SKLWriteResGroup38], (instregex "VPSRLWrr")>;

def SKLWriteResGroup39 : SchedWriteRes<[SKLPort6,SKLPort0156]> {
  let Latency = 2;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup39], (instregex "CLFLUSH")>;

def SKLWriteResGroup40 : SchedWriteRes<[SKLPort06,SKLPort15]> {
  let Latency = 2;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup40], (instregex "BEXTR32rr")>;
def: InstRW<[SKLWriteResGroup40], (instregex "BEXTR64rr")>;
def: InstRW<[SKLWriteResGroup40], (instregex "BSWAP(16|32|64)r")>;

def SKLWriteResGroup41 : SchedWriteRes<[SKLPort06,SKLPort0156]> {
  let Latency = 2;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup41], (instregex "ADC8i8")>;
def: InstRW<[SKLWriteResGroup41], (instregex "ADC8ri")>;
def: InstRW<[SKLWriteResGroup41], (instregex "CWD")>;
def: InstRW<[SKLWriteResGroup41], (instregex "JRCXZ")>;
def: InstRW<[SKLWriteResGroup41], (instregex "SBB8i8")>;
def: InstRW<[SKLWriteResGroup41], (instregex "SBB8ri")>;

def SKLWriteResGroup42 : SchedWriteRes<[SKLPort5,SKLPort23]> {
  let Latency = 2;
  let NumMicroOps = 3;
  let ResourceCycles = [2,1];
}
def: InstRW<[SKLWriteResGroup42], (instregex "MMX_PACKSSDWirm")>;
def: InstRW<[SKLWriteResGroup42], (instregex "MMX_PACKSSWBirm")>;
def: InstRW<[SKLWriteResGroup42], (instregex "MMX_PACKUSWBirm")>;

def SKLWriteResGroup43 : SchedWriteRes<[SKLPort23,SKLPort06]> {
  let Latency = 2;
  let NumMicroOps = 3;
  let ResourceCycles = [1,2];
}
def: InstRW<[SKLWriteResGroup43], (instregex "CMOVA(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup43], (instregex "CMOVBE(16|32|64)rm")>;

def SKLWriteResGroup44 : SchedWriteRes<[SKLPort23,SKLPort015]> {
  let Latency = 2;
  let NumMicroOps = 3;
  let ResourceCycles = [1,2];
}
def: InstRW<[SKLWriteResGroup44], (instregex "BLENDVPDrm0")>;
def: InstRW<[SKLWriteResGroup44], (instregex "BLENDVPSrm0")>;
def: InstRW<[SKLWriteResGroup44], (instregex "PBLENDVBrm0")>;
def: InstRW<[SKLWriteResGroup44], (instregex "VBLENDVPDYrm")>;
def: InstRW<[SKLWriteResGroup44], (instregex "VBLENDVPDrm")>;
def: InstRW<[SKLWriteResGroup44], (instregex "VBLENDVPSYrm")>;
def: InstRW<[SKLWriteResGroup44], (instregex "VBLENDVPSrm")>;
def: InstRW<[SKLWriteResGroup44], (instregex "VPBLENDVBYrm")>;
def: InstRW<[SKLWriteResGroup44], (instregex "VPBLENDVBrm")>;

def SKLWriteResGroup45 : SchedWriteRes<[SKLPort23,SKLPort0156]> {
  let Latency = 2;
  let NumMicroOps = 3;
  let ResourceCycles = [1,2];
}
def: InstRW<[SKLWriteResGroup45], (instregex "LEAVE64")>;
def: InstRW<[SKLWriteResGroup45], (instregex "SCASB")>;
def: InstRW<[SKLWriteResGroup45], (instregex "SCASL")>;
def: InstRW<[SKLWriteResGroup45], (instregex "SCASQ")>;
def: InstRW<[SKLWriteResGroup45], (instregex "SCASW")>;

def SKLWriteResGroup46 : SchedWriteRes<[SKLPort237,SKLPort0156]> {
  let Latency = 2;
  let NumMicroOps = 3;
  let ResourceCycles = [1,2];
}
def: InstRW<[SKLWriteResGroup46], (instregex "MFENCE")>;

def SKLWriteResGroup47 : SchedWriteRes<[SKLPort0,SKLPort4,SKLPort237]> {
  let Latency = 2;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup47], (instregex "FNSTSWm")>;

def SKLWriteResGroup48 : SchedWriteRes<[SKLPort0,SKLPort23,SKLPort05]> {
  let Latency = 2;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup48], (instregex "FLDCW16m")>;

def SKLWriteResGroup49 : SchedWriteRes<[SKLPort0,SKLPort23,SKLPort0156]> {
  let Latency = 2;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup49], (instregex "LDMXCSR")>;
def: InstRW<[SKLWriteResGroup49], (instregex "VLDMXCSR")>;

def SKLWriteResGroup50 : SchedWriteRes<[SKLPort5,SKLPort23,SKLPort0156]> {
  let Latency = 2;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup50], (instregex "KMOVBkm")>;
def: InstRW<[SKLWriteResGroup50], (instregex "KMOVDkm")>;
def: InstRW<[SKLWriteResGroup50], (instregex "KMOVQkm")>;
def: InstRW<[SKLWriteResGroup50], (instregex "KMOVWkm")>;

def SKLWriteResGroup51 : SchedWriteRes<[SKLPort6,SKLPort23,SKLPort0156]> {
  let Latency = 2;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup51], (instregex "LRETQ")>;
def: InstRW<[SKLWriteResGroup51], (instregex "RETQ")>;

def SKLWriteResGroup52 : SchedWriteRes<[SKLPort23,SKLPort06,SKLPort15]> {
  let Latency = 2;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup52], (instregex "BEXTR32rm")>;
def: InstRW<[SKLWriteResGroup52], (instregex "BEXTR64rm")>;

def SKLWriteResGroup53 : SchedWriteRes<[SKLPort4,SKLPort237,SKLPort06]> {
  let Latency = 2;
  let NumMicroOps = 4;
  let ResourceCycles = [1,1,2];
}
def: InstRW<[SKLWriteResGroup53], (instregex "SETAm")>;
def: InstRW<[SKLWriteResGroup53], (instregex "SETBEm")>;

def SKLWriteResGroup54 : SchedWriteRes<[SKLPort4,SKLPort6,SKLPort237,SKLPort0156]> {
  let Latency = 2;
  let NumMicroOps = 4;
  let ResourceCycles = [1,1,1,1];
}
def: InstRW<[SKLWriteResGroup54], (instregex "CALL(16|32|64)r")>;

def SKLWriteResGroup55 : SchedWriteRes<[SKLPort4,SKLPort237,SKLPort06,SKLPort0156]> {
  let Latency = 2;
  let NumMicroOps = 4;
  let ResourceCycles = [1,1,1,1];
}
def: InstRW<[SKLWriteResGroup55], (instregex "CALL64pcrel32")>;

def SKLWriteResGroup56 : SchedWriteRes<[SKLPort4,SKLPort23,SKLPort237,SKLPort06]> {
  let Latency = 2;
  let NumMicroOps = 5;
  let ResourceCycles = [1,1,1,2];
}
def: InstRW<[SKLWriteResGroup56], (instregex "ROL(16|32|64)m1")>;
def: InstRW<[SKLWriteResGroup56], (instregex "ROL(16|32|64)mi")>;
def: InstRW<[SKLWriteResGroup56], (instregex "ROL8m1")>;
def: InstRW<[SKLWriteResGroup56], (instregex "ROL8mi")>;
def: InstRW<[SKLWriteResGroup56], (instregex "ROR(16|32|64)m1")>;
def: InstRW<[SKLWriteResGroup56], (instregex "ROR(16|32|64)mi")>;
def: InstRW<[SKLWriteResGroup56], (instregex "ROR8m1")>;
def: InstRW<[SKLWriteResGroup56], (instregex "ROR8mi")>;

def SKLWriteResGroup57 : SchedWriteRes<[SKLPort4,SKLPort23,SKLPort237,SKLPort0156]> {
  let Latency = 2;
  let NumMicroOps = 5;
  let ResourceCycles = [1,1,1,2];
}
def: InstRW<[SKLWriteResGroup57], (instregex "XADD(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup57], (instregex "XADD8rm")>;

def SKLWriteResGroup58 : SchedWriteRes<[SKLPort4,SKLPort6,SKLPort23,SKLPort237,SKLPort0156]> {
  let Latency = 2;
  let NumMicroOps = 5;
  let ResourceCycles = [1,1,1,1,1];
}
def: InstRW<[SKLWriteResGroup58], (instregex "CALL(16|32|64)m")>;
def: InstRW<[SKLWriteResGroup58], (instregex "FARCALL64")>;

def SKLWriteResGroup59 : SchedWriteRes<[SKLPort0]> {
  let Latency = 3;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup59], (instregex "KADDBrr")>;
def: InstRW<[SKLWriteResGroup59], (instregex "KADDDrr")>;
def: InstRW<[SKLWriteResGroup59], (instregex "KADDQrr")>;
def: InstRW<[SKLWriteResGroup59], (instregex "KADDWrr")>;
def: InstRW<[SKLWriteResGroup59], (instregex "KMOVBrk")>;
def: InstRW<[SKLWriteResGroup59], (instregex "KMOVDrk")>;
def: InstRW<[SKLWriteResGroup59], (instregex "KMOVQrk")>;
def: InstRW<[SKLWriteResGroup59], (instregex "KMOVWrk")>;
def: InstRW<[SKLWriteResGroup59], (instregex "KORTESTBrr")>;
def: InstRW<[SKLWriteResGroup59], (instregex "KORTESTDrr")>;
def: InstRW<[SKLWriteResGroup59], (instregex "KORTESTQrr")>;
def: InstRW<[SKLWriteResGroup59], (instregex "KORTESTWrr")>;
def: InstRW<[SKLWriteResGroup59], (instregex "KTESTBrr")>;
def: InstRW<[SKLWriteResGroup59], (instregex "KTESTDrr")>;
def: InstRW<[SKLWriteResGroup59], (instregex "KTESTQrr")>;
def: InstRW<[SKLWriteResGroup59], (instregex "KTESTWrr")>;

def SKLWriteResGroup60 : SchedWriteRes<[SKLPort1]> {
  let Latency = 3;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup60], (instregex "BSF(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup60], (instregex "BSR(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup60], (instregex "IMUL64rr(i8?)")>;
def: InstRW<[SKLWriteResGroup60], (instregex "IMUL8r")>;
def: InstRW<[SKLWriteResGroup60], (instregex "LZCNT(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup60], (instregex "MUL8r")>;
def: InstRW<[SKLWriteResGroup60], (instregex "PDEP32rr")>;
def: InstRW<[SKLWriteResGroup60], (instregex "PDEP64rr")>;
def: InstRW<[SKLWriteResGroup60], (instregex "PEXT32rr")>;
def: InstRW<[SKLWriteResGroup60], (instregex "PEXT64rr")>;
def: InstRW<[SKLWriteResGroup60], (instregex "POPCNT(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup60], (instregex "SHLD(16|32|64)rri8")>;
def: InstRW<[SKLWriteResGroup60], (instregex "SHRD(16|32|64)rri8")>;
def: InstRW<[SKLWriteResGroup60], (instregex "TZCNT(16|32|64)rr")>;

def SKLWriteResGroup60_16 : SchedWriteRes<[SKLPort1, SKLPort0156]> {
  let Latency = 3;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup60_16], (instregex "IMUL16rr(i8?)")>;

def SKLWriteResGroup60_32 : SchedWriteRes<[SKLPort1]> {
  let Latency = 3;
  let NumMicroOps = 1;
}
def: InstRW<[SKLWriteResGroup60_32], (instregex "IMUL32rr(i8?)")>;

def SKLWriteResGroup61 : SchedWriteRes<[SKLPort5]> {
  let Latency = 3;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup61], (instregex "ADD_FPrST0")>;
def: InstRW<[SKLWriteResGroup61], (instregex "ADD_FST0r")>;
def: InstRW<[SKLWriteResGroup61], (instregex "ADD_FrST0")>;
def: InstRW<[SKLWriteResGroup61], (instregex "KSHIFTLBri")>;
def: InstRW<[SKLWriteResGroup61], (instregex "KSHIFTLDri")>;
def: InstRW<[SKLWriteResGroup61], (instregex "KSHIFTLQri")>;
def: InstRW<[SKLWriteResGroup61], (instregex "KSHIFTLWri")>;
def: InstRW<[SKLWriteResGroup61], (instregex "KSHIFTRBri")>;
def: InstRW<[SKLWriteResGroup61], (instregex "KSHIFTRDri")>;
def: InstRW<[SKLWriteResGroup61], (instregex "KSHIFTRQri")>;
def: InstRW<[SKLWriteResGroup61], (instregex "KSHIFTRWri")>;
def: InstRW<[SKLWriteResGroup61], (instregex "KUNPCKBWrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "KUNPCKDQrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "KUNPCKWDrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "MMX_PSADBWirr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "PCMPGTQrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "PSADBWrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "SUBR_FPrST0")>;
def: InstRW<[SKLWriteResGroup61], (instregex "SUBR_FST0r")>;
def: InstRW<[SKLWriteResGroup61], (instregex "SUBR_FrST0")>;
def: InstRW<[SKLWriteResGroup61], (instregex "SUB_FPrST0")>;
def: InstRW<[SKLWriteResGroup61], (instregex "SUB_FST0r")>;
def: InstRW<[SKLWriteResGroup61], (instregex "SUB_FrST0")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VALIGNDZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VALIGNDZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VALIGNQZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VALIGNQZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VBROADCASTF32X2Z256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VBROADCASTI32X2Z256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VBROADCASTSDYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VBROADCASTSDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VBROADCASTSSYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VBROADCASTSSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VBROADCASTSSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VCMPPDZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VCMPPDZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VCMPPSZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VCMPPSZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VDBPSADBWZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VDBPSADBWZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VEXTRACTF128rr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VEXTRACTF32x4Z256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VEXTRACTF64x2Z256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VEXTRACTI128rr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VEXTRACTI32x4Z256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VEXTRACTI64x2Z256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VFPCLASSPDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VFPCLASSPDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VFPCLASSPSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VFPCLASSPSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VFPCLASSSDrr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VFPCLASSSSrr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VINSERTF128rr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VINSERTF32x4Z256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VINSERTF64x2Z256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VINSERTI128rr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VINSERTI32x4Z256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VINSERTI64x2Z256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPBROADCASTBYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPBROADCASTBZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPBROADCASTBZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPBROADCASTBrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPBROADCASTDYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPBROADCASTDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPBROADCASTDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPBROADCASTDrZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPBROADCASTDrZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPBROADCASTQYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPBROADCASTQZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPBROADCASTQZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPBROADCASTQrZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPBROADCASTQrZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPBROADCASTWYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPBROADCASTWZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPBROADCASTWZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPBROADCASTWrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPBZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPBZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPDZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPDZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPEQBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPEQBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPEQDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPEQDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPEQQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPEQQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPEQWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPEQWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPGTBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPGTBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPGTDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPGTDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPGTQYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPGTQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPGTQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPGTQrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPGTWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPGTWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPQZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPQZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPUBZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPUBZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPUDZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPUDZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPUQZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPUQZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPUWZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPUWZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPWZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPCMPWZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERM2F128rr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERM2I128rr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMDYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMI2D128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMI2D256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMI2Drr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMI2PD128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMI2PD256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMI2PDrr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMI2PS128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMI2PS256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMI2PSrr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMI2Q128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMI2Q256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMI2Qrr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMPDYri")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMPDZ256r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMPDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMPSYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMPSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMQYri")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMQZ256r(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMT2D128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMT2D256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMT2Drr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMT2PD128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMT2PD256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMT2PDrr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMT2PS128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMT2PS256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMT2PSrr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMT2Q128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMT2Q256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPERMT2Qrr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMAXSQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMAXSQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMAXUQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMAXUQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMINSQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMINSQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMINUQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMINUQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVQDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVQDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVSXBDYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVSXBDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVSXBDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVSXBQYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVSXBQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVSXBQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVSXBWYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVSXBWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVSXBWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVSXDQYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVSXDQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVSXDQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVSXWDYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVSXWDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVSXWDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVSXWQYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVSXWQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVSXWQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVZXBDYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVZXBDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVZXBDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVZXBQYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVZXBQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVZXBQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVZXBWYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVZXBWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVZXBWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVZXDQYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVZXDQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVZXDQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVZXWDYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVZXWDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVZXWDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVZXWQYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVZXWQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPMOVZXWQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPSADBWYrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPSADBWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPSADBWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPSADBWrr")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPTESTMBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPTESTMBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPTESTMDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPTESTMDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPTESTMQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPTESTMQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPTESTMWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPTESTMWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPTESTNMBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPTESTNMBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPTESTNMDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPTESTNMDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPTESTNMQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPTESTNMQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPTESTNMWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VPTESTNMWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VSHUFF32X4Z256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VSHUFF64X2Z256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VSHUFI32X4Z256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup61], (instregex "VSHUFI64X2Z256rri(b?)(k?)(z?)")>;

def SKLWriteResGroup62 : SchedWriteRes<[SKLPort0,SKLPort5]> {
  let Latency = 3;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup62], (instregex "EXTRACTPSrr")>;
def: InstRW<[SKLWriteResGroup62], (instregex "MMX_PEXTRWirri")>;
def: InstRW<[SKLWriteResGroup62], (instregex "PEXTRBrr")>;
def: InstRW<[SKLWriteResGroup62], (instregex "PEXTRDrr")>;
def: InstRW<[SKLWriteResGroup62], (instregex "PEXTRQrr")>;
def: InstRW<[SKLWriteResGroup62], (instregex "PEXTRWri")>;
def: InstRW<[SKLWriteResGroup62], (instregex "PEXTRWrr_REV")>;
def: InstRW<[SKLWriteResGroup62], (instregex "PTESTrr")>;
def: InstRW<[SKLWriteResGroup62], (instregex "VEXTRACTPSrr")>;
def: InstRW<[SKLWriteResGroup62], (instregex "VPEXTRBrr")>;
def: InstRW<[SKLWriteResGroup62], (instregex "VPEXTRDrr")>;
def: InstRW<[SKLWriteResGroup62], (instregex "VPEXTRQrr")>;
def: InstRW<[SKLWriteResGroup62], (instregex "VPEXTRWri")>;
def: InstRW<[SKLWriteResGroup62], (instregex "VPEXTRWrr_REV")>;
def: InstRW<[SKLWriteResGroup62], (instregex "VPTESTYrr")>;
def: InstRW<[SKLWriteResGroup62], (instregex "VPTESTrr")>;

def SKLWriteResGroup63 : SchedWriteRes<[SKLPort0,SKLPort0156]> {
  let Latency = 3;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup63], (instregex "FNSTSW16r")>;

def SKLWriteResGroup64 : SchedWriteRes<[SKLPort1,SKLPort23]> {
  let Latency = 3;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup64], (instregex "BSF(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup64], (instregex "BSR(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup64], (instregex "IMUL64m")>;
def: InstRW<[SKLWriteResGroup64], (instregex "IMUL(32|64)rm(i8?)")>;
def: InstRW<[SKLWriteResGroup64], (instregex "IMUL8m")>;
def: InstRW<[SKLWriteResGroup64], (instregex "LZCNT(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup64], (instregex "MUL64m")>;
def: InstRW<[SKLWriteResGroup64], (instregex "MUL8m")>;
def: InstRW<[SKLWriteResGroup64], (instregex "PDEP32rm")>;
def: InstRW<[SKLWriteResGroup64], (instregex "PDEP64rm")>;
def: InstRW<[SKLWriteResGroup64], (instregex "PEXT32rm")>;
def: InstRW<[SKLWriteResGroup64], (instregex "PEXT64rm")>;
def: InstRW<[SKLWriteResGroup64], (instregex "POPCNT(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup64], (instregex "TZCNT(16|32|64)rm")>;

def SKLWriteResGroup64_16 : SchedWriteRes<[SKLPort1, SKLPort0156, SKLPort23]> {
  let Latency = 3;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1]; 
}
def: InstRW<[SKLWriteResGroup64_16], (instregex "IMUL16rm(i8?)")>;

def SKLWriteResGroup64_16_2 : SchedWriteRes<[SKLPort1, SKLPort0156, SKLPort23]> {
  let Latency = 3;
  let NumMicroOps = 5;
}
def: InstRW<[SKLWriteResGroup64_16_2], (instregex "IMUL16m")>;
def: InstRW<[SKLWriteResGroup64_16_2], (instregex "MUL16m")>;

def SKLWriteResGroup64_32 : SchedWriteRes<[SKLPort1, SKLPort0156, SKLPort23]> {
  let Latency = 3;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup64_32], (instregex "IMUL32m")>;
def: InstRW<[SKLWriteResGroup64_32], (instregex "MUL32m")>;

def SKLWriteResGroup65 : SchedWriteRes<[SKLPort5,SKLPort23]> {
  let Latency = 3;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup65], (instregex "ADD_F32m")>;
def: InstRW<[SKLWriteResGroup65], (instregex "ADD_F64m")>;
def: InstRW<[SKLWriteResGroup65], (instregex "ILD_F16m")>;
def: InstRW<[SKLWriteResGroup65], (instregex "ILD_F32m")>;
def: InstRW<[SKLWriteResGroup65], (instregex "ILD_F64m")>;
def: InstRW<[SKLWriteResGroup65], (instregex "MMX_PSADBWirm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "PCMPGTQrm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "PSADBWrm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "SUBR_F32m")>;
def: InstRW<[SKLWriteResGroup65], (instregex "SUBR_F64m")>;
def: InstRW<[SKLWriteResGroup65], (instregex "SUB_F32m")>;
def: InstRW<[SKLWriteResGroup65], (instregex "SUB_F64m")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VALIGNDZ128rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VALIGNDZ256rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VALIGNQZ128rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VALIGNQZ256rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VCMPPDZ128rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VCMPPDZ256rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VCMPPSZ128rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VCMPPSZ256rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VDBPSADBWZ128rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VDBPSADBWZ256rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VFPCLASSSDrm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VFPCLASSSSrm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPBZ128rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPBZ256rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPDZ128rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPDZ256rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPEQBZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPEQBZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPEQDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPEQDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPEQQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPEQQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPEQWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPEQWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPGTBZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPGTBZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPGTDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPGTDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPGTQYrm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPGTQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPGTQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPGTQrm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPGTWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPGTWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPQZ128rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPQZ256rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPUBZ128rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPUBZ256rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPUDZ128rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPUDZ256rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPUQZ128rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPUQZ256rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPUWZ128rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPUWZ256rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPWZ128rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPCMPWZ256rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERM2F128rm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERM2I128rm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMDYrm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMI2D128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMI2D256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMI2Drm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMI2PD128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMI2PD256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMI2PDrm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMI2PS128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMI2PS256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMI2PSrm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMI2Q128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMI2Q256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMI2Qrm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMPDYmi")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMPDZ256m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMPDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMPSYrm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMPSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMQYmi")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMQZ256m(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMT2D128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMT2D256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMT2Drm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMT2PD128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMT2PD256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMT2PDrm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMT2PS128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMT2PS256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMT2PSrm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMT2Q128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMT2Q256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPERMT2Qrm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMAXSQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMAXSQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMAXUQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMAXUQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMINSQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMINSQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMINUQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMINUQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVSXBDYrm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVSXBDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVSXBDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVSXBQYrm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVSXBQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVSXBQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVSXBWYrm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVSXBWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVSXBWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVSXDQYrm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVSXDQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVSXDQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVSXWDYrm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVSXWDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVSXWDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVSXWQYrm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVSXWQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVSXWQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVZXBDYrm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVZXBDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVZXBDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVZXBQYrm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVZXBQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVZXBQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVZXBWYrm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVZXBWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVZXBWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVZXDQYrm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVZXDQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVZXDQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVZXWDYrm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVZXWDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVZXWDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVZXWQYrm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVZXWQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPMOVZXWQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPSADBWYrm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPSADBWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPSADBWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPSADBWrm")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPTESTMBZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPTESTMBZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPTESTMDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPTESTMDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPTESTMQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPTESTMQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPTESTMWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPTESTMWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPTESTNMBZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPTESTNMBZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPTESTNMDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPTESTNMDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPTESTNMQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPTESTNMQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPTESTNMWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VPTESTNMWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VSHUFF32X4Z256rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VSHUFF64X2Z256rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VSHUFI32X4Z256rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup65], (instregex "VSHUFI64X2Z256rm(b?)i(k?)(z?)")>;

def SKLWriteResGroup66 : SchedWriteRes<[SKLPort06]> {
  let Latency = 3;
  let NumMicroOps = 3;
  let ResourceCycles = [3];
}
def: InstRW<[SKLWriteResGroup66], (instregex "ROL(16|32|64)rCL")>;
def: InstRW<[SKLWriteResGroup66], (instregex "ROL8rCL")>;
def: InstRW<[SKLWriteResGroup66], (instregex "ROR(16|32|64)rCL")>;
def: InstRW<[SKLWriteResGroup66], (instregex "ROR8rCL")>;
def: InstRW<[SKLWriteResGroup66], (instregex "SAR(16|32|64)rCL")>;
def: InstRW<[SKLWriteResGroup66], (instregex "SAR8rCL")>;
def: InstRW<[SKLWriteResGroup66], (instregex "SHL(16|32|64)rCL")>;
def: InstRW<[SKLWriteResGroup66], (instregex "SHL8rCL")>;
def: InstRW<[SKLWriteResGroup66], (instregex "SHR(16|32|64)rCL")>;
def: InstRW<[SKLWriteResGroup66], (instregex "SHR8rCL")>;

def SKLWriteResGroup67 : SchedWriteRes<[SKLPort0156]> {
  let Latency = 3;
  let NumMicroOps = 3;
  let ResourceCycles = [3];
}
def: InstRW<[SKLWriteResGroup67], (instregex "XADD(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup67], (instregex "XADD8rr")>;
def: InstRW<[SKLWriteResGroup67], (instregex "XCHG8rr")>;

def SKLWriteResGroup68 : SchedWriteRes<[SKLPort0,SKLPort5]> {
  let Latency = 3;
  let NumMicroOps = 3;
  let ResourceCycles = [1,2];
}
def: InstRW<[SKLWriteResGroup68], (instregex "MMX_PHADDSWrr64")>;
def: InstRW<[SKLWriteResGroup68], (instregex "MMX_PHSUBSWrr64")>;

def SKLWriteResGroup69 : SchedWriteRes<[SKLPort5,SKLPort01]> {
  let Latency = 3;
  let NumMicroOps = 3;
  let ResourceCycles = [2,1];
}
def: InstRW<[SKLWriteResGroup69], (instregex "PHADDSWrr128")>;
def: InstRW<[SKLWriteResGroup69], (instregex "PHSUBSWrr128")>;
def: InstRW<[SKLWriteResGroup69], (instregex "VPHADDSWrr128")>;
def: InstRW<[SKLWriteResGroup69], (instregex "VPHADDSWrr256")>;
def: InstRW<[SKLWriteResGroup69], (instregex "VPHSUBSWrr128")>;
def: InstRW<[SKLWriteResGroup69], (instregex "VPHSUBSWrr256")>;

def SKLWriteResGroup70 : SchedWriteRes<[SKLPort5,SKLPort05]> {
  let Latency = 3;
  let NumMicroOps = 3;
  let ResourceCycles = [2,1];
}
def: InstRW<[SKLWriteResGroup70], (instregex "MMX_PHADDWrr64")>;
def: InstRW<[SKLWriteResGroup70], (instregex "MMX_PHADDrr64")>;
def: InstRW<[SKLWriteResGroup70], (instregex "MMX_PHSUBDrr64")>;
def: InstRW<[SKLWriteResGroup70], (instregex "MMX_PHSUBWrr64")>;

def SKLWriteResGroup71 : SchedWriteRes<[SKLPort5,SKLPort015]> {
  let Latency = 3;
  let NumMicroOps = 3;
  let ResourceCycles = [2,1];
}
def: InstRW<[SKLWriteResGroup71], (instregex "PHADDDrr")>;
def: InstRW<[SKLWriteResGroup71], (instregex "PHADDWrr")>;
def: InstRW<[SKLWriteResGroup71], (instregex "PHSUBDrr")>;
def: InstRW<[SKLWriteResGroup71], (instregex "PHSUBWrr")>;
def: InstRW<[SKLWriteResGroup71], (instregex "VPHADDDYrr")>;
def: InstRW<[SKLWriteResGroup71], (instregex "VPHADDDrr")>;
def: InstRW<[SKLWriteResGroup71], (instregex "VPHADDWYrr")>;
def: InstRW<[SKLWriteResGroup71], (instregex "VPHADDWrr")>;
def: InstRW<[SKLWriteResGroup71], (instregex "VPHSUBDYrr")>;
def: InstRW<[SKLWriteResGroup71], (instregex "VPHSUBDrr")>;
def: InstRW<[SKLWriteResGroup71], (instregex "VPHSUBWYrr")>;
def: InstRW<[SKLWriteResGroup71], (instregex "VPHSUBWrr")>;

def SKLWriteResGroup72 : SchedWriteRes<[SKLPort5,SKLPort0156]> {
  let Latency = 3;
  let NumMicroOps = 3;
  let ResourceCycles = [2,1];
}
def: InstRW<[SKLWriteResGroup72], (instregex "MMX_PACKSSDWirr")>;
def: InstRW<[SKLWriteResGroup72], (instregex "MMX_PACKSSWBirr")>;
def: InstRW<[SKLWriteResGroup72], (instregex "MMX_PACKUSWBirr")>;

def SKLWriteResGroup73 : SchedWriteRes<[SKLPort6,SKLPort0156]> {
  let Latency = 3;
  let NumMicroOps = 3;
  let ResourceCycles = [1,2];
}
def: InstRW<[SKLWriteResGroup73], (instregex "CLD")>;

def SKLWriteResGroup74 : SchedWriteRes<[SKLPort06,SKLPort0156]> {
  let Latency = 3;
  let NumMicroOps = 3;
  let ResourceCycles = [1,2];
}
def: InstRW<[SKLWriteResGroup74], (instregex "RCL(16|32|64)r1")>;
def: InstRW<[SKLWriteResGroup74], (instregex "RCL(16|32|64)ri")>;
def: InstRW<[SKLWriteResGroup74], (instregex "RCL8r1")>;
def: InstRW<[SKLWriteResGroup74], (instregex "RCL8ri")>;
def: InstRW<[SKLWriteResGroup74], (instregex "RCR(16|32|64)r1")>;
def: InstRW<[SKLWriteResGroup74], (instregex "RCR(16|32|64)ri")>;
def: InstRW<[SKLWriteResGroup74], (instregex "RCR8r1")>;
def: InstRW<[SKLWriteResGroup74], (instregex "RCR8ri")>;

def SKLWriteResGroup75 : SchedWriteRes<[SKLPort0,SKLPort5,SKLPort23]> {
  let Latency = 3;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup75], (instregex "PTESTrm")>;
def: InstRW<[SKLWriteResGroup75], (instregex "VPTESTYrm")>;
def: InstRW<[SKLWriteResGroup75], (instregex "VPTESTrm")>;

def SKLWriteResGroup76 : SchedWriteRes<[SKLPort4,SKLPort5,SKLPort237]> {
  let Latency = 3;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup76], (instregex "ISTT_FP16m")>;
def: InstRW<[SKLWriteResGroup76], (instregex "ISTT_FP32m")>;
def: InstRW<[SKLWriteResGroup76], (instregex "ISTT_FP64m")>;
def: InstRW<[SKLWriteResGroup76], (instregex "IST_F16m")>;
def: InstRW<[SKLWriteResGroup76], (instregex "IST_F32m")>;
def: InstRW<[SKLWriteResGroup76], (instregex "IST_FP16m")>;
def: InstRW<[SKLWriteResGroup76], (instregex "IST_FP32m")>;
def: InstRW<[SKLWriteResGroup76], (instregex "IST_FP64m")>;
def: InstRW<[SKLWriteResGroup76], (instregex "VPMOVQDZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup76], (instregex "VPMOVQDZ256mr(b?)(k?)(z?)")>;

def SKLWriteResGroup77 : SchedWriteRes<[SKLPort0,SKLPort5,SKLPort23]> {
  let Latency = 3;
  let NumMicroOps = 4;
  let ResourceCycles = [1,2,1];
}
def: InstRW<[SKLWriteResGroup77], (instregex "MMX_PHADDSWrm64")>;
def: InstRW<[SKLWriteResGroup77], (instregex "MMX_PHSUBSWrm64")>;

def SKLWriteResGroup78 : SchedWriteRes<[SKLPort5,SKLPort01,SKLPort23]> {
  let Latency = 3;
  let NumMicroOps = 4;
  let ResourceCycles = [2,1,1];
}
def: InstRW<[SKLWriteResGroup78], (instregex "PHADDSWrm128")>;
def: InstRW<[SKLWriteResGroup78], (instregex "PHSUBSWrm128")>;
def: InstRW<[SKLWriteResGroup78], (instregex "VPHADDSWrm128")>;
def: InstRW<[SKLWriteResGroup78], (instregex "VPHADDSWrm256")>;
def: InstRW<[SKLWriteResGroup78], (instregex "VPHSUBSWrm128")>;
def: InstRW<[SKLWriteResGroup78], (instregex "VPHSUBSWrm256")>;

def SKLWriteResGroup79 : SchedWriteRes<[SKLPort5,SKLPort23,SKLPort05]> {
  let Latency = 3;
  let NumMicroOps = 4;
  let ResourceCycles = [2,1,1];
}
def: InstRW<[SKLWriteResGroup79], (instregex "MMX_PHADDWrm64")>;
def: InstRW<[SKLWriteResGroup79], (instregex "MMX_PHADDrm64")>;
def: InstRW<[SKLWriteResGroup79], (instregex "MMX_PHSUBDrm64")>;
def: InstRW<[SKLWriteResGroup79], (instregex "MMX_PHSUBWrm64")>;

def SKLWriteResGroup80 : SchedWriteRes<[SKLPort5,SKLPort23,SKLPort015]> {
  let Latency = 3;
  let NumMicroOps = 4;
  let ResourceCycles = [2,1,1];
}
def: InstRW<[SKLWriteResGroup80], (instregex "PHADDDrm")>;
def: InstRW<[SKLWriteResGroup80], (instregex "PHADDWrm")>;
def: InstRW<[SKLWriteResGroup80], (instregex "PHSUBDrm")>;
def: InstRW<[SKLWriteResGroup80], (instregex "PHSUBWrm")>;
def: InstRW<[SKLWriteResGroup80], (instregex "VPHADDDYrm")>;
def: InstRW<[SKLWriteResGroup80], (instregex "VPHADDDrm")>;
def: InstRW<[SKLWriteResGroup80], (instregex "VPHADDWYrm")>;
def: InstRW<[SKLWriteResGroup80], (instregex "VPHADDWrm")>;
def: InstRW<[SKLWriteResGroup80], (instregex "VPHSUBDYrm")>;
def: InstRW<[SKLWriteResGroup80], (instregex "VPHSUBDrm")>;
def: InstRW<[SKLWriteResGroup80], (instregex "VPHSUBWYrm")>;
def: InstRW<[SKLWriteResGroup80], (instregex "VPHSUBWrm")>;

def SKLWriteResGroup81 : SchedWriteRes<[SKLPort23,SKLPort237,SKLPort06]> {
  let Latency = 3;
  let NumMicroOps = 5;
  let ResourceCycles = [1,1,3];
}
def: InstRW<[SKLWriteResGroup81], (instregex "ROR(16|32|64)mCL")>;
def: InstRW<[SKLWriteResGroup81], (instregex "ROR8mCL")>;

def SKLWriteResGroup82 : SchedWriteRes<[SKLPort23,SKLPort237,SKLPort06,SKLPort0156]> {
  let Latency = 3;
  let NumMicroOps = 5;
  let ResourceCycles = [1,1,1,2];
}
def: InstRW<[SKLWriteResGroup82], (instregex "RCL(16|32|64)m1")>;
def: InstRW<[SKLWriteResGroup82], (instregex "RCL(16|32|64)mi")>;
def: InstRW<[SKLWriteResGroup82], (instregex "RCL8m1")>;
def: InstRW<[SKLWriteResGroup82], (instregex "RCL8mi")>;
def: InstRW<[SKLWriteResGroup82], (instregex "RCR(16|32|64)m1")>;
def: InstRW<[SKLWriteResGroup82], (instregex "RCR(16|32|64)mi")>;
def: InstRW<[SKLWriteResGroup82], (instregex "RCR8m1")>;
def: InstRW<[SKLWriteResGroup82], (instregex "RCR8mi")>;

def SKLWriteResGroup83 : SchedWriteRes<[SKLPort4,SKLPort23,SKLPort237,SKLPort06]> {
  let Latency = 3;
  let NumMicroOps = 6;
  let ResourceCycles = [1,1,1,3];
}
def: InstRW<[SKLWriteResGroup83], (instregex "ROL(16|32|64)mCL")>;
def: InstRW<[SKLWriteResGroup83], (instregex "ROL8mCL")>;
def: InstRW<[SKLWriteResGroup83], (instregex "SAR(16|32|64)mCL")>;
def: InstRW<[SKLWriteResGroup83], (instregex "SAR8mCL")>;
def: InstRW<[SKLWriteResGroup83], (instregex "SHL(16|32|64)mCL")>;
def: InstRW<[SKLWriteResGroup83], (instregex "SHL8mCL")>;
def: InstRW<[SKLWriteResGroup83], (instregex "SHR(16|32|64)mCL")>;
def: InstRW<[SKLWriteResGroup83], (instregex "SHR8mCL")>;

def SKLWriteResGroup84 : SchedWriteRes<[SKLPort4,SKLPort23,SKLPort237,SKLPort0156]> {
  let Latency = 3;
  let NumMicroOps = 6;
  let ResourceCycles = [1,1,1,3];
}
def: InstRW<[SKLWriteResGroup84], (instregex "ADC(16|32|64)mi8")>;
def: InstRW<[SKLWriteResGroup84], (instregex "ADC8mi")>;

def SKLWriteResGroup85 : SchedWriteRes<[SKLPort4,SKLPort23,SKLPort237,SKLPort06,SKLPort0156]> {
  let Latency = 3;
  let NumMicroOps = 6;
  let ResourceCycles = [1,1,1,2,1];
}
def: InstRW<[SKLWriteResGroup85], (instregex "ADC(16|32|64)mr")>;
def: InstRW<[SKLWriteResGroup85], (instregex "ADC8mr")>;
def: InstRW<[SKLWriteResGroup85], (instregex "CMPXCHG(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup85], (instregex "CMPXCHG8rm")>;
def: InstRW<[SKLWriteResGroup85], (instregex "SBB(16|32|64)mi8")>;
def: InstRW<[SKLWriteResGroup85], (instregex "SBB(16|32|64)mr")>;
def: InstRW<[SKLWriteResGroup85], (instregex "SBB8mi")>;
def: InstRW<[SKLWriteResGroup85], (instregex "SBB8mr")>;

def SKLWriteResGroup86 : SchedWriteRes<[SKLPort0]> {
  let Latency = 4;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup86], (instregex "AESDECLASTrr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "AESDECrr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "AESENCLASTrr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "AESENCrr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "MMX_PMADDUBSWrr64")>;
def: InstRW<[SKLWriteResGroup86], (instregex "MMX_PMADDWDirr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "MMX_PMULHRSWrr64")>;
def: InstRW<[SKLWriteResGroup86], (instregex "MMX_PMULHUWirr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "MMX_PMULHWirr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "MMX_PMULLWirr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "MMX_PMULUDQirr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "MUL_FPrST0")>;
def: InstRW<[SKLWriteResGroup86], (instregex "MUL_FST0r")>;
def: InstRW<[SKLWriteResGroup86], (instregex "MUL_FrST0")>;
def: InstRW<[SKLWriteResGroup86], (instregex "RCPPSr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "RCPSSr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "RSQRTPSr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "RSQRTSSr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VAESDECLASTrr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VAESDECrr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VAESENCLASTrr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VAESENCrr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VRCP14PDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VRCP14PDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VRCP14PSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VRCP14PSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VRCP14SDrr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VRCP14SSrr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VRCPPSYr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VRCPPSr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VRCPSSr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VRSQRT14PDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VRSQRT14PDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VRSQRT14PSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VRSQRT14PSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VRSQRT14SDrr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VRSQRT14SSrr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VRSQRTPSYr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VRSQRTPSr")>;
def: InstRW<[SKLWriteResGroup86], (instregex "VRSQRTSSr")>;

def SKLWriteResGroup87 : SchedWriteRes<[SKLPort01]> {
  let Latency = 4;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup87], (instregex "ADDPDrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "ADDPSrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "ADDSDrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "ADDSSrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "ADDSUBPDrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "ADDSUBPSrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "MULPDrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "MULPSrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "MULSDrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "MULSSrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "SUBPDrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "SUBPSrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "SUBSDrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "SUBSSrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VADDPDYrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VADDPDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VADDPDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VADDPDrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VADDPSYrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VADDPSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VADDPSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VADDPSrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VADDSDrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VADDSSrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VADDSUBPDYrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VADDSUBPDrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VADDSUBPSYrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VADDSUBPSrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD132PDYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD132PDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD132PDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD132PDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD132PSYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD132PSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD132PSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD132PSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD132SDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD132SSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD213PDYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD213PDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD213PDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD213PDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD213PSYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD213PSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD213PSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD213PSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD213SDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD213SSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD231PDYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD231PDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD231PDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD231PDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD231PSYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD231PSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD231PSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD231PSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD231SDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADD231SSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB132PDYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB132PDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB132PDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB132PDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB132PSYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB132PSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB132PSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB132PSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB213PDYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB213PDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB213PDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB213PDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB213PSYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB213PSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB213PSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB213PSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB231PDYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB231PDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB231PDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB231PDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB231PSYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB231PSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB231PSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMADDSUB231PSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB132PDYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB132PDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB132PDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB132PDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB132PSYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB132PSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB132PSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB132PSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB132SDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB132SSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB213PDYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB213PDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB213PDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB213PDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB213PSYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB213PSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB213PSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB213PSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB213SDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB213SSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB231PDYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB231PDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB231PDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB231PDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB231PSYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB231PSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB231PSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB231PSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB231SDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUB231SSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD132PDYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD132PDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD132PDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD132PDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD132PSYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD132PSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD132PSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD132PSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD213PDYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD213PDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD213PDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD213PDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD213PSYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD213PSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD213PSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD213PSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD231PDYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD231PDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD231PDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD231PDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD231PSYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD231PSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD231PSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFMSUBADD231PSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD132PDYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD132PDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD132PDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD132PDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD132PSYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD132PSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD132PSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD132PSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD132SDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD132SSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD213PDYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD213PDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD213PDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD213PDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD213PSYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD213PSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD213PSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD213PSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD213SDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD213SSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD231PDYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD231PDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD231PDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD231PDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD231PSYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD231PSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD231PSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD231PSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD231SDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMADD231SSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB132PDYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB132PDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB132PDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB132PDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB132PSYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB132PSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB132PSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB132PSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB132SDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB132SSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB213PDYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB213PDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB213PDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB213PDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB213PSYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB213PSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB213PSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB213PSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB213SDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB213SSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB231PDYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB231PDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB231PDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB231PDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB231PSYr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB231PSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB231PSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB231PSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB231SDr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VFNMSUB231SSr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VMULPDYrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VMULPDrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VMULPSYrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VMULPSrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VMULSDrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VMULSSrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VPMADDUBSWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VPMADDUBSWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VPMADDWDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VPMADDWDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VPMULDQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VPMULDQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VPMULHRSWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VPMULHRSWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VPMULHUWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VPMULHUWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VPMULHWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VPMULHWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VPMULLWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VPMULUDQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VPMULUDQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VSUBPDYrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VSUBPDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VSUBPDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VSUBPDrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VSUBPSYrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VSUBPSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VSUBPSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VSUBPSrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VSUBSDrr")>;
def: InstRW<[SKLWriteResGroup87], (instregex "VSUBSSrr")>;

def SKLWriteResGroup88 : SchedWriteRes<[SKLPort05]> {
  let Latency = 4;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}

def SKLWriteResGroup89 : SchedWriteRes<[SKLPort015]> {
  let Latency = 4;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup89], (instregex "CMPPDrri")>;
def: InstRW<[SKLWriteResGroup89], (instregex "CMPPSrri")>;
def: InstRW<[SKLWriteResGroup89], (instregex "CMPSSrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "CVTDQ2PSrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "CVTPS2DQrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "CVTTPS2DQrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "MAXPDrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "MAXPSrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "MAXSDrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "MAXSSrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "MINPDrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "MINPSrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "MINSDrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "MINSSrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "PHMINPOSUWrr128")>;
def: InstRW<[SKLWriteResGroup89], (instregex "PMADDUBSWrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "PMADDWDrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "PMULDQrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "PMULHRSWrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "PMULHUWrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "PMULHWrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "PMULLWrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "PMULUDQrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCMPPDYrri")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCMPPDrri")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCMPPSYrri")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCMPPSrri")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCMPSDrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCMPSSrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTDQ2PSYrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTDQ2PSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTDQ2PSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTDQ2PSrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTPD2QQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTPD2QQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTPD2UQQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTPD2UQQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTPS2DQYrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTPS2DQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTPS2DQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTPS2DQrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTPS2UDQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTPS2UDQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTQQ2PDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTQQ2PDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTTPD2QQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTTPD2QQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTTPD2UQQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTTPD2UQQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTTPS2DQYrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTTPS2DQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTTPS2DQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTTPS2DQrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTTPS2UDQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTTPS2UDQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTUDQ2PSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTUDQ2PSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTUQQ2PDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VCVTUQQ2PDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VFIXUPIMMPDZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VFIXUPIMMPDZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VFIXUPIMMPSZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VFIXUPIMMPSZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VFIXUPIMMSDrri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VFIXUPIMMSSrri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VGETEXPPDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VGETEXPPDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VGETEXPPDr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VGETEXPPSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VGETEXPPSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VGETEXPPSr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VGETEXPSDr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VGETEXPSSr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VGETMANTPDZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VGETMANTPDZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VGETMANTPSZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VGETMANTPSZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VGETMANTSDZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VGETMANTSSZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMAXPDYrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMAXPDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMAXPDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMAXPDrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMAXPSYrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMAXPSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMAXPSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMAXPSrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMAXSDrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMAXSSrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMINPDYrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMINPDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMINPDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMINPDrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMINPSYrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMINPSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMINPSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMINPSrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMINSDrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMINSSrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMULPDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMULPDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMULPSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VMULPSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VPHMINPOSUWrr128")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VPLZCNTDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VPLZCNTDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VPLZCNTQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VPLZCNTQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VPMADDUBSWYrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VPMADDUBSWrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VPMADDWDYrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VPMADDWDrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VPMULDQYrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VPMULDQrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VPMULHRSWYrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VPMULHRSWrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VPMULHUWYrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VPMULHUWrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VPMULHWYrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VPMULHWrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VPMULLWYrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VPMULLWrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VPMULUDQYrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VPMULUDQrr")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VRANGEPDZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VRANGEPDZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VRANGEPSZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VRANGEPSZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VRANGESDZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VRANGESSZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VREDUCEPDZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VREDUCEPDZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VREDUCEPSZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VREDUCEPSZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VREDUCESDZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VREDUCESSZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VSCALEFPDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VSCALEFPDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VSCALEFPSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VSCALEFPSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VSCALEFSDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup89], (instregex "VSCALEFSSZ128rr(b?)(k?)(z?)")>;

def SKLWriteResGroup90 : SchedWriteRes<[SKLPort5]> {
  let Latency = 4;
  let NumMicroOps = 2;
  let ResourceCycles = [2];
}
def: InstRW<[SKLWriteResGroup90], (instregex "MPSADBWrri")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VEXPANDPDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VEXPANDPDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VEXPANDPSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VEXPANDPSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VMPSADBWYrri")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VMPSADBWrri")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPEXPANDDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPEXPANDDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPEXPANDQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPEXPANDQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVDBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVDBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVDWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVDWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVQBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVQBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVQWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVQWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVSDBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVSDBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVSDWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVSDWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVSQBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVSQBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVSQDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVSQDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVSQWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVSQWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVSWBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVSWBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVUSDBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVUSDBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVUSDWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVUSDWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVUSQBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVUSQBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVUSQDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVUSQDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVUSQWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVUSQWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVUSWBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVUSWBZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVWBZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup90], (instregex "VPMOVWBZ256rr(b?)(k?)(z?)")>;

def SKLWriteResGroup91 : SchedWriteRes<[SKLPort0,SKLPort23]> {
  let Latency = 4;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup91], (instregex "AESDECLASTrm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "AESDECrm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "AESENCLASTrm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "AESENCrm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "MMX_CVTPI2PSirm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "MMX_PMADDUBSWrm64")>;
def: InstRW<[SKLWriteResGroup91], (instregex "MMX_PMADDWDirm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "MMX_PMULHRSWrm64")>;
def: InstRW<[SKLWriteResGroup91], (instregex "MMX_PMULHUWirm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "MMX_PMULHWirm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "MMX_PMULLWirm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "MMX_PMULUDQirm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "MUL_F32m")>;
def: InstRW<[SKLWriteResGroup91], (instregex "MUL_F64m")>;
def: InstRW<[SKLWriteResGroup91], (instregex "RCPPSm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "RCPSSm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "RSQRTPSm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "RSQRTSSm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VAESDECLASTrm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VAESDECrm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VAESENCLASTrm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VAESENCrm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VRCP14PDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VRCP14PDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VRCP14PSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VRCP14PSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VRCP14SDrm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VRCP14SSrm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VRCPPSYm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VRCPPSm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VRCPSSm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VRSQRT14PDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VRSQRT14PDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VRSQRT14PSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VRSQRT14PSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VRSQRT14SDrm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VRSQRT14SSrm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VRSQRTPSYm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VRSQRTPSm")>;
def: InstRW<[SKLWriteResGroup91], (instregex "VRSQRTSSm")>;

def SKLWriteResGroup92 : SchedWriteRes<[SKLPort1,SKLPort5]> {
  let Latency = 4;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup92], (instregex "IMUL64r")>;
def: InstRW<[SKLWriteResGroup92], (instregex "MUL64r")>;
def: InstRW<[SKLWriteResGroup92], (instregex "MULX64rr")>;

def SKLWriteResGroup92_16 : SchedWriteRes<[SKLPort1,SKLPort06,SKLPort0156]> {
  let Latency = 4;
  let NumMicroOps = 4;  
}
def: InstRW<[SKLWriteResGroup92_16], (instregex "IMUL16r")>;
def: InstRW<[SKLWriteResGroup92_16], (instregex "MUL16r")>;

def SKLWriteResGroup92_32 : SchedWriteRes<[SKLPort1,SKLPort06,SKLPort0156]> {
  let Latency = 4;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup92_32], (instregex "IMUL32r")>;
def: InstRW<[SKLWriteResGroup92_32], (instregex "MUL32r")>;
def: InstRW<[SKLWriteResGroup92_32], (instregex "MULX32rr")>;

def SKLWriteResGroup93 : SchedWriteRes<[SKLPort5,SKLPort01]> {
  let Latency = 4;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup93], (instregex "VPSLLDYrr")>;
def: InstRW<[SKLWriteResGroup93], (instregex "VPSLLDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup93], (instregex "VPSLLQYrr")>;
def: InstRW<[SKLWriteResGroup93], (instregex "VPSLLQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup93], (instregex "VPSLLWYrr")>;
def: InstRW<[SKLWriteResGroup93], (instregex "VPSLLWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup93], (instregex "VPSRADYrr")>;
def: InstRW<[SKLWriteResGroup93], (instregex "VPSRADZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup93], (instregex "VPSRAQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup93], (instregex "VPSRAWYrr")>;
def: InstRW<[SKLWriteResGroup93], (instregex "VPSRAWZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup93], (instregex "VPSRLDYrr")>;
def: InstRW<[SKLWriteResGroup93], (instregex "VPSRLDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup93], (instregex "VPSRLQYrr")>;
def: InstRW<[SKLWriteResGroup93], (instregex "VPSRLQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup93], (instregex "VPSRLWYrr")>;
def: InstRW<[SKLWriteResGroup93], (instregex "VPSRLWZ256rr(b?)(k?)(z?)")>;

def SKLWriteResGroup94 : SchedWriteRes<[SKLPort01,SKLPort23]> {
  let Latency = 4;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup94], (instregex "ADDPDrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "ADDPSrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "ADDSDrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "ADDSSrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "ADDSUBPDrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "ADDSUBPSrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "MULPDrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "MULPSrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "MULSDrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "MULSSrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "SUBPDrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "SUBPSrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "SUBSDrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "SUBSSrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VADDPDYrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VADDPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VADDPDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VADDPDrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VADDPSYrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VADDPSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VADDPSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VADDPSrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VADDSDrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VADDSSrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VADDSUBPDYrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VADDSUBPDrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VADDSUBPSYrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VADDSUBPSrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD132PDYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD132PDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD132PDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD132PDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD132PSYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD132PSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD132PSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD132PSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD132SDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD132SSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD213PDYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD213PDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD213PDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD213PDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD213PSYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD213PSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD213PSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD213PSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD213SDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD213SSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD231PDYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD231PDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD231PDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD231PDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD231PSYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD231PSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD231PSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD231PSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD231SDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADD231SSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB132PDYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB132PDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB132PDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB132PDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB132PSYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB132PSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB132PSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB132PSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB213PDYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB213PDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB213PDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB213PDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB213PSYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB213PSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB213PSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB213PSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB231PDYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB231PDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB231PDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB231PDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB231PSYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB231PSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB231PSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMADDSUB231PSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB132PDYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB132PDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB132PDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB132PDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB132PSYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB132PSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB132PSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB132PSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB132SDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB132SSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB213PDYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB213PDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB213PDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB213PDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB213PSYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB213PSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB213PSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB213PSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB213SDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB213SSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB231PDYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB231PDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB231PDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB231PDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB231PSYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB231PSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB231PSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB231PSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB231SDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUB231SSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD132PDYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD132PDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD132PDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD132PDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD132PSYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD132PSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD132PSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD132PSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD213PDYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD213PDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD213PDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD213PDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD213PSYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD213PSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD213PSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD213PSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD231PDYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD231PDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD231PDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD231PDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD231PSYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD231PSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD231PSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFMSUBADD231PSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD132PDYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD132PDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD132PDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD132PDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD132PSYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD132PSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD132PSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD132PSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD132SDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD132SSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD213PDYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD213PDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD213PDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD213PDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD213PSYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD213PSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD213PSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD213PSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD213SDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD213SSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD231PDYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD231PDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD231PDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD231PDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD231PSYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD231PSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD231PSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD231PSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD231SDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMADD231SSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB132PDYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB132PDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB132PDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB132PDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB132PSYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB132PSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB132PSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB132PSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB132SDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB132SSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB213PDYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB213PDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB213PDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB213PDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB213PSYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB213PSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB213PSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB213PSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB213SDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB213SSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB231PDYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB231PDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB231PDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB231PDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB231PSYm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB231PSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB231PSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB231PSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB231SDm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VFNMSUB231SSm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VMULPDYrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VMULPDrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VMULPSYrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VMULPSrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VMULSDrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VMULSSrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VPMADDUBSWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VPMADDUBSWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VPMADDWDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VPMADDWDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VPMULDQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VPMULDQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VPMULHRSWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VPMULHRSWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VPMULHUWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VPMULHUWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VPMULHWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VPMULHWZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VPMULLWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VPMULUDQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VPMULUDQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VSUBPDYrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VSUBPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VSUBPDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VSUBPDrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VSUBPSYrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VSUBPSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VSUBPSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VSUBPSrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VSUBSDrm")>;
def: InstRW<[SKLWriteResGroup94], (instregex "VSUBSSrm")>;

def SKLWriteResGroup95 : SchedWriteRes<[SKLPort23,SKLPort05]> {
  let Latency = 4;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}

def SKLWriteResGroup96 : SchedWriteRes<[SKLPort23,SKLPort015]> {
  let Latency = 4;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup96], (instregex "CMPPDrmi")>;
def: InstRW<[SKLWriteResGroup96], (instregex "CMPPSrmi")>;
def: InstRW<[SKLWriteResGroup96], (instregex "CMPSSrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "CVTDQ2PSrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "CVTPS2DQrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "CVTPS2PDrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "CVTSS2SDrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "CVTTPS2DQrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "MAXPDrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "MAXPSrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "MAXSDrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "MAXSSrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "MINPDrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "MINPSrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "MINSDrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "MINSSrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "MMX_CVTPS2PIirm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "MMX_CVTTPS2PIirm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "PHMINPOSUWrm128")>;
def: InstRW<[SKLWriteResGroup96], (instregex "PMADDUBSWrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "PMADDWDrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "PMULDQrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "PMULHRSWrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "PMULHUWrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "PMULHWrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "PMULLWrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "PMULUDQrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCMPPDYrmi")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCMPPDrmi")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCMPPSYrmi")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCMPPSrmi")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCMPSDrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCMPSSrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTDQ2PDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTDQ2PDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTDQ2PSYrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTDQ2PSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTDQ2PSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTDQ2PSrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPD2QQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPD2QQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPD2UQQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPD2UQQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPH2PSYrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPH2PSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPH2PSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPH2PSrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPS2DQYrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPS2DQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPS2DQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPS2DQrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPS2PDYrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPS2PDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPS2PDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPS2PDrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPS2QQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPS2QQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPS2UDQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPS2UDQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPS2UQQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTPS2UQQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTQQ2PDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTQQ2PDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTSS2SDrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTTPD2QQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTTPD2QQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTTPD2UQQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTTPD2UQQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTTPS2DQYrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTTPS2DQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTTPS2DQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTTPS2DQrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTTPS2QQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTTPS2QQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTTPS2UDQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTTPS2UDQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTTPS2UQQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTTPS2UQQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTUDQ2PDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTUDQ2PDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTUDQ2PSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTUDQ2PSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTUQQ2PDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VCVTUQQ2PDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VFIXUPIMMPDZ128rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VFIXUPIMMPDZ256rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VFIXUPIMMPSZ128rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VFIXUPIMMPSZ256rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VFIXUPIMMSDrmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VFIXUPIMMSSrmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VGETEXPPDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VGETEXPPDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VGETEXPPDm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VGETEXPPSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VGETEXPPSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VGETEXPPSm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VGETEXPSDm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VGETEXPSSm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VGETMANTPDZ128rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VGETMANTPDZ256rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VGETMANTPSZ128rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VGETMANTPSZ256rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VGETMANTSDZ128rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VGETMANTSSZ128rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMAXPDYrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMAXPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMAXPDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMAXPDrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMAXPSYrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMAXPSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMAXPSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMAXPSrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMAXSDrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMAXSSrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMINPDYrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMINPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMINPDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMINPDrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMINPSYrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMINPSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMINPSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMINPSrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMINSDrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMINSSrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMULPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMULPDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMULPSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VMULPSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VPHMINPOSUWrm128")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VPLZCNTDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VPLZCNTDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VPLZCNTQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VPLZCNTQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VPMADDUBSWYrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VPMADDUBSWrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VPMADDWDYrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VPMADDWDrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VPMULDQYrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VPMULDQrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VPMULHRSWYrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VPMULHRSWrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VPMULHUWYrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VPMULHUWrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VPMULHWYrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VPMULHWrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VPMULLWYrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VPMULLWrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VPMULUDQYrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VPMULUDQrm")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VRANGEPDZ128rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VRANGEPDZ256rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VRANGEPSZ128rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VRANGEPSZ256rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VRANGESDZ128rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VRANGESSZ128rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VREDUCEPDZ128rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VREDUCEPDZ256rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VREDUCEPSZ128rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VREDUCEPSZ256rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VREDUCESDZ128rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VREDUCESSZ128rmi(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VSCALEFPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VSCALEFPDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VSCALEFPSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VSCALEFPSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VSCALEFSDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup96], (instregex "VSCALEFSSZ128rm(b?)(k?)(z?)")>;

def SKLWriteResGroup97 : SchedWriteRes<[SKLPort5,SKLPort23]> {
  let Latency = 4;
  let NumMicroOps = 3;
  let ResourceCycles = [2,1];
}
def: InstRW<[SKLWriteResGroup97], (instregex "FICOM16m")>;
def: InstRW<[SKLWriteResGroup97], (instregex "FICOM32m")>;
def: InstRW<[SKLWriteResGroup97], (instregex "FICOMP16m")>;
def: InstRW<[SKLWriteResGroup97], (instregex "FICOMP32m")>;
def: InstRW<[SKLWriteResGroup97], (instregex "MPSADBWrmi")>;
def: InstRW<[SKLWriteResGroup97], (instregex "VEXPANDPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup97], (instregex "VEXPANDPDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup97], (instregex "VEXPANDPSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup97], (instregex "VEXPANDPSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup97], (instregex "VMPSADBWYrmi")>;
def: InstRW<[SKLWriteResGroup97], (instregex "VMPSADBWrmi")>;
def: InstRW<[SKLWriteResGroup97], (instregex "VPEXPANDDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup97], (instregex "VPEXPANDDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup97], (instregex "VPEXPANDQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup97], (instregex "VPEXPANDQZ256rm(b?)(k?)(z?)")>;

def SKLWriteResGroup98 : SchedWriteRes<[SKLPort1,SKLPort5,SKLPort23]> {
  let Latency = 4;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup98], (instregex "MULX64rm")>;

def SKLWriteResGroup99 : SchedWriteRes<[SKLPort4,SKLPort237,SKLPort015]> {
  let Latency = 4;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup99], (instregex "VCVTPS2PHZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup99], (instregex "VCVTPS2PHZ256mr(b?)(k?)(z?)")>;

def SKLWriteResGroup100 : SchedWriteRes<[SKLPort0156]> {
  let Latency = 4;
  let NumMicroOps = 4;
  let ResourceCycles = [4];
}
def: InstRW<[SKLWriteResGroup100], (instregex "FNCLEX")>;

def SKLWriteResGroup101 : SchedWriteRes<[SKLPort6,SKLPort0156]> {
  let Latency = 4;
  let NumMicroOps = 4;
  let ResourceCycles = [1,3];
}
def: InstRW<[SKLWriteResGroup101], (instregex "PAUSE")>;

def SKLWriteResGroup102 : SchedWriteRes<[SKLPort015,SKLPort0156]> {
  let Latency = 4;
  let NumMicroOps = 4;
  let ResourceCycles = [1,3];
}
def: InstRW<[SKLWriteResGroup102], (instregex "VZEROUPPER")>;

def SKLWriteResGroup103 : SchedWriteRes<[SKLPort1,SKLPort6,SKLPort0156]> {
  let Latency = 4;
  let NumMicroOps = 4;
  let ResourceCycles = [1,1,2];
}
def: InstRW<[SKLWriteResGroup103], (instregex "LAR(16|32|64)rr")>;

def SKLWriteResGroup104 : SchedWriteRes<[SKLPort4,SKLPort5,SKLPort237]> {
  let Latency = 4;
  let NumMicroOps = 4;
  let ResourceCycles = [1,2,1];
}
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVDBZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVDBZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVDWZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVDWZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVQBZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVQBZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVQWZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVQWZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVSDBZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVSDBZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVSDWZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVSDWZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVSQBZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVSQBZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVSQDZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVSQDZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVSQWZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVSQWZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVSWBZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVSWBZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVUSDBZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVUSDBZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVUSDWZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVUSDWZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVUSQBZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVUSQBZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVUSQDZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVUSQDZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVUSQWZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVUSQWZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVUSWBZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVUSWBZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVWBZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup104], (instregex "VPMOVWBZ256mr(b?)(k?)(z?)")>;

def SKLWriteResGroup105 : SchedWriteRes<[SKLPort1,SKLPort23,SKLPort237,SKLPort0156]> {
  let Latency = 4;
  let NumMicroOps = 4;
  let ResourceCycles = [1,1,1,1];
}
def: InstRW<[SKLWriteResGroup105], (instregex "SHLD(16|32|64)mri8")>;
def: InstRW<[SKLWriteResGroup105], (instregex "SHRD(16|32|64)mri8")>;

def SKLWriteResGroup106 : SchedWriteRes<[SKLPort1,SKLPort6,SKLPort23,SKLPort0156]> {
  let Latency = 4;
  let NumMicroOps = 5;
  let ResourceCycles = [1,2,1,1];
}
def: InstRW<[SKLWriteResGroup106], (instregex "LAR(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup106], (instregex "LSL(16|32|64)rm")>;

def SKLWriteResGroup107 : SchedWriteRes<[SKLPort4,SKLPort237,SKLPort0156]> {
  let Latency = 4;
  let NumMicroOps = 6;
  let ResourceCycles = [1,1,4];
}
def: InstRW<[SKLWriteResGroup107], (instregex "PUSHF16")>;
def: InstRW<[SKLWriteResGroup107], (instregex "PUSHF64")>;

def SKLWriteResGroup108 : SchedWriteRes<[SKLPort015]> {
  let Latency = 5;
  let NumMicroOps = 2;
  let ResourceCycles = [2];
}

def SKLWriteResGroup109 : SchedWriteRes<[SKLPort0,SKLPort5]> {
  let Latency = 5;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup109], (instregex "CVTDQ2PDrr")>;
def: InstRW<[SKLWriteResGroup109], (instregex "MMX_CVTPI2PDirr")>;
def: InstRW<[SKLWriteResGroup109], (instregex "VCVTDQ2PDrr")>;

def SKLWriteResGroup110 : SchedWriteRes<[SKLPort5,SKLPort015]> {
  let Latency = 5;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup110], (instregex "CVTPD2DQrr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "CVTPD2PSrr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "CVTPS2PDrr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "CVTSD2SSrr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "CVTSI2SD64rr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "CVTSI2SDrr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "CVTSI2SSrr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "CVTSS2SDrr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "CVTTPD2DQrr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "MMX_CVTPD2PIirr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "MMX_CVTPS2PIirr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "MMX_CVTTPD2PIirr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "MMX_CVTTPS2PIirr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTDQ2PDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTPD2DQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTPD2DQrr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTPD2PSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTPD2PSrr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTPD2UDQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTPH2PSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTPH2PSrr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTPS2PDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTPS2PDrr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTPS2PHZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTPS2PHrr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTPS2QQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTPS2UQQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTQQ2PSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTSD2SSrr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTSI2SD64rr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTSI2SDrr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTSI2SSrr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTSS2SDrr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTTPD2DQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTTPD2DQrr")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTTPD2UDQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTTPS2QQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTTPS2UQQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTUDQ2PDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup110], (instregex "VCVTUQQ2PSZ128rr(b?)(k?)(z?)")>;

def SKLWriteResGroup111 : SchedWriteRes<[SKLPort5,SKLPort015]> {
  let Latency = 5;
  let NumMicroOps = 3;
  let ResourceCycles = [2,1];
}
def: InstRW<[SKLWriteResGroup111], (instregex "VPCONFLICTQZ128rr(b?)(k?)(z?)")>;

def SKLWriteResGroup112 : SchedWriteRes<[SKLPort23,SKLPort015]> {
  let Latency = 5;
  let NumMicroOps = 3;
  let ResourceCycles = [1,2];
}

def SKLWriteResGroup113 : SchedWriteRes<[SKLPort0,SKLPort5,SKLPort23]> {
  let Latency = 5;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup113], (instregex "CVTDQ2PDrm")>;
def: InstRW<[SKLWriteResGroup113], (instregex "MMX_CVTPI2PDirm")>;
def: InstRW<[SKLWriteResGroup113], (instregex "VCVTDQ2PDrm")>;

def SKLWriteResGroup114 : SchedWriteRes<[SKLPort1,SKLPort6,SKLPort06]> {
  let Latency = 5;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup114], (instregex "STR(16|32|64)r")>;

def SKLWriteResGroup116 : SchedWriteRes<[SKLPort5,SKLPort23,SKLPort015]> {
  let Latency = 5;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup116], (instregex "CVTPD2DQrm")>;
def: InstRW<[SKLWriteResGroup116], (instregex "CVTPD2PSrm")>;
def: InstRW<[SKLWriteResGroup116], (instregex "CVTSD2SSrm")>;
def: InstRW<[SKLWriteResGroup116], (instregex "CVTTPD2DQrm")>;
def: InstRW<[SKLWriteResGroup116], (instregex "MMX_CVTPD2PIirm")>;
def: InstRW<[SKLWriteResGroup116], (instregex "MMX_CVTTPD2PIirm")>;
def: InstRW<[SKLWriteResGroup116], (instregex "VCVTSD2SSrm")>;

def SKLWriteResGroup117 : SchedWriteRes<[SKLPort5,SKLPort23,SKLPort015]> {
  let Latency = 5;
  let NumMicroOps = 4;
  let ResourceCycles = [2,1,1];
}
def: InstRW<[SKLWriteResGroup117], (instregex "VPCONFLICTQZ128rm(b?)(k?)(z?)")>;

def SKLWriteResGroup118 : SchedWriteRes<[SKLPort1,SKLPort23,SKLPort06,SKLPort0156]> {
  let Latency = 5;
  let NumMicroOps = 4;
  let ResourceCycles = [1,1,1,1];
}
def: InstRW<[SKLWriteResGroup118], (instregex "MULX32rm")>;

def SKLWriteResGroup119 : SchedWriteRes<[SKLPort4,SKLPort5,SKLPort237,SKLPort015]> {
  let Latency = 5;
  let NumMicroOps = 4;
  let ResourceCycles = [1,1,1,1];
}
def: InstRW<[SKLWriteResGroup119], (instregex "VCVTPS2PHmr")>;

def SKLWriteResGroup120 : SchedWriteRes<[SKLPort06,SKLPort0156]> {
  let Latency = 5;
  let NumMicroOps = 5;
  let ResourceCycles = [1,4];
}
def: InstRW<[SKLWriteResGroup120], (instregex "XSETBV")>;

def SKLWriteResGroup121 : SchedWriteRes<[SKLPort06,SKLPort0156]> {
  let Latency = 5;
  let NumMicroOps = 5;
  let ResourceCycles = [2,3];
}
def: InstRW<[SKLWriteResGroup121], (instregex "CMPXCHG(16|32|64)rr")>;
def: InstRW<[SKLWriteResGroup121], (instregex "CMPXCHG8rr")>;

def SKLWriteResGroup122 : SchedWriteRes<[SKLPort4,SKLPort6,SKLPort23,SKLPort237,SKLPort06,SKLPort0156]> {
  let Latency = 5;
  let NumMicroOps = 8;
  let ResourceCycles = [1,1,1,1,1,3];
}
def: InstRW<[SKLWriteResGroup122], (instregex "ADD8mi")>;
def: InstRW<[SKLWriteResGroup122], (instregex "AND8mi")>;
def: InstRW<[SKLWriteResGroup122], (instregex "OR8mi")>;
def: InstRW<[SKLWriteResGroup122], (instregex "SUB8mi")>;
def: InstRW<[SKLWriteResGroup122], (instregex "XCHG(16|32|64)rm")>;
def: InstRW<[SKLWriteResGroup122], (instregex "XCHG8rm")>;
def: InstRW<[SKLWriteResGroup122], (instregex "XOR8mi")>;

def SKLWriteResGroup123 : SchedWriteRes<[SKLPort5]> {
  let Latency = 6;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup123], (instregex "PCLMULQDQrr")>;
def: InstRW<[SKLWriteResGroup123], (instregex "VPCLMULQDQrr")>;

def SKLWriteResGroup124 : SchedWriteRes<[SKLPort0]> {
  let Latency = 6;
  let NumMicroOps = 2;
  let ResourceCycles = [2];
}
def: InstRW<[SKLWriteResGroup124], (instregex "MMX_CVTPI2PSirr")>;
def: InstRW<[SKLWriteResGroup124], (instregex "VCOMPRESSPDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup124], (instregex "VCOMPRESSPDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup124], (instregex "VCOMPRESSPSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup124], (instregex "VCOMPRESSPSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup124], (instregex "VPCOMPRESSDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup124], (instregex "VPCOMPRESSDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup124], (instregex "VPCOMPRESSQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup124], (instregex "VPCOMPRESSQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup124], (instregex "VPERMWZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup124], (instregex "VPERMWZ256rr(b?)(k?)(z?)")>;

def SKLWriteResGroup125 : SchedWriteRes<[SKLPort0,SKLPort015]> {
  let Latency = 6;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup125], (instregex "CVTSD2SI64rr")>;
def: InstRW<[SKLWriteResGroup125], (instregex "CVTSD2SIrr")>;
def: InstRW<[SKLWriteResGroup125], (instregex "CVTSS2SI64rr")>;
def: InstRW<[SKLWriteResGroup125], (instregex "CVTSS2SIrr")>;
def: InstRW<[SKLWriteResGroup125], (instregex "CVTTSD2SI64rr")>;
def: InstRW<[SKLWriteResGroup125], (instregex "CVTTSD2SIrr")>;
def: InstRW<[SKLWriteResGroup125], (instregex "VCVTSD2SI64rr")>;
def: InstRW<[SKLWriteResGroup125], (instregex "VCVTSD2SIrr")>;
def: InstRW<[SKLWriteResGroup125], (instregex "VCVTSS2SI64rr")>;
def: InstRW<[SKLWriteResGroup125], (instregex "VCVTSS2SIrr")>;
def: InstRW<[SKLWriteResGroup125], (instregex "VCVTTSD2SI64rr")>;
def: InstRW<[SKLWriteResGroup125], (instregex "VCVTTSD2SIrr")>;

def SKLWriteResGroup126 : SchedWriteRes<[SKLPort5,SKLPort23]> {
  let Latency = 6;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup126], (instregex "PCLMULQDQrm")>;
def: InstRW<[SKLWriteResGroup126], (instregex "VPCLMULQDQrm")>;

def SKLWriteResGroup127 : SchedWriteRes<[SKLPort5,SKLPort01]> {
  let Latency = 6;
  let NumMicroOps = 3;
  let ResourceCycles = [2,1];
}
def: InstRW<[SKLWriteResGroup127], (instregex "HADDPDrr")>;
def: InstRW<[SKLWriteResGroup127], (instregex "HADDPSrr")>;
def: InstRW<[SKLWriteResGroup127], (instregex "HSUBPDrr")>;
def: InstRW<[SKLWriteResGroup127], (instregex "HSUBPSrr")>;
def: InstRW<[SKLWriteResGroup127], (instregex "VHADDPDYrr")>;
def: InstRW<[SKLWriteResGroup127], (instregex "VHADDPDrr")>;
def: InstRW<[SKLWriteResGroup127], (instregex "VHADDPSYrr")>;
def: InstRW<[SKLWriteResGroup127], (instregex "VHADDPSrr")>;
def: InstRW<[SKLWriteResGroup127], (instregex "VHSUBPDYrr")>;
def: InstRW<[SKLWriteResGroup127], (instregex "VHSUBPDrr")>;
def: InstRW<[SKLWriteResGroup127], (instregex "VHSUBPSYrr")>;
def: InstRW<[SKLWriteResGroup127], (instregex "VHSUBPSrr")>;

def SKLWriteResGroup128 : SchedWriteRes<[SKLPort5,SKLPort23]> {
  let Latency = 6;
  let NumMicroOps = 3;
  let ResourceCycles = [2,1];
}
def: InstRW<[SKLWriteResGroup128], (instregex "ADD_FI16m")>;
def: InstRW<[SKLWriteResGroup128], (instregex "ADD_FI32m")>;
def: InstRW<[SKLWriteResGroup128], (instregex "SUBR_FI16m")>;
def: InstRW<[SKLWriteResGroup128], (instregex "SUBR_FI32m")>;
def: InstRW<[SKLWriteResGroup128], (instregex "SUB_FI16m")>;
def: InstRW<[SKLWriteResGroup128], (instregex "SUB_FI32m")>;
def: InstRW<[SKLWriteResGroup128], (instregex "VPERMWZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup128], (instregex "VPERMWZ256rm(b?)(k?)(z?)")>;

def SKLWriteResGroup129 : SchedWriteRes<[SKLPort5,SKLPort015]> {
  let Latency = 6;
  let NumMicroOps = 3;
  let ResourceCycles = [2,1];
}
def: InstRW<[SKLWriteResGroup129], (instregex "CVTSI2SS64rr")>;
def: InstRW<[SKLWriteResGroup129], (instregex "VCVTSI2SS64rr")>;

def SKLWriteResGroup130 : SchedWriteRes<[SKLPort0,SKLPort23,SKLPort015]> {
  let Latency = 6;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup130], (instregex "CVTSD2SI64rm")>;
def: InstRW<[SKLWriteResGroup130], (instregex "CVTSD2SIrm")>;
def: InstRW<[SKLWriteResGroup130], (instregex "CVTSS2SI64rm")>;
def: InstRW<[SKLWriteResGroup130], (instregex "CVTSS2SIrm")>;
def: InstRW<[SKLWriteResGroup130], (instregex "CVTTSD2SI64rm")>;
def: InstRW<[SKLWriteResGroup130], (instregex "CVTTSD2SIrm")>;
def: InstRW<[SKLWriteResGroup130], (instregex "CVTTSS2SIrm")>;
def: InstRW<[SKLWriteResGroup130], (instregex "VCVTSD2SI64rm")>;
def: InstRW<[SKLWriteResGroup130], (instregex "VCVTSD2SIrm")>;
def: InstRW<[SKLWriteResGroup130], (instregex "VCVTSS2SI64rm")>;
def: InstRW<[SKLWriteResGroup130], (instregex "VCVTSS2SIrm")>;
def: InstRW<[SKLWriteResGroup130], (instregex "VCVTTSD2SI64rm")>;
def: InstRW<[SKLWriteResGroup130], (instregex "VCVTTSD2SIrm")>;
def: InstRW<[SKLWriteResGroup130], (instregex "VCVTTSS2SI64rm")>;
def: InstRW<[SKLWriteResGroup130], (instregex "VCVTTSS2SIrm")>;

def SKLWriteResGroup131 : SchedWriteRes<[SKLPort1,SKLPort06,SKLPort0156]> {
  let Latency = 6;
  let NumMicroOps = 4;
  let ResourceCycles = [1,2,1];
}
def: InstRW<[SKLWriteResGroup131], (instregex "SHLD(16|32|64)rrCL")>;
def: InstRW<[SKLWriteResGroup131], (instregex "SHRD(16|32|64)rrCL")>;

def SKLWriteResGroup132 : SchedWriteRes<[SKLPort4,SKLPort5,SKLPort237]> {
  let Latency = 6;
  let NumMicroOps = 4;
  let ResourceCycles = [1,2,1];
}
def: InstRW<[SKLWriteResGroup132], (instregex "VCOMPRESSPDZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup132], (instregex "VCOMPRESSPDZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup132], (instregex "VCOMPRESSPSZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup132], (instregex "VCOMPRESSPSZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup132], (instregex "VPCOMPRESSDZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup132], (instregex "VPCOMPRESSDZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup132], (instregex "VPCOMPRESSQZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup132], (instregex "VPCOMPRESSQZ256mr(b?)(k?)(z?)")>;

def SKLWriteResGroup133 : SchedWriteRes<[SKLPort5,SKLPort01,SKLPort23]> {
  let Latency = 6;
  let NumMicroOps = 4;
  let ResourceCycles = [2,1,1];
}
def: InstRW<[SKLWriteResGroup133], (instregex "HADDPDrm")>;
def: InstRW<[SKLWriteResGroup133], (instregex "HADDPSrm")>;
def: InstRW<[SKLWriteResGroup133], (instregex "HSUBPDrm")>;
def: InstRW<[SKLWriteResGroup133], (instregex "HSUBPSrm")>;
def: InstRW<[SKLWriteResGroup133], (instregex "VHADDPDYrm")>;
def: InstRW<[SKLWriteResGroup133], (instregex "VHADDPDrm")>;
def: InstRW<[SKLWriteResGroup133], (instregex "VHADDPSYrm")>;
def: InstRW<[SKLWriteResGroup133], (instregex "VHADDPSrm")>;
def: InstRW<[SKLWriteResGroup133], (instregex "VHSUBPDYrm")>;
def: InstRW<[SKLWriteResGroup133], (instregex "VHSUBPDrm")>;
def: InstRW<[SKLWriteResGroup133], (instregex "VHSUBPSYrm")>;
def: InstRW<[SKLWriteResGroup133], (instregex "VHSUBPSrm")>;

def SKLWriteResGroup134 : SchedWriteRes<[SKLPort1,SKLPort6,SKLPort06,SKLPort0156]> {
  let Latency = 6;
  let NumMicroOps = 4;
  let ResourceCycles = [1,1,1,1];
}
def: InstRW<[SKLWriteResGroup134], (instregex "SLDT(16|32|64)r")>;

def SKLWriteResGroup135 : SchedWriteRes<[SKLPort0,SKLPort23,SKLPort015,SKLPort0156]> {
  let Latency = 6;
  let NumMicroOps = 5;
  let ResourceCycles = [1,2,1,1];
}
def: InstRW<[SKLWriteResGroup135], (instregex "VGATHERDPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup135], (instregex "VGATHERDPDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup135], (instregex "VGATHERDPSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup135], (instregex "VGATHERQPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup135], (instregex "VGATHERQPDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup135], (instregex "VGATHERQPSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup135], (instregex "VGATHERQPSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup135], (instregex "VPGATHERDDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup135], (instregex "VPGATHERDQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup135], (instregex "VPGATHERDQZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup135], (instregex "VPGATHERQDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup135], (instregex "VPGATHERQDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup135], (instregex "VPGATHERQQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup135], (instregex "VPGATHERQQZ256rm(b?)(k?)(z?)")>;

def SKLWriteResGroup136 : SchedWriteRes<[SKLPort6,SKLPort0156]> {
  let Latency = 6;
  let NumMicroOps = 6;
  let ResourceCycles = [1,5];
}
def: InstRW<[SKLWriteResGroup136], (instregex "STD")>;

def SKLWriteResGroup137 : SchedWriteRes<[SKLPort1,SKLPort23,SKLPort237,SKLPort06,SKLPort0156]> {
  let Latency = 6;
  let NumMicroOps = 6;
  let ResourceCycles = [1,1,1,2,1];
}
def: InstRW<[SKLWriteResGroup137], (instregex "SHLD(16|32|64)mrCL")>;
def: InstRW<[SKLWriteResGroup137], (instregex "SHRD(16|32|64)mrCL")>;

def SKLWriteResGroup138 : SchedWriteRes<[SKLPort0,SKLPort4,SKLPort237,SKLPort0156]> {
  let Latency = 6;
  let NumMicroOps = 7;
  let ResourceCycles = [1,2,2,2];
}
def: InstRW<[SKLWriteResGroup138], (instregex "VPSCATTERDQZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup138], (instregex "VPSCATTERQQZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup138], (instregex "VSCATTERDPDZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup138], (instregex "VSCATTERQPDZ128mr(b?)(k?)(z?)")>;

def SKLWriteResGroup139 : SchedWriteRes<[SKLPort0,SKLPort4,SKLPort237,SKLPort0156]> {
  let Latency = 6;
  let NumMicroOps = 11;
  let ResourceCycles = [1,4,4,2];
}
def: InstRW<[SKLWriteResGroup139], (instregex "VPSCATTERDQZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup139], (instregex "VPSCATTERQQZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup139], (instregex "VSCATTERDPDZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup139], (instregex "VSCATTERQPDZ256mr(b?)(k?)(z?)")>;

def SKLWriteResGroup140 : SchedWriteRes<[SKLPort0,SKLPort4,SKLPort237,SKLPort0156]> {
  let Latency = 6;
  let NumMicroOps = 19;
  let ResourceCycles = [1,8,8,2];
}

def SKLWriteResGroup141 : SchedWriteRes<[SKLPort0,SKLPort4,SKLPort5,SKLPort237,SKLPort0156]> {
  let Latency = 6;
  let NumMicroOps = 36;
  let ResourceCycles = [1,16,1,16,2];
}

def SKLWriteResGroup142 : SchedWriteRes<[SKLPort0,SKLPort5]> {
  let Latency = 7;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup142], (instregex "VCVTDQ2PDYrr")>;

def SKLWriteResGroup143 : SchedWriteRes<[SKLPort5,SKLPort015]> {
  let Latency = 7;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTDQ2PDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTPD2DQYrr")>;
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTPD2DQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTPD2PSYrr")>;
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTPD2PSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTPD2UDQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTPH2PSYrr")>;
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTPH2PSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTPS2PDYrr")>;
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTPS2PDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTPS2PHYrr")>;
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTPS2PHZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTPS2QQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTPS2UQQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTQQ2PSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTTPD2DQYrr")>;
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTTPD2DQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTTPD2UDQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTTPS2QQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTTPS2UQQZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTUDQ2PDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup143], (instregex "VCVTUQQ2PSZ256rr(b?)(k?)(z?)")>;

def SKLWriteResGroup144 : SchedWriteRes<[SKLPort5,SKLPort015]> {
  let Latency = 7;
  let NumMicroOps = 3;
  let ResourceCycles = [2,1];
}
def: InstRW<[SKLWriteResGroup144], (instregex "VPERMI2W128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup144], (instregex "VPERMI2W256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup144], (instregex "VPERMI2Wrr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup144], (instregex "VPERMT2W128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup144], (instregex "VPERMT2W256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup144], (instregex "VPERMT2Wrr(b?)(k?)(z?)")>;

def SKLWriteResGroup145 : SchedWriteRes<[SKLPort0,SKLPort5,SKLPort23]> {
  let Latency = 7;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup145], (instregex "MUL_FI16m")>;
def: InstRW<[SKLWriteResGroup145], (instregex "MUL_FI32m")>;
def: InstRW<[SKLWriteResGroup145], (instregex "VCVTDQ2PDYrm")>;

def SKLWriteResGroup146 : SchedWriteRes<[SKLPort0,SKLPort5,SKLPort015]> {
  let Latency = 7;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup146], (instregex "CVTTSS2SI64rr")>;
def: InstRW<[SKLWriteResGroup146], (instregex "CVTTSS2SIrr")>;
def: InstRW<[SKLWriteResGroup146], (instregex "VCVTTSS2SI64rr")>;
def: InstRW<[SKLWriteResGroup146], (instregex "VCVTTSS2SIrr")>;

def SKLWriteResGroup147 : SchedWriteRes<[SKLPort5,SKLPort23,SKLPort015]> {
  let Latency = 7;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}

def SKLWriteResGroup148 : SchedWriteRes<[SKLPort5,SKLPort23,SKLPort015]> {
  let Latency = 7;
  let NumMicroOps = 4;
  let ResourceCycles = [2,1,1];
}
def: InstRW<[SKLWriteResGroup148], (instregex "VPERMI2W128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup148], (instregex "VPERMI2W256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup148], (instregex "VPERMI2Wrm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup148], (instregex "VPERMT2W128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup148], (instregex "VPERMT2W256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup148], (instregex "VPERMT2Wrm(b?)(k?)(z?)")>;

def SKLWriteResGroup149 : SchedWriteRes<[SKLPort0,SKLPort5,SKLPort23,SKLPort015]> {
  let Latency = 7;
  let NumMicroOps = 4;
  let ResourceCycles = [1,1,1,1];
}
def: InstRW<[SKLWriteResGroup149], (instregex "CVTTSS2SI64rm")>;

def SKLWriteResGroup150 : SchedWriteRes<[SKLPort4,SKLPort5,SKLPort237,SKLPort015]> {
  let Latency = 7;
  let NumMicroOps = 4;
  let ResourceCycles = [1,1,1,1];
}
def: InstRW<[SKLWriteResGroup150], (instregex "VCVTPS2PHYmr")>;

def SKLWriteResGroup151 : SchedWriteRes<[SKLPort6,SKLPort06,SKLPort15,SKLPort0156]> {
  let Latency = 7;
  let NumMicroOps = 7;
  let ResourceCycles = [1,3,1,2];
}
def: InstRW<[SKLWriteResGroup151], (instregex "LOOP")>;

def SKLWriteResGroup152 : SchedWriteRes<[SKLPort0,SKLPort4,SKLPort5,SKLPort237,SKLPort0156]> {
  let Latency = 7;
  let NumMicroOps = 8;
  let ResourceCycles = [1,2,1,2,2];
}
def: InstRW<[SKLWriteResGroup152], (instregex "VPSCATTERQDZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup152], (instregex "VPSCATTERQDZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup152], (instregex "VSCATTERQPSZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup152], (instregex "VSCATTERQPSZ256mr(b?)(k?)(z?)")>;

def SKLWriteResGroup153 : SchedWriteRes<[SKLPort0,SKLPort4,SKLPort5,SKLPort237,SKLPort0156]> {
  let Latency = 7;
  let NumMicroOps = 12;
  let ResourceCycles = [1,4,1,4,2];
}
def: InstRW<[SKLWriteResGroup153], (instregex "VPSCATTERDDZ128mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup153], (instregex "VSCATTERDPSZ128mr(b?)(k?)(z?)")>;

def SKLWriteResGroup154 : SchedWriteRes<[SKLPort0,SKLPort4,SKLPort5,SKLPort237,SKLPort0156]> {
  let Latency = 7;
  let NumMicroOps = 20;
  let ResourceCycles = [1,8,1,8,2];
}
def: InstRW<[SKLWriteResGroup154], (instregex "VPSCATTERDDZ256mr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup154], (instregex "VSCATTERDPSZ256mr(b?)(k?)(z?)")>;

def SKLWriteResGroup155 : SchedWriteRes<[SKLPort0,SKLPort4,SKLPort5,SKLPort237,SKLPort0156]> {
  let Latency = 7;
  let NumMicroOps = 36;
  let ResourceCycles = [1,16,1,16,2];
}

def SKLWriteResGroup156 : SchedWriteRes<[SKLPort0]> {
  let Latency = 8;
  let NumMicroOps = 2;
  let ResourceCycles = [2];
}
def: InstRW<[SKLWriteResGroup156], (instregex "AESIMCrr")>;
def: InstRW<[SKLWriteResGroup156], (instregex "VAESIMCrr")>;

def SKLWriteResGroup157 : SchedWriteRes<[SKLPort015]> {
  let Latency = 8;
  let NumMicroOps = 2;
  let ResourceCycles = [2];
}
def: InstRW<[SKLWriteResGroup157], (instregex "PMULLDrr")>;
def: InstRW<[SKLWriteResGroup157], (instregex "ROUNDPDr")>;
def: InstRW<[SKLWriteResGroup157], (instregex "ROUNDPSr")>;
def: InstRW<[SKLWriteResGroup157], (instregex "ROUNDSDr")>;
def: InstRW<[SKLWriteResGroup157], (instregex "ROUNDSSr")>;
def: InstRW<[SKLWriteResGroup157], (instregex "VPMULLDYrr")>;
def: InstRW<[SKLWriteResGroup157], (instregex "VPMULLDrr")>;
def: InstRW<[SKLWriteResGroup157], (instregex "VRNDSCALEPDZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup157], (instregex "VRNDSCALEPDZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup157], (instregex "VRNDSCALEPSZ128rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup157], (instregex "VRNDSCALEPSZ256rri(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup157], (instregex "VRNDSCALESDr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup157], (instregex "VRNDSCALESSr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup157], (instregex "VROUNDPDr")>;
def: InstRW<[SKLWriteResGroup157], (instregex "VROUNDPSr")>;
def: InstRW<[SKLWriteResGroup157], (instregex "VROUNDSDr")>;
def: InstRW<[SKLWriteResGroup157], (instregex "VROUNDSSr")>;
def: InstRW<[SKLWriteResGroup157], (instregex "VROUNDYPDr")>;
def: InstRW<[SKLWriteResGroup157], (instregex "VROUNDYPSr")>;

def SKLWriteResGroup158 : SchedWriteRes<[SKLPort01,SKLPort015]> {
  let Latency = 8;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup158], (instregex "VPMULLDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup158], (instregex "VPMULLDZ256rr(b?)(k?)(z?)")>;

def SKLWriteResGroup159 : SchedWriteRes<[SKLPort05,SKLPort015]> {
  let Latency = 8;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}

def SKLWriteResGroup160 : SchedWriteRes<[SKLPort0,SKLPort23]> {
  let Latency = 8;
  let NumMicroOps = 3;
  let ResourceCycles = [2,1];
}
def: InstRW<[SKLWriteResGroup160], (instregex "AESIMCrm")>;
def: InstRW<[SKLWriteResGroup160], (instregex "VAESIMCrm")>;

def SKLWriteResGroup161 : SchedWriteRes<[SKLPort23,SKLPort015]> {
  let Latency = 8;
  let NumMicroOps = 3;
  let ResourceCycles = [1,2];
}
def: InstRW<[SKLWriteResGroup161], (instregex "PMULLDrm")>;
def: InstRW<[SKLWriteResGroup161], (instregex "ROUNDPDm")>;
def: InstRW<[SKLWriteResGroup161], (instregex "ROUNDPSm")>;
def: InstRW<[SKLWriteResGroup161], (instregex "ROUNDSDm")>;
def: InstRW<[SKLWriteResGroup161], (instregex "ROUNDSSm")>;
def: InstRW<[SKLWriteResGroup161], (instregex "VPMULLDYrm")>;
def: InstRW<[SKLWriteResGroup161], (instregex "VPMULLDrm")>;
def: InstRW<[SKLWriteResGroup161], (instregex "VRNDSCALEPDZ128rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup161], (instregex "VRNDSCALEPDZ256rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup161], (instregex "VRNDSCALEPSZ128rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup161], (instregex "VRNDSCALEPSZ256rm(b?)i(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup161], (instregex "VRNDSCALESDm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup161], (instregex "VRNDSCALESSm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup161], (instregex "VROUNDPDm")>;
def: InstRW<[SKLWriteResGroup161], (instregex "VROUNDPSm")>;
def: InstRW<[SKLWriteResGroup161], (instregex "VROUNDSDm")>;
def: InstRW<[SKLWriteResGroup161], (instregex "VROUNDSSm")>;
def: InstRW<[SKLWriteResGroup161], (instregex "VROUNDYPDm")>;
def: InstRW<[SKLWriteResGroup161], (instregex "VROUNDYPSm")>;

def SKLWriteResGroup162 : SchedWriteRes<[SKLPort01,SKLPort23,SKLPort015]> {
  let Latency = 8;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup162], (instregex "VPMULLDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup162], (instregex "VPMULLDZ256rm(b?)(k?)(z?)")>;

def SKLWriteResGroup163 : SchedWriteRes<[SKLPort23,SKLPort05,SKLPort015]> {
  let Latency = 8;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}

def SKLWriteResGroup164 : SchedWriteRes<[SKLPort0,SKLPort015]> {
  let Latency = 9;
  let NumMicroOps = 3;
  let ResourceCycles = [2,1];
}

def SKLWriteResGroup165 : SchedWriteRes<[SKLPort5,SKLPort015]> {
  let Latency = 9;
  let NumMicroOps = 3;
  let ResourceCycles = [1,2];
}
def: InstRW<[SKLWriteResGroup165], (instregex "DPPDrri")>;
def: InstRW<[SKLWriteResGroup165], (instregex "VDPPDrri")>;

def SKLWriteResGroup166 : SchedWriteRes<[SKLPort0,SKLPort23,SKLPort015]> {
  let Latency = 9;
  let NumMicroOps = 4;
  let ResourceCycles = [2,1,1];
}

def SKLWriteResGroup167 : SchedWriteRes<[SKLPort5,SKLPort23,SKLPort015]> {
  let Latency = 9;
  let NumMicroOps = 4;
  let ResourceCycles = [1,1,2];
}
def: InstRW<[SKLWriteResGroup167], (instregex "DPPDrmi")>;
def: InstRW<[SKLWriteResGroup167], (instregex "VDPPDrmi")>;

def SKLWriteResGroup168 : SchedWriteRes<[SKLPort1,SKLPort5,SKLPort01,SKLPort23,SKLPort015]> {
  let Latency = 9;
  let NumMicroOps = 8;
  let ResourceCycles = [1,2,2,1,2];
}
def: InstRW<[SKLWriteResGroup168], (instregex "VPCONFLICTDZ128rm(b?)(k?)(z?)")>;

def SKLWriteResGroup169 : SchedWriteRes<[SKLPort0]> {
  let Latency = 10;
  let NumMicroOps = 3;
  let ResourceCycles = [3];
}
def: InstRW<[SKLWriteResGroup169], (instregex "PCMPISTRIrr")>;
def: InstRW<[SKLWriteResGroup169], (instregex "PCMPISTRM128rr")>;
def: InstRW<[SKLWriteResGroup169], (instregex "VPCMPISTRIrr")>;
def: InstRW<[SKLWriteResGroup169], (instregex "VPCMPISTRM128rr")>;

def SKLWriteResGroup170 : SchedWriteRes<[SKLPort0,SKLPort23]> {
  let Latency = 10;
  let NumMicroOps = 4;
  let ResourceCycles = [3,1];
}
def: InstRW<[SKLWriteResGroup170], (instregex "PCMPISTRIrm")>;
def: InstRW<[SKLWriteResGroup170], (instregex "PCMPISTRM128rm")>;
def: InstRW<[SKLWriteResGroup170], (instregex "VPCMPISTRIrm")>;
def: InstRW<[SKLWriteResGroup170], (instregex "VPCMPISTRM128rm")>;

def SKLWriteResGroup171 : SchedWriteRes<[SKLPort05,SKLPort0156]> {
  let Latency = 10;
  let NumMicroOps = 10;
  let ResourceCycles = [9,1];
}
def: InstRW<[SKLWriteResGroup171], (instregex "MMX_EMMS")>;

def SKLWriteResGroup172 : SchedWriteRes<[SKLPort1,SKLPort23,SKLPort237,SKLPort06,SKLPort15,SKLPort0156]> {
  let Latency = 10;
  let NumMicroOps = 10;
  let ResourceCycles = [1,1,1,5,1,1];
}
def: InstRW<[SKLWriteResGroup172], (instregex "RCL(16|32|64)mCL")>;
def: InstRW<[SKLWriteResGroup172], (instregex "RCL8mCL")>;

def SKLWriteResGroup173 : SchedWriteRes<[SKLPort0]> {
  let Latency = 11;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup173], (instregex "DIVPSrr")>;
def: InstRW<[SKLWriteResGroup173], (instregex "DIVSSrr")>;
def: InstRW<[SKLWriteResGroup173], (instregex "VDIVPSYrr")>;
def: InstRW<[SKLWriteResGroup173], (instregex "VDIVPSZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup173], (instregex "VDIVPSZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup173], (instregex "VDIVPSrr")>;
def: InstRW<[SKLWriteResGroup173], (instregex "VDIVSSrr")>;

def SKLWriteResGroup174 : SchedWriteRes<[SKLPort0,SKLPort23]> {
  let Latency = 11;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup174], (instregex "DIVPSrm")>;
def: InstRW<[SKLWriteResGroup174], (instregex "DIVSSrm")>;
def: InstRW<[SKLWriteResGroup174], (instregex "VDIVPSYrm")>;
def: InstRW<[SKLWriteResGroup174], (instregex "VDIVPSZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup174], (instregex "VDIVPSZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup174], (instregex "VDIVPSrm")>;
def: InstRW<[SKLWriteResGroup174], (instregex "VDIVSSrm")>;

def SKLWriteResGroup175 : SchedWriteRes<[SKLPort1,SKLPort06,SKLPort0156]> {
  let Latency = 11;
  let NumMicroOps = 7;
  let ResourceCycles = [2,3,2];
}
def: InstRW<[SKLWriteResGroup175], (instregex "RCL(16|32|64)rCL")>;
def: InstRW<[SKLWriteResGroup175], (instregex "RCR(16|32|64)rCL")>;

def SKLWriteResGroup176 : SchedWriteRes<[SKLPort1,SKLPort06,SKLPort15,SKLPort0156]> {
  let Latency = 11;
  let NumMicroOps = 9;
  let ResourceCycles = [1,5,1,2];
}
def: InstRW<[SKLWriteResGroup176], (instregex "RCL8rCL")>;

def SKLWriteResGroup177 : SchedWriteRes<[SKLPort06,SKLPort0156]> {
  let Latency = 11;
  let NumMicroOps = 11;
  let ResourceCycles = [2,9];
}
def: InstRW<[SKLWriteResGroup177], (instregex "LOOPE")>;
def: InstRW<[SKLWriteResGroup177], (instregex "LOOPNE")>;

def SKLWriteResGroup178 : SchedWriteRes<[SKLPort4,SKLPort23,SKLPort237,SKLPort06,SKLPort15,SKLPort0156]> {
  let Latency = 11;
  let NumMicroOps = 14;
  let ResourceCycles = [1,1,1,4,2,5];
}
def: InstRW<[SKLWriteResGroup178], (instregex "CMPXCHG8B")>;

def SKLWriteResGroup179 : SchedWriteRes<[SKLPort0]> {
  let Latency = 12;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup179], (instregex "SQRTPSr")>;
def: InstRW<[SKLWriteResGroup179], (instregex "SQRTSSr")>;
def: InstRW<[SKLWriteResGroup179], (instregex "VSQRTPSYr")>;
def: InstRW<[SKLWriteResGroup179], (instregex "VSQRTPSZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup179], (instregex "VSQRTPSZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup179], (instregex "VSQRTPSr")>;
def: InstRW<[SKLWriteResGroup179], (instregex "VSQRTSSr")>;

def SKLWriteResGroup180 : SchedWriteRes<[SKLPort0,SKLPort23]> {
  let Latency = 12;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup180], (instregex "SQRTPSm")>;
def: InstRW<[SKLWriteResGroup180], (instregex "SQRTSSm")>;
def: InstRW<[SKLWriteResGroup180], (instregex "VSQRTPSYm")>;
def: InstRW<[SKLWriteResGroup180], (instregex "VSQRTPSZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup180], (instregex "VSQRTPSZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup180], (instregex "VSQRTPSm")>;
def: InstRW<[SKLWriteResGroup180], (instregex "VSQRTSSm")>;

def SKLWriteResGroup181 : SchedWriteRes<[SKLPort01]> {
  let Latency = 12;
  let NumMicroOps = 3;
  let ResourceCycles = [3];
}
def: InstRW<[SKLWriteResGroup181], (instregex "VPMULLQZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup181], (instregex "VPMULLQZ256rr(b?)(k?)(z?)")>;

def SKLWriteResGroup182 : SchedWriteRes<[SKLPort05]> {
  let Latency = 12;
  let NumMicroOps = 3;
  let ResourceCycles = [3];
}

def SKLWriteResGroup183 : SchedWriteRes<[SKLPort01,SKLPort23]> {
  let Latency = 12;
  let NumMicroOps = 4;
  let ResourceCycles = [3,1];
}
def: InstRW<[SKLWriteResGroup183], (instregex "VPMULLQZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup183], (instregex "VPMULLQZ256rm(b?)(k?)(z?)")>;

def SKLWriteResGroup184 : SchedWriteRes<[SKLPort23,SKLPort05]> {
  let Latency = 12;
  let NumMicroOps = 4;
  let ResourceCycles = [1,3];
}

def SKLWriteResGroup185 : SchedWriteRes<[SKLPort0]> {
  let Latency = 13;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}

def SKLWriteResGroup186 : SchedWriteRes<[SKLPort0,SKLPort23]> {
  let Latency = 13;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}

def SKLWriteResGroup187 : SchedWriteRes<[SKLPort5,SKLPort015]> {
  let Latency = 13;
  let NumMicroOps = 4;
  let ResourceCycles = [1,3];
}
def: InstRW<[SKLWriteResGroup187], (instregex "DPPSrri")>;
def: InstRW<[SKLWriteResGroup187], (instregex "VDPPSYrri")>;
def: InstRW<[SKLWriteResGroup187], (instregex "VDPPSrri")>;

def SKLWriteResGroup188 : SchedWriteRes<[SKLPort5,SKLPort23,SKLPort015]> {
  let Latency = 13;
  let NumMicroOps = 5;
  let ResourceCycles = [1,1,3];
}
def: InstRW<[SKLWriteResGroup188], (instregex "DPPSrmi")>;
def: InstRW<[SKLWriteResGroup188], (instregex "VDPPSYrmi")>;
def: InstRW<[SKLWriteResGroup188], (instregex "VDPPSrmi")>;

def SKLWriteResGroup189 : SchedWriteRes<[SKLPort1,SKLPort23,SKLPort237,SKLPort06,SKLPort15,SKLPort0156]> {
  let Latency = 13;
  let NumMicroOps = 11;
  let ResourceCycles = [2,1,1,4,1,2];
}
def: InstRW<[SKLWriteResGroup189], (instregex "RCR(16|32|64)mCL")>;
def: InstRW<[SKLWriteResGroup189], (instregex "RCR8mCL")>;

def SKLWriteResGroup190 : SchedWriteRes<[SKLPort0]> {
  let Latency = 14;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup190], (instregex "DIVPDrr")>;
def: InstRW<[SKLWriteResGroup190], (instregex "DIVSDrr")>;
def: InstRW<[SKLWriteResGroup190], (instregex "VDIVPDYrr")>;
def: InstRW<[SKLWriteResGroup190], (instregex "VDIVPDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup190], (instregex "VDIVPDZ256rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup190], (instregex "VDIVPDrr")>;
def: InstRW<[SKLWriteResGroup190], (instregex "VDIVSDrr")>;

def SKLWriteResGroup191 : SchedWriteRes<[SKLPort0,SKLPort23]> {
  let Latency = 14;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup191], (instregex "DIVPDrm")>;
def: InstRW<[SKLWriteResGroup191], (instregex "DIVSDrm")>;
def: InstRW<[SKLWriteResGroup191], (instregex "VDIVPDYrm")>;
def: InstRW<[SKLWriteResGroup191], (instregex "VDIVPDZ128rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup191], (instregex "VDIVPDZ256rm(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup191], (instregex "VDIVPDrm")>;
def: InstRW<[SKLWriteResGroup191], (instregex "VDIVSDrm")>;

def SKLWriteResGroup192 : SchedWriteRes<[SKLPort1,SKLPort06,SKLPort15,SKLPort0156]> {
  let Latency = 14;
  let NumMicroOps = 10;
  let ResourceCycles = [2,4,1,3];
}
def: InstRW<[SKLWriteResGroup192], (instregex "RCR8rCL")>;

def SKLWriteResGroup193 : SchedWriteRes<[SKLPort0]> {
  let Latency = 15;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup193], (instregex "DIVR_FPrST0")>;
def: InstRW<[SKLWriteResGroup193], (instregex "DIVR_FST0r")>;
def: InstRW<[SKLWriteResGroup193], (instregex "DIVR_FrST0")>;

def SKLWriteResGroup194 : SchedWriteRes<[SKLPort0,SKLPort23]> {
  let Latency = 15;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup194], (instregex "DIV_F32m")>;
def: InstRW<[SKLWriteResGroup194], (instregex "DIV_F64m")>;

def SKLWriteResGroup195 : SchedWriteRes<[SKLPort4,SKLPort5,SKLPort6,SKLPort23,SKLPort237,SKLPort06,SKLPort0156]> {
  let Latency = 15;
  let NumMicroOps = 8;
  let ResourceCycles = [1,1,1,1,1,1,2];
}
def: InstRW<[SKLWriteResGroup195], (instregex "INSB")>;
def: InstRW<[SKLWriteResGroup195], (instregex "INSL")>;
def: InstRW<[SKLWriteResGroup195], (instregex "INSW")>;

def SKLWriteResGroup196 : SchedWriteRes<[SKLPort0156]> {
  let Latency = 16;
  let NumMicroOps = 16;
  let ResourceCycles = [16];
}
def: InstRW<[SKLWriteResGroup196], (instregex "VZEROALL")>;

def SKLWriteResGroup197 : SchedWriteRes<[SKLPort0,SKLPort1,SKLPort5,SKLPort6,SKLPort05,SKLPort0156]> {
  let Latency = 17;
  let NumMicroOps = 15;
  let ResourceCycles = [2,1,2,4,2,4];
}
def: InstRW<[SKLWriteResGroup197], (instregex "XCH_F")>;

def SKLWriteResGroup198 : SchedWriteRes<[SKLPort0]> {
  let Latency = 18;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup198], (instregex "SQRTPDr")>;
def: InstRW<[SKLWriteResGroup198], (instregex "SQRTSDr")>;
def: InstRW<[SKLWriteResGroup198], (instregex "VSQRTPDYr")>;
def: InstRW<[SKLWriteResGroup198], (instregex "VSQRTPDZ128r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup198], (instregex "VSQRTPDZ256r(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup198], (instregex "VSQRTPDr")>;
def: InstRW<[SKLWriteResGroup198], (instregex "VSQRTSDr")>;

def SKLWriteResGroup199 : SchedWriteRes<[SKLPort0,SKLPort23]> {
  let Latency = 18;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup199], (instregex "SQRTPDm")>;
def: InstRW<[SKLWriteResGroup199], (instregex "SQRTSDm")>;
def: InstRW<[SKLWriteResGroup199], (instregex "VSQRTPDYm")>;
def: InstRW<[SKLWriteResGroup199], (instregex "VSQRTPDZ128m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup199], (instregex "VSQRTPDZ256m(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup199], (instregex "VSQRTPDm")>;
def: InstRW<[SKLWriteResGroup199], (instregex "VSQRTSDm")>;

def SKLWriteResGroup200 : SchedWriteRes<[SKLPort0,SKLPort5,SKLPort23]> {
  let Latency = 18;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup200], (instregex "DIV_FI16m")>;
def: InstRW<[SKLWriteResGroup200], (instregex "DIV_FI32m")>;

def SKLWriteResGroup201 : SchedWriteRes<[SKLPort0,SKLPort5,SKLPort0156]> {
  let Latency = 18;
  let NumMicroOps = 8;
  let ResourceCycles = [4,3,1];
}
def: InstRW<[SKLWriteResGroup201], (instregex "PCMPESTRIrr")>;
def: InstRW<[SKLWriteResGroup201], (instregex "VPCMPESTRIrr")>;

def SKLWriteResGroup202 : SchedWriteRes<[SKLPort5,SKLPort6,SKLPort06,SKLPort0156]> {
  let Latency = 18;
  let NumMicroOps = 8;
  let ResourceCycles = [1,1,1,5];
}
def: InstRW<[SKLWriteResGroup202], (instregex "CPUID")>;
def: InstRW<[SKLWriteResGroup202], (instregex "RDTSC")>;

def SKLWriteResGroup203 : SchedWriteRes<[SKLPort0,SKLPort5,SKLPort23,SKLPort0156]> {
  let Latency = 18;
  let NumMicroOps = 9;
  let ResourceCycles = [4,3,1,1];
}
def: InstRW<[SKLWriteResGroup203], (instregex "PCMPESTRIrm")>;
def: InstRW<[SKLWriteResGroup203], (instregex "VPCMPESTRIrm")>;

def SKLWriteResGroup204 : SchedWriteRes<[SKLPort0,SKLPort4,SKLPort5,SKLPort23,SKLPort237,SKLPort06,SKLPort0156]> {
  let Latency = 18;
  let NumMicroOps = 19;
  let ResourceCycles = [2,1,4,1,1,4,6];
}
def: InstRW<[SKLWriteResGroup204], (instregex "CMPXCHG16B")>;

def SKLWriteResGroup205 : SchedWriteRes<[SKLPort0,SKLPort5,SKLPort015,SKLPort0156]> {
  let Latency = 19;
  let NumMicroOps = 9;
  let ResourceCycles = [4,3,1,1];
}
def: InstRW<[SKLWriteResGroup205], (instregex "PCMPESTRM128rr")>;
def: InstRW<[SKLWriteResGroup205], (instregex "VPCMPESTRM128rr")>;

def SKLWriteResGroup206 : SchedWriteRes<[SKLPort0,SKLPort5,SKLPort23,SKLPort015,SKLPort0156]> {
  let Latency = 19;
  let NumMicroOps = 10;
  let ResourceCycles = [4,3,1,1,1];
}
def: InstRW<[SKLWriteResGroup206], (instregex "PCMPESTRM128rm")>;
def: InstRW<[SKLWriteResGroup206], (instregex "VPCMPESTRM128rm")>;

def SKLWriteResGroup207 : SchedWriteRes<[SKLPort0,SKLPort5,SKLPort23,SKLPort015]> {
  let Latency = 19;
  let NumMicroOps = 11;
  let ResourceCycles = [3,6,1,1];
}
def: InstRW<[SKLWriteResGroup207], (instregex "AESKEYGENASSIST128rm")>;
def: InstRW<[SKLWriteResGroup207], (instregex "VAESKEYGENASSIST128rm")>;

def SKLWriteResGroup208 : SchedWriteRes<[SKLPort0]> {
  let Latency = 20;
  let NumMicroOps = 1;
  let ResourceCycles = [1];
}
def: InstRW<[SKLWriteResGroup208], (instregex "DIV_FPrST0")>;
def: InstRW<[SKLWriteResGroup208], (instregex "DIV_FST0r")>;
def: InstRW<[SKLWriteResGroup208], (instregex "DIV_FrST0")>;

def SKLWriteResGroup209 : SchedWriteRes<[SKLPort0,SKLPort23]> {
  let Latency = 20;
  let NumMicroOps = 2;
  let ResourceCycles = [1,1];
}
def: InstRW<[SKLWriteResGroup209], (instregex "DIVR_F32m")>;
def: InstRW<[SKLWriteResGroup209], (instregex "DIVR_F64m")>;

def SKLWriteResGroup210 : SchedWriteRes<[SKLPort5,SKLPort6,SKLPort0156]> {
  let Latency = 20;
  let NumMicroOps = 10;
  let ResourceCycles = [1,2,7];
}
def: InstRW<[SKLWriteResGroup210], (instregex "MWAITrr")>;

def SKLWriteResGroup211 : SchedWriteRes<[SKLPort0,SKLPort5,SKLPort015]> {
  let Latency = 20;
  let NumMicroOps = 11;
  let ResourceCycles = [3,6,2];
}
def: InstRW<[SKLWriteResGroup211], (instregex "AESKEYGENASSIST128rr")>;
def: InstRW<[SKLWriteResGroup211], (instregex "VAESKEYGENASSIST128rr")>;

def SKLWriteResGroup212 : SchedWriteRes<[SKLPort5,SKLPort01,SKLPort015]> {
  let Latency = 22;
  let NumMicroOps = 14;
  let ResourceCycles = [5,5,4];
}
def: InstRW<[SKLWriteResGroup212], (instregex "VPCONFLICTDZ128rr(b?)(k?)(z?)")>;
def: InstRW<[SKLWriteResGroup212], (instregex "VPCONFLICTQZ256rr(b?)(k?)(z?)")>;

def SKLWriteResGroup213 : SchedWriteRes<[SKLPort5,SKLPort01,SKLPort23,SKLPort015]> {
  let Latency = 22;
  let NumMicroOps = 15;
  let ResourceCycles = [5,5,1,4];
}
def: InstRW<[SKLWriteResGroup213], (instregex "VPCONFLICTQZ256rm(b?)(k?)(z?)")>;

def SKLWriteResGroup214 : SchedWriteRes<[SKLPort0,SKLPort015]> {
  let Latency = 23;
  let NumMicroOps = 3;
  let ResourceCycles = [2,1];
}

def SKLWriteResGroup215 : SchedWriteRes<[SKLPort0,SKLPort5,SKLPort23]> {
  let Latency = 23;
  let NumMicroOps = 3;
  let ResourceCycles = [1,1,1];
}
def: InstRW<[SKLWriteResGroup215], (instregex "DIVR_FI16m")>;
def: InstRW<[SKLWriteResGroup215], (instregex "DIVR_FI32m")>;

def SKLWriteResGroup216 : SchedWriteRes<[SKLPort0,SKLPort23,SKLPort015]> {
  let Latency = 23;
  let NumMicroOps = 4;
  let ResourceCycles = [2,1,1];
}

def SKLWriteResGroup217 : SchedWriteRes<[SKLPort0,SKLPort5,SKLPort23,SKLPort0156]> {
  let Latency = 23;
  let NumMicroOps = 8;
  let ResourceCycles = [2,4,1,1];
}
def: InstRW<[SKLWriteResGroup217], (instregex "IDIV(16|32|64)m")>;
def: InstRW<[SKLWriteResGroup217], (instregex "IDIV8m")>;

def SKLWriteResGroup218 : SchedWriteRes<[SKLPort0,SKLPort015]> {
  let Latency = 25;
  let NumMicroOps = 3;
  let ResourceCycles = [2,1];
}

def SKLWriteResGroup219 : SchedWriteRes<[SKLPort0,SKLPort23,SKLPort015]> {
  let Latency = 25;
  let NumMicroOps = 4;
  let ResourceCycles = [2,1,1];
}

def SKLWriteResGroup220 : SchedWriteRes<[SKLPort0,SKLPort015]> {
  let Latency = 29;
  let NumMicroOps = 3;
  let ResourceCycles = [2,1];
}

def SKLWriteResGroup221 : SchedWriteRes<[SKLPort0,SKLPort23,SKLPort015]> {
  let Latency = 29;
  let NumMicroOps = 4;
  let ResourceCycles = [2,1,1];
}

def SKLWriteResGroup222 : SchedWriteRes<[SKLPort5,SKLPort6,SKLPort23,SKLPort06,SKLPort0156]> {
  let Latency = 30;
  let NumMicroOps = 23;
  let ResourceCycles = [1,5,3,4,10];
}
def: InstRW<[SKLWriteResGroup222], (instregex "IN32ri")>;
def: InstRW<[SKLWriteResGroup222], (instregex "IN32rr")>;
def: InstRW<[SKLWriteResGroup222], (instregex "IN8ri")>;
def: InstRW<[SKLWriteResGroup222], (instregex "IN8rr")>;

def SKLWriteResGroup223 : SchedWriteRes<[SKLPort5,SKLPort6,SKLPort23,SKLPort237,SKLPort06,SKLPort0156]> {
  let Latency = 30;
  let NumMicroOps = 23;
  let ResourceCycles = [1,5,2,1,4,10];
}
def: InstRW<[SKLWriteResGroup223], (instregex "OUT32ir")>;
def: InstRW<[SKLWriteResGroup223], (instregex "OUT32rr")>;
def: InstRW<[SKLWriteResGroup223], (instregex "OUT8ir")>;
def: InstRW<[SKLWriteResGroup223], (instregex "OUT8rr")>;

def SKLWriteResGroup224 : SchedWriteRes<[SKLPort1,SKLPort6,SKLPort23,SKLPort0156]> {
  let Latency = 32;
  let NumMicroOps = 31;
  let ResourceCycles = [1,8,1,21];
}
def: InstRW<[SKLWriteResGroup224], (instregex "XRSTOR")>;
// def: InstRW<[SKLWriteResGroup224], (instregex "XRSTOR64")>;

def SKLWriteResGroup225 : SchedWriteRes<[SKLPort1,SKLPort4,SKLPort5,SKLPort6,SKLPort23,SKLPort237,SKLPort15,SKLPort0156]> {
  let Latency = 35;
  let NumMicroOps = 18;
  let ResourceCycles = [1,1,2,3,1,1,1,8];
}
def: InstRW<[SKLWriteResGroup225], (instregex "VMCLEARm")>;

def SKLWriteResGroup226 : SchedWriteRes<[SKLPort4,SKLPort6,SKLPort23,SKLPort237,SKLPort0156]> {
  let Latency = 36;
  let NumMicroOps = 39;
  let ResourceCycles = [1,10,1,1,26];
}
def: InstRW<[SKLWriteResGroup226], (instregex "XSAVE64")>;

def SKLWriteResGroup227 : SchedWriteRes<[SKLPort0,SKLPort015]> {
  let Latency = 37;
  let NumMicroOps = 3;
  let ResourceCycles = [2,1];
}

def SKLWriteResGroup228 : SchedWriteRes<[SKLPort0,SKLPort23,SKLPort015]> {
  let Latency = 37;
  let NumMicroOps = 4;
  let ResourceCycles = [2,1,1];
}

def SKLWriteResGroup229 : SchedWriteRes<[SKLPort5,SKLPort01,SKLPort015]> {
  let Latency = 37;
  let NumMicroOps = 21;
  let ResourceCycles = [9,7,5];
}
def: InstRW<[SKLWriteResGroup229], (instregex "VPCONFLICTDZ256rr(b?)(k?)(z?)")>;

def SKLWriteResGroup230 : SchedWriteRes<[SKLPort5,SKLPort01,SKLPort23,SKLPort015]> {
  let Latency = 37;
  let NumMicroOps = 22;
  let ResourceCycles = [9,7,1,5];
}
def: InstRW<[SKLWriteResGroup230], (instregex "VPCONFLICTDZ256rm(b?)(k?)(z?)")>;

def SKLWriteResGroup231 : SchedWriteRes<[SKLPort4,SKLPort6,SKLPort23,SKLPort237,SKLPort0156]> {
  let Latency = 37;
  let NumMicroOps = 40;
  let ResourceCycles = [1,11,1,1,26];
}
def: InstRW<[SKLWriteResGroup231], (instregex "XSAVE")>;

def SKLWriteResGroup232 : SchedWriteRes<[SKLPort4,SKLPort6,SKLPort23,SKLPort237,SKLPort0156]> {
  let Latency = 41;
  let NumMicroOps = 44;
  let ResourceCycles = [1,11,1,1,30];
}
// def: InstRW<[SKLWriteResGroup232], (instregex "XSAVEOPT")>;
// def: InstRW<[SKLWriteResGroup232], (instregex "XSAVEOPT64")>;

def SKLWriteResGroup233 : SchedWriteRes<[SKLPort5,SKLPort0156]> {
  let Latency = 42;
  let NumMicroOps = 22;
  let ResourceCycles = [2,20];
}
def: InstRW<[SKLWriteResGroup233], (instregex "RDTSCP")>;

def SKLWriteResGroup234 : SchedWriteRes<[SKLPort0,SKLPort23,SKLPort05,SKLPort06,SKLPort0156]> {
  let Latency = 57;
  let NumMicroOps = 64;
  let ResourceCycles = [2,8,5,10,39];
}
def: InstRW<[SKLWriteResGroup234], (instregex "FLDENVm")>;
def: InstRW<[SKLWriteResGroup234], (instregex "FLDENVm")>;

def SKLWriteResGroup235 : SchedWriteRes<[SKLPort0,SKLPort6,SKLPort23,SKLPort05,SKLPort06,SKLPort15,SKLPort0156]> {
  let Latency = 58;
  let NumMicroOps = 88;
  let ResourceCycles = [4,4,31,1,2,1,45];
}
def: InstRW<[SKLWriteResGroup235], (instregex "FXRSTOR64")>;

def SKLWriteResGroup236 : SchedWriteRes<[SKLPort0,SKLPort6,SKLPort23,SKLPort05,SKLPort06,SKLPort15,SKLPort0156]> {
  let Latency = 58;
  let NumMicroOps = 90;
  let ResourceCycles = [4,2,33,1,2,1,47];
}
def: InstRW<[SKLWriteResGroup236], (instregex "FXRSTOR")>;

def SKLWriteResGroup237 : SchedWriteRes<[SKLPort5,SKLPort01,SKLPort015]> {
  let Latency = 67;
  let NumMicroOps = 35;
  let ResourceCycles = [17,11,7];
}

def SKLWriteResGroup238 : SchedWriteRes<[SKLPort5,SKLPort01,SKLPort23,SKLPort015]> {
  let Latency = 67;
  let NumMicroOps = 36;
  let ResourceCycles = [17,11,1,7];
}

def SKLWriteResGroup239 : SchedWriteRes<[SKLPort5,SKLPort05,SKLPort0156]> {
  let Latency = 75;
  let NumMicroOps = 15;
  let ResourceCycles = [6,3,6];
}
def: InstRW<[SKLWriteResGroup239], (instregex "FNINIT")>;

def SKLWriteResGroup240 : SchedWriteRes<[SKLPort0,SKLPort1,SKLPort5,SKLPort6,SKLPort05,SKLPort0156]> {
  let Latency = 76;
  let NumMicroOps = 32;
  let ResourceCycles = [7,2,8,3,1,11];
}
def: InstRW<[SKLWriteResGroup240], (instregex "DIV(16|32|64)r")>;

def SKLWriteResGroup241 : SchedWriteRes<[SKLPort0,SKLPort1,SKLPort5,SKLPort6,SKLPort06,SKLPort0156]> {
  let Latency = 102;
  let NumMicroOps = 66;
  let ResourceCycles = [4,2,4,8,14,34];
}
def: InstRW<[SKLWriteResGroup241], (instregex "IDIV(16|32|64)r")>;

def SKLWriteResGroup242 : SchedWriteRes<[SKLPort0,SKLPort1,SKLPort4,SKLPort5,SKLPort6,SKLPort237,SKLPort06,SKLPort0156]> {
  let Latency = 105;
  let NumMicroOps = 100;
  let ResourceCycles = [9,1,11,16,1,11,21,30];
}
def: InstRW<[SKLWriteResGroup242], (instregex "FSTENVm")>;
def: InstRW<[SKLWriteResGroup242], (instregex "FSTENVm")>;

} // SchedModel
