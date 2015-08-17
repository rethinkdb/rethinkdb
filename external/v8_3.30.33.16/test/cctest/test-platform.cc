// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include "src/base/build_config.h"
#include "src/base/platform/platform.h"
#include "test/cctest/cctest.h"

#ifdef V8_CC_GNU

static uintptr_t sp_addr = 0;

void GetStackPointer(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if V8_HOST_ARCH_X64
  __asm__ __volatile__("mov %%rsp, %0" : "=g"(sp_addr));
#elif V8_HOST_ARCH_IA32
  __asm__ __volatile__("mov %%esp, %0" : "=g"(sp_addr));
#elif V8_HOST_ARCH_ARM
  __asm__ __volatile__("str %%sp, %0" : "=g"(sp_addr));
#elif V8_HOST_ARCH_ARM64
  __asm__ __volatile__("mov x16, sp; str x16, %0" : "=g"(sp_addr));
#elif V8_HOST_ARCH_MIPS
  __asm__ __volatile__("sw $sp, %0" : "=g"(sp_addr));
#elif V8_HOST_ARCH_MIPS64
  __asm__ __volatile__("sd $sp, %0" : "=g"(sp_addr));
#else
#error Host architecture was not detected as supported by v8
#endif

  args.GetReturnValue().Set(v8::Integer::NewFromUnsigned(
      args.GetIsolate(), static_cast<uint32_t>(sp_addr)));
}


TEST(StackAlignment) {
  v8::Isolate* isolate = CcTest::isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Handle<v8::ObjectTemplate> global_template =
      v8::ObjectTemplate::New(isolate);
  global_template->Set(v8_str("get_stack_pointer"),
                       v8::FunctionTemplate::New(isolate, GetStackPointer));

  LocalContext env(NULL, global_template);
  CompileRun(
      "function foo() {"
      "  return get_stack_pointer();"
      "}");

  v8::Local<v8::Object> global_object = env->Global();
  v8::Local<v8::Function> foo =
      v8::Local<v8::Function>::Cast(global_object->Get(v8_str("foo")));

  v8::Local<v8::Value> result = foo->Call(global_object, 0, NULL);
  CHECK_EQ(0, result->Uint32Value() % v8::base::OS::ActivationFrameAlignment());
}

#endif  // V8_CC_GNU
