; RUN: opt %s -S -simplifycfg | FileCheck %s
declare void @foo(i32)

define void @test(i1 %a) {
; CHECK-LABEL @test
; CHECK: br i1 [[IGNORE:%.*]], label %true, label %false
  switch i1 %a, label %default [i1 1, label %true
                                i1 0, label %false]
true:
  call void @foo(i32 1)
  ret void
false:
  call void @foo(i32 3)
  ret void
default:
  call void @foo(i32 2)
  ret void
}  

define void @test2(i2 %a) {
; CHECK-LABEL @test2
  switch i2 %a, label %default [i2 0, label %case0
                                i2 1, label %case1
                                i2 2, label %case2
                                i2 3, label %case3]
case0:
  call void @foo(i32 0)
  ret void
case1:
  call void @foo(i32 1)
  ret void
case2:
  call void @foo(i32 2)
  ret void
case3:
  call void @foo(i32 3)
  ret void
default:
; CHECK-LABEL: default1:
; CHECK-NEXT: unreachable
  call void @foo(i32 4)
  ret void
}  

; This one is a negative test - we know the value of the default,
; but that's about it
define void @test3(i2 %a) {
; CHECK-LABEL @test3
  switch i2 %a, label %default [i2 0, label %case0
                                i2 1, label %case1
                                i2 2, label %case2]

case0:
  call void @foo(i32 0)
  ret void
case1:
  call void @foo(i32 1)
  ret void
case2:
  call void @foo(i32 2)
  ret void
default:
; CHECK-LABEL: default:
; CHECK-NEXT: call void @foo
  call void @foo(i32 0)
  ret void
}  

; Negative test - check for possible overflow when computing
; number of possible cases.
define void @test4(i128 %a) {
; CHECK-LABEL @test4
  switch i128 %a, label %default [i128 0, label %case0
                                  i128 1, label %case1]

case0:
  call void @foo(i32 0)
  ret void
case1:
  call void @foo(i32 1)
  ret void
default:
; CHECK-LABEL: default:
; CHECK-NEXT: call void @foo
  call void @foo(i32 0)
  ret void
}  
