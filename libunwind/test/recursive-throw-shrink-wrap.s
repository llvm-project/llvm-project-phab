# This comes from SingleSource/Regression/C++/EH/Regression-C++-recursive-throw with advanced shrink-wrapping enabled.
	.section	__TEXT,__text,regular,pure_instructions
	.ios_version_min 5, 0
	.globl	__Z3thri                ; -- Begin function _Z3thri
	.p2align	2
__Z3thri:                               ; @_Z3thri
	.cfi_startproc
; BB#0:                                 ; %entry
	cmp		w0, #1          ; =1
	b.le	LBB0_2
; BB#1:                                 ; %if.end
	stp	x29, x30, [sp, #-16]!   ; 8-byte Folded Spill
Lcfi0:
	.cfi_def_cfa w29, 16
Lcfi1:
	.cfi_offset w29, -16
Lcfi2:
	.cfi_offset w30, -8
	mov	 x29, sp
	sub	w0, w0, #2              ; =2
	bl	__Z3thri
	ldp	x29, x30, [sp], #16     ; 8-byte Folded Reload
Lcfi3:
	.cfi_restore w30
Lcfi4:
	.cfi_restore w29
	ret
LBB0_2:                                 ; %invoke.cont
	adrp	x8, _thrown@PAGE
	orr	w9, wzr, #0x1
	strb	w9, [x8, _thrown@PAGEOFF]
	orr	w0, wzr, #0x1
	bl	___cxa_allocate_exception
Lloh0:
	adrp	x1, __ZTI13TestException@PAGE
Lloh1:
	add	x1, x1, __ZTI13TestException@PAGEOFF
	mov	x2, #0
	bl	___cxa_throw
	.loh AdrpAdd	Lloh0, Lloh1
	.cfi_endproc
                                        ; -- End function
	.private_extern	___clang_call_terminate ; -- Begin function __clang_call_terminate
	.globl	___clang_call_terminate
	.weak_def_can_be_hidden	___clang_call_terminate
	.p2align	2
___clang_call_terminate:                ; @__clang_call_terminate
; BB#0:
	stp	x29, x30, [sp, #-16]!   ; 8-byte Folded Spill
	bl	___cxa_begin_catch
	bl	__ZSt9terminatev
                                        ; -- End function
	.globl	__Z3runv                ; -- Begin function _Z3runv
	.p2align	2
__Z3runv:                               ; @_Z3runv
Lfunc_begin0:
	.cfi_startproc
	.cfi_personality 155, ___gxx_personality_v0
	.cfi_lsda 16, Lexception0
; BB#0:                                 ; %entry
	stp	x29, x30, [sp, #-16]!   ; 8-byte Folded Spill
Lcfi5:
	.cfi_def_cfa w29, 16
Lcfi6:
	.cfi_offset w29, -16
Lcfi7:
	.cfi_offset w30, -8
	mov	 x29, sp
Ltmp0:
	orr	w0, wzr, #0x2
	bl	__Z3thri
Ltmp1:
; BB#1:                                 ; %invoke.cont
Ltmp2:
	bl	_abort
Ltmp3:
; BB#2:                                 ; %invoke.cont1
LBB2_3:                                 ; %lpad
Ltmp4:
	bl	___cxa_begin_catch
	adrp	x8, _caught@PAGE
	orr	w9, wzr, #0x1
	strb	w9, [x8, _caught@PAGEOFF]
	ldp	x29, x30, [sp], #16     ; 8-byte Folded Reload
Lcfi8:
	.cfi_restore w30
Lcfi9:
	.cfi_restore w29
	b	___cxa_end_catch
Lfunc_end0:
	.cfi_endproc
	.section	__TEXT,__gcc_except_tab
	.p2align	2
GCC_except_table2:
Lexception0:
	.byte	255                     ; @LPStart Encoding = omit
	.byte	155                     ; @TType Encoding = indirect pcrel sdata4
	.asciz	"\242\200\200"          ; @TType base offset
	.byte	3                       ; Call site Encoding = udata4
	.byte	26                      ; Call site table length
Lset0 = Ltmp0-Lfunc_begin0              ; >> Call Site 1 <<
	.long	Lset0
Lset1 = Ltmp3-Ltmp0                     ;   Call between Ltmp0 and Ltmp3
	.long	Lset1
Lset2 = Ltmp4-Lfunc_begin0              ;     jumps to Ltmp4
	.long	Lset2
	.byte	1                       ;   On action: 1
Lset3 = Ltmp3-Lfunc_begin0              ; >> Call Site 2 <<
	.long	Lset3
Lset4 = Lfunc_end0-Ltmp3                ;   Call between Ltmp3 and Lfunc_end0
	.long	Lset4
	.long	0                       ;     has no landing pad
	.byte	0                       ;   On action: cleanup
	.byte	1                       ; >> Action Record 1 <<
                                        ;   Catch TypeInfo 1
	.byte	0                       ;   No further actions
                                        ; >> Catch TypeInfos <<
Ltmp5:                                  ; TypeInfo 1
	.long	__ZTI13TestException@GOT-Ltmp5
	.p2align	2
                                        ; -- End function
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_main                   ; -- Begin function main
	.p2align	2
_main:                                  ; @main
Lfunc_begin1:
	.cfi_startproc
	.cfi_personality 155, ___gxx_personality_v0
	.cfi_lsda 16, Lexception1
; BB#0:                                 ; %entry
	stp	x29, x30, [sp, #-16]!   ; 8-byte Folded Spill
Lcfi10:
	.cfi_def_cfa w29, 16
Lcfi11:
	.cfi_offset w29, -16
Lcfi12:
	.cfi_offset w30, -8
	mov	 x29, sp
Ltmp6:
	bl	__Z3runv
Ltmp7:
; BB#1:                                 ; %try.cont
	adrp	x8, _thrown@PAGE
	ldrb	w8, [x8, _thrown@PAGEOFF]
	adrp	x9, _caught@PAGE
	ldrb	w9, [x9, _caught@PAGEOFF]
	cmp		w9, #0          ; =0
	cset	 w9, eq
	eor	w8, w8, #0x1
	and	w8, w8, #0xff
	orr		w0, w9, w8
	ldp	x29, x30, [sp], #16     ; 8-byte Folded Reload
Lcfi13:
	.cfi_restore w30
Lcfi14:
	.cfi_restore w29
	ret
LBB3_2:                                 ; %lpad
Lcfi15:
	.cfi_offset w30, -8
Lcfi16:
	.cfi_offset w29, -16
Ltmp8:
	bl	___cxa_begin_catch
Ltmp9:
	bl	_abort
Ltmp10:
; BB#3:                                 ; %invoke.cont2
LBB3_4:                                 ; %lpad1
Ltmp11:
	mov	 x19, x0
Ltmp12:
	bl	___cxa_end_catch
Ltmp13:
; BB#5:                                 ; %eh.resume
	mov	 x0, x19
	bl	__Unwind_Resume
LBB3_6:                                 ; %terminate.lpad
Ltmp14:
	bl	___clang_call_terminate
Lfunc_end1:
	.cfi_endproc
	.section	__TEXT,__gcc_except_tab
	.p2align	2
GCC_except_table3:
Lexception1:
	.byte	255                     ; @LPStart Encoding = omit
	.byte	155                     ; @TType Encoding = indirect pcrel sdata4
	.byte	73                      ; @TType base offset
	.byte	3                       ; Call site Encoding = udata4
	.byte	65                      ; Call site table length
Lset5 = Ltmp6-Lfunc_begin1              ; >> Call Site 1 <<
	.long	Lset5
Lset6 = Ltmp7-Ltmp6                     ;   Call between Ltmp6 and Ltmp7
	.long	Lset6
Lset7 = Ltmp8-Lfunc_begin1              ;     jumps to Ltmp8
	.long	Lset7
	.byte	1                       ;   On action: 1
Lset8 = Ltmp7-Lfunc_begin1              ; >> Call Site 2 <<
	.long	Lset8
Lset9 = Ltmp9-Ltmp7                     ;   Call between Ltmp7 and Ltmp9
	.long	Lset9
	.long	0                       ;     has no landing pad
	.byte	0                       ;   On action: cleanup
Lset10 = Ltmp9-Lfunc_begin1             ; >> Call Site 3 <<
	.long	Lset10
Lset11 = Ltmp10-Ltmp9                   ;   Call between Ltmp9 and Ltmp10
	.long	Lset11
Lset12 = Ltmp11-Lfunc_begin1            ;     jumps to Ltmp11
	.long	Lset12
	.byte	0                       ;   On action: cleanup
Lset13 = Ltmp12-Lfunc_begin1            ; >> Call Site 4 <<
	.long	Lset13
Lset14 = Ltmp13-Ltmp12                  ;   Call between Ltmp12 and Ltmp13
	.long	Lset14
Lset15 = Ltmp14-Lfunc_begin1            ;     jumps to Ltmp14
	.long	Lset15
	.byte	1                       ;   On action: 1
Lset16 = Ltmp13-Lfunc_begin1            ; >> Call Site 5 <<
	.long	Lset16
Lset17 = Lfunc_end1-Ltmp13              ;   Call between Ltmp13 and Lfunc_end1
	.long	Lset17
	.long	0                       ;     has no landing pad
	.byte	0                       ;   On action: cleanup
	.byte	1                       ; >> Action Record 1 <<
                                        ;   Catch TypeInfo 1
	.byte	0                       ;   No further actions
                                        ; >> Catch TypeInfos <<
	.long	0                       ; TypeInfo 1
	.p2align	2
                                        ; -- End function
	.globl	_thrown                 ; @thrown
.zerofill __DATA,__common,_thrown,1,0
	.globl	_caught                 ; @caught
.zerofill __DATA,__common,_caught,1,0
	.private_extern	__ZTS13TestException ; @_ZTS13TestException
	.section	__TEXT,__const
	.globl	__ZTS13TestException
	.weak_definition	__ZTS13TestException
__ZTS13TestException:
	.asciz	"13TestException"

	.private_extern	__ZTI13TestException ; @_ZTI13TestException
	.section	__DATA,__data
	.globl	__ZTI13TestException
	.weak_definition	__ZTI13TestException
	.p2align	3
__ZTI13TestException:
	.quad	__ZTVN10__cxxabiv117__class_type_infoE+16
	.quad	__ZTS13TestException-9223372036854775808


.subsections_via_symbols
