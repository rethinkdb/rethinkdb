// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_TRANSITIONS_INL_H_
#define V8_TRANSITIONS_INL_H_

#include "src/transitions.h"

namespace v8 {
namespace internal {


#define FIELD_ADDR(p, offset) \
  (reinterpret_cast<byte*>(p) + offset - kHeapObjectTag)

#define WRITE_FIELD(p, offset, value) \
  (*reinterpret_cast<Object**>(FIELD_ADDR(p, offset)) = value)

#define CONDITIONAL_WRITE_BARRIER(heap, object, offset, value, mode)    \
  if (mode == UPDATE_WRITE_BARRIER) {                                   \
    heap->incremental_marking()->RecordWrite(                           \
      object, HeapObject::RawField(object, offset), value);             \
    if (heap->InNewSpace(value)) {                                      \
      heap->RecordWrite(object->address(), offset);                     \
    }                                                                   \
  }


TransitionArray* TransitionArray::cast(Object* object) {
  DCHECK(object->IsTransitionArray());
  return reinterpret_cast<TransitionArray*>(object);
}


bool TransitionArray::HasElementsTransition() {
  return Search(GetHeap()->elements_transition_symbol()) != kNotFound;
}


Object* TransitionArray::back_pointer_storage() {
  return get(kBackPointerStorageIndex);
}


void TransitionArray::set_back_pointer_storage(Object* back_pointer,
                                               WriteBarrierMode mode) {
  Heap* heap = GetHeap();
  WRITE_FIELD(this, kBackPointerStorageOffset, back_pointer);
  CONDITIONAL_WRITE_BARRIER(
      heap, this, kBackPointerStorageOffset, back_pointer, mode);
}


bool TransitionArray::HasPrototypeTransitions() {
  return IsFullTransitionArray() &&
      get(kPrototypeTransitionsIndex) != Smi::FromInt(0);
}


FixedArray* TransitionArray::GetPrototypeTransitions() {
  DCHECK(IsFullTransitionArray());
  Object* prototype_transitions = get(kPrototypeTransitionsIndex);
  return FixedArray::cast(prototype_transitions);
}


void TransitionArray::SetPrototypeTransitions(FixedArray* transitions,
                                              WriteBarrierMode mode) {
  DCHECK(IsFullTransitionArray());
  DCHECK(transitions->IsFixedArray());
  Heap* heap = GetHeap();
  WRITE_FIELD(this, kPrototypeTransitionsOffset, transitions);
  CONDITIONAL_WRITE_BARRIER(
      heap, this, kPrototypeTransitionsOffset, transitions, mode);
}


Object** TransitionArray::GetPrototypeTransitionsSlot() {
  return HeapObject::RawField(reinterpret_cast<HeapObject*>(this),
                              kPrototypeTransitionsOffset);
}


Object** TransitionArray::GetKeySlot(int transition_number) {
  DCHECK(!IsSimpleTransition());
  DCHECK(transition_number < number_of_transitions());
  return RawFieldOfElementAt(ToKeyIndex(transition_number));
}


Name* TransitionArray::GetKey(int transition_number) {
  if (IsSimpleTransition()) {
    Map* target = GetTarget(kSimpleTransitionIndex);
    int descriptor = target->LastAdded();
    Name* key = target->instance_descriptors()->GetKey(descriptor);
    return key;
  }
  DCHECK(transition_number < number_of_transitions());
  return Name::cast(get(ToKeyIndex(transition_number)));
}


void TransitionArray::SetKey(int transition_number, Name* key) {
  DCHECK(!IsSimpleTransition());
  DCHECK(transition_number < number_of_transitions());
  set(ToKeyIndex(transition_number), key);
}


Map* TransitionArray::GetTarget(int transition_number) {
  if (IsSimpleTransition()) {
    DCHECK(transition_number == kSimpleTransitionIndex);
    return Map::cast(get(kSimpleTransitionTarget));
  }
  DCHECK(transition_number < number_of_transitions());
  return Map::cast(get(ToTargetIndex(transition_number)));
}


void TransitionArray::SetTarget(int transition_number, Map* value) {
  if (IsSimpleTransition()) {
    DCHECK(transition_number == kSimpleTransitionIndex);
    return set(kSimpleTransitionTarget, value);
  }
  DCHECK(transition_number < number_of_transitions());
  set(ToTargetIndex(transition_number), value);
}


PropertyDetails TransitionArray::GetTargetDetails(int transition_number) {
  Map* map = GetTarget(transition_number);
  return map->GetLastDescriptorDetails();
}


Object* TransitionArray::GetTargetValue(int transition_number) {
  Map* map = GetTarget(transition_number);
  return map->instance_descriptors()->GetValue(map->LastAdded());
}


int TransitionArray::Search(Name* name, int* out_insertion_index) {
  if (IsSimpleTransition()) {
    Name* key = GetKey(kSimpleTransitionIndex);
    if (key->Equals(name)) return kSimpleTransitionIndex;
    if (out_insertion_index != NULL) {
      *out_insertion_index = key->Hash() > name->Hash() ? 0 : 1;
    }
    return kNotFound;
  }
  return internal::Search<ALL_ENTRIES>(this, name, 0, out_insertion_index);
}


void TransitionArray::NoIncrementalWriteBarrierSet(int transition_number,
                                                   Name* key,
                                                   Map* target) {
  FixedArray::NoIncrementalWriteBarrierSet(
      this, ToKeyIndex(transition_number), key);
  FixedArray::NoIncrementalWriteBarrierSet(
      this, ToTargetIndex(transition_number), target);
}


void TransitionArray::SetNumberOfTransitions(int number_of_transitions) {
  if (IsFullTransitionArray()) {
    DCHECK(number_of_transitions <= number_of_transitions_storage());
    WRITE_FIELD(this, kTransitionLengthOffset,
                Smi::FromInt(number_of_transitions));
  }
}


#undef FIELD_ADDR
#undef WRITE_FIELD
#undef CONDITIONAL_WRITE_BARRIER


} }  // namespace v8::internal

#endif  // V8_TRANSITIONS_INL_H_
