; RUN: opt < %s -wasmehprepare -S | FileCheck %s

target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-unknown-wasm"

; CHECK: @__wasm_lpad_context = external global { i32, i8*, i8*, i32 }

@_ZTIi = external constant i8*

; There are two landing pads: one landing pad has two invokes as its
; predecessors, and the other has one.
define void @test1() personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*) {
; CHECK-LABEL: @test1()
entry:
  invoke void @foo()
          to label %invoke.cont unwind label %lpad
; CHECK: entry:
; CHECK-NEXT: call void @llvm.wasm.eh.landingpad.index(i32 0)

invoke.cont:                                      ; preds = %entry
  invoke void @foo()
          to label %try.cont unwind label %lpad
; CHECK: invoke.cont:
; CHECK-NEXT: call void @llvm.wasm.eh.landingpad.index(i32 0)

lpad:                                             ; preds = %invoke.cont, %entry
  %0 = landingpad { i8*, i32 }
          catch i8* null
  %1 = extractvalue { i8*, i32 } %0, 0
  %2 = extractvalue { i8*, i32 } %0, 1
  %3 = call i8* @__cxa_begin_catch(i8* %1)
  call void @__cxa_end_catch()
  br label %try.cont
; CHECK: lpad:
; CHECK: store volatile i32 0, i32* getelementptr inbounds ({ i32, i8*, i8*, i32 }, { i32, i8*, i8*, i32 }* @__wasm_lpad_context, i32 0, i32 0)

try.cont:                                         ; preds = %invoke.cont, %lpad
  invoke void @foo()
          to label %try.cont6 unwind label %lpad2
; CHECK: try.cont:
; CHECK-NEXT: call void @llvm.wasm.eh.landingpad.index(i32 1)

lpad2:                                            ; preds = %try.cont
  %4 = landingpad { i8*, i32 }
          catch i8* bitcast (i8** @_ZTIi to i8*)
  %5 = extractvalue { i8*, i32 } %4, 0
  %6 = extractvalue { i8*, i32 } %4, 1
  %7 = call i32 @llvm.eh.typeid.for(i8* bitcast (i8** @_ZTIi to i8*))
  %matches = icmp eq i32 %6, %7
  br i1 %matches, label %catch4, label %eh.resume
; CHECK: lpad2:
; CHECK-NEXT: landingpad
; CHECK-NEXT: catch
; CHECK-NEXT: %[[EXN:.*]] = call i8* @llvm.wasm.catch(i32 0)
; CHECK-NEXT: store volatile i32 1, i32* getelementptr inbounds ({ i32, i8*, i8*, i32 }, { i32, i8*, i8*, i32 }* @__wasm_lpad_context, i32 0, i32 0)
; CHECK-NEXT: store volatile i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*), i8** getelementptr inbounds ({ i32, i8*, i8*, i32 }, { i32, i8*, i8*, i32 }* @__wasm_lpad_context, i32 0, i32 1)
; CHECK-NEXT: %[[LSDA:.*]] = call i8* @llvm.wasm.eh.lsda()
; CHECK-NEXT: store volatile i8* %[[LSDA]], i8** getelementptr inbounds ({ i32, i8*, i8*, i32 }, { i32, i8*, i8*, i32 }* @__wasm_lpad_context, i32 0, i32 2)
; CHECK-NEXT: call i32 @_Unwind_Wasm_CallPersonality(i8* %[[EXN]])
; CHECK-NEXT: %[[SELECTOR:.*]] = load i32, i32* getelementptr inbounds ({ i32, i8*, i8*, i32 }, { i32, i8*, i8*, i32 }* @__wasm_lpad_context, i32 0, i32 3)
; CHECK: icmp eq i32 %[[SELECTOR]]

catch4:                                           ; preds = %lpad2
  %8 = call i8* @__cxa_begin_catch(i8* %5)
  %9 = bitcast i8* %8 to i32*
  %10 = load i32, i32* %9, align 4
  call void @__cxa_end_catch()
  br label %try.cont6
; CHECK: catch4:
; CHECK-NEXT: call i8* @__cxa_begin_catch(i8* %[[EXN]])

try.cont6:                                        ; preds = %try.cont, %catch4
  ret void

eh.resume:                                        ; preds = %lpad2
  %lpad.val = insertvalue { i8*, i32 } undef, i8* %5, 0
  %lpad.val9 = insertvalue { i8*, i32 } %lpad.val, i32 %6, 1
  resume { i8*, i32 } %lpad.val9
; CHECK: eh.resume:
; CHECK-NEXT: %[[VAL:.*]] = insertvalue { i8*, i32 } undef, i8* %[[EXN]], 0
; CHECK-NEXT: %[[VAL2:.*]] = insertvalue { i8*, i32 } %[[VAL]], i32 %[[SELECTOR]], 1
; CHECK-NEXT: resume { i8*, i32 } %[[VAL2]]
}

; Optimization test: non-top-level landing pads should not store personality
; function address and LSDA address again unnecessarily.
define void @test2() personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*) {
; CHECK-LABEL: @test2()
entry:
  invoke void @foo()
          to label %try.cont unwind label %lpad

lpad:                                             ; preds = %entry
  %0 = landingpad { i8*, i32 }
          catch i8* null
  %1 = extractvalue { i8*, i32 } %0, 0
  %2 = extractvalue { i8*, i32 } %0, 1
  %3 = call i8* @__cxa_begin_catch(i8* %1) #3
  invoke void @foo()
          to label %invoke.cont2 unwind label %lpad1
; CHECK: lpad:
; CHECK-NEXT: landingpad
; CHECK-NEXT: catch
; CHECK-NEXT: @llvm.wasm.catch
; CHECK-NEXT: store volatile i32 0
; CHECK-NEXT: @__gxx_personality_v0
; CHECK-NEXT: @llvm.wasm.eh.lsda
; CHECK-NEXT: store volatile i8*
; CHECK-NEXT: call i32 @_Unwind_Wasm_CallPersonality

invoke.cont2:                                     ; preds = %lpad
  call void @__cxa_end_catch()
  br label %try.cont

try.cont:                                         ; preds = %entry, %invoke.cont2
  ret void

lpad1:                                            ; preds = %lpad
  %4 = landingpad { i8*, i32 }
          cleanup
  %5 = extractvalue { i8*, i32 } %4, 0
  %6 = extractvalue { i8*, i32 } %4, 1
  invoke void @__cxa_end_catch()
          to label %eh.resume unwind label %terminate.lpad
; CHECK: lpad1:
; CHECK-NEXT: landingpad
; CHECK-NEXT: cleanup
; CHECK-NEXT: @llvm.wasm.catch
; CHECK-NEXT: store volatile i32 1
; CHECK-NEXT: call i32 @_Unwind_Wasm_CallPersonality

eh.resume:                                        ; preds = %lpad1
  %lpad.val = insertvalue { i8*, i32 } undef, i8* %5, 0
  %lpad.val5 = insertvalue { i8*, i32 } %lpad.val, i32 %6, 1
  resume { i8*, i32 } %lpad.val5

terminate.lpad:                                   ; preds = %lpad1
  %7 = landingpad { i8*, i32 }
          catch i8* null
  %8 = extractvalue { i8*, i32 } %7, 0
  call void @__clang_call_terminate(i8* %8)
  unreachable
; CHECK: terminate.lpad:
; CHECK-NEXT: landingpad
; CHECK-NEXT: catch
; CHECK-NEXT: @llvm.wasm.catch
; CHECK-NEXT: store volatile i32 2
; CHECK-NEXT: call i32 @_Unwind_Wasm_CallPersonality
}

declare void @foo()
declare i32 @__gxx_personality_v0(...)
declare i8* @__cxa_begin_catch(i8*)
declare void @__cxa_end_catch()
declare i32 @llvm.eh.typeid.for(i8*)
declare void @__clang_call_terminate(i8*)

; CHECK-DAG: declare i8* @llvm.wasm.catch(i32)
; CHECK-DAG: declare void @llvm.wasm.eh.landingpad.index(i32)
; CHECK-DAG: declare i8* @llvm.wasm.eh.lsda()
; CHECK-DAG: declare i32 @_Unwind_Wasm_CallPersonality(i8*)
