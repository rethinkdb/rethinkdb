// Copyright 2012 the V8 project authors. All rights reserved.
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

#ifndef V8_ARGUMENTS_H_
#define V8_ARGUMENTS_H_

#include "allocation.h"

namespace v8 {
namespace internal {

// Arguments provides access to runtime call parameters.
//
// It uses the fact that the instance fields of Arguments
// (length_, arguments_) are "overlayed" with the parameters
// (no. of parameters, and the parameter pointer) passed so
// that inside the C++ function, the parameters passed can
// be accessed conveniently:
//
//   Object* Runtime_function(Arguments args) {
//     ... use args[i] here ...
//   }

class Arguments BASE_EMBEDDED {
 public:
  Arguments(int length, Object** arguments)
      : length_(length), arguments_(arguments) { }

  Object*& operator[] (int index) {
    ASSERT(0 <= index && index < length_);
    return arguments_[-index];
  }

  template <class S> Handle<S> at(int index) {
    Object** value = &((*this)[index]);
    // This cast checks that the object we're accessing does indeed have the
    // expected type.
    S::cast(*value);
    return Handle<S>(reinterpret_cast<S**>(value));
  }

  int smi_at(int index) {
    return Smi::cast((*this)[index])->value();
  }

  double number_at(int index) {
    return (*this)[index]->Number();
  }

  // Get the total number of arguments including the receiver.
  int length() const { return length_; }

  Object** arguments() { return arguments_; }

 private:
  int length_;
  Object** arguments_;
};


// Custom arguments replicate a small segment of stack that can be
// accessed through an Arguments object the same way the actual stack
// can.
class CustomArguments : public Relocatable {
 public:
  inline CustomArguments(Isolate* isolate,
                         Object* data,
                         Object* self,
                         JSObject* holder) : Relocatable(isolate) {
    ASSERT(reinterpret_cast<Object*>(isolate)->IsSmi());
    values_[3] = self;
    values_[2] = holder;
    values_[1] = data;
    values_[0] = reinterpret_cast<Object*>(isolate);
  }

  inline explicit CustomArguments(Isolate* isolate) : Relocatable(isolate) {
#ifdef DEBUG
    for (size_t i = 0; i < ARRAY_SIZE(values_); i++) {
      values_[i] = reinterpret_cast<Object*>(kZapValue);
    }
#endif
  }

  void IterateInstance(ObjectVisitor* v);
  Object** end() { return values_ + ARRAY_SIZE(values_) - 1; }

 private:
  Object* values_[4];
};


#define DECLARE_RUNTIME_FUNCTION(Type, Name)    \
Type Name(Arguments args, Isolate* isolate)


#define RUNTIME_FUNCTION(Type, Name)            \
Type Name(Arguments args, Isolate* isolate)


#define RUNTIME_ARGUMENTS(isolate, args) args, isolate


} }  // namespace v8::internal

#endif  // V8_ARGUMENTS_H_
