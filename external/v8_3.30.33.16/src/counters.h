// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_COUNTERS_H_
#define V8_COUNTERS_H_

#include "include/v8.h"
#include "src/allocation.h"
#include "src/base/platform/elapsed-timer.h"
#include "src/globals.h"
#include "src/objects.h"

namespace v8 {
namespace internal {

// StatsCounters is an interface for plugging into external
// counters for monitoring.  Counters can be looked up and
// manipulated by name.

class StatsTable {
 public:
  // Register an application-defined function where
  // counters can be looked up.
  void SetCounterFunction(CounterLookupCallback f) {
    lookup_function_ = f;
  }

  // Register an application-defined function to create
  // a histogram for passing to the AddHistogramSample function
  void SetCreateHistogramFunction(CreateHistogramCallback f) {
    create_histogram_function_ = f;
  }

  // Register an application-defined function to add a sample
  // to a histogram created with CreateHistogram function
  void SetAddHistogramSampleFunction(AddHistogramSampleCallback f) {
    add_histogram_sample_function_ = f;
  }

  bool HasCounterFunction() const {
    return lookup_function_ != NULL;
  }

  // Lookup the location of a counter by name.  If the lookup
  // is successful, returns a non-NULL pointer for writing the
  // value of the counter.  Each thread calling this function
  // may receive a different location to store it's counter.
  // The return value must not be cached and re-used across
  // threads, although a single thread is free to cache it.
  int* FindLocation(const char* name) {
    if (!lookup_function_) return NULL;
    return lookup_function_(name);
  }

  // Create a histogram by name. If the create is successful,
  // returns a non-NULL pointer for use with AddHistogramSample
  // function. min and max define the expected minimum and maximum
  // sample values. buckets is the maximum number of buckets
  // that the samples will be grouped into.
  void* CreateHistogram(const char* name,
                        int min,
                        int max,
                        size_t buckets) {
    if (!create_histogram_function_) return NULL;
    return create_histogram_function_(name, min, max, buckets);
  }

  // Add a sample to a histogram created with the CreateHistogram
  // function.
  void AddHistogramSample(void* histogram, int sample) {
    if (!add_histogram_sample_function_) return;
    return add_histogram_sample_function_(histogram, sample);
  }

 private:
  StatsTable();

  CounterLookupCallback lookup_function_;
  CreateHistogramCallback create_histogram_function_;
  AddHistogramSampleCallback add_histogram_sample_function_;

  friend class Isolate;

  DISALLOW_COPY_AND_ASSIGN(StatsTable);
};

// StatsCounters are dynamically created values which can be tracked in
// the StatsTable.  They are designed to be lightweight to create and
// easy to use.
//
// Internally, a counter represents a value in a row of a StatsTable.
// The row has a 32bit value for each process/thread in the table and also
// a name (stored in the table metadata).  Since the storage location can be
// thread-specific, this class cannot be shared across threads.
class StatsCounter {
 public:
  StatsCounter() { }
  explicit StatsCounter(Isolate* isolate, const char* name)
      : isolate_(isolate), name_(name), ptr_(NULL), lookup_done_(false) { }

  // Sets the counter to a specific value.
  void Set(int value) {
    int* loc = GetPtr();
    if (loc) *loc = value;
  }

  // Increments the counter.
  void Increment() {
    int* loc = GetPtr();
    if (loc) (*loc)++;
  }

  void Increment(int value) {
    int* loc = GetPtr();
    if (loc)
      (*loc) += value;
  }

  // Decrements the counter.
  void Decrement() {
    int* loc = GetPtr();
    if (loc) (*loc)--;
  }

  void Decrement(int value) {
    int* loc = GetPtr();
    if (loc) (*loc) -= value;
  }

  // Is this counter enabled?
  // Returns false if table is full.
  bool Enabled() {
    return GetPtr() != NULL;
  }

  // Get the internal pointer to the counter. This is used
  // by the code generator to emit code that manipulates a
  // given counter without calling the runtime system.
  int* GetInternalPointer() {
    int* loc = GetPtr();
    DCHECK(loc != NULL);
    return loc;
  }

  // Reset the cached internal pointer.
  void Reset() { lookup_done_ = false; }

 protected:
  // Returns the cached address of this counter location.
  int* GetPtr() {
    if (lookup_done_) return ptr_;
    lookup_done_ = true;
    ptr_ = FindLocationInStatsTable();
    return ptr_;
  }

 private:
  int* FindLocationInStatsTable() const;

  Isolate* isolate_;
  const char* name_;
  int* ptr_;
  bool lookup_done_;
};

// A Histogram represents a dynamically created histogram in the StatsTable.
// It will be registered with the histogram system on first use.
class Histogram {
 public:
  Histogram() { }
  Histogram(const char* name,
            int min,
            int max,
            int num_buckets,
            Isolate* isolate)
      : name_(name),
        min_(min),
        max_(max),
        num_buckets_(num_buckets),
        histogram_(NULL),
        lookup_done_(false),
        isolate_(isolate) { }

  // Add a single sample to this histogram.
  void AddSample(int sample);

  // Returns true if this histogram is enabled.
  bool Enabled() {
    return GetHistogram() != NULL;
  }

  // Reset the cached internal pointer.
  void Reset() {
    lookup_done_ = false;
  }

 protected:
  // Returns the handle to the histogram.
  void* GetHistogram() {
    if (!lookup_done_) {
      lookup_done_ = true;
      histogram_ = CreateHistogram();
    }
    return histogram_;
  }

  const char* name() { return name_; }
  Isolate* isolate() const { return isolate_; }

 private:
  void* CreateHistogram() const;

  const char* name_;
  int min_;
  int max_;
  int num_buckets_;
  void* histogram_;
  bool lookup_done_;
  Isolate* isolate_;
};

// A HistogramTimer allows distributions of results to be created.
class HistogramTimer : public Histogram {
 public:
  HistogramTimer() { }
  HistogramTimer(const char* name,
                 int min,
                 int max,
                 int num_buckets,
                 Isolate* isolate)
      : Histogram(name, min, max, num_buckets, isolate) {}

  // Start the timer.
  void Start();

  // Stop the timer and record the results.
  void Stop();

  // Returns true if the timer is running.
  bool Running() {
    return Enabled() && timer_.IsStarted();
  }

  // TODO(bmeurer): Remove this when HistogramTimerScope is fixed.
#ifdef DEBUG
  base::ElapsedTimer* timer() { return &timer_; }
#endif

 private:
  base::ElapsedTimer timer_;
};

// Helper class for scoping a HistogramTimer.
// TODO(bmeurer): The ifdeffery is an ugly hack around the fact that the
// Parser is currently reentrant (when it throws an error, we call back
// into JavaScript and all bets are off), but ElapsedTimer is not
// reentry-safe. Fix this properly and remove |allow_nesting|.
class HistogramTimerScope BASE_EMBEDDED {
 public:
  explicit HistogramTimerScope(HistogramTimer* timer,
                               bool allow_nesting = false)
#ifdef DEBUG
      : timer_(timer),
        skipped_timer_start_(false) {
    if (timer_->timer()->IsStarted() && allow_nesting) {
      skipped_timer_start_ = true;
    } else {
      timer_->Start();
    }
  }
#else
      : timer_(timer) {
    timer_->Start();
  }
#endif
  ~HistogramTimerScope() {
#ifdef DEBUG
    if (!skipped_timer_start_) {
      timer_->Stop();
    }
#else
    timer_->Stop();
#endif
  }

 private:
  HistogramTimer* timer_;
#ifdef DEBUG
  bool skipped_timer_start_;
#endif
};

#define HISTOGRAM_RANGE_LIST(HR)                                              \
  /* Generic range histograms */                                              \
  HR(gc_idle_time_allotted_in_ms, V8.GCIdleTimeAllottedInMS, 0, 10000, 101)   \
  HR(gc_idle_time_limit_overshot, V8.GCIdleTimeLimit.Overshot, 0, 10000, 101) \
  HR(gc_idle_time_limit_undershot, V8.GCIdleTimeLimit.Undershot, 0, 10000, 101)

#define HISTOGRAM_TIMER_LIST(HT)                             \
  /* Garbage collection timers. */                           \
  HT(gc_compactor, V8.GCCompactor)                           \
  HT(gc_scavenger, V8.GCScavenger)                           \
  HT(gc_context, V8.GCContext) /* GC context cleanup time */ \
  HT(gc_idle_notification, V8.GCIdleNotification)            \
  HT(gc_incremental_marking, V8.GCIncrementalMarking)        \
  HT(gc_low_memory_notification, V8.GCLowMemoryNotification) \
  /* Parsing timers. */                                      \
  HT(parse, V8.Parse)                                        \
  HT(parse_lazy, V8.ParseLazy)                               \
  HT(pre_parse, V8.PreParse)                                 \
  /* Total compilation times. */                             \
  HT(compile, V8.Compile)                                    \
  HT(compile_eval, V8.CompileEval)                           \
  /* Serialization as part of compilation (code caching) */  \
  HT(compile_serialize, V8.CompileSerialize)                 \
  HT(compile_deserialize, V8.CompileDeserialize)


#define HISTOGRAM_PERCENTAGE_LIST(HP)                                 \
  /* Heap fragmentation. */                                           \
  HP(external_fragmentation_total,                                    \
     V8.MemoryExternalFragmentationTotal)                             \
  HP(external_fragmentation_old_pointer_space,                        \
     V8.MemoryExternalFragmentationOldPointerSpace)                   \
  HP(external_fragmentation_old_data_space,                           \
     V8.MemoryExternalFragmentationOldDataSpace)                      \
  HP(external_fragmentation_code_space,                               \
     V8.MemoryExternalFragmentationCodeSpace)                         \
  HP(external_fragmentation_map_space,                                \
     V8.MemoryExternalFragmentationMapSpace)                          \
  HP(external_fragmentation_cell_space,                               \
     V8.MemoryExternalFragmentationCellSpace)                         \
  HP(external_fragmentation_property_cell_space,                      \
     V8.MemoryExternalFragmentationPropertyCellSpace)                 \
  HP(external_fragmentation_lo_space,                                 \
     V8.MemoryExternalFragmentationLoSpace)                           \
  /* Percentages of heap committed to each space. */                  \
  HP(heap_fraction_new_space,                                         \
     V8.MemoryHeapFractionNewSpace)                                   \
  HP(heap_fraction_old_pointer_space,                                 \
     V8.MemoryHeapFractionOldPointerSpace)                            \
  HP(heap_fraction_old_data_space,                                    \
     V8.MemoryHeapFractionOldDataSpace)                               \
  HP(heap_fraction_code_space,                                        \
     V8.MemoryHeapFractionCodeSpace)                                  \
  HP(heap_fraction_map_space,                                         \
     V8.MemoryHeapFractionMapSpace)                                   \
  HP(heap_fraction_cell_space,                                        \
     V8.MemoryHeapFractionCellSpace)                                  \
  HP(heap_fraction_property_cell_space,                               \
     V8.MemoryHeapFractionPropertyCellSpace)                          \
  HP(heap_fraction_lo_space,                                          \
     V8.MemoryHeapFractionLoSpace)                                    \
  /* Percentage of crankshafted codegen. */                           \
  HP(codegen_fraction_crankshaft,                                     \
     V8.CodegenFractionCrankshaft)                                    \


#define HISTOGRAM_MEMORY_LIST(HM)                                     \
  HM(heap_sample_total_committed, V8.MemoryHeapSampleTotalCommitted)  \
  HM(heap_sample_total_used, V8.MemoryHeapSampleTotalUsed)            \
  HM(heap_sample_map_space_committed,                                 \
     V8.MemoryHeapSampleMapSpaceCommitted)                            \
  HM(heap_sample_cell_space_committed,                                \
     V8.MemoryHeapSampleCellSpaceCommitted)                           \
  HM(heap_sample_property_cell_space_committed,                       \
     V8.MemoryHeapSamplePropertyCellSpaceCommitted)                   \
  HM(heap_sample_code_space_committed,                                \
     V8.MemoryHeapSampleCodeSpaceCommitted)                           \
  HM(heap_sample_maximum_committed,                                   \
     V8.MemoryHeapSampleMaximumCommitted)                             \


// WARNING: STATS_COUNTER_LIST_* is a very large macro that is causing MSVC
// Intellisense to crash.  It was broken into two macros (each of length 40
// lines) rather than one macro (of length about 80 lines) to work around
// this problem.  Please avoid using recursive macros of this length when
// possible.
#define STATS_COUNTER_LIST_1(SC)                                      \
  /* Global Handle Count*/                                            \
  SC(global_handles, V8.GlobalHandles)                                \
  /* OS Memory allocated */                                           \
  SC(memory_allocated, V8.OsMemoryAllocated)                          \
  SC(normalized_maps, V8.NormalizedMaps)                              \
  SC(props_to_dictionary, V8.ObjectPropertiesToDictionary)            \
  SC(elements_to_dictionary, V8.ObjectElementsToDictionary)           \
  SC(alive_after_last_gc, V8.AliveAfterLastGC)                        \
  SC(objs_since_last_young, V8.ObjsSinceLastYoung)                    \
  SC(objs_since_last_full, V8.ObjsSinceLastFull)                      \
  SC(string_table_capacity, V8.StringTableCapacity)                   \
  SC(number_of_symbols, V8.NumberOfSymbols)                           \
  SC(script_wrappers, V8.ScriptWrappers)                              \
  SC(call_initialize_stubs, V8.CallInitializeStubs)                   \
  SC(call_premonomorphic_stubs, V8.CallPreMonomorphicStubs)           \
  SC(call_normal_stubs, V8.CallNormalStubs)                           \
  SC(call_megamorphic_stubs, V8.CallMegamorphicStubs)                 \
  SC(inlined_copied_elements, V8.InlinedCopiedElements)              \
  SC(arguments_adaptors, V8.ArgumentsAdaptors)                        \
  SC(compilation_cache_hits, V8.CompilationCacheHits)                 \
  SC(compilation_cache_misses, V8.CompilationCacheMisses)             \
  SC(string_ctor_calls, V8.StringConstructorCalls)                    \
  SC(string_ctor_conversions, V8.StringConstructorConversions)        \
  SC(string_ctor_cached_number, V8.StringConstructorCachedNumber)     \
  SC(string_ctor_string_value, V8.StringConstructorStringValue)       \
  SC(string_ctor_gc_required, V8.StringConstructorGCRequired)         \
  /* Amount of evaled source code. */                                 \
  SC(total_eval_size, V8.TotalEvalSize)                               \
  /* Amount of loaded source code. */                                 \
  SC(total_load_size, V8.TotalLoadSize)                               \
  /* Amount of parsed source code. */                                 \
  SC(total_parse_size, V8.TotalParseSize)                             \
  /* Amount of source code skipped over using preparsing. */          \
  SC(total_preparse_skipped, V8.TotalPreparseSkipped)                 \
  /* Number of symbol lookups skipped using preparsing */             \
  SC(total_preparse_symbols_skipped, V8.TotalPreparseSymbolSkipped)   \
  /* Amount of compiled source code. */                               \
  SC(total_compile_size, V8.TotalCompileSize)                         \
  /* Amount of source code compiled with the full codegen. */         \
  SC(total_full_codegen_source_size, V8.TotalFullCodegenSourceSize)   \
  /* Number of contexts created from scratch. */                      \
  SC(contexts_created_from_scratch, V8.ContextsCreatedFromScratch)    \
  /* Number of contexts created by partial snapshot. */               \
  SC(contexts_created_by_snapshot, V8.ContextsCreatedBySnapshot)      \
  /* Number of code objects found from pc. */                         \
  SC(pc_to_code, V8.PcToCode)                                         \
  SC(pc_to_code_cached, V8.PcToCodeCached)                            \
  /* The store-buffer implementation of the write barrier. */         \
  SC(store_buffer_compactions, V8.StoreBufferCompactions)             \
  SC(store_buffer_overflows, V8.StoreBufferOverflows)


#define STATS_COUNTER_LIST_2(SC)                                               \
  /* Number of code stubs. */                                                  \
  SC(code_stubs, V8.CodeStubs)                                                 \
  /* Amount of stub code. */                                                   \
  SC(total_stubs_code_size, V8.TotalStubsCodeSize)                             \
  /* Amount of (JS) compiled code. */                                          \
  SC(total_compiled_code_size, V8.TotalCompiledCodeSize)                       \
  SC(gc_compactor_caused_by_request, V8.GCCompactorCausedByRequest)            \
  SC(gc_compactor_caused_by_promoted_data, V8.GCCompactorCausedByPromotedData) \
  SC(gc_compactor_caused_by_oldspace_exhaustion,                               \
     V8.GCCompactorCausedByOldspaceExhaustion)                                 \
  SC(gc_last_resort_from_js, V8.GCLastResortFromJS)                            \
  SC(gc_last_resort_from_handles, V8.GCLastResortFromHandles)                  \
  /* How is the generic keyed-load stub used? */                               \
  SC(keyed_load_generic_smi, V8.KeyedLoadGenericSmi)                           \
  SC(keyed_load_generic_symbol, V8.KeyedLoadGenericSymbol)                     \
  SC(keyed_load_generic_lookup_cache, V8.KeyedLoadGenericLookupCache)          \
  SC(keyed_load_generic_slow, V8.KeyedLoadGenericSlow)                         \
  SC(keyed_load_polymorphic_stubs, V8.KeyedLoadPolymorphicStubs)               \
  SC(keyed_load_external_array_slow, V8.KeyedLoadExternalArraySlow)            \
  /* How is the generic keyed-call stub used? */                               \
  SC(keyed_call_generic_smi_fast, V8.KeyedCallGenericSmiFast)                  \
  SC(keyed_call_generic_smi_dict, V8.KeyedCallGenericSmiDict)                  \
  SC(keyed_call_generic_lookup_cache, V8.KeyedCallGenericLookupCache)          \
  SC(keyed_call_generic_lookup_dict, V8.KeyedCallGenericLookupDict)            \
  SC(keyed_call_generic_slow, V8.KeyedCallGenericSlow)                         \
  SC(keyed_call_generic_slow_load, V8.KeyedCallGenericSlowLoad)                \
  SC(named_load_global_stub, V8.NamedLoadGlobalStub)                           \
  SC(named_store_global_inline, V8.NamedStoreGlobalInline)                     \
  SC(named_store_global_inline_miss, V8.NamedStoreGlobalInlineMiss)            \
  SC(keyed_store_polymorphic_stubs, V8.KeyedStorePolymorphicStubs)             \
  SC(keyed_store_external_array_slow, V8.KeyedStoreExternalArraySlow)          \
  SC(store_normal_miss, V8.StoreNormalMiss)                                    \
  SC(store_normal_hit, V8.StoreNormalHit)                                      \
  SC(cow_arrays_created_stub, V8.COWArraysCreatedStub)                         \
  SC(cow_arrays_created_runtime, V8.COWArraysCreatedRuntime)                   \
  SC(cow_arrays_converted, V8.COWArraysConverted)                              \
  SC(call_miss, V8.CallMiss)                                                   \
  SC(keyed_call_miss, V8.KeyedCallMiss)                                        \
  SC(load_miss, V8.LoadMiss)                                                   \
  SC(keyed_load_miss, V8.KeyedLoadMiss)                                        \
  SC(call_const, V8.CallConst)                                                 \
  SC(call_const_fast_api, V8.CallConstFastApi)                                 \
  SC(call_const_interceptor, V8.CallConstInterceptor)                          \
  SC(call_const_interceptor_fast_api, V8.CallConstInterceptorFastApi)          \
  SC(call_global_inline, V8.CallGlobalInline)                                  \
  SC(call_global_inline_miss, V8.CallGlobalInlineMiss)                         \
  SC(constructed_objects, V8.ConstructedObjects)                               \
  SC(constructed_objects_runtime, V8.ConstructedObjectsRuntime)                \
  SC(negative_lookups, V8.NegativeLookups)                                     \
  SC(negative_lookups_miss, V8.NegativeLookupsMiss)                            \
  SC(megamorphic_stub_cache_probes, V8.MegamorphicStubCacheProbes)             \
  SC(megamorphic_stub_cache_misses, V8.MegamorphicStubCacheMisses)             \
  SC(megamorphic_stub_cache_updates, V8.MegamorphicStubCacheUpdates)           \
  SC(array_function_runtime, V8.ArrayFunctionRuntime)                          \
  SC(array_function_native, V8.ArrayFunctionNative)                            \
  SC(for_in, V8.ForIn)                                                         \
  SC(enum_cache_hits, V8.EnumCacheHits)                                        \
  SC(enum_cache_misses, V8.EnumCacheMisses)                                    \
  SC(zone_segment_bytes, V8.ZoneSegmentBytes)                                  \
  SC(fast_new_closure_total, V8.FastNewClosureTotal)                           \
  SC(fast_new_closure_try_optimized, V8.FastNewClosureTryOptimized)            \
  SC(fast_new_closure_install_optimized, V8.FastNewClosureInstallOptimized)    \
  SC(string_add_runtime, V8.StringAddRuntime)                                  \
  SC(string_add_native, V8.StringAddNative)                                    \
  SC(string_add_runtime_ext_to_one_byte, V8.StringAddRuntimeExtToOneByte)      \
  SC(sub_string_runtime, V8.SubStringRuntime)                                  \
  SC(sub_string_native, V8.SubStringNative)                                    \
  SC(string_add_make_two_char, V8.StringAddMakeTwoChar)                        \
  SC(string_compare_native, V8.StringCompareNative)                            \
  SC(string_compare_runtime, V8.StringCompareRuntime)                          \
  SC(regexp_entry_runtime, V8.RegExpEntryRuntime)                              \
  SC(regexp_entry_native, V8.RegExpEntryNative)                                \
  SC(number_to_string_native, V8.NumberToStringNative)                         \
  SC(number_to_string_runtime, V8.NumberToStringRuntime)                       \
  SC(math_acos, V8.MathAcos)                                                   \
  SC(math_asin, V8.MathAsin)                                                   \
  SC(math_atan, V8.MathAtan)                                                   \
  SC(math_atan2, V8.MathAtan2)                                                 \
  SC(math_exp, V8.MathExp)                                                     \
  SC(math_floor, V8.MathFloor)                                                 \
  SC(math_log, V8.MathLog)                                                     \
  SC(math_pow, V8.MathPow)                                                     \
  SC(math_round, V8.MathRound)                                                 \
  SC(math_sqrt, V8.MathSqrt)                                                   \
  SC(stack_interrupts, V8.StackInterrupts)                                     \
  SC(runtime_profiler_ticks, V8.RuntimeProfilerTicks)                          \
  SC(bounds_checks_eliminated, V8.BoundsChecksEliminated)                      \
  SC(bounds_checks_hoisted, V8.BoundsChecksHoisted)                            \
  SC(soft_deopts_requested, V8.SoftDeoptsRequested)                            \
  SC(soft_deopts_inserted, V8.SoftDeoptsInserted)                              \
  SC(soft_deopts_executed, V8.SoftDeoptsExecuted)                              \
  /* Number of write barriers in generated code. */                            \
  SC(write_barriers_dynamic, V8.WriteBarriersDynamic)                          \
  SC(write_barriers_static, V8.WriteBarriersStatic)                            \
  SC(new_space_bytes_available, V8.MemoryNewSpaceBytesAvailable)               \
  SC(new_space_bytes_committed, V8.MemoryNewSpaceBytesCommitted)               \
  SC(new_space_bytes_used, V8.MemoryNewSpaceBytesUsed)                         \
  SC(old_pointer_space_bytes_available,                                        \
     V8.MemoryOldPointerSpaceBytesAvailable)                                   \
  SC(old_pointer_space_bytes_committed,                                        \
     V8.MemoryOldPointerSpaceBytesCommitted)                                   \
  SC(old_pointer_space_bytes_used, V8.MemoryOldPointerSpaceBytesUsed)          \
  SC(old_data_space_bytes_available, V8.MemoryOldDataSpaceBytesAvailable)      \
  SC(old_data_space_bytes_committed, V8.MemoryOldDataSpaceBytesCommitted)      \
  SC(old_data_space_bytes_used, V8.MemoryOldDataSpaceBytesUsed)                \
  SC(code_space_bytes_available, V8.MemoryCodeSpaceBytesAvailable)             \
  SC(code_space_bytes_committed, V8.MemoryCodeSpaceBytesCommitted)             \
  SC(code_space_bytes_used, V8.MemoryCodeSpaceBytesUsed)                       \
  SC(map_space_bytes_available, V8.MemoryMapSpaceBytesAvailable)               \
  SC(map_space_bytes_committed, V8.MemoryMapSpaceBytesCommitted)               \
  SC(map_space_bytes_used, V8.MemoryMapSpaceBytesUsed)                         \
  SC(cell_space_bytes_available, V8.MemoryCellSpaceBytesAvailable)             \
  SC(cell_space_bytes_committed, V8.MemoryCellSpaceBytesCommitted)             \
  SC(cell_space_bytes_used, V8.MemoryCellSpaceBytesUsed)                       \
  SC(property_cell_space_bytes_available,                                      \
     V8.MemoryPropertyCellSpaceBytesAvailable)                                 \
  SC(property_cell_space_bytes_committed,                                      \
     V8.MemoryPropertyCellSpaceBytesCommitted)                                 \
  SC(property_cell_space_bytes_used, V8.MemoryPropertyCellSpaceBytesUsed)      \
  SC(lo_space_bytes_available, V8.MemoryLoSpaceBytesAvailable)                 \
  SC(lo_space_bytes_committed, V8.MemoryLoSpaceBytesCommitted)                 \
  SC(lo_space_bytes_used, V8.MemoryLoSpaceBytesUsed)


// This file contains all the v8 counters that are in use.
class Counters {
 public:
#define HR(name, caption, min, max, num_buckets) \
  Histogram* name() { return &name##_; }
  HISTOGRAM_RANGE_LIST(HR)
#undef HR

#define HT(name, caption) \
  HistogramTimer* name() { return &name##_; }
  HISTOGRAM_TIMER_LIST(HT)
#undef HT

#define HP(name, caption) \
  Histogram* name() { return &name##_; }
  HISTOGRAM_PERCENTAGE_LIST(HP)
#undef HP

#define HM(name, caption) \
  Histogram* name() { return &name##_; }
  HISTOGRAM_MEMORY_LIST(HM)
#undef HM

#define SC(name, caption) \
  StatsCounter* name() { return &name##_; }
  STATS_COUNTER_LIST_1(SC)
  STATS_COUNTER_LIST_2(SC)
#undef SC

#define SC(name) \
  StatsCounter* count_of_##name() { return &count_of_##name##_; } \
  StatsCounter* size_of_##name() { return &size_of_##name##_; }
  INSTANCE_TYPE_LIST(SC)
#undef SC

#define SC(name) \
  StatsCounter* count_of_CODE_TYPE_##name() \
    { return &count_of_CODE_TYPE_##name##_; } \
  StatsCounter* size_of_CODE_TYPE_##name() \
    { return &size_of_CODE_TYPE_##name##_; }
  CODE_KIND_LIST(SC)
#undef SC

#define SC(name) \
  StatsCounter* count_of_FIXED_ARRAY_##name() \
    { return &count_of_FIXED_ARRAY_##name##_; } \
  StatsCounter* size_of_FIXED_ARRAY_##name() \
    { return &size_of_FIXED_ARRAY_##name##_; }
  FIXED_ARRAY_SUB_INSTANCE_TYPE_LIST(SC)
#undef SC

#define SC(name) \
  StatsCounter* count_of_CODE_AGE_##name() \
    { return &count_of_CODE_AGE_##name##_; } \
  StatsCounter* size_of_CODE_AGE_##name() \
    { return &size_of_CODE_AGE_##name##_; }
  CODE_AGE_LIST_COMPLETE(SC)
#undef SC

  enum Id {
#define RATE_ID(name, caption) k_##name,
    HISTOGRAM_TIMER_LIST(RATE_ID)
#undef RATE_ID
#define PERCENTAGE_ID(name, caption) k_##name,
    HISTOGRAM_PERCENTAGE_LIST(PERCENTAGE_ID)
#undef PERCENTAGE_ID
#define MEMORY_ID(name, caption) k_##name,
    HISTOGRAM_MEMORY_LIST(MEMORY_ID)
#undef MEMORY_ID
#define COUNTER_ID(name, caption) k_##name,
    STATS_COUNTER_LIST_1(COUNTER_ID)
    STATS_COUNTER_LIST_2(COUNTER_ID)
#undef COUNTER_ID
#define COUNTER_ID(name) kCountOf##name, kSizeOf##name,
    INSTANCE_TYPE_LIST(COUNTER_ID)
#undef COUNTER_ID
#define COUNTER_ID(name) kCountOfCODE_TYPE_##name, \
    kSizeOfCODE_TYPE_##name,
    CODE_KIND_LIST(COUNTER_ID)
#undef COUNTER_ID
#define COUNTER_ID(name) kCountOfFIXED_ARRAY__##name, \
    kSizeOfFIXED_ARRAY__##name,
    FIXED_ARRAY_SUB_INSTANCE_TYPE_LIST(COUNTER_ID)
#undef COUNTER_ID
#define COUNTER_ID(name) kCountOfCODE_AGE__##name, \
    kSizeOfCODE_AGE__##name,
    CODE_AGE_LIST_COMPLETE(COUNTER_ID)
#undef COUNTER_ID
    stats_counter_count
  };

  void ResetCounters();
  void ResetHistograms();

 private:
#define HR(name, caption, min, max, num_buckets) Histogram name##_;
  HISTOGRAM_RANGE_LIST(HR)
#undef HR

#define HT(name, caption) \
  HistogramTimer name##_;
  HISTOGRAM_TIMER_LIST(HT)
#undef HT

#define HP(name, caption) \
  Histogram name##_;
  HISTOGRAM_PERCENTAGE_LIST(HP)
#undef HP

#define HM(name, caption) \
  Histogram name##_;
  HISTOGRAM_MEMORY_LIST(HM)
#undef HM

#define SC(name, caption) \
  StatsCounter name##_;
  STATS_COUNTER_LIST_1(SC)
  STATS_COUNTER_LIST_2(SC)
#undef SC

#define SC(name) \
  StatsCounter size_of_##name##_; \
  StatsCounter count_of_##name##_;
  INSTANCE_TYPE_LIST(SC)
#undef SC

#define SC(name) \
  StatsCounter size_of_CODE_TYPE_##name##_; \
  StatsCounter count_of_CODE_TYPE_##name##_;
  CODE_KIND_LIST(SC)
#undef SC

#define SC(name) \
  StatsCounter size_of_FIXED_ARRAY_##name##_; \
  StatsCounter count_of_FIXED_ARRAY_##name##_;
  FIXED_ARRAY_SUB_INSTANCE_TYPE_LIST(SC)
#undef SC

#define SC(name) \
  StatsCounter size_of_CODE_AGE_##name##_; \
  StatsCounter count_of_CODE_AGE_##name##_;
  CODE_AGE_LIST_COMPLETE(SC)
#undef SC

  friend class Isolate;

  explicit Counters(Isolate* isolate);

  DISALLOW_IMPLICIT_CONSTRUCTORS(Counters);
};

} }  // namespace v8::internal

#endif  // V8_COUNTERS_H_
