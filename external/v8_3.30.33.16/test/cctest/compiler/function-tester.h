// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_CCTEST_COMPILER_FUNCTION_TESTER_H_
#define V8_CCTEST_COMPILER_FUNCTION_TESTER_H_

#include "src/v8.h"
#include "test/cctest/cctest.h"

#include "src/ast-numbering.h"
#include "src/compiler.h"
#include "src/compiler/linkage.h"
#include "src/compiler/pipeline.h"
#include "src/execution.h"
#include "src/full-codegen.h"
#include "src/handles.h"
#include "src/objects-inl.h"
#include "src/parser.h"
#include "src/rewriter.h"
#include "src/scopes.h"

#define USE_CRANKSHAFT 0

namespace v8 {
namespace internal {
namespace compiler {

class FunctionTester : public InitializedHandleScope {
 public:
  explicit FunctionTester(const char* source, uint32_t flags = 0)
      : isolate(main_isolate()),
        function((FLAG_allow_natives_syntax = true, NewFunction(source))),
        flags_(flags) {
    Compile(function);
    const uint32_t supported_flags = CompilationInfo::kContextSpecializing |
                                     CompilationInfo::kInliningEnabled |
                                     CompilationInfo::kTypingEnabled;
    CHECK_EQ(0, flags_ & ~supported_flags);
  }

  explicit FunctionTester(Graph* graph)
      : isolate(main_isolate()),
        function(NewFunction("(function(a,b){})")),
        flags_(0) {
    CompileGraph(graph);
  }

  Isolate* isolate;
  Handle<JSFunction> function;

  MaybeHandle<Object> Call(Handle<Object> a, Handle<Object> b) {
    Handle<Object> args[] = {a, b};
    return Execution::Call(isolate, function, undefined(), 2, args, false);
  }

  void CheckThrows(Handle<Object> a, Handle<Object> b) {
    TryCatch try_catch;
    MaybeHandle<Object> no_result = Call(a, b);
    CHECK(isolate->has_pending_exception());
    CHECK(try_catch.HasCaught());
    CHECK(no_result.is_null());
    // TODO(mstarzinger): Temporary workaround for issue chromium:362388.
    isolate->OptionalRescheduleException(true);
  }

  v8::Handle<v8::Message> CheckThrowsReturnMessage(Handle<Object> a,
                                                   Handle<Object> b) {
    TryCatch try_catch;
    MaybeHandle<Object> no_result = Call(a, b);
    CHECK(isolate->has_pending_exception());
    CHECK(try_catch.HasCaught());
    CHECK(no_result.is_null());
    // TODO(mstarzinger): Calling OptionalRescheduleException is a dirty hack,
    // it's the only way to make Message() not to assert because an external
    // exception has been caught by the try_catch.
    isolate->OptionalRescheduleException(true);
    return try_catch.Message();
  }

  void CheckCall(Handle<Object> expected, Handle<Object> a, Handle<Object> b) {
    Handle<Object> result = Call(a, b).ToHandleChecked();
    CHECK(expected->SameValue(*result));
  }

  void CheckCall(Handle<Object> expected, Handle<Object> a) {
    CheckCall(expected, a, undefined());
  }

  void CheckCall(Handle<Object> expected) {
    CheckCall(expected, undefined(), undefined());
  }

  void CheckCall(double expected, double a, double b) {
    CheckCall(Val(expected), Val(a), Val(b));
  }

  void CheckTrue(Handle<Object> a, Handle<Object> b) {
    CheckCall(true_value(), a, b);
  }

  void CheckTrue(Handle<Object> a) { CheckCall(true_value(), a, undefined()); }

  void CheckTrue(double a, double b) {
    CheckCall(true_value(), Val(a), Val(b));
  }

  void CheckFalse(Handle<Object> a, Handle<Object> b) {
    CheckCall(false_value(), a, b);
  }

  void CheckFalse(Handle<Object> a) {
    CheckCall(false_value(), a, undefined());
  }

  void CheckFalse(double a, double b) {
    CheckCall(false_value(), Val(a), Val(b));
  }

  Handle<JSFunction> NewFunction(const char* source) {
    return v8::Utils::OpenHandle(
        *v8::Handle<v8::Function>::Cast(CompileRun(source)));
  }

  Handle<JSObject> NewObject(const char* source) {
    return v8::Utils::OpenHandle(
        *v8::Handle<v8::Object>::Cast(CompileRun(source)));
  }

  Handle<String> Val(const char* string) {
    return isolate->factory()->InternalizeUtf8String(string);
  }

  Handle<Object> Val(double value) {
    return isolate->factory()->NewNumber(value);
  }

  Handle<Object> infinity() { return isolate->factory()->infinity_value(); }

  Handle<Object> minus_infinity() { return Val(-V8_INFINITY); }

  Handle<Object> nan() { return isolate->factory()->nan_value(); }

  Handle<Object> undefined() { return isolate->factory()->undefined_value(); }

  Handle<Object> null() { return isolate->factory()->null_value(); }

  Handle<Object> true_value() { return isolate->factory()->true_value(); }

  Handle<Object> false_value() { return isolate->factory()->false_value(); }

  Handle<JSFunction> Compile(Handle<JSFunction> function) {
// TODO(titzer): make this method private.
#if V8_TURBOFAN_TARGET
    CompilationInfoWithZone info(function);

    CHECK(Parser::Parse(&info));
    info.SetOptimizing(BailoutId::None(), Handle<Code>(function->code()));
    if (flags_ & CompilationInfo::kContextSpecializing) {
      info.MarkAsContextSpecializing();
    }
    if (flags_ & CompilationInfo::kInliningEnabled) {
      info.MarkAsInliningEnabled();
    }
    if (flags_ & CompilationInfo::kTypingEnabled) {
      info.MarkAsTypingEnabled();
    }
    CHECK(Compiler::Analyze(&info));
    CHECK(Compiler::EnsureDeoptimizationSupport(&info));

    Pipeline pipeline(&info);
    Handle<Code> code = pipeline.GenerateCode();
    if (FLAG_turbo_deoptimization) {
      info.context()->native_context()->AddOptimizedCode(*code);
    }

    CHECK(!code.is_null());
    function->ReplaceCode(*code);
#elif USE_CRANKSHAFT
    Handle<Code> unoptimized = Handle<Code>(function->code());
    Handle<Code> code = Compiler::GetOptimizedCode(function, unoptimized,
                                                   Compiler::NOT_CONCURRENT);
    CHECK(!code.is_null());
#if ENABLE_DISASSEMBLER
    if (FLAG_print_opt_code) {
      CodeTracer::Scope tracing_scope(isolate->GetCodeTracer());
      code->Disassemble("test code", tracing_scope.file());
    }
#endif
    function->ReplaceCode(*code);
#endif
    return function;
  }

  static Handle<JSFunction> ForMachineGraph(Graph* graph) {
    JSFunction* p = NULL;
    {  // because of the implicit handle scope of FunctionTester.
      FunctionTester f(graph);
      p = *f.function;
    }
    return Handle<JSFunction>(p);  // allocated in outer handle scope.
  }

 private:
  uint32_t flags_;

  // Compile the given machine graph instead of the source of the function
  // and replace the JSFunction's code with the result.
  Handle<JSFunction> CompileGraph(Graph* graph) {
    CHECK(Pipeline::SupportedTarget());
    CompilationInfoWithZone info(function);

    CHECK(Parser::Parse(&info));
    info.SetOptimizing(BailoutId::None(),
                       Handle<Code>(function->shared()->code()));
    CHECK(Compiler::Analyze(&info));
    CHECK(Compiler::EnsureDeoptimizationSupport(&info));

    Pipeline pipeline(&info);
    Linkage linkage(info.zone(), &info);
    Handle<Code> code = pipeline.GenerateCodeForMachineGraph(&linkage, graph);
    CHECK(!code.is_null());
    function->ReplaceCode(*code);
    return function;
  }
};
}
}
}  // namespace v8::internal::compiler

#endif  // V8_CCTEST_COMPILER_FUNCTION_TESTER_H_
