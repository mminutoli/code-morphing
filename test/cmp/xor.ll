; RUN: opt -load %projshlibdir/CMPasses.so \
; RUN:     -static-morphing                \
; RUN:     -S -o - %s | FileCheck %s
; REQUIRES: loadable_module


target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define i32 @prova(i32 %a, i32 %b) #0 {
entry:
  %a.addr = alloca i32, align 4
  %b.addr = alloca i32, align 4
  store i32 %a, i32* %a.addr, align 4
  store i32 %b, i32* %b.addr, align 4
  %0 = load i32* %a.addr, align 4
  %1 = load i32* %b.addr, align 4
  %xor = xor i32 %0, %1
  ret i32 %xor
}
;CHECK: AlternativeNumber

;CHECK: prova
;CHECK:      loopCondition:
;CHECK-NEXT:   %0 = load i32* %idx.vector
;CHECK-NEXT:   %.dec = add i32 %0, -1
;CHECK-NEXT:   store i32 %.dec, i32* %idx.vector
;CHECK-NEXT:   %1 = icmp ne i32 %0, 0
;CHECK-NEXT:   br i1 %1, label %loopBody, label %oldEntry

;CHECK:      loopBody:
;CHECK-NEXT:   %2 = load i32* %idx.vector
;CHECK-NEXT:   %3 = getelementptr inbounds [1 x i32]* @AlternativeNumber, i32 0, i32 %2
;CHECK-NEXT:   %4 = load i32* %3
;CHECK-NEXT:   %5 = call i32 @randomize(i32 %4)
;CHECK-NEXT:   %6 = getelementptr inbounds [1 x i32]* %choice.vector, i32 0, i32 %2
;CHECK-NEXT:   store i32 %5, i32* %6
;CHECK-NEXT:   br label %loopCondition

;CHECK:      oldEntry:
;CHECK-NEXT:   %a.addr = alloca i32, align 4
;CHECK-NEXT:   %b.addr = alloca i32, align 4
;CHECK-NEXT:   store i32 %a, i32* %a.addr, align 4
;CHECK-NEXT:   store i32 %b, i32* %b.addr, align 4
;CHECK-NEXT:   %7 = load i32* %a.addr, align 4
;CHECK-NEXT:   %8 = load i32* %b.addr, align 4
;CHECK-NEXT:   %9 = getelementptr [1 x i32]* %choice.vector, i32 0, i32 0
;CHECK-NEXT:   %10 = load i32* %9
;CHECK-NEXT:   switch i32 %10, label %11 [
;CHECK-NEXT:     i32 0, label %11
;CHECK-NEXT:     i32 1, label %14
;CHECK-NEXT:     i32 2, label %19
;CHECK-NEXT:   ]

;CHECK:      ; <label>:11
;CHECK-NEXT:   %xor = xor i32 %7, %8
;CHECK-NEXT:   br label %12

;CHECK:      ; <label>:12
;CHECK-NEXT:   %13 = phi i32 [ %xor, %11 ], [ %18, %14 ]
;CHECK-NEXT:   ret i32 %13

;CHECK:;     <label>:14
;CHECK-NEXT:  %15 = or i32 %7, %8
;CHECK-NEXT:  %16 = and i32 %7, %8
;CHECK-NEXT:  %17 = xor i32 %16, -1
;CHECK-NEXT:  %18 = and i32 %17, %15
;CHECK-NEXT:  br label %12

;CHECK:     ; <label>:19
;CHECK-NEXT:  %20 = xor i32 %8, -1
;CHECK-NEXT:  %21 = and i32 %7, %20
;CHECK-NEXT:  %22 = xor i32 %7, -1
;CHECK-NEXT:  %23 = and i32 %8, %22
;CHECK-NEXT:  %24 = or i32 %21, %23
;CHECK-NEXT:  br label %12

;CHECK: randomize


attributes #0 = { nounwind uwtable "fp-contract-model"="standard" "no-frame-pointer-elim-non-leaf" "realign-stack" "relocation-model"="static" "ssp-buffers-size"="8" }
