// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_API_H_
#define V8_API_H_

#include "src/v8.h"

#include "include/v8-testing.h"
#include "src/contexts.h"
#include "src/factory.h"
#include "src/isolate.h"
#include "src/list-inl.h"

namespace v8 {

// Constants used in the implementation of the API.  The most natural thing
// would usually be to place these with the classes that use them, but
// we want to keep them out of v8.h because it is an externally
// visible file.
class Consts {
 public:
  enum TemplateType {
    FUNCTION_TEMPLATE = 0,
    OBJECT_TEMPLATE = 1
  };
};


// Utilities for working with neander-objects, primitive
// env-independent JSObjects used by the api.
class NeanderObject {
 public:
  explicit NeanderObject(v8::internal::Isolate* isolate, int size);
  explicit inline NeanderObject(v8::internal::Handle<v8::internal::Object> obj);
  explicit inline NeanderObject(v8::internal::Object* obj);
  inline v8::internal::Object* get(int index);
  inline void set(int index, v8::internal::Object* value);
  inline v8::internal::Handle<v8::internal::JSObject> value() { return value_; }
  int size();
 private:
  v8::internal::Handle<v8::internal::JSObject> value_;
};


// Utilities for working with neander-arrays, a simple extensible
// array abstraction built on neander-objects.
class NeanderArray {
 public:
  explicit NeanderArray(v8::internal::Isolate* isolate);
  explicit inline NeanderArray(v8::internal::Handle<v8::internal::Object> obj);
  inline v8::internal::Handle<v8::internal::JSObject> value() {
    return obj_.value();
  }

  void add(internal::Isolate* isolate,
           v8::internal::Handle<v8::internal::Object> value);

  int length();

  v8::internal::Object* get(int index);
  // Change the value at an index to undefined value. If the index is
  // out of bounds, the request is ignored. Returns the old value.
  void set(int index, v8::internal::Object* value);
 private:
  NeanderObject obj_;
};


NeanderObject::NeanderObject(v8::internal::Handle<v8::internal::Object> obj)
    : value_(v8::internal::Handle<v8::internal::JSObject>::cast(obj)) { }


NeanderObject::NeanderObject(v8::internal::Object* obj)
    : value_(v8::internal::Handle<v8::internal::JSObject>(
        v8::internal::JSObject::cast(obj))) { }


NeanderArray::NeanderArray(v8::internal::Handle<v8::internal::Object> obj)
    : obj_(obj) { }


v8::internal::Object* NeanderObject::get(int offset) {
  DCHECK(value()->HasFastObjectElements());
  return v8::internal::FixedArray::cast(value()->elements())->get(offset);
}


void NeanderObject::set(int offset, v8::internal::Object* value) {
  DCHECK(value_->HasFastObjectElements());
  v8::internal::FixedArray::cast(value_->elements())->set(offset, value);
}


template <typename T> inline T ToCData(v8::internal::Object* obj) {
  STATIC_ASSERT(sizeof(T) == sizeof(v8::internal::Address));
  return reinterpret_cast<T>(
      reinterpret_cast<intptr_t>(
          v8::internal::Foreign::cast(obj)->foreign_address()));
}


template <typename T>
inline v8::internal::Handle<v8::internal::Object> FromCData(
    v8::internal::Isolate* isolate, T obj) {
  STATIC_ASSERT(sizeof(T) == sizeof(v8::internal::Address));
  return isolate->factory()->NewForeign(
      reinterpret_cast<v8::internal::Address>(reinterpret_cast<intptr_t>(obj)));
}


class ApiFunction {
 public:
  explicit ApiFunction(v8::internal::Address addr) : addr_(addr) { }
  v8::internal::Address address() { return addr_; }
 private:
  v8::internal::Address addr_;
};



class RegisteredExtension {
 public:
  explicit RegisteredExtension(Extension* extension);
  static void Register(RegisteredExtension* that);
  static void UnregisterAll();
  Extension* extension() { return extension_; }
  RegisteredExtension* next() { return next_; }
  static RegisteredExtension* first_extension() { return first_extension_; }
 private:
  Extension* extension_;
  RegisteredExtension* next_;
  static RegisteredExtension* first_extension_;
};


#define OPEN_HANDLE_LIST(V)                    \
  V(Template, TemplateInfo)                    \
  V(FunctionTemplate, FunctionTemplateInfo)    \
  V(ObjectTemplate, ObjectTemplateInfo)        \
  V(Signature, SignatureInfo)                  \
  V(AccessorSignature, FunctionTemplateInfo)   \
  V(TypeSwitch, TypeSwitchInfo)                \
  V(Data, Object)                              \
  V(RegExp, JSRegExp)                          \
  V(Object, JSObject)                          \
  V(Array, JSArray)                            \
  V(ArrayBuffer, JSArrayBuffer)                \
  V(ArrayBufferView, JSArrayBufferView)        \
  V(TypedArray, JSTypedArray)                  \
  V(Uint8Array, JSTypedArray)                  \
  V(Uint8ClampedArray, JSTypedArray)           \
  V(Int8Array, JSTypedArray)                   \
  V(Uint16Array, JSTypedArray)                 \
  V(Int16Array, JSTypedArray)                  \
  V(Uint32Array, JSTypedArray)                 \
  V(Int32Array, JSTypedArray)                  \
  V(Float32Array, JSTypedArray)                \
  V(Float64Array, JSTypedArray)                \
  V(DataView, JSDataView)                      \
  V(Name, Name)                                \
  V(String, String)                            \
  V(Symbol, Symbol)                            \
  V(Script, JSFunction)                        \
  V(UnboundScript, SharedFunctionInfo)         \
  V(Function, JSFunction)                      \
  V(Message, JSMessageObject)                  \
  V(Context, Context)                          \
  V(External, Object)                          \
  V(StackTrace, JSArray)                       \
  V(StackFrame, JSObject)                      \
  V(DeclaredAccessorDescriptor, DeclaredAccessorDescriptor)


class Utils {
 public:
  static inline bool ApiCheck(bool condition,
                              const char* location,
                              const char* message) {
    if (!condition) Utils::ReportApiFailure(location, message);
    return condition;
  }

  static Local<FunctionTemplate> ToFunctionTemplate(NeanderObject obj);
  static Local<ObjectTemplate> ToObjectTemplate(NeanderObject obj);

  static inline Local<Context> ToLocal(
      v8::internal::Handle<v8::internal::Context> obj);
  static inline Local<Value> ToLocal(
      v8::internal::Handle<v8::internal::Object> obj);
  static inline Local<Function> ToLocal(
      v8::internal::Handle<v8::internal::JSFunction> obj);
  static inline Local<Name> ToLocal(
      v8::internal::Handle<v8::internal::Name> obj);
  static inline Local<String> ToLocal(
      v8::internal::Handle<v8::internal::String> obj);
  static inline Local<Symbol> ToLocal(
      v8::internal::Handle<v8::internal::Symbol> obj);
  static inline Local<RegExp> ToLocal(
      v8::internal::Handle<v8::internal::JSRegExp> obj);
  static inline Local<Object> ToLocal(
      v8::internal::Handle<v8::internal::JSObject> obj);
  static inline Local<Array> ToLocal(
      v8::internal::Handle<v8::internal::JSArray> obj);
  static inline Local<ArrayBuffer> ToLocal(
      v8::internal::Handle<v8::internal::JSArrayBuffer> obj);
  static inline Local<ArrayBufferView> ToLocal(
      v8::internal::Handle<v8::internal::JSArrayBufferView> obj);
  static inline Local<DataView> ToLocal(
      v8::internal::Handle<v8::internal::JSDataView> obj);

  static inline Local<TypedArray> ToLocal(
      v8::internal::Handle<v8::internal::JSTypedArray> obj);
  static inline Local<Uint8Array> ToLocalUint8Array(
      v8::internal::Handle<v8::internal::JSTypedArray> obj);
  static inline Local<Uint8ClampedArray> ToLocalUint8ClampedArray(
      v8::internal::Handle<v8::internal::JSTypedArray> obj);
  static inline Local<Int8Array> ToLocalInt8Array(
      v8::internal::Handle<v8::internal::JSTypedArray> obj);
  static inline Local<Uint16Array> ToLocalUint16Array(
      v8::internal::Handle<v8::internal::JSTypedArray> obj);
  static inline Local<Int16Array> ToLocalInt16Array(
      v8::internal::Handle<v8::internal::JSTypedArray> obj);
  static inline Local<Uint32Array> ToLocalUint32Array(
      v8::internal::Handle<v8::internal::JSTypedArray> obj);
  static inline Local<Int32Array> ToLocalInt32Array(
      v8::internal::Handle<v8::internal::JSTypedArray> obj);
  static inline Local<Float32Array> ToLocalFloat32Array(
      v8::internal::Handle<v8::internal::JSTypedArray> obj);
  static inline Local<Float64Array> ToLocalFloat64Array(
      v8::internal::Handle<v8::internal::JSTypedArray> obj);

  static inline Local<Message> MessageToLocal(
      v8::internal::Handle<v8::internal::Object> obj);
  static inline Local<Promise> PromiseToLocal(
      v8::internal::Handle<v8::internal::JSObject> obj);
  static inline Local<StackTrace> StackTraceToLocal(
      v8::internal::Handle<v8::internal::JSArray> obj);
  static inline Local<StackFrame> StackFrameToLocal(
      v8::internal::Handle<v8::internal::JSObject> obj);
  static inline Local<Number> NumberToLocal(
      v8::internal::Handle<v8::internal::Object> obj);
  static inline Local<Integer> IntegerToLocal(
      v8::internal::Handle<v8::internal::Object> obj);
  static inline Local<Uint32> Uint32ToLocal(
      v8::internal::Handle<v8::internal::Object> obj);
  static inline Local<FunctionTemplate> ToLocal(
      v8::internal::Handle<v8::internal::FunctionTemplateInfo> obj);
  static inline Local<ObjectTemplate> ToLocal(
      v8::internal::Handle<v8::internal::ObjectTemplateInfo> obj);
  static inline Local<Signature> ToLocal(
      v8::internal::Handle<v8::internal::SignatureInfo> obj);
  static inline Local<AccessorSignature> AccessorSignatureToLocal(
      v8::internal::Handle<v8::internal::FunctionTemplateInfo> obj);
  static inline Local<TypeSwitch> ToLocal(
      v8::internal::Handle<v8::internal::TypeSwitchInfo> obj);
  static inline Local<External> ExternalToLocal(
      v8::internal::Handle<v8::internal::JSObject> obj);
  static inline Local<DeclaredAccessorDescriptor> ToLocal(
      v8::internal::Handle<v8::internal::DeclaredAccessorDescriptor> obj);

#define DECLARE_OPEN_HANDLE(From, To) \
  static inline v8::internal::Handle<v8::internal::To> \
      OpenHandle(const From* that, bool allow_empty_handle = false);

OPEN_HANDLE_LIST(DECLARE_OPEN_HANDLE)

#undef DECLARE_OPEN_HANDLE

  template<class From, class To>
  static inline Local<To> Convert(v8::internal::Handle<From> obj) {
    DCHECK(obj.is_null() || !obj->IsTheHole());
    return Local<To>(reinterpret_cast<To*>(obj.location()));
  }

  template <class T>
  static inline v8::internal::Handle<v8::internal::Object> OpenPersistent(
      const v8::Persistent<T>& persistent) {
    return v8::internal::Handle<v8::internal::Object>(
        reinterpret_cast<v8::internal::Object**>(persistent.val_));
  }

  template <class T>
  static inline v8::internal::Handle<v8::internal::Object> OpenPersistent(
      v8::Persistent<T>* persistent) {
    return OpenPersistent(*persistent);
  }

  template <class From, class To>
  static inline v8::internal::Handle<To> OpenHandle(v8::Local<From> handle) {
    return OpenHandle(*handle);
  }

 private:
  static void ReportApiFailure(const char* location, const char* message);
};


template <class T>
v8::internal::Handle<T> v8::internal::Handle<T>::EscapeFrom(
    v8::EscapableHandleScope* scope) {
  v8::internal::Handle<T> handle;
  if (!is_null()) {
    handle = *this;
  }
  return Utils::OpenHandle(*scope->Escape(Utils::ToLocal(handle)), true);
}


template <class T>
inline T* ToApi(v8::internal::Handle<v8::internal::Object> obj) {
  return reinterpret_cast<T*>(obj.location());
}

template <class T>
inline v8::Local<T> ToApiHandle(
    v8::internal::Handle<v8::internal::Object> obj) {
  return Utils::Convert<v8::internal::Object, T>(obj);
}


// Implementations of ToLocal

#define MAKE_TO_LOCAL(Name, From, To)                                       \
  Local<v8::To> Utils::Name(v8::internal::Handle<v8::internal::From> obj) { \
    return Convert<v8::internal::From, v8::To>(obj);  \
  }


#define MAKE_TO_LOCAL_TYPED_ARRAY(Type, typeName, TYPE, ctype, size)        \
  Local<v8::Type##Array> Utils::ToLocal##Type##Array(                       \
      v8::internal::Handle<v8::internal::JSTypedArray> obj) {               \
    DCHECK(obj->type() == kExternal##Type##Array);                          \
    return Convert<v8::internal::JSTypedArray, v8::Type##Array>(obj);       \
  }


MAKE_TO_LOCAL(ToLocal, Context, Context)
MAKE_TO_LOCAL(ToLocal, Object, Value)
MAKE_TO_LOCAL(ToLocal, JSFunction, Function)
MAKE_TO_LOCAL(ToLocal, Name, Name)
MAKE_TO_LOCAL(ToLocal, String, String)
MAKE_TO_LOCAL(ToLocal, Symbol, Symbol)
MAKE_TO_LOCAL(ToLocal, JSRegExp, RegExp)
MAKE_TO_LOCAL(ToLocal, JSObject, Object)
MAKE_TO_LOCAL(ToLocal, JSArray, Array)
MAKE_TO_LOCAL(ToLocal, JSArrayBuffer, ArrayBuffer)
MAKE_TO_LOCAL(ToLocal, JSArrayBufferView, ArrayBufferView)
MAKE_TO_LOCAL(ToLocal, JSDataView, DataView)
MAKE_TO_LOCAL(ToLocal, JSTypedArray, TypedArray)

TYPED_ARRAYS(MAKE_TO_LOCAL_TYPED_ARRAY)

MAKE_TO_LOCAL(ToLocal, FunctionTemplateInfo, FunctionTemplate)
MAKE_TO_LOCAL(ToLocal, ObjectTemplateInfo, ObjectTemplate)
MAKE_TO_LOCAL(ToLocal, SignatureInfo, Signature)
MAKE_TO_LOCAL(AccessorSignatureToLocal, FunctionTemplateInfo, AccessorSignature)
MAKE_TO_LOCAL(ToLocal, TypeSwitchInfo, TypeSwitch)
MAKE_TO_LOCAL(MessageToLocal, Object, Message)
MAKE_TO_LOCAL(PromiseToLocal, JSObject, Promise)
MAKE_TO_LOCAL(StackTraceToLocal, JSArray, StackTrace)
MAKE_TO_LOCAL(StackFrameToLocal, JSObject, StackFrame)
MAKE_TO_LOCAL(NumberToLocal, Object, Number)
MAKE_TO_LOCAL(IntegerToLocal, Object, Integer)
MAKE_TO_LOCAL(Uint32ToLocal, Object, Uint32)
MAKE_TO_LOCAL(ExternalToLocal, JSObject, External)
MAKE_TO_LOCAL(ToLocal, DeclaredAccessorDescriptor, DeclaredAccessorDescriptor)

#undef MAKE_TO_LOCAL_TYPED_ARRAY
#undef MAKE_TO_LOCAL


// Implementations of OpenHandle

#define MAKE_OPEN_HANDLE(From, To)                                          \
  v8::internal::Handle<v8::internal::To> Utils::OpenHandle(                 \
    const v8::From* that, bool allow_empty_handle) {                        \
    EXTRA_CHECK(allow_empty_handle || that != NULL);                        \
    EXTRA_CHECK(that == NULL ||                                             \
        (*reinterpret_cast<v8::internal::Object* const*>(that))->Is##To()); \
    return v8::internal::Handle<v8::internal::To>(                          \
        reinterpret_cast<v8::internal::To**>(const_cast<v8::From*>(that))); \
  }

OPEN_HANDLE_LIST(MAKE_OPEN_HANDLE)

#undef MAKE_OPEN_HANDLE
#undef OPEN_HANDLE_LIST


namespace internal {

// Tracks string usage to help make better decisions when
// externalizing strings.
//
// Implementation note: internally this class only tracks fresh
// strings and keeps a single use counter for them.
class StringTracker {
 public:
  // Records that the given string's characters were copied to some
  // external buffer. If this happens often we should honor
  // externalization requests for the string.
  void RecordWrite(Handle<String> string) {
    Address address = reinterpret_cast<Address>(*string);
    Address top = isolate_->heap()->NewSpaceTop();
    if (IsFreshString(address, top)) {
      IncrementUseCount(top);
    }
  }

  // Estimates freshness and use frequency of the given string based
  // on how close it is to the new space top and the recorded usage
  // history.
  inline bool IsFreshUnusedString(Handle<String> string) {
    Address address = reinterpret_cast<Address>(*string);
    Address top = isolate_->heap()->NewSpaceTop();
    return IsFreshString(address, top) && IsUseCountLow(top);
  }

 private:
  StringTracker() : use_count_(0), last_top_(NULL), isolate_(NULL) { }

  static inline bool IsFreshString(Address string, Address top) {
    return top - kFreshnessLimit <= string && string <= top;
  }

  inline bool IsUseCountLow(Address top) {
    if (last_top_ != top) return true;
    return use_count_ < kUseLimit;
  }

  inline void IncrementUseCount(Address top) {
    if (last_top_ != top) {
      use_count_ = 0;
      last_top_ = top;
    }
    ++use_count_;
  }

  // Single use counter shared by all fresh strings.
  int use_count_;

  // Last new space top when the use count above was valid.
  Address last_top_;

  Isolate* isolate_;

  // How close to the new space top a fresh string has to be.
  static const int kFreshnessLimit = 1024;

  // The number of uses required to consider a string useful.
  static const int kUseLimit = 32;

  friend class Isolate;

  DISALLOW_COPY_AND_ASSIGN(StringTracker);
};


class DeferredHandles {
 public:
  ~DeferredHandles();

 private:
  DeferredHandles(Object** first_block_limit, Isolate* isolate)
      : next_(NULL),
        previous_(NULL),
        first_block_limit_(first_block_limit),
        isolate_(isolate) {
    isolate->LinkDeferredHandles(this);
  }

  void Iterate(ObjectVisitor* v);

  List<Object**> blocks_;
  DeferredHandles* next_;
  DeferredHandles* previous_;
  Object** first_block_limit_;
  Isolate* isolate_;

  friend class HandleScopeImplementer;
  friend class Isolate;
};


// This class is here in order to be able to declare it a friend of
// HandleScope.  Moving these methods to be members of HandleScope would be
// neat in some ways, but it would expose internal implementation details in
// our public header file, which is undesirable.
//
// An isolate has a single instance of this class to hold the current thread's
// data. In multithreaded V8 programs this data is copied in and out of storage
// so that the currently executing thread always has its own copy of this
// data.
class HandleScopeImplementer {
 public:
  explicit HandleScopeImplementer(Isolate* isolate)
      : isolate_(isolate),
        blocks_(0),
        entered_contexts_(0),
        saved_contexts_(0),
        spare_(NULL),
        call_depth_(0),
        last_handle_before_deferred_block_(NULL) { }

  ~HandleScopeImplementer() {
    DeleteArray(spare_);
  }

  // Threading support for handle data.
  static int ArchiveSpacePerThread();
  char* RestoreThread(char* from);
  char* ArchiveThread(char* to);
  void FreeThreadResources();

  // Garbage collection support.
  void Iterate(v8::internal::ObjectVisitor* v);
  static char* Iterate(v8::internal::ObjectVisitor* v, char* data);


  inline internal::Object** GetSpareOrNewBlock();
  inline void DeleteExtensions(internal::Object** prev_limit);

  inline void IncrementCallDepth() {call_depth_++;}
  inline void DecrementCallDepth() {call_depth_--;}
  inline bool CallDepthIsZero() { return call_depth_ == 0; }

  inline void EnterContext(Handle<Context> context);
  inline void LeaveContext();
  inline bool LastEnteredContextWas(Handle<Context> context);

  // Returns the last entered context or an empty handle if no
  // contexts have been entered.
  inline Handle<Context> LastEnteredContext();

  inline void SaveContext(Context* context);
  inline Context* RestoreContext();
  inline bool HasSavedContexts();

  inline List<internal::Object**>* blocks() { return &blocks_; }
  Isolate* isolate() const { return isolate_; }

  void ReturnBlock(Object** block) {
    DCHECK(block != NULL);
    if (spare_ != NULL) DeleteArray(spare_);
    spare_ = block;
  }

 private:
  void ResetAfterArchive() {
    blocks_.Initialize(0);
    entered_contexts_.Initialize(0);
    saved_contexts_.Initialize(0);
    spare_ = NULL;
    last_handle_before_deferred_block_ = NULL;
    call_depth_ = 0;
  }

  void Free() {
    DCHECK(blocks_.length() == 0);
    DCHECK(entered_contexts_.length() == 0);
    DCHECK(saved_contexts_.length() == 0);
    blocks_.Free();
    entered_contexts_.Free();
    saved_contexts_.Free();
    if (spare_ != NULL) {
      DeleteArray(spare_);
      spare_ = NULL;
    }
    DCHECK(call_depth_ == 0);
  }

  void BeginDeferredScope();
  DeferredHandles* Detach(Object** prev_limit);

  Isolate* isolate_;
  List<internal::Object**> blocks_;
  // Used as a stack to keep track of entered contexts.
  List<Context*> entered_contexts_;
  // Used as a stack to keep track of saved contexts.
  List<Context*> saved_contexts_;
  Object** spare_;
  int call_depth_;
  Object** last_handle_before_deferred_block_;
  // This is only used for threading support.
  HandleScopeData handle_scope_data_;

  void IterateThis(ObjectVisitor* v);
  char* RestoreThreadHelper(char* from);
  char* ArchiveThreadHelper(char* to);

  friend class DeferredHandles;
  friend class DeferredHandleScope;

  DISALLOW_COPY_AND_ASSIGN(HandleScopeImplementer);
};


const int kHandleBlockSize = v8::internal::KB - 2;  // fit in one page


void HandleScopeImplementer::SaveContext(Context* context) {
  saved_contexts_.Add(context);
}


Context* HandleScopeImplementer::RestoreContext() {
  return saved_contexts_.RemoveLast();
}


bool HandleScopeImplementer::HasSavedContexts() {
  return !saved_contexts_.is_empty();
}


void HandleScopeImplementer::EnterContext(Handle<Context> context) {
  entered_contexts_.Add(*context);
}


void HandleScopeImplementer::LeaveContext() {
  entered_contexts_.RemoveLast();
}


bool HandleScopeImplementer::LastEnteredContextWas(Handle<Context> context) {
  return !entered_contexts_.is_empty() && entered_contexts_.last() == *context;
}


Handle<Context> HandleScopeImplementer::LastEnteredContext() {
  if (entered_contexts_.is_empty()) return Handle<Context>::null();
  return Handle<Context>(entered_contexts_.last());
}


// If there's a spare block, use it for growing the current scope.
internal::Object** HandleScopeImplementer::GetSpareOrNewBlock() {
  internal::Object** block = (spare_ != NULL) ?
      spare_ :
      NewArray<internal::Object*>(kHandleBlockSize);
  spare_ = NULL;
  return block;
}


void HandleScopeImplementer::DeleteExtensions(internal::Object** prev_limit) {
  while (!blocks_.is_empty()) {
    internal::Object** block_start = blocks_.last();
    internal::Object** block_limit = block_start + kHandleBlockSize;
#ifdef DEBUG
    // SealHandleScope may make the prev_limit to point inside the block.
    if (block_start <= prev_limit && prev_limit <= block_limit) {
#ifdef ENABLE_HANDLE_ZAPPING
      internal::HandleScope::ZapRange(prev_limit, block_limit);
#endif
      break;
    }
#else
    if (prev_limit == block_limit) break;
#endif

    blocks_.RemoveLast();
#ifdef ENABLE_HANDLE_ZAPPING
    internal::HandleScope::ZapRange(block_start, block_limit);
#endif
    if (spare_ != NULL) {
      DeleteArray(spare_);
    }
    spare_ = block_start;
  }
  DCHECK((blocks_.is_empty() && prev_limit == NULL) ||
         (!blocks_.is_empty() && prev_limit != NULL));
}


// Interceptor functions called from generated inline caches to notify
// CPU profiler that external callbacks are invoked.
void InvokeAccessorGetterCallback(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info,
    v8::AccessorNameGetterCallback getter);

void InvokeFunctionCallback(const v8::FunctionCallbackInfo<v8::Value>& info,
                            v8::FunctionCallback callback);

class Testing {
 public:
  static v8::Testing::StressType stress_type() { return stress_type_; }
  static void set_stress_type(v8::Testing::StressType stress_type) {
    stress_type_ = stress_type;
  }

 private:
  static v8::Testing::StressType stress_type_;
};

} }  // namespace v8::internal

#endif  // V8_API_H_
