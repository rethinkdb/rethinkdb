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

#include "accessors.h"
#include "api.h"
#include "bootstrapper.h"
#include "codegen.h"
#include "compilation-cache.h"
#include "debug.h"
#include "deoptimizer.h"
#include "global-handles.h"
#include "heap-profiler.h"
#include "incremental-marking.h"
#include "liveobjectlist-inl.h"
#include "mark-compact.h"
#include "natives.h"
#include "objects-visiting.h"
#include "objects-visiting-inl.h"
#include "once.h"
#include "runtime-profiler.h"
#include "scopeinfo.h"
#include "snapshot.h"
#include "store-buffer.h"
#include "v8threads.h"
#include "v8utils.h"
#include "vm-state-inl.h"
#if V8_TARGET_ARCH_ARM && !V8_INTERPRETED_REGEXP
#include "regexp-macro-assembler.h"
#include "arm/regexp-macro-assembler-arm.h"
#endif
#if V8_TARGET_ARCH_MIPS && !V8_INTERPRETED_REGEXP
#include "regexp-macro-assembler.h"
#include "mips/regexp-macro-assembler-mips.h"
#endif

namespace v8 {
namespace internal {


Heap::Heap()
    : isolate_(NULL),
// semispace_size_ should be a power of 2 and old_generation_size_ should be
// a multiple of Page::kPageSize.
#if defined(V8_TARGET_ARCH_X64)
#define LUMP_OF_MEMORY (2 * MB)
      code_range_size_(512*MB),
#else
#define LUMP_OF_MEMORY MB
      code_range_size_(0),
#endif
#if defined(ANDROID)
      reserved_semispace_size_(4 * Max(LUMP_OF_MEMORY, Page::kPageSize)),
      max_semispace_size_(4 * Max(LUMP_OF_MEMORY, Page::kPageSize)),
      initial_semispace_size_(Page::kPageSize),
      max_old_generation_size_(192*MB),
      max_executable_size_(max_old_generation_size_),
#else
      reserved_semispace_size_(8 * Max(LUMP_OF_MEMORY, Page::kPageSize)),
      max_semispace_size_(8 * Max(LUMP_OF_MEMORY, Page::kPageSize)),
      initial_semispace_size_(Page::kPageSize),
      max_old_generation_size_(700ul * LUMP_OF_MEMORY),
      max_executable_size_(256l * LUMP_OF_MEMORY),
#endif

// Variables set based on semispace_size_ and old_generation_size_ in
// ConfigureHeap (survived_since_last_expansion_, external_allocation_limit_)
// Will be 4 * reserved_semispace_size_ to ensure that young
// generation can be aligned to its size.
      survived_since_last_expansion_(0),
      sweep_generation_(0),
      always_allocate_scope_depth_(0),
      linear_allocation_scope_depth_(0),
      contexts_disposed_(0),
      global_ic_age_(0),
      flush_monomorphic_ics_(false),
      scan_on_scavenge_pages_(0),
      new_space_(this),
      old_pointer_space_(NULL),
      old_data_space_(NULL),
      code_space_(NULL),
      map_space_(NULL),
      cell_space_(NULL),
      lo_space_(NULL),
      gc_state_(NOT_IN_GC),
      gc_post_processing_depth_(0),
      ms_count_(0),
      gc_count_(0),
      remembered_unmapped_pages_index_(0),
      unflattened_strings_length_(0),
#ifdef DEBUG
      allocation_allowed_(true),
      allocation_timeout_(0),
      disallow_allocation_failure_(false),
      debug_utils_(NULL),
#endif  // DEBUG
      new_space_high_promotion_mode_active_(false),
      old_gen_promotion_limit_(kMinimumPromotionLimit),
      old_gen_allocation_limit_(kMinimumAllocationLimit),
      old_gen_limit_factor_(1),
      size_of_old_gen_at_last_old_space_gc_(0),
      external_allocation_limit_(0),
      amount_of_external_allocated_memory_(0),
      amount_of_external_allocated_memory_at_last_global_gc_(0),
      old_gen_exhausted_(false),
      store_buffer_rebuilder_(store_buffer()),
      hidden_symbol_(NULL),
      global_gc_prologue_callback_(NULL),
      global_gc_epilogue_callback_(NULL),
      gc_safe_size_of_old_object_(NULL),
      total_regexp_code_generated_(0),
      tracer_(NULL),
      young_survivors_after_last_gc_(0),
      high_survival_rate_period_length_(0),
      survival_rate_(0),
      previous_survival_rate_trend_(Heap::STABLE),
      survival_rate_trend_(Heap::STABLE),
      max_gc_pause_(0),
      total_gc_time_ms_(0),
      max_alive_after_gc_(0),
      min_in_mutator_(kMaxInt),
      alive_after_last_gc_(0),
      last_gc_end_timestamp_(0.0),
      store_buffer_(this),
      marking_(this),
      incremental_marking_(this),
      number_idle_notifications_(0),
      last_idle_notification_gc_count_(0),
      last_idle_notification_gc_count_init_(false),
      mark_sweeps_since_idle_round_started_(0),
      ms_count_at_last_idle_notification_(0),
      gc_count_at_last_idle_gc_(0),
      scavenges_since_last_idle_round_(kIdleScavengeThreshold),
      promotion_queue_(this),
      configured_(false),
      chunks_queued_for_free_(NULL),
      relocation_mutex_(NULL) {
  // Allow build-time customization of the max semispace size. Building
  // V8 with snapshots and a non-default max semispace size is much
  // easier if you can define it as part of the build environment.
#if defined(V8_MAX_SEMISPACE_SIZE)
  max_semispace_size_ = reserved_semispace_size_ = V8_MAX_SEMISPACE_SIZE;
#endif

  intptr_t max_virtual = OS::MaxVirtualMemory();

  if (max_virtual > 0) {
    if (code_range_size_ > 0) {
      // Reserve no more than 1/8 of the memory for the code range.
      code_range_size_ = Min(code_range_size_, max_virtual >> 3);
    }
  }

  memset(roots_, 0, sizeof(roots_[0]) * kRootListLength);
  native_contexts_list_ = NULL;
  mark_compact_collector_.heap_ = this;
  external_string_table_.heap_ = this;
  // Put a dummy entry in the remembered pages so we can find the list the
  // minidump even if there are no real unmapped pages.
  RememberUnmappedPage(NULL, false);

  ClearObjectStats(true);
}


intptr_t Heap::Capacity() {
  if (!HasBeenSetUp()) return 0;

  return new_space_.Capacity() +
      old_pointer_space_->Capacity() +
      old_data_space_->Capacity() +
      code_space_->Capacity() +
      map_space_->Capacity() +
      cell_space_->Capacity();
}


intptr_t Heap::CommittedMemory() {
  if (!HasBeenSetUp()) return 0;

  return new_space_.CommittedMemory() +
      old_pointer_space_->CommittedMemory() +
      old_data_space_->CommittedMemory() +
      code_space_->CommittedMemory() +
      map_space_->CommittedMemory() +
      cell_space_->CommittedMemory() +
      lo_space_->Size();
}

intptr_t Heap::CommittedMemoryExecutable() {
  if (!HasBeenSetUp()) return 0;

  return isolate()->memory_allocator()->SizeExecutable();
}


intptr_t Heap::Available() {
  if (!HasBeenSetUp()) return 0;

  return new_space_.Available() +
      old_pointer_space_->Available() +
      old_data_space_->Available() +
      code_space_->Available() +
      map_space_->Available() +
      cell_space_->Available();
}


bool Heap::HasBeenSetUp() {
  return old_pointer_space_ != NULL &&
         old_data_space_ != NULL &&
         code_space_ != NULL &&
         map_space_ != NULL &&
         cell_space_ != NULL &&
         lo_space_ != NULL;
}


int Heap::GcSafeSizeOfOldObject(HeapObject* object) {
  if (IntrusiveMarking::IsMarked(object)) {
    return IntrusiveMarking::SizeOfMarkedObject(object);
  }
  return object->SizeFromMap(object->map());
}


GarbageCollector Heap::SelectGarbageCollector(AllocationSpace space,
                                              const char** reason) {
  // Is global GC requested?
  if (space != NEW_SPACE) {
    isolate_->counters()->gc_compactor_caused_by_request()->Increment();
    *reason = "GC in old space requested";
    return MARK_COMPACTOR;
  }

  if (FLAG_gc_global || (FLAG_stress_compaction && (gc_count_ & 1) != 0)) {
    *reason = "GC in old space forced by flags";
    return MARK_COMPACTOR;
  }

  // Is enough data promoted to justify a global GC?
  if (OldGenerationPromotionLimitReached()) {
    isolate_->counters()->gc_compactor_caused_by_promoted_data()->Increment();
    *reason = "promotion limit reached";
    return MARK_COMPACTOR;
  }

  // Have allocation in OLD and LO failed?
  if (old_gen_exhausted_) {
    isolate_->counters()->
        gc_compactor_caused_by_oldspace_exhaustion()->Increment();
    *reason = "old generations exhausted";
    return MARK_COMPACTOR;
  }

  // Is there enough space left in OLD to guarantee that a scavenge can
  // succeed?
  //
  // Note that MemoryAllocator->MaxAvailable() undercounts the memory available
  // for object promotion. It counts only the bytes that the memory
  // allocator has not yet allocated from the OS and assigned to any space,
  // and does not count available bytes already in the old space or code
  // space.  Undercounting is safe---we may get an unrequested full GC when
  // a scavenge would have succeeded.
  if (isolate_->memory_allocator()->MaxAvailable() <= new_space_.Size()) {
    isolate_->counters()->
        gc_compactor_caused_by_oldspace_exhaustion()->Increment();
    *reason = "scavenge might not succeed";
    return MARK_COMPACTOR;
  }

  // Default
  *reason = NULL;
  return SCAVENGER;
}


// TODO(1238405): Combine the infrastructure for --heap-stats and
// --log-gc to avoid the complicated preprocessor and flag testing.
void Heap::ReportStatisticsBeforeGC() {
  // Heap::ReportHeapStatistics will also log NewSpace statistics when
  // compiled --log-gc is set.  The following logic is used to avoid
  // double logging.
#ifdef DEBUG
  if (FLAG_heap_stats || FLAG_log_gc) new_space_.CollectStatistics();
  if (FLAG_heap_stats) {
    ReportHeapStatistics("Before GC");
  } else if (FLAG_log_gc) {
    new_space_.ReportStatistics();
  }
  if (FLAG_heap_stats || FLAG_log_gc) new_space_.ClearHistograms();
#else
  if (FLAG_log_gc) {
    new_space_.CollectStatistics();
    new_space_.ReportStatistics();
    new_space_.ClearHistograms();
  }
#endif  // DEBUG
}


void Heap::PrintShortHeapStatistics() {
  if (!FLAG_trace_gc_verbose) return;
  PrintPID("Memory allocator,   used: %6" V8_PTR_PREFIX "d KB"
               ", available: %6" V8_PTR_PREFIX "d KB\n",
           isolate_->memory_allocator()->Size() / KB,
           isolate_->memory_allocator()->Available() / KB);
  PrintPID("New space,          used: %6" V8_PTR_PREFIX "d KB"
               ", available: %6" V8_PTR_PREFIX "d KB"
               ", committed: %6" V8_PTR_PREFIX "d KB\n",
           new_space_.Size() / KB,
           new_space_.Available() / KB,
           new_space_.CommittedMemory() / KB);
  PrintPID("Old pointers,       used: %6" V8_PTR_PREFIX "d KB"
               ", available: %6" V8_PTR_PREFIX "d KB"
               ", committed: %6" V8_PTR_PREFIX "d KB\n",
           old_pointer_space_->SizeOfObjects() / KB,
           old_pointer_space_->Available() / KB,
           old_pointer_space_->CommittedMemory() / KB);
  PrintPID("Old data space,     used: %6" V8_PTR_PREFIX "d KB"
               ", available: %6" V8_PTR_PREFIX "d KB"
               ", committed: %6" V8_PTR_PREFIX "d KB\n",
           old_data_space_->SizeOfObjects() / KB,
           old_data_space_->Available() / KB,
           old_data_space_->CommittedMemory() / KB);
  PrintPID("Code space,         used: %6" V8_PTR_PREFIX "d KB"
               ", available: %6" V8_PTR_PREFIX "d KB"
               ", committed: %6" V8_PTR_PREFIX "d KB\n",
           code_space_->SizeOfObjects() / KB,
           code_space_->Available() / KB,
           code_space_->CommittedMemory() / KB);
  PrintPID("Map space,          used: %6" V8_PTR_PREFIX "d KB"
               ", available: %6" V8_PTR_PREFIX "d KB"
               ", committed: %6" V8_PTR_PREFIX "d KB\n",
           map_space_->SizeOfObjects() / KB,
           map_space_->Available() / KB,
           map_space_->CommittedMemory() / KB);
  PrintPID("Cell space,         used: %6" V8_PTR_PREFIX "d KB"
               ", available: %6" V8_PTR_PREFIX "d KB"
               ", committed: %6" V8_PTR_PREFIX "d KB\n",
           cell_space_->SizeOfObjects() / KB,
           cell_space_->Available() / KB,
           cell_space_->CommittedMemory() / KB);
  PrintPID("Large object space, used: %6" V8_PTR_PREFIX "d KB"
               ", available: %6" V8_PTR_PREFIX "d KB"
               ", committed: %6" V8_PTR_PREFIX "d KB\n",
           lo_space_->SizeOfObjects() / KB,
           lo_space_->Available() / KB,
           lo_space_->CommittedMemory() / KB);
  PrintPID("All spaces,         used: %6" V8_PTR_PREFIX "d KB"
               ", available: %6" V8_PTR_PREFIX "d KB"
               ", committed: %6" V8_PTR_PREFIX "d KB\n",
           this->SizeOfObjects() / KB,
           this->Available() / KB,
           this->CommittedMemory() / KB);
  PrintPID("Total time spent in GC  : %d ms\n", total_gc_time_ms_);
}


// TODO(1238405): Combine the infrastructure for --heap-stats and
// --log-gc to avoid the complicated preprocessor and flag testing.
void Heap::ReportStatisticsAfterGC() {
  // Similar to the before GC, we use some complicated logic to ensure that
  // NewSpace statistics are logged exactly once when --log-gc is turned on.
#if defined(DEBUG)
  if (FLAG_heap_stats) {
    new_space_.CollectStatistics();
    ReportHeapStatistics("After GC");
  } else if (FLAG_log_gc) {
    new_space_.ReportStatistics();
  }
#else
  if (FLAG_log_gc) new_space_.ReportStatistics();
#endif  // DEBUG
}


void Heap::GarbageCollectionPrologue() {
  isolate_->transcendental_cache()->Clear();
  ClearJSFunctionResultCaches();
  gc_count_++;
  unflattened_strings_length_ = 0;

#ifdef VERIFY_HEAP
  if (FLAG_verify_heap) {
    Verify();
  }
#endif

#ifdef DEBUG
  ASSERT(allocation_allowed_ && gc_state_ == NOT_IN_GC);
  allow_allocation(false);

  if (FLAG_gc_verbose) Print();

  ReportStatisticsBeforeGC();
#endif  // DEBUG

  LiveObjectList::GCPrologue();
  store_buffer()->GCPrologue();
}


intptr_t Heap::SizeOfObjects() {
  intptr_t total = 0;
  AllSpaces spaces;
  for (Space* space = spaces.next(); space != NULL; space = spaces.next()) {
    total += space->SizeOfObjects();
  }
  return total;
}


void Heap::RepairFreeListsAfterBoot() {
  PagedSpaces spaces;
  for (PagedSpace* space = spaces.next();
       space != NULL;
       space = spaces.next()) {
    space->RepairFreeListsAfterBoot();
  }
}


void Heap::GarbageCollectionEpilogue() {
  store_buffer()->GCEpilogue();
  LiveObjectList::GCEpilogue();

  // In release mode, we only zap the from space under heap verification.
  if (Heap::ShouldZapGarbage()) {
    ZapFromSpace();
  }

#ifdef VERIFY_HEAP
  if (FLAG_verify_heap) {
    Verify();
  }
#endif

#ifdef DEBUG
  allow_allocation(true);
  if (FLAG_print_global_handles) isolate_->global_handles()->Print();
  if (FLAG_print_handles) PrintHandles();
  if (FLAG_gc_verbose) Print();
  if (FLAG_code_stats) ReportCodeStatistics("After GC");
#endif

  isolate_->counters()->alive_after_last_gc()->Set(
      static_cast<int>(SizeOfObjects()));

  isolate_->counters()->symbol_table_capacity()->Set(
      symbol_table()->Capacity());
  isolate_->counters()->number_of_symbols()->Set(
      symbol_table()->NumberOfElements());

  if (CommittedMemory() > 0) {
    isolate_->counters()->external_fragmentation_total()->AddSample(
        static_cast<int>(100 - (SizeOfObjects() * 100.0) / CommittedMemory()));

    isolate_->counters()->heap_fraction_map_space()->AddSample(
        static_cast<int>(
            (map_space()->CommittedMemory() * 100.0) / CommittedMemory()));
    isolate_->counters()->heap_fraction_cell_space()->AddSample(
        static_cast<int>(
            (cell_space()->CommittedMemory() * 100.0) / CommittedMemory()));

    isolate_->counters()->heap_sample_total_committed()->AddSample(
        static_cast<int>(CommittedMemory() / KB));
    isolate_->counters()->heap_sample_total_used()->AddSample(
        static_cast<int>(SizeOfObjects() / KB));
    isolate_->counters()->heap_sample_map_space_committed()->AddSample(
        static_cast<int>(map_space()->CommittedMemory() / KB));
    isolate_->counters()->heap_sample_cell_space_committed()->AddSample(
        static_cast<int>(cell_space()->CommittedMemory() / KB));
  }

#define UPDATE_COUNTERS_FOR_SPACE(space)                                       \
  isolate_->counters()->space##_bytes_available()->Set(                        \
      static_cast<int>(space()->Available()));                                 \
  isolate_->counters()->space##_bytes_committed()->Set(                        \
      static_cast<int>(space()->CommittedMemory()));                           \
  isolate_->counters()->space##_bytes_used()->Set(                             \
      static_cast<int>(space()->SizeOfObjects()));
#define UPDATE_FRAGMENTATION_FOR_SPACE(space)                                  \
  if (space()->CommittedMemory() > 0) {                                        \
    isolate_->counters()->external_fragmentation_##space()->AddSample(         \
        static_cast<int>(100 -                                                 \
            (space()->SizeOfObjects() * 100.0) / space()->CommittedMemory())); \
  }
#define UPDATE_COUNTERS_AND_FRAGMENTATION_FOR_SPACE(space)                     \
  UPDATE_COUNTERS_FOR_SPACE(space)                                             \
  UPDATE_FRAGMENTATION_FOR_SPACE(space)

  UPDATE_COUNTERS_FOR_SPACE(new_space)
  UPDATE_COUNTERS_AND_FRAGMENTATION_FOR_SPACE(old_pointer_space)
  UPDATE_COUNTERS_AND_FRAGMENTATION_FOR_SPACE(old_data_space)
  UPDATE_COUNTERS_AND_FRAGMENTATION_FOR_SPACE(code_space)
  UPDATE_COUNTERS_AND_FRAGMENTATION_FOR_SPACE(map_space)
  UPDATE_COUNTERS_AND_FRAGMENTATION_FOR_SPACE(cell_space)
  UPDATE_COUNTERS_AND_FRAGMENTATION_FOR_SPACE(lo_space)
#undef UPDATE_COUNTERS_FOR_SPACE
#undef UPDATE_FRAGMENTATION_FOR_SPACE
#undef UPDATE_COUNTERS_AND_FRAGMENTATION_FOR_SPACE

#if defined(DEBUG)
  ReportStatisticsAfterGC();
#endif  // DEBUG
#ifdef ENABLE_DEBUGGER_SUPPORT
  isolate_->debug()->AfterGarbageCollection();
#endif  // ENABLE_DEBUGGER_SUPPORT
}


void Heap::CollectAllGarbage(int flags, const char* gc_reason) {
  // Since we are ignoring the return value, the exact choice of space does
  // not matter, so long as we do not specify NEW_SPACE, which would not
  // cause a full GC.
  mark_compact_collector_.SetFlags(flags);
  CollectGarbage(OLD_POINTER_SPACE, gc_reason);
  mark_compact_collector_.SetFlags(kNoGCFlags);
}


void Heap::CollectAllAvailableGarbage(const char* gc_reason) {
  // Since we are ignoring the return value, the exact choice of space does
  // not matter, so long as we do not specify NEW_SPACE, which would not
  // cause a full GC.
  // Major GC would invoke weak handle callbacks on weakly reachable
  // handles, but won't collect weakly reachable objects until next
  // major GC.  Therefore if we collect aggressively and weak handle callback
  // has been invoked, we rerun major GC to release objects which become
  // garbage.
  // Note: as weak callbacks can execute arbitrary code, we cannot
  // hope that eventually there will be no weak callbacks invocations.
  // Therefore stop recollecting after several attempts.
  mark_compact_collector()->SetFlags(kMakeHeapIterableMask |
                                     kReduceMemoryFootprintMask);
  isolate_->compilation_cache()->Clear();
  const int kMaxNumberOfAttempts = 7;
  for (int attempt = 0; attempt < kMaxNumberOfAttempts; attempt++) {
    if (!CollectGarbage(OLD_POINTER_SPACE, MARK_COMPACTOR, gc_reason, NULL)) {
      break;
    }
  }
  mark_compact_collector()->SetFlags(kNoGCFlags);
  new_space_.Shrink();
  UncommitFromSpace();
  Shrink();
  incremental_marking()->UncommitMarkingDeque();
}


bool Heap::CollectGarbage(AllocationSpace space,
                          GarbageCollector collector,
                          const char* gc_reason,
                          const char* collector_reason) {
  // The VM is in the GC state until exiting this function.
  VMState state(isolate_, GC);

#ifdef DEBUG
  // Reset the allocation timeout to the GC interval, but make sure to
  // allow at least a few allocations after a collection. The reason
  // for this is that we have a lot of allocation sequences and we
  // assume that a garbage collection will allow the subsequent
  // allocation attempts to go through.
  allocation_timeout_ = Max(6, FLAG_gc_interval);
#endif

  if (collector == SCAVENGER && !incremental_marking()->IsStopped()) {
    if (FLAG_trace_incremental_marking) {
      PrintF("[IncrementalMarking] Scavenge during marking.\n");
    }
  }

  if (collector == MARK_COMPACTOR &&
      !mark_compact_collector()->abort_incremental_marking_ &&
      !incremental_marking()->IsStopped() &&
      !incremental_marking()->should_hurry() &&
      FLAG_incremental_marking_steps) {
    // Make progress in incremental marking.
    const intptr_t kStepSizeWhenDelayedByScavenge = 1 * MB;
    incremental_marking()->Step(kStepSizeWhenDelayedByScavenge,
                                IncrementalMarking::NO_GC_VIA_STACK_GUARD);
    if (!incremental_marking()->IsComplete()) {
      if (FLAG_trace_incremental_marking) {
        PrintF("[IncrementalMarking] Delaying MarkSweep.\n");
      }
      collector = SCAVENGER;
      collector_reason = "incremental marking delaying mark-sweep";
    }
  }

  bool next_gc_likely_to_collect_more = false;

  { GCTracer tracer(this, gc_reason, collector_reason);
    GarbageCollectionPrologue();
    // The GC count was incremented in the prologue.  Tell the tracer about
    // it.
    tracer.set_gc_count(gc_count_);

    // Tell the tracer which collector we've selected.
    tracer.set_collector(collector);

    HistogramTimer* rate = (collector == SCAVENGER)
        ? isolate_->counters()->gc_scavenger()
        : isolate_->counters()->gc_compactor();
    rate->Start();
    next_gc_likely_to_collect_more =
        PerformGarbageCollection(collector, &tracer);
    rate->Stop();

    ASSERT(collector == SCAVENGER || incremental_marking()->IsStopped());

    // This can do debug callbacks and restart incremental marking.
    GarbageCollectionEpilogue();
  }

  if (incremental_marking()->IsStopped()) {
    if (incremental_marking()->WorthActivating() && NextGCIsLikelyToBeFull()) {
      incremental_marking()->Start();
    }
  }

  return next_gc_likely_to_collect_more;
}


void Heap::PerformScavenge() {
  GCTracer tracer(this, NULL, NULL);
  if (incremental_marking()->IsStopped()) {
    PerformGarbageCollection(SCAVENGER, &tracer);
  } else {
    PerformGarbageCollection(MARK_COMPACTOR, &tracer);
  }
}


#ifdef VERIFY_HEAP
// Helper class for verifying the symbol table.
class SymbolTableVerifier : public ObjectVisitor {
 public:
  void VisitPointers(Object** start, Object** end) {
    // Visit all HeapObject pointers in [start, end).
    for (Object** p = start; p < end; p++) {
      if ((*p)->IsHeapObject()) {
        // Check that the symbol is actually a symbol.
        CHECK((*p)->IsTheHole() || (*p)->IsUndefined() || (*p)->IsSymbol());
      }
    }
  }
};


static void VerifySymbolTable() {
  SymbolTableVerifier verifier;
  HEAP->symbol_table()->IterateElements(&verifier);
}
#endif  // VERIFY_HEAP


static bool AbortIncrementalMarkingAndCollectGarbage(
    Heap* heap,
    AllocationSpace space,
    const char* gc_reason = NULL) {
  heap->mark_compact_collector()->SetFlags(Heap::kAbortIncrementalMarkingMask);
  bool result = heap->CollectGarbage(space, gc_reason);
  heap->mark_compact_collector()->SetFlags(Heap::kNoGCFlags);
  return result;
}


void Heap::ReserveSpace(
    int *sizes,
    Address *locations_out) {
  bool gc_performed = true;
  int counter = 0;
  static const int kThreshold = 20;
  while (gc_performed && counter++ < kThreshold) {
    gc_performed = false;
    ASSERT(NEW_SPACE == FIRST_PAGED_SPACE - 1);
    for (int space = NEW_SPACE; space <= LAST_PAGED_SPACE; space++) {
      if (sizes[space] != 0) {
        MaybeObject* allocation;
        if (space == NEW_SPACE) {
          allocation = new_space()->AllocateRaw(sizes[space]);
        } else {
          allocation = paged_space(space)->AllocateRaw(sizes[space]);
        }
        FreeListNode* node;
        if (!allocation->To<FreeListNode>(&node)) {
          if (space == NEW_SPACE) {
            Heap::CollectGarbage(NEW_SPACE,
                                 "failed to reserve space in the new space");
          } else {
            AbortIncrementalMarkingAndCollectGarbage(
                this,
                static_cast<AllocationSpace>(space),
                "failed to reserve space in paged space");
          }
          gc_performed = true;
          break;
        } else {
          // Mark with a free list node, in case we have a GC before
          // deserializing.
          node->set_size(this, sizes[space]);
          locations_out[space] = node->address();
        }
      }
    }
  }

  if (gc_performed) {
    // Failed to reserve the space after several attempts.
    V8::FatalProcessOutOfMemory("Heap::ReserveSpace");
  }
}


void Heap::EnsureFromSpaceIsCommitted() {
  if (new_space_.CommitFromSpaceIfNeeded()) return;

  // Committing memory to from space failed.
  // Try shrinking and try again.
  Shrink();
  if (new_space_.CommitFromSpaceIfNeeded()) return;

  // Committing memory to from space failed again.
  // Memory is exhausted and we will die.
  V8::FatalProcessOutOfMemory("Committing semi space failed.");
}


void Heap::ClearJSFunctionResultCaches() {
  if (isolate_->bootstrapper()->IsActive()) return;

  Object* context = native_contexts_list_;
  while (!context->IsUndefined()) {
    // Get the caches for this context. GC can happen when the context
    // is not fully initialized, so the caches can be undefined.
    Object* caches_or_undefined =
        Context::cast(context)->get(Context::JSFUNCTION_RESULT_CACHES_INDEX);
    if (!caches_or_undefined->IsUndefined()) {
      FixedArray* caches = FixedArray::cast(caches_or_undefined);
      // Clear the caches:
      int length = caches->length();
      for (int i = 0; i < length; i++) {
        JSFunctionResultCache::cast(caches->get(i))->Clear();
      }
    }
    // Get the next context:
    context = Context::cast(context)->get(Context::NEXT_CONTEXT_LINK);
  }
}



void Heap::ClearNormalizedMapCaches() {
  if (isolate_->bootstrapper()->IsActive() &&
      !incremental_marking()->IsMarking()) {
    return;
  }

  Object* context = native_contexts_list_;
  while (!context->IsUndefined()) {
    // GC can happen when the context is not fully initialized,
    // so the cache can be undefined.
    Object* cache =
        Context::cast(context)->get(Context::NORMALIZED_MAP_CACHE_INDEX);
    if (!cache->IsUndefined()) {
      NormalizedMapCache::cast(cache)->Clear();
    }
    context = Context::cast(context)->get(Context::NEXT_CONTEXT_LINK);
  }
}


void Heap::UpdateSurvivalRateTrend(int start_new_space_size) {
  double survival_rate =
      (static_cast<double>(young_survivors_after_last_gc_) * 100) /
      start_new_space_size;

  if (survival_rate > kYoungSurvivalRateHighThreshold) {
    high_survival_rate_period_length_++;
  } else {
    high_survival_rate_period_length_ = 0;
  }

  if (survival_rate < kYoungSurvivalRateLowThreshold) {
    low_survival_rate_period_length_++;
  } else {
    low_survival_rate_period_length_ = 0;
  }

  double survival_rate_diff = survival_rate_ - survival_rate;

  if (survival_rate_diff > kYoungSurvivalRateAllowedDeviation) {
    set_survival_rate_trend(DECREASING);
  } else if (survival_rate_diff < -kYoungSurvivalRateAllowedDeviation) {
    set_survival_rate_trend(INCREASING);
  } else {
    set_survival_rate_trend(STABLE);
  }

  survival_rate_ = survival_rate;
}

bool Heap::PerformGarbageCollection(GarbageCollector collector,
                                    GCTracer* tracer) {
  bool next_gc_likely_to_collect_more = false;

  if (collector != SCAVENGER) {
    PROFILE(isolate_, CodeMovingGCEvent());
  }

#ifdef VERIFY_HEAP
  if (FLAG_verify_heap) {
    VerifySymbolTable();
  }
#endif

  if (collector == MARK_COMPACTOR && global_gc_prologue_callback_) {
    ASSERT(!allocation_allowed_);
    GCTracer::Scope scope(tracer, GCTracer::Scope::EXTERNAL);
    global_gc_prologue_callback_();
  }

  GCType gc_type =
      collector == MARK_COMPACTOR ? kGCTypeMarkSweepCompact : kGCTypeScavenge;

  for (int i = 0; i < gc_prologue_callbacks_.length(); ++i) {
    if (gc_type & gc_prologue_callbacks_[i].gc_type) {
      gc_prologue_callbacks_[i].callback(gc_type, kNoGCCallbackFlags);
    }
  }

  EnsureFromSpaceIsCommitted();

  int start_new_space_size = Heap::new_space()->SizeAsInt();

  if (IsHighSurvivalRate()) {
    // We speed up the incremental marker if it is running so that it
    // does not fall behind the rate of promotion, which would cause a
    // constantly growing old space.
    incremental_marking()->NotifyOfHighPromotionRate();
  }

  if (collector == MARK_COMPACTOR) {
    // Perform mark-sweep with optional compaction.
    MarkCompact(tracer);
    sweep_generation_++;
    bool high_survival_rate_during_scavenges = IsHighSurvivalRate() &&
        IsStableOrIncreasingSurvivalTrend();

    UpdateSurvivalRateTrend(start_new_space_size);

    size_of_old_gen_at_last_old_space_gc_ = PromotedSpaceSizeOfObjects();

    if (high_survival_rate_during_scavenges &&
        IsStableOrIncreasingSurvivalTrend()) {
      // Stable high survival rates of young objects both during partial and
      // full collection indicate that mutator is either building or modifying
      // a structure with a long lifetime.
      // In this case we aggressively raise old generation memory limits to
      // postpone subsequent mark-sweep collection and thus trade memory
      // space for the mutation speed.
      old_gen_limit_factor_ = 2;
    } else {
      old_gen_limit_factor_ = 1;
    }

    old_gen_promotion_limit_ =
        OldGenPromotionLimit(size_of_old_gen_at_last_old_space_gc_);
    old_gen_allocation_limit_ =
        OldGenAllocationLimit(size_of_old_gen_at_last_old_space_gc_);

    old_gen_exhausted_ = false;
  } else {
    tracer_ = tracer;
    Scavenge();
    tracer_ = NULL;

    UpdateSurvivalRateTrend(start_new_space_size);
  }

  if (!new_space_high_promotion_mode_active_ &&
      new_space_.Capacity() == new_space_.MaximumCapacity() &&
      IsStableOrIncreasingSurvivalTrend() &&
      IsHighSurvivalRate()) {
    // Stable high survival rates even though young generation is at
    // maximum capacity indicates that most objects will be promoted.
    // To decrease scavenger pauses and final mark-sweep pauses, we
    // have to limit maximal capacity of the young generation.
    new_space_high_promotion_mode_active_ = true;
    if (FLAG_trace_gc) {
      PrintPID("Limited new space size due to high promotion rate: %d MB\n",
               new_space_.InitialCapacity() / MB);
    }
  } else if (new_space_high_promotion_mode_active_ &&
      IsStableOrDecreasingSurvivalTrend() &&
      IsLowSurvivalRate()) {
    // Decreasing low survival rates might indicate that the above high
    // promotion mode is over and we should allow the young generation
    // to grow again.
    new_space_high_promotion_mode_active_ = false;
    if (FLAG_trace_gc) {
      PrintPID("Unlimited new space size due to low promotion rate: %d MB\n",
               new_space_.MaximumCapacity() / MB);
    }
  }

  if (new_space_high_promotion_mode_active_ &&
      new_space_.Capacity() > new_space_.InitialCapacity()) {
    new_space_.Shrink();
  }

  isolate_->counters()->objs_since_last_young()->Set(0);

  gc_post_processing_depth_++;
  { DisableAssertNoAllocation allow_allocation;
    GCTracer::Scope scope(tracer, GCTracer::Scope::EXTERNAL);
    next_gc_likely_to_collect_more =
        isolate_->global_handles()->PostGarbageCollectionProcessing(collector);
  }
  gc_post_processing_depth_--;

  // Update relocatables.
  Relocatable::PostGarbageCollectionProcessing();

  if (collector == MARK_COMPACTOR) {
    // Register the amount of external allocated memory.
    amount_of_external_allocated_memory_at_last_global_gc_ =
        amount_of_external_allocated_memory_;
  }

  GCCallbackFlags callback_flags = kNoGCCallbackFlags;
  for (int i = 0; i < gc_epilogue_callbacks_.length(); ++i) {
    if (gc_type & gc_epilogue_callbacks_[i].gc_type) {
      gc_epilogue_callbacks_[i].callback(gc_type, callback_flags);
    }
  }

  if (collector == MARK_COMPACTOR && global_gc_epilogue_callback_) {
    ASSERT(!allocation_allowed_);
    GCTracer::Scope scope(tracer, GCTracer::Scope::EXTERNAL);
    global_gc_epilogue_callback_();
  }

#ifdef VERIFY_HEAP
  if (FLAG_verify_heap) {
    VerifySymbolTable();
  }
#endif

  return next_gc_likely_to_collect_more;
}


void Heap::MarkCompact(GCTracer* tracer) {
  gc_state_ = MARK_COMPACT;
  LOG(isolate_, ResourceEvent("markcompact", "begin"));

  mark_compact_collector_.Prepare(tracer);

  ms_count_++;
  tracer->set_full_gc_count(ms_count_);

  MarkCompactPrologue();

  mark_compact_collector_.CollectGarbage();

  LOG(isolate_, ResourceEvent("markcompact", "end"));

  gc_state_ = NOT_IN_GC;

  isolate_->counters()->objs_since_last_full()->Set(0);

  contexts_disposed_ = 0;

  flush_monomorphic_ics_ = false;
}


void Heap::MarkCompactPrologue() {
  // At any old GC clear the keyed lookup cache to enable collection of unused
  // maps.
  isolate_->keyed_lookup_cache()->Clear();
  isolate_->context_slot_cache()->Clear();
  isolate_->descriptor_lookup_cache()->Clear();
  RegExpResultsCache::Clear(string_split_cache());
  RegExpResultsCache::Clear(regexp_multiple_cache());

  isolate_->compilation_cache()->MarkCompactPrologue();

  CompletelyClearInstanceofCache();

  FlushNumberStringCache();
  if (FLAG_cleanup_code_caches_at_gc) {
    polymorphic_code_cache()->set_cache(undefined_value());
  }

  ClearNormalizedMapCaches();
}


Object* Heap::FindCodeObject(Address a) {
  return isolate()->inner_pointer_to_code_cache()->
      GcSafeFindCodeForInnerPointer(a);
}


// Helper class for copying HeapObjects
class ScavengeVisitor: public ObjectVisitor {
 public:
  explicit ScavengeVisitor(Heap* heap) : heap_(heap) {}

  void VisitPointer(Object** p) { ScavengePointer(p); }

  void VisitPointers(Object** start, Object** end) {
    // Copy all HeapObject pointers in [start, end)
    for (Object** p = start; p < end; p++) ScavengePointer(p);
  }

 private:
  void ScavengePointer(Object** p) {
    Object* object = *p;
    if (!heap_->InNewSpace(object)) return;
    Heap::ScavengeObject(reinterpret_cast<HeapObject**>(p),
                         reinterpret_cast<HeapObject*>(object));
  }

  Heap* heap_;
};


#ifdef VERIFY_HEAP
// Visitor class to verify pointers in code or data space do not point into
// new space.
class VerifyNonPointerSpacePointersVisitor: public ObjectVisitor {
 public:
  void VisitPointers(Object** start, Object**end) {
    for (Object** current = start; current < end; current++) {
      if ((*current)->IsHeapObject()) {
        CHECK(!HEAP->InNewSpace(HeapObject::cast(*current)));
      }
    }
  }
};


static void VerifyNonPointerSpacePointers() {
  // Verify that there are no pointers to new space in spaces where we
  // do not expect them.
  VerifyNonPointerSpacePointersVisitor v;
  HeapObjectIterator code_it(HEAP->code_space());
  for (HeapObject* object = code_it.Next();
       object != NULL; object = code_it.Next())
    object->Iterate(&v);

  // The old data space was normally swept conservatively so that the iterator
  // doesn't work, so we normally skip the next bit.
  if (!HEAP->old_data_space()->was_swept_conservatively()) {
    HeapObjectIterator data_it(HEAP->old_data_space());
    for (HeapObject* object = data_it.Next();
         object != NULL; object = data_it.Next())
      object->Iterate(&v);
  }
}
#endif  // VERIFY_HEAP


void Heap::CheckNewSpaceExpansionCriteria() {
  if (new_space_.Capacity() < new_space_.MaximumCapacity() &&
      survived_since_last_expansion_ > new_space_.Capacity() &&
      !new_space_high_promotion_mode_active_) {
    // Grow the size of new space if there is room to grow, enough data
    // has survived scavenge since the last expansion and we are not in
    // high promotion mode.
    new_space_.Grow();
    survived_since_last_expansion_ = 0;
  }
}


static bool IsUnscavengedHeapObject(Heap* heap, Object** p) {
  return heap->InNewSpace(*p) &&
      !HeapObject::cast(*p)->map_word().IsForwardingAddress();
}


void Heap::ScavengeStoreBufferCallback(
    Heap* heap,
    MemoryChunk* page,
    StoreBufferEvent event) {
  heap->store_buffer_rebuilder_.Callback(page, event);
}


void StoreBufferRebuilder::Callback(MemoryChunk* page, StoreBufferEvent event) {
  if (event == kStoreBufferStartScanningPagesEvent) {
    start_of_current_page_ = NULL;
    current_page_ = NULL;
  } else if (event == kStoreBufferScanningPageEvent) {
    if (current_page_ != NULL) {
      // If this page already overflowed the store buffer during this iteration.
      if (current_page_->scan_on_scavenge()) {
        // Then we should wipe out the entries that have been added for it.
        store_buffer_->SetTop(start_of_current_page_);
      } else if (store_buffer_->Top() - start_of_current_page_ >=
                 (store_buffer_->Limit() - store_buffer_->Top()) >> 2) {
        // Did we find too many pointers in the previous page?  The heuristic is
        // that no page can take more then 1/5 the remaining slots in the store
        // buffer.
        current_page_->set_scan_on_scavenge(true);
        store_buffer_->SetTop(start_of_current_page_);
      } else {
        // In this case the page we scanned took a reasonable number of slots in
        // the store buffer.  It has now been rehabilitated and is no longer
        // marked scan_on_scavenge.
        ASSERT(!current_page_->scan_on_scavenge());
      }
    }
    start_of_current_page_ = store_buffer_->Top();
    current_page_ = page;
  } else if (event == kStoreBufferFullEvent) {
    // The current page overflowed the store buffer again.  Wipe out its entries
    // in the store buffer and mark it scan-on-scavenge again.  This may happen
    // several times while scanning.
    if (current_page_ == NULL) {
      // Store Buffer overflowed while scanning promoted objects.  These are not
      // in any particular page, though they are likely to be clustered by the
      // allocation routines.
      store_buffer_->EnsureSpace(StoreBuffer::kStoreBufferSize);
    } else {
      // Store Buffer overflowed while scanning a particular old space page for
      // pointers to new space.
      ASSERT(current_page_ == page);
      ASSERT(page != NULL);
      current_page_->set_scan_on_scavenge(true);
      ASSERT(start_of_current_page_ != store_buffer_->Top());
      store_buffer_->SetTop(start_of_current_page_);
    }
  } else {
    UNREACHABLE();
  }
}


void PromotionQueue::Initialize() {
  // Assumes that a NewSpacePage exactly fits a number of promotion queue
  // entries (where each is a pair of intptr_t). This allows us to simplify
  // the test fpr when to switch pages.
  ASSERT((Page::kPageSize - MemoryChunk::kBodyOffset) % (2 * kPointerSize)
         == 0);
  limit_ = reinterpret_cast<intptr_t*>(heap_->new_space()->ToSpaceStart());
  front_ = rear_ =
      reinterpret_cast<intptr_t*>(heap_->new_space()->ToSpaceEnd());
  emergency_stack_ = NULL;
  guard_ = false;
}


void PromotionQueue::RelocateQueueHead() {
  ASSERT(emergency_stack_ == NULL);

  Page* p = Page::FromAllocationTop(reinterpret_cast<Address>(rear_));
  intptr_t* head_start = rear_;
  intptr_t* head_end =
      Min(front_, reinterpret_cast<intptr_t*>(p->area_end()));

  int entries_count =
      static_cast<int>(head_end - head_start) / kEntrySizeInWords;

  emergency_stack_ = new List<Entry>(2 * entries_count);

  while (head_start != head_end) {
    int size = static_cast<int>(*(head_start++));
    HeapObject* obj = reinterpret_cast<HeapObject*>(*(head_start++));
    emergency_stack_->Add(Entry(obj, size));
  }
  rear_ = head_end;
}


class ScavengeWeakObjectRetainer : public WeakObjectRetainer {
 public:
  explicit ScavengeWeakObjectRetainer(Heap* heap) : heap_(heap) { }

  virtual Object* RetainAs(Object* object) {
    if (!heap_->InFromSpace(object)) {
      return object;
    }

    MapWord map_word = HeapObject::cast(object)->map_word();
    if (map_word.IsForwardingAddress()) {
      return map_word.ToForwardingAddress();
    }
    return NULL;
  }

 private:
  Heap* heap_;
};


void Heap::Scavenge() {
  RelocationLock relocation_lock(this);

#ifdef VERIFY_HEAP
  if (FLAG_verify_heap) VerifyNonPointerSpacePointers();
#endif

  gc_state_ = SCAVENGE;

  // Implements Cheney's copying algorithm
  LOG(isolate_, ResourceEvent("scavenge", "begin"));

  // Clear descriptor cache.
  isolate_->descriptor_lookup_cache()->Clear();

  // Used for updating survived_since_last_expansion_ at function end.
  intptr_t survived_watermark = PromotedSpaceSizeOfObjects();

  CheckNewSpaceExpansionCriteria();

  SelectScavengingVisitorsTable();

  incremental_marking()->PrepareForScavenge();

  AdvanceSweepers(static_cast<int>(new_space_.Size()));

  // Flip the semispaces.  After flipping, to space is empty, from space has
  // live objects.
  new_space_.Flip();
  new_space_.ResetAllocationInfo();

  // We need to sweep newly copied objects which can be either in the
  // to space or promoted to the old generation.  For to-space
  // objects, we treat the bottom of the to space as a queue.  Newly
  // copied and unswept objects lie between a 'front' mark and the
  // allocation pointer.
  //
  // Promoted objects can go into various old-generation spaces, and
  // can be allocated internally in the spaces (from the free list).
  // We treat the top of the to space as a queue of addresses of
  // promoted objects.  The addresses of newly promoted and unswept
  // objects lie between a 'front' mark and a 'rear' mark that is
  // updated as a side effect of promoting an object.
  //
  // There is guaranteed to be enough room at the top of the to space
  // for the addresses of promoted objects: every object promoted
  // frees up its size in bytes from the top of the new space, and
  // objects are at least one pointer in size.
  Address new_space_front = new_space_.ToSpaceStart();
  promotion_queue_.Initialize();

#ifdef DEBUG
  store_buffer()->Clean();
#endif

  ScavengeVisitor scavenge_visitor(this);
  // Copy roots.
  IterateRoots(&scavenge_visitor, VISIT_ALL_IN_SCAVENGE);

  // Copy objects reachable from the old generation.
  {
    StoreBufferRebuildScope scope(this,
                                  store_buffer(),
                                  &ScavengeStoreBufferCallback);
    store_buffer()->IteratePointersToNewSpace(&ScavengeObject);
  }

  // Copy objects reachable from cells by scavenging cell values directly.
  HeapObjectIterator cell_iterator(cell_space_);
  for (HeapObject* heap_object = cell_iterator.Next();
       heap_object != NULL;
       heap_object = cell_iterator.Next()) {
    if (heap_object->IsJSGlobalPropertyCell()) {
      JSGlobalPropertyCell* cell = JSGlobalPropertyCell::cast(heap_object);
      Address value_address = cell->ValueAddress();
      scavenge_visitor.VisitPointer(reinterpret_cast<Object**>(value_address));
    }
  }

  // Scavenge object reachable from the native contexts list directly.
  scavenge_visitor.VisitPointer(BitCast<Object**>(&native_contexts_list_));

  new_space_front = DoScavenge(&scavenge_visitor, new_space_front);
  isolate_->global_handles()->IdentifyNewSpaceWeakIndependentHandles(
      &IsUnscavengedHeapObject);
  isolate_->global_handles()->IterateNewSpaceWeakIndependentRoots(
      &scavenge_visitor);
  new_space_front = DoScavenge(&scavenge_visitor, new_space_front);

  UpdateNewSpaceReferencesInExternalStringTable(
      &UpdateNewSpaceReferenceInExternalStringTableEntry);

  promotion_queue_.Destroy();

  LiveObjectList::UpdateReferencesForScavengeGC();
  if (!FLAG_watch_ic_patching) {
    isolate()->runtime_profiler()->UpdateSamplesAfterScavenge();
  }
  incremental_marking()->UpdateMarkingDequeAfterScavenge();

  ScavengeWeakObjectRetainer weak_object_retainer(this);
  ProcessWeakReferences(&weak_object_retainer);

  ASSERT(new_space_front == new_space_.top());

  // Set age mark.
  new_space_.set_age_mark(new_space_.top());

  new_space_.LowerInlineAllocationLimit(
      new_space_.inline_allocation_limit_step());

  // Update how much has survived scavenge.
  IncrementYoungSurvivorsCounter(static_cast<int>(
      (PromotedSpaceSizeOfObjects() - survived_watermark) + new_space_.Size()));

  LOG(isolate_, ResourceEvent("scavenge", "end"));

  gc_state_ = NOT_IN_GC;

  scavenges_since_last_idle_round_++;
}


String* Heap::UpdateNewSpaceReferenceInExternalStringTableEntry(Heap* heap,
                                                                Object** p) {
  MapWord first_word = HeapObject::cast(*p)->map_word();

  if (!first_word.IsForwardingAddress()) {
    // Unreachable external string can be finalized.
    heap->FinalizeExternalString(String::cast(*p));
    return NULL;
  }

  // String is still reachable.
  return String::cast(first_word.ToForwardingAddress());
}


void Heap::UpdateNewSpaceReferencesInExternalStringTable(
    ExternalStringTableUpdaterCallback updater_func) {
#ifdef VERIFY_HEAP
  if (FLAG_verify_heap) {
    external_string_table_.Verify();
  }
#endif

  if (external_string_table_.new_space_strings_.is_empty()) return;

  Object** start = &external_string_table_.new_space_strings_[0];
  Object** end = start + external_string_table_.new_space_strings_.length();
  Object** last = start;

  for (Object** p = start; p < end; ++p) {
    ASSERT(InFromSpace(*p));
    String* target = updater_func(this, p);

    if (target == NULL) continue;

    ASSERT(target->IsExternalString());

    if (InNewSpace(target)) {
      // String is still in new space.  Update the table entry.
      *last = target;
      ++last;
    } else {
      // String got promoted.  Move it to the old string list.
      external_string_table_.AddOldString(target);
    }
  }

  ASSERT(last <= end);
  external_string_table_.ShrinkNewStrings(static_cast<int>(last - start));
}


void Heap::UpdateReferencesInExternalStringTable(
    ExternalStringTableUpdaterCallback updater_func) {

  // Update old space string references.
  if (external_string_table_.old_space_strings_.length() > 0) {
    Object** start = &external_string_table_.old_space_strings_[0];
    Object** end = start + external_string_table_.old_space_strings_.length();
    for (Object** p = start; p < end; ++p) *p = updater_func(this, p);
  }

  UpdateNewSpaceReferencesInExternalStringTable(updater_func);
}


static Object* ProcessFunctionWeakReferences(Heap* heap,
                                             Object* function,
                                             WeakObjectRetainer* retainer,
                                             bool record_slots) {
  Object* undefined = heap->undefined_value();
  Object* head = undefined;
  JSFunction* tail = NULL;
  Object* candidate = function;
  while (candidate != undefined) {
    // Check whether to keep the candidate in the list.
    JSFunction* candidate_function = reinterpret_cast<JSFunction*>(candidate);
    Object* retain = retainer->RetainAs(candidate);
    if (retain != NULL) {
      if (head == undefined) {
        // First element in the list.
        head = retain;
      } else {
        // Subsequent elements in the list.
        ASSERT(tail != NULL);
        tail->set_next_function_link(retain);
        if (record_slots) {
          Object** next_function =
              HeapObject::RawField(tail, JSFunction::kNextFunctionLinkOffset);
          heap->mark_compact_collector()->RecordSlot(
              next_function, next_function, retain);
        }
      }
      // Retained function is new tail.
      candidate_function = reinterpret_cast<JSFunction*>(retain);
      tail = candidate_function;

      ASSERT(retain->IsUndefined() || retain->IsJSFunction());

      if (retain == undefined) break;
    }

    // Move to next element in the list.
    candidate = candidate_function->next_function_link();
  }

  // Terminate the list if there is one or more elements.
  if (tail != NULL) {
    tail->set_next_function_link(undefined);
  }

  return head;
}


void Heap::ProcessWeakReferences(WeakObjectRetainer* retainer) {
  Object* undefined = undefined_value();
  Object* head = undefined;
  Context* tail = NULL;
  Object* candidate = native_contexts_list_;

  // We don't record weak slots during marking or scavenges.
  // Instead we do it once when we complete mark-compact cycle.
  // Note that write barrier has no effect if we are already in the middle of
  // compacting mark-sweep cycle and we have to record slots manually.
  bool record_slots =
      gc_state() == MARK_COMPACT &&
      mark_compact_collector()->is_compacting();

  while (candidate != undefined) {
    // Check whether to keep the candidate in the list.
    Context* candidate_context = reinterpret_cast<Context*>(candidate);
    Object* retain = retainer->RetainAs(candidate);
    if (retain != NULL) {
      if (head == undefined) {
        // First element in the list.
        head = retain;
      } else {
        // Subsequent elements in the list.
        ASSERT(tail != NULL);
        tail->set_unchecked(this,
                            Context::NEXT_CONTEXT_LINK,
                            retain,
                            UPDATE_WRITE_BARRIER);

        if (record_slots) {
          Object** next_context =
              HeapObject::RawField(
                  tail, FixedArray::SizeFor(Context::NEXT_CONTEXT_LINK));
          mark_compact_collector()->RecordSlot(
              next_context, next_context, retain);
        }
      }
      // Retained context is new tail.
      candidate_context = reinterpret_cast<Context*>(retain);
      tail = candidate_context;

      if (retain == undefined) break;

      // Process the weak list of optimized functions for the context.
      Object* function_list_head =
          ProcessFunctionWeakReferences(
              this,
              candidate_context->get(Context::OPTIMIZED_FUNCTIONS_LIST),
              retainer,
              record_slots);
      candidate_context->set_unchecked(this,
                                       Context::OPTIMIZED_FUNCTIONS_LIST,
                                       function_list_head,
                                       UPDATE_WRITE_BARRIER);
      if (record_slots) {
        Object** optimized_functions =
            HeapObject::RawField(
                tail, FixedArray::SizeFor(Context::OPTIMIZED_FUNCTIONS_LIST));
        mark_compact_collector()->RecordSlot(
            optimized_functions, optimized_functions, function_list_head);
      }
    }

    // Move to next element in the list.
    candidate = candidate_context->get(Context::NEXT_CONTEXT_LINK);
  }

  // Terminate the list if there is one or more elements.
  if (tail != NULL) {
    tail->set_unchecked(this,
                        Context::NEXT_CONTEXT_LINK,
                        Heap::undefined_value(),
                        UPDATE_WRITE_BARRIER);
  }

  // Update the head of the list of contexts.
  native_contexts_list_ = head;
}


void Heap::VisitExternalResources(v8::ExternalResourceVisitor* visitor) {
  AssertNoAllocation no_allocation;

  class VisitorAdapter : public ObjectVisitor {
   public:
    explicit VisitorAdapter(v8::ExternalResourceVisitor* visitor)
        : visitor_(visitor) {}
    virtual void VisitPointers(Object** start, Object** end) {
      for (Object** p = start; p < end; p++) {
        if ((*p)->IsExternalString()) {
          visitor_->VisitExternalString(Utils::ToLocal(
              Handle<String>(String::cast(*p))));
        }
      }
    }
   private:
    v8::ExternalResourceVisitor* visitor_;
  } visitor_adapter(visitor);
  external_string_table_.Iterate(&visitor_adapter);
}


class NewSpaceScavenger : public StaticNewSpaceVisitor<NewSpaceScavenger> {
 public:
  static inline void VisitPointer(Heap* heap, Object** p) {
    Object* object = *p;
    if (!heap->InNewSpace(object)) return;
    Heap::ScavengeObject(reinterpret_cast<HeapObject**>(p),
                         reinterpret_cast<HeapObject*>(object));
  }
};


Address Heap::DoScavenge(ObjectVisitor* scavenge_visitor,
                         Address new_space_front) {
  do {
    SemiSpace::AssertValidRange(new_space_front, new_space_.top());
    // The addresses new_space_front and new_space_.top() define a
    // queue of unprocessed copied objects.  Process them until the
    // queue is empty.
    while (new_space_front != new_space_.top()) {
      if (!NewSpacePage::IsAtEnd(new_space_front)) {
        HeapObject* object = HeapObject::FromAddress(new_space_front);
        new_space_front +=
          NewSpaceScavenger::IterateBody(object->map(), object);
      } else {
        new_space_front =
            NewSpacePage::FromLimit(new_space_front)->next_page()->area_start();
      }
    }

    // Promote and process all the to-be-promoted objects.
    {
      StoreBufferRebuildScope scope(this,
                                    store_buffer(),
                                    &ScavengeStoreBufferCallback);
      while (!promotion_queue()->is_empty()) {
        HeapObject* target;
        int size;
        promotion_queue()->remove(&target, &size);

        // Promoted object might be already partially visited
        // during old space pointer iteration. Thus we search specificly
        // for pointers to from semispace instead of looking for pointers
        // to new space.
        ASSERT(!target->IsMap());
        IterateAndMarkPointersToFromSpace(target->address(),
                                          target->address() + size,
                                          &ScavengeObject);
      }
    }

    // Take another spin if there are now unswept objects in new space
    // (there are currently no more unswept promoted objects).
  } while (new_space_front != new_space_.top());

  return new_space_front;
}


STATIC_ASSERT((FixedDoubleArray::kHeaderSize & kDoubleAlignmentMask) == 0);


INLINE(static HeapObject* EnsureDoubleAligned(Heap* heap,
                                              HeapObject* object,
                                              int size));

static HeapObject* EnsureDoubleAligned(Heap* heap,
                                       HeapObject* object,
                                       int size) {
  if ((OffsetFrom(object->address()) & kDoubleAlignmentMask) != 0) {
    heap->CreateFillerObjectAt(object->address(), kPointerSize);
    return HeapObject::FromAddress(object->address() + kPointerSize);
  } else {
    heap->CreateFillerObjectAt(object->address() + size - kPointerSize,
                               kPointerSize);
    return object;
  }
}


enum LoggingAndProfiling {
  LOGGING_AND_PROFILING_ENABLED,
  LOGGING_AND_PROFILING_DISABLED
};


enum MarksHandling { TRANSFER_MARKS, IGNORE_MARKS };


template<MarksHandling marks_handling,
         LoggingAndProfiling logging_and_profiling_mode>
class ScavengingVisitor : public StaticVisitorBase {
 public:
  static void Initialize() {
    table_.Register(kVisitSeqAsciiString, &EvacuateSeqAsciiString);
    table_.Register(kVisitSeqTwoByteString, &EvacuateSeqTwoByteString);
    table_.Register(kVisitShortcutCandidate, &EvacuateShortcutCandidate);
    table_.Register(kVisitByteArray, &EvacuateByteArray);
    table_.Register(kVisitFixedArray, &EvacuateFixedArray);
    table_.Register(kVisitFixedDoubleArray, &EvacuateFixedDoubleArray);

    table_.Register(kVisitNativeContext,
                    &ObjectEvacuationStrategy<POINTER_OBJECT>::
                        template VisitSpecialized<Context::kSize>);

    table_.Register(kVisitConsString,
                    &ObjectEvacuationStrategy<POINTER_OBJECT>::
                        template VisitSpecialized<ConsString::kSize>);

    table_.Register(kVisitSlicedString,
                    &ObjectEvacuationStrategy<POINTER_OBJECT>::
                        template VisitSpecialized<SlicedString::kSize>);

    table_.Register(kVisitSharedFunctionInfo,
                    &ObjectEvacuationStrategy<POINTER_OBJECT>::
                        template VisitSpecialized<SharedFunctionInfo::kSize>);

    table_.Register(kVisitJSWeakMap,
                    &ObjectEvacuationStrategy<POINTER_OBJECT>::
                    Visit);

    table_.Register(kVisitJSRegExp,
                    &ObjectEvacuationStrategy<POINTER_OBJECT>::
                    Visit);

    if (marks_handling == IGNORE_MARKS) {
      table_.Register(kVisitJSFunction,
                      &ObjectEvacuationStrategy<POINTER_OBJECT>::
                          template VisitSpecialized<JSFunction::kSize>);
    } else {
      table_.Register(kVisitJSFunction, &EvacuateJSFunction);
    }

    table_.RegisterSpecializations<ObjectEvacuationStrategy<DATA_OBJECT>,
                                   kVisitDataObject,
                                   kVisitDataObjectGeneric>();

    table_.RegisterSpecializations<ObjectEvacuationStrategy<POINTER_OBJECT>,
                                   kVisitJSObject,
                                   kVisitJSObjectGeneric>();

    table_.RegisterSpecializations<ObjectEvacuationStrategy<POINTER_OBJECT>,
                                   kVisitStruct,
                                   kVisitStructGeneric>();
  }

  static VisitorDispatchTable<ScavengingCallback>* GetTable() {
    return &table_;
  }

 private:
  enum ObjectContents  { DATA_OBJECT, POINTER_OBJECT };
  enum SizeRestriction { SMALL, UNKNOWN_SIZE };

  static void RecordCopiedObject(Heap* heap, HeapObject* obj) {
    bool should_record = false;
#ifdef DEBUG
    should_record = FLAG_heap_stats;
#endif
    should_record = should_record || FLAG_log_gc;
    if (should_record) {
      if (heap->new_space()->Contains(obj)) {
        heap->new_space()->RecordAllocation(obj);
      } else {
        heap->new_space()->RecordPromotion(obj);
      }
    }
  }

  // Helper function used by CopyObject to copy a source object to an
  // allocated target object and update the forwarding pointer in the source
  // object.  Returns the target object.
  INLINE(static void MigrateObject(Heap* heap,
                                   HeapObject* source,
                                   HeapObject* target,
                                   int size)) {
    // Copy the content of source to target.
    heap->CopyBlock(target->address(), source->address(), size);

    // Set the forwarding address.
    source->set_map_word(MapWord::FromForwardingAddress(target));

    if (logging_and_profiling_mode == LOGGING_AND_PROFILING_ENABLED) {
      // Update NewSpace stats if necessary.
      RecordCopiedObject(heap, target);
      HEAP_PROFILE(heap, ObjectMoveEvent(source->address(), target->address()));
      Isolate* isolate = heap->isolate();
      if (isolate->logger()->is_logging_code_events() ||
          CpuProfiler::is_profiling(isolate)) {
        if (target->IsSharedFunctionInfo()) {
          PROFILE(isolate, SharedFunctionInfoMoveEvent(
              source->address(), target->address()));
        }
      }
    }

    if (marks_handling == TRANSFER_MARKS) {
      if (Marking::TransferColor(source, target)) {
        MemoryChunk::IncrementLiveBytesFromGC(target->address(), size);
      }
    }
  }


  template<ObjectContents object_contents,
           SizeRestriction size_restriction,
           int alignment>
  static inline void EvacuateObject(Map* map,
                                    HeapObject** slot,
                                    HeapObject* object,
                                    int object_size) {
    SLOW_ASSERT((size_restriction != SMALL) ||
                (object_size <= Page::kMaxNonCodeHeapObjectSize));
    SLOW_ASSERT(object->Size() == object_size);

    int allocation_size = object_size;
    if (alignment != kObjectAlignment) {
      ASSERT(alignment == kDoubleAlignment);
      allocation_size += kPointerSize;
    }

    Heap* heap = map->GetHeap();
    if (heap->ShouldBePromoted(object->address(), object_size)) {
      MaybeObject* maybe_result;

      if ((size_restriction != SMALL) &&
          (allocation_size > Page::kMaxNonCodeHeapObjectSize)) {
        maybe_result = heap->lo_space()->AllocateRaw(allocation_size,
                                                     NOT_EXECUTABLE);
      } else {
        if (object_contents == DATA_OBJECT) {
          maybe_result = heap->old_data_space()->AllocateRaw(allocation_size);
        } else {
          maybe_result =
              heap->old_pointer_space()->AllocateRaw(allocation_size);
        }
      }

      Object* result = NULL;  // Initialization to please compiler.
      if (maybe_result->ToObject(&result)) {
        HeapObject* target = HeapObject::cast(result);

        if (alignment != kObjectAlignment) {
          target = EnsureDoubleAligned(heap, target, allocation_size);
        }

        // Order is important: slot might be inside of the target if target
        // was allocated over a dead object and slot comes from the store
        // buffer.
        *slot = target;
        MigrateObject(heap, object, target, object_size);

        if (object_contents == POINTER_OBJECT) {
          if (map->instance_type() == JS_FUNCTION_TYPE) {
            heap->promotion_queue()->insert(
                target, JSFunction::kNonWeakFieldsEndOffset);
          } else {
            heap->promotion_queue()->insert(target, object_size);
          }
        }

        heap->tracer()->increment_promoted_objects_size(object_size);
        return;
      }
    }
    MaybeObject* allocation = heap->new_space()->AllocateRaw(allocation_size);
    heap->promotion_queue()->SetNewLimit(heap->new_space()->top());
    Object* result = allocation->ToObjectUnchecked();
    HeapObject* target = HeapObject::cast(result);

    if (alignment != kObjectAlignment) {
      target = EnsureDoubleAligned(heap, target, allocation_size);
    }

    // Order is important: slot might be inside of the target if target
    // was allocated over a dead object and slot comes from the store
    // buffer.
    *slot = target;
    MigrateObject(heap, object, target, object_size);
    return;
  }


  static inline void EvacuateJSFunction(Map* map,
                                        HeapObject** slot,
                                        HeapObject* object) {
    ObjectEvacuationStrategy<POINTER_OBJECT>::
        template VisitSpecialized<JSFunction::kSize>(map, slot, object);

    HeapObject* target = *slot;
    MarkBit mark_bit = Marking::MarkBitFrom(target);
    if (Marking::IsBlack(mark_bit)) {
      // This object is black and it might not be rescanned by marker.
      // We should explicitly record code entry slot for compaction because
      // promotion queue processing (IterateAndMarkPointersToFromSpace) will
      // miss it as it is not HeapObject-tagged.
      Address code_entry_slot =
          target->address() + JSFunction::kCodeEntryOffset;
      Code* code = Code::cast(Code::GetObjectFromEntryAddress(code_entry_slot));
      map->GetHeap()->mark_compact_collector()->
          RecordCodeEntrySlot(code_entry_slot, code);
    }
  }


  static inline void EvacuateFixedArray(Map* map,
                                        HeapObject** slot,
                                        HeapObject* object) {
    int object_size = FixedArray::BodyDescriptor::SizeOf(map, object);
    EvacuateObject<POINTER_OBJECT, UNKNOWN_SIZE, kObjectAlignment>(map,
                                                 slot,
                                                 object,
                                                 object_size);
  }


  static inline void EvacuateFixedDoubleArray(Map* map,
                                              HeapObject** slot,
                                              HeapObject* object) {
    int length = reinterpret_cast<FixedDoubleArray*>(object)->length();
    int object_size = FixedDoubleArray::SizeFor(length);
    EvacuateObject<DATA_OBJECT, UNKNOWN_SIZE, kDoubleAlignment>(
        map,
        slot,
        object,
        object_size);
  }


  static inline void EvacuateByteArray(Map* map,
                                       HeapObject** slot,
                                       HeapObject* object) {
    int object_size = reinterpret_cast<ByteArray*>(object)->ByteArraySize();
    EvacuateObject<DATA_OBJECT, UNKNOWN_SIZE, kObjectAlignment>(
        map, slot, object, object_size);
  }


  static inline void EvacuateSeqAsciiString(Map* map,
                                            HeapObject** slot,
                                            HeapObject* object) {
    int object_size = SeqAsciiString::cast(object)->
        SeqAsciiStringSize(map->instance_type());
    EvacuateObject<DATA_OBJECT, UNKNOWN_SIZE, kObjectAlignment>(
        map, slot, object, object_size);
  }


  static inline void EvacuateSeqTwoByteString(Map* map,
                                              HeapObject** slot,
                                              HeapObject* object) {
    int object_size = SeqTwoByteString::cast(object)->
        SeqTwoByteStringSize(map->instance_type());
    EvacuateObject<DATA_OBJECT, UNKNOWN_SIZE, kObjectAlignment>(
        map, slot, object, object_size);
  }


  static inline bool IsShortcutCandidate(int type) {
    return ((type & kShortcutTypeMask) == kShortcutTypeTag);
  }

  static inline void EvacuateShortcutCandidate(Map* map,
                                               HeapObject** slot,
                                               HeapObject* object) {
    ASSERT(IsShortcutCandidate(map->instance_type()));

    Heap* heap = map->GetHeap();

    if (marks_handling == IGNORE_MARKS &&
        ConsString::cast(object)->unchecked_second() ==
        heap->empty_string()) {
      HeapObject* first =
          HeapObject::cast(ConsString::cast(object)->unchecked_first());

      *slot = first;

      if (!heap->InNewSpace(first)) {
        object->set_map_word(MapWord::FromForwardingAddress(first));
        return;
      }

      MapWord first_word = first->map_word();
      if (first_word.IsForwardingAddress()) {
        HeapObject* target = first_word.ToForwardingAddress();

        *slot = target;
        object->set_map_word(MapWord::FromForwardingAddress(target));
        return;
      }

      heap->DoScavengeObject(first->map(), slot, first);
      object->set_map_word(MapWord::FromForwardingAddress(*slot));
      return;
    }

    int object_size = ConsString::kSize;
    EvacuateObject<POINTER_OBJECT, SMALL, kObjectAlignment>(
        map, slot, object, object_size);
  }

  template<ObjectContents object_contents>
  class ObjectEvacuationStrategy {
   public:
    template<int object_size>
    static inline void VisitSpecialized(Map* map,
                                        HeapObject** slot,
                                        HeapObject* object) {
      EvacuateObject<object_contents, SMALL, kObjectAlignment>(
          map, slot, object, object_size);
    }

    static inline void Visit(Map* map,
                             HeapObject** slot,
                             HeapObject* object) {
      int object_size = map->instance_size();
      EvacuateObject<object_contents, SMALL, kObjectAlignment>(
          map, slot, object, object_size);
    }
  };

  static VisitorDispatchTable<ScavengingCallback> table_;
};


template<MarksHandling marks_handling,
         LoggingAndProfiling logging_and_profiling_mode>
VisitorDispatchTable<ScavengingCallback>
    ScavengingVisitor<marks_handling, logging_and_profiling_mode>::table_;


static void InitializeScavengingVisitorsTables() {
  ScavengingVisitor<TRANSFER_MARKS,
                    LOGGING_AND_PROFILING_DISABLED>::Initialize();
  ScavengingVisitor<IGNORE_MARKS, LOGGING_AND_PROFILING_DISABLED>::Initialize();
  ScavengingVisitor<TRANSFER_MARKS,
                    LOGGING_AND_PROFILING_ENABLED>::Initialize();
  ScavengingVisitor<IGNORE_MARKS, LOGGING_AND_PROFILING_ENABLED>::Initialize();
}


void Heap::SelectScavengingVisitorsTable() {
  bool logging_and_profiling =
      isolate()->logger()->is_logging() ||
      CpuProfiler::is_profiling(isolate()) ||
      (isolate()->heap_profiler() != NULL &&
       isolate()->heap_profiler()->is_profiling());

  if (!incremental_marking()->IsMarking()) {
    if (!logging_and_profiling) {
      scavenging_visitors_table_.CopyFrom(
          ScavengingVisitor<IGNORE_MARKS,
                            LOGGING_AND_PROFILING_DISABLED>::GetTable());
    } else {
      scavenging_visitors_table_.CopyFrom(
          ScavengingVisitor<IGNORE_MARKS,
                            LOGGING_AND_PROFILING_ENABLED>::GetTable());
    }
  } else {
    if (!logging_and_profiling) {
      scavenging_visitors_table_.CopyFrom(
          ScavengingVisitor<TRANSFER_MARKS,
                            LOGGING_AND_PROFILING_DISABLED>::GetTable());
    } else {
      scavenging_visitors_table_.CopyFrom(
          ScavengingVisitor<TRANSFER_MARKS,
                            LOGGING_AND_PROFILING_ENABLED>::GetTable());
    }

    if (incremental_marking()->IsCompacting()) {
      // When compacting forbid short-circuiting of cons-strings.
      // Scavenging code relies on the fact that new space object
      // can't be evacuated into evacuation candidate but
      // short-circuiting violates this assumption.
      scavenging_visitors_table_.Register(
          StaticVisitorBase::kVisitShortcutCandidate,
          scavenging_visitors_table_.GetVisitorById(
              StaticVisitorBase::kVisitConsString));
    }
  }
}


void Heap::ScavengeObjectSlow(HeapObject** p, HeapObject* object) {
  SLOW_ASSERT(HEAP->InFromSpace(object));
  MapWord first_word = object->map_word();
  SLOW_ASSERT(!first_word.IsForwardingAddress());
  Map* map = first_word.ToMap();
  map->GetHeap()->DoScavengeObject(map, p, object);
}


MaybeObject* Heap::AllocatePartialMap(InstanceType instance_type,
                                      int instance_size) {
  Object* result;
  MaybeObject* maybe_result = AllocateRawMap();
  if (!maybe_result->ToObject(&result)) return maybe_result;

  // Map::cast cannot be used due to uninitialized map field.
  reinterpret_cast<Map*>(result)->set_map(raw_unchecked_meta_map());
  reinterpret_cast<Map*>(result)->set_instance_type(instance_type);
  reinterpret_cast<Map*>(result)->set_instance_size(instance_size);
  reinterpret_cast<Map*>(result)->set_visitor_id(
        StaticVisitorBase::GetVisitorId(instance_type, instance_size));
  reinterpret_cast<Map*>(result)->set_inobject_properties(0);
  reinterpret_cast<Map*>(result)->set_pre_allocated_property_fields(0);
  reinterpret_cast<Map*>(result)->set_unused_property_fields(0);
  reinterpret_cast<Map*>(result)->set_bit_field(0);
  reinterpret_cast<Map*>(result)->set_bit_field2(0);
  int bit_field3 = Map::EnumLengthBits::encode(Map::kInvalidEnumCache) |
                   Map::OwnsDescriptors::encode(true);
  reinterpret_cast<Map*>(result)->set_bit_field3(bit_field3);
  return result;
}


MaybeObject* Heap::AllocateMap(InstanceType instance_type,
                               int instance_size,
                               ElementsKind elements_kind) {
  Object* result;
  MaybeObject* maybe_result = AllocateRawMap();
  if (!maybe_result->To(&result)) return maybe_result;

  Map* map = reinterpret_cast<Map*>(result);
  map->set_map_no_write_barrier(meta_map());
  map->set_instance_type(instance_type);
  map->set_visitor_id(
      StaticVisitorBase::GetVisitorId(instance_type, instance_size));
  map->set_prototype(null_value(), SKIP_WRITE_BARRIER);
  map->set_constructor(null_value(), SKIP_WRITE_BARRIER);
  map->set_instance_size(instance_size);
  map->set_inobject_properties(0);
  map->set_pre_allocated_property_fields(0);
  map->set_code_cache(empty_fixed_array(), SKIP_WRITE_BARRIER);
  map->init_back_pointer(undefined_value());
  map->set_unused_property_fields(0);
  map->set_instance_descriptors(empty_descriptor_array());
  map->set_bit_field(0);
  map->set_bit_field2(1 << Map::kIsExtensible);
  int bit_field3 = Map::EnumLengthBits::encode(Map::kInvalidEnumCache) |
                   Map::OwnsDescriptors::encode(true);
  map->set_bit_field3(bit_field3);
  map->set_elements_kind(elements_kind);

  return map;
}


MaybeObject* Heap::AllocateCodeCache() {
  CodeCache* code_cache;
  { MaybeObject* maybe_code_cache = AllocateStruct(CODE_CACHE_TYPE);
    if (!maybe_code_cache->To(&code_cache)) return maybe_code_cache;
  }
  code_cache->set_default_cache(empty_fixed_array(), SKIP_WRITE_BARRIER);
  code_cache->set_normal_type_cache(undefined_value(), SKIP_WRITE_BARRIER);
  return code_cache;
}


MaybeObject* Heap::AllocatePolymorphicCodeCache() {
  return AllocateStruct(POLYMORPHIC_CODE_CACHE_TYPE);
}


MaybeObject* Heap::AllocateAccessorPair() {
  AccessorPair* accessors;
  { MaybeObject* maybe_accessors = AllocateStruct(ACCESSOR_PAIR_TYPE);
    if (!maybe_accessors->To(&accessors)) return maybe_accessors;
  }
  accessors->set_getter(the_hole_value(), SKIP_WRITE_BARRIER);
  accessors->set_setter(the_hole_value(), SKIP_WRITE_BARRIER);
  return accessors;
}


MaybeObject* Heap::AllocateTypeFeedbackInfo() {
  TypeFeedbackInfo* info;
  { MaybeObject* maybe_info = AllocateStruct(TYPE_FEEDBACK_INFO_TYPE);
    if (!maybe_info->To(&info)) return maybe_info;
  }
  info->initialize_storage();
  info->set_type_feedback_cells(TypeFeedbackCells::cast(empty_fixed_array()),
                                SKIP_WRITE_BARRIER);
  return info;
}


MaybeObject* Heap::AllocateAliasedArgumentsEntry(int aliased_context_slot) {
  AliasedArgumentsEntry* entry;
  { MaybeObject* maybe_entry = AllocateStruct(ALIASED_ARGUMENTS_ENTRY_TYPE);
    if (!maybe_entry->To(&entry)) return maybe_entry;
  }
  entry->set_aliased_context_slot(aliased_context_slot);
  return entry;
}


const Heap::StringTypeTable Heap::string_type_table[] = {
#define STRING_TYPE_ELEMENT(type, size, name, camel_name)                      \
  {type, size, k##camel_name##MapRootIndex},
  STRING_TYPE_LIST(STRING_TYPE_ELEMENT)
#undef STRING_TYPE_ELEMENT
};


const Heap::ConstantSymbolTable Heap::constant_symbol_table[] = {
#define CONSTANT_SYMBOL_ELEMENT(name, contents)                                \
  {contents, k##name##RootIndex},
  SYMBOL_LIST(CONSTANT_SYMBOL_ELEMENT)
#undef CONSTANT_SYMBOL_ELEMENT
};


const Heap::StructTable Heap::struct_table[] = {
#define STRUCT_TABLE_ELEMENT(NAME, Name, name)                                 \
  { NAME##_TYPE, Name::kSize, k##Name##MapRootIndex },
  STRUCT_LIST(STRUCT_TABLE_ELEMENT)
#undef STRUCT_TABLE_ELEMENT
};


bool Heap::CreateInitialMaps() {
  Object* obj;
  { MaybeObject* maybe_obj = AllocatePartialMap(MAP_TYPE, Map::kSize);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  // Map::cast cannot be used due to uninitialized map field.
  Map* new_meta_map = reinterpret_cast<Map*>(obj);
  set_meta_map(new_meta_map);
  new_meta_map->set_map(new_meta_map);

  { MaybeObject* maybe_obj =
        AllocatePartialMap(FIXED_ARRAY_TYPE, kVariableSizeSentinel);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_fixed_array_map(Map::cast(obj));

  { MaybeObject* maybe_obj = AllocatePartialMap(ODDBALL_TYPE, Oddball::kSize);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_oddball_map(Map::cast(obj));

  // Allocate the empty array.
  { MaybeObject* maybe_obj = AllocateEmptyFixedArray();
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_empty_fixed_array(FixedArray::cast(obj));

  { MaybeObject* maybe_obj = Allocate(oddball_map(), OLD_POINTER_SPACE);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_null_value(Oddball::cast(obj));
  Oddball::cast(obj)->set_kind(Oddball::kNull);

  { MaybeObject* maybe_obj = Allocate(oddball_map(), OLD_POINTER_SPACE);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_undefined_value(Oddball::cast(obj));
  Oddball::cast(obj)->set_kind(Oddball::kUndefined);
  ASSERT(!InNewSpace(undefined_value()));

  // Allocate the empty descriptor array.
  { MaybeObject* maybe_obj = AllocateEmptyFixedArray();
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_empty_descriptor_array(DescriptorArray::cast(obj));

  // Fix the instance_descriptors for the existing maps.
  meta_map()->set_code_cache(empty_fixed_array());
  meta_map()->init_back_pointer(undefined_value());
  meta_map()->set_instance_descriptors(empty_descriptor_array());

  fixed_array_map()->set_code_cache(empty_fixed_array());
  fixed_array_map()->init_back_pointer(undefined_value());
  fixed_array_map()->set_instance_descriptors(empty_descriptor_array());

  oddball_map()->set_code_cache(empty_fixed_array());
  oddball_map()->init_back_pointer(undefined_value());
  oddball_map()->set_instance_descriptors(empty_descriptor_array());

  // Fix prototype object for existing maps.
  meta_map()->set_prototype(null_value());
  meta_map()->set_constructor(null_value());

  fixed_array_map()->set_prototype(null_value());
  fixed_array_map()->set_constructor(null_value());

  oddball_map()->set_prototype(null_value());
  oddball_map()->set_constructor(null_value());

  { MaybeObject* maybe_obj =
        AllocateMap(FIXED_ARRAY_TYPE, kVariableSizeSentinel);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_fixed_cow_array_map(Map::cast(obj));
  ASSERT(fixed_array_map() != fixed_cow_array_map());

  { MaybeObject* maybe_obj =
        AllocateMap(FIXED_ARRAY_TYPE, kVariableSizeSentinel);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_scope_info_map(Map::cast(obj));

  { MaybeObject* maybe_obj = AllocateMap(HEAP_NUMBER_TYPE, HeapNumber::kSize);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_heap_number_map(Map::cast(obj));

  { MaybeObject* maybe_obj = AllocateMap(FOREIGN_TYPE, Foreign::kSize);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_foreign_map(Map::cast(obj));

  for (unsigned i = 0; i < ARRAY_SIZE(string_type_table); i++) {
    const StringTypeTable& entry = string_type_table[i];
    { MaybeObject* maybe_obj = AllocateMap(entry.type, entry.size);
      if (!maybe_obj->ToObject(&obj)) return false;
    }
    roots_[entry.index] = Map::cast(obj);
  }

  { MaybeObject* maybe_obj = AllocateMap(STRING_TYPE, kVariableSizeSentinel);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_undetectable_string_map(Map::cast(obj));
  Map::cast(obj)->set_is_undetectable();

  { MaybeObject* maybe_obj =
        AllocateMap(ASCII_STRING_TYPE, kVariableSizeSentinel);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_undetectable_ascii_string_map(Map::cast(obj));
  Map::cast(obj)->set_is_undetectable();

  { MaybeObject* maybe_obj =
        AllocateMap(FIXED_DOUBLE_ARRAY_TYPE, kVariableSizeSentinel);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_fixed_double_array_map(Map::cast(obj));

  { MaybeObject* maybe_obj =
        AllocateMap(BYTE_ARRAY_TYPE, kVariableSizeSentinel);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_byte_array_map(Map::cast(obj));

  { MaybeObject* maybe_obj =
        AllocateMap(FREE_SPACE_TYPE, kVariableSizeSentinel);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_free_space_map(Map::cast(obj));

  { MaybeObject* maybe_obj = AllocateByteArray(0, TENURED);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_empty_byte_array(ByteArray::cast(obj));

  { MaybeObject* maybe_obj =
        AllocateMap(EXTERNAL_PIXEL_ARRAY_TYPE, ExternalArray::kAlignedSize);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_external_pixel_array_map(Map::cast(obj));

  { MaybeObject* maybe_obj = AllocateMap(EXTERNAL_BYTE_ARRAY_TYPE,
                                         ExternalArray::kAlignedSize);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_external_byte_array_map(Map::cast(obj));

  { MaybeObject* maybe_obj = AllocateMap(EXTERNAL_UNSIGNED_BYTE_ARRAY_TYPE,
                                         ExternalArray::kAlignedSize);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_external_unsigned_byte_array_map(Map::cast(obj));

  { MaybeObject* maybe_obj = AllocateMap(EXTERNAL_SHORT_ARRAY_TYPE,
                                         ExternalArray::kAlignedSize);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_external_short_array_map(Map::cast(obj));

  { MaybeObject* maybe_obj = AllocateMap(EXTERNAL_UNSIGNED_SHORT_ARRAY_TYPE,
                                         ExternalArray::kAlignedSize);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_external_unsigned_short_array_map(Map::cast(obj));

  { MaybeObject* maybe_obj = AllocateMap(EXTERNAL_INT_ARRAY_TYPE,
                                         ExternalArray::kAlignedSize);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_external_int_array_map(Map::cast(obj));

  { MaybeObject* maybe_obj = AllocateMap(EXTERNAL_UNSIGNED_INT_ARRAY_TYPE,
                                         ExternalArray::kAlignedSize);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_external_unsigned_int_array_map(Map::cast(obj));

  { MaybeObject* maybe_obj = AllocateMap(EXTERNAL_FLOAT_ARRAY_TYPE,
                                         ExternalArray::kAlignedSize);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_external_float_array_map(Map::cast(obj));

  { MaybeObject* maybe_obj =
        AllocateMap(FIXED_ARRAY_TYPE, kVariableSizeSentinel);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_non_strict_arguments_elements_map(Map::cast(obj));

  { MaybeObject* maybe_obj = AllocateMap(EXTERNAL_DOUBLE_ARRAY_TYPE,
                                         ExternalArray::kAlignedSize);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_external_double_array_map(Map::cast(obj));

  { MaybeObject* maybe_obj = AllocateMap(CODE_TYPE, kVariableSizeSentinel);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_code_map(Map::cast(obj));

  { MaybeObject* maybe_obj = AllocateMap(JS_GLOBAL_PROPERTY_CELL_TYPE,
                                         JSGlobalPropertyCell::kSize);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_global_property_cell_map(Map::cast(obj));

  { MaybeObject* maybe_obj = AllocateMap(FILLER_TYPE, kPointerSize);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_one_pointer_filler_map(Map::cast(obj));

  { MaybeObject* maybe_obj = AllocateMap(FILLER_TYPE, 2 * kPointerSize);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_two_pointer_filler_map(Map::cast(obj));

  for (unsigned i = 0; i < ARRAY_SIZE(struct_table); i++) {
    const StructTable& entry = struct_table[i];
    { MaybeObject* maybe_obj = AllocateMap(entry.type, entry.size);
      if (!maybe_obj->ToObject(&obj)) return false;
    }
    roots_[entry.index] = Map::cast(obj);
  }

  { MaybeObject* maybe_obj =
        AllocateMap(FIXED_ARRAY_TYPE, kVariableSizeSentinel);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_hash_table_map(Map::cast(obj));

  { MaybeObject* maybe_obj =
        AllocateMap(FIXED_ARRAY_TYPE, kVariableSizeSentinel);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_function_context_map(Map::cast(obj));

  { MaybeObject* maybe_obj =
        AllocateMap(FIXED_ARRAY_TYPE, kVariableSizeSentinel);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_catch_context_map(Map::cast(obj));

  { MaybeObject* maybe_obj =
        AllocateMap(FIXED_ARRAY_TYPE, kVariableSizeSentinel);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_with_context_map(Map::cast(obj));

  { MaybeObject* maybe_obj =
        AllocateMap(FIXED_ARRAY_TYPE, kVariableSizeSentinel);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_block_context_map(Map::cast(obj));

  { MaybeObject* maybe_obj =
        AllocateMap(FIXED_ARRAY_TYPE, kVariableSizeSentinel);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_module_context_map(Map::cast(obj));

  { MaybeObject* maybe_obj =
        AllocateMap(FIXED_ARRAY_TYPE, kVariableSizeSentinel);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_global_context_map(Map::cast(obj));

  { MaybeObject* maybe_obj =
        AllocateMap(FIXED_ARRAY_TYPE, kVariableSizeSentinel);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  Map* native_context_map = Map::cast(obj);
  native_context_map->set_dictionary_map(true);
  native_context_map->set_visitor_id(StaticVisitorBase::kVisitNativeContext);
  set_native_context_map(native_context_map);

  { MaybeObject* maybe_obj = AllocateMap(SHARED_FUNCTION_INFO_TYPE,
                                         SharedFunctionInfo::kAlignedSize);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_shared_function_info_map(Map::cast(obj));

  { MaybeObject* maybe_obj = AllocateMap(JS_MESSAGE_OBJECT_TYPE,
                                         JSMessageObject::kSize);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_message_object_map(Map::cast(obj));

  ASSERT(!InNewSpace(empty_fixed_array()));
  return true;
}


MaybeObject* Heap::AllocateHeapNumber(double value, PretenureFlag pretenure) {
  // Statically ensure that it is safe to allocate heap numbers in paged
  // spaces.
  STATIC_ASSERT(HeapNumber::kSize <= Page::kNonCodeObjectAreaSize);
  AllocationSpace space = (pretenure == TENURED) ? OLD_DATA_SPACE : NEW_SPACE;

  Object* result;
  { MaybeObject* maybe_result =
        AllocateRaw(HeapNumber::kSize, space, OLD_DATA_SPACE);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }

  HeapObject::cast(result)->set_map_no_write_barrier(heap_number_map());
  HeapNumber::cast(result)->set_value(value);
  return result;
}


MaybeObject* Heap::AllocateHeapNumber(double value) {
  // Use general version, if we're forced to always allocate.
  if (always_allocate()) return AllocateHeapNumber(value, TENURED);

  // This version of AllocateHeapNumber is optimized for
  // allocation in new space.
  STATIC_ASSERT(HeapNumber::kSize <= Page::kMaxNonCodeHeapObjectSize);
  ASSERT(allocation_allowed_ && gc_state_ == NOT_IN_GC);
  Object* result;
  { MaybeObject* maybe_result = new_space_.AllocateRaw(HeapNumber::kSize);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }
  HeapObject::cast(result)->set_map_no_write_barrier(heap_number_map());
  HeapNumber::cast(result)->set_value(value);
  return result;
}


MaybeObject* Heap::AllocateJSGlobalPropertyCell(Object* value) {
  Object* result;
  { MaybeObject* maybe_result = AllocateRawCell();
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }
  HeapObject::cast(result)->set_map_no_write_barrier(
      global_property_cell_map());
  JSGlobalPropertyCell::cast(result)->set_value(value);
  return result;
}


MaybeObject* Heap::CreateOddball(const char* to_string,
                                 Object* to_number,
                                 byte kind) {
  Object* result;
  { MaybeObject* maybe_result = Allocate(oddball_map(), OLD_POINTER_SPACE);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }
  return Oddball::cast(result)->Initialize(to_string, to_number, kind);
}


bool Heap::CreateApiObjects() {
  Object* obj;

  { MaybeObject* maybe_obj = AllocateMap(JS_OBJECT_TYPE, JSObject::kHeaderSize);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  // Don't use Smi-only elements optimizations for objects with the neander
  // map. There are too many cases where element values are set directly with a
  // bottleneck to trap the Smi-only -> fast elements transition, and there
  // appears to be no benefit for optimize this case.
  Map* new_neander_map = Map::cast(obj);
  new_neander_map->set_elements_kind(TERMINAL_FAST_ELEMENTS_KIND);
  set_neander_map(new_neander_map);

  { MaybeObject* maybe_obj = AllocateJSObjectFromMap(neander_map());
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  Object* elements;
  { MaybeObject* maybe_elements = AllocateFixedArray(2);
    if (!maybe_elements->ToObject(&elements)) return false;
  }
  FixedArray::cast(elements)->set(0, Smi::FromInt(0));
  JSObject::cast(obj)->set_elements(FixedArray::cast(elements));
  set_message_listeners(JSObject::cast(obj));

  return true;
}


void Heap::CreateJSEntryStub() {
  JSEntryStub stub;
  set_js_entry_code(*stub.GetCode());
}


void Heap::CreateJSConstructEntryStub() {
  JSConstructEntryStub stub;
  set_js_construct_entry_code(*stub.GetCode());
}


void Heap::CreateFixedStubs() {
  // Here we create roots for fixed stubs. They are needed at GC
  // for cooking and uncooking (check out frames.cc).
  // The eliminates the need for doing dictionary lookup in the
  // stub cache for these stubs.
  HandleScope scope;
  // gcc-4.4 has problem generating correct code of following snippet:
  // {  JSEntryStub stub;
  //    js_entry_code_ = *stub.GetCode();
  // }
  // {  JSConstructEntryStub stub;
  //    js_construct_entry_code_ = *stub.GetCode();
  // }
  // To workaround the problem, make separate functions without inlining.
  Heap::CreateJSEntryStub();
  Heap::CreateJSConstructEntryStub();

  // Create stubs that should be there, so we don't unexpectedly have to
  // create them if we need them during the creation of another stub.
  // Stub creation mixes raw pointers and handles in an unsafe manner so
  // we cannot create stubs while we are creating stubs.
  CodeStub::GenerateStubsAheadOfTime();
}


bool Heap::CreateInitialObjects() {
  Object* obj;

  // The -0 value must be set before NumberFromDouble works.
  { MaybeObject* maybe_obj = AllocateHeapNumber(-0.0, TENURED);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_minus_zero_value(HeapNumber::cast(obj));
  ASSERT(signbit(minus_zero_value()->Number()) != 0);

  { MaybeObject* maybe_obj = AllocateHeapNumber(OS::nan_value(), TENURED);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_nan_value(HeapNumber::cast(obj));

  { MaybeObject* maybe_obj = AllocateHeapNumber(V8_INFINITY, TENURED);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_infinity_value(HeapNumber::cast(obj));

  // The hole has not been created yet, but we want to put something
  // predictable in the gaps in the symbol table, so lets make that Smi zero.
  set_the_hole_value(reinterpret_cast<Oddball*>(Smi::FromInt(0)));

  // Allocate initial symbol table.
  { MaybeObject* maybe_obj = SymbolTable::Allocate(kInitialSymbolTableSize);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  // Don't use set_symbol_table() due to asserts.
  roots_[kSymbolTableRootIndex] = obj;

  // Finish initializing oddballs after creating symboltable.
  { MaybeObject* maybe_obj =
        undefined_value()->Initialize("undefined",
                                      nan_value(),
                                      Oddball::kUndefined);
    if (!maybe_obj->ToObject(&obj)) return false;
  }

  // Initialize the null_value.
  { MaybeObject* maybe_obj =
        null_value()->Initialize("null", Smi::FromInt(0), Oddball::kNull);
    if (!maybe_obj->ToObject(&obj)) return false;
  }

  { MaybeObject* maybe_obj = CreateOddball("true",
                                           Smi::FromInt(1),
                                           Oddball::kTrue);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_true_value(Oddball::cast(obj));

  { MaybeObject* maybe_obj = CreateOddball("false",
                                           Smi::FromInt(0),
                                           Oddball::kFalse);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_false_value(Oddball::cast(obj));

  { MaybeObject* maybe_obj = CreateOddball("hole",
                                           Smi::FromInt(-1),
                                           Oddball::kTheHole);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_the_hole_value(Oddball::cast(obj));

  { MaybeObject* maybe_obj = CreateOddball("arguments_marker",
                                           Smi::FromInt(-4),
                                           Oddball::kArgumentMarker);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_arguments_marker(Oddball::cast(obj));

  { MaybeObject* maybe_obj = CreateOddball("no_interceptor_result_sentinel",
                                           Smi::FromInt(-2),
                                           Oddball::kOther);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_no_interceptor_result_sentinel(obj);

  { MaybeObject* maybe_obj = CreateOddball("termination_exception",
                                           Smi::FromInt(-3),
                                           Oddball::kOther);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_termination_exception(obj);

  // Allocate the empty string.
  { MaybeObject* maybe_obj = AllocateRawAsciiString(0, TENURED);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_empty_string(String::cast(obj));

  for (unsigned i = 0; i < ARRAY_SIZE(constant_symbol_table); i++) {
    { MaybeObject* maybe_obj =
          LookupAsciiSymbol(constant_symbol_table[i].contents);
      if (!maybe_obj->ToObject(&obj)) return false;
    }
    roots_[constant_symbol_table[i].index] = String::cast(obj);
  }

  // Allocate the hidden symbol which is used to identify the hidden properties
  // in JSObjects. The hash code has a special value so that it will not match
  // the empty string when searching for the property. It cannot be part of the
  // loop above because it needs to be allocated manually with the special
  // hash code in place. The hash code for the hidden_symbol is zero to ensure
  // that it will always be at the first entry in property descriptors.
  { MaybeObject* maybe_obj =
        AllocateSymbol(CStrVector(""), 0, String::kEmptyStringHash);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  hidden_symbol_ = String::cast(obj);

  // Allocate the foreign for __proto__.
  { MaybeObject* maybe_obj =
        AllocateForeign((Address) &Accessors::ObjectPrototype);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_prototype_accessors(Foreign::cast(obj));

  // Allocate the code_stubs dictionary. The initial size is set to avoid
  // expanding the dictionary during bootstrapping.
  { MaybeObject* maybe_obj = UnseededNumberDictionary::Allocate(128);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_code_stubs(UnseededNumberDictionary::cast(obj));


  // Allocate the non_monomorphic_cache used in stub-cache.cc. The initial size
  // is set to avoid expanding the dictionary during bootstrapping.
  { MaybeObject* maybe_obj = UnseededNumberDictionary::Allocate(64);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_non_monomorphic_cache(UnseededNumberDictionary::cast(obj));

  { MaybeObject* maybe_obj = AllocatePolymorphicCodeCache();
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_polymorphic_code_cache(PolymorphicCodeCache::cast(obj));

  set_instanceof_cache_function(Smi::FromInt(0));
  set_instanceof_cache_map(Smi::FromInt(0));
  set_instanceof_cache_answer(Smi::FromInt(0));

  CreateFixedStubs();

  // Allocate the dictionary of intrinsic function names.
  { MaybeObject* maybe_obj = StringDictionary::Allocate(Runtime::kNumFunctions);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  { MaybeObject* maybe_obj = Runtime::InitializeIntrinsicFunctionNames(this,
                                                                       obj);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_intrinsic_function_names(StringDictionary::cast(obj));

  { MaybeObject* maybe_obj = AllocateInitialNumberStringCache();
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_number_string_cache(FixedArray::cast(obj));

  // Allocate cache for single character ASCII strings.
  { MaybeObject* maybe_obj =
        AllocateFixedArray(String::kMaxAsciiCharCode + 1, TENURED);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_single_character_string_cache(FixedArray::cast(obj));

  // Allocate cache for string split.
  { MaybeObject* maybe_obj = AllocateFixedArray(
      RegExpResultsCache::kRegExpResultsCacheSize, TENURED);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_string_split_cache(FixedArray::cast(obj));

  { MaybeObject* maybe_obj = AllocateFixedArray(
      RegExpResultsCache::kRegExpResultsCacheSize, TENURED);
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_regexp_multiple_cache(FixedArray::cast(obj));

  // Allocate cache for external strings pointing to native source code.
  { MaybeObject* maybe_obj = AllocateFixedArray(Natives::GetBuiltinsCount());
    if (!maybe_obj->ToObject(&obj)) return false;
  }
  set_natives_source_cache(FixedArray::cast(obj));

  // Handling of script id generation is in FACTORY->NewScript.
  set_last_script_id(undefined_value());

  // Initialize keyed lookup cache.
  isolate_->keyed_lookup_cache()->Clear();

  // Initialize context slot cache.
  isolate_->context_slot_cache()->Clear();

  // Initialize descriptor cache.
  isolate_->descriptor_lookup_cache()->Clear();

  // Initialize compilation cache.
  isolate_->compilation_cache()->Clear();

  return true;
}


Object* RegExpResultsCache::Lookup(Heap* heap,
                                   String* key_string,
                                   Object* key_pattern,
                                   ResultsCacheType type) {
  FixedArray* cache;
  if (!key_string->IsSymbol()) return Smi::FromInt(0);
  if (type == STRING_SPLIT_SUBSTRINGS) {
    ASSERT(key_pattern->IsString());
    if (!key_pattern->IsSymbol()) return Smi::FromInt(0);
    cache = heap->string_split_cache();
  } else {
    ASSERT(type == REGEXP_MULTIPLE_INDICES);
    ASSERT(key_pattern->IsFixedArray());
    cache = heap->regexp_multiple_cache();
  }

  uint32_t hash = key_string->Hash();
  uint32_t index = ((hash & (kRegExpResultsCacheSize - 1)) &
      ~(kArrayEntriesPerCacheEntry - 1));
  if (cache->get(index + kStringOffset) == key_string &&
      cache->get(index + kPatternOffset) == key_pattern) {
    return cache->get(index + kArrayOffset);
  }
  index =
      ((index + kArrayEntriesPerCacheEntry) & (kRegExpResultsCacheSize - 1));
  if (cache->get(index + kStringOffset) == key_string &&
      cache->get(index + kPatternOffset) == key_pattern) {
    return cache->get(index + kArrayOffset);
  }
  return Smi::FromInt(0);
}


void RegExpResultsCache::Enter(Heap* heap,
                               String* key_string,
                               Object* key_pattern,
                               FixedArray* value_array,
                               ResultsCacheType type) {
  FixedArray* cache;
  if (!key_string->IsSymbol()) return;
  if (type == STRING_SPLIT_SUBSTRINGS) {
    ASSERT(key_pattern->IsString());
    if (!key_pattern->IsSymbol()) return;
    cache = heap->string_split_cache();
  } else {
    ASSERT(type == REGEXP_MULTIPLE_INDICES);
    ASSERT(key_pattern->IsFixedArray());
    cache = heap->regexp_multiple_cache();
  }

  uint32_t hash = key_string->Hash();
  uint32_t index = ((hash & (kRegExpResultsCacheSize - 1)) &
      ~(kArrayEntriesPerCacheEntry - 1));
  if (cache->get(index + kStringOffset) == Smi::FromInt(0)) {
    cache->set(index + kStringOffset, key_string);
    cache->set(index + kPatternOffset, key_pattern);
    cache->set(index + kArrayOffset, value_array);
  } else {
    uint32_t index2 =
        ((index + kArrayEntriesPerCacheEntry) & (kRegExpResultsCacheSize - 1));
    if (cache->get(index2 + kStringOffset) == Smi::FromInt(0)) {
      cache->set(index2 + kStringOffset, key_string);
      cache->set(index2 + kPatternOffset, key_pattern);
      cache->set(index2 + kArrayOffset, value_array);
    } else {
      cache->set(index2 + kStringOffset, Smi::FromInt(0));
      cache->set(index2 + kPatternOffset, Smi::FromInt(0));
      cache->set(index2 + kArrayOffset, Smi::FromInt(0));
      cache->set(index + kStringOffset, key_string);
      cache->set(index + kPatternOffset, key_pattern);
      cache->set(index + kArrayOffset, value_array);
    }
  }
  // If the array is a reasonably short list of substrings, convert it into a
  // list of symbols.
  if (type == STRING_SPLIT_SUBSTRINGS && value_array->length() < 100) {
    for (int i = 0; i < value_array->length(); i++) {
      String* str = String::cast(value_array->get(i));
      Object* symbol;
      MaybeObject* maybe_symbol = heap->LookupSymbol(str);
      if (maybe_symbol->ToObject(&symbol)) {
        value_array->set(i, symbol);
      }
    }
  }
  // Convert backing store to a copy-on-write array.
  value_array->set_map_no_write_barrier(heap->fixed_cow_array_map());
}


void RegExpResultsCache::Clear(FixedArray* cache) {
  for (int i = 0; i < kRegExpResultsCacheSize; i++) {
    cache->set(i, Smi::FromInt(0));
  }
}


MaybeObject* Heap::AllocateInitialNumberStringCache() {
  MaybeObject* maybe_obj =
      AllocateFixedArray(kInitialNumberStringCacheSize * 2, TENURED);
  return maybe_obj;
}


int Heap::FullSizeNumberStringCacheLength() {
  // Compute the size of the number string cache based on the max newspace size.
  // The number string cache has a minimum size based on twice the initial cache
  // size to ensure that it is bigger after being made 'full size'.
  int number_string_cache_size = max_semispace_size_ / 512;
  number_string_cache_size = Max(kInitialNumberStringCacheSize * 2,
                                 Min(0x4000, number_string_cache_size));
  // There is a string and a number per entry so the length is twice the number
  // of entries.
  return number_string_cache_size * 2;
}


void Heap::AllocateFullSizeNumberStringCache() {
  // The idea is to have a small number string cache in the snapshot to keep
  // boot-time memory usage down.  If we expand the number string cache already
  // while creating the snapshot then that didn't work out.
  ASSERT(!Serializer::enabled() || FLAG_extra_code != NULL);
  MaybeObject* maybe_obj =
      AllocateFixedArray(FullSizeNumberStringCacheLength(), TENURED);
  Object* new_cache;
  if (maybe_obj->ToObject(&new_cache)) {
    // We don't bother to repopulate the cache with entries from the old cache.
    // It will be repopulated soon enough with new strings.
    set_number_string_cache(FixedArray::cast(new_cache));
  }
  // If allocation fails then we just return without doing anything.  It is only
  // a cache, so best effort is OK here.
}


void Heap::FlushNumberStringCache() {
  // Flush the number to string cache.
  int len = number_string_cache()->length();
  for (int i = 0; i < len; i++) {
    number_string_cache()->set_undefined(this, i);
  }
}


static inline int double_get_hash(double d) {
  DoubleRepresentation rep(d);
  return static_cast<int>(rep.bits) ^ static_cast<int>(rep.bits >> 32);
}


static inline int smi_get_hash(Smi* smi) {
  return smi->value();
}


Object* Heap::GetNumberStringCache(Object* number) {
  int hash;
  int mask = (number_string_cache()->length() >> 1) - 1;
  if (number->IsSmi()) {
    hash = smi_get_hash(Smi::cast(number)) & mask;
  } else {
    hash = double_get_hash(number->Number()) & mask;
  }
  Object* key = number_string_cache()->get(hash * 2);
  if (key == number) {
    return String::cast(number_string_cache()->get(hash * 2 + 1));
  } else if (key->IsHeapNumber() &&
             number->IsHeapNumber() &&
             key->Number() == number->Number()) {
    return String::cast(number_string_cache()->get(hash * 2 + 1));
  }
  return undefined_value();
}


void Heap::SetNumberStringCache(Object* number, String* string) {
  int hash;
  int mask = (number_string_cache()->length() >> 1) - 1;
  if (number->IsSmi()) {
    hash = smi_get_hash(Smi::cast(number)) & mask;
  } else {
    hash = double_get_hash(number->Number()) & mask;
  }
  if (number_string_cache()->get(hash * 2) != undefined_value() &&
      number_string_cache()->length() != FullSizeNumberStringCacheLength()) {
    // The first time we have a hash collision, we move to the full sized
    // number string cache.
    AllocateFullSizeNumberStringCache();
    return;
  }
  number_string_cache()->set(hash * 2, number);
  number_string_cache()->set(hash * 2 + 1, string);
}


MaybeObject* Heap::NumberToString(Object* number,
                                  bool check_number_string_cache) {
  isolate_->counters()->number_to_string_runtime()->Increment();
  if (check_number_string_cache) {
    Object* cached = GetNumberStringCache(number);
    if (cached != undefined_value()) {
      return cached;
    }
  }

  char arr[100];
  Vector<char> buffer(arr, ARRAY_SIZE(arr));
  const char* str;
  if (number->IsSmi()) {
    int num = Smi::cast(number)->value();
    str = IntToCString(num, buffer);
  } else {
    double num = HeapNumber::cast(number)->value();
    str = DoubleToCString(num, buffer);
  }

  Object* js_string;
  MaybeObject* maybe_js_string = AllocateStringFromAscii(CStrVector(str));
  if (maybe_js_string->ToObject(&js_string)) {
    SetNumberStringCache(number, String::cast(js_string));
  }
  return maybe_js_string;
}


MaybeObject* Heap::Uint32ToString(uint32_t value,
                                  bool check_number_string_cache) {
  Object* number;
  MaybeObject* maybe = NumberFromUint32(value);
  if (!maybe->To<Object>(&number)) return maybe;
  return NumberToString(number, check_number_string_cache);
}


Map* Heap::MapForExternalArrayType(ExternalArrayType array_type) {
  return Map::cast(roots_[RootIndexForExternalArrayType(array_type)]);
}


Heap::RootListIndex Heap::RootIndexForExternalArrayType(
    ExternalArrayType array_type) {
  switch (array_type) {
    case kExternalByteArray:
      return kExternalByteArrayMapRootIndex;
    case kExternalUnsignedByteArray:
      return kExternalUnsignedByteArrayMapRootIndex;
    case kExternalShortArray:
      return kExternalShortArrayMapRootIndex;
    case kExternalUnsignedShortArray:
      return kExternalUnsignedShortArrayMapRootIndex;
    case kExternalIntArray:
      return kExternalIntArrayMapRootIndex;
    case kExternalUnsignedIntArray:
      return kExternalUnsignedIntArrayMapRootIndex;
    case kExternalFloatArray:
      return kExternalFloatArrayMapRootIndex;
    case kExternalDoubleArray:
      return kExternalDoubleArrayMapRootIndex;
    case kExternalPixelArray:
      return kExternalPixelArrayMapRootIndex;
    default:
      UNREACHABLE();
      return kUndefinedValueRootIndex;
  }
}


MaybeObject* Heap::NumberFromDouble(double value, PretenureFlag pretenure) {
  // We need to distinguish the minus zero value and this cannot be
  // done after conversion to int. Doing this by comparing bit
  // patterns is faster than using fpclassify() et al.
  static const DoubleRepresentation minus_zero(-0.0);

  DoubleRepresentation rep(value);
  if (rep.bits == minus_zero.bits) {
    return AllocateHeapNumber(-0.0, pretenure);
  }

  int int_value = FastD2I(value);
  if (value == int_value && Smi::IsValid(int_value)) {
    return Smi::FromInt(int_value);
  }

  // Materialize the value in the heap.
  return AllocateHeapNumber(value, pretenure);
}


MaybeObject* Heap::AllocateForeign(Address address, PretenureFlag pretenure) {
  // Statically ensure that it is safe to allocate foreigns in paged spaces.
  STATIC_ASSERT(Foreign::kSize <= Page::kMaxNonCodeHeapObjectSize);
  AllocationSpace space = (pretenure == TENURED) ? OLD_DATA_SPACE : NEW_SPACE;
  Foreign* result;
  MaybeObject* maybe_result = Allocate(foreign_map(), space);
  if (!maybe_result->To(&result)) return maybe_result;
  result->set_foreign_address(address);
  return result;
}


MaybeObject* Heap::AllocateSharedFunctionInfo(Object* name) {
  SharedFunctionInfo* share;
  MaybeObject* maybe = Allocate(shared_function_info_map(), OLD_POINTER_SPACE);
  if (!maybe->To<SharedFunctionInfo>(&share)) return maybe;

  // Set pointer fields.
  share->set_name(name);
  Code* illegal = isolate_->builtins()->builtin(Builtins::kIllegal);
  share->set_code(illegal);
  share->ClearOptimizedCodeMap();
  share->set_scope_info(ScopeInfo::Empty());
  Code* construct_stub =
      isolate_->builtins()->builtin(Builtins::kJSConstructStubGeneric);
  share->set_construct_stub(construct_stub);
  share->set_instance_class_name(Object_symbol());
  share->set_function_data(undefined_value(), SKIP_WRITE_BARRIER);
  share->set_script(undefined_value(), SKIP_WRITE_BARRIER);
  share->set_debug_info(undefined_value(), SKIP_WRITE_BARRIER);
  share->set_inferred_name(empty_string(), SKIP_WRITE_BARRIER);
  share->set_initial_map(undefined_value(), SKIP_WRITE_BARRIER);
  share->set_this_property_assignments(undefined_value(), SKIP_WRITE_BARRIER);
  share->set_ast_node_count(0);
  share->set_stress_deopt_counter(FLAG_deopt_every_n_times);
  share->set_counters(0);

  // Set integer fields (smi or int, depending on the architecture).
  share->set_length(0);
  share->set_formal_parameter_count(0);
  share->set_expected_nof_properties(0);
  share->set_num_literals(0);
  share->set_start_position_and_type(0);
  share->set_end_position(0);
  share->set_function_token_position(0);
  // All compiler hints default to false or 0.
  share->set_compiler_hints(0);
  share->set_this_property_assignments_count(0);
  share->set_opt_count(0);

  return share;
}


MaybeObject* Heap::AllocateJSMessageObject(String* type,
                                           JSArray* arguments,
                                           int start_position,
                                           int end_position,
                                           Object* script,
                                           Object* stack_trace,
                                           Object* stack_frames) {
  Object* result;
  { MaybeObject* maybe_result = Allocate(message_object_map(), NEW_SPACE);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }
  JSMessageObject* message = JSMessageObject::cast(result);
  message->set_properties(Heap::empty_fixed_array(), SKIP_WRITE_BARRIER);
  message->initialize_elements();
  message->set_elements(Heap::empty_fixed_array(), SKIP_WRITE_BARRIER);
  message->set_type(type);
  message->set_arguments(arguments);
  message->set_start_position(start_position);
  message->set_end_position(end_position);
  message->set_script(script);
  message->set_stack_trace(stack_trace);
  message->set_stack_frames(stack_frames);
  return result;
}



// Returns true for a character in a range.  Both limits are inclusive.
static inline bool Between(uint32_t character, uint32_t from, uint32_t to) {
  // This makes uses of the the unsigned wraparound.
  return character - from <= to - from;
}


MUST_USE_RESULT static inline MaybeObject* MakeOrFindTwoCharacterString(
    Heap* heap,
    uint32_t c1,
    uint32_t c2) {
  String* symbol;
  // Numeric strings have a different hash algorithm not known by
  // LookupTwoCharsSymbolIfExists, so we skip this step for such strings.
  if ((!Between(c1, '0', '9') || !Between(c2, '0', '9')) &&
      heap->symbol_table()->LookupTwoCharsSymbolIfExists(c1, c2, &symbol)) {
    return symbol;
  // Now we know the length is 2, we might as well make use of that fact
  // when building the new string.
  } else if ((c1 | c2) <= String::kMaxAsciiCharCodeU) {  // We can do this
    ASSERT(IsPowerOf2(String::kMaxAsciiCharCodeU + 1));  // because of this.
    Object* result;
    { MaybeObject* maybe_result = heap->AllocateRawAsciiString(2);
      if (!maybe_result->ToObject(&result)) return maybe_result;
    }
    char* dest = SeqAsciiString::cast(result)->GetChars();
    dest[0] = c1;
    dest[1] = c2;
    return result;
  } else {
    Object* result;
    { MaybeObject* maybe_result = heap->AllocateRawTwoByteString(2);
      if (!maybe_result->ToObject(&result)) return maybe_result;
    }
    uc16* dest = SeqTwoByteString::cast(result)->GetChars();
    dest[0] = c1;
    dest[1] = c2;
    return result;
  }
}


MaybeObject* Heap::AllocateConsString(String* first, String* second) {
  int first_length = first->length();
  if (first_length == 0) {
    return second;
  }

  int second_length = second->length();
  if (second_length == 0) {
    return first;
  }

  int length = first_length + second_length;

  // Optimization for 2-byte strings often used as keys in a decompression
  // dictionary.  Check whether we already have the string in the symbol
  // table to prevent creation of many unneccesary strings.
  if (length == 2) {
    unsigned c1 = first->Get(0);
    unsigned c2 = second->Get(0);
    return MakeOrFindTwoCharacterString(this, c1, c2);
  }

  bool first_is_ascii = first->IsAsciiRepresentation();
  bool second_is_ascii = second->IsAsciiRepresentation();
  bool is_ascii = first_is_ascii && second_is_ascii;

  // Make sure that an out of memory exception is thrown if the length
  // of the new cons string is too large.
  if (length > String::kMaxLength || length < 0) {
    isolate()->context()->mark_out_of_memory();
    return Failure::OutOfMemoryException();
  }

  bool is_ascii_data_in_two_byte_string = false;
  if (!is_ascii) {
    // At least one of the strings uses two-byte representation so we
    // can't use the fast case code for short ASCII strings below, but
    // we can try to save memory if all chars actually fit in ASCII.
    is_ascii_data_in_two_byte_string =
        first->HasOnlyAsciiChars() && second->HasOnlyAsciiChars();
    if (is_ascii_data_in_two_byte_string) {
      isolate_->counters()->string_add_runtime_ext_to_ascii()->Increment();
    }
  }

  // If the resulting string is small make a flat string.
  if (length < ConsString::kMinLength) {
    // Note that neither of the two inputs can be a slice because:
    STATIC_ASSERT(ConsString::kMinLength <= SlicedString::kMinLength);
    ASSERT(first->IsFlat());
    ASSERT(second->IsFlat());
    if (is_ascii) {
      Object* result;
      { MaybeObject* maybe_result = AllocateRawAsciiString(length);
        if (!maybe_result->ToObject(&result)) return maybe_result;
      }
      // Copy the characters into the new object.
      char* dest = SeqAsciiString::cast(result)->GetChars();
      // Copy first part.
      const char* src;
      if (first->IsExternalString()) {
        src = ExternalAsciiString::cast(first)->GetChars();
      } else {
        src = SeqAsciiString::cast(first)->GetChars();
      }
      for (int i = 0; i < first_length; i++) *dest++ = src[i];
      // Copy second part.
      if (second->IsExternalString()) {
        src = ExternalAsciiString::cast(second)->GetChars();
      } else {
        src = SeqAsciiString::cast(second)->GetChars();
      }
      for (int i = 0; i < second_length; i++) *dest++ = src[i];
      return result;
    } else {
      if (is_ascii_data_in_two_byte_string) {
        Object* result;
        { MaybeObject* maybe_result = AllocateRawAsciiString(length);
          if (!maybe_result->ToObject(&result)) return maybe_result;
        }
        // Copy the characters into the new object.
        char* dest = SeqAsciiString::cast(result)->GetChars();
        String::WriteToFlat(first, dest, 0, first_length);
        String::WriteToFlat(second, dest + first_length, 0, second_length);
        isolate_->counters()->string_add_runtime_ext_to_ascii()->Increment();
        return result;
      }

      Object* result;
      { MaybeObject* maybe_result = AllocateRawTwoByteString(length);
        if (!maybe_result->ToObject(&result)) return maybe_result;
      }
      // Copy the characters into the new object.
      uc16* dest = SeqTwoByteString::cast(result)->GetChars();
      String::WriteToFlat(first, dest, 0, first_length);
      String::WriteToFlat(second, dest + first_length, 0, second_length);
      return result;
    }
  }

  Map* map = (is_ascii || is_ascii_data_in_two_byte_string) ?
      cons_ascii_string_map() : cons_string_map();

  Object* result;
  { MaybeObject* maybe_result = Allocate(map, NEW_SPACE);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }

  AssertNoAllocation no_gc;
  ConsString* cons_string = ConsString::cast(result);
  WriteBarrierMode mode = cons_string->GetWriteBarrierMode(no_gc);
  cons_string->set_length(length);
  cons_string->set_hash_field(String::kEmptyHashField);
  cons_string->set_first(first, mode);
  cons_string->set_second(second, mode);
  return result;
}


MaybeObject* Heap::AllocateSubString(String* buffer,
                                     int start,
                                     int end,
                                     PretenureFlag pretenure) {
  int length = end - start;
  if (length <= 0) {
    return empty_string();
  } else if (length == 1) {
    return LookupSingleCharacterStringFromCode(buffer->Get(start));
  } else if (length == 2) {
    // Optimization for 2-byte strings often used as keys in a decompression
    // dictionary.  Check whether we already have the string in the symbol
    // table to prevent creation of many unneccesary strings.
    unsigned c1 = buffer->Get(start);
    unsigned c2 = buffer->Get(start + 1);
    return MakeOrFindTwoCharacterString(this, c1, c2);
  }

  // Make an attempt to flatten the buffer to reduce access time.
  buffer = buffer->TryFlattenGetString();

  if (!FLAG_string_slices ||
      !buffer->IsFlat() ||
      length < SlicedString::kMinLength ||
      pretenure == TENURED) {
    Object* result;
    // WriteToFlat takes care of the case when an indirect string has a
    // different encoding from its underlying string.  These encodings may
    // differ because of externalization.
    bool is_ascii = buffer->IsAsciiRepresentation();
    { MaybeObject* maybe_result = is_ascii
                                  ? AllocateRawAsciiString(length, pretenure)
                                  : AllocateRawTwoByteString(length, pretenure);
      if (!maybe_result->ToObject(&result)) return maybe_result;
    }
    String* string_result = String::cast(result);
    // Copy the characters into the new object.
    if (is_ascii) {
      ASSERT(string_result->IsAsciiRepresentation());
      char* dest = SeqAsciiString::cast(string_result)->GetChars();
      String::WriteToFlat(buffer, dest, start, end);
    } else {
      ASSERT(string_result->IsTwoByteRepresentation());
      uc16* dest = SeqTwoByteString::cast(string_result)->GetChars();
      String::WriteToFlat(buffer, dest, start, end);
    }
    return result;
  }

  ASSERT(buffer->IsFlat());
#if VERIFY_HEAP
  if (FLAG_verify_heap) {
    buffer->StringVerify();
  }
#endif

  Object* result;
  // When slicing an indirect string we use its encoding for a newly created
  // slice and don't check the encoding of the underlying string.  This is safe
  // even if the encodings are different because of externalization.  If an
  // indirect ASCII string is pointing to a two-byte string, the two-byte char
  // codes of the underlying string must still fit into ASCII (because
  // externalization must not change char codes).
  { Map* map = buffer->IsAsciiRepresentation()
                 ? sliced_ascii_string_map()
                 : sliced_string_map();
    MaybeObject* maybe_result = Allocate(map, NEW_SPACE);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }

  AssertNoAllocation no_gc;
  SlicedString* sliced_string = SlicedString::cast(result);
  sliced_string->set_length(length);
  sliced_string->set_hash_field(String::kEmptyHashField);
  if (buffer->IsConsString()) {
    ConsString* cons = ConsString::cast(buffer);
    ASSERT(cons->second()->length() == 0);
    sliced_string->set_parent(cons->first());
    sliced_string->set_offset(start);
  } else if (buffer->IsSlicedString()) {
    // Prevent nesting sliced strings.
    SlicedString* parent_slice = SlicedString::cast(buffer);
    sliced_string->set_parent(parent_slice->parent());
    sliced_string->set_offset(start + parent_slice->offset());
  } else {
    sliced_string->set_parent(buffer);
    sliced_string->set_offset(start);
  }
  ASSERT(sliced_string->parent()->IsSeqString() ||
         sliced_string->parent()->IsExternalString());
  return result;
}


MaybeObject* Heap::AllocateExternalStringFromAscii(
    const ExternalAsciiString::Resource* resource) {
  size_t length = resource->length();
  if (length > static_cast<size_t>(String::kMaxLength)) {
    isolate()->context()->mark_out_of_memory();
    return Failure::OutOfMemoryException();
  }

  ASSERT(String::IsAscii(resource->data(), static_cast<int>(length)));

  Map* map = external_ascii_string_map();
  Object* result;
  { MaybeObject* maybe_result = Allocate(map, NEW_SPACE);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }

  ExternalAsciiString* external_string = ExternalAsciiString::cast(result);
  external_string->set_length(static_cast<int>(length));
  external_string->set_hash_field(String::kEmptyHashField);
  external_string->set_resource(resource);

  return result;
}


MaybeObject* Heap::AllocateExternalStringFromTwoByte(
    const ExternalTwoByteString::Resource* resource) {
  size_t length = resource->length();
  if (length > static_cast<size_t>(String::kMaxLength)) {
    isolate()->context()->mark_out_of_memory();
    return Failure::OutOfMemoryException();
  }

  // For small strings we check whether the resource contains only
  // ASCII characters.  If yes, we use a different string map.
  static const size_t kAsciiCheckLengthLimit = 32;
  bool is_ascii = length <= kAsciiCheckLengthLimit &&
      String::IsAscii(resource->data(), static_cast<int>(length));
  Map* map = is_ascii ?
      external_string_with_ascii_data_map() : external_string_map();
  Object* result;
  { MaybeObject* maybe_result = Allocate(map, NEW_SPACE);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }

  ExternalTwoByteString* external_string = ExternalTwoByteString::cast(result);
  external_string->set_length(static_cast<int>(length));
  external_string->set_hash_field(String::kEmptyHashField);
  external_string->set_resource(resource);

  return result;
}


MaybeObject* Heap::LookupSingleCharacterStringFromCode(uint16_t code) {
  if (code <= String::kMaxAsciiCharCode) {
    Object* value = single_character_string_cache()->get(code);
    if (value != undefined_value()) return value;

    char buffer[1];
    buffer[0] = static_cast<char>(code);
    Object* result;
    MaybeObject* maybe_result = LookupSymbol(Vector<const char>(buffer, 1));

    if (!maybe_result->ToObject(&result)) return maybe_result;
    single_character_string_cache()->set(code, result);
    return result;
  }

  Object* result;
  { MaybeObject* maybe_result = AllocateRawTwoByteString(1);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }
  String* answer = String::cast(result);
  answer->Set(0, code);
  return answer;
}


MaybeObject* Heap::AllocateByteArray(int length, PretenureFlag pretenure) {
  if (length < 0 || length > ByteArray::kMaxLength) {
    return Failure::OutOfMemoryException();
  }
  if (pretenure == NOT_TENURED) {
    return AllocateByteArray(length);
  }
  int size = ByteArray::SizeFor(length);
  Object* result;
  { MaybeObject* maybe_result = (size <= Page::kMaxNonCodeHeapObjectSize)
                   ? old_data_space_->AllocateRaw(size)
                   : lo_space_->AllocateRaw(size, NOT_EXECUTABLE);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }

  reinterpret_cast<ByteArray*>(result)->set_map_no_write_barrier(
      byte_array_map());
  reinterpret_cast<ByteArray*>(result)->set_length(length);
  return result;
}


MaybeObject* Heap::AllocateByteArray(int length) {
  if (length < 0 || length > ByteArray::kMaxLength) {
    return Failure::OutOfMemoryException();
  }
  int size = ByteArray::SizeFor(length);
  AllocationSpace space =
      (size > Page::kMaxNonCodeHeapObjectSize) ? LO_SPACE : NEW_SPACE;
  Object* result;
  { MaybeObject* maybe_result = AllocateRaw(size, space, OLD_DATA_SPACE);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }

  reinterpret_cast<ByteArray*>(result)->set_map_no_write_barrier(
      byte_array_map());
  reinterpret_cast<ByteArray*>(result)->set_length(length);
  return result;
}


void Heap::CreateFillerObjectAt(Address addr, int size) {
  if (size == 0) return;
  HeapObject* filler = HeapObject::FromAddress(addr);
  if (size == kPointerSize) {
    filler->set_map_no_write_barrier(one_pointer_filler_map());
  } else if (size == 2 * kPointerSize) {
    filler->set_map_no_write_barrier(two_pointer_filler_map());
  } else {
    filler->set_map_no_write_barrier(free_space_map());
    FreeSpace::cast(filler)->set_size(size);
  }
}


MaybeObject* Heap::AllocateExternalArray(int length,
                                         ExternalArrayType array_type,
                                         void* external_pointer,
                                         PretenureFlag pretenure) {
  AllocationSpace space = (pretenure == TENURED) ? OLD_DATA_SPACE : NEW_SPACE;
  Object* result;
  { MaybeObject* maybe_result = AllocateRaw(ExternalArray::kAlignedSize,
                                            space,
                                            OLD_DATA_SPACE);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }

  reinterpret_cast<ExternalArray*>(result)->set_map_no_write_barrier(
      MapForExternalArrayType(array_type));
  reinterpret_cast<ExternalArray*>(result)->set_length(length);
  reinterpret_cast<ExternalArray*>(result)->set_external_pointer(
      external_pointer);

  return result;
}


MaybeObject* Heap::CreateCode(const CodeDesc& desc,
                              Code::Flags flags,
                              Handle<Object> self_reference,
                              bool immovable) {
  // Allocate ByteArray before the Code object, so that we do not risk
  // leaving uninitialized Code object (and breaking the heap).
  ByteArray* reloc_info;
  MaybeObject* maybe_reloc_info = AllocateByteArray(desc.reloc_size, TENURED);
  if (!maybe_reloc_info->To(&reloc_info)) return maybe_reloc_info;

  // Compute size.
  int body_size = RoundUp(desc.instr_size, kObjectAlignment);
  int obj_size = Code::SizeFor(body_size);
  ASSERT(IsAligned(static_cast<intptr_t>(obj_size), kCodeAlignment));
  MaybeObject* maybe_result;
  // Large code objects and code objects which should stay at a fixed address
  // are allocated in large object space.
  HeapObject* result;
  bool force_lo_space = obj_size > code_space()->AreaSize();
  if (force_lo_space) {
    maybe_result = lo_space_->AllocateRaw(obj_size, EXECUTABLE);
  } else {
    maybe_result = code_space_->AllocateRaw(obj_size);
  }
  if (!maybe_result->To<HeapObject>(&result)) return maybe_result;

  if (immovable && !force_lo_space &&
      // Objects on the first page of each space are never moved.
      !code_space_->FirstPage()->Contains(result->address())) {
    // Discard the first code allocation, which was on a page where it could be
    // moved.
    CreateFillerObjectAt(result->address(), obj_size);
    maybe_result = lo_space_->AllocateRaw(obj_size, EXECUTABLE);
    if (!maybe_result->To<HeapObject>(&result)) return maybe_result;
  }

  // Initialize the object
  result->set_map_no_write_barrier(code_map());
  Code* code = Code::cast(result);
  ASSERT(!isolate_->code_range()->exists() ||
      isolate_->code_range()->contains(code->address()));
  code->set_instruction_size(desc.instr_size);
  code->set_relocation_info(reloc_info);
  code->set_flags(flags);
  if (code->is_call_stub() || code->is_keyed_call_stub()) {
    code->set_check_type(RECEIVER_MAP_CHECK);
  }
  code->set_deoptimization_data(empty_fixed_array(), SKIP_WRITE_BARRIER);
  code->set_type_feedback_info(undefined_value(), SKIP_WRITE_BARRIER);
  code->set_handler_table(empty_fixed_array(), SKIP_WRITE_BARRIER);
  code->set_gc_metadata(Smi::FromInt(0));
  code->set_ic_age(global_ic_age_);
  // Allow self references to created code object by patching the handle to
  // point to the newly allocated Code object.
  if (!self_reference.is_null()) {
    *(self_reference.location()) = code;
  }
  // Migrate generated code.
  // The generated code can contain Object** values (typically from handles)
  // that are dereferenced during the copy to point directly to the actual heap
  // objects. These pointers can include references to the code object itself,
  // through the self_reference parameter.
  code->CopyFrom(desc);

#ifdef VERIFY_HEAP
  if (FLAG_verify_heap) {
    code->Verify();
  }
#endif
  return code;
}


MaybeObject* Heap::CopyCode(Code* code) {
  // Allocate an object the same size as the code object.
  int obj_size = code->Size();
  MaybeObject* maybe_result;
  if (obj_size > code_space()->AreaSize()) {
    maybe_result = lo_space_->AllocateRaw(obj_size, EXECUTABLE);
  } else {
    maybe_result = code_space_->AllocateRaw(obj_size);
  }

  Object* result;
  if (!maybe_result->ToObject(&result)) return maybe_result;

  // Copy code object.
  Address old_addr = code->address();
  Address new_addr = reinterpret_cast<HeapObject*>(result)->address();
  CopyBlock(new_addr, old_addr, obj_size);
  // Relocate the copy.
  Code* new_code = Code::cast(result);
  ASSERT(!isolate_->code_range()->exists() ||
      isolate_->code_range()->contains(code->address()));
  new_code->Relocate(new_addr - old_addr);
  return new_code;
}


MaybeObject* Heap::CopyCode(Code* code, Vector<byte> reloc_info) {
  // Allocate ByteArray before the Code object, so that we do not risk
  // leaving uninitialized Code object (and breaking the heap).
  Object* reloc_info_array;
  { MaybeObject* maybe_reloc_info_array =
        AllocateByteArray(reloc_info.length(), TENURED);
    if (!maybe_reloc_info_array->ToObject(&reloc_info_array)) {
      return maybe_reloc_info_array;
    }
  }

  int new_body_size = RoundUp(code->instruction_size(), kObjectAlignment);

  int new_obj_size = Code::SizeFor(new_body_size);

  Address old_addr = code->address();

  size_t relocation_offset =
      static_cast<size_t>(code->instruction_end() - old_addr);

  MaybeObject* maybe_result;
  if (new_obj_size > code_space()->AreaSize()) {
    maybe_result = lo_space_->AllocateRaw(new_obj_size, EXECUTABLE);
  } else {
    maybe_result = code_space_->AllocateRaw(new_obj_size);
  }

  Object* result;
  if (!maybe_result->ToObject(&result)) return maybe_result;

  // Copy code object.
  Address new_addr = reinterpret_cast<HeapObject*>(result)->address();

  // Copy header and instructions.
  memcpy(new_addr, old_addr, relocation_offset);

  Code* new_code = Code::cast(result);
  new_code->set_relocation_info(ByteArray::cast(reloc_info_array));

  // Copy patched rinfo.
  memcpy(new_code->relocation_start(), reloc_info.start(), reloc_info.length());

  // Relocate the copy.
  ASSERT(!isolate_->code_range()->exists() ||
      isolate_->code_range()->contains(code->address()));
  new_code->Relocate(new_addr - old_addr);

#ifdef VERIFY_HEAP
  if (FLAG_verify_heap) {
    code->Verify();
  }
#endif
  return new_code;
}


MaybeObject* Heap::Allocate(Map* map, AllocationSpace space) {
  ASSERT(gc_state_ == NOT_IN_GC);
  ASSERT(map->instance_type() != MAP_TYPE);
  // If allocation failures are disallowed, we may allocate in a different
  // space when new space is full and the object is not a large object.
  AllocationSpace retry_space =
      (space != NEW_SPACE) ? space : TargetSpaceId(map->instance_type());
  Object* result;
  { MaybeObject* maybe_result =
        AllocateRaw(map->instance_size(), space, retry_space);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }
  // No need for write barrier since object is white and map is in old space.
  HeapObject::cast(result)->set_map_no_write_barrier(map);
  return result;
}


void Heap::InitializeFunction(JSFunction* function,
                              SharedFunctionInfo* shared,
                              Object* prototype) {
  ASSERT(!prototype->IsMap());
  function->initialize_properties();
  function->initialize_elements();
  function->set_shared(shared);
  function->set_code(shared->code());
  function->set_prototype_or_initial_map(prototype);
  function->set_context(undefined_value());
  function->set_literals_or_bindings(empty_fixed_array());
  function->set_next_function_link(undefined_value());
}


MaybeObject* Heap::AllocateFunctionPrototype(JSFunction* function) {
  // Allocate the prototype.  Make sure to use the object function
  // from the function's context, since the function can be from a
  // different context.
  JSFunction* object_function =
      function->context()->native_context()->object_function();

  // Each function prototype gets a copy of the object function map.
  // This avoid unwanted sharing of maps between prototypes of different
  // constructors.
  Map* new_map;
  ASSERT(object_function->has_initial_map());
  MaybeObject* maybe_map = object_function->initial_map()->Copy();
  if (!maybe_map->To(&new_map)) return maybe_map;

  Object* prototype;
  MaybeObject* maybe_prototype = AllocateJSObjectFromMap(new_map);
  if (!maybe_prototype->ToObject(&prototype)) return maybe_prototype;

  // When creating the prototype for the function we must set its
  // constructor to the function.
  MaybeObject* maybe_failure =
      JSObject::cast(prototype)->SetLocalPropertyIgnoreAttributes(
          constructor_symbol(), function, DONT_ENUM);
  if (maybe_failure->IsFailure()) return maybe_failure;

  return prototype;
}


MaybeObject* Heap::AllocateFunction(Map* function_map,
                                    SharedFunctionInfo* shared,
                                    Object* prototype,
                                    PretenureFlag pretenure) {
  AllocationSpace space =
      (pretenure == TENURED) ? OLD_POINTER_SPACE : NEW_SPACE;
  Object* result;
  { MaybeObject* maybe_result = Allocate(function_map, space);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }
  InitializeFunction(JSFunction::cast(result), shared, prototype);
  return result;
}


MaybeObject* Heap::AllocateArgumentsObject(Object* callee, int length) {
  // To get fast allocation and map sharing for arguments objects we
  // allocate them based on an arguments boilerplate.

  JSObject* boilerplate;
  int arguments_object_size;
  bool strict_mode_callee = callee->IsJSFunction() &&
      !JSFunction::cast(callee)->shared()->is_classic_mode();
  if (strict_mode_callee) {
    boilerplate =
        isolate()->context()->native_context()->
            strict_mode_arguments_boilerplate();
    arguments_object_size = kArgumentsObjectSizeStrict;
  } else {
    boilerplate =
        isolate()->context()->native_context()->arguments_boilerplate();
    arguments_object_size = kArgumentsObjectSize;
  }

  // This calls Copy directly rather than using Heap::AllocateRaw so we
  // duplicate the check here.
  ASSERT(allocation_allowed_ && gc_state_ == NOT_IN_GC);

  // Check that the size of the boilerplate matches our
  // expectations. The ArgumentsAccessStub::GenerateNewObject relies
  // on the size being a known constant.
  ASSERT(arguments_object_size == boilerplate->map()->instance_size());

  // Do the allocation.
  Object* result;
  { MaybeObject* maybe_result =
        AllocateRaw(arguments_object_size, NEW_SPACE, OLD_POINTER_SPACE);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }

  // Copy the content. The arguments boilerplate doesn't have any
  // fields that point to new space so it's safe to skip the write
  // barrier here.
  CopyBlock(HeapObject::cast(result)->address(),
            boilerplate->address(),
            JSObject::kHeaderSize);

  // Set the length property.
  JSObject::cast(result)->InObjectPropertyAtPut(kArgumentsLengthIndex,
                                                Smi::FromInt(length),
                                                SKIP_WRITE_BARRIER);
  // Set the callee property for non-strict mode arguments object only.
  if (!strict_mode_callee) {
    JSObject::cast(result)->InObjectPropertyAtPut(kArgumentsCalleeIndex,
                                                  callee);
  }

  // Check the state of the object
  ASSERT(JSObject::cast(result)->HasFastProperties());
  ASSERT(JSObject::cast(result)->HasFastObjectElements());

  return result;
}


static bool HasDuplicates(DescriptorArray* descriptors) {
  int count = descriptors->number_of_descriptors();
  if (count > 1) {
    String* prev_key = descriptors->GetKey(0);
    for (int i = 1; i != count; i++) {
      String* current_key = descriptors->GetKey(i);
      if (prev_key == current_key) return true;
      prev_key = current_key;
    }
  }
  return false;
}


MaybeObject* Heap::AllocateInitialMap(JSFunction* fun) {
  ASSERT(!fun->has_initial_map());

  // First create a new map with the size and number of in-object properties
  // suggested by the function.
  int instance_size = fun->shared()->CalculateInstanceSize();
  int in_object_properties = fun->shared()->CalculateInObjectProperties();
  Map* map;
  MaybeObject* maybe_map = AllocateMap(JS_OBJECT_TYPE, instance_size);
  if (!maybe_map->To(&map)) return maybe_map;

  // Fetch or allocate prototype.
  Object* prototype;
  if (fun->has_instance_prototype()) {
    prototype = fun->instance_prototype();
  } else {
    MaybeObject* maybe_prototype = AllocateFunctionPrototype(fun);
    if (!maybe_prototype->To(&prototype)) return maybe_prototype;
  }
  map->set_inobject_properties(in_object_properties);
  map->set_unused_property_fields(in_object_properties);
  map->set_prototype(prototype);
  ASSERT(map->has_fast_object_elements());

  // If the function has only simple this property assignments add
  // field descriptors for these to the initial map as the object
  // cannot be constructed without having these properties.  Guard by
  // the inline_new flag so we only change the map if we generate a
  // specialized construct stub.
  ASSERT(in_object_properties <= Map::kMaxPreAllocatedPropertyFields);
  if (fun->shared()->CanGenerateInlineConstructor(prototype)) {
    int count = fun->shared()->this_property_assignments_count();
    if (count > in_object_properties) {
      // Inline constructor can only handle inobject properties.
      fun->shared()->ForbidInlineConstructor();
    } else {
      DescriptorArray* descriptors;
      MaybeObject* maybe_descriptors = DescriptorArray::Allocate(count);
      if (!maybe_descriptors->To(&descriptors)) return maybe_descriptors;

      DescriptorArray::WhitenessWitness witness(descriptors);
      for (int i = 0; i < count; i++) {
        String* name = fun->shared()->GetThisPropertyAssignmentName(i);
        ASSERT(name->IsSymbol());
        FieldDescriptor field(name, i, NONE, i + 1);
        descriptors->Set(i, &field, witness);
      }
      descriptors->Sort();

      // The descriptors may contain duplicates because the compiler does not
      // guarantee the uniqueness of property names (it would have required
      // quadratic time). Once the descriptors are sorted we can check for
      // duplicates in linear time.
      if (HasDuplicates(descriptors)) {
        fun->shared()->ForbidInlineConstructor();
      } else {
        map->InitializeDescriptors(descriptors);
        map->set_pre_allocated_property_fields(count);
        map->set_unused_property_fields(in_object_properties - count);
      }
    }
  }

  fun->shared()->StartInobjectSlackTracking(map);

  return map;
}


void Heap::InitializeJSObjectFromMap(JSObject* obj,
                                     FixedArray* properties,
                                     Map* map) {
  obj->set_properties(properties);
  obj->initialize_elements();
  // TODO(1240798): Initialize the object's body using valid initial values
  // according to the object's initial map.  For example, if the map's
  // instance type is JS_ARRAY_TYPE, the length field should be initialized
  // to a number (e.g. Smi::FromInt(0)) and the elements initialized to a
  // fixed array (e.g. Heap::empty_fixed_array()).  Currently, the object
  // verification code has to cope with (temporarily) invalid objects.  See
  // for example, JSArray::JSArrayVerify).
  Object* filler;
  // We cannot always fill with one_pointer_filler_map because objects
  // created from API functions expect their internal fields to be initialized
  // with undefined_value.
  // Pre-allocated fields need to be initialized with undefined_value as well
  // so that object accesses before the constructor completes (e.g. in the
  // debugger) will not cause a crash.
  if (map->constructor()->IsJSFunction() &&
      JSFunction::cast(map->constructor())->shared()->
          IsInobjectSlackTrackingInProgress()) {
    // We might want to shrink the object later.
    ASSERT(obj->GetInternalFieldCount() == 0);
    filler = Heap::one_pointer_filler_map();
  } else {
    filler = Heap::undefined_value();
  }
  obj->InitializeBody(map, Heap::undefined_value(), filler);
}


MaybeObject* Heap::AllocateJSObjectFromMap(Map* map, PretenureFlag pretenure) {
  // JSFunctions should be allocated using AllocateFunction to be
  // properly initialized.
  ASSERT(map->instance_type() != JS_FUNCTION_TYPE);

  // Both types of global objects should be allocated using
  // AllocateGlobalObject to be properly initialized.
  ASSERT(map->instance_type() != JS_GLOBAL_OBJECT_TYPE);
  ASSERT(map->instance_type() != JS_BUILTINS_OBJECT_TYPE);

  // Allocate the backing storage for the properties.
  int prop_size =
      map->pre_allocated_property_fields() +
      map->unused_property_fields() -
      map->inobject_properties();
  ASSERT(prop_size >= 0);
  Object* properties;
  { MaybeObject* maybe_properties = AllocateFixedArray(prop_size, pretenure);
    if (!maybe_properties->ToObject(&properties)) return maybe_properties;
  }

  // Allocate the JSObject.
  AllocationSpace space =
      (pretenure == TENURED) ? OLD_POINTER_SPACE : NEW_SPACE;
  if (map->instance_size() > Page::kMaxNonCodeHeapObjectSize) space = LO_SPACE;
  Object* obj;
  { MaybeObject* maybe_obj = Allocate(map, space);
    if (!maybe_obj->ToObject(&obj)) return maybe_obj;
  }

  // Initialize the JSObject.
  InitializeJSObjectFromMap(JSObject::cast(obj),
                            FixedArray::cast(properties),
                            map);
  ASSERT(JSObject::cast(obj)->HasFastSmiOrObjectElements());
  return obj;
}


MaybeObject* Heap::AllocateJSObject(JSFunction* constructor,
                                    PretenureFlag pretenure) {
  // Allocate the initial map if absent.
  if (!constructor->has_initial_map()) {
    Object* initial_map;
    { MaybeObject* maybe_initial_map = AllocateInitialMap(constructor);
      if (!maybe_initial_map->ToObject(&initial_map)) return maybe_initial_map;
    }
    constructor->set_initial_map(Map::cast(initial_map));
    Map::cast(initial_map)->set_constructor(constructor);
  }
  // Allocate the object based on the constructors initial map.
  MaybeObject* result = AllocateJSObjectFromMap(
      constructor->initial_map(), pretenure);
#ifdef DEBUG
  // Make sure result is NOT a global object if valid.
  Object* non_failure;
  ASSERT(!result->ToObject(&non_failure) || !non_failure->IsGlobalObject());
#endif
  return result;
}


MaybeObject* Heap::AllocateJSModule(Context* context, ScopeInfo* scope_info) {
  // Allocate a fresh map. Modules do not have a prototype.
  Map* map;
  MaybeObject* maybe_map = AllocateMap(JS_MODULE_TYPE, JSModule::kSize);
  if (!maybe_map->To(&map)) return maybe_map;
  // Allocate the object based on the map.
  JSModule* module;
  MaybeObject* maybe_module = AllocateJSObjectFromMap(map, TENURED);
  if (!maybe_module->To(&module)) return maybe_module;
  module->set_context(context);
  module->set_scope_info(scope_info);
  return module;
}


MaybeObject* Heap::AllocateJSArrayAndStorage(
    ElementsKind elements_kind,
    int length,
    int capacity,
    ArrayStorageAllocationMode mode,
    PretenureFlag pretenure) {
  ASSERT(capacity >= length);
  if (length != 0 && mode == INITIALIZE_ARRAY_ELEMENTS_WITH_HOLE) {
    elements_kind = GetHoleyElementsKind(elements_kind);
  }
  MaybeObject* maybe_array = AllocateJSArray(elements_kind, pretenure);
  JSArray* array;
  if (!maybe_array->To(&array)) return maybe_array;

  if (capacity == 0) {
    array->set_length(Smi::FromInt(0));
    array->set_elements(empty_fixed_array());
    return array;
  }

  FixedArrayBase* elms;
  MaybeObject* maybe_elms = NULL;
  if (elements_kind == FAST_DOUBLE_ELEMENTS) {
    if (mode == DONT_INITIALIZE_ARRAY_ELEMENTS) {
      maybe_elms = AllocateUninitializedFixedDoubleArray(capacity);
    } else {
      ASSERT(mode == INITIALIZE_ARRAY_ELEMENTS_WITH_HOLE);
      maybe_elms = AllocateFixedDoubleArrayWithHoles(capacity);
    }
  } else {
    ASSERT(IsFastSmiOrObjectElementsKind(elements_kind));
    if (mode == DONT_INITIALIZE_ARRAY_ELEMENTS) {
      maybe_elms = AllocateUninitializedFixedArray(capacity);
    } else {
      ASSERT(mode == INITIALIZE_ARRAY_ELEMENTS_WITH_HOLE);
      maybe_elms = AllocateFixedArrayWithHoles(capacity);
    }
  }
  if (!maybe_elms->To(&elms)) return maybe_elms;

  array->set_elements(elms);
  array->set_length(Smi::FromInt(length));
  return array;
}


MaybeObject* Heap::AllocateJSArrayWithElements(
    FixedArrayBase* elements,
    ElementsKind elements_kind,
    PretenureFlag pretenure) {
  MaybeObject* maybe_array = AllocateJSArray(elements_kind, pretenure);
  JSArray* array;
  if (!maybe_array->To(&array)) return maybe_array;

  array->set_elements(elements);
  array->set_length(Smi::FromInt(elements->length()));
  array->ValidateElements();
  return array;
}


MaybeObject* Heap::AllocateJSProxy(Object* handler, Object* prototype) {
  // Allocate map.
  // TODO(rossberg): Once we optimize proxies, think about a scheme to share
  // maps. Will probably depend on the identity of the handler object, too.
  Map* map;
  MaybeObject* maybe_map_obj = AllocateMap(JS_PROXY_TYPE, JSProxy::kSize);
  if (!maybe_map_obj->To<Map>(&map)) return maybe_map_obj;
  map->set_prototype(prototype);

  // Allocate the proxy object.
  JSProxy* result;
  MaybeObject* maybe_result = Allocate(map, NEW_SPACE);
  if (!maybe_result->To<JSProxy>(&result)) return maybe_result;
  result->InitializeBody(map->instance_size(), Smi::FromInt(0));
  result->set_handler(handler);
  result->set_hash(undefined_value(), SKIP_WRITE_BARRIER);
  return result;
}


MaybeObject* Heap::AllocateJSFunctionProxy(Object* handler,
                                           Object* call_trap,
                                           Object* construct_trap,
                                           Object* prototype) {
  // Allocate map.
  // TODO(rossberg): Once we optimize proxies, think about a scheme to share
  // maps. Will probably depend on the identity of the handler object, too.
  Map* map;
  MaybeObject* maybe_map_obj =
      AllocateMap(JS_FUNCTION_PROXY_TYPE, JSFunctionProxy::kSize);
  if (!maybe_map_obj->To<Map>(&map)) return maybe_map_obj;
  map->set_prototype(prototype);

  // Allocate the proxy object.
  JSFunctionProxy* result;
  MaybeObject* maybe_result = Allocate(map, NEW_SPACE);
  if (!maybe_result->To<JSFunctionProxy>(&result)) return maybe_result;
  result->InitializeBody(map->instance_size(), Smi::FromInt(0));
  result->set_handler(handler);
  result->set_hash(undefined_value(), SKIP_WRITE_BARRIER);
  result->set_call_trap(call_trap);
  result->set_construct_trap(construct_trap);
  return result;
}


MaybeObject* Heap::AllocateGlobalObject(JSFunction* constructor) {
  ASSERT(constructor->has_initial_map());
  Map* map = constructor->initial_map();
  ASSERT(map->is_dictionary_map());

  // Make sure no field properties are described in the initial map.
  // This guarantees us that normalizing the properties does not
  // require us to change property values to JSGlobalPropertyCells.
  ASSERT(map->NextFreePropertyIndex() == 0);

  // Make sure we don't have a ton of pre-allocated slots in the
  // global objects. They will be unused once we normalize the object.
  ASSERT(map->unused_property_fields() == 0);
  ASSERT(map->inobject_properties() == 0);

  // Initial size of the backing store to avoid resize of the storage during
  // bootstrapping. The size differs between the JS global object ad the
  // builtins object.
  int initial_size = map->instance_type() == JS_GLOBAL_OBJECT_TYPE ? 64 : 512;

  // Allocate a dictionary object for backing storage.
  StringDictionary* dictionary;
  MaybeObject* maybe_dictionary =
      StringDictionary::Allocate(
          map->NumberOfOwnDescriptors() * 2 + initial_size);
  if (!maybe_dictionary->To(&dictionary)) return maybe_dictionary;

  // The global object might be created from an object template with accessors.
  // Fill these accessors into the dictionary.
  DescriptorArray* descs = map->instance_descriptors();
  for (int i = 0; i < descs->number_of_descriptors(); i++) {
    PropertyDetails details = descs->GetDetails(i);
    ASSERT(details.type() == CALLBACKS);  // Only accessors are expected.
    PropertyDetails d = PropertyDetails(details.attributes(),
                                        CALLBACKS,
                                        details.descriptor_index());
    Object* value = descs->GetCallbacksObject(i);
    MaybeObject* maybe_value = AllocateJSGlobalPropertyCell(value);
    if (!maybe_value->ToObject(&value)) return maybe_value;

    MaybeObject* maybe_added = dictionary->Add(descs->GetKey(i), value, d);
    if (!maybe_added->To(&dictionary)) return maybe_added;
  }

  // Allocate the global object and initialize it with the backing store.
  JSObject* global;
  MaybeObject* maybe_global = Allocate(map, OLD_POINTER_SPACE);
  if (!maybe_global->To(&global)) return maybe_global;

  InitializeJSObjectFromMap(global, dictionary, map);

  // Create a new map for the global object.
  Map* new_map;
  MaybeObject* maybe_map = map->CopyDropDescriptors();
  if (!maybe_map->To(&new_map)) return maybe_map;
  new_map->set_dictionary_map(true);

  // Set up the global object as a normalized object.
  global->set_map(new_map);
  global->set_properties(dictionary);

  // Make sure result is a global object with properties in dictionary.
  ASSERT(global->IsGlobalObject());
  ASSERT(!global->HasFastProperties());
  return global;
}


MaybeObject* Heap::CopyJSObject(JSObject* source) {
  // Never used to copy functions.  If functions need to be copied we
  // have to be careful to clear the literals array.
  SLOW_ASSERT(!source->IsJSFunction());

  // Make the clone.
  Map* map = source->map();
  int object_size = map->instance_size();
  Object* clone;

  WriteBarrierMode wb_mode = UPDATE_WRITE_BARRIER;

  // If we're forced to always allocate, we use the general allocation
  // functions which may leave us with an object in old space.
  if (always_allocate()) {
    { MaybeObject* maybe_clone =
          AllocateRaw(object_size, NEW_SPACE, OLD_POINTER_SPACE);
      if (!maybe_clone->ToObject(&clone)) return maybe_clone;
    }
    Address clone_address = HeapObject::cast(clone)->address();
    CopyBlock(clone_address,
              source->address(),
              object_size);
    // Update write barrier for all fields that lie beyond the header.
    RecordWrites(clone_address,
                 JSObject::kHeaderSize,
                 (object_size - JSObject::kHeaderSize) / kPointerSize);
  } else {
    wb_mode = SKIP_WRITE_BARRIER;
    { MaybeObject* maybe_clone = new_space_.AllocateRaw(object_size);
      if (!maybe_clone->ToObject(&clone)) return maybe_clone;
    }
    SLOW_ASSERT(InNewSpace(clone));
    // Since we know the clone is allocated in new space, we can copy
    // the contents without worrying about updating the write barrier.
    CopyBlock(HeapObject::cast(clone)->address(),
              source->address(),
              object_size);
  }

  SLOW_ASSERT(
      JSObject::cast(clone)->GetElementsKind() == source->GetElementsKind());
  FixedArrayBase* elements = FixedArrayBase::cast(source->elements());
  FixedArray* properties = FixedArray::cast(source->properties());
  // Update elements if necessary.
  if (elements->length() > 0) {
    Object* elem;
    { MaybeObject* maybe_elem;
      if (elements->map() == fixed_cow_array_map()) {
        maybe_elem = FixedArray::cast(elements);
      } else if (source->HasFastDoubleElements()) {
        maybe_elem = CopyFixedDoubleArray(FixedDoubleArray::cast(elements));
      } else {
        maybe_elem = CopyFixedArray(FixedArray::cast(elements));
      }
      if (!maybe_elem->ToObject(&elem)) return maybe_elem;
    }
    JSObject::cast(clone)->set_elements(FixedArrayBase::cast(elem), wb_mode);
  }
  // Update properties if necessary.
  if (properties->length() > 0) {
    Object* prop;
    { MaybeObject* maybe_prop = CopyFixedArray(properties);
      if (!maybe_prop->ToObject(&prop)) return maybe_prop;
    }
    JSObject::cast(clone)->set_properties(FixedArray::cast(prop), wb_mode);
  }
  // Return the new clone.
  return clone;
}


MaybeObject* Heap::ReinitializeJSReceiver(
    JSReceiver* object, InstanceType type, int size) {
  ASSERT(type >= FIRST_JS_OBJECT_TYPE);

  // Allocate fresh map.
  // TODO(rossberg): Once we optimize proxies, cache these maps.
  Map* map;
  MaybeObject* maybe = AllocateMap(type, size);
  if (!maybe->To<Map>(&map)) return maybe;

  // Check that the receiver has at least the size of the fresh object.
  int size_difference = object->map()->instance_size() - map->instance_size();
  ASSERT(size_difference >= 0);

  map->set_prototype(object->map()->prototype());

  // Allocate the backing storage for the properties.
  int prop_size = map->unused_property_fields() - map->inobject_properties();
  Object* properties;
  maybe = AllocateFixedArray(prop_size, TENURED);
  if (!maybe->ToObject(&properties)) return maybe;

  // Functions require some allocation, which might fail here.
  SharedFunctionInfo* shared = NULL;
  if (type == JS_FUNCTION_TYPE) {
    String* name;
    maybe = LookupAsciiSymbol("<freezing call trap>");
    if (!maybe->To<String>(&name)) return maybe;
    maybe = AllocateSharedFunctionInfo(name);
    if (!maybe->To<SharedFunctionInfo>(&shared)) return maybe;
  }

  // Because of possible retries of this function after failure,
  // we must NOT fail after this point, where we have changed the type!

  // Reset the map for the object.
  object->set_map(map);
  JSObject* jsobj = JSObject::cast(object);

  // Reinitialize the object from the constructor map.
  InitializeJSObjectFromMap(jsobj, FixedArray::cast(properties), map);

  // Functions require some minimal initialization.
  if (type == JS_FUNCTION_TYPE) {
    map->set_function_with_prototype(true);
    InitializeFunction(JSFunction::cast(object), shared, the_hole_value());
    JSFunction::cast(object)->set_context(
        isolate()->context()->native_context());
  }

  // Put in filler if the new object is smaller than the old.
  if (size_difference > 0) {
    CreateFillerObjectAt(
        object->address() + map->instance_size(), size_difference);
  }

  return object;
}


MaybeObject* Heap::ReinitializeJSGlobalProxy(JSFunction* constructor,
                                             JSGlobalProxy* object) {
  ASSERT(constructor->has_initial_map());
  Map* map = constructor->initial_map();

  // Check that the already allocated object has the same size and type as
  // objects allocated using the constructor.
  ASSERT(map->instance_size() == object->map()->instance_size());
  ASSERT(map->instance_type() == object->map()->instance_type());

  // Allocate the backing storage for the properties.
  int prop_size = map->unused_property_fields() - map->inobject_properties();
  Object* properties;
  { MaybeObject* maybe_properties = AllocateFixedArray(prop_size, TENURED);
    if (!maybe_properties->ToObject(&properties)) return maybe_properties;
  }

  // Reset the map for the object.
  object->set_map(constructor->initial_map());

  // Reinitialize the object from the constructor map.
  InitializeJSObjectFromMap(object, FixedArray::cast(properties), map);
  return object;
}


MaybeObject* Heap::AllocateStringFromAscii(Vector<const char> string,
                                           PretenureFlag pretenure) {
  int length = string.length();
  if (length == 1) {
    return Heap::LookupSingleCharacterStringFromCode(string[0]);
  }
  Object* result;
  { MaybeObject* maybe_result =
        AllocateRawAsciiString(string.length(), pretenure);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }

  // Copy the characters into the new object.
  CopyChars(SeqAsciiString::cast(result)->GetChars(), string.start(), length);
  return result;
}


MaybeObject* Heap::AllocateStringFromUtf8Slow(Vector<const char> string,
                                              int non_ascii_start,
                                              PretenureFlag pretenure) {
  // Continue counting the number of characters in the UTF-8 string, starting
  // from the first non-ascii character or word.
  int chars = non_ascii_start;
  Access<UnicodeCache::Utf8Decoder>
      decoder(isolate_->unicode_cache()->utf8_decoder());
  decoder->Reset(string.start() + non_ascii_start, string.length() - chars);
  while (decoder->has_more()) {
    uint32_t r = decoder->GetNext();
    if (r <= unibrow::Utf16::kMaxNonSurrogateCharCode) {
      chars++;
    } else {
      chars += 2;
    }
  }

  Object* result;
  { MaybeObject* maybe_result = AllocateRawTwoByteString(chars, pretenure);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }

  // Convert and copy the characters into the new object.
  SeqTwoByteString* twobyte = SeqTwoByteString::cast(result);
  decoder->Reset(string.start(), string.length());
  int i = 0;
  while (i < chars) {
    uint32_t r = decoder->GetNext();
    if (r > unibrow::Utf16::kMaxNonSurrogateCharCode) {
      twobyte->SeqTwoByteStringSet(i++, unibrow::Utf16::LeadSurrogate(r));
      twobyte->SeqTwoByteStringSet(i++, unibrow::Utf16::TrailSurrogate(r));
    } else {
      twobyte->SeqTwoByteStringSet(i++, r);
    }
  }
  return result;
}


MaybeObject* Heap::AllocateStringFromTwoByte(Vector<const uc16> string,
                                             PretenureFlag pretenure) {
  // Check if the string is an ASCII string.
  Object* result;
  int length = string.length();
  const uc16* start = string.start();

  if (String::IsAscii(start, length)) {
    MaybeObject* maybe_result = AllocateRawAsciiString(length, pretenure);
    if (!maybe_result->ToObject(&result)) return maybe_result;
    CopyChars(SeqAsciiString::cast(result)->GetChars(), start, length);
  } else {  // It's not an ASCII string.
    MaybeObject* maybe_result = AllocateRawTwoByteString(length, pretenure);
    if (!maybe_result->ToObject(&result)) return maybe_result;
    CopyChars(SeqTwoByteString::cast(result)->GetChars(), start, length);
  }
  return result;
}


Map* Heap::SymbolMapForString(String* string) {
  // If the string is in new space it cannot be used as a symbol.
  if (InNewSpace(string)) return NULL;

  // Find the corresponding symbol map for strings.
  switch (string->map()->instance_type()) {
    case STRING_TYPE: return symbol_map();
    case ASCII_STRING_TYPE: return ascii_symbol_map();
    case CONS_STRING_TYPE: return cons_symbol_map();
    case CONS_ASCII_STRING_TYPE: return cons_ascii_symbol_map();
    case EXTERNAL_STRING_TYPE: return external_symbol_map();
    case EXTERNAL_ASCII_STRING_TYPE: return external_ascii_symbol_map();
    case EXTERNAL_STRING_WITH_ASCII_DATA_TYPE:
      return external_symbol_with_ascii_data_map();
    case SHORT_EXTERNAL_STRING_TYPE: return short_external_symbol_map();
    case SHORT_EXTERNAL_ASCII_STRING_TYPE:
      return short_external_ascii_symbol_map();
    case SHORT_EXTERNAL_STRING_WITH_ASCII_DATA_TYPE:
      return short_external_symbol_with_ascii_data_map();
    default: return NULL;  // No match found.
  }
}


MaybeObject* Heap::AllocateInternalSymbol(unibrow::CharacterStream* buffer,
                                          int chars,
                                          uint32_t hash_field) {
  ASSERT(chars >= 0);
  // Ensure the chars matches the number of characters in the buffer.
  ASSERT(static_cast<unsigned>(chars) == buffer->Utf16Length());
  // Determine whether the string is ASCII.
  bool is_ascii = true;
  while (buffer->has_more()) {
    if (buffer->GetNext() > unibrow::Utf8::kMaxOneByteChar) {
      is_ascii = false;
      break;
    }
  }
  buffer->Rewind();

  // Compute map and object size.
  int size;
  Map* map;

  if (is_ascii) {
    if (chars > SeqAsciiString::kMaxLength) {
      return Failure::OutOfMemoryException();
    }
    map = ascii_symbol_map();
    size = SeqAsciiString::SizeFor(chars);
  } else {
    if (chars > SeqTwoByteString::kMaxLength) {
      return Failure::OutOfMemoryException();
    }
    map = symbol_map();
    size = SeqTwoByteString::SizeFor(chars);
  }

  // Allocate string.
  Object* result;
  { MaybeObject* maybe_result = (size > Page::kMaxNonCodeHeapObjectSize)
                   ? lo_space_->AllocateRaw(size, NOT_EXECUTABLE)
                   : old_data_space_->AllocateRaw(size);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }

  reinterpret_cast<HeapObject*>(result)->set_map_no_write_barrier(map);
  // Set length and hash fields of the allocated string.
  String* answer = String::cast(result);
  answer->set_length(chars);
  answer->set_hash_field(hash_field);

  ASSERT_EQ(size, answer->Size());

  // Fill in the characters.
  int i = 0;
  while (i < chars) {
    uint32_t character = buffer->GetNext();
    if (character > unibrow::Utf16::kMaxNonSurrogateCharCode) {
      answer->Set(i++, unibrow::Utf16::LeadSurrogate(character));
      answer->Set(i++, unibrow::Utf16::TrailSurrogate(character));
    } else {
      answer->Set(i++, character);
    }
  }
  return answer;
}


MaybeObject* Heap::AllocateRawAsciiString(int length, PretenureFlag pretenure) {
  if (length < 0 || length > SeqAsciiString::kMaxLength) {
    return Failure::OutOfMemoryException();
  }

  int size = SeqAsciiString::SizeFor(length);
  ASSERT(size <= SeqAsciiString::kMaxSize);

  AllocationSpace space = (pretenure == TENURED) ? OLD_DATA_SPACE : NEW_SPACE;
  AllocationSpace retry_space = OLD_DATA_SPACE;

  if (space == NEW_SPACE) {
    if (size > kMaxObjectSizeInNewSpace) {
      // Allocate in large object space, retry space will be ignored.
      space = LO_SPACE;
    } else if (size > Page::kMaxNonCodeHeapObjectSize) {
      // Allocate in new space, retry in large object space.
      retry_space = LO_SPACE;
    }
  } else if (space == OLD_DATA_SPACE &&
             size > Page::kMaxNonCodeHeapObjectSize) {
    space = LO_SPACE;
  }
  Object* result;
  { MaybeObject* maybe_result = AllocateRaw(size, space, retry_space);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }

  // Partially initialize the object.
  HeapObject::cast(result)->set_map_no_write_barrier(ascii_string_map());
  String::cast(result)->set_length(length);
  String::cast(result)->set_hash_field(String::kEmptyHashField);
  ASSERT_EQ(size, HeapObject::cast(result)->Size());

#ifdef VERIFY_HEAP
  if (FLAG_verify_heap) {
    // Initialize string's content to ensure ASCII-ness (character range 0-127)
    // as required when verifying the heap.
    char* dest = SeqAsciiString::cast(result)->GetChars();
    memset(dest, 0x0F, length * kCharSize);
  }
#endif

  return result;
}


MaybeObject* Heap::AllocateRawTwoByteString(int length,
                                            PretenureFlag pretenure) {
  if (length < 0 || length > SeqTwoByteString::kMaxLength) {
    return Failure::OutOfMemoryException();
  }
  int size = SeqTwoByteString::SizeFor(length);
  ASSERT(size <= SeqTwoByteString::kMaxSize);
  AllocationSpace space = (pretenure == TENURED) ? OLD_DATA_SPACE : NEW_SPACE;
  AllocationSpace retry_space = OLD_DATA_SPACE;

  if (space == NEW_SPACE) {
    if (size > kMaxObjectSizeInNewSpace) {
      // Allocate in large object space, retry space will be ignored.
      space = LO_SPACE;
    } else if (size > Page::kMaxNonCodeHeapObjectSize) {
      // Allocate in new space, retry in large object space.
      retry_space = LO_SPACE;
    }
  } else if (space == OLD_DATA_SPACE &&
             size > Page::kMaxNonCodeHeapObjectSize) {
    space = LO_SPACE;
  }
  Object* result;
  { MaybeObject* maybe_result = AllocateRaw(size, space, retry_space);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }

  // Partially initialize the object.
  HeapObject::cast(result)->set_map_no_write_barrier(string_map());
  String::cast(result)->set_length(length);
  String::cast(result)->set_hash_field(String::kEmptyHashField);
  ASSERT_EQ(size, HeapObject::cast(result)->Size());
  return result;
}


MaybeObject* Heap::AllocateJSArray(
    ElementsKind elements_kind,
    PretenureFlag pretenure) {
  Context* native_context = isolate()->context()->native_context();
  JSFunction* array_function = native_context->array_function();
  Map* map = array_function->initial_map();
  Object* maybe_map_array = native_context->js_array_maps();
  if (!maybe_map_array->IsUndefined()) {
    Object* maybe_transitioned_map =
        FixedArray::cast(maybe_map_array)->get(elements_kind);
    if (!maybe_transitioned_map->IsUndefined()) {
      map = Map::cast(maybe_transitioned_map);
    }
  }

  return AllocateJSObjectFromMap(map, pretenure);
}


MaybeObject* Heap::AllocateEmptyFixedArray() {
  int size = FixedArray::SizeFor(0);
  Object* result;
  { MaybeObject* maybe_result =
        AllocateRaw(size, OLD_DATA_SPACE, OLD_DATA_SPACE);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }
  // Initialize the object.
  reinterpret_cast<FixedArray*>(result)->set_map_no_write_barrier(
      fixed_array_map());
  reinterpret_cast<FixedArray*>(result)->set_length(0);
  return result;
}


MaybeObject* Heap::AllocateRawFixedArray(int length) {
  if (length < 0 || length > FixedArray::kMaxLength) {
    return Failure::OutOfMemoryException();
  }
  ASSERT(length > 0);
  // Use the general function if we're forced to always allocate.
  if (always_allocate()) return AllocateFixedArray(length, TENURED);
  // Allocate the raw data for a fixed array.
  int size = FixedArray::SizeFor(length);
  return size <= kMaxObjectSizeInNewSpace
      ? new_space_.AllocateRaw(size)
      : lo_space_->AllocateRaw(size, NOT_EXECUTABLE);
}


MaybeObject* Heap::CopyFixedArrayWithMap(FixedArray* src, Map* map) {
  int len = src->length();
  Object* obj;
  { MaybeObject* maybe_obj = AllocateRawFixedArray(len);
    if (!maybe_obj->ToObject(&obj)) return maybe_obj;
  }
  if (InNewSpace(obj)) {
    HeapObject* dst = HeapObject::cast(obj);
    dst->set_map_no_write_barrier(map);
    CopyBlock(dst->address() + kPointerSize,
              src->address() + kPointerSize,
              FixedArray::SizeFor(len) - kPointerSize);
    return obj;
  }
  HeapObject::cast(obj)->set_map_no_write_barrier(map);
  FixedArray* result = FixedArray::cast(obj);
  result->set_length(len);

  // Copy the content
  AssertNoAllocation no_gc;
  WriteBarrierMode mode = result->GetWriteBarrierMode(no_gc);
  for (int i = 0; i < len; i++) result->set(i, src->get(i), mode);
  return result;
}


MaybeObject* Heap::CopyFixedDoubleArrayWithMap(FixedDoubleArray* src,
                                               Map* map) {
  int len = src->length();
  Object* obj;
  { MaybeObject* maybe_obj = AllocateRawFixedDoubleArray(len, NOT_TENURED);
    if (!maybe_obj->ToObject(&obj)) return maybe_obj;
  }
  HeapObject* dst = HeapObject::cast(obj);
  dst->set_map_no_write_barrier(map);
  CopyBlock(
      dst->address() + FixedDoubleArray::kLengthOffset,
      src->address() + FixedDoubleArray::kLengthOffset,
      FixedDoubleArray::SizeFor(len) - FixedDoubleArray::kLengthOffset);
  return obj;
}


MaybeObject* Heap::AllocateFixedArray(int length) {
  ASSERT(length >= 0);
  if (length == 0) return empty_fixed_array();
  Object* result;
  { MaybeObject* maybe_result = AllocateRawFixedArray(length);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }
  // Initialize header.
  FixedArray* array = reinterpret_cast<FixedArray*>(result);
  array->set_map_no_write_barrier(fixed_array_map());
  array->set_length(length);
  // Initialize body.
  ASSERT(!InNewSpace(undefined_value()));
  MemsetPointer(array->data_start(), undefined_value(), length);
  return result;
}


MaybeObject* Heap::AllocateRawFixedArray(int length, PretenureFlag pretenure) {
  if (length < 0 || length > FixedArray::kMaxLength) {
    return Failure::OutOfMemoryException();
  }

  AllocationSpace space =
      (pretenure == TENURED) ? OLD_POINTER_SPACE : NEW_SPACE;
  int size = FixedArray::SizeFor(length);
  if (space == NEW_SPACE && size > kMaxObjectSizeInNewSpace) {
    // Too big for new space.
    space = LO_SPACE;
  } else if (space == OLD_POINTER_SPACE &&
             size > Page::kMaxNonCodeHeapObjectSize) {
    // Too big for old pointer space.
    space = LO_SPACE;
  }

  AllocationSpace retry_space =
      (size <= Page::kMaxNonCodeHeapObjectSize) ? OLD_POINTER_SPACE : LO_SPACE;

  return AllocateRaw(size, space, retry_space);
}


MUST_USE_RESULT static MaybeObject* AllocateFixedArrayWithFiller(
    Heap* heap,
    int length,
    PretenureFlag pretenure,
    Object* filler) {
  ASSERT(length >= 0);
  ASSERT(heap->empty_fixed_array()->IsFixedArray());
  if (length == 0) return heap->empty_fixed_array();

  ASSERT(!heap->InNewSpace(filler));
  Object* result;
  { MaybeObject* maybe_result = heap->AllocateRawFixedArray(length, pretenure);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }

  HeapObject::cast(result)->set_map_no_write_barrier(heap->fixed_array_map());
  FixedArray* array = FixedArray::cast(result);
  array->set_length(length);
  MemsetPointer(array->data_start(), filler, length);
  return array;
}


MaybeObject* Heap::AllocateFixedArray(int length, PretenureFlag pretenure) {
  return AllocateFixedArrayWithFiller(this,
                                      length,
                                      pretenure,
                                      undefined_value());
}


MaybeObject* Heap::AllocateFixedArrayWithHoles(int length,
                                               PretenureFlag pretenure) {
  return AllocateFixedArrayWithFiller(this,
                                      length,
                                      pretenure,
                                      the_hole_value());
}


MaybeObject* Heap::AllocateUninitializedFixedArray(int length) {
  if (length == 0) return empty_fixed_array();

  Object* obj;
  { MaybeObject* maybe_obj = AllocateRawFixedArray(length);
    if (!maybe_obj->ToObject(&obj)) return maybe_obj;
  }

  reinterpret_cast<FixedArray*>(obj)->set_map_no_write_barrier(
      fixed_array_map());
  FixedArray::cast(obj)->set_length(length);
  return obj;
}


MaybeObject* Heap::AllocateEmptyFixedDoubleArray() {
  int size = FixedDoubleArray::SizeFor(0);
  Object* result;
  { MaybeObject* maybe_result =
        AllocateRaw(size, OLD_DATA_SPACE, OLD_DATA_SPACE);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }
  // Initialize the object.
  reinterpret_cast<FixedDoubleArray*>(result)->set_map_no_write_barrier(
      fixed_double_array_map());
  reinterpret_cast<FixedDoubleArray*>(result)->set_length(0);
  return result;
}


MaybeObject* Heap::AllocateUninitializedFixedDoubleArray(
    int length,
    PretenureFlag pretenure) {
  if (length == 0) return empty_fixed_array();

  Object* elements_object;
  MaybeObject* maybe_obj = AllocateRawFixedDoubleArray(length, pretenure);
  if (!maybe_obj->ToObject(&elements_object)) return maybe_obj;
  FixedDoubleArray* elements =
      reinterpret_cast<FixedDoubleArray*>(elements_object);

  elements->set_map_no_write_barrier(fixed_double_array_map());
  elements->set_length(length);
  return elements;
}


MaybeObject* Heap::AllocateFixedDoubleArrayWithHoles(
    int length,
    PretenureFlag pretenure) {
  if (length == 0) return empty_fixed_array();

  Object* elements_object;
  MaybeObject* maybe_obj = AllocateRawFixedDoubleArray(length, pretenure);
  if (!maybe_obj->ToObject(&elements_object)) return maybe_obj;
  FixedDoubleArray* elements =
      reinterpret_cast<FixedDoubleArray*>(elements_object);

  for (int i = 0; i < length; ++i) {
    elements->set_the_hole(i);
  }

  elements->set_map_no_write_barrier(fixed_double_array_map());
  elements->set_length(length);
  return elements;
}


MaybeObject* Heap::AllocateRawFixedDoubleArray(int length,
                                               PretenureFlag pretenure) {
  if (length < 0 || length > FixedDoubleArray::kMaxLength) {
    return Failure::OutOfMemoryException();
  }

  AllocationSpace space =
      (pretenure == TENURED) ? OLD_DATA_SPACE : NEW_SPACE;
  int size = FixedDoubleArray::SizeFor(length);

#ifndef V8_HOST_ARCH_64_BIT
  size += kPointerSize;
#endif

  if (space == NEW_SPACE && size > kMaxObjectSizeInNewSpace) {
    // Too big for new space.
    space = LO_SPACE;
  } else if (space == OLD_DATA_SPACE &&
             size > Page::kMaxNonCodeHeapObjectSize) {
    // Too big for old data space.
    space = LO_SPACE;
  }

  AllocationSpace retry_space =
      (size <= Page::kMaxNonCodeHeapObjectSize) ? OLD_DATA_SPACE : LO_SPACE;

  HeapObject* object;
  { MaybeObject* maybe_object = AllocateRaw(size, space, retry_space);
    if (!maybe_object->To<HeapObject>(&object)) return maybe_object;
  }

  return EnsureDoubleAligned(this, object, size);
}


MaybeObject* Heap::AllocateHashTable(int length, PretenureFlag pretenure) {
  Object* result;
  { MaybeObject* maybe_result = AllocateFixedArray(length, pretenure);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }
  reinterpret_cast<HeapObject*>(result)->set_map_no_write_barrier(
      hash_table_map());
  ASSERT(result->IsHashTable());
  return result;
}


MaybeObject* Heap::AllocateNativeContext() {
  Object* result;
  { MaybeObject* maybe_result =
        AllocateFixedArray(Context::NATIVE_CONTEXT_SLOTS);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }
  Context* context = reinterpret_cast<Context*>(result);
  context->set_map_no_write_barrier(native_context_map());
  context->set_js_array_maps(undefined_value());
  ASSERT(context->IsNativeContext());
  ASSERT(result->IsContext());
  return result;
}


MaybeObject* Heap::AllocateGlobalContext(JSFunction* function,
                                         ScopeInfo* scope_info) {
  Object* result;
  { MaybeObject* maybe_result =
        AllocateFixedArray(scope_info->ContextLength(), TENURED);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }
  Context* context = reinterpret_cast<Context*>(result);
  context->set_map_no_write_barrier(global_context_map());
  context->set_closure(function);
  context->set_previous(function->context());
  context->set_extension(scope_info);
  context->set_global_object(function->context()->global_object());
  ASSERT(context->IsGlobalContext());
  ASSERT(result->IsContext());
  return context;
}


MaybeObject* Heap::AllocateModuleContext(ScopeInfo* scope_info) {
  Object* result;
  { MaybeObject* maybe_result =
        AllocateFixedArray(scope_info->ContextLength(), TENURED);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }
  Context* context = reinterpret_cast<Context*>(result);
  context->set_map_no_write_barrier(module_context_map());
  // Context links will be set later.
  context->set_extension(Smi::FromInt(0));
  return context;
}


MaybeObject* Heap::AllocateFunctionContext(int length, JSFunction* function) {
  ASSERT(length >= Context::MIN_CONTEXT_SLOTS);
  Object* result;
  { MaybeObject* maybe_result = AllocateFixedArray(length);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }
  Context* context = reinterpret_cast<Context*>(result);
  context->set_map_no_write_barrier(function_context_map());
  context->set_closure(function);
  context->set_previous(function->context());
  context->set_extension(Smi::FromInt(0));
  context->set_global_object(function->context()->global_object());
  return context;
}


MaybeObject* Heap::AllocateCatchContext(JSFunction* function,
                                        Context* previous,
                                        String* name,
                                        Object* thrown_object) {
  STATIC_ASSERT(Context::MIN_CONTEXT_SLOTS == Context::THROWN_OBJECT_INDEX);
  Object* result;
  { MaybeObject* maybe_result =
        AllocateFixedArray(Context::MIN_CONTEXT_SLOTS + 1);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }
  Context* context = reinterpret_cast<Context*>(result);
  context->set_map_no_write_barrier(catch_context_map());
  context->set_closure(function);
  context->set_previous(previous);
  context->set_extension(name);
  context->set_global_object(previous->global_object());
  context->set(Context::THROWN_OBJECT_INDEX, thrown_object);
  return context;
}


MaybeObject* Heap::AllocateWithContext(JSFunction* function,
                                       Context* previous,
                                       JSObject* extension) {
  Object* result;
  { MaybeObject* maybe_result = AllocateFixedArray(Context::MIN_CONTEXT_SLOTS);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }
  Context* context = reinterpret_cast<Context*>(result);
  context->set_map_no_write_barrier(with_context_map());
  context->set_closure(function);
  context->set_previous(previous);
  context->set_extension(extension);
  context->set_global_object(previous->global_object());
  return context;
}


MaybeObject* Heap::AllocateBlockContext(JSFunction* function,
                                        Context* previous,
                                        ScopeInfo* scope_info) {
  Object* result;
  { MaybeObject* maybe_result =
        AllocateFixedArrayWithHoles(scope_info->ContextLength());
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }
  Context* context = reinterpret_cast<Context*>(result);
  context->set_map_no_write_barrier(block_context_map());
  context->set_closure(function);
  context->set_previous(previous);
  context->set_extension(scope_info);
  context->set_global_object(previous->global_object());
  return context;
}


MaybeObject* Heap::AllocateScopeInfo(int length) {
  FixedArray* scope_info;
  MaybeObject* maybe_scope_info = AllocateFixedArray(length, TENURED);
  if (!maybe_scope_info->To(&scope_info)) return maybe_scope_info;
  scope_info->set_map_no_write_barrier(scope_info_map());
  return scope_info;
}


MaybeObject* Heap::AllocateStruct(InstanceType type) {
  Map* map;
  switch (type) {
#define MAKE_CASE(NAME, Name, name) \
    case NAME##_TYPE: map = name##_map(); break;
STRUCT_LIST(MAKE_CASE)
#undef MAKE_CASE
    default:
      UNREACHABLE();
      return Failure::InternalError();
  }
  int size = map->instance_size();
  AllocationSpace space =
      (size > Page::kMaxNonCodeHeapObjectSize) ? LO_SPACE : OLD_POINTER_SPACE;
  Object* result;
  { MaybeObject* maybe_result = Allocate(map, space);
    if (!maybe_result->ToObject(&result)) return maybe_result;
  }
  Struct::cast(result)->InitializeBody(size);
  return result;
}


bool Heap::IsHeapIterable() {
  return (!old_pointer_space()->was_swept_conservatively() &&
          !old_data_space()->was_swept_conservatively());
}


void Heap::EnsureHeapIsIterable() {
  ASSERT(IsAllocationAllowed());
  if (!IsHeapIterable()) {
    CollectAllGarbage(kMakeHeapIterableMask, "Heap::EnsureHeapIsIterable");
  }
  ASSERT(IsHeapIterable());
}


void Heap::AdvanceIdleIncrementalMarking(intptr_t step_size) {
  incremental_marking()->Step(step_size,
                              IncrementalMarking::NO_GC_VIA_STACK_GUARD);

  if (incremental_marking()->IsComplete()) {
    bool uncommit = false;
    if (gc_count_at_last_idle_gc_ == gc_count_) {
      // No GC since the last full GC, the mutator is probably not active.
      isolate_->compilation_cache()->Clear();
      uncommit = true;
    }
    CollectAllGarbage(kNoGCFlags, "idle notification: finalize incremental");
    gc_count_at_last_idle_gc_ = gc_count_;
    if (uncommit) {
      new_space_.Shrink();
      UncommitFromSpace();
    }
  }
}


bool Heap::IdleNotification(int hint) {
  // Hints greater than this value indicate that
  // the embedder is requesting a lot of GC work.
  const int kMaxHint = 1000;
  // Minimal hint that allows to do full GC.
  const int kMinHintForFullGC = 100;
  intptr_t size_factor = Min(Max(hint, 20), kMaxHint) / 4;
  // The size factor is in range [5..250]. The numbers here are chosen from
  // experiments. If you changes them, make sure to test with
  // chrome/performance_ui_tests --gtest_filter="GeneralMixMemoryTest.*
  intptr_t step_size =
      size_factor * IncrementalMarking::kAllocatedThreshold;

  if (contexts_disposed_ > 0) {
    if (hint >= kMaxHint) {
      // The embedder is requesting a lot of GC work after context disposal,
      // we age inline caches so that they don't keep objects from
      // the old context alive.
      AgeInlineCaches();
    }
    int mark_sweep_time = Min(TimeMarkSweepWouldTakeInMs(), 1000);
    if (hint >= mark_sweep_time && !FLAG_expose_gc &&
        incremental_marking()->IsStopped()) {
      HistogramTimerScope scope(isolate_->counters()->gc_context());
      CollectAllGarbage(kReduceMemoryFootprintMask,
                        "idle notification: contexts disposed");
    } else {
      AdvanceIdleIncrementalMarking(step_size);
      contexts_disposed_ = 0;
    }
    // Make sure that we have no pending context disposals.
    // Take into account that we might have decided to delay full collection
    // because incremental marking is in progress.
    ASSERT((contexts_disposed_ == 0) || !incremental_marking()->IsStopped());
    // After context disposal there is likely a lot of garbage remaining, reset
    // the idle notification counters in order to trigger more incremental GCs
    // on subsequent idle notifications.
    StartIdleRound();
    return false;
  }

  if (!FLAG_incremental_marking || FLAG_expose_gc || Serializer::enabled()) {
    return IdleGlobalGC();
  }

  // By doing small chunks of GC work in each IdleNotification,
  // perform a round of incremental GCs and after that wait until
  // the mutator creates enough garbage to justify a new round.
  // An incremental GC progresses as follows:
  // 1. many incremental marking steps,
  // 2. one old space mark-sweep-compact,
  // 3. many lazy sweep steps.
  // Use mark-sweep-compact events to count incremental GCs in a round.


  if (incremental_marking()->IsStopped()) {
    if (!IsSweepingComplete() &&
        !AdvanceSweepers(static_cast<int>(step_size))) {
      return false;
    }
  }

  if (mark_sweeps_since_idle_round_started_ >= kMaxMarkSweepsInIdleRound) {
    if (EnoughGarbageSinceLastIdleRound()) {
      StartIdleRound();
    } else {
      return true;
    }
  }

  int new_mark_sweeps = ms_count_ - ms_count_at_last_idle_notification_;
  mark_sweeps_since_idle_round_started_ += new_mark_sweeps;
  ms_count_at_last_idle_notification_ = ms_count_;

  int remaining_mark_sweeps = kMaxMarkSweepsInIdleRound -
                              mark_sweeps_since_idle_round_started_;

  if (remaining_mark_sweeps <= 0) {
    FinishIdleRound();
    return true;
  }

  if (incremental_marking()->IsStopped()) {
    // If there are no more than two GCs left in this idle round and we are
    // allowed to do a full GC, then make those GCs full in order to compact
    // the code space.
    // TODO(ulan): Once we enable code compaction for incremental marking,
    // we can get rid of this special case and always start incremental marking.
    if (remaining_mark_sweeps <= 2 && hint >= kMinHintForFullGC) {
      CollectAllGarbage(kReduceMemoryFootprintMask,
                        "idle notification: finalize idle round");
    } else {
      incremental_marking()->Start();
    }
  }
  if (!incremental_marking()->IsStopped()) {
    AdvanceIdleIncrementalMarking(step_size);
  }
  return false;
}


bool Heap::IdleGlobalGC() {
  static const int kIdlesBeforeScavenge = 4;
  static const int kIdlesBeforeMarkSweep = 7;
  static const int kIdlesBeforeMarkCompact = 8;
  static const int kMaxIdleCount = kIdlesBeforeMarkCompact + 1;
  static const unsigned int kGCsBetweenCleanup = 4;

  if (!last_idle_notification_gc_count_init_) {
    last_idle_notification_gc_count_ = gc_count_;
    last_idle_notification_gc_count_init_ = true;
  }

  bool uncommit = true;
  bool finished = false;

  // Reset the number of idle notifications received when a number of
  // GCs have taken place. This allows another round of cleanup based
  // on idle notifications if enough work has been carried out to
  // provoke a number of garbage collections.
  if (gc_count_ - last_idle_notification_gc_count_ < kGCsBetweenCleanup) {
    number_idle_notifications_ =
        Min(number_idle_notifications_ + 1, kMaxIdleCount);
  } else {
    number_idle_notifications_ = 0;
    last_idle_notification_gc_count_ = gc_count_;
  }

  if (number_idle_notifications_ == kIdlesBeforeScavenge) {
    CollectGarbage(NEW_SPACE, "idle notification");
    new_space_.Shrink();
    last_idle_notification_gc_count_ = gc_count_;
  } else if (number_idle_notifications_ == kIdlesBeforeMarkSweep) {
    // Before doing the mark-sweep collections we clear the
    // compilation cache to avoid hanging on to source code and
    // generated code for cached functions.
    isolate_->compilation_cache()->Clear();

    CollectAllGarbage(kReduceMemoryFootprintMask, "idle notification");
    new_space_.Shrink();
    last_idle_notification_gc_count_ = gc_count_;

  } else if (number_idle_notifications_ == kIdlesBeforeMarkCompact) {
    CollectAllGarbage(kReduceMemoryFootprintMask, "idle notification");
    new_space_.Shrink();
    last_idle_notification_gc_count_ = gc_count_;
    number_idle_notifications_ = 0;
    finished = true;
  } else if (number_idle_notifications_ > kIdlesBeforeMarkCompact) {
    // If we have received more than kIdlesBeforeMarkCompact idle
    // notifications we do not perform any cleanup because we don't
    // expect to gain much by doing so.
    finished = true;
  }

  if (uncommit) UncommitFromSpace();

  return finished;
}


#ifdef DEBUG

void Heap::Print() {
  if (!HasBeenSetUp()) return;
  isolate()->PrintStack();
  AllSpaces spaces;
  for (Space* space = spaces.next(); space != NULL; space = spaces.next())
    space->Print();
}


void Heap::ReportCodeStatistics(const char* title) {
  PrintF(">>>>>> Code Stats (%s) >>>>>>\n", title);
  PagedSpace::ResetCodeStatistics();
  // We do not look for code in new space, map space, or old space.  If code
  // somehow ends up in those spaces, we would miss it here.
  code_space_->CollectCodeStatistics();
  lo_space_->CollectCodeStatistics();
  PagedSpace::ReportCodeStatistics();
}


// This function expects that NewSpace's allocated objects histogram is
// populated (via a call to CollectStatistics or else as a side effect of a
// just-completed scavenge collection).
void Heap::ReportHeapStatistics(const char* title) {
  USE(title);
  PrintF(">>>>>> =============== %s (%d) =============== >>>>>>\n",
         title, gc_count_);
  PrintF("old_gen_promotion_limit_ %" V8_PTR_PREFIX "d\n",
         old_gen_promotion_limit_);
  PrintF("old_gen_allocation_limit_ %" V8_PTR_PREFIX "d\n",
         old_gen_allocation_limit_);
  PrintF("old_gen_limit_factor_ %d\n", old_gen_limit_factor_);

  PrintF("\n");
  PrintF("Number of handles : %d\n", HandleScope::NumberOfHandles());
  isolate_->global_handles()->PrintStats();
  PrintF("\n");

  PrintF("Heap statistics : ");
  isolate_->memory_allocator()->ReportStatistics();
  PrintF("To space : ");
  new_space_.ReportStatistics();
  PrintF("Old pointer space : ");
  old_pointer_space_->ReportStatistics();
  PrintF("Old data space : ");
  old_data_space_->ReportStatistics();
  PrintF("Code space : ");
  code_space_->ReportStatistics();
  PrintF("Map space : ");
  map_space_->ReportStatistics();
  PrintF("Cell space : ");
  cell_space_->ReportStatistics();
  PrintF("Large object space : ");
  lo_space_->ReportStatistics();
  PrintF(">>>>>> ========================================= >>>>>>\n");
}

#endif  // DEBUG

bool Heap::Contains(HeapObject* value) {
  return Contains(value->address());
}


bool Heap::Contains(Address addr) {
  if (OS::IsOutsideAllocatedSpace(addr)) return false;
  return HasBeenSetUp() &&
    (new_space_.ToSpaceContains(addr) ||
     old_pointer_space_->Contains(addr) ||
     old_data_space_->Contains(addr) ||
     code_space_->Contains(addr) ||
     map_space_->Contains(addr) ||
     cell_space_->Contains(addr) ||
     lo_space_->SlowContains(addr));
}


bool Heap::InSpace(HeapObject* value, AllocationSpace space) {
  return InSpace(value->address(), space);
}


bool Heap::InSpace(Address addr, AllocationSpace space) {
  if (OS::IsOutsideAllocatedSpace(addr)) return false;
  if (!HasBeenSetUp()) return false;

  switch (space) {
    case NEW_SPACE:
      return new_space_.ToSpaceContains(addr);
    case OLD_POINTER_SPACE:
      return old_pointer_space_->Contains(addr);
    case OLD_DATA_SPACE:
      return old_data_space_->Contains(addr);
    case CODE_SPACE:
      return code_space_->Contains(addr);
    case MAP_SPACE:
      return map_space_->Contains(addr);
    case CELL_SPACE:
      return cell_space_->Contains(addr);
    case LO_SPACE:
      return lo_space_->SlowContains(addr);
  }

  return false;
}


#ifdef VERIFY_HEAP
void Heap::Verify() {
  CHECK(HasBeenSetUp());

  store_buffer()->Verify();

  VerifyPointersVisitor visitor;
  IterateRoots(&visitor, VISIT_ONLY_STRONG);

  new_space_.Verify();

  old_pointer_space_->Verify(&visitor);
  map_space_->Verify(&visitor);

  VerifyPointersVisitor no_dirty_regions_visitor;
  old_data_space_->Verify(&no_dirty_regions_visitor);
  code_space_->Verify(&no_dirty_regions_visitor);
  cell_space_->Verify(&no_dirty_regions_visitor);

  lo_space_->Verify();
}
#endif


MaybeObject* Heap::LookupSymbol(Vector<const char> string) {
  Object* symbol = NULL;
  Object* new_table;
  { MaybeObject* maybe_new_table =
        symbol_table()->LookupSymbol(string, &symbol);
    if (!maybe_new_table->ToObject(&new_table)) return maybe_new_table;
  }
  // Can't use set_symbol_table because SymbolTable::cast knows that
  // SymbolTable is a singleton and checks for identity.
  roots_[kSymbolTableRootIndex] = new_table;
  ASSERT(symbol != NULL);
  return symbol;
}


MaybeObject* Heap::LookupAsciiSymbol(Vector<const char> string) {
  Object* symbol = NULL;
  Object* new_table;
  { MaybeObject* maybe_new_table =
        symbol_table()->LookupAsciiSymbol(string, &symbol);
    if (!maybe_new_table->ToObject(&new_table)) return maybe_new_table;
  }
  // Can't use set_symbol_table because SymbolTable::cast knows that
  // SymbolTable is a singleton and checks for identity.
  roots_[kSymbolTableRootIndex] = new_table;
  ASSERT(symbol != NULL);
  return symbol;
}


MaybeObject* Heap::LookupAsciiSymbol(Handle<SeqAsciiString> string,
                                     int from,
                                     int length) {
  Object* symbol = NULL;
  Object* new_table;
  { MaybeObject* maybe_new_table =
        symbol_table()->LookupSubStringAsciiSymbol(string,
                                                   from,
                                                   length,
                                                   &symbol);
    if (!maybe_new_table->ToObject(&new_table)) return maybe_new_table;
  }
  // Can't use set_symbol_table because SymbolTable::cast knows that
  // SymbolTable is a singleton and checks for identity.
  roots_[kSymbolTableRootIndex] = new_table;
  ASSERT(symbol != NULL);
  return symbol;
}


MaybeObject* Heap::LookupTwoByteSymbol(Vector<const uc16> string) {
  Object* symbol = NULL;
  Object* new_table;
  { MaybeObject* maybe_new_table =
        symbol_table()->LookupTwoByteSymbol(string, &symbol);
    if (!maybe_new_table->ToObject(&new_table)) return maybe_new_table;
  }
  // Can't use set_symbol_table because SymbolTable::cast knows that
  // SymbolTable is a singleton and checks for identity.
  roots_[kSymbolTableRootIndex] = new_table;
  ASSERT(symbol != NULL);
  return symbol;
}


MaybeObject* Heap::LookupSymbol(String* string) {
  if (string->IsSymbol()) return string;
  Object* symbol = NULL;
  Object* new_table;
  { MaybeObject* maybe_new_table =
        symbol_table()->LookupString(string, &symbol);
    if (!maybe_new_table->ToObject(&new_table)) return maybe_new_table;
  }
  // Can't use set_symbol_table because SymbolTable::cast knows that
  // SymbolTable is a singleton and checks for identity.
  roots_[kSymbolTableRootIndex] = new_table;
  ASSERT(symbol != NULL);
  return symbol;
}


bool Heap::LookupSymbolIfExists(String* string, String** symbol) {
  if (string->IsSymbol()) {
    *symbol = string;
    return true;
  }
  return symbol_table()->LookupSymbolIfExists(string, symbol);
}

void Heap::ZapFromSpace() {
  NewSpacePageIterator it(new_space_.FromSpaceStart(),
                          new_space_.FromSpaceEnd());
  while (it.has_next()) {
    NewSpacePage* page = it.next();
    for (Address cursor = page->area_start(), limit = page->area_end();
         cursor < limit;
         cursor += kPointerSize) {
      Memory::Address_at(cursor) = kFromSpaceZapValue;
    }
  }
}


void Heap::IterateAndMarkPointersToFromSpace(Address start,
                                             Address end,
                                             ObjectSlotCallback callback) {
  Address slot_address = start;

  // We are not collecting slots on new space objects during mutation
  // thus we have to scan for pointers to evacuation candidates when we
  // promote objects. But we should not record any slots in non-black
  // objects. Grey object's slots would be rescanned.
  // White object might not survive until the end of collection
  // it would be a violation of the invariant to record it's slots.
  bool record_slots = false;
  if (incremental_marking()->IsCompacting()) {
    MarkBit mark_bit = Marking::MarkBitFrom(HeapObject::FromAddress(start));
    record_slots = Marking::IsBlack(mark_bit);
  }

  while (slot_address < end) {
    Object** slot = reinterpret_cast<Object**>(slot_address);
    Object* object = *slot;
    // If the store buffer becomes overfull we mark pages as being exempt from
    // the store buffer.  These pages are scanned to find pointers that point
    // to the new space.  In that case we may hit newly promoted objects and
    // fix the pointers before the promotion queue gets to them.  Thus the 'if'.
    if (object->IsHeapObject()) {
      if (Heap::InFromSpace(object)) {
        callback(reinterpret_cast<HeapObject**>(slot),
                 HeapObject::cast(object));
        Object* new_object = *slot;
        if (InNewSpace(new_object)) {
          SLOW_ASSERT(Heap::InToSpace(new_object));
          SLOW_ASSERT(new_object->IsHeapObject());
          store_buffer_.EnterDirectlyIntoStoreBuffer(
              reinterpret_cast<Address>(slot));
        }
        SLOW_ASSERT(!MarkCompactCollector::IsOnEvacuationCandidate(new_object));
      } else if (record_slots &&
                 MarkCompactCollector::IsOnEvacuationCandidate(object)) {
        mark_compact_collector()->RecordSlot(slot, slot, object);
      }
    }
    slot_address += kPointerSize;
  }
}


#ifdef DEBUG
typedef bool (*CheckStoreBufferFilter)(Object** addr);


bool IsAMapPointerAddress(Object** addr) {
  uintptr_t a = reinterpret_cast<uintptr_t>(addr);
  int mod = a % Map::kSize;
  return mod >= Map::kPointerFieldsBeginOffset &&
         mod < Map::kPointerFieldsEndOffset;
}


bool EverythingsAPointer(Object** addr) {
  return true;
}


static void CheckStoreBuffer(Heap* heap,
                             Object** current,
                             Object** limit,
                             Object**** store_buffer_position,
                             Object*** store_buffer_top,
                             CheckStoreBufferFilter filter,
                             Address special_garbage_start,
                             Address special_garbage_end) {
  Map* free_space_map = heap->free_space_map();
  for ( ; current < limit; current++) {
    Object* o = *current;
    Address current_address = reinterpret_cast<Address>(current);
    // Skip free space.
    if (o == free_space_map) {
      Address current_address = reinterpret_cast<Address>(current);
      FreeSpace* free_space =
          FreeSpace::cast(HeapObject::FromAddress(current_address));
      int skip = free_space->Size();
      ASSERT(current_address + skip <= reinterpret_cast<Address>(limit));
      ASSERT(skip > 0);
      current_address += skip - kPointerSize;
      current = reinterpret_cast<Object**>(current_address);
      continue;
    }
    // Skip the current linear allocation space between top and limit which is
    // unmarked with the free space map, but can contain junk.
    if (current_address == special_garbage_start &&
        special_garbage_end != special_garbage_start) {
      current_address = special_garbage_end - kPointerSize;
      current = reinterpret_cast<Object**>(current_address);
      continue;
    }
    if (!(*filter)(current)) continue;
    ASSERT(current_address < special_garbage_start ||
           current_address >= special_garbage_end);
    ASSERT(reinterpret_cast<uintptr_t>(o) != kFreeListZapValue);
    // We have to check that the pointer does not point into new space
    // without trying to cast it to a heap object since the hash field of
    // a string can contain values like 1 and 3 which are tagged null
    // pointers.
    if (!heap->InNewSpace(o)) continue;
    while (**store_buffer_position < current &&
           *store_buffer_position < store_buffer_top) {
      (*store_buffer_position)++;
    }
    if (**store_buffer_position != current ||
        *store_buffer_position == store_buffer_top) {
      Object** obj_start = current;
      while (!(*obj_start)->IsMap()) obj_start--;
      UNREACHABLE();
    }
  }
}


// Check that the store buffer contains all intergenerational pointers by
// scanning a page and ensuring that all pointers to young space are in the
// store buffer.
void Heap::OldPointerSpaceCheckStoreBuffer() {
  OldSpace* space = old_pointer_space();
  PageIterator pages(space);

  store_buffer()->SortUniq();

  while (pages.has_next()) {
    Page* page = pages.next();
    Object** current = reinterpret_cast<Object**>(page->area_start());

    Address end = page->area_end();

    Object*** store_buffer_position = store_buffer()->Start();
    Object*** store_buffer_top = store_buffer()->Top();

    Object** limit = reinterpret_cast<Object**>(end);
    CheckStoreBuffer(this,
                     current,
                     limit,
                     &store_buffer_position,
                     store_buffer_top,
                     &EverythingsAPointer,
                     space->top(),
                     space->limit());
  }
}


void Heap::MapSpaceCheckStoreBuffer() {
  MapSpace* space = map_space();
  PageIterator pages(space);

  store_buffer()->SortUniq();

  while (pages.has_next()) {
    Page* page = pages.next();
    Object** current = reinterpret_cast<Object**>(page->area_start());

    Address end = page->area_end();

    Object*** store_buffer_position = store_buffer()->Start();
    Object*** store_buffer_top = store_buffer()->Top();

    Object** limit = reinterpret_cast<Object**>(end);
    CheckStoreBuffer(this,
                     current,
                     limit,
                     &store_buffer_position,
                     store_buffer_top,
                     &IsAMapPointerAddress,
                     space->top(),
                     space->limit());
  }
}


void Heap::LargeObjectSpaceCheckStoreBuffer() {
  LargeObjectIterator it(lo_space());
  for (HeapObject* object = it.Next(); object != NULL; object = it.Next()) {
    // We only have code, sequential strings, or fixed arrays in large
    // object space, and only fixed arrays can possibly contain pointers to
    // the young generation.
    if (object->IsFixedArray()) {
      Object*** store_buffer_position = store_buffer()->Start();
      Object*** store_buffer_top = store_buffer()->Top();
      Object** current = reinterpret_cast<Object**>(object->address());
      Object** limit =
          reinterpret_cast<Object**>(object->address() + object->Size());
      CheckStoreBuffer(this,
                       current,
                       limit,
                       &store_buffer_position,
                       store_buffer_top,
                       &EverythingsAPointer,
                       NULL,
                       NULL);
    }
  }
}
#endif


void Heap::IterateRoots(ObjectVisitor* v, VisitMode mode) {
  IterateStrongRoots(v, mode);
  IterateWeakRoots(v, mode);
}


void Heap::IterateWeakRoots(ObjectVisitor* v, VisitMode mode) {
  v->VisitPointer(reinterpret_cast<Object**>(&roots_[kSymbolTableRootIndex]));
  v->Synchronize(VisitorSynchronization::kSymbolTable);
  if (mode != VISIT_ALL_IN_SCAVENGE &&
      mode != VISIT_ALL_IN_SWEEP_NEWSPACE) {
    // Scavenge collections have special processing for this.
    external_string_table_.Iterate(v);
  }
  v->Synchronize(VisitorSynchronization::kExternalStringsTable);
}


void Heap::IterateStrongRoots(ObjectVisitor* v, VisitMode mode) {
  v->VisitPointers(&roots_[0], &roots_[kStrongRootListLength]);
  v->Synchronize(VisitorSynchronization::kStrongRootList);

  v->VisitPointer(BitCast<Object**>(&hidden_symbol_));
  v->Synchronize(VisitorSynchronization::kSymbol);

  isolate_->bootstrapper()->Iterate(v);
  v->Synchronize(VisitorSynchronization::kBootstrapper);
  isolate_->Iterate(v);
  v->Synchronize(VisitorSynchronization::kTop);
  Relocatable::Iterate(v);
  v->Synchronize(VisitorSynchronization::kRelocatable);

#ifdef ENABLE_DEBUGGER_SUPPORT
  isolate_->debug()->Iterate(v);
  if (isolate_->deoptimizer_data() != NULL) {
    isolate_->deoptimizer_data()->Iterate(v);
  }
#endif
  v->Synchronize(VisitorSynchronization::kDebug);
  isolate_->compilation_cache()->Iterate(v);
  v->Synchronize(VisitorSynchronization::kCompilationCache);

  // Iterate over local handles in handle scopes.
  isolate_->handle_scope_implementer()->Iterate(v);
  isolate_->IterateDeferredHandles(v);
  v->Synchronize(VisitorSynchronization::kHandleScope);

  // Iterate over the builtin code objects and code stubs in the
  // heap. Note that it is not necessary to iterate over code objects
  // on scavenge collections.
  if (mode != VISIT_ALL_IN_SCAVENGE) {
    isolate_->builtins()->IterateBuiltins(v);
  }
  v->Synchronize(VisitorSynchronization::kBuiltins);

  // Iterate over global handles.
  switch (mode) {
    case VISIT_ONLY_STRONG:
      isolate_->global_handles()->IterateStrongRoots(v);
      break;
    case VISIT_ALL_IN_SCAVENGE:
      isolate_->global_handles()->IterateNewSpaceStrongAndDependentRoots(v);
      break;
    case VISIT_ALL_IN_SWEEP_NEWSPACE:
    case VISIT_ALL:
      isolate_->global_handles()->IterateAllRoots(v);
      break;
  }
  v->Synchronize(VisitorSynchronization::kGlobalHandles);

  // Iterate over pointers being held by inactive threads.
  isolate_->thread_manager()->Iterate(v);
  v->Synchronize(VisitorSynchronization::kThreadManager);

  // Iterate over the pointers the Serialization/Deserialization code is
  // holding.
  // During garbage collection this keeps the partial snapshot cache alive.
  // During deserialization of the startup snapshot this creates the partial
  // snapshot cache and deserializes the objects it refers to.  During
  // serialization this does nothing, since the partial snapshot cache is
  // empty.  However the next thing we do is create the partial snapshot,
  // filling up the partial snapshot cache with objects it needs as we go.
  SerializerDeserializer::Iterate(v);
  // We don't do a v->Synchronize call here, because in debug mode that will
  // output a flag to the snapshot.  However at this point the serializer and
  // deserializer are deliberately a little unsynchronized (see above) so the
  // checking of the sync flag in the snapshot would fail.
}


// TODO(1236194): Since the heap size is configurable on the command line
// and through the API, we should gracefully handle the case that the heap
// size is not big enough to fit all the initial objects.
bool Heap::ConfigureHeap(int max_semispace_size,
                         intptr_t max_old_gen_size,
                         intptr_t max_executable_size) {
  if (HasBeenSetUp()) return false;

  if (FLAG_stress_compaction) {
    // This will cause more frequent GCs when stressing.
    max_semispace_size_ = Page::kPageSize;
  }

  if (max_semispace_size > 0) {
    if (max_semispace_size < Page::kPageSize) {
      max_semispace_size = Page::kPageSize;
      if (FLAG_trace_gc) {
        PrintPID("Max semispace size cannot be less than %dkbytes\n",
                 Page::kPageSize >> 10);
      }
    }
    max_semispace_size_ = max_semispace_size;
  }

  if (Snapshot::IsEnabled()) {
    // If we are using a snapshot we always reserve the default amount
    // of memory for each semispace because code in the snapshot has
    // write-barrier code that relies on the size and alignment of new
    // space.  We therefore cannot use a larger max semispace size
    // than the default reserved semispace size.
    if (max_semispace_size_ > reserved_semispace_size_) {
      max_semispace_size_ = reserved_semispace_size_;
      if (FLAG_trace_gc) {
        PrintPID("Max semispace size cannot be more than %dkbytes\n",
                 reserved_semispace_size_ >> 10);
      }
    }
  } else {
    // If we are not using snapshots we reserve space for the actual
    // max semispace size.
    reserved_semispace_size_ = max_semispace_size_;
  }

  if (max_old_gen_size > 0) max_old_generation_size_ = max_old_gen_size;
  if (max_executable_size > 0) {
    max_executable_size_ = RoundUp(max_executable_size, Page::kPageSize);
  }

  // The max executable size must be less than or equal to the max old
  // generation size.
  if (max_executable_size_ > max_old_generation_size_) {
    max_executable_size_ = max_old_generation_size_;
  }

  // The new space size must be a power of two to support single-bit testing
  // for containment.
  max_semispace_size_ = RoundUpToPowerOf2(max_semispace_size_);
  reserved_semispace_size_ = RoundUpToPowerOf2(reserved_semispace_size_);
  initial_semispace_size_ = Min(initial_semispace_size_, max_semispace_size_);
  external_allocation_limit_ = 16 * max_semispace_size_;

  // The old generation is paged and needs at least one page for each space.
  int paged_space_count = LAST_PAGED_SPACE - FIRST_PAGED_SPACE + 1;
  max_old_generation_size_ = Max(static_cast<intptr_t>(paged_space_count *
                                                       Page::kPageSize),
                                 RoundUp(max_old_generation_size_,
                                         Page::kPageSize));

  configured_ = true;
  return true;
}


bool Heap::ConfigureHeapDefault() {
  return ConfigureHeap(static_cast<intptr_t>(FLAG_max_new_space_size / 2) * KB,
                       static_cast<intptr_t>(FLAG_max_old_space_size) * MB,
                       static_cast<intptr_t>(FLAG_max_executable_size) * MB);
}


void Heap::RecordStats(HeapStats* stats, bool take_snapshot) {
  *stats->start_marker = HeapStats::kStartMarker;
  *stats->end_marker = HeapStats::kEndMarker;
  *stats->new_space_size = new_space_.SizeAsInt();
  *stats->new_space_capacity = static_cast<int>(new_space_.Capacity());
  *stats->old_pointer_space_size = old_pointer_space_->SizeOfObjects();
  *stats->old_pointer_space_capacity = old_pointer_space_->Capacity();
  *stats->old_data_space_size = old_data_space_->SizeOfObjects();
  *stats->old_data_space_capacity = old_data_space_->Capacity();
  *stats->code_space_size = code_space_->SizeOfObjects();
  *stats->code_space_capacity = code_space_->Capacity();
  *stats->map_space_size = map_space_->SizeOfObjects();
  *stats->map_space_capacity = map_space_->Capacity();
  *stats->cell_space_size = cell_space_->SizeOfObjects();
  *stats->cell_space_capacity = cell_space_->Capacity();
  *stats->lo_space_size = lo_space_->Size();
  isolate_->global_handles()->RecordStats(stats);
  *stats->memory_allocator_size = isolate()->memory_allocator()->Size();
  *stats->memory_allocator_capacity =
      isolate()->memory_allocator()->Size() +
      isolate()->memory_allocator()->Available();
  *stats->os_error = OS::GetLastError();
      isolate()->memory_allocator()->Available();
  if (take_snapshot) {
    HeapIterator iterator;
    for (HeapObject* obj = iterator.next();
         obj != NULL;
         obj = iterator.next()) {
      InstanceType type = obj->map()->instance_type();
      ASSERT(0 <= type && type <= LAST_TYPE);
      stats->objects_per_type[type]++;
      stats->size_per_type[type] += obj->Size();
    }
  }
}


intptr_t Heap::PromotedSpaceSizeOfObjects() {
  return old_pointer_space_->SizeOfObjects()
      + old_data_space_->SizeOfObjects()
      + code_space_->SizeOfObjects()
      + map_space_->SizeOfObjects()
      + cell_space_->SizeOfObjects()
      + lo_space_->SizeOfObjects();
}


intptr_t Heap::PromotedExternalMemorySize() {
  if (amount_of_external_allocated_memory_
      <= amount_of_external_allocated_memory_at_last_global_gc_) return 0;
  return amount_of_external_allocated_memory_
      - amount_of_external_allocated_memory_at_last_global_gc_;
}

#ifdef DEBUG

// Tags 0, 1, and 3 are used. Use 2 for marking visited HeapObject.
static const int kMarkTag = 2;


class HeapDebugUtils {
 public:
  explicit HeapDebugUtils(Heap* heap)
    : search_for_any_global_(false),
      search_target_(NULL),
      found_target_(false),
      object_stack_(20),
      heap_(heap) {
  }

  class MarkObjectVisitor : public ObjectVisitor {
   public:
    explicit MarkObjectVisitor(HeapDebugUtils* utils) : utils_(utils) { }

    void VisitPointers(Object** start, Object** end) {
      // Copy all HeapObject pointers in [start, end)
      for (Object** p = start; p < end; p++) {
        if ((*p)->IsHeapObject())
          utils_->MarkObjectRecursively(p);
      }
    }

    HeapDebugUtils* utils_;
  };

  void MarkObjectRecursively(Object** p) {
    if (!(*p)->IsHeapObject()) return;

    HeapObject* obj = HeapObject::cast(*p);

    Object* map = obj->map();

    if (!map->IsHeapObject()) return;  // visited before

    if (found_target_) return;  // stop if target found
    object_stack_.Add(obj);
    if ((search_for_any_global_ && obj->IsJSGlobalObject()) ||
        (!search_for_any_global_ && (obj == search_target_))) {
      found_target_ = true;
      return;
    }

    // not visited yet
    Map* map_p = reinterpret_cast<Map*>(HeapObject::cast(map));

    Address map_addr = map_p->address();

    obj->set_map_no_write_barrier(reinterpret_cast<Map*>(map_addr + kMarkTag));

    MarkObjectRecursively(&map);

    MarkObjectVisitor mark_visitor(this);

    obj->IterateBody(map_p->instance_type(), obj->SizeFromMap(map_p),
                     &mark_visitor);

    if (!found_target_)  // don't pop if found the target
      object_stack_.RemoveLast();
  }


  class UnmarkObjectVisitor : public ObjectVisitor {
   public:
    explicit UnmarkObjectVisitor(HeapDebugUtils* utils) : utils_(utils) { }

    void VisitPointers(Object** start, Object** end) {
      // Copy all HeapObject pointers in [start, end)
      for (Object** p = start; p < end; p++) {
        if ((*p)->IsHeapObject())
          utils_->UnmarkObjectRecursively(p);
      }
    }

    HeapDebugUtils* utils_;
  };


  void UnmarkObjectRecursively(Object** p) {
    if (!(*p)->IsHeapObject()) return;

    HeapObject* obj = HeapObject::cast(*p);

    Object* map = obj->map();

    if (map->IsHeapObject()) return;  // unmarked already

    Address map_addr = reinterpret_cast<Address>(map);

    map_addr -= kMarkTag;

    ASSERT_TAG_ALIGNED(map_addr);

    HeapObject* map_p = HeapObject::FromAddress(map_addr);

    obj->set_map_no_write_barrier(reinterpret_cast<Map*>(map_p));

    UnmarkObjectRecursively(reinterpret_cast<Object**>(&map_p));

    UnmarkObjectVisitor unmark_visitor(this);

    obj->IterateBody(Map::cast(map_p)->instance_type(),
                     obj->SizeFromMap(Map::cast(map_p)),
                     &unmark_visitor);
  }


  void MarkRootObjectRecursively(Object** root) {
    if (search_for_any_global_) {
      ASSERT(search_target_ == NULL);
    } else {
      ASSERT(search_target_->IsHeapObject());
    }
    found_target_ = false;
    object_stack_.Clear();

    MarkObjectRecursively(root);
    UnmarkObjectRecursively(root);

    if (found_target_) {
      PrintF("=====================================\n");
      PrintF("====        Path to object       ====\n");
      PrintF("=====================================\n\n");

      ASSERT(!object_stack_.is_empty());
      for (int i = 0; i < object_stack_.length(); i++) {
        if (i > 0) PrintF("\n     |\n     |\n     V\n\n");
        Object* obj = object_stack_[i];
        obj->Print();
      }
      PrintF("=====================================\n");
    }
  }

  // Helper class for visiting HeapObjects recursively.
  class MarkRootVisitor: public ObjectVisitor {
   public:
    explicit MarkRootVisitor(HeapDebugUtils* utils) : utils_(utils) { }

    void VisitPointers(Object** start, Object** end) {
      // Visit all HeapObject pointers in [start, end)
      for (Object** p = start; p < end; p++) {
        if ((*p)->IsHeapObject())
          utils_->MarkRootObjectRecursively(p);
      }
    }

    HeapDebugUtils* utils_;
  };

  bool search_for_any_global_;
  Object* search_target_;
  bool found_target_;
  List<Object*> object_stack_;
  Heap* heap_;

  friend class Heap;
};

#endif


V8_DECLARE_ONCE(initialize_gc_once);

static void InitializeGCOnce() {
  InitializeScavengingVisitorsTables();
  NewSpaceScavenger::Initialize();
  MarkCompactCollector::Initialize();
}

bool Heap::SetUp(bool create_heap_objects) {
#ifdef DEBUG
  allocation_timeout_ = FLAG_gc_interval;
  debug_utils_ = new HeapDebugUtils(this);
#endif

  // Initialize heap spaces and initial maps and objects. Whenever something
  // goes wrong, just return false. The caller should check the results and
  // call Heap::TearDown() to release allocated memory.
  //
  // If the heap is not yet configured (e.g. through the API), configure it.
  // Configuration is based on the flags new-space-size (really the semispace
  // size) and old-space-size if set or the initial values of semispace_size_
  // and old_generation_size_ otherwise.
  if (!configured_) {
    if (!ConfigureHeapDefault()) return false;
  }

  CallOnce(&initialize_gc_once, &InitializeGCOnce);

  MarkMapPointersAsEncoded(false);

  // Set up memory allocator.
  if (!isolate_->memory_allocator()->SetUp(MaxReserved(), MaxExecutableSize()))
      return false;

  // Set up new space.
  if (!new_space_.SetUp(reserved_semispace_size_, max_semispace_size_)) {
    return false;
  }

  // Initialize old pointer space.
  old_pointer_space_ =
      new OldSpace(this,
                   max_old_generation_size_,
                   OLD_POINTER_SPACE,
                   NOT_EXECUTABLE);
  if (old_pointer_space_ == NULL) return false;
  if (!old_pointer_space_->SetUp()) return false;

  // Initialize old data space.
  old_data_space_ =
      new OldSpace(this,
                   max_old_generation_size_,
                   OLD_DATA_SPACE,
                   NOT_EXECUTABLE);
  if (old_data_space_ == NULL) return false;
  if (!old_data_space_->SetUp()) return false;

  // Initialize the code space, set its maximum capacity to the old
  // generation size. It needs executable memory.
  // On 64-bit platform(s), we put all code objects in a 2 GB range of
  // virtual address space, so that they can call each other with near calls.
  if (code_range_size_ > 0) {
    if (!isolate_->code_range()->SetUp(code_range_size_)) {
      return false;
    }
  }

  code_space_ =
      new OldSpace(this, max_old_generation_size_, CODE_SPACE, EXECUTABLE);
  if (code_space_ == NULL) return false;
  if (!code_space_->SetUp()) return false;

  // Initialize map space.
  map_space_ = new MapSpace(this, max_old_generation_size_, MAP_SPACE);
  if (map_space_ == NULL) return false;
  if (!map_space_->SetUp()) return false;

  // Initialize global property cell space.
  cell_space_ = new CellSpace(this, max_old_generation_size_, CELL_SPACE);
  if (cell_space_ == NULL) return false;
  if (!cell_space_->SetUp()) return false;

  // The large object code space may contain code or data.  We set the memory
  // to be non-executable here for safety, but this means we need to enable it
  // explicitly when allocating large code objects.
  lo_space_ = new LargeObjectSpace(this, max_old_generation_size_, LO_SPACE);
  if (lo_space_ == NULL) return false;
  if (!lo_space_->SetUp()) return false;

  // Set up the seed that is used to randomize the string hash function.
  ASSERT(hash_seed() == 0);
  if (FLAG_randomize_hashes) {
    if (FLAG_hash_seed == 0) {
      set_hash_seed(
          Smi::FromInt(V8::RandomPrivate(isolate()) & 0x3fffffff));
    } else {
      set_hash_seed(Smi::FromInt(FLAG_hash_seed));
    }
  }

  if (create_heap_objects) {
    // Create initial maps.
    if (!CreateInitialMaps()) return false;
    if (!CreateApiObjects()) return false;

    // Create initial objects
    if (!CreateInitialObjects()) return false;

    native_contexts_list_ = undefined_value();
  }

  LOG(isolate_, IntPtrTEvent("heap-capacity", Capacity()));
  LOG(isolate_, IntPtrTEvent("heap-available", Available()));

  store_buffer()->SetUp();

  if (FLAG_parallel_recompilation) relocation_mutex_ = OS::CreateMutex();

  return true;
}


void Heap::SetStackLimits() {
  ASSERT(isolate_ != NULL);
  ASSERT(isolate_ == isolate());
  // On 64 bit machines, pointers are generally out of range of Smis.  We write
  // something that looks like an out of range Smi to the GC.

  // Set up the special root array entries containing the stack limits.
  // These are actually addresses, but the tag makes the GC ignore it.
  roots_[kStackLimitRootIndex] =
      reinterpret_cast<Object*>(
          (isolate_->stack_guard()->jslimit() & ~kSmiTagMask) | kSmiTag);
  roots_[kRealStackLimitRootIndex] =
      reinterpret_cast<Object*>(
          (isolate_->stack_guard()->real_jslimit() & ~kSmiTagMask) | kSmiTag);
}


void Heap::TearDown() {
#ifdef VERIFY_HEAP
  if (FLAG_verify_heap) {
    Verify();
  }
#endif

  if (FLAG_print_cumulative_gc_stat) {
    PrintF("\n\n");
    PrintF("gc_count=%d ", gc_count_);
    PrintF("mark_sweep_count=%d ", ms_count_);
    PrintF("max_gc_pause=%d ", get_max_gc_pause());
    PrintF("total_gc_time=%d ", total_gc_time_ms_);
    PrintF("min_in_mutator=%d ", get_min_in_mutator());
    PrintF("max_alive_after_gc=%" V8_PTR_PREFIX "d ",
           get_max_alive_after_gc());
    PrintF("\n\n");
  }

  isolate_->global_handles()->TearDown();

  external_string_table_.TearDown();

  new_space_.TearDown();

  if (old_pointer_space_ != NULL) {
    old_pointer_space_->TearDown();
    delete old_pointer_space_;
    old_pointer_space_ = NULL;
  }

  if (old_data_space_ != NULL) {
    old_data_space_->TearDown();
    delete old_data_space_;
    old_data_space_ = NULL;
  }

  if (code_space_ != NULL) {
    code_space_->TearDown();
    delete code_space_;
    code_space_ = NULL;
  }

  if (map_space_ != NULL) {
    map_space_->TearDown();
    delete map_space_;
    map_space_ = NULL;
  }

  if (cell_space_ != NULL) {
    cell_space_->TearDown();
    delete cell_space_;
    cell_space_ = NULL;
  }

  if (lo_space_ != NULL) {
    lo_space_->TearDown();
    delete lo_space_;
    lo_space_ = NULL;
  }

  store_buffer()->TearDown();
  incremental_marking()->TearDown();

  isolate_->memory_allocator()->TearDown();

  delete relocation_mutex_;

#ifdef DEBUG
  delete debug_utils_;
  debug_utils_ = NULL;
#endif
}


void Heap::Shrink() {
  // Try to shrink all paged spaces.
  PagedSpaces spaces;
  for (PagedSpace* space = spaces.next();
       space != NULL;
       space = spaces.next()) {
    space->ReleaseAllUnusedPages();
  }
}


void Heap::AddGCPrologueCallback(GCPrologueCallback callback, GCType gc_type) {
  ASSERT(callback != NULL);
  GCPrologueCallbackPair pair(callback, gc_type);
  ASSERT(!gc_prologue_callbacks_.Contains(pair));
  return gc_prologue_callbacks_.Add(pair);
}


void Heap::RemoveGCPrologueCallback(GCPrologueCallback callback) {
  ASSERT(callback != NULL);
  for (int i = 0; i < gc_prologue_callbacks_.length(); ++i) {
    if (gc_prologue_callbacks_[i].callback == callback) {
      gc_prologue_callbacks_.Remove(i);
      return;
    }
  }
  UNREACHABLE();
}


void Heap::AddGCEpilogueCallback(GCEpilogueCallback callback, GCType gc_type) {
  ASSERT(callback != NULL);
  GCEpilogueCallbackPair pair(callback, gc_type);
  ASSERT(!gc_epilogue_callbacks_.Contains(pair));
  return gc_epilogue_callbacks_.Add(pair);
}


void Heap::RemoveGCEpilogueCallback(GCEpilogueCallback callback) {
  ASSERT(callback != NULL);
  for (int i = 0; i < gc_epilogue_callbacks_.length(); ++i) {
    if (gc_epilogue_callbacks_[i].callback == callback) {
      gc_epilogue_callbacks_.Remove(i);
      return;
    }
  }
  UNREACHABLE();
}


#ifdef DEBUG

class PrintHandleVisitor: public ObjectVisitor {
 public:
  void VisitPointers(Object** start, Object** end) {
    for (Object** p = start; p < end; p++)
      PrintF("  handle %p to %p\n",
             reinterpret_cast<void*>(p),
             reinterpret_cast<void*>(*p));
  }
};

void Heap::PrintHandles() {
  PrintF("Handles:\n");
  PrintHandleVisitor v;
  isolate_->handle_scope_implementer()->Iterate(&v);
}

#endif


Space* AllSpaces::next() {
  switch (counter_++) {
    case NEW_SPACE:
      return HEAP->new_space();
    case OLD_POINTER_SPACE:
      return HEAP->old_pointer_space();
    case OLD_DATA_SPACE:
      return HEAP->old_data_space();
    case CODE_SPACE:
      return HEAP->code_space();
    case MAP_SPACE:
      return HEAP->map_space();
    case CELL_SPACE:
      return HEAP->cell_space();
    case LO_SPACE:
      return HEAP->lo_space();
    default:
      return NULL;
  }
}


PagedSpace* PagedSpaces::next() {
  switch (counter_++) {
    case OLD_POINTER_SPACE:
      return HEAP->old_pointer_space();
    case OLD_DATA_SPACE:
      return HEAP->old_data_space();
    case CODE_SPACE:
      return HEAP->code_space();
    case MAP_SPACE:
      return HEAP->map_space();
    case CELL_SPACE:
      return HEAP->cell_space();
    default:
      return NULL;
  }
}



OldSpace* OldSpaces::next() {
  switch (counter_++) {
    case OLD_POINTER_SPACE:
      return HEAP->old_pointer_space();
    case OLD_DATA_SPACE:
      return HEAP->old_data_space();
    case CODE_SPACE:
      return HEAP->code_space();
    default:
      return NULL;
  }
}


SpaceIterator::SpaceIterator()
    : current_space_(FIRST_SPACE),
      iterator_(NULL),
      size_func_(NULL) {
}


SpaceIterator::SpaceIterator(HeapObjectCallback size_func)
    : current_space_(FIRST_SPACE),
      iterator_(NULL),
      size_func_(size_func) {
}


SpaceIterator::~SpaceIterator() {
  // Delete active iterator if any.
  delete iterator_;
}


bool SpaceIterator::has_next() {
  // Iterate until no more spaces.
  return current_space_ != LAST_SPACE;
}


ObjectIterator* SpaceIterator::next() {
  if (iterator_ != NULL) {
    delete iterator_;
    iterator_ = NULL;
    // Move to the next space
    current_space_++;
    if (current_space_ > LAST_SPACE) {
      return NULL;
    }
  }

  // Return iterator for the new current space.
  return CreateIterator();
}


// Create an iterator for the space to iterate.
ObjectIterator* SpaceIterator::CreateIterator() {
  ASSERT(iterator_ == NULL);

  switch (current_space_) {
    case NEW_SPACE:
      iterator_ = new SemiSpaceIterator(HEAP->new_space(), size_func_);
      break;
    case OLD_POINTER_SPACE:
      iterator_ = new HeapObjectIterator(HEAP->old_pointer_space(), size_func_);
      break;
    case OLD_DATA_SPACE:
      iterator_ = new HeapObjectIterator(HEAP->old_data_space(), size_func_);
      break;
    case CODE_SPACE:
      iterator_ = new HeapObjectIterator(HEAP->code_space(), size_func_);
      break;
    case MAP_SPACE:
      iterator_ = new HeapObjectIterator(HEAP->map_space(), size_func_);
      break;
    case CELL_SPACE:
      iterator_ = new HeapObjectIterator(HEAP->cell_space(), size_func_);
      break;
    case LO_SPACE:
      iterator_ = new LargeObjectIterator(HEAP->lo_space(), size_func_);
      break;
  }

  // Return the newly allocated iterator;
  ASSERT(iterator_ != NULL);
  return iterator_;
}


class HeapObjectsFilter {
 public:
  virtual ~HeapObjectsFilter() {}
  virtual bool SkipObject(HeapObject* object) = 0;
};


class UnreachableObjectsFilter : public HeapObjectsFilter {
 public:
  UnreachableObjectsFilter() {
    MarkReachableObjects();
  }

  ~UnreachableObjectsFilter() {
    Isolate::Current()->heap()->mark_compact_collector()->ClearMarkbits();
  }

  bool SkipObject(HeapObject* object) {
    MarkBit mark_bit = Marking::MarkBitFrom(object);
    return !mark_bit.Get();
  }

 private:
  class MarkingVisitor : public ObjectVisitor {
   public:
    MarkingVisitor() : marking_stack_(10) {}

    void VisitPointers(Object** start, Object** end) {
      for (Object** p = start; p < end; p++) {
        if (!(*p)->IsHeapObject()) continue;
        HeapObject* obj = HeapObject::cast(*p);
        MarkBit mark_bit = Marking::MarkBitFrom(obj);
        if (!mark_bit.Get()) {
          mark_bit.Set();
          marking_stack_.Add(obj);
        }
      }
    }

    void TransitiveClosure() {
      while (!marking_stack_.is_empty()) {
        HeapObject* obj = marking_stack_.RemoveLast();
        obj->Iterate(this);
      }
    }

   private:
    List<HeapObject*> marking_stack_;
  };

  void MarkReachableObjects() {
    Heap* heap = Isolate::Current()->heap();
    MarkingVisitor visitor;
    heap->IterateRoots(&visitor, VISIT_ALL);
    visitor.TransitiveClosure();
  }

  AssertNoAllocation no_alloc;
};


HeapIterator::HeapIterator()
    : filtering_(HeapIterator::kNoFiltering),
      filter_(NULL) {
  Init();
}


HeapIterator::HeapIterator(HeapIterator::HeapObjectsFiltering filtering)
    : filtering_(filtering),
      filter_(NULL) {
  Init();
}


HeapIterator::~HeapIterator() {
  Shutdown();
}


void HeapIterator::Init() {
  // Start the iteration.
  space_iterator_ = new SpaceIterator;
  switch (filtering_) {
    case kFilterUnreachable:
      filter_ = new UnreachableObjectsFilter;
      break;
    default:
      break;
  }
  object_iterator_ = space_iterator_->next();
}


void HeapIterator::Shutdown() {
#ifdef DEBUG
  // Assert that in filtering mode we have iterated through all
  // objects. Otherwise, heap will be left in an inconsistent state.
  if (filtering_ != kNoFiltering) {
    ASSERT(object_iterator_ == NULL);
  }
#endif
  // Make sure the last iterator is deallocated.
  delete space_iterator_;
  space_iterator_ = NULL;
  object_iterator_ = NULL;
  delete filter_;
  filter_ = NULL;
}


HeapObject* HeapIterator::next() {
  if (filter_ == NULL) return NextObject();

  HeapObject* obj = NextObject();
  while (obj != NULL && filter_->SkipObject(obj)) obj = NextObject();
  return obj;
}


HeapObject* HeapIterator::NextObject() {
  // No iterator means we are done.
  if (object_iterator_ == NULL) return NULL;

  if (HeapObject* obj = object_iterator_->next_object()) {
    // If the current iterator has more objects we are fine.
    return obj;
  } else {
    // Go though the spaces looking for one that has objects.
    while (space_iterator_->has_next()) {
      object_iterator_ = space_iterator_->next();
      if (HeapObject* obj = object_iterator_->next_object()) {
        return obj;
      }
    }
  }
  // Done with the last space.
  object_iterator_ = NULL;
  return NULL;
}


void HeapIterator::reset() {
  // Restart the iterator.
  Shutdown();
  Init();
}


#if defined(DEBUG) || defined(LIVE_OBJECT_LIST)

Object* const PathTracer::kAnyGlobalObject = reinterpret_cast<Object*>(NULL);

class PathTracer::MarkVisitor: public ObjectVisitor {
 public:
  explicit MarkVisitor(PathTracer* tracer) : tracer_(tracer) {}
  void VisitPointers(Object** start, Object** end) {
    // Scan all HeapObject pointers in [start, end)
    for (Object** p = start; !tracer_->found() && (p < end); p++) {
      if ((*p)->IsHeapObject())
        tracer_->MarkRecursively(p, this);
    }
  }

 private:
  PathTracer* tracer_;
};


class PathTracer::UnmarkVisitor: public ObjectVisitor {
 public:
  explicit UnmarkVisitor(PathTracer* tracer) : tracer_(tracer) {}
  void VisitPointers(Object** start, Object** end) {
    // Scan all HeapObject pointers in [start, end)
    for (Object** p = start; p < end; p++) {
      if ((*p)->IsHeapObject())
        tracer_->UnmarkRecursively(p, this);
    }
  }

 private:
  PathTracer* tracer_;
};


void PathTracer::VisitPointers(Object** start, Object** end) {
  bool done = ((what_to_find_ == FIND_FIRST) && found_target_);
  // Visit all HeapObject pointers in [start, end)
  for (Object** p = start; !done && (p < end); p++) {
    if ((*p)->IsHeapObject()) {
      TracePathFrom(p);
      done = ((what_to_find_ == FIND_FIRST) && found_target_);
    }
  }
}


void PathTracer::Reset() {
  found_target_ = false;
  object_stack_.Clear();
}


void PathTracer::TracePathFrom(Object** root) {
  ASSERT((search_target_ == kAnyGlobalObject) ||
         search_target_->IsHeapObject());
  found_target_in_trace_ = false;
  Reset();

  MarkVisitor mark_visitor(this);
  MarkRecursively(root, &mark_visitor);

  UnmarkVisitor unmark_visitor(this);
  UnmarkRecursively(root, &unmark_visitor);

  ProcessResults();
}


static bool SafeIsNativeContext(HeapObject* obj) {
  return obj->map() == obj->GetHeap()->raw_unchecked_native_context_map();
}


void PathTracer::MarkRecursively(Object** p, MarkVisitor* mark_visitor) {
  if (!(*p)->IsHeapObject()) return;

  HeapObject* obj = HeapObject::cast(*p);

  Object* map = obj->map();

  if (!map->IsHeapObject()) return;  // visited before

  if (found_target_in_trace_) return;  // stop if target found
  object_stack_.Add(obj);
  if (((search_target_ == kAnyGlobalObject) && obj->IsJSGlobalObject()) ||
      (obj == search_target_)) {
    found_target_in_trace_ = true;
    found_target_ = true;
    return;
  }

  bool is_native_context = SafeIsNativeContext(obj);

  // not visited yet
  Map* map_p = reinterpret_cast<Map*>(HeapObject::cast(map));

  Address map_addr = map_p->address();

  obj->set_map_no_write_barrier(reinterpret_cast<Map*>(map_addr + kMarkTag));

  // Scan the object body.
  if (is_native_context && (visit_mode_ == VISIT_ONLY_STRONG)) {
    // This is specialized to scan Context's properly.
    Object** start = reinterpret_cast<Object**>(obj->address() +
                                                Context::kHeaderSize);
    Object** end = reinterpret_cast<Object**>(obj->address() +
        Context::kHeaderSize + Context::FIRST_WEAK_SLOT * kPointerSize);
    mark_visitor->VisitPointers(start, end);
  } else {
    obj->IterateBody(map_p->instance_type(),
                     obj->SizeFromMap(map_p),
                     mark_visitor);
  }

  // Scan the map after the body because the body is a lot more interesting
  // when doing leak detection.
  MarkRecursively(&map, mark_visitor);

  if (!found_target_in_trace_)  // don't pop if found the target
    object_stack_.RemoveLast();
}


void PathTracer::UnmarkRecursively(Object** p, UnmarkVisitor* unmark_visitor) {
  if (!(*p)->IsHeapObject()) return;

  HeapObject* obj = HeapObject::cast(*p);

  Object* map = obj->map();

  if (map->IsHeapObject()) return;  // unmarked already

  Address map_addr = reinterpret_cast<Address>(map);

  map_addr -= kMarkTag;

  ASSERT_TAG_ALIGNED(map_addr);

  HeapObject* map_p = HeapObject::FromAddress(map_addr);

  obj->set_map_no_write_barrier(reinterpret_cast<Map*>(map_p));

  UnmarkRecursively(reinterpret_cast<Object**>(&map_p), unmark_visitor);

  obj->IterateBody(Map::cast(map_p)->instance_type(),
                   obj->SizeFromMap(Map::cast(map_p)),
                   unmark_visitor);
}


void PathTracer::ProcessResults() {
  if (found_target_) {
    PrintF("=====================================\n");
    PrintF("====        Path to object       ====\n");
    PrintF("=====================================\n\n");

    ASSERT(!object_stack_.is_empty());
    for (int i = 0; i < object_stack_.length(); i++) {
      if (i > 0) PrintF("\n     |\n     |\n     V\n\n");
      Object* obj = object_stack_[i];
      obj->Print();
    }
    PrintF("=====================================\n");
  }
}
#endif  // DEBUG || LIVE_OBJECT_LIST


#ifdef DEBUG
// Triggers a depth-first traversal of reachable objects from one
// given root object and finds a path to a specific heap object and
// prints it.
void Heap::TracePathToObjectFrom(Object* target, Object* root) {
  PathTracer tracer(target, PathTracer::FIND_ALL, VISIT_ALL);
  tracer.VisitPointer(&root);
}


// Triggers a depth-first traversal of reachable objects from roots
// and finds a path to a specific heap object and prints it.
void Heap::TracePathToObject(Object* target) {
  PathTracer tracer(target, PathTracer::FIND_ALL, VISIT_ALL);
  IterateRoots(&tracer, VISIT_ONLY_STRONG);
}


// Triggers a depth-first traversal of reachable objects from roots
// and finds a path to any global object and prints it. Useful for
// determining the source for leaks of global objects.
void Heap::TracePathToGlobal() {
  PathTracer tracer(PathTracer::kAnyGlobalObject,
                    PathTracer::FIND_ALL,
                    VISIT_ALL);
  IterateRoots(&tracer, VISIT_ONLY_STRONG);
}
#endif


static intptr_t CountTotalHolesSize() {
  intptr_t holes_size = 0;
  OldSpaces spaces;
  for (OldSpace* space = spaces.next();
       space != NULL;
       space = spaces.next()) {
    holes_size += space->Waste() + space->Available();
  }
  return holes_size;
}


GCTracer::GCTracer(Heap* heap,
                   const char* gc_reason,
                   const char* collector_reason)
    : start_time_(0.0),
      start_object_size_(0),
      start_memory_size_(0),
      gc_count_(0),
      full_gc_count_(0),
      allocated_since_last_gc_(0),
      spent_in_mutator_(0),
      promoted_objects_size_(0),
      heap_(heap),
      gc_reason_(gc_reason),
      collector_reason_(collector_reason) {
  if (!FLAG_trace_gc && !FLAG_print_cumulative_gc_stat) return;
  start_time_ = OS::TimeCurrentMillis();
  start_object_size_ = heap_->SizeOfObjects();
  start_memory_size_ = heap_->isolate()->memory_allocator()->Size();

  for (int i = 0; i < Scope::kNumberOfScopes; i++) {
    scopes_[i] = 0;
  }

  in_free_list_or_wasted_before_gc_ = CountTotalHolesSize();

  allocated_since_last_gc_ =
      heap_->SizeOfObjects() - heap_->alive_after_last_gc_;

  if (heap_->last_gc_end_timestamp_ > 0) {
    spent_in_mutator_ = Max(start_time_ - heap_->last_gc_end_timestamp_, 0.0);
  }

  steps_count_ = heap_->incremental_marking()->steps_count();
  steps_took_ = heap_->incremental_marking()->steps_took();
  longest_step_ = heap_->incremental_marking()->longest_step();
  steps_count_since_last_gc_ =
      heap_->incremental_marking()->steps_count_since_last_gc();
  steps_took_since_last_gc_ =
      heap_->incremental_marking()->steps_took_since_last_gc();
}


GCTracer::~GCTracer() {
  // Printf ONE line iff flag is set.
  if (!FLAG_trace_gc && !FLAG_print_cumulative_gc_stat) return;

  bool first_gc = (heap_->last_gc_end_timestamp_ == 0);

  heap_->alive_after_last_gc_ = heap_->SizeOfObjects();
  heap_->last_gc_end_timestamp_ = OS::TimeCurrentMillis();

  int time = static_cast<int>(heap_->last_gc_end_timestamp_ - start_time_);

  // Update cumulative GC statistics if required.
  if (FLAG_print_cumulative_gc_stat) {
    heap_->total_gc_time_ms_ += time;
    heap_->max_gc_pause_ = Max(heap_->max_gc_pause_, time);
    heap_->max_alive_after_gc_ = Max(heap_->max_alive_after_gc_,
                                     heap_->alive_after_last_gc_);
    if (!first_gc) {
      heap_->min_in_mutator_ = Min(heap_->min_in_mutator_,
                                   static_cast<int>(spent_in_mutator_));
    }
  } else if (FLAG_trace_gc_verbose) {
    heap_->total_gc_time_ms_ += time;
  }

  if (collector_ == SCAVENGER && FLAG_trace_gc_ignore_scavenger) return;

  PrintPID("%8.0f ms: ", heap_->isolate()->time_millis_since_init());

  if (!FLAG_trace_gc_nvp) {
    int external_time = static_cast<int>(scopes_[Scope::EXTERNAL]);

    double end_memory_size_mb =
        static_cast<double>(heap_->isolate()->memory_allocator()->Size()) / MB;

    PrintF("%s %.1f (%.1f) -> %.1f (%.1f) MB, ",
           CollectorString(),
           static_cast<double>(start_object_size_) / MB,
           static_cast<double>(start_memory_size_) / MB,
           SizeOfHeapObjects(),
           end_memory_size_mb);

    if (external_time > 0) PrintF("%d / ", external_time);
    PrintF("%d ms", time);
    if (steps_count_ > 0) {
      if (collector_ == SCAVENGER) {
        PrintF(" (+ %d ms in %d steps since last GC)",
               static_cast<int>(steps_took_since_last_gc_),
               steps_count_since_last_gc_);
      } else {
        PrintF(" (+ %d ms in %d steps since start of marking, "
                   "biggest step %f ms)",
               static_cast<int>(steps_took_),
               steps_count_,
               longest_step_);
      }
    }

    if (gc_reason_ != NULL) {
      PrintF(" [%s]", gc_reason_);
    }

    if (collector_reason_ != NULL) {
      PrintF(" [%s]", collector_reason_);
    }

    PrintF(".\n");
  } else {
    PrintF("pause=%d ", time);
    PrintF("mutator=%d ", static_cast<int>(spent_in_mutator_));
    PrintF("gc=");
    switch (collector_) {
      case SCAVENGER:
        PrintF("s");
        break;
      case MARK_COMPACTOR:
        PrintF("ms");
        break;
      default:
        UNREACHABLE();
    }
    PrintF(" ");

    PrintF("external=%d ", static_cast<int>(scopes_[Scope::EXTERNAL]));
    PrintF("mark=%d ", static_cast<int>(scopes_[Scope::MC_MARK]));
    PrintF("sweep=%d ", static_cast<int>(scopes_[Scope::MC_SWEEP]));
    PrintF("sweepns=%d ", static_cast<int>(scopes_[Scope::MC_SWEEP_NEWSPACE]));
    PrintF("evacuate=%d ", static_cast<int>(scopes_[Scope::MC_EVACUATE_PAGES]));
    PrintF("new_new=%d ",
           static_cast<int>(scopes_[Scope::MC_UPDATE_NEW_TO_NEW_POINTERS]));
    PrintF("root_new=%d ",
           static_cast<int>(scopes_[Scope::MC_UPDATE_ROOT_TO_NEW_POINTERS]));
    PrintF("old_new=%d ",
           static_cast<int>(scopes_[Scope::MC_UPDATE_OLD_TO_NEW_POINTERS]));
    PrintF("compaction_ptrs=%d ",
           static_cast<int>(scopes_[Scope::MC_UPDATE_POINTERS_TO_EVACUATED]));
    PrintF("intracompaction_ptrs=%d ", static_cast<int>(scopes_[
        Scope::MC_UPDATE_POINTERS_BETWEEN_EVACUATED]));
    PrintF("misc_compaction=%d ",
           static_cast<int>(scopes_[Scope::MC_UPDATE_MISC_POINTERS]));

    PrintF("total_size_before=%" V8_PTR_PREFIX "d ", start_object_size_);
    PrintF("total_size_after=%" V8_PTR_PREFIX "d ", heap_->SizeOfObjects());
    PrintF("holes_size_before=%" V8_PTR_PREFIX "d ",
           in_free_list_or_wasted_before_gc_);
    PrintF("holes_size_after=%" V8_PTR_PREFIX "d ", CountTotalHolesSize());

    PrintF("allocated=%" V8_PTR_PREFIX "d ", allocated_since_last_gc_);
    PrintF("promoted=%" V8_PTR_PREFIX "d ", promoted_objects_size_);

    if (collector_ == SCAVENGER) {
      PrintF("stepscount=%d ", steps_count_since_last_gc_);
      PrintF("stepstook=%d ", static_cast<int>(steps_took_since_last_gc_));
    } else {
      PrintF("stepscount=%d ", steps_count_);
      PrintF("stepstook=%d ", static_cast<int>(steps_took_));
    }

    PrintF("\n");
  }

  heap_->PrintShortHeapStatistics();
}


const char* GCTracer::CollectorString() {
  switch (collector_) {
    case SCAVENGER:
      return "Scavenge";
    case MARK_COMPACTOR:
      return "Mark-sweep";
  }
  return "Unknown GC";
}


int KeyedLookupCache::Hash(Map* map, String* name) {
  // Uses only lower 32 bits if pointers are larger.
  uintptr_t addr_hash =
      static_cast<uint32_t>(reinterpret_cast<uintptr_t>(map)) >> kMapHashShift;
  return static_cast<uint32_t>((addr_hash ^ name->Hash()) & kCapacityMask);
}


int KeyedLookupCache::Lookup(Map* map, String* name) {
  int index = (Hash(map, name) & kHashMask);
  for (int i = 0; i < kEntriesPerBucket; i++) {
    Key& key = keys_[index + i];
    if ((key.map == map) && key.name->Equals(name)) {
      return field_offsets_[index + i];
    }
  }
  return kNotFound;
}


void KeyedLookupCache::Update(Map* map, String* name, int field_offset) {
  String* symbol;
  if (HEAP->LookupSymbolIfExists(name, &symbol)) {
    int index = (Hash(map, symbol) & kHashMask);
    // After a GC there will be free slots, so we use them in order (this may
    // help to get the most frequently used one in position 0).
    for (int i = 0; i< kEntriesPerBucket; i++) {
      Key& key = keys_[index];
      Object* free_entry_indicator = NULL;
      if (key.map == free_entry_indicator) {
        key.map = map;
        key.name = symbol;
        field_offsets_[index + i] = field_offset;
        return;
      }
    }
    // No free entry found in this bucket, so we move them all down one and
    // put the new entry at position zero.
    for (int i = kEntriesPerBucket - 1; i > 0; i--) {
      Key& key = keys_[index + i];
      Key& key2 = keys_[index + i - 1];
      key = key2;
      field_offsets_[index + i] = field_offsets_[index + i - 1];
    }

    // Write the new first entry.
    Key& key = keys_[index];
    key.map = map;
    key.name = symbol;
    field_offsets_[index] = field_offset;
  }
}


void KeyedLookupCache::Clear() {
  for (int index = 0; index < kLength; index++) keys_[index].map = NULL;
}


void DescriptorLookupCache::Clear() {
  for (int index = 0; index < kLength; index++) keys_[index].source = NULL;
}


#ifdef DEBUG
void Heap::GarbageCollectionGreedyCheck() {
  ASSERT(FLAG_gc_greedy);
  if (isolate_->bootstrapper()->IsActive()) return;
  if (disallow_allocation_failure()) return;
  CollectGarbage(NEW_SPACE);
}
#endif


TranscendentalCache::SubCache::SubCache(Type t)
  : type_(t),
    isolate_(Isolate::Current()) {
  uint32_t in0 = 0xffffffffu;  // Bit-pattern for a NaN that isn't
  uint32_t in1 = 0xffffffffu;  // generated by the FPU.
  for (int i = 0; i < kCacheSize; i++) {
    elements_[i].in[0] = in0;
    elements_[i].in[1] = in1;
    elements_[i].output = NULL;
  }
}


void TranscendentalCache::Clear() {
  for (int i = 0; i < kNumberOfCaches; i++) {
    if (caches_[i] != NULL) {
      delete caches_[i];
      caches_[i] = NULL;
    }
  }
}


void ExternalStringTable::CleanUp() {
  int last = 0;
  for (int i = 0; i < new_space_strings_.length(); ++i) {
    if (new_space_strings_[i] == heap_->raw_unchecked_the_hole_value()) {
      continue;
    }
    if (heap_->InNewSpace(new_space_strings_[i])) {
      new_space_strings_[last++] = new_space_strings_[i];
    } else {
      old_space_strings_.Add(new_space_strings_[i]);
    }
  }
  new_space_strings_.Rewind(last);
  last = 0;
  for (int i = 0; i < old_space_strings_.length(); ++i) {
    if (old_space_strings_[i] == heap_->raw_unchecked_the_hole_value()) {
      continue;
    }
    ASSERT(!heap_->InNewSpace(old_space_strings_[i]));
    old_space_strings_[last++] = old_space_strings_[i];
  }
  old_space_strings_.Rewind(last);
#ifdef VERIFY_HEAP
  if (FLAG_verify_heap) {
    Verify();
  }
#endif
}


void ExternalStringTable::TearDown() {
  new_space_strings_.Free();
  old_space_strings_.Free();
}


void Heap::QueueMemoryChunkForFree(MemoryChunk* chunk) {
  chunk->set_next_chunk(chunks_queued_for_free_);
  chunks_queued_for_free_ = chunk;
}


void Heap::FreeQueuedChunks() {
  if (chunks_queued_for_free_ == NULL) return;
  MemoryChunk* next;
  MemoryChunk* chunk;
  for (chunk = chunks_queued_for_free_; chunk != NULL; chunk = next) {
    next = chunk->next_chunk();
    chunk->SetFlag(MemoryChunk::ABOUT_TO_BE_FREED);

    if (chunk->owner()->identity() == LO_SPACE) {
      // StoreBuffer::Filter relies on MemoryChunk::FromAnyPointerAddress.
      // If FromAnyPointerAddress encounters a slot that belongs to a large
      // chunk queued for deletion it will fail to find the chunk because
      // it try to perform a search in the list of pages owned by of the large
      // object space and queued chunks were detached from that list.
      // To work around this we split large chunk into normal kPageSize aligned
      // pieces and initialize size, owner and flags field of every piece.
      // If FromAnyPointerAddress encounters a slot that belongs to one of
      // these smaller pieces it will treat it as a slot on a normal Page.
      Address chunk_end = chunk->address() + chunk->size();
      MemoryChunk* inner = MemoryChunk::FromAddress(
          chunk->address() + Page::kPageSize);
      MemoryChunk* inner_last = MemoryChunk::FromAddress(chunk_end - 1);
      while (inner <= inner_last) {
        // Size of a large chunk is always a multiple of
        // OS::AllocateAlignment() so there is always
        // enough space for a fake MemoryChunk header.
        Address area_end = Min(inner->address() + Page::kPageSize, chunk_end);
        // Guard against overflow.
        if (area_end < inner->address()) area_end = chunk_end;
        inner->SetArea(inner->address(), area_end);
        inner->set_size(Page::kPageSize);
        inner->set_owner(lo_space());
        inner->SetFlag(MemoryChunk::ABOUT_TO_BE_FREED);
        inner = MemoryChunk::FromAddress(
            inner->address() + Page::kPageSize);
      }
    }
  }
  isolate_->heap()->store_buffer()->Compact();
  isolate_->heap()->store_buffer()->Filter(MemoryChunk::ABOUT_TO_BE_FREED);
  for (chunk = chunks_queued_for_free_; chunk != NULL; chunk = next) {
    next = chunk->next_chunk();
    isolate_->memory_allocator()->Free(chunk);
  }
  chunks_queued_for_free_ = NULL;
}


void Heap::RememberUnmappedPage(Address page, bool compacted) {
  uintptr_t p = reinterpret_cast<uintptr_t>(page);
  // Tag the page pointer to make it findable in the dump file.
  if (compacted) {
    p ^= 0xc1ead & (Page::kPageSize - 1);  // Cleared.
  } else {
    p ^= 0x1d1ed & (Page::kPageSize - 1);  // I died.
  }
  remembered_unmapped_pages_[remembered_unmapped_pages_index_] =
      reinterpret_cast<Address>(p);
  remembered_unmapped_pages_index_++;
  remembered_unmapped_pages_index_ %= kRememberedUnmappedPages;
}


void Heap::ClearObjectStats(bool clear_last_time_stats) {
  memset(object_counts_, 0, sizeof(object_counts_));
  memset(object_sizes_, 0, sizeof(object_sizes_));
  if (clear_last_time_stats) {
    memset(object_counts_last_time_, 0, sizeof(object_counts_last_time_));
    memset(object_sizes_last_time_, 0, sizeof(object_sizes_last_time_));
  }
}


static LazyMutex checkpoint_object_stats_mutex = LAZY_MUTEX_INITIALIZER;


void Heap::CheckpointObjectStats() {
  ScopedLock lock(checkpoint_object_stats_mutex.Pointer());
  Counters* counters = isolate()->counters();
#define ADJUST_LAST_TIME_OBJECT_COUNT(name)                                    \
  counters->count_of_##name()->Increment(                                      \
      static_cast<int>(object_counts_[name]));                                 \
  counters->count_of_##name()->Decrement(                                      \
      static_cast<int>(object_counts_last_time_[name]));                       \
  counters->size_of_##name()->Increment(                                       \
      static_cast<int>(object_sizes_[name]));                                  \
  counters->size_of_##name()->Decrement(                                       \
      static_cast<int>(object_sizes_last_time_[name]));
  INSTANCE_TYPE_LIST(ADJUST_LAST_TIME_OBJECT_COUNT)
#undef ADJUST_LAST_TIME_OBJECT_COUNT
  int index;
#define ADJUST_LAST_TIME_OBJECT_COUNT(name)               \
  index = FIRST_CODE_KIND_SUB_TYPE + Code::name;          \
  counters->count_of_CODE_TYPE_##name()->Increment(       \
      static_cast<int>(object_counts_[index]));           \
  counters->count_of_CODE_TYPE_##name()->Decrement(       \
      static_cast<int>(object_counts_last_time_[index])); \
  counters->size_of_CODE_TYPE_##name()->Increment(        \
      static_cast<int>(object_sizes_[index]));            \
  counters->size_of_CODE_TYPE_##name()->Decrement(        \
      static_cast<int>(object_sizes_last_time_[index]));
  CODE_KIND_LIST(ADJUST_LAST_TIME_OBJECT_COUNT)
#undef ADJUST_LAST_TIME_OBJECT_COUNT
#define ADJUST_LAST_TIME_OBJECT_COUNT(name)               \
  index = FIRST_FIXED_ARRAY_SUB_TYPE + name;              \
  counters->count_of_FIXED_ARRAY_##name()->Increment(     \
      static_cast<int>(object_counts_[index]));           \
  counters->count_of_FIXED_ARRAY_##name()->Decrement(     \
      static_cast<int>(object_counts_last_time_[index])); \
  counters->size_of_FIXED_ARRAY_##name()->Increment(      \
      static_cast<int>(object_sizes_[index]));            \
  counters->size_of_FIXED_ARRAY_##name()->Decrement(      \
      static_cast<int>(object_sizes_last_time_[index]));
  FIXED_ARRAY_SUB_INSTANCE_TYPE_LIST(ADJUST_LAST_TIME_OBJECT_COUNT)
#undef ADJUST_LAST_TIME_OBJECT_COUNT

  memcpy(object_counts_last_time_, object_counts_, sizeof(object_counts_));
  memcpy(object_sizes_last_time_, object_sizes_, sizeof(object_sizes_));
  ClearObjectStats();
}

} }  // namespace v8::internal
