//===- IntrinsicsWebAssembly.td - Defines wasm intrinsics --*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief This file defines all of the WebAssembly-specific intrinsics.
///
//===----------------------------------------------------------------------===//

let TargetPrefix = "wasm" in {  // All intrinsics start with "llvm.wasm.".

// Note that current_memory is not IntrNoMem because it must be sequenced with
// respect to grow_memory calls.
def int_wasm_current_memory : Intrinsic<[llvm_anyint_ty], [], [IntrReadMem]>;
def int_wasm_grow_memory : Intrinsic<[llvm_anyint_ty], [LLVMMatchType<0>], []>;

// Exception handling intrinsics

def int_wasm_throw : Intrinsic<[], [llvm_i32_ty, llvm_ptr_ty],
                               [Throws, IntrNoReturn]>;
def int_wasm_rethrow : Intrinsic<[], [], [Throws, IntrNoReturn]>;
// wasm.catch returns the pointer to the exception object caught by wasm 'catch'
// instruction.
def int_wasm_catch : Intrinsic<[llvm_ptr_ty], [llvm_i32_ty], []>;
// WebAssembly EH must maintain the landingpads in the order assigned to them
// by WasmEHPrepare pass to generate landingpad table in EHStreamer. This is
// used in order to give them the indices in WasmEHPrepare.
def int_wasm_eh_landingpad_index: Intrinsic<[], [llvm_i32_ty], [IntrNoMem]>;
// Returns LSDA address of the current function.
def int_wasm_eh_lsda : Intrinsic<[llvm_ptr_ty], [], [IntrNoMem]>;
}
