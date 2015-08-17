// Copyright 2011 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/v8.h"

#include "src/heap/objects-visiting.h"

namespace v8 {
namespace internal {


StaticVisitorBase::VisitorId StaticVisitorBase::GetVisitorId(
    int instance_type, int instance_size) {
  if (instance_type < FIRST_NONSTRING_TYPE) {
    switch (instance_type & kStringRepresentationMask) {
      case kSeqStringTag:
        if ((instance_type & kStringEncodingMask) == kOneByteStringTag) {
          return kVisitSeqOneByteString;
        } else {
          return kVisitSeqTwoByteString;
        }

      case kConsStringTag:
        if (IsShortcutCandidate(instance_type)) {
          return kVisitShortcutCandidate;
        } else {
          return kVisitConsString;
        }

      case kSlicedStringTag:
        return kVisitSlicedString;

      case kExternalStringTag:
        return GetVisitorIdForSize(kVisitDataObject, kVisitDataObjectGeneric,
                                   instance_size);
    }
    UNREACHABLE();
  }

  switch (instance_type) {
    case BYTE_ARRAY_TYPE:
      return kVisitByteArray;

    case FREE_SPACE_TYPE:
      return kVisitFreeSpace;

    case FIXED_ARRAY_TYPE:
      return kVisitFixedArray;

    case FIXED_DOUBLE_ARRAY_TYPE:
      return kVisitFixedDoubleArray;

    case CONSTANT_POOL_ARRAY_TYPE:
      return kVisitConstantPoolArray;

    case ODDBALL_TYPE:
      return kVisitOddball;

    case MAP_TYPE:
      return kVisitMap;

    case CODE_TYPE:
      return kVisitCode;

    case CELL_TYPE:
      return kVisitCell;

    case PROPERTY_CELL_TYPE:
      return kVisitPropertyCell;

    case WEAK_CELL_TYPE:
      return kVisitWeakCell;

    case JS_SET_TYPE:
      return GetVisitorIdForSize(kVisitStruct, kVisitStructGeneric,
                                 JSSet::kSize);

    case JS_MAP_TYPE:
      return GetVisitorIdForSize(kVisitStruct, kVisitStructGeneric,
                                 JSMap::kSize);

    case JS_WEAK_MAP_TYPE:
    case JS_WEAK_SET_TYPE:
      return kVisitJSWeakCollection;

    case JS_REGEXP_TYPE:
      return kVisitJSRegExp;

    case SHARED_FUNCTION_INFO_TYPE:
      return kVisitSharedFunctionInfo;

    case JS_PROXY_TYPE:
      return GetVisitorIdForSize(kVisitStruct, kVisitStructGeneric,
                                 JSProxy::kSize);

    case JS_FUNCTION_PROXY_TYPE:
      return GetVisitorIdForSize(kVisitStruct, kVisitStructGeneric,
                                 JSFunctionProxy::kSize);

    case FOREIGN_TYPE:
      return GetVisitorIdForSize(kVisitDataObject, kVisitDataObjectGeneric,
                                 Foreign::kSize);

    case SYMBOL_TYPE:
      return kVisitSymbol;

    case FILLER_TYPE:
      return kVisitDataObjectGeneric;

    case JS_ARRAY_BUFFER_TYPE:
      return kVisitJSArrayBuffer;

    case JS_TYPED_ARRAY_TYPE:
      return kVisitJSTypedArray;

    case JS_DATA_VIEW_TYPE:
      return kVisitJSDataView;

    case JS_OBJECT_TYPE:
    case JS_CONTEXT_EXTENSION_OBJECT_TYPE:
    case JS_GENERATOR_OBJECT_TYPE:
    case JS_MODULE_TYPE:
    case JS_VALUE_TYPE:
    case JS_DATE_TYPE:
    case JS_ARRAY_TYPE:
    case JS_GLOBAL_PROXY_TYPE:
    case JS_GLOBAL_OBJECT_TYPE:
    case JS_BUILTINS_OBJECT_TYPE:
    case JS_MESSAGE_OBJECT_TYPE:
    case JS_SET_ITERATOR_TYPE:
    case JS_MAP_ITERATOR_TYPE:
      return GetVisitorIdForSize(kVisitJSObject, kVisitJSObjectGeneric,
                                 instance_size);

    case JS_FUNCTION_TYPE:
      return kVisitJSFunction;

    case HEAP_NUMBER_TYPE:
    case MUTABLE_HEAP_NUMBER_TYPE:
#define EXTERNAL_ARRAY_CASE(Type, type, TYPE, ctype, size) \
  case EXTERNAL_##TYPE##_ARRAY_TYPE:

      TYPED_ARRAYS(EXTERNAL_ARRAY_CASE)
      return GetVisitorIdForSize(kVisitDataObject, kVisitDataObjectGeneric,
                                 instance_size);
#undef EXTERNAL_ARRAY_CASE

    case FIXED_UINT8_ARRAY_TYPE:
    case FIXED_INT8_ARRAY_TYPE:
    case FIXED_UINT16_ARRAY_TYPE:
    case FIXED_INT16_ARRAY_TYPE:
    case FIXED_UINT32_ARRAY_TYPE:
    case FIXED_INT32_ARRAY_TYPE:
    case FIXED_FLOAT32_ARRAY_TYPE:
    case FIXED_UINT8_CLAMPED_ARRAY_TYPE:
      return kVisitFixedTypedArray;

    case FIXED_FLOAT64_ARRAY_TYPE:
      return kVisitFixedFloat64Array;

#define MAKE_STRUCT_CASE(NAME, Name, name) case NAME##_TYPE:
      STRUCT_LIST(MAKE_STRUCT_CASE)
#undef MAKE_STRUCT_CASE
      if (instance_type == ALLOCATION_SITE_TYPE) {
        return kVisitAllocationSite;
      }

      return GetVisitorIdForSize(kVisitStruct, kVisitStructGeneric,
                                 instance_size);

    default:
      UNREACHABLE();
      return kVisitorIdCount;
  }
}


// We don't record weak slots during marking or scavenges. Instead we do it
// once when we complete mark-compact cycle.  Note that write barrier has no
// effect if we are already in the middle of compacting mark-sweep cycle and we
// have to record slots manually.
static bool MustRecordSlots(Heap* heap) {
  return heap->gc_state() == Heap::MARK_COMPACT &&
         heap->mark_compact_collector()->is_compacting();
}


template <class T>
struct WeakListVisitor;


template <class T>
Object* VisitWeakList(Heap* heap, Object* list, WeakObjectRetainer* retainer) {
  Object* undefined = heap->undefined_value();
  Object* head = undefined;
  T* tail = NULL;
  MarkCompactCollector* collector = heap->mark_compact_collector();
  bool record_slots = MustRecordSlots(heap);
  while (list != undefined) {
    // Check whether to keep the candidate in the list.
    T* candidate = reinterpret_cast<T*>(list);
    Object* retained = retainer->RetainAs(list);
    if (retained != NULL) {
      if (head == undefined) {
        // First element in the list.
        head = retained;
      } else {
        // Subsequent elements in the list.
        DCHECK(tail != NULL);
        WeakListVisitor<T>::SetWeakNext(tail, retained);
        if (record_slots) {
          Object** next_slot =
              HeapObject::RawField(tail, WeakListVisitor<T>::WeakNextOffset());
          collector->RecordSlot(next_slot, next_slot, retained);
        }
      }
      // Retained object is new tail.
      DCHECK(!retained->IsUndefined());
      candidate = reinterpret_cast<T*>(retained);
      tail = candidate;


      // tail is a live object, visit it.
      WeakListVisitor<T>::VisitLiveObject(heap, tail, retainer);
    } else {
      WeakListVisitor<T>::VisitPhantomObject(heap, candidate);
    }

    // Move to next element in the list.
    list = WeakListVisitor<T>::WeakNext(candidate);
  }

  // Terminate the list if there is one or more elements.
  if (tail != NULL) {
    WeakListVisitor<T>::SetWeakNext(tail, undefined);
  }
  return head;
}


template <class T>
static void ClearWeakList(Heap* heap, Object* list) {
  Object* undefined = heap->undefined_value();
  while (list != undefined) {
    T* candidate = reinterpret_cast<T*>(list);
    list = WeakListVisitor<T>::WeakNext(candidate);
    WeakListVisitor<T>::SetWeakNext(candidate, undefined);
  }
}


template <>
struct WeakListVisitor<JSFunction> {
  static void SetWeakNext(JSFunction* function, Object* next) {
    function->set_next_function_link(next);
  }

  static Object* WeakNext(JSFunction* function) {
    return function->next_function_link();
  }

  static int WeakNextOffset() { return JSFunction::kNextFunctionLinkOffset; }

  static void VisitLiveObject(Heap*, JSFunction*, WeakObjectRetainer*) {}

  static void VisitPhantomObject(Heap*, JSFunction*) {}
};


template <>
struct WeakListVisitor<Code> {
  static void SetWeakNext(Code* code, Object* next) {
    code->set_next_code_link(next);
  }

  static Object* WeakNext(Code* code) { return code->next_code_link(); }

  static int WeakNextOffset() { return Code::kNextCodeLinkOffset; }

  static void VisitLiveObject(Heap*, Code*, WeakObjectRetainer*) {}

  static void VisitPhantomObject(Heap*, Code*) {}
};


template <>
struct WeakListVisitor<Context> {
  static void SetWeakNext(Context* context, Object* next) {
    context->set(Context::NEXT_CONTEXT_LINK, next, UPDATE_WRITE_BARRIER);
  }

  static Object* WeakNext(Context* context) {
    return context->get(Context::NEXT_CONTEXT_LINK);
  }

  static int WeakNextOffset() {
    return FixedArray::SizeFor(Context::NEXT_CONTEXT_LINK);
  }

  static void VisitLiveObject(Heap* heap, Context* context,
                              WeakObjectRetainer* retainer) {
    // Process the three weak lists linked off the context.
    DoWeakList<JSFunction>(heap, context, retainer,
                           Context::OPTIMIZED_FUNCTIONS_LIST);
    DoWeakList<Code>(heap, context, retainer, Context::OPTIMIZED_CODE_LIST);
    DoWeakList<Code>(heap, context, retainer, Context::DEOPTIMIZED_CODE_LIST);
  }

  template <class T>
  static void DoWeakList(Heap* heap, Context* context,
                         WeakObjectRetainer* retainer, int index) {
    // Visit the weak list, removing dead intermediate elements.
    Object* list_head = VisitWeakList<T>(heap, context->get(index), retainer);

    // Update the list head.
    context->set(index, list_head, UPDATE_WRITE_BARRIER);

    if (MustRecordSlots(heap)) {
      // Record the updated slot if necessary.
      Object** head_slot =
          HeapObject::RawField(context, FixedArray::SizeFor(index));
      heap->mark_compact_collector()->RecordSlot(head_slot, head_slot,
                                                 list_head);
    }
  }

  static void VisitPhantomObject(Heap* heap, Context* context) {
    ClearWeakList<JSFunction>(heap,
                              context->get(Context::OPTIMIZED_FUNCTIONS_LIST));
    ClearWeakList<Code>(heap, context->get(Context::OPTIMIZED_CODE_LIST));
    ClearWeakList<Code>(heap, context->get(Context::DEOPTIMIZED_CODE_LIST));
  }
};


template <>
struct WeakListVisitor<JSArrayBufferView> {
  static void SetWeakNext(JSArrayBufferView* obj, Object* next) {
    obj->set_weak_next(next);
  }

  static Object* WeakNext(JSArrayBufferView* obj) { return obj->weak_next(); }

  static int WeakNextOffset() { return JSArrayBufferView::kWeakNextOffset; }

  static void VisitLiveObject(Heap*, JSArrayBufferView*, WeakObjectRetainer*) {}

  static void VisitPhantomObject(Heap*, JSArrayBufferView*) {}
};


template <>
struct WeakListVisitor<JSArrayBuffer> {
  static void SetWeakNext(JSArrayBuffer* obj, Object* next) {
    obj->set_weak_next(next);
  }

  static Object* WeakNext(JSArrayBuffer* obj) { return obj->weak_next(); }

  static int WeakNextOffset() { return JSArrayBuffer::kWeakNextOffset; }

  static void VisitLiveObject(Heap* heap, JSArrayBuffer* array_buffer,
                              WeakObjectRetainer* retainer) {
    Object* typed_array_obj = VisitWeakList<JSArrayBufferView>(
        heap, array_buffer->weak_first_view(), retainer);
    array_buffer->set_weak_first_view(typed_array_obj);
    if (typed_array_obj != heap->undefined_value() && MustRecordSlots(heap)) {
      Object** slot = HeapObject::RawField(array_buffer,
                                           JSArrayBuffer::kWeakFirstViewOffset);
      heap->mark_compact_collector()->RecordSlot(slot, slot, typed_array_obj);
    }
  }

  static void VisitPhantomObject(Heap* heap, JSArrayBuffer* phantom) {
    Runtime::FreeArrayBuffer(heap->isolate(), phantom);
  }
};


template <>
struct WeakListVisitor<AllocationSite> {
  static void SetWeakNext(AllocationSite* obj, Object* next) {
    obj->set_weak_next(next);
  }

  static Object* WeakNext(AllocationSite* obj) { return obj->weak_next(); }

  static int WeakNextOffset() { return AllocationSite::kWeakNextOffset; }

  static void VisitLiveObject(Heap*, AllocationSite*, WeakObjectRetainer*) {}

  static void VisitPhantomObject(Heap*, AllocationSite*) {}
};


template Object* VisitWeakList<Code>(Heap* heap, Object* list,
                                     WeakObjectRetainer* retainer);


template Object* VisitWeakList<JSFunction>(Heap* heap, Object* list,
                                           WeakObjectRetainer* retainer);


template Object* VisitWeakList<Context>(Heap* heap, Object* list,
                                        WeakObjectRetainer* retainer);


template Object* VisitWeakList<JSArrayBuffer>(Heap* heap, Object* list,
                                              WeakObjectRetainer* retainer);


template Object* VisitWeakList<AllocationSite>(Heap* heap, Object* list,
                                               WeakObjectRetainer* retainer);
}
}  // namespace v8::internal
