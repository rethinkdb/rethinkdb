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

#include "v8.h"

#include "api.h"
#include "arguments.h"
#include "bootstrapper.h"
#include "builtins.h"
#include "cpu-profiler.h"
#include "gdb-jit.h"
#include "ic-inl.h"
#include "heap-profiler.h"
#include "mark-compact.h"
#include "stub-cache.h"
#include "vm-state-inl.h"

namespace v8 {
namespace internal {

namespace {

// Arguments object passed to C++ builtins.
template <BuiltinExtraArguments extra_args>
class BuiltinArguments : public Arguments {
 public:
  BuiltinArguments(int length, Object** arguments)
      : Arguments(length, arguments) { }

  Object*& operator[] (int index) {
    ASSERT(index < length());
    return Arguments::operator[](index);
  }

  template <class S> Handle<S> at(int index) {
    ASSERT(index < length());
    return Arguments::at<S>(index);
  }

  Handle<Object> receiver() {
    return Arguments::at<Object>(0);
  }

  Handle<JSFunction> called_function() {
    STATIC_ASSERT(extra_args == NEEDS_CALLED_FUNCTION);
    return Arguments::at<JSFunction>(Arguments::length() - 1);
  }

  // Gets the total number of arguments including the receiver (but
  // excluding extra arguments).
  int length() const {
    STATIC_ASSERT(extra_args == NO_EXTRA_ARGUMENTS);
    return Arguments::length();
  }

#ifdef DEBUG
  void Verify() {
    // Check we have at least the receiver.
    ASSERT(Arguments::length() >= 1);
  }
#endif
};


// Specialize BuiltinArguments for the called function extra argument.

template <>
int BuiltinArguments<NEEDS_CALLED_FUNCTION>::length() const {
  return Arguments::length() - 1;
}

#ifdef DEBUG
template <>
void BuiltinArguments<NEEDS_CALLED_FUNCTION>::Verify() {
  // Check we have at least the receiver and the called function.
  ASSERT(Arguments::length() >= 2);
  // Make sure cast to JSFunction succeeds.
  called_function();
}
#endif


#define DEF_ARG_TYPE(name, spec)                      \
  typedef BuiltinArguments<spec> name##ArgumentsType;
BUILTIN_LIST_C(DEF_ARG_TYPE)
#undef DEF_ARG_TYPE

}  // namespace

// ----------------------------------------------------------------------------
// Support macro for defining builtins in C++.
// ----------------------------------------------------------------------------
//
// A builtin function is defined by writing:
//
//   BUILTIN(name) {
//     ...
//   }
//
// In the body of the builtin function the arguments can be accessed
// through the BuiltinArguments object args.

#ifdef DEBUG

#define BUILTIN(name)                                            \
  MUST_USE_RESULT static MaybeObject* Builtin_Impl_##name(       \
      name##ArgumentsType args, Isolate* isolate);               \
  MUST_USE_RESULT static MaybeObject* Builtin_##name(            \
      int args_length, Object** args_object, Isolate* isolate) { \
    name##ArgumentsType args(args_length, args_object);          \
    args.Verify();                                               \
    return Builtin_Impl_##name(args, isolate);                   \
  }                                                              \
  MUST_USE_RESULT static MaybeObject* Builtin_Impl_##name(       \
      name##ArgumentsType args, Isolate* isolate)

#else  // For release mode.

#define BUILTIN(name)                                            \
  static MaybeObject* Builtin_impl##name(                        \
      name##ArgumentsType args, Isolate* isolate);               \
  static MaybeObject* Builtin_##name(                            \
      int args_length, Object** args_object, Isolate* isolate) { \
    name##ArgumentsType args(args_length, args_object);          \
    return Builtin_impl##name(args, isolate);                    \
  }                                                              \
  static MaybeObject* Builtin_impl##name(                        \
      name##ArgumentsType args, Isolate* isolate)
#endif


static inline bool CalledAsConstructor(Isolate* isolate) {
#ifdef DEBUG
  // Calculate the result using a full stack frame iterator and check
  // that the state of the stack is as we assume it to be in the
  // code below.
  StackFrameIterator it(isolate);
  ASSERT(it.frame()->is_exit());
  it.Advance();
  StackFrame* frame = it.frame();
  bool reference_result = frame->is_construct();
#endif
  Address fp = Isolate::c_entry_fp(isolate->thread_local_top());
  // Because we know fp points to an exit frame we can use the relevant
  // part of ExitFrame::ComputeCallerState directly.
  const int kCallerOffset = ExitFrameConstants::kCallerFPOffset;
  Address caller_fp = Memory::Address_at(fp + kCallerOffset);
  // This inlines the part of StackFrame::ComputeType that grabs the
  // type of the current frame.  Note that StackFrame::ComputeType
  // has been specialized for each architecture so if any one of them
  // changes this code has to be changed as well.
  const int kMarkerOffset = StandardFrameConstants::kMarkerOffset;
  const Smi* kConstructMarker = Smi::FromInt(StackFrame::CONSTRUCT);
  Object* marker = Memory::Object_at(caller_fp + kMarkerOffset);
  bool result = (marker == kConstructMarker);
  ASSERT_EQ(result, reference_result);
  return result;
}


// ----------------------------------------------------------------------------

BUILTIN(Illegal) {
  UNREACHABLE();
  return isolate->heap()->undefined_value();  // Make compiler happy.
}


BUILTIN(EmptyFunction) {
  return isolate->heap()->undefined_value();
}


static void MoveDoubleElements(FixedDoubleArray* dst,
                               int dst_index,
                               FixedDoubleArray* src,
                               int src_index,
                               int len) {
  if (len == 0) return;
  OS::MemMove(dst->data_start() + dst_index,
              src->data_start() + src_index,
              len * kDoubleSize);
}


static void FillWithHoles(Heap* heap, FixedArray* dst, int from, int to) {
  ASSERT(dst->map() != heap->fixed_cow_array_map());
  MemsetPointer(dst->data_start() + from, heap->the_hole_value(), to - from);
}


static void FillWithHoles(FixedDoubleArray* dst, int from, int to) {
  for (int i = from; i < to; i++) {
    dst->set_the_hole(i);
  }
}


static FixedArrayBase* LeftTrimFixedArray(Heap* heap,
                                          FixedArrayBase* elms,
                                          int to_trim) {
  Map* map = elms->map();
  int entry_size;
  if (elms->IsFixedArray()) {
    entry_size = kPointerSize;
  } else {
    entry_size = kDoubleSize;
  }
  ASSERT(elms->map() != heap->fixed_cow_array_map());
  // For now this trick is only applied to fixed arrays in new and paged space.
  // In large object space the object's start must coincide with chunk
  // and thus the trick is just not applicable.
  ASSERT(!heap->lo_space()->Contains(elms));

  STATIC_ASSERT(FixedArrayBase::kMapOffset == 0);
  STATIC_ASSERT(FixedArrayBase::kLengthOffset == kPointerSize);
  STATIC_ASSERT(FixedArrayBase::kHeaderSize == 2 * kPointerSize);

  Object** former_start = HeapObject::RawField(elms, 0);

  const int len = elms->length();

  if (to_trim * entry_size > FixedArrayBase::kHeaderSize &&
      elms->IsFixedArray() &&
      !heap->new_space()->Contains(elms)) {
    // If we are doing a big trim in old space then we zap the space that was
    // formerly part of the array so that the GC (aided by the card-based
    // remembered set) won't find pointers to new-space there.
    Object** zap = reinterpret_cast<Object**>(elms->address());
    zap++;  // Header of filler must be at least one word so skip that.
    for (int i = 1; i < to_trim; i++) {
      *zap++ = Smi::FromInt(0);
    }
  }
  // Technically in new space this write might be omitted (except for
  // debug mode which iterates through the heap), but to play safer
  // we still do it.
  heap->CreateFillerObjectAt(elms->address(), to_trim * entry_size);

  int new_start_index = to_trim * (entry_size / kPointerSize);
  former_start[new_start_index] = map;
  former_start[new_start_index + 1] = Smi::FromInt(len - to_trim);

  // Maintain marking consistency for HeapObjectIterator and
  // IncrementalMarking.
  int size_delta = to_trim * entry_size;
  if (heap->marking()->TransferMark(elms->address(),
                                    elms->address() + size_delta)) {
    MemoryChunk::IncrementLiveBytesFromMutator(elms->address(), -size_delta);
  }

  FixedArrayBase* new_elms = FixedArrayBase::cast(HeapObject::FromAddress(
      elms->address() + size_delta));
  HeapProfiler* profiler = heap->isolate()->heap_profiler();
  if (profiler->is_profiling()) {
    profiler->ObjectMoveEvent(elms->address(),
                              new_elms->address(),
                              new_elms->Size());
    if (profiler->is_tracking_allocations()) {
      // Report filler object as a new allocation.
      // Otherwise it will become an untracked object.
      profiler->NewObjectEvent(elms->address(), elms->Size());
    }
  }
  return new_elms;
}


static bool ArrayPrototypeHasNoElements(Heap* heap,
                                        Context* native_context,
                                        JSObject* array_proto) {
  // This method depends on non writability of Object and Array prototype
  // fields.
  if (array_proto->elements() != heap->empty_fixed_array()) return false;
  // Object.prototype
  Object* proto = array_proto->GetPrototype();
  if (proto == heap->null_value()) return false;
  array_proto = JSObject::cast(proto);
  if (array_proto != native_context->initial_object_prototype()) return false;
  if (array_proto->elements() != heap->empty_fixed_array()) return false;
  return array_proto->GetPrototype()->IsNull();
}


MUST_USE_RESULT
static inline MaybeObject* EnsureJSArrayWithWritableFastElements(
    Heap* heap, Object* receiver, Arguments* args, int first_added_arg) {
  if (!receiver->IsJSArray()) return NULL;
  JSArray* array = JSArray::cast(receiver);
  HeapObject* elms = array->elements();
  Map* map = elms->map();
  if (map == heap->fixed_array_map()) {
    if (args == NULL || array->HasFastObjectElements()) return elms;
  } else if (map == heap->fixed_cow_array_map()) {
    MaybeObject* maybe_writable_result = array->EnsureWritableFastElements();
    if (args == NULL || array->HasFastObjectElements() ||
        !maybe_writable_result->To(&elms)) {
      return maybe_writable_result;
    }
  } else if (map == heap->fixed_double_array_map()) {
    if (args == NULL) return elms;
  } else {
    return NULL;
  }

  // Need to ensure that the arguments passed in args can be contained in
  // the array.
  int args_length = args->length();
  if (first_added_arg >= args_length) return array->elements();

  ElementsKind origin_kind = array->map()->elements_kind();
  ASSERT(!IsFastObjectElementsKind(origin_kind));
  ElementsKind target_kind = origin_kind;
  int arg_count = args->length() - first_added_arg;
  Object** arguments = args->arguments() - first_added_arg - (arg_count - 1);
  for (int i = 0; i < arg_count; i++) {
    Object* arg = arguments[i];
    if (arg->IsHeapObject()) {
      if (arg->IsHeapNumber()) {
        target_kind = FAST_DOUBLE_ELEMENTS;
      } else {
        target_kind = FAST_ELEMENTS;
        break;
      }
    }
  }
  if (target_kind != origin_kind) {
    MaybeObject* maybe_failure = array->TransitionElementsKind(target_kind);
    if (maybe_failure->IsFailure()) return maybe_failure;
    return array->elements();
  }
  return elms;
}


static inline bool IsJSArrayFastElementMovingAllowed(Heap* heap,
                                                     JSArray* receiver) {
  if (!FLAG_clever_optimizations) return false;
  Context* native_context = heap->isolate()->context()->native_context();
  JSObject* array_proto =
      JSObject::cast(native_context->array_function()->prototype());
  return receiver->GetPrototype() == array_proto &&
         ArrayPrototypeHasNoElements(heap, native_context, array_proto);
}


MUST_USE_RESULT static MaybeObject* CallJsBuiltin(
    Isolate* isolate,
    const char* name,
    BuiltinArguments<NO_EXTRA_ARGUMENTS> args) {
  HandleScope handleScope(isolate);

  Handle<Object> js_builtin =
      GetProperty(Handle<JSObject>(isolate->native_context()->builtins()),
                  name);
  Handle<JSFunction> function = Handle<JSFunction>::cast(js_builtin);
  int argc = args.length() - 1;
  ScopedVector<Handle<Object> > argv(argc);
  for (int i = 0; i < argc; ++i) {
    argv[i] = args.at<Object>(i + 1);
  }
  bool pending_exception;
  Handle<Object> result = Execution::Call(isolate,
                                          function,
                                          args.receiver(),
                                          argc,
                                          argv.start(),
                                          &pending_exception);
  if (pending_exception) return Failure::Exception();
  return *result;
}


BUILTIN(ArrayPush) {
  Heap* heap = isolate->heap();
  Object* receiver = *args.receiver();
  FixedArrayBase* elms_obj;
  MaybeObject* maybe_elms_obj =
      EnsureJSArrayWithWritableFastElements(heap, receiver, &args, 1);
  if (maybe_elms_obj == NULL) {
    return CallJsBuiltin(isolate, "ArrayPush", args);
  }
  if (!maybe_elms_obj->To(&elms_obj)) return maybe_elms_obj;

  JSArray* array = JSArray::cast(receiver);
  ASSERT(!array->map()->is_observed());

  ElementsKind kind = array->GetElementsKind();

  if (IsFastSmiOrObjectElementsKind(kind)) {
    FixedArray* elms = FixedArray::cast(elms_obj);

    int len = Smi::cast(array->length())->value();
    int to_add = args.length() - 1;
    if (to_add == 0) {
      return Smi::FromInt(len);
    }
    // Currently fixed arrays cannot grow too big, so
    // we should never hit this case.
    ASSERT(to_add <= (Smi::kMaxValue - len));

    int new_length = len + to_add;

    if (new_length > elms->length()) {
      // New backing storage is needed.
      int capacity = new_length + (new_length >> 1) + 16;
      FixedArray* new_elms;
      MaybeObject* maybe_obj = heap->AllocateUninitializedFixedArray(capacity);
      if (!maybe_obj->To(&new_elms)) return maybe_obj;

      ElementsAccessor* accessor = array->GetElementsAccessor();
      MaybeObject* maybe_failure = accessor->CopyElements(
           NULL, 0, kind, new_elms, 0,
           ElementsAccessor::kCopyToEndAndInitializeToHole, elms_obj);
      ASSERT(!maybe_failure->IsFailure());
      USE(maybe_failure);

      elms = new_elms;
    }

    // Add the provided values.
    DisallowHeapAllocation no_gc;
    WriteBarrierMode mode = elms->GetWriteBarrierMode(no_gc);
    for (int index = 0; index < to_add; index++) {
      elms->set(index + len, args[index + 1], mode);
    }

    if (elms != array->elements()) {
      array->set_elements(elms);
    }

    // Set the length.
    array->set_length(Smi::FromInt(new_length));
    return Smi::FromInt(new_length);
  } else {
    int len = Smi::cast(array->length())->value();
    int elms_len = elms_obj->length();

    int to_add = args.length() - 1;
    if (to_add == 0) {
      return Smi::FromInt(len);
    }
    // Currently fixed arrays cannot grow too big, so
    // we should never hit this case.
    ASSERT(to_add <= (Smi::kMaxValue - len));

    int new_length = len + to_add;

    FixedDoubleArray* new_elms;

    if (new_length > elms_len) {
      // New backing storage is needed.
      int capacity = new_length + (new_length >> 1) + 16;
      MaybeObject* maybe_obj =
          heap->AllocateUninitializedFixedDoubleArray(capacity);
      if (!maybe_obj->To(&new_elms)) return maybe_obj;

      ElementsAccessor* accessor = array->GetElementsAccessor();
      MaybeObject* maybe_failure = accessor->CopyElements(
              NULL, 0, kind, new_elms, 0,
              ElementsAccessor::kCopyToEndAndInitializeToHole, elms_obj);
      ASSERT(!maybe_failure->IsFailure());
      USE(maybe_failure);
    } else {
      // to_add is > 0 and new_length <= elms_len, so elms_obj cannot be the
      // empty_fixed_array.
      new_elms = FixedDoubleArray::cast(elms_obj);
    }

    // Add the provided values.
    DisallowHeapAllocation no_gc;
    int index;
    for (index = 0; index < to_add; index++) {
      Object* arg = args[index + 1];
      new_elms->set(index + len, arg->Number());
    }

    if (new_elms != array->elements()) {
      array->set_elements(new_elms);
    }

    // Set the length.
    array->set_length(Smi::FromInt(new_length));
    return Smi::FromInt(new_length);
  }
}


BUILTIN(ArrayPop) {
  Heap* heap = isolate->heap();
  Object* receiver = *args.receiver();
  FixedArrayBase* elms_obj;
  MaybeObject* maybe_elms =
      EnsureJSArrayWithWritableFastElements(heap, receiver, NULL, 0);
  if (maybe_elms == NULL) return CallJsBuiltin(isolate, "ArrayPop", args);
  if (!maybe_elms->To(&elms_obj)) return maybe_elms;

  JSArray* array = JSArray::cast(receiver);
  ASSERT(!array->map()->is_observed());

  int len = Smi::cast(array->length())->value();
  if (len == 0) return heap->undefined_value();

  ElementsAccessor* accessor = array->GetElementsAccessor();
  int new_length = len - 1;
  MaybeObject* maybe_result;
  if (accessor->HasElement(array, array, new_length, elms_obj)) {
    maybe_result = accessor->Get(array, array, new_length, elms_obj);
  } else {
    maybe_result = array->GetPrototype()->GetElement(isolate, len - 1);
  }
  if (maybe_result->IsFailure()) return maybe_result;
  MaybeObject* maybe_failure =
      accessor->SetLength(array, Smi::FromInt(new_length));
  if (maybe_failure->IsFailure()) return maybe_failure;
  return maybe_result;
}


BUILTIN(ArrayShift) {
  Heap* heap = isolate->heap();
  Object* receiver = *args.receiver();
  FixedArrayBase* elms_obj;
  MaybeObject* maybe_elms_obj =
      EnsureJSArrayWithWritableFastElements(heap, receiver, NULL, 0);
  if (maybe_elms_obj == NULL)
      return CallJsBuiltin(isolate, "ArrayShift", args);
  if (!maybe_elms_obj->To(&elms_obj)) return maybe_elms_obj;

  if (!IsJSArrayFastElementMovingAllowed(heap, JSArray::cast(receiver))) {
    return CallJsBuiltin(isolate, "ArrayShift", args);
  }
  JSArray* array = JSArray::cast(receiver);
  ASSERT(!array->map()->is_observed());

  int len = Smi::cast(array->length())->value();
  if (len == 0) return heap->undefined_value();

  // Get first element
  ElementsAccessor* accessor = array->GetElementsAccessor();
  Object* first;
  MaybeObject* maybe_first = accessor->Get(receiver, array, 0, elms_obj);
  if (!maybe_first->To(&first)) return maybe_first;
  if (first->IsTheHole()) {
    first = heap->undefined_value();
  }

  if (!heap->lo_space()->Contains(elms_obj)) {
    array->set_elements(LeftTrimFixedArray(heap, elms_obj, 1));
  } else {
    // Shift the elements.
    if (elms_obj->IsFixedArray()) {
      FixedArray* elms = FixedArray::cast(elms_obj);
      DisallowHeapAllocation no_gc;
      heap->MoveElements(elms, 0, 1, len - 1);
      elms->set(len - 1, heap->the_hole_value());
    } else {
      FixedDoubleArray* elms = FixedDoubleArray::cast(elms_obj);
      MoveDoubleElements(elms, 0, elms, 1, len - 1);
      elms->set_the_hole(len - 1);
    }
  }

  // Set the length.
  array->set_length(Smi::FromInt(len - 1));

  return first;
}


BUILTIN(ArrayUnshift) {
  Heap* heap = isolate->heap();
  Object* receiver = *args.receiver();
  FixedArrayBase* elms_obj;
  MaybeObject* maybe_elms_obj =
      EnsureJSArrayWithWritableFastElements(heap, receiver, NULL, 0);
  if (maybe_elms_obj == NULL)
      return CallJsBuiltin(isolate, "ArrayUnshift", args);
  if (!maybe_elms_obj->To(&elms_obj)) return maybe_elms_obj;

  if (!IsJSArrayFastElementMovingAllowed(heap, JSArray::cast(receiver))) {
    return CallJsBuiltin(isolate, "ArrayUnshift", args);
  }
  JSArray* array = JSArray::cast(receiver);
  ASSERT(!array->map()->is_observed());
  if (!array->HasFastSmiOrObjectElements()) {
    return CallJsBuiltin(isolate, "ArrayUnshift", args);
  }
  FixedArray* elms = FixedArray::cast(elms_obj);

  int len = Smi::cast(array->length())->value();
  int to_add = args.length() - 1;
  int new_length = len + to_add;
  // Currently fixed arrays cannot grow too big, so
  // we should never hit this case.
  ASSERT(to_add <= (Smi::kMaxValue - len));

  MaybeObject* maybe_object =
      array->EnsureCanContainElements(&args, 1, to_add,
                                      DONT_ALLOW_DOUBLE_ELEMENTS);
  if (maybe_object->IsFailure()) return maybe_object;

  if (new_length > elms->length()) {
    // New backing storage is needed.
    int capacity = new_length + (new_length >> 1) + 16;
    FixedArray* new_elms;
    MaybeObject* maybe_elms = heap->AllocateUninitializedFixedArray(capacity);
    if (!maybe_elms->To(&new_elms)) return maybe_elms;

    ElementsKind kind = array->GetElementsKind();
    ElementsAccessor* accessor = array->GetElementsAccessor();
    MaybeObject* maybe_failure = accessor->CopyElements(
            NULL, 0, kind, new_elms, to_add,
            ElementsAccessor::kCopyToEndAndInitializeToHole, elms);
    ASSERT(!maybe_failure->IsFailure());
    USE(maybe_failure);

    elms = new_elms;
    array->set_elements(elms);
  } else {
    DisallowHeapAllocation no_gc;
    heap->MoveElements(elms, to_add, 0, len);
  }

  // Add the provided values.
  DisallowHeapAllocation no_gc;
  WriteBarrierMode mode = elms->GetWriteBarrierMode(no_gc);
  for (int i = 0; i < to_add; i++) {
    elms->set(i, args[i + 1], mode);
  }

  // Set the length.
  array->set_length(Smi::FromInt(new_length));
  return Smi::FromInt(new_length);
}


BUILTIN(ArraySlice) {
  Heap* heap = isolate->heap();
  Object* receiver = *args.receiver();
  FixedArrayBase* elms;
  int len = -1;
  if (receiver->IsJSArray()) {
    JSArray* array = JSArray::cast(receiver);
    if (!IsJSArrayFastElementMovingAllowed(heap, array)) {
      return CallJsBuiltin(isolate, "ArraySlice", args);
    }

    if (array->HasFastElements()) {
      elms = array->elements();
    } else {
      return CallJsBuiltin(isolate, "ArraySlice", args);
    }

    len = Smi::cast(array->length())->value();
  } else {
    // Array.slice(arguments, ...) is quite a common idiom (notably more
    // than 50% of invocations in Web apps).  Treat it in C++ as well.
    Map* arguments_map =
        isolate->context()->native_context()->arguments_boilerplate()->map();

    bool is_arguments_object_with_fast_elements =
        receiver->IsJSObject() &&
        JSObject::cast(receiver)->map() == arguments_map;
    if (!is_arguments_object_with_fast_elements) {
      return CallJsBuiltin(isolate, "ArraySlice", args);
    }
    JSObject* object = JSObject::cast(receiver);

    if (object->HasFastElements()) {
      elms = object->elements();
    } else {
      return CallJsBuiltin(isolate, "ArraySlice", args);
    }
    Object* len_obj = object->InObjectPropertyAt(Heap::kArgumentsLengthIndex);
    if (!len_obj->IsSmi()) {
      return CallJsBuiltin(isolate, "ArraySlice", args);
    }
    len = Smi::cast(len_obj)->value();
    if (len > elms->length()) {
      return CallJsBuiltin(isolate, "ArraySlice", args);
    }
  }

  JSObject* object = JSObject::cast(receiver);

  ASSERT(len >= 0);
  int n_arguments = args.length() - 1;

  // Note carefully choosen defaults---if argument is missing,
  // it's undefined which gets converted to 0 for relative_start
  // and to len for relative_end.
  int relative_start = 0;
  int relative_end = len;
  if (n_arguments > 0) {
    Object* arg1 = args[1];
    if (arg1->IsSmi()) {
      relative_start = Smi::cast(arg1)->value();
    } else if (arg1->IsHeapNumber()) {
      double start = HeapNumber::cast(arg1)->value();
      if (start < kMinInt || start > kMaxInt) {
        return CallJsBuiltin(isolate, "ArraySlice", args);
      }
      relative_start = std::isnan(start) ? 0 : static_cast<int>(start);
    } else if (!arg1->IsUndefined()) {
      return CallJsBuiltin(isolate, "ArraySlice", args);
    }
    if (n_arguments > 1) {
      Object* arg2 = args[2];
      if (arg2->IsSmi()) {
        relative_end = Smi::cast(arg2)->value();
      } else if (arg2->IsHeapNumber()) {
        double end = HeapNumber::cast(arg2)->value();
        if (end < kMinInt || end > kMaxInt) {
          return CallJsBuiltin(isolate, "ArraySlice", args);
        }
        relative_end = std::isnan(end) ? 0 : static_cast<int>(end);
      } else if (!arg2->IsUndefined()) {
        return CallJsBuiltin(isolate, "ArraySlice", args);
      }
    }
  }

  // ECMAScript 232, 3rd Edition, Section 15.4.4.10, step 6.
  int k = (relative_start < 0) ? Max(len + relative_start, 0)
                               : Min(relative_start, len);

  // ECMAScript 232, 3rd Edition, Section 15.4.4.10, step 8.
  int final = (relative_end < 0) ? Max(len + relative_end, 0)
                                 : Min(relative_end, len);

  // Calculate the length of result array.
  int result_len = Max(final - k, 0);

  ElementsKind kind = object->GetElementsKind();
  if (IsHoleyElementsKind(kind)) {
    bool packed = true;
    ElementsAccessor* accessor = ElementsAccessor::ForKind(kind);
    for (int i = k; i < final; i++) {
      if (!accessor->HasElement(object, object, i, elms)) {
        packed = false;
        break;
      }
    }
    if (packed) {
      kind = GetPackedElementsKind(kind);
    } else if (!receiver->IsJSArray()) {
      return CallJsBuiltin(isolate, "ArraySlice", args);
    }
  }

  JSArray* result_array;
  MaybeObject* maybe_array = heap->AllocateJSArrayAndStorage(kind,
                                                             result_len,
                                                             result_len);

  DisallowHeapAllocation no_gc;
  if (result_len == 0) return maybe_array;
  if (!maybe_array->To(&result_array)) return maybe_array;

  ElementsAccessor* accessor = object->GetElementsAccessor();
  MaybeObject* maybe_failure = accessor->CopyElements(
      NULL, k, kind, result_array->elements(), 0, result_len, elms);
  ASSERT(!maybe_failure->IsFailure());
  USE(maybe_failure);

  return result_array;
}


BUILTIN(ArraySplice) {
  Heap* heap = isolate->heap();
  Object* receiver = *args.receiver();
  FixedArrayBase* elms_obj;
  MaybeObject* maybe_elms =
      EnsureJSArrayWithWritableFastElements(heap, receiver, &args, 3);
  if (maybe_elms == NULL) {
    return CallJsBuiltin(isolate, "ArraySplice", args);
  }
  if (!maybe_elms->To(&elms_obj)) return maybe_elms;

  if (!IsJSArrayFastElementMovingAllowed(heap, JSArray::cast(receiver))) {
    return CallJsBuiltin(isolate, "ArraySplice", args);
  }
  JSArray* array = JSArray::cast(receiver);
  ASSERT(!array->map()->is_observed());

  int len = Smi::cast(array->length())->value();

  int n_arguments = args.length() - 1;

  int relative_start = 0;
  if (n_arguments > 0) {
    Object* arg1 = args[1];
    if (arg1->IsSmi()) {
      relative_start = Smi::cast(arg1)->value();
    } else if (arg1->IsHeapNumber()) {
      double start = HeapNumber::cast(arg1)->value();
      if (start < kMinInt || start > kMaxInt) {
        return CallJsBuiltin(isolate, "ArraySplice", args);
      }
      relative_start = std::isnan(start) ? 0 : static_cast<int>(start);
    } else if (!arg1->IsUndefined()) {
      return CallJsBuiltin(isolate, "ArraySplice", args);
    }
  }
  int actual_start = (relative_start < 0) ? Max(len + relative_start, 0)
                                          : Min(relative_start, len);

  // SpiderMonkey, TraceMonkey and JSC treat the case where no delete count is
  // given as a request to delete all the elements from the start.
  // And it differs from the case of undefined delete count.
  // This does not follow ECMA-262, but we do the same for
  // compatibility.
  int actual_delete_count;
  if (n_arguments == 1) {
    ASSERT(len - actual_start >= 0);
    actual_delete_count = len - actual_start;
  } else {
    int value = 0;  // ToInteger(undefined) == 0
    if (n_arguments > 1) {
      Object* arg2 = args[2];
      if (arg2->IsSmi()) {
        value = Smi::cast(arg2)->value();
      } else {
        return CallJsBuiltin(isolate, "ArraySplice", args);
      }
    }
    actual_delete_count = Min(Max(value, 0), len - actual_start);
  }

  ElementsKind elements_kind = array->GetElementsKind();

  int item_count = (n_arguments > 1) ? (n_arguments - 2) : 0;
  int new_length = len - actual_delete_count + item_count;

  // For double mode we do not support changing the length.
  if (new_length > len && IsFastDoubleElementsKind(elements_kind)) {
    return CallJsBuiltin(isolate, "ArraySplice", args);
  }

  if (new_length == 0) {
    MaybeObject* maybe_array = heap->AllocateJSArrayWithElements(
        elms_obj, elements_kind, actual_delete_count);
    if (maybe_array->IsFailure()) return maybe_array;
    array->set_elements(heap->empty_fixed_array());
    array->set_length(Smi::FromInt(0));
    return maybe_array;
  }

  JSArray* result_array = NULL;
  MaybeObject* maybe_array =
      heap->AllocateJSArrayAndStorage(elements_kind,
                                      actual_delete_count,
                                      actual_delete_count);
  if (!maybe_array->To(&result_array)) return maybe_array;

  if (actual_delete_count > 0) {
    DisallowHeapAllocation no_gc;
    ElementsAccessor* accessor = array->GetElementsAccessor();
    MaybeObject* maybe_failure = accessor->CopyElements(
        NULL, actual_start, elements_kind, result_array->elements(),
        0, actual_delete_count, elms_obj);
    // Cannot fail since the origin and target array are of the same elements
    // kind.
    ASSERT(!maybe_failure->IsFailure());
    USE(maybe_failure);
  }

  bool elms_changed = false;
  if (item_count < actual_delete_count) {
    // Shrink the array.
    const bool trim_array = !heap->lo_space()->Contains(elms_obj) &&
      ((actual_start + item_count) <
          (len - actual_delete_count - actual_start));
    if (trim_array) {
      const int delta = actual_delete_count - item_count;

      if (elms_obj->IsFixedDoubleArray()) {
        FixedDoubleArray* elms = FixedDoubleArray::cast(elms_obj);
        MoveDoubleElements(elms, delta, elms, 0, actual_start);
      } else {
        FixedArray* elms = FixedArray::cast(elms_obj);
        DisallowHeapAllocation no_gc;
        heap->MoveElements(elms, delta, 0, actual_start);
      }

      elms_obj = LeftTrimFixedArray(heap, elms_obj, delta);

      elms_changed = true;
    } else {
      if (elms_obj->IsFixedDoubleArray()) {
        FixedDoubleArray* elms = FixedDoubleArray::cast(elms_obj);
        MoveDoubleElements(elms, actual_start + item_count,
                           elms, actual_start + actual_delete_count,
                           (len - actual_delete_count - actual_start));
        FillWithHoles(elms, new_length, len);
      } else {
        FixedArray* elms = FixedArray::cast(elms_obj);
        DisallowHeapAllocation no_gc;
        heap->MoveElements(elms, actual_start + item_count,
                           actual_start + actual_delete_count,
                           (len - actual_delete_count - actual_start));
        FillWithHoles(heap, elms, new_length, len);
      }
    }
  } else if (item_count > actual_delete_count) {
    FixedArray* elms = FixedArray::cast(elms_obj);
    // Currently fixed arrays cannot grow too big, so
    // we should never hit this case.
    ASSERT((item_count - actual_delete_count) <= (Smi::kMaxValue - len));

    // Check if array need to grow.
    if (new_length > elms->length()) {
      // New backing storage is needed.
      int capacity = new_length + (new_length >> 1) + 16;
      FixedArray* new_elms;
      MaybeObject* maybe_obj = heap->AllocateUninitializedFixedArray(capacity);
      if (!maybe_obj->To(&new_elms)) return maybe_obj;

      DisallowHeapAllocation no_gc;

      ElementsKind kind = array->GetElementsKind();
      ElementsAccessor* accessor = array->GetElementsAccessor();
      if (actual_start > 0) {
        // Copy the part before actual_start as is.
        MaybeObject* maybe_failure = accessor->CopyElements(
            NULL, 0, kind, new_elms, 0, actual_start, elms);
        ASSERT(!maybe_failure->IsFailure());
        USE(maybe_failure);
      }
      MaybeObject* maybe_failure = accessor->CopyElements(
          NULL, actual_start + actual_delete_count, kind, new_elms,
          actual_start + item_count,
          ElementsAccessor::kCopyToEndAndInitializeToHole, elms);
      ASSERT(!maybe_failure->IsFailure());
      USE(maybe_failure);

      elms_obj = new_elms;
      elms_changed = true;
    } else {
      DisallowHeapAllocation no_gc;
      heap->MoveElements(elms, actual_start + item_count,
                         actual_start + actual_delete_count,
                         (len - actual_delete_count - actual_start));
    }
  }

  if (IsFastDoubleElementsKind(elements_kind)) {
    FixedDoubleArray* elms = FixedDoubleArray::cast(elms_obj);
    for (int k = actual_start; k < actual_start + item_count; k++) {
      Object* arg = args[3 + k - actual_start];
      if (arg->IsSmi()) {
        elms->set(k, Smi::cast(arg)->value());
      } else {
        elms->set(k, HeapNumber::cast(arg)->value());
      }
    }
  } else {
    FixedArray* elms = FixedArray::cast(elms_obj);
    DisallowHeapAllocation no_gc;
    WriteBarrierMode mode = elms->GetWriteBarrierMode(no_gc);
    for (int k = actual_start; k < actual_start + item_count; k++) {
      elms->set(k, args[3 + k - actual_start], mode);
    }
  }

  if (elms_changed) {
    array->set_elements(elms_obj);
  }
  // Set the length.
  array->set_length(Smi::FromInt(new_length));

  return result_array;
}


BUILTIN(ArrayConcat) {
  Heap* heap = isolate->heap();
  Context* native_context = isolate->context()->native_context();
  JSObject* array_proto =
      JSObject::cast(native_context->array_function()->prototype());
  if (!ArrayPrototypeHasNoElements(heap, native_context, array_proto)) {
    return CallJsBuiltin(isolate, "ArrayConcat", args);
  }

  // Iterate through all the arguments performing checks
  // and calculating total length.
  int n_arguments = args.length();
  int result_len = 0;
  ElementsKind elements_kind = GetInitialFastElementsKind();
  bool has_double = false;
  bool is_holey = false;
  for (int i = 0; i < n_arguments; i++) {
    Object* arg = args[i];
    if (!arg->IsJSArray() ||
        !JSArray::cast(arg)->HasFastElements() ||
        JSArray::cast(arg)->GetPrototype() != array_proto) {
      return CallJsBuiltin(isolate, "ArrayConcat", args);
    }
    int len = Smi::cast(JSArray::cast(arg)->length())->value();

    // We shouldn't overflow when adding another len.
    const int kHalfOfMaxInt = 1 << (kBitsPerInt - 2);
    STATIC_ASSERT(FixedArray::kMaxLength < kHalfOfMaxInt);
    USE(kHalfOfMaxInt);
    result_len += len;
    ASSERT(result_len >= 0);

    if (result_len > FixedDoubleArray::kMaxLength) {
      return CallJsBuiltin(isolate, "ArrayConcat", args);
    }

    ElementsKind arg_kind = JSArray::cast(arg)->map()->elements_kind();
    has_double = has_double || IsFastDoubleElementsKind(arg_kind);
    is_holey = is_holey || IsFastHoleyElementsKind(arg_kind);
    if (IsMoreGeneralElementsKindTransition(elements_kind, arg_kind)) {
      elements_kind = arg_kind;
    }
  }

  if (is_holey) elements_kind = GetHoleyElementsKind(elements_kind);

  // If a double array is concatted into a fast elements array, the fast
  // elements array needs to be initialized to contain proper holes, since
  // boxing doubles may cause incremental marking.
  ArrayStorageAllocationMode mode =
      has_double && IsFastObjectElementsKind(elements_kind)
      ? INITIALIZE_ARRAY_ELEMENTS_WITH_HOLE : DONT_INITIALIZE_ARRAY_ELEMENTS;
  JSArray* result_array;
  // Allocate result.
  MaybeObject* maybe_array =
      heap->AllocateJSArrayAndStorage(elements_kind,
                                      result_len,
                                      result_len,
                                      mode);
  if (!maybe_array->To(&result_array)) return maybe_array;
  if (result_len == 0) return result_array;

  int j = 0;
  FixedArrayBase* storage = result_array->elements();
  ElementsAccessor* accessor = ElementsAccessor::ForKind(elements_kind);
  for (int i = 0; i < n_arguments; i++) {
    JSArray* array = JSArray::cast(args[i]);
    int len = Smi::cast(array->length())->value();
    ElementsKind from_kind = array->GetElementsKind();
    if (len > 0) {
      MaybeObject* maybe_failure =
          accessor->CopyElements(array, 0, from_kind, storage, j, len);
      if (maybe_failure->IsFailure()) return maybe_failure;
      j += len;
    }
  }

  ASSERT(j == result_len);

  return result_array;
}


// -----------------------------------------------------------------------------
// Strict mode poison pills


BUILTIN(StrictModePoisonPill) {
  HandleScope scope(isolate);
  return isolate->Throw(*isolate->factory()->NewTypeError(
      "strict_poison_pill", HandleVector<Object>(NULL, 0)));
}


// -----------------------------------------------------------------------------
//


// Searches the hidden prototype chain of the given object for the first
// object that is an instance of the given type.  If no such object can
// be found then Heap::null_value() is returned.
static inline Object* FindHidden(Heap* heap,
                                 Object* object,
                                 FunctionTemplateInfo* type) {
  if (object->IsInstanceOf(type)) return object;
  Object* proto = object->GetPrototype(heap->isolate());
  if (proto->IsJSObject() &&
      JSObject::cast(proto)->map()->is_hidden_prototype()) {
    return FindHidden(heap, proto, type);
  }
  return heap->null_value();
}


// Returns the holder JSObject if the function can legally be called
// with this receiver.  Returns Heap::null_value() if the call is
// illegal.  Any arguments that don't fit the expected type is
// overwritten with undefined.  Note that holder and the arguments are
// implicitly rewritten with the first object in the hidden prototype
// chain that actually has the expected type.
static inline Object* TypeCheck(Heap* heap,
                                int argc,
                                Object** argv,
                                FunctionTemplateInfo* info) {
  Object* recv = argv[0];
  // API calls are only supported with JSObject receivers.
  if (!recv->IsJSObject()) return heap->null_value();
  Object* sig_obj = info->signature();
  if (sig_obj->IsUndefined()) return recv;
  SignatureInfo* sig = SignatureInfo::cast(sig_obj);
  // If necessary, check the receiver
  Object* recv_type = sig->receiver();
  Object* holder = recv;
  if (!recv_type->IsUndefined()) {
    holder = FindHidden(heap, holder, FunctionTemplateInfo::cast(recv_type));
    if (holder == heap->null_value()) return heap->null_value();
  }
  Object* args_obj = sig->args();
  // If there is no argument signature we're done
  if (args_obj->IsUndefined()) return holder;
  FixedArray* args = FixedArray::cast(args_obj);
  int length = args->length();
  if (argc <= length) length = argc - 1;
  for (int i = 0; i < length; i++) {
    Object* argtype = args->get(i);
    if (argtype->IsUndefined()) continue;
    Object** arg = &argv[-1 - i];
    Object* current = *arg;
    current = FindHidden(heap, current, FunctionTemplateInfo::cast(argtype));
    if (current == heap->null_value()) current = heap->undefined_value();
    *arg = current;
  }
  return holder;
}


template <bool is_construct>
MUST_USE_RESULT static MaybeObject* HandleApiCallHelper(
    BuiltinArguments<NEEDS_CALLED_FUNCTION> args, Isolate* isolate) {
  ASSERT(is_construct == CalledAsConstructor(isolate));
  Heap* heap = isolate->heap();

  HandleScope scope(isolate);
  Handle<JSFunction> function = args.called_function();
  ASSERT(function->shared()->IsApiFunction());

  FunctionTemplateInfo* fun_data = function->shared()->get_api_func_data();
  if (is_construct) {
    Handle<FunctionTemplateInfo> desc(fun_data, isolate);
    bool pending_exception = false;
    isolate->factory()->ConfigureInstance(
        desc, Handle<JSObject>::cast(args.receiver()), &pending_exception);
    ASSERT(isolate->has_pending_exception() == pending_exception);
    if (pending_exception) return Failure::Exception();
    fun_data = *desc;
  }

  Object* raw_holder = TypeCheck(heap, args.length(), &args[0], fun_data);

  if (raw_holder->IsNull()) {
    // This function cannot be called with the given receiver.  Abort!
    Handle<Object> obj =
        isolate->factory()->NewTypeError(
            "illegal_invocation", HandleVector(&function, 1));
    return isolate->Throw(*obj);
  }

  Object* raw_call_data = fun_data->call_code();
  if (!raw_call_data->IsUndefined()) {
    CallHandlerInfo* call_data = CallHandlerInfo::cast(raw_call_data);
    Object* callback_obj = call_data->callback();
    v8::FunctionCallback callback =
        v8::ToCData<v8::FunctionCallback>(callback_obj);
    Object* data_obj = call_data->data();
    Object* result;

    LOG(isolate, ApiObjectAccess("call", JSObject::cast(*args.receiver())));
    ASSERT(raw_holder->IsJSObject());

    FunctionCallbackArguments custom(isolate,
                                     data_obj,
                                     *function,
                                     raw_holder,
                                     &args[0] - 1,
                                     args.length() - 1,
                                     is_construct);

    v8::Handle<v8::Value> value = custom.Call(callback);
    if (value.IsEmpty()) {
      result = heap->undefined_value();
    } else {
      result = *reinterpret_cast<Object**>(*value);
      result->VerifyApiCallResultType();
    }

    RETURN_IF_SCHEDULED_EXCEPTION(isolate);
    if (!is_construct || result->IsJSObject()) return result;
  }

  return *args.receiver();
}


BUILTIN(HandleApiCall) {
  return HandleApiCallHelper<false>(args, isolate);
}


BUILTIN(HandleApiCallConstruct) {
  return HandleApiCallHelper<true>(args, isolate);
}


// Helper function to handle calls to non-function objects created through the
// API. The object can be called as either a constructor (using new) or just as
// a function (without new).
MUST_USE_RESULT static MaybeObject* HandleApiCallAsFunctionOrConstructor(
    Isolate* isolate,
    bool is_construct_call,
    BuiltinArguments<NO_EXTRA_ARGUMENTS> args) {
  // Non-functions are never called as constructors. Even if this is an object
  // called as a constructor the delegate call is not a construct call.
  ASSERT(!CalledAsConstructor(isolate));
  Heap* heap = isolate->heap();

  Handle<Object> receiver = args.receiver();

  // Get the object called.
  JSObject* obj = JSObject::cast(*receiver);

  // Get the invocation callback from the function descriptor that was
  // used to create the called object.
  ASSERT(obj->map()->has_instance_call_handler());
  JSFunction* constructor = JSFunction::cast(obj->map()->constructor());
  ASSERT(constructor->shared()->IsApiFunction());
  Object* handler =
      constructor->shared()->get_api_func_data()->instance_call_handler();
  ASSERT(!handler->IsUndefined());
  CallHandlerInfo* call_data = CallHandlerInfo::cast(handler);
  Object* callback_obj = call_data->callback();
  v8::FunctionCallback callback =
      v8::ToCData<v8::FunctionCallback>(callback_obj);

  // Get the data for the call and perform the callback.
  Object* result;
  {
    HandleScope scope(isolate);
    LOG(isolate, ApiObjectAccess("call non-function", obj));

    FunctionCallbackArguments custom(isolate,
                                     call_data->data(),
                                     constructor,
                                     obj,
                                     &args[0] - 1,
                                     args.length() - 1,
                                     is_construct_call);
    v8::Handle<v8::Value> value = custom.Call(callback);
    if (value.IsEmpty()) {
      result = heap->undefined_value();
    } else {
      result = *reinterpret_cast<Object**>(*value);
      result->VerifyApiCallResultType();
    }
  }
  // Check for exceptions and return result.
  RETURN_IF_SCHEDULED_EXCEPTION(isolate);
  return result;
}


// Handle calls to non-function objects created through the API. This delegate
// function is used when the call is a normal function call.
BUILTIN(HandleApiCallAsFunction) {
  return HandleApiCallAsFunctionOrConstructor(isolate, false, args);
}


// Handle calls to non-function objects created through the API. This delegate
// function is used when the call is a construct call.
BUILTIN(HandleApiCallAsConstructor) {
  return HandleApiCallAsFunctionOrConstructor(isolate, true, args);
}


static void Generate_LoadIC_Initialize(MacroAssembler* masm) {
  LoadIC::GenerateInitialize(masm);
}


static void Generate_LoadIC_PreMonomorphic(MacroAssembler* masm) {
  LoadIC::GeneratePreMonomorphic(masm);
}


static void Generate_LoadIC_Miss(MacroAssembler* masm) {
  LoadIC::GenerateMiss(masm);
}


static void Generate_LoadIC_Megamorphic(MacroAssembler* masm) {
  LoadIC::GenerateMegamorphic(masm);
}


static void Generate_LoadIC_Normal(MacroAssembler* masm) {
  LoadIC::GenerateNormal(masm);
}


static void Generate_LoadIC_Getter_ForDeopt(MacroAssembler* masm) {
  LoadStubCompiler::GenerateLoadViaGetter(
      masm, LoadStubCompiler::registers()[0], Handle<JSFunction>());
}


static void Generate_LoadIC_Slow(MacroAssembler* masm) {
  LoadIC::GenerateRuntimeGetProperty(masm);
}


static void Generate_KeyedLoadIC_Initialize(MacroAssembler* masm) {
  KeyedLoadIC::GenerateInitialize(masm);
}


static void Generate_KeyedLoadIC_Slow(MacroAssembler* masm) {
  KeyedLoadIC::GenerateRuntimeGetProperty(masm);
}


static void Generate_KeyedLoadIC_Miss(MacroAssembler* masm) {
  KeyedLoadIC::GenerateMiss(masm, MISS);
}


static void Generate_KeyedLoadIC_MissForceGeneric(MacroAssembler* masm) {
  KeyedLoadIC::GenerateMiss(masm, MISS_FORCE_GENERIC);
}


static void Generate_KeyedLoadIC_Generic(MacroAssembler* masm) {
  KeyedLoadIC::GenerateGeneric(masm);
}


static void Generate_KeyedLoadIC_String(MacroAssembler* masm) {
  KeyedLoadIC::GenerateString(masm);
}


static void Generate_KeyedLoadIC_PreMonomorphic(MacroAssembler* masm) {
  KeyedLoadIC::GeneratePreMonomorphic(masm);
}


static void Generate_KeyedLoadIC_IndexedInterceptor(MacroAssembler* masm) {
  KeyedLoadIC::GenerateIndexedInterceptor(masm);
}


static void Generate_KeyedLoadIC_NonStrictArguments(MacroAssembler* masm) {
  KeyedLoadIC::GenerateNonStrictArguments(masm);
}


static void Generate_StoreIC_Slow(MacroAssembler* masm) {
  StoreIC::GenerateSlow(masm);
}


static void Generate_StoreIC_Slow_Strict(MacroAssembler* masm) {
  StoreIC::GenerateSlow(masm);
}


static void Generate_StoreIC_Initialize(MacroAssembler* masm) {
  StoreIC::GenerateInitialize(masm);
}


static void Generate_StoreIC_Initialize_Strict(MacroAssembler* masm) {
  StoreIC::GenerateInitialize(masm);
}


static void Generate_StoreIC_PreMonomorphic(MacroAssembler* masm) {
  StoreIC::GeneratePreMonomorphic(masm);
}


static void Generate_StoreIC_PreMonomorphic_Strict(MacroAssembler* masm) {
  StoreIC::GeneratePreMonomorphic(masm);
}


static void Generate_StoreIC_Miss(MacroAssembler* masm) {
  StoreIC::GenerateMiss(masm);
}


static void Generate_StoreIC_Normal(MacroAssembler* masm) {
  StoreIC::GenerateNormal(masm);
}


static void Generate_StoreIC_Normal_Strict(MacroAssembler* masm) {
  StoreIC::GenerateNormal(masm);
}


static void Generate_StoreIC_Megamorphic(MacroAssembler* masm) {
  StoreIC::GenerateMegamorphic(masm, kNonStrictMode);
}


static void Generate_StoreIC_Megamorphic_Strict(MacroAssembler* masm) {
  StoreIC::GenerateMegamorphic(masm, kStrictMode);
}


static void Generate_StoreIC_GlobalProxy(MacroAssembler* masm) {
  StoreIC::GenerateRuntimeSetProperty(masm, kNonStrictMode);
}


static void Generate_StoreIC_GlobalProxy_Strict(MacroAssembler* masm) {
  StoreIC::GenerateRuntimeSetProperty(masm, kStrictMode);
}


static void Generate_StoreIC_Setter_ForDeopt(MacroAssembler* masm) {
  StoreStubCompiler::GenerateStoreViaSetter(masm, Handle<JSFunction>());
}


static void Generate_StoreIC_Generic(MacroAssembler* masm) {
  StoreIC::GenerateRuntimeSetProperty(masm, kNonStrictMode);
}


static void Generate_StoreIC_Generic_Strict(MacroAssembler* masm) {
  StoreIC::GenerateRuntimeSetProperty(masm, kStrictMode);
}


static void Generate_KeyedStoreIC_Generic(MacroAssembler* masm) {
  KeyedStoreIC::GenerateGeneric(masm, kNonStrictMode);
}


static void Generate_KeyedStoreIC_Generic_Strict(MacroAssembler* masm) {
  KeyedStoreIC::GenerateGeneric(masm, kStrictMode);
}


static void Generate_KeyedStoreIC_Miss(MacroAssembler* masm) {
  KeyedStoreIC::GenerateMiss(masm, MISS);
}


static void Generate_KeyedStoreIC_MissForceGeneric(MacroAssembler* masm) {
  KeyedStoreIC::GenerateMiss(masm, MISS_FORCE_GENERIC);
}


static void Generate_KeyedStoreIC_Slow(MacroAssembler* masm) {
  KeyedStoreIC::GenerateSlow(masm);
}


static void Generate_KeyedStoreIC_Slow_Strict(MacroAssembler* masm) {
  KeyedStoreIC::GenerateSlow(masm);
}


static void Generate_KeyedStoreIC_Initialize(MacroAssembler* masm) {
  KeyedStoreIC::GenerateInitialize(masm);
}


static void Generate_KeyedStoreIC_Initialize_Strict(MacroAssembler* masm) {
  KeyedStoreIC::GenerateInitialize(masm);
}


static void Generate_KeyedStoreIC_PreMonomorphic(MacroAssembler* masm) {
  KeyedStoreIC::GeneratePreMonomorphic(masm);
}


static void Generate_KeyedStoreIC_PreMonomorphic_Strict(MacroAssembler* masm) {
  KeyedStoreIC::GeneratePreMonomorphic(masm);
}


static void Generate_KeyedStoreIC_NonStrictArguments(MacroAssembler* masm) {
  KeyedStoreIC::GenerateNonStrictArguments(masm);
}


#ifdef ENABLE_DEBUGGER_SUPPORT
static void Generate_LoadIC_DebugBreak(MacroAssembler* masm) {
  Debug::GenerateLoadICDebugBreak(masm);
}


static void Generate_StoreIC_DebugBreak(MacroAssembler* masm) {
  Debug::GenerateStoreICDebugBreak(masm);
}


static void Generate_KeyedLoadIC_DebugBreak(MacroAssembler* masm) {
  Debug::GenerateKeyedLoadICDebugBreak(masm);
}


static void Generate_KeyedStoreIC_DebugBreak(MacroAssembler* masm) {
  Debug::GenerateKeyedStoreICDebugBreak(masm);
}


static void Generate_CompareNilIC_DebugBreak(MacroAssembler* masm) {
  Debug::GenerateCompareNilICDebugBreak(masm);
}


static void Generate_Return_DebugBreak(MacroAssembler* masm) {
  Debug::GenerateReturnDebugBreak(masm);
}


static void Generate_CallFunctionStub_DebugBreak(MacroAssembler* masm) {
  Debug::GenerateCallFunctionStubDebugBreak(masm);
}


static void Generate_CallFunctionStub_Recording_DebugBreak(
    MacroAssembler* masm) {
  Debug::GenerateCallFunctionStubRecordDebugBreak(masm);
}


static void Generate_CallConstructStub_DebugBreak(MacroAssembler* masm) {
  Debug::GenerateCallConstructStubDebugBreak(masm);
}


static void Generate_CallConstructStub_Recording_DebugBreak(
    MacroAssembler* masm) {
  Debug::GenerateCallConstructStubRecordDebugBreak(masm);
}


static void Generate_Slot_DebugBreak(MacroAssembler* masm) {
  Debug::GenerateSlotDebugBreak(masm);
}


static void Generate_PlainReturn_LiveEdit(MacroAssembler* masm) {
  Debug::GeneratePlainReturnLiveEdit(masm);
}


static void Generate_FrameDropper_LiveEdit(MacroAssembler* masm) {
  Debug::GenerateFrameDropperLiveEdit(masm);
}
#endif


Builtins::Builtins() : initialized_(false) {
  memset(builtins_, 0, sizeof(builtins_[0]) * builtin_count);
  memset(names_, 0, sizeof(names_[0]) * builtin_count);
}


Builtins::~Builtins() {
}


#define DEF_ENUM_C(name, ignore) FUNCTION_ADDR(Builtin_##name),
Address const Builtins::c_functions_[cfunction_count] = {
  BUILTIN_LIST_C(DEF_ENUM_C)
};
#undef DEF_ENUM_C

#define DEF_JS_NAME(name, ignore) #name,
#define DEF_JS_ARGC(ignore, argc) argc,
const char* const Builtins::javascript_names_[id_count] = {
  BUILTINS_LIST_JS(DEF_JS_NAME)
};

int const Builtins::javascript_argc_[id_count] = {
  BUILTINS_LIST_JS(DEF_JS_ARGC)
};
#undef DEF_JS_NAME
#undef DEF_JS_ARGC

struct BuiltinDesc {
  byte* generator;
  byte* c_code;
  const char* s_name;  // name is only used for generating log information.
  int name;
  Code::Flags flags;
  BuiltinExtraArguments extra_args;
};

#define BUILTIN_FUNCTION_TABLE_INIT { V8_ONCE_INIT, {} }

class BuiltinFunctionTable {
 public:
  BuiltinDesc* functions() {
    CallOnce(&once_, &Builtins::InitBuiltinFunctionTable);
    return functions_;
  }

  OnceType once_;
  BuiltinDesc functions_[Builtins::builtin_count + 1];

  friend class Builtins;
};

static BuiltinFunctionTable builtin_function_table =
    BUILTIN_FUNCTION_TABLE_INIT;

// Define array of pointers to generators and C builtin functions.
// We do this in a sort of roundabout way so that we can do the initialization
// within the lexical scope of Builtins:: and within a context where
// Code::Flags names a non-abstract type.
void Builtins::InitBuiltinFunctionTable() {
  BuiltinDesc* functions = builtin_function_table.functions_;
  functions[builtin_count].generator = NULL;
  functions[builtin_count].c_code = NULL;
  functions[builtin_count].s_name = NULL;
  functions[builtin_count].name = builtin_count;
  functions[builtin_count].flags = static_cast<Code::Flags>(0);
  functions[builtin_count].extra_args = NO_EXTRA_ARGUMENTS;

#define DEF_FUNCTION_PTR_C(aname, aextra_args)                         \
    functions->generator = FUNCTION_ADDR(Generate_Adaptor);            \
    functions->c_code = FUNCTION_ADDR(Builtin_##aname);                \
    functions->s_name = #aname;                                        \
    functions->name = c_##aname;                                       \
    functions->flags = Code::ComputeFlags(Code::BUILTIN);              \
    functions->extra_args = aextra_args;                               \
    ++functions;

#define DEF_FUNCTION_PTR_A(aname, kind, state, extra)                       \
    functions->generator = FUNCTION_ADDR(Generate_##aname);                 \
    functions->c_code = NULL;                                               \
    functions->s_name = #aname;                                             \
    functions->name = k##aname;                                             \
    functions->flags = Code::ComputeFlags(Code::kind,                       \
                                          state,                            \
                                          extra);                           \
    functions->extra_args = NO_EXTRA_ARGUMENTS;                             \
    ++functions;

#define DEF_FUNCTION_PTR_H(aname, kind, extra)                              \
    functions->generator = FUNCTION_ADDR(Generate_##aname);                 \
    functions->c_code = NULL;                                               \
    functions->s_name = #aname;                                             \
    functions->name = k##aname;                                             \
    functions->flags = Code::ComputeFlags(                                  \
        Code::HANDLER, MONOMORPHIC, extra, Code::NORMAL, Code::kind);       \
    functions->extra_args = NO_EXTRA_ARGUMENTS;                             \
    ++functions;

  BUILTIN_LIST_C(DEF_FUNCTION_PTR_C)
  BUILTIN_LIST_A(DEF_FUNCTION_PTR_A)
  BUILTIN_LIST_H(DEF_FUNCTION_PTR_H)
  BUILTIN_LIST_DEBUG_A(DEF_FUNCTION_PTR_A)

#undef DEF_FUNCTION_PTR_C
#undef DEF_FUNCTION_PTR_A
}


void Builtins::SetUp(Isolate* isolate, bool create_heap_objects) {
  ASSERT(!initialized_);
  Heap* heap = isolate->heap();

  // Create a scope for the handles in the builtins.
  HandleScope scope(isolate);

  const BuiltinDesc* functions = builtin_function_table.functions();

  // For now we generate builtin adaptor code into a stack-allocated
  // buffer, before copying it into individual code objects. Be careful
  // with alignment, some platforms don't like unaligned code.
  union { int force_alignment; byte buffer[8*KB]; } u;

  // Traverse the list of builtins and generate an adaptor in a
  // separate code object for each one.
  for (int i = 0; i < builtin_count; i++) {
    if (create_heap_objects) {
      MacroAssembler masm(isolate, u.buffer, sizeof u.buffer);
      // Generate the code/adaptor.
      typedef void (*Generator)(MacroAssembler*, int, BuiltinExtraArguments);
      Generator g = FUNCTION_CAST<Generator>(functions[i].generator);
      // We pass all arguments to the generator, but it may not use all of
      // them.  This works because the first arguments are on top of the
      // stack.
      ASSERT(!masm.has_frame());
      g(&masm, functions[i].name, functions[i].extra_args);
      // Move the code into the object heap.
      CodeDesc desc;
      masm.GetCode(&desc);
      Code::Flags flags =  functions[i].flags;
      Object* code = NULL;
      {
        // During startup it's OK to always allocate and defer GC to later.
        // This simplifies things because we don't need to retry.
        AlwaysAllocateScope __scope__;
        { MaybeObject* maybe_code =
              heap->CreateCode(desc, flags, masm.CodeObject());
          if (!maybe_code->ToObject(&code)) {
            v8::internal::V8::FatalProcessOutOfMemory("CreateCode");
          }
        }
      }
      // Log the event and add the code to the builtins array.
      PROFILE(isolate,
              CodeCreateEvent(Logger::BUILTIN_TAG,
                              Code::cast(code),
                              functions[i].s_name));
      GDBJIT(AddCode(GDBJITInterface::BUILTIN,
                     functions[i].s_name,
                     Code::cast(code)));
      builtins_[i] = code;
#ifdef ENABLE_DISASSEMBLER
      if (FLAG_print_builtin_code) {
        PrintF("Builtin: %s\n", functions[i].s_name);
        Code::cast(code)->Disassemble(functions[i].s_name);
        PrintF("\n");
      }
#endif
    } else {
      // Deserializing. The values will be filled in during IterateBuiltins.
      builtins_[i] = NULL;
    }
    names_[i] = functions[i].s_name;
  }

  // Mark as initialized.
  initialized_ = true;
}


void Builtins::TearDown() {
  initialized_ = false;
}


void Builtins::IterateBuiltins(ObjectVisitor* v) {
  v->VisitPointers(&builtins_[0], &builtins_[0] + builtin_count);
}


const char* Builtins::Lookup(byte* pc) {
  // may be called during initialization (disassembler!)
  if (initialized_) {
    for (int i = 0; i < builtin_count; i++) {
      Code* entry = Code::cast(builtins_[i]);
      if (entry->contains(pc)) {
        return names_[i];
      }
    }
  }
  return NULL;
}


void Builtins::Generate_InterruptCheck(MacroAssembler* masm) {
  masm->TailCallRuntime(Runtime::kInterrupt, 0, 1);
}


void Builtins::Generate_StackCheck(MacroAssembler* masm) {
  masm->TailCallRuntime(Runtime::kStackGuard, 0, 1);
}


#define DEFINE_BUILTIN_ACCESSOR_C(name, ignore)               \
Handle<Code> Builtins::name() {                               \
  Code** code_address =                                       \
      reinterpret_cast<Code**>(builtin_address(k##name));     \
  return Handle<Code>(code_address);                          \
}
#define DEFINE_BUILTIN_ACCESSOR_A(name, kind, state, extra) \
Handle<Code> Builtins::name() {                             \
  Code** code_address =                                     \
      reinterpret_cast<Code**>(builtin_address(k##name));   \
  return Handle<Code>(code_address);                        \
}
#define DEFINE_BUILTIN_ACCESSOR_H(name, kind, extra)        \
Handle<Code> Builtins::name() {                             \
  Code** code_address =                                     \
      reinterpret_cast<Code**>(builtin_address(k##name));   \
  return Handle<Code>(code_address);                        \
}
BUILTIN_LIST_C(DEFINE_BUILTIN_ACCESSOR_C)
BUILTIN_LIST_A(DEFINE_BUILTIN_ACCESSOR_A)
BUILTIN_LIST_H(DEFINE_BUILTIN_ACCESSOR_H)
BUILTIN_LIST_DEBUG_A(DEFINE_BUILTIN_ACCESSOR_A)
#undef DEFINE_BUILTIN_ACCESSOR_C
#undef DEFINE_BUILTIN_ACCESSOR_A


} }  // namespace v8::internal
