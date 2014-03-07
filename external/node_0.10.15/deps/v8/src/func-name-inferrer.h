// Copyright 2006-2009 the V8 project authors. All rights reserved.
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

#ifndef V8_FUNC_NAME_INFERRER_H_
#define V8_FUNC_NAME_INFERRER_H_

namespace v8 {
namespace internal {

class Isolate;

// FuncNameInferrer is a stateful class that is used to perform name
// inference for anonymous functions during static analysis of source code.
// Inference is performed in cases when an anonymous function is assigned
// to a variable or a property (see test-func-name-inference.cc for examples.)
//
// The basic idea is that during parsing of LHSs of certain expressions
// (assignments, declarations, object literals) we collect name strings,
// and during parsing of the RHS, a function literal can be collected. After
// parsing the RHS we can infer a name for function literals that do not have
// a name.
class FuncNameInferrer : public ZoneObject {
 public:
  FuncNameInferrer(Isolate* isolate, Zone* zone);

  // Returns whether we have entered name collection state.
  bool IsOpen() const { return !entries_stack_.is_empty(); }

  // Pushes an enclosing the name of enclosing function onto names stack.
  void PushEnclosingName(Handle<String> name);

  // Enters name collection state.
  void Enter() {
    entries_stack_.Add(names_stack_.length(), zone());
  }

  // Pushes an encountered name onto names stack when in collection state.
  void PushLiteralName(Handle<String> name);

  void PushVariableName(Handle<String> name);

  // Adds a function to infer name for.
  void AddFunction(FunctionLiteral* func_to_infer) {
    if (IsOpen()) {
      funcs_to_infer_.Add(func_to_infer, zone());
    }
  }

  void RemoveLastFunction() {
    if (IsOpen() && !funcs_to_infer_.is_empty()) {
      funcs_to_infer_.RemoveLast();
    }
  }

  // Infers a function name and leaves names collection state.
  void Infer() {
    ASSERT(IsOpen());
    if (!funcs_to_infer_.is_empty()) {
      InferFunctionsNames();
    }
  }

  // Leaves names collection state.
  void Leave() {
    ASSERT(IsOpen());
    names_stack_.Rewind(entries_stack_.RemoveLast());
    if (entries_stack_.is_empty())
      funcs_to_infer_.Clear();
  }

 private:
  enum NameType {
    kEnclosingConstructorName,
    kLiteralName,
    kVariableName
  };
  struct Name {
    Name(Handle<String> name, NameType type) : name(name), type(type) { }
    Handle<String> name;
    NameType type;
  };

  Isolate* isolate() { return isolate_; }
  Zone* zone() const { return zone_; }

  // Constructs a full name in dotted notation from gathered names.
  Handle<String> MakeNameFromStack();

  // A helper function for MakeNameFromStack.
  Handle<String> MakeNameFromStackHelper(int pos, Handle<String> prev);

  // Performs name inferring for added functions.
  void InferFunctionsNames();

  Isolate* isolate_;
  ZoneList<int> entries_stack_;
  ZoneList<Name> names_stack_;
  ZoneList<FunctionLiteral*> funcs_to_infer_;
  Zone* zone_;

  DISALLOW_COPY_AND_ASSIGN(FuncNameInferrer);
};


} }  // namespace v8::internal

#endif  // V8_FUNC_NAME_INFERRER_H_
