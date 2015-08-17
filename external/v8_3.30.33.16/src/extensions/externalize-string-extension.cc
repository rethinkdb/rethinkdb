// Copyright 2010 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/extensions/externalize-string-extension.h"

namespace v8 {
namespace internal {

template <typename Char, typename Base>
class SimpleStringResource : public Base {
 public:
  // Takes ownership of |data|.
  SimpleStringResource(Char* data, size_t length)
      : data_(data),
        length_(length) {}

  virtual ~SimpleStringResource() { delete[] data_; }

  virtual const Char* data() const { return data_; }

  virtual size_t length() const { return length_; }

 private:
  Char* const data_;
  const size_t length_;
};


typedef SimpleStringResource<char, v8::String::ExternalOneByteStringResource>
    SimpleOneByteStringResource;
typedef SimpleStringResource<uc16, v8::String::ExternalStringResource>
    SimpleTwoByteStringResource;


const char* const ExternalizeStringExtension::kSource =
    "native function externalizeString();"
    "native function isOneByteString();";

v8::Handle<v8::FunctionTemplate>
ExternalizeStringExtension::GetNativeFunctionTemplate(
    v8::Isolate* isolate, v8::Handle<v8::String> str) {
  if (strcmp(*v8::String::Utf8Value(str), "externalizeString") == 0) {
    return v8::FunctionTemplate::New(isolate,
                                     ExternalizeStringExtension::Externalize);
  } else {
    DCHECK(strcmp(*v8::String::Utf8Value(str), "isOneByteString") == 0);
    return v8::FunctionTemplate::New(isolate,
                                     ExternalizeStringExtension::IsOneByte);
  }
}


void ExternalizeStringExtension::Externalize(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() < 1 || !args[0]->IsString()) {
    args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(
        args.GetIsolate(),
        "First parameter to externalizeString() must be a string."));
    return;
  }
  bool force_two_byte = false;
  if (args.Length() >= 2) {
    if (args[1]->IsBoolean()) {
      force_two_byte = args[1]->BooleanValue();
    } else {
      args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(
        args.GetIsolate(),
        "Second parameter to externalizeString() must be a boolean."));
      return;
    }
  }
  bool result = false;
  Handle<String> string = Utils::OpenHandle(*args[0].As<v8::String>());
  if (string->IsExternalString()) {
    args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(
        args.GetIsolate(),
        "externalizeString() can't externalize twice."));
    return;
  }
  if (string->IsOneByteRepresentation() && !force_two_byte) {
    uint8_t* data = new uint8_t[string->length()];
    String::WriteToFlat(*string, data, 0, string->length());
    SimpleOneByteStringResource* resource = new SimpleOneByteStringResource(
        reinterpret_cast<char*>(data), string->length());
    result = string->MakeExternal(resource);
    if (result) {
      i::Isolate* isolate = reinterpret_cast<i::Isolate*>(args.GetIsolate());
      isolate->heap()->external_string_table()->AddString(*string);
    }
    if (!result) delete resource;
  } else {
    uc16* data = new uc16[string->length()];
    String::WriteToFlat(*string, data, 0, string->length());
    SimpleTwoByteStringResource* resource = new SimpleTwoByteStringResource(
        data, string->length());
    result = string->MakeExternal(resource);
    if (result) {
      i::Isolate* isolate = reinterpret_cast<i::Isolate*>(args.GetIsolate());
      isolate->heap()->external_string_table()->AddString(*string);
    }
    if (!result) delete resource;
  }
  if (!result) {
    args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(
        args.GetIsolate(), "externalizeString() failed."));
    return;
  }
}


void ExternalizeStringExtension::IsOneByte(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() != 1 || !args[0]->IsString()) {
    args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(
        args.GetIsolate(),
        "isOneByteString() requires a single string argument."));
    return;
  }
  bool is_one_byte =
      Utils::OpenHandle(*args[0].As<v8::String>())->IsOneByteRepresentation();
  args.GetReturnValue().Set(is_one_byte);
}

} }  // namespace v8::internal
