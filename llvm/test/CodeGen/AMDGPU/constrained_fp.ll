; RUN:  llc -amdgpu-scalarize-global-loads=false  -march=amdgcn -verify-machineinstrs < %s | FileCheck -check-prefix=SI -check-prefix=FUNC %s

declare double @llvm.experimental.constrained.fma.f64(double, double, double, metadata, metadata) nounwind readnone

; FUNC-LABEL: {{^}}fma_f64:
; FUNC: s_setreg_b32
; FUNC: v_fma_f64
; FUNC: s_setreg_b32
define amdgpu_kernel void @fma_f64(double addrspace(1)* %out, double addrspace(1)* %in1,
                     double addrspace(1)* %in2, double addrspace(1)* %in3) {
   %r0 = load double, double addrspace(1)* %in1
   %r1 = load double, double addrspace(1)* %in2
   %r2 = load double, double addrspace(1)* %in3
   %r3 = tail call double @llvm.experimental.constrained.fma.f64(double %r0, double %r1, double %r2, metadata !"round.dynamic", metadata !"fpexcept.strict")
   store double %r3, double addrspace(1)* %out
   ret void
}


