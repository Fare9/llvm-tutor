; ModuleID = '../inputs/input_for_cff_2.c'
source_filename = "../inputs/input_for_cff_2.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [32 x i8] c"Checking how big the number is\0A\00", align 1
@.str.1 = private unnamed_addr constant [12 x i8] c"number < 5\0A\00", align 1
@.str.2 = private unnamed_addr constant [13 x i8] c"number >= 5\0A\00", align 1
@.str.3 = private unnamed_addr constant [15 x i8] c"Some divisors\0A\00", align 1
@.str.4 = private unnamed_addr constant [13 x i8] c"number %% 3\0A\00", align 1
@.str.5 = private unnamed_addr constant [13 x i8] c"number %% 5\0A\00", align 1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @obfuscate_me(i32 noundef %number) #0 {
entry:
  %number.addr = alloca i32, align 4
  %counter = alloca i32, align 4
  store i32 %number, ptr %number.addr, align 4
  %call = call i32 (ptr, ...) @printf(ptr noundef @.str)
  store i32 1, ptr %counter, align 4
  %0 = load i32, ptr %number.addr, align 4
  %cmp = icmp slt i32 %0, 5
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %call1 = call i32 (ptr, ...) @printf(ptr noundef @.str.1)
  %1 = load i32, ptr %counter, align 4
  %inc = add nsw i32 %1, 1
  store i32 %inc, ptr %counter, align 4
  br label %if.end

if.else:                                          ; preds = %entry
  %call2 = call i32 (ptr, ...) @printf(ptr noundef @.str.2)
  %2 = load i32, ptr %counter, align 4
  %add = add nsw i32 %2, 2
  store i32 %add, ptr %counter, align 4
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %call3 = call i32 (ptr, ...) @printf(ptr noundef @.str.3)
  %3 = load i32, ptr %number.addr, align 4
  %rem = srem i32 %3, 3
  %tobool = icmp ne i32 %rem, 0
  br i1 %tobool, label %if.then4, label %if.end7

if.then4:                                         ; preds = %if.end
  %call5 = call i32 (ptr, ...) @printf(ptr noundef @.str.4)
  %4 = load i32, ptr %counter, align 4
  %inc6 = add nsw i32 %4, 1
  store i32 %inc6, ptr %counter, align 4
  br label %if.end7

if.end7:                                          ; preds = %if.then4, %if.end
  %5 = load i32, ptr %number.addr, align 4
  %rem8 = srem i32 %5, 5
  %tobool9 = icmp ne i32 %rem8, 0
  br i1 %tobool9, label %if.then10, label %if.end13

if.then10:                                        ; preds = %if.end7
  %call11 = call i32 (ptr, ...) @printf(ptr noundef @.str.5)
  %6 = load i32, ptr %counter, align 4
  %inc12 = add nsw i32 %6, 1
  store i32 %inc12, ptr %counter, align 4
  br label %if.end13

if.end13:                                         ; preds = %if.then10, %if.end7
  ret void
}

declare i32 @printf(ptr noundef, ...) #1

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 17.0.0 (https://github.com/llvm/llvm-project.git a93ca35a44948ce2376c5940c40e7e01a502696f)"}
