; RUN: llc -march=r600 -mtriple=r600---amdgiz < %s | FileCheck %s

; CHECK: DOT4 * T{{[0-9]\.W}} (MASKED)
; CHECK: MAX T{{[0-9].[XYZW]}}, 0.0, PV.X
define amdgpu_vs void @main(<4 x float> inreg %reg0, <4 x float> inreg %reg1, <4 x float> inreg %reg2, <4 x float> inreg %reg3, <4 x float> inreg %reg4, <4 x float> inreg %reg5, <4 x float> inreg %reg6, <4 x float> inreg %reg7) {
main_body:
  %tmp = extractelement <4 x float> %reg1, i32 0
  %tmp13 = extractelement <4 x float> %reg1, i32 1
  %tmp14 = extractelement <4 x float> %reg1, i32 2
  %tmp15 = extractelement <4 x float> %reg1, i32 3
  %tmp16 = extractelement <4 x float> %reg2, i32 0
  %tmp17 = extractelement <4 x float> %reg2, i32 1
  %tmp18 = extractelement <4 x float> %reg2, i32 2
  %tmp19 = extractelement <4 x float> %reg2, i32 3
  %tmp20 = extractelement <4 x float> %reg3, i32 0
  %tmp21 = extractelement <4 x float> %reg3, i32 1
  %tmp22 = extractelement <4 x float> %reg3, i32 2
  %tmp23 = extractelement <4 x float> %reg3, i32 3
  %tmp24 = extractelement <4 x float> %reg4, i32 0
  %tmp25 = extractelement <4 x float> %reg4, i32 1
  %tmp26 = extractelement <4 x float> %reg4, i32 2
  %tmp27 = extractelement <4 x float> %reg4, i32 3
  %tmp28 = extractelement <4 x float> %reg5, i32 0
  %tmp29 = extractelement <4 x float> %reg5, i32 1
  %tmp30 = extractelement <4 x float> %reg5, i32 2
  %tmp31 = extractelement <4 x float> %reg5, i32 3
  %tmp32 = extractelement <4 x float> %reg6, i32 0
  %tmp33 = extractelement <4 x float> %reg6, i32 1
  %tmp34 = extractelement <4 x float> %reg6, i32 2
  %tmp35 = extractelement <4 x float> %reg6, i32 3
  %tmp36 = extractelement <4 x float> %reg7, i32 0
  %tmp37 = extractelement <4 x float> %reg7, i32 1
  %tmp38 = extractelement <4 x float> %reg7, i32 2
  %tmp39 = extractelement <4 x float> %reg7, i32 3
  %tmp40 = load <4 x float>, <4 x float> addrspace(8)* null
  %tmp41 = extractelement <4 x float> %tmp40, i32 0
  %tmp42 = fmul float %tmp, %tmp41
  %tmp43 = load <4 x float>, <4 x float> addrspace(8)* null
  %tmp44 = extractelement <4 x float> %tmp43, i32 1
  %tmp45 = fmul float %tmp, %tmp44
  %tmp46 = load <4 x float>, <4 x float> addrspace(8)* null
  %tmp47 = extractelement <4 x float> %tmp46, i32 2
  %tmp48 = fmul float %tmp, %tmp47
  %tmp49 = load <4 x float>, <4 x float> addrspace(8)* null
  %tmp50 = extractelement <4 x float> %tmp49, i32 3
  %tmp51 = fmul float %tmp, %tmp50
  %tmp52 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 1)
  %tmp53 = extractelement <4 x float> %tmp52, i32 0
  %tmp54 = fmul float %tmp13, %tmp53
  %tmp55 = fadd float %tmp54, %tmp42
  %tmp56 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 1)
  %tmp57 = extractelement <4 x float> %tmp56, i32 1
  %tmp58 = fmul float %tmp13, %tmp57
  %tmp59 = fadd float %tmp58, %tmp45
  %tmp60 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 1)
  %tmp61 = extractelement <4 x float> %tmp60, i32 2
  %tmp62 = fmul float %tmp13, %tmp61
  %tmp63 = fadd float %tmp62, %tmp48
  %tmp64 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 1)
  %tmp65 = extractelement <4 x float> %tmp64, i32 3
  %tmp66 = fmul float %tmp13, %tmp65
  %tmp67 = fadd float %tmp66, %tmp51
  %tmp68 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 2)
  %tmp69 = extractelement <4 x float> %tmp68, i32 0
  %tmp70 = fmul float %tmp14, %tmp69
  %tmp71 = fadd float %tmp70, %tmp55
  %tmp72 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 2)
  %tmp73 = extractelement <4 x float> %tmp72, i32 1
  %tmp74 = fmul float %tmp14, %tmp73
  %tmp75 = fadd float %tmp74, %tmp59
  %tmp76 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 2)
  %tmp77 = extractelement <4 x float> %tmp76, i32 2
  %tmp78 = fmul float %tmp14, %tmp77
  %tmp79 = fadd float %tmp78, %tmp63
  %tmp80 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 2)
  %tmp81 = extractelement <4 x float> %tmp80, i32 3
  %tmp82 = fmul float %tmp14, %tmp81
  %tmp83 = fadd float %tmp82, %tmp67
  %tmp84 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 3)
  %tmp85 = extractelement <4 x float> %tmp84, i32 0
  %tmp86 = fmul float %tmp15, %tmp85
  %tmp87 = fadd float %tmp86, %tmp71
  %tmp88 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 3)
  %tmp89 = extractelement <4 x float> %tmp88, i32 1
  %tmp90 = fmul float %tmp15, %tmp89
  %tmp91 = fadd float %tmp90, %tmp75
  %tmp92 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 3)
  %tmp93 = extractelement <4 x float> %tmp92, i32 2
  %tmp94 = fmul float %tmp15, %tmp93
  %tmp95 = fadd float %tmp94, %tmp79
  %tmp96 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 3)
  %tmp97 = extractelement <4 x float> %tmp96, i32 3
  %tmp98 = fmul float %tmp15, %tmp97
  %tmp99 = fadd float %tmp98, %tmp83
  %tmp100 = insertelement <4 x float> undef, float %tmp16, i32 0
  %tmp101 = insertelement <4 x float> %tmp100, float %tmp17, i32 1
  %tmp102 = insertelement <4 x float> %tmp101, float %tmp18, i32 2
  %tmp103 = insertelement <4 x float> %tmp102, float 0.000000e+00, i32 3
  %tmp104 = insertelement <4 x float> undef, float %tmp16, i32 0
  %tmp105 = insertelement <4 x float> %tmp104, float %tmp17, i32 1
  %tmp106 = insertelement <4 x float> %tmp105, float %tmp18, i32 2
  %tmp107 = insertelement <4 x float> %tmp106, float 0.000000e+00, i32 3
  %tmp108 = call float @llvm.r600.dot4(<4 x float> %tmp103, <4 x float> %tmp107)
  %tmp109 = call float @llvm.fabs.f32(float %tmp108)
  %tmp110 = call float @llvm.r600.recipsqrt.clamped.f32(float %tmp109)
  %tmp111 = fmul float %tmp16, %tmp110
  %tmp112 = fmul float %tmp17, %tmp110
  %tmp113 = fmul float %tmp18, %tmp110
  %tmp114 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 4)
  %tmp115 = extractelement <4 x float> %tmp114, i32 0
  %tmp116 = fmul float %tmp115, %tmp20
  %tmp117 = fadd float %tmp116, %tmp32
  %tmp118 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 4)
  %tmp119 = extractelement <4 x float> %tmp118, i32 1
  %tmp120 = fmul float %tmp119, %tmp21
  %tmp121 = fadd float %tmp120, %tmp33
  %tmp122 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 4)
  %tmp123 = extractelement <4 x float> %tmp122, i32 2
  %tmp124 = fmul float %tmp123, %tmp22
  %tmp125 = fadd float %tmp124, %tmp34
  %max.0.i = call float @llvm.maxnum.f32(float %tmp117, float 0.000000e+00)
  %clamp.i = call float @llvm.minnum.f32(float %max.0.i, float 1.000000e+00)
  %max.0.i11 = call float @llvm.maxnum.f32(float %tmp121, float 0.000000e+00)
  %clamp.i12 = call float @llvm.minnum.f32(float %max.0.i11, float 1.000000e+00)
  %max.0.i9 = call float @llvm.maxnum.f32(float %tmp125, float 0.000000e+00)
  %clamp.i10 = call float @llvm.minnum.f32(float %max.0.i9, float 1.000000e+00)
  %max.0.i7 = call float @llvm.maxnum.f32(float %tmp27, float 0.000000e+00)
  %clamp.i8 = call float @llvm.minnum.f32(float %max.0.i7, float 1.000000e+00)
  %tmp126 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 5)
  %tmp127 = extractelement <4 x float> %tmp126, i32 0
  %tmp128 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 5)
  %tmp129 = extractelement <4 x float> %tmp128, i32 1
  %tmp130 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 5)
  %tmp131 = extractelement <4 x float> %tmp130, i32 2
  %tmp132 = insertelement <4 x float> undef, float %tmp111, i32 0
  %tmp133 = insertelement <4 x float> %tmp132, float %tmp112, i32 1
  %tmp134 = insertelement <4 x float> %tmp133, float %tmp113, i32 2
  %tmp135 = insertelement <4 x float> %tmp134, float 0.000000e+00, i32 3
  %tmp136 = insertelement <4 x float> undef, float %tmp127, i32 0
  %tmp137 = insertelement <4 x float> %tmp136, float %tmp129, i32 1
  %tmp138 = insertelement <4 x float> %tmp137, float %tmp131, i32 2
  %tmp139 = insertelement <4 x float> %tmp138, float 0.000000e+00, i32 3
  %tmp140 = call float @llvm.r600.dot4(<4 x float> %tmp135, <4 x float> %tmp139)
  %tmp141 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 7)
  %tmp142 = extractelement <4 x float> %tmp141, i32 0
  %tmp143 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 7)
  %tmp144 = extractelement <4 x float> %tmp143, i32 1
  %tmp145 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 7)
  %tmp146 = extractelement <4 x float> %tmp145, i32 2
  %tmp147 = insertelement <4 x float> undef, float %tmp111, i32 0
  %tmp148 = insertelement <4 x float> %tmp147, float %tmp112, i32 1
  %tmp149 = insertelement <4 x float> %tmp148, float %tmp113, i32 2
  %tmp150 = insertelement <4 x float> %tmp149, float 0.000000e+00, i32 3
  %tmp151 = insertelement <4 x float> undef, float %tmp142, i32 0
  %tmp152 = insertelement <4 x float> %tmp151, float %tmp144, i32 1
  %tmp153 = insertelement <4 x float> %tmp152, float %tmp146, i32 2
  %tmp154 = insertelement <4 x float> %tmp153, float 0.000000e+00, i32 3
  %tmp155 = call float @llvm.r600.dot4(<4 x float> %tmp150, <4 x float> %tmp154)
  %tmp156 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 8)
  %tmp157 = extractelement <4 x float> %tmp156, i32 0
  %tmp158 = fmul float %tmp157, %tmp20
  %tmp159 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 8)
  %tmp160 = extractelement <4 x float> %tmp159, i32 1
  %tmp161 = fmul float %tmp160, %tmp21
  %tmp162 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 8)
  %tmp163 = extractelement <4 x float> %tmp162, i32 2
  %tmp164 = fmul float %tmp163, %tmp22
  %tmp165 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 9)
  %tmp166 = extractelement <4 x float> %tmp165, i32 0
  %tmp167 = fmul float %tmp166, %tmp24
  %tmp168 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 9)
  %tmp169 = extractelement <4 x float> %tmp168, i32 1
  %tmp170 = fmul float %tmp169, %tmp25
  %tmp171 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 9)
  %tmp172 = extractelement <4 x float> %tmp171, i32 2
  %tmp173 = fmul float %tmp172, %tmp26
  %tmp174 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 10)
  %tmp175 = extractelement <4 x float> %tmp174, i32 0
  %tmp176 = fmul float %tmp175, %tmp28
  %tmp177 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 10)
  %tmp178 = extractelement <4 x float> %tmp177, i32 1
  %tmp179 = fmul float %tmp178, %tmp29
  %tmp180 = load <4 x float>, <4 x float> addrspace(8)* getelementptr ([1024 x <4 x float>], [1024 x <4 x float>] addrspace(8)* null, i64 0, i32 10)
  %tmp181 = extractelement <4 x float> %tmp180, i32 2
  %tmp182 = fmul float %tmp181, %tmp30
  %tmp183 = fcmp uge float %tmp140, 0.000000e+00
  %tmp184 = select i1 %tmp183, float %tmp140, float 0.000000e+00
  %tmp185 = fcmp uge float %tmp155, 0.000000e+00
  %tmp186 = select i1 %tmp185, float %tmp155, float 0.000000e+00
  %tmp187 = call float @llvm.pow.f32(float %tmp186, float %tmp36)
  %tmp188 = fcmp ult float %tmp140, 0.000000e+00
  %tmp189 = select i1 %tmp188, float 0.000000e+00, float %tmp187
  %tmp190 = fadd float %tmp158, %tmp117
  %tmp191 = fadd float %tmp161, %tmp121
  %tmp192 = fadd float %tmp164, %tmp125
  %tmp193 = fmul float %tmp184, %tmp167
  %tmp194 = fadd float %tmp193, %tmp190
  %tmp195 = fmul float %tmp184, %tmp170
  %tmp196 = fadd float %tmp195, %tmp191
  %tmp197 = fmul float %tmp184, %tmp173
  %tmp198 = fadd float %tmp197, %tmp192
  %tmp199 = fmul float %tmp189, %tmp176
  %tmp200 = fadd float %tmp199, %tmp194
  %tmp201 = fmul float %tmp189, %tmp179
  %tmp202 = fadd float %tmp201, %tmp196
  %tmp203 = fmul float %tmp189, %tmp182
  %tmp204 = fadd float %tmp203, %tmp198
  %max.0.i5 = call float @llvm.maxnum.f32(float %tmp200, float 0.000000e+00)
  %clamp.i6 = call float @llvm.minnum.f32(float %max.0.i5, float 1.000000e+00)
  %max.0.i3 = call float @llvm.maxnum.f32(float %tmp202, float 0.000000e+00)
  %clamp.i4 = call float @llvm.minnum.f32(float %max.0.i3, float 1.000000e+00)
  %max.0.i1 = call float @llvm.maxnum.f32(float %tmp204, float 0.000000e+00)
  %clamp.i2 = call float @llvm.minnum.f32(float %max.0.i1, float 1.000000e+00)
  %tmp205 = insertelement <4 x float> undef, float %tmp87, i32 0
  %tmp206 = insertelement <4 x float> %tmp205, float %tmp91, i32 1
  %tmp207 = insertelement <4 x float> %tmp206, float %tmp95, i32 2
  %tmp208 = insertelement <4 x float> %tmp207, float %tmp99, i32 3
  call void @llvm.r600.store.swizzle(<4 x float> %tmp208, i32 60, i32 1)
  %tmp209 = insertelement <4 x float> undef, float %clamp.i6, i32 0
  %tmp210 = insertelement <4 x float> %tmp209, float %clamp.i4, i32 1
  %tmp211 = insertelement <4 x float> %tmp210, float %clamp.i2, i32 2
  %tmp212 = insertelement <4 x float> %tmp211, float %clamp.i8, i32 3
  call void @llvm.r600.store.swizzle(<4 x float> %tmp212, i32 0, i32 2)
  ret void
}

declare float @llvm.minnum.f32(float, float) #0
declare float @llvm.maxnum.f32(float, float) #0
declare float @llvm.r600.dot4(<4 x float>, <4 x float>) #0
declare float @llvm.fabs.f32(float) #0
declare float @llvm.r600.recipsqrt.clamped.f32(float) #0
declare float @llvm.pow.f32(float, float) #0
declare void @llvm.r600.store.swizzle(<4 x float>, i32, i32) #1

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind }
