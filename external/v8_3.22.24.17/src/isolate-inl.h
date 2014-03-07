// Copyright 2011 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef V8_ISOLATE_INL_H_
#define V8_ISOLATE_INL_H_

#include "debug.h"
#include "isolate.h"
#include "utils/random-number-generator.h"

namespace v8 {
namespace internal {


SaveContext::SaveContext(Isolate* isolate)
  : isolate_(isolate),
    prev_(isolate->save_context()) {
  if (isolate->context() != NULL) {
    context_ = Handle<Context>(isolate->context());
  }
  isolate->set_save_context(this);

  c_entry_fp_ = isolate->c_entry_fp(isolate->thread_local_top());
}


bool Isolate::IsCodePreAgingActive() {
  return FLAG_optimize_for_size && FLAG_age_code && !IsDebuggerActive();
}


bool Isolate::IsDebuggerActive() {
#ifdef ENABLE_DEBUGGER_SUPPORT
  if (!NoBarrier_Load(&debugger_initialized_)) return false;
  return debugger()->IsDebuggerActive();
#else
  return false;
#endif
}


bool Isolate::DebuggerHasBreakPoints() {
#ifdef ENABLE_DEBUGGER_SUPPORT
  return debug()->has_break_points();
#else
  return false;
#endif
}


RandomNumberGenerator* Isolate::random_number_generator() {
  if (random_number_generator_ == NULL) {
    random_number_generator_ = new RandomNumberGenerator;
  }
  return random_number_generator_;
}

} }  // namespace v8::internal

#endif  // V8_ISOLATE_INL_H_
