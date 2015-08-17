// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_HEAP_HEAP_INL_H_
#define V8_HEAP_HEAP_INL_H_

#include <cmath>

#include "src/base/platform/platform.h"
#include "src/cpu-profiler.h"
#include "src/heap/heap.h"
#include "src/heap/store-buffer.h"
#include "src/heap/store-buffer-inl.h"
#include "src/heap-profiler.h"
#include "src/isolate.h"
#include "src/list-inl.h"
#include "src/msan.h"
#include "src/objects.h"

namespace v8 {
namespace internal {

void PromotionQueue::insert(HeapObject* target, int size) {
  if (emergency_stack_ != NULL) {
    emergency_stack_->Add(Entry(target, size));
    return;
  }

  if (NewSpacePage::IsAtStart(reinterpret_cast<Address>(rear_))) {
    NewSpacePage* rear_page =
        NewSpacePage::FromAddress(reinterpret_cast<Address>(rear_));
    DCHECK(!rear_page->prev_page()->is_anchor());
    rear_ = reinterpret_cast<intptr_t*>(rear_page->prev_page()->area_end());
  }

  if ((rear_ - 2) < limit_) {
    RelocateQueueHead();
    emergency_stack_->Add(Entry(target, size));
    return;
  }

  *(--rear_) = reinterpret_cast<intptr_t>(target);
  *(--rear_) = size;
// Assert no overflow into live objects.
#ifdef DEBUG
  SemiSpace::AssertValidRange(target->GetIsolate()->heap()->new_space()->top(),
                              reinterpret_cast<Address>(rear_));
#endif
}


template <>
bool inline Heap::IsOneByte(Vector<const char> str, int chars) {
  // TODO(dcarney): incorporate Latin-1 check when Latin-1 is supported?
  return chars == str.length();
}


template <>
bool inline Heap::IsOneByte(String* str, int chars) {
  return str->IsOneByteRepresentation();
}


AllocationResult Heap::AllocateInternalizedStringFromUtf8(
    Vector<const char> str, int chars, uint32_t hash_field) {
  if (IsOneByte(str, chars)) {
    return AllocateOneByteInternalizedString(Vector<const uint8_t>::cast(str),
                                             hash_field);
  }
  return AllocateInternalizedStringImpl<false>(str, chars, hash_field);
}


template <typename T>
AllocationResult Heap::AllocateInternalizedStringImpl(T t, int chars,
                                                      uint32_t hash_field) {
  if (IsOneByte(t, chars)) {
    return AllocateInternalizedStringImpl<true>(t, chars, hash_field);
  }
  return AllocateInternalizedStringImpl<false>(t, chars, hash_field);
}


AllocationResult Heap::AllocateOneByteInternalizedString(
    Vector<const uint8_t> str, uint32_t hash_field) {
  CHECK_GE(String::kMaxLength, str.length());
  // Compute map and object size.
  Map* map = one_byte_internalized_string_map();
  int size = SeqOneByteString::SizeFor(str.length());
  AllocationSpace space = SelectSpace(size, OLD_DATA_SPACE, TENURED);

  // Allocate string.
  HeapObject* result;
  {
    AllocationResult allocation = AllocateRaw(size, space, OLD_DATA_SPACE);
    if (!allocation.To(&result)) return allocation;
  }

  // String maps are all immortal immovable objects.
  result->set_map_no_write_barrier(map);
  // Set length and hash fields of the allocated string.
  String* answer = String::cast(result);
  answer->set_length(str.length());
  answer->set_hash_field(hash_field);

  DCHECK_EQ(size, answer->Size());

  // Fill in the characters.
  MemCopy(answer->address() + SeqOneByteString::kHeaderSize, str.start(),
          str.length());

  return answer;
}


AllocationResult Heap::AllocateTwoByteInternalizedString(Vector<const uc16> str,
                                                         uint32_t hash_field) {
  CHECK_GE(String::kMaxLength, str.length());
  // Compute map and object size.
  Map* map = internalized_string_map();
  int size = SeqTwoByteString::SizeFor(str.length());
  AllocationSpace space = SelectSpace(size, OLD_DATA_SPACE, TENURED);

  // Allocate string.
  HeapObject* result;
  {
    AllocationResult allocation = AllocateRaw(size, space, OLD_DATA_SPACE);
    if (!allocation.To(&result)) return allocation;
  }

  result->set_map(map);
  // Set length and hash fields of the allocated string.
  String* answer = String::cast(result);
  answer->set_length(str.length());
  answer->set_hash_field(hash_field);

  DCHECK_EQ(size, answer->Size());

  // Fill in the characters.
  MemCopy(answer->address() + SeqTwoByteString::kHeaderSize, str.start(),
          str.length() * kUC16Size);

  return answer;
}

AllocationResult Heap::CopyFixedArray(FixedArray* src) {
  if (src->length() == 0) return src;
  return CopyFixedArrayWithMap(src, src->map());
}


AllocationResult Heap::CopyFixedDoubleArray(FixedDoubleArray* src) {
  if (src->length() == 0) return src;
  return CopyFixedDoubleArrayWithMap(src, src->map());
}


AllocationResult Heap::CopyConstantPoolArray(ConstantPoolArray* src) {
  if (src->length() == 0) return src;
  return CopyConstantPoolArrayWithMap(src, src->map());
}


AllocationResult Heap::AllocateRaw(int size_in_bytes, AllocationSpace space,
                                   AllocationSpace retry_space) {
  DCHECK(AllowHandleAllocation::IsAllowed());
  DCHECK(AllowHeapAllocation::IsAllowed());
  DCHECK(gc_state_ == NOT_IN_GC);
#ifdef DEBUG
  if (FLAG_gc_interval >= 0 && AllowAllocationFailure::IsAllowed(isolate_) &&
      Heap::allocation_timeout_-- <= 0) {
    return AllocationResult::Retry(space);
  }
  isolate_->counters()->objs_since_last_full()->Increment();
  isolate_->counters()->objs_since_last_young()->Increment();
#endif

  HeapObject* object;
  AllocationResult allocation;
  if (NEW_SPACE == space) {
    allocation = new_space_.AllocateRaw(size_in_bytes);
    if (always_allocate() && allocation.IsRetry() && retry_space != NEW_SPACE) {
      space = retry_space;
    } else {
      if (allocation.To(&object)) {
        OnAllocationEvent(object, size_in_bytes);
      }
      return allocation;
    }
  }

  if (OLD_POINTER_SPACE == space) {
    allocation = old_pointer_space_->AllocateRaw(size_in_bytes);
  } else if (OLD_DATA_SPACE == space) {
    allocation = old_data_space_->AllocateRaw(size_in_bytes);
  } else if (CODE_SPACE == space) {
    if (size_in_bytes <= code_space()->AreaSize()) {
      allocation = code_space_->AllocateRaw(size_in_bytes);
    } else {
      // Large code objects are allocated in large object space.
      allocation = lo_space_->AllocateRaw(size_in_bytes, EXECUTABLE);
    }
  } else if (LO_SPACE == space) {
    allocation = lo_space_->AllocateRaw(size_in_bytes, NOT_EXECUTABLE);
  } else if (CELL_SPACE == space) {
    allocation = cell_space_->AllocateRaw(size_in_bytes);
  } else if (PROPERTY_CELL_SPACE == space) {
    allocation = property_cell_space_->AllocateRaw(size_in_bytes);
  } else {
    DCHECK(MAP_SPACE == space);
    allocation = map_space_->AllocateRaw(size_in_bytes);
  }
  if (allocation.To(&object)) {
    OnAllocationEvent(object, size_in_bytes);
  } else {
    old_gen_exhausted_ = true;
  }
  return allocation;
}


void Heap::OnAllocationEvent(HeapObject* object, int size_in_bytes) {
  HeapProfiler* profiler = isolate_->heap_profiler();
  if (profiler->is_tracking_allocations()) {
    profiler->AllocationEvent(object->address(), size_in_bytes);
  }

  if (FLAG_verify_predictable) {
    ++allocations_count_;

    UpdateAllocationsHash(object);
    UpdateAllocationsHash(size_in_bytes);

    if ((FLAG_dump_allocations_digest_at_alloc > 0) &&
        (--dump_allocations_hash_countdown_ == 0)) {
      dump_allocations_hash_countdown_ = FLAG_dump_allocations_digest_at_alloc;
      PrintAlloctionsHash();
    }
  }
}


void Heap::OnMoveEvent(HeapObject* target, HeapObject* source,
                       int size_in_bytes) {
  HeapProfiler* heap_profiler = isolate_->heap_profiler();
  if (heap_profiler->is_tracking_object_moves()) {
    heap_profiler->ObjectMoveEvent(source->address(), target->address(),
                                   size_in_bytes);
  }

  if (isolate_->logger()->is_logging_code_events() ||
      isolate_->cpu_profiler()->is_profiling()) {
    if (target->IsSharedFunctionInfo()) {
      PROFILE(isolate_, SharedFunctionInfoMoveEvent(source->address(),
                                                    target->address()));
    }
  }

  if (FLAG_verify_predictable) {
    ++allocations_count_;

    UpdateAllocationsHash(source);
    UpdateAllocationsHash(target);
    UpdateAllocationsHash(size_in_bytes);

    if ((FLAG_dump_allocations_digest_at_alloc > 0) &&
        (--dump_allocations_hash_countdown_ == 0)) {
      dump_allocations_hash_countdown_ = FLAG_dump_allocations_digest_at_alloc;
      PrintAlloctionsHash();
    }
  }
}


void Heap::UpdateAllocationsHash(HeapObject* object) {
  Address object_address = object->address();
  MemoryChunk* memory_chunk = MemoryChunk::FromAddress(object_address);
  AllocationSpace allocation_space = memory_chunk->owner()->identity();

  STATIC_ASSERT(kSpaceTagSize + kPageSizeBits <= 32);
  uint32_t value =
      static_cast<uint32_t>(object_address - memory_chunk->address()) |
      (static_cast<uint32_t>(allocation_space) << kPageSizeBits);

  UpdateAllocationsHash(value);
}


void Heap::UpdateAllocationsHash(uint32_t value) {
  uint16_t c1 = static_cast<uint16_t>(value);
  uint16_t c2 = static_cast<uint16_t>(value >> 16);
  raw_allocations_hash_ =
      StringHasher::AddCharacterCore(raw_allocations_hash_, c1);
  raw_allocations_hash_ =
      StringHasher::AddCharacterCore(raw_allocations_hash_, c2);
}


void Heap::PrintAlloctionsHash() {
  uint32_t hash = StringHasher::GetHashCore(raw_allocations_hash_);
  PrintF("\n### Allocations = %u, hash = 0x%08x\n", allocations_count_, hash);
}


void Heap::FinalizeExternalString(String* string) {
  DCHECK(string->IsExternalString());
  v8::String::ExternalStringResourceBase** resource_addr =
      reinterpret_cast<v8::String::ExternalStringResourceBase**>(
          reinterpret_cast<byte*>(string) + ExternalString::kResourceOffset -
          kHeapObjectTag);

  // Dispose of the C++ object if it has not already been disposed.
  if (*resource_addr != NULL) {
    (*resource_addr)->Dispose();
    *resource_addr = NULL;
  }
}


bool Heap::InNewSpace(Object* object) {
  bool result = new_space_.Contains(object);
  DCHECK(!result ||                 // Either not in new space
         gc_state_ != NOT_IN_GC ||  // ... or in the middle of GC
         InToSpace(object));        // ... or in to-space (where we allocate).
  return result;
}


bool Heap::InNewSpace(Address address) { return new_space_.Contains(address); }


bool Heap::InFromSpace(Object* object) {
  return new_space_.FromSpaceContains(object);
}


bool Heap::InToSpace(Object* object) {
  return new_space_.ToSpaceContains(object);
}


bool Heap::InOldPointerSpace(Address address) {
  return old_pointer_space_->Contains(address);
}


bool Heap::InOldPointerSpace(Object* object) {
  return InOldPointerSpace(reinterpret_cast<Address>(object));
}


bool Heap::InOldDataSpace(Address address) {
  return old_data_space_->Contains(address);
}


bool Heap::InOldDataSpace(Object* object) {
  return InOldDataSpace(reinterpret_cast<Address>(object));
}


bool Heap::OldGenerationAllocationLimitReached() {
  if (!incremental_marking()->IsStopped()) return false;
  return OldGenerationSpaceAvailable() < 0;
}


bool Heap::ShouldBePromoted(Address old_address, int object_size) {
  NewSpacePage* page = NewSpacePage::FromAddress(old_address);
  Address age_mark = new_space_.age_mark();
  return page->IsFlagSet(MemoryChunk::NEW_SPACE_BELOW_AGE_MARK) &&
         (!page->ContainsLimit(age_mark) || old_address < age_mark);
}


void Heap::RecordWrite(Address address, int offset) {
  if (!InNewSpace(address)) store_buffer_.Mark(address + offset);
}


void Heap::RecordWrites(Address address, int start, int len) {
  if (!InNewSpace(address)) {
    for (int i = 0; i < len; i++) {
      store_buffer_.Mark(address + start + i * kPointerSize);
    }
  }
}


OldSpace* Heap::TargetSpace(HeapObject* object) {
  InstanceType type = object->map()->instance_type();
  AllocationSpace space = TargetSpaceId(type);
  return (space == OLD_POINTER_SPACE) ? old_pointer_space_ : old_data_space_;
}


AllocationSpace Heap::TargetSpaceId(InstanceType type) {
  // Heap numbers and sequential strings are promoted to old data space, all
  // other object types are promoted to old pointer space.  We do not use
  // object->IsHeapNumber() and object->IsSeqString() because we already
  // know that object has the heap object tag.

  // These objects are never allocated in new space.
  DCHECK(type != MAP_TYPE);
  DCHECK(type != CODE_TYPE);
  DCHECK(type != ODDBALL_TYPE);
  DCHECK(type != CELL_TYPE);
  DCHECK(type != PROPERTY_CELL_TYPE);

  if (type <= LAST_NAME_TYPE) {
    if (type == SYMBOL_TYPE) return OLD_POINTER_SPACE;
    DCHECK(type < FIRST_NONSTRING_TYPE);
    // There are four string representations: sequential strings, external
    // strings, cons strings, and sliced strings.
    // Only the latter two contain non-map-word pointers to heap objects.
    return ((type & kIsIndirectStringMask) == kIsIndirectStringTag)
               ? OLD_POINTER_SPACE
               : OLD_DATA_SPACE;
  } else {
    return (type <= LAST_DATA_TYPE) ? OLD_DATA_SPACE : OLD_POINTER_SPACE;
  }
}


bool Heap::AllowedToBeMigrated(HeapObject* obj, AllocationSpace dst) {
  // Object migration is governed by the following rules:
  //
  // 1) Objects in new-space can be migrated to one of the old spaces
  //    that matches their target space or they stay in new-space.
  // 2) Objects in old-space stay in the same space when migrating.
  // 3) Fillers (two or more words) can migrate due to left-trimming of
  //    fixed arrays in new-space, old-data-space and old-pointer-space.
  // 4) Fillers (one word) can never migrate, they are skipped by
  //    incremental marking explicitly to prevent invalid pattern.
  // 5) Short external strings can end up in old pointer space when a cons
  //    string in old pointer space is made external (String::MakeExternal).
  //
  // Since this function is used for debugging only, we do not place
  // asserts here, but check everything explicitly.
  if (obj->map() == one_pointer_filler_map()) return false;
  InstanceType type = obj->map()->instance_type();
  MemoryChunk* chunk = MemoryChunk::FromAddress(obj->address());
  AllocationSpace src = chunk->owner()->identity();
  switch (src) {
    case NEW_SPACE:
      return dst == src || dst == TargetSpaceId(type);
    case OLD_POINTER_SPACE:
      return dst == src && (dst == TargetSpaceId(type) || obj->IsFiller() ||
                            obj->IsExternalString());
    case OLD_DATA_SPACE:
      return dst == src && dst == TargetSpaceId(type);
    case CODE_SPACE:
      return dst == src && type == CODE_TYPE;
    case MAP_SPACE:
    case CELL_SPACE:
    case PROPERTY_CELL_SPACE:
    case LO_SPACE:
      return false;
  }
  UNREACHABLE();
  return false;
}


void Heap::CopyBlock(Address dst, Address src, int byte_size) {
  CopyWords(reinterpret_cast<Object**>(dst), reinterpret_cast<Object**>(src),
            static_cast<size_t>(byte_size / kPointerSize));
}


void Heap::MoveBlock(Address dst, Address src, int byte_size) {
  DCHECK(IsAligned(byte_size, kPointerSize));

  int size_in_words = byte_size / kPointerSize;

  if ((dst < src) || (dst >= (src + byte_size))) {
    Object** src_slot = reinterpret_cast<Object**>(src);
    Object** dst_slot = reinterpret_cast<Object**>(dst);
    Object** end_slot = src_slot + size_in_words;

    while (src_slot != end_slot) {
      *dst_slot++ = *src_slot++;
    }
  } else {
    MemMove(dst, src, static_cast<size_t>(byte_size));
  }
}


void Heap::ScavengePointer(HeapObject** p) { ScavengeObject(p, *p); }


AllocationMemento* Heap::FindAllocationMemento(HeapObject* object) {
  // Check if there is potentially a memento behind the object. If
  // the last word of the memento is on another page we return
  // immediately.
  Address object_address = object->address();
  Address memento_address = object_address + object->Size();
  Address last_memento_word_address = memento_address + kPointerSize;
  if (!NewSpacePage::OnSamePage(object_address, last_memento_word_address)) {
    return NULL;
  }

  HeapObject* candidate = HeapObject::FromAddress(memento_address);
  Map* candidate_map = candidate->map();
  // This fast check may peek at an uninitialized word. However, the slow check
  // below (memento_address == top) ensures that this is safe. Mark the word as
  // initialized to silence MemorySanitizer warnings.
  MSAN_MEMORY_IS_INITIALIZED(&candidate_map, sizeof(candidate_map));
  if (candidate_map != allocation_memento_map()) return NULL;

  // Either the object is the last object in the new space, or there is another
  // object of at least word size (the header map word) following it, so
  // suffices to compare ptr and top here. Note that technically we do not have
  // to compare with the current top pointer of the from space page during GC,
  // since we always install filler objects above the top pointer of a from
  // space page when performing a garbage collection. However, always performing
  // the test makes it possible to have a single, unified version of
  // FindAllocationMemento that is used both by the GC and the mutator.
  Address top = NewSpaceTop();
  DCHECK(memento_address == top ||
         memento_address + HeapObject::kHeaderSize <= top ||
         !NewSpacePage::OnSamePage(memento_address, top));
  if (memento_address == top) return NULL;

  AllocationMemento* memento = AllocationMemento::cast(candidate);
  if (!memento->IsValid()) return NULL;
  return memento;
}


void Heap::UpdateAllocationSiteFeedback(HeapObject* object,
                                        ScratchpadSlotMode mode) {
  Heap* heap = object->GetHeap();
  DCHECK(heap->InFromSpace(object));

  if (!FLAG_allocation_site_pretenuring ||
      !AllocationSite::CanTrack(object->map()->instance_type()))
    return;

  AllocationMemento* memento = heap->FindAllocationMemento(object);
  if (memento == NULL) return;

  if (memento->GetAllocationSite()->IncrementMementoFoundCount()) {
    heap->AddAllocationSiteToScratchpad(memento->GetAllocationSite(), mode);
  }
}


void Heap::ScavengeObject(HeapObject** p, HeapObject* object) {
  DCHECK(object->GetIsolate()->heap()->InFromSpace(object));

  // We use the first word (where the map pointer usually is) of a heap
  // object to record the forwarding pointer.  A forwarding pointer can
  // point to an old space, the code space, or the to space of the new
  // generation.
  MapWord first_word = object->map_word();

  // If the first word is a forwarding address, the object has already been
  // copied.
  if (first_word.IsForwardingAddress()) {
    HeapObject* dest = first_word.ToForwardingAddress();
    DCHECK(object->GetIsolate()->heap()->InFromSpace(*p));
    *p = dest;
    return;
  }

  UpdateAllocationSiteFeedback(object, IGNORE_SCRATCHPAD_SLOT);

  // AllocationMementos are unrooted and shouldn't survive a scavenge
  DCHECK(object->map() != object->GetHeap()->allocation_memento_map());
  // Call the slow part of scavenge object.
  return ScavengeObjectSlow(p, object);
}


bool Heap::CollectGarbage(AllocationSpace space, const char* gc_reason,
                          const v8::GCCallbackFlags callbackFlags) {
  const char* collector_reason = NULL;
  GarbageCollector collector = SelectGarbageCollector(space, &collector_reason);
  return CollectGarbage(collector, gc_reason, collector_reason, callbackFlags);
}


Isolate* Heap::isolate() {
  return reinterpret_cast<Isolate*>(
      reinterpret_cast<intptr_t>(this) -
      reinterpret_cast<size_t>(reinterpret_cast<Isolate*>(4)->heap()) + 4);
}


// Calls the FUNCTION_CALL function and retries it up to three times
// to guarantee that any allocations performed during the call will
// succeed if there's enough memory.

// Warning: Do not use the identifiers __object__, __maybe_object__ or
// __scope__ in a call to this macro.

#define RETURN_OBJECT_UNLESS_RETRY(ISOLATE, RETURN_VALUE) \
  if (__allocation__.To(&__object__)) {                   \
    DCHECK(__object__ != (ISOLATE)->heap()->exception()); \
    RETURN_VALUE;                                         \
  }

#define CALL_AND_RETRY(ISOLATE, FUNCTION_CALL, RETURN_VALUE, RETURN_EMPTY)    \
  do {                                                                        \
    AllocationResult __allocation__ = FUNCTION_CALL;                          \
    Object* __object__ = NULL;                                                \
    RETURN_OBJECT_UNLESS_RETRY(ISOLATE, RETURN_VALUE)                         \
    (ISOLATE)->heap()->CollectGarbage(__allocation__.RetrySpace(),            \
                                      "allocation failure");                  \
    __allocation__ = FUNCTION_CALL;                                           \
    RETURN_OBJECT_UNLESS_RETRY(ISOLATE, RETURN_VALUE)                         \
    (ISOLATE)->counters()->gc_last_resort_from_handles()->Increment();        \
    (ISOLATE)->heap()->CollectAllAvailableGarbage("last resort gc");          \
    {                                                                         \
      AlwaysAllocateScope __scope__(ISOLATE);                                 \
      __allocation__ = FUNCTION_CALL;                                         \
    }                                                                         \
    RETURN_OBJECT_UNLESS_RETRY(ISOLATE, RETURN_VALUE)                         \
    /* TODO(1181417): Fix this. */                                            \
    v8::internal::Heap::FatalProcessOutOfMemory("CALL_AND_RETRY_LAST", true); \
    RETURN_EMPTY;                                                             \
  } while (false)

#define CALL_AND_RETRY_OR_DIE(ISOLATE, FUNCTION_CALL, RETURN_VALUE, \
                              RETURN_EMPTY)                         \
  CALL_AND_RETRY(ISOLATE, FUNCTION_CALL, RETURN_VALUE, RETURN_EMPTY)

#define CALL_HEAP_FUNCTION(ISOLATE, FUNCTION_CALL, TYPE)                      \
  CALL_AND_RETRY_OR_DIE(ISOLATE, FUNCTION_CALL,                               \
                        return Handle<TYPE>(TYPE::cast(__object__), ISOLATE), \
                        return Handle<TYPE>())


#define CALL_HEAP_FUNCTION_VOID(ISOLATE, FUNCTION_CALL) \
  CALL_AND_RETRY_OR_DIE(ISOLATE, FUNCTION_CALL, return, return)


void ExternalStringTable::AddString(String* string) {
  DCHECK(string->IsExternalString());
  if (heap_->InNewSpace(string)) {
    new_space_strings_.Add(string);
  } else {
    old_space_strings_.Add(string);
  }
}


void ExternalStringTable::Iterate(ObjectVisitor* v) {
  if (!new_space_strings_.is_empty()) {
    Object** start = &new_space_strings_[0];
    v->VisitPointers(start, start + new_space_strings_.length());
  }
  if (!old_space_strings_.is_empty()) {
    Object** start = &old_space_strings_[0];
    v->VisitPointers(start, start + old_space_strings_.length());
  }
}


// Verify() is inline to avoid ifdef-s around its calls in release
// mode.
void ExternalStringTable::Verify() {
#ifdef DEBUG
  for (int i = 0; i < new_space_strings_.length(); ++i) {
    Object* obj = Object::cast(new_space_strings_[i]);
    DCHECK(heap_->InNewSpace(obj));
    DCHECK(obj != heap_->the_hole_value());
  }
  for (int i = 0; i < old_space_strings_.length(); ++i) {
    Object* obj = Object::cast(old_space_strings_[i]);
    DCHECK(!heap_->InNewSpace(obj));
    DCHECK(obj != heap_->the_hole_value());
  }
#endif
}


void ExternalStringTable::AddOldString(String* string) {
  DCHECK(string->IsExternalString());
  DCHECK(!heap_->InNewSpace(string));
  old_space_strings_.Add(string);
}


void ExternalStringTable::ShrinkNewStrings(int position) {
  new_space_strings_.Rewind(position);
#ifdef VERIFY_HEAP
  if (FLAG_verify_heap) {
    Verify();
  }
#endif
}


void Heap::ClearInstanceofCache() {
  set_instanceof_cache_function(Smi::FromInt(0));
}


Object* Heap::ToBoolean(bool condition) {
  return condition ? true_value() : false_value();
}


void Heap::CompletelyClearInstanceofCache() {
  set_instanceof_cache_map(Smi::FromInt(0));
  set_instanceof_cache_function(Smi::FromInt(0));
}


AlwaysAllocateScope::AlwaysAllocateScope(Isolate* isolate)
    : heap_(isolate->heap()), daf_(isolate) {
  // We shouldn't hit any nested scopes, because that requires
  // non-handle code to call handle code. The code still works but
  // performance will degrade, so we want to catch this situation
  // in debug mode.
  DCHECK(heap_->always_allocate_scope_depth_ == 0);
  heap_->always_allocate_scope_depth_++;
}


AlwaysAllocateScope::~AlwaysAllocateScope() {
  heap_->always_allocate_scope_depth_--;
  DCHECK(heap_->always_allocate_scope_depth_ == 0);
}


#ifdef VERIFY_HEAP
NoWeakObjectVerificationScope::NoWeakObjectVerificationScope() {
  Isolate* isolate = Isolate::Current();
  isolate->heap()->no_weak_object_verification_scope_depth_++;
}


NoWeakObjectVerificationScope::~NoWeakObjectVerificationScope() {
  Isolate* isolate = Isolate::Current();
  isolate->heap()->no_weak_object_verification_scope_depth_--;
}
#endif


GCCallbacksScope::GCCallbacksScope(Heap* heap) : heap_(heap) {
  heap_->gc_callbacks_depth_++;
}


GCCallbacksScope::~GCCallbacksScope() { heap_->gc_callbacks_depth_--; }


bool GCCallbacksScope::CheckReenter() {
  return heap_->gc_callbacks_depth_ == 1;
}


void VerifyPointersVisitor::VisitPointers(Object** start, Object** end) {
  for (Object** current = start; current < end; current++) {
    if ((*current)->IsHeapObject()) {
      HeapObject* object = HeapObject::cast(*current);
      CHECK(object->GetIsolate()->heap()->Contains(object));
      CHECK(object->map()->IsMap());
    }
  }
}


void VerifySmisVisitor::VisitPointers(Object** start, Object** end) {
  for (Object** current = start; current < end; current++) {
    CHECK((*current)->IsSmi());
  }
}
}
}  // namespace v8::internal

#endif  // V8_HEAP_HEAP_INL_H_
