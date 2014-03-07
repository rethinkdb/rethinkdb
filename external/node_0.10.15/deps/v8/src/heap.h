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

#ifndef V8_HEAP_H_
#define V8_HEAP_H_

#include <math.h>

#include "allocation.h"
#include "globals.h"
#include "incremental-marking.h"
#include "list.h"
#include "mark-compact.h"
#include "objects-visiting.h"
#include "spaces.h"
#include "splay-tree-inl.h"
#include "store-buffer.h"
#include "v8-counters.h"
#include "v8globals.h"

namespace v8 {
namespace internal {

// Defines all the roots in Heap.
#define STRONG_ROOT_LIST(V)                                                    \
  V(Map, byte_array_map, ByteArrayMap)                                         \
  V(Map, free_space_map, FreeSpaceMap)                                         \
  V(Map, one_pointer_filler_map, OnePointerFillerMap)                          \
  V(Map, two_pointer_filler_map, TwoPointerFillerMap)                          \
  /* Cluster the most popular ones in a few cache lines here at the top.    */ \
  V(Smi, store_buffer_top, StoreBufferTop)                                     \
  V(Oddball, undefined_value, UndefinedValue)                                  \
  V(Oddball, the_hole_value, TheHoleValue)                                     \
  V(Oddball, null_value, NullValue)                                            \
  V(Oddball, true_value, TrueValue)                                            \
  V(Oddball, false_value, FalseValue)                                          \
  V(Map, global_property_cell_map, GlobalPropertyCellMap)                      \
  V(Map, shared_function_info_map, SharedFunctionInfoMap)                      \
  V(Map, meta_map, MetaMap)                                                    \
  V(Map, ascii_symbol_map, AsciiSymbolMap)                                     \
  V(Map, ascii_string_map, AsciiStringMap)                                     \
  V(Map, heap_number_map, HeapNumberMap)                                       \
  V(Map, native_context_map, NativeContextMap)                                 \
  V(Map, fixed_array_map, FixedArrayMap)                                       \
  V(Map, code_map, CodeMap)                                                    \
  V(Map, scope_info_map, ScopeInfoMap)                                         \
  V(Map, fixed_cow_array_map, FixedCOWArrayMap)                                \
  V(Map, fixed_double_array_map, FixedDoubleArrayMap)                          \
  V(Object, no_interceptor_result_sentinel, NoInterceptorResultSentinel)       \
  V(Map, hash_table_map, HashTableMap)                                         \
  V(FixedArray, empty_fixed_array, EmptyFixedArray)                            \
  V(ByteArray, empty_byte_array, EmptyByteArray)                               \
  V(String, empty_string, EmptyString)                                         \
  V(DescriptorArray, empty_descriptor_array, EmptyDescriptorArray)             \
  V(Smi, stack_limit, StackLimit)                                              \
  V(Oddball, arguments_marker, ArgumentsMarker)                                \
  /* The first 32 roots above this line should be boring from a GC point of */ \
  /* view.  This means they are never in new space and never on a page that */ \
  /* is being compacted.                                                    */ \
  V(FixedArray, number_string_cache, NumberStringCache)                        \
  V(Object, instanceof_cache_function, InstanceofCacheFunction)                \
  V(Object, instanceof_cache_map, InstanceofCacheMap)                          \
  V(Object, instanceof_cache_answer, InstanceofCacheAnswer)                    \
  V(FixedArray, single_character_string_cache, SingleCharacterStringCache)     \
  V(FixedArray, string_split_cache, StringSplitCache)                          \
  V(FixedArray, regexp_multiple_cache, RegExpMultipleCache)                    \
  V(Object, termination_exception, TerminationException)                       \
  V(Smi, hash_seed, HashSeed)                                                  \
  V(Map, string_map, StringMap)                                                \
  V(Map, symbol_map, SymbolMap)                                                \
  V(Map, cons_string_map, ConsStringMap)                                       \
  V(Map, cons_ascii_string_map, ConsAsciiStringMap)                            \
  V(Map, sliced_string_map, SlicedStringMap)                                   \
  V(Map, sliced_ascii_string_map, SlicedAsciiStringMap)                        \
  V(Map, cons_symbol_map, ConsSymbolMap)                                       \
  V(Map, cons_ascii_symbol_map, ConsAsciiSymbolMap)                            \
  V(Map, external_symbol_map, ExternalSymbolMap)                               \
  V(Map, external_symbol_with_ascii_data_map, ExternalSymbolWithAsciiDataMap)  \
  V(Map, external_ascii_symbol_map, ExternalAsciiSymbolMap)                    \
  V(Map, external_string_map, ExternalStringMap)                               \
  V(Map, external_string_with_ascii_data_map, ExternalStringWithAsciiDataMap)  \
  V(Map, external_ascii_string_map, ExternalAsciiStringMap)                    \
  V(Map, short_external_symbol_map, ShortExternalSymbolMap)                    \
  V(Map,                                                                       \
    short_external_symbol_with_ascii_data_map,                                 \
    ShortExternalSymbolWithAsciiDataMap)                                       \
  V(Map, short_external_ascii_symbol_map, ShortExternalAsciiSymbolMap)         \
  V(Map, short_external_string_map, ShortExternalStringMap)                    \
  V(Map,                                                                       \
    short_external_string_with_ascii_data_map,                                 \
    ShortExternalStringWithAsciiDataMap)                                       \
  V(Map, short_external_ascii_string_map, ShortExternalAsciiStringMap)         \
  V(Map, undetectable_string_map, UndetectableStringMap)                       \
  V(Map, undetectable_ascii_string_map, UndetectableAsciiStringMap)            \
  V(Map, external_pixel_array_map, ExternalPixelArrayMap)                      \
  V(Map, external_byte_array_map, ExternalByteArrayMap)                        \
  V(Map, external_unsigned_byte_array_map, ExternalUnsignedByteArrayMap)       \
  V(Map, external_short_array_map, ExternalShortArrayMap)                      \
  V(Map, external_unsigned_short_array_map, ExternalUnsignedShortArrayMap)     \
  V(Map, external_int_array_map, ExternalIntArrayMap)                          \
  V(Map, external_unsigned_int_array_map, ExternalUnsignedIntArrayMap)         \
  V(Map, external_float_array_map, ExternalFloatArrayMap)                      \
  V(Map, external_double_array_map, ExternalDoubleArrayMap)                    \
  V(Map, non_strict_arguments_elements_map, NonStrictArgumentsElementsMap)     \
  V(Map, function_context_map, FunctionContextMap)                             \
  V(Map, catch_context_map, CatchContextMap)                                   \
  V(Map, with_context_map, WithContextMap)                                     \
  V(Map, block_context_map, BlockContextMap)                                   \
  V(Map, module_context_map, ModuleContextMap)                                 \
  V(Map, global_context_map, GlobalContextMap)                                 \
  V(Map, oddball_map, OddballMap)                                              \
  V(Map, message_object_map, JSMessageObjectMap)                               \
  V(Map, foreign_map, ForeignMap)                                              \
  V(HeapNumber, nan_value, NanValue)                                           \
  V(HeapNumber, infinity_value, InfinityValue)                                 \
  V(HeapNumber, minus_zero_value, MinusZeroValue)                              \
  V(Map, neander_map, NeanderMap)                                              \
  V(JSObject, message_listeners, MessageListeners)                             \
  V(Foreign, prototype_accessors, PrototypeAccessors)                          \
  V(UnseededNumberDictionary, code_stubs, CodeStubs)                           \
  V(UnseededNumberDictionary, non_monomorphic_cache, NonMonomorphicCache)      \
  V(PolymorphicCodeCache, polymorphic_code_cache, PolymorphicCodeCache)        \
  V(Code, js_entry_code, JsEntryCode)                                          \
  V(Code, js_construct_entry_code, JsConstructEntryCode)                       \
  V(FixedArray, natives_source_cache, NativesSourceCache)                      \
  V(Object, last_script_id, LastScriptId)                                      \
  V(Script, empty_script, EmptyScript)                                         \
  V(Smi, real_stack_limit, RealStackLimit)                                     \
  V(StringDictionary, intrinsic_function_names, IntrinsicFunctionNames)        \
  V(Smi, arguments_adaptor_deopt_pc_offset, ArgumentsAdaptorDeoptPCOffset)     \
  V(Smi, construct_stub_deopt_pc_offset, ConstructStubDeoptPCOffset)           \
  V(Smi, getter_stub_deopt_pc_offset, GetterStubDeoptPCOffset)                 \
  V(Smi, setter_stub_deopt_pc_offset, SetterStubDeoptPCOffset)

#define ROOT_LIST(V)                                  \
  STRONG_ROOT_LIST(V)                                 \
  V(SymbolTable, symbol_table, SymbolTable)

#define SYMBOL_LIST(V)                                                   \
  V(Array_symbol, "Array")                                               \
  V(Object_symbol, "Object")                                             \
  V(Proto_symbol, "__proto__")                                           \
  V(StringImpl_symbol, "StringImpl")                                     \
  V(arguments_symbol, "arguments")                                       \
  V(Arguments_symbol, "Arguments")                                       \
  V(call_symbol, "call")                                                 \
  V(apply_symbol, "apply")                                               \
  V(caller_symbol, "caller")                                             \
  V(boolean_symbol, "boolean")                                           \
  V(Boolean_symbol, "Boolean")                                           \
  V(callee_symbol, "callee")                                             \
  V(constructor_symbol, "constructor")                                   \
  V(code_symbol, ".code")                                                \
  V(result_symbol, ".result")                                            \
  V(dot_for_symbol, ".for.")                                             \
  V(catch_var_symbol, ".catch-var")                                      \
  V(empty_symbol, "")                                                    \
  V(eval_symbol, "eval")                                                 \
  V(function_symbol, "function")                                         \
  V(length_symbol, "length")                                             \
  V(module_symbol, "module")                                             \
  V(name_symbol, "name")                                                 \
  V(native_symbol, "native")                                             \
  V(null_symbol, "null")                                                 \
  V(number_symbol, "number")                                             \
  V(Number_symbol, "Number")                                             \
  V(nan_symbol, "NaN")                                                   \
  V(RegExp_symbol, "RegExp")                                             \
  V(source_symbol, "source")                                             \
  V(global_symbol, "global")                                             \
  V(ignore_case_symbol, "ignoreCase")                                    \
  V(multiline_symbol, "multiline")                                       \
  V(input_symbol, "input")                                               \
  V(index_symbol, "index")                                               \
  V(last_index_symbol, "lastIndex")                                      \
  V(object_symbol, "object")                                             \
  V(prototype_symbol, "prototype")                                       \
  V(string_symbol, "string")                                             \
  V(String_symbol, "String")                                             \
  V(Date_symbol, "Date")                                                 \
  V(this_symbol, "this")                                                 \
  V(to_string_symbol, "toString")                                        \
  V(char_at_symbol, "CharAt")                                            \
  V(undefined_symbol, "undefined")                                       \
  V(value_of_symbol, "valueOf")                                          \
  V(InitializeVarGlobal_symbol, "InitializeVarGlobal")                   \
  V(InitializeConstGlobal_symbol, "InitializeConstGlobal")               \
  V(KeyedLoadElementMonomorphic_symbol,                                  \
    "KeyedLoadElementMonomorphic")                                       \
  V(KeyedStoreElementMonomorphic_symbol,                                 \
    "KeyedStoreElementMonomorphic")                                      \
  V(KeyedStoreAndGrowElementMonomorphic_symbol,                          \
    "KeyedStoreAndGrowElementMonomorphic")                               \
  V(stack_overflow_symbol, "kStackOverflowBoilerplate")                  \
  V(illegal_access_symbol, "illegal access")                             \
  V(out_of_memory_symbol, "out-of-memory")                               \
  V(illegal_execution_state_symbol, "illegal execution state")           \
  V(get_symbol, "get")                                                   \
  V(set_symbol, "set")                                                   \
  V(function_class_symbol, "Function")                                   \
  V(illegal_argument_symbol, "illegal argument")                         \
  V(MakeReferenceError_symbol, "MakeReferenceError")                     \
  V(MakeSyntaxError_symbol, "MakeSyntaxError")                           \
  V(MakeTypeError_symbol, "MakeTypeError")                               \
  V(invalid_lhs_in_assignment_symbol, "invalid_lhs_in_assignment")       \
  V(invalid_lhs_in_for_in_symbol, "invalid_lhs_in_for_in")               \
  V(invalid_lhs_in_postfix_op_symbol, "invalid_lhs_in_postfix_op")       \
  V(invalid_lhs_in_prefix_op_symbol, "invalid_lhs_in_prefix_op")         \
  V(illegal_return_symbol, "illegal_return")                             \
  V(illegal_break_symbol, "illegal_break")                               \
  V(illegal_continue_symbol, "illegal_continue")                         \
  V(unknown_label_symbol, "unknown_label")                               \
  V(redeclaration_symbol, "redeclaration")                               \
  V(failure_symbol, "<failure>")                                         \
  V(space_symbol, " ")                                                   \
  V(exec_symbol, "exec")                                                 \
  V(zero_symbol, "0")                                                    \
  V(global_eval_symbol, "GlobalEval")                                    \
  V(identity_hash_symbol, "v8::IdentityHash")                            \
  V(closure_symbol, "(closure)")                                         \
  V(use_strict, "use strict")                                            \
  V(dot_symbol, ".")                                                     \
  V(anonymous_function_symbol, "(anonymous function)")                   \
  V(compare_ic_symbol, "==")                                             \
  V(strict_compare_ic_symbol, "===")                                     \
  V(infinity_symbol, "Infinity")                                         \
  V(minus_infinity_symbol, "-Infinity")                                  \
  V(hidden_stack_trace_symbol, "v8::hidden_stack_trace")                 \
  V(query_colon_symbol, "(?:)")

// Forward declarations.
class GCTracer;
class HeapStats;
class Isolate;
class WeakObjectRetainer;


typedef String* (*ExternalStringTableUpdaterCallback)(Heap* heap,
                                                      Object** pointer);

class StoreBufferRebuilder {
 public:
  explicit StoreBufferRebuilder(StoreBuffer* store_buffer)
      : store_buffer_(store_buffer) {
  }

  void Callback(MemoryChunk* page, StoreBufferEvent event);

 private:
  StoreBuffer* store_buffer_;

  // We record in this variable how full the store buffer was when we started
  // iterating over the current page, finding pointers to new space.  If the
  // store buffer overflows again we can exempt the page from the store buffer
  // by rewinding to this point instead of having to search the store buffer.
  Object*** start_of_current_page_;
  // The current page we are scanning in the store buffer iterator.
  MemoryChunk* current_page_;
};



// The all static Heap captures the interface to the global object heap.
// All JavaScript contexts by this process share the same object heap.

#ifdef DEBUG
class HeapDebugUtils;
#endif


// A queue of objects promoted during scavenge. Each object is accompanied
// by it's size to avoid dereferencing a map pointer for scanning.
class PromotionQueue {
 public:
  explicit PromotionQueue(Heap* heap)
      : front_(NULL),
        rear_(NULL),
        limit_(NULL),
        emergency_stack_(0),
        heap_(heap) { }

  void Initialize();

  void Destroy() {
    ASSERT(is_empty());
    delete emergency_stack_;
    emergency_stack_ = NULL;
  }

  inline void ActivateGuardIfOnTheSamePage();

  Page* GetHeadPage() {
    return Page::FromAllocationTop(reinterpret_cast<Address>(rear_));
  }

  void SetNewLimit(Address limit) {
    if (!guard_) {
      return;
    }

    ASSERT(GetHeadPage() == Page::FromAllocationTop(limit));
    limit_ = reinterpret_cast<intptr_t*>(limit);

    if (limit_ <= rear_) {
      return;
    }

    RelocateQueueHead();
  }

  bool is_empty() {
    return (front_ == rear_) &&
        (emergency_stack_ == NULL || emergency_stack_->length() == 0);
  }

  inline void insert(HeapObject* target, int size);

  void remove(HeapObject** target, int* size) {
    ASSERT(!is_empty());
    if (front_ == rear_) {
      Entry e = emergency_stack_->RemoveLast();
      *target = e.obj_;
      *size = e.size_;
      return;
    }

    if (NewSpacePage::IsAtStart(reinterpret_cast<Address>(front_))) {
      NewSpacePage* front_page =
          NewSpacePage::FromAddress(reinterpret_cast<Address>(front_));
      ASSERT(!front_page->prev_page()->is_anchor());
      front_ =
          reinterpret_cast<intptr_t*>(front_page->prev_page()->area_end());
    }
    *target = reinterpret_cast<HeapObject*>(*(--front_));
    *size = static_cast<int>(*(--front_));
    // Assert no underflow.
    SemiSpace::AssertValidRange(reinterpret_cast<Address>(rear_),
                                reinterpret_cast<Address>(front_));
  }

 private:
  // The front of the queue is higher in the memory page chain than the rear.
  intptr_t* front_;
  intptr_t* rear_;
  intptr_t* limit_;

  bool guard_;

  static const int kEntrySizeInWords = 2;

  struct Entry {
    Entry(HeapObject* obj, int size) : obj_(obj), size_(size) { }

    HeapObject* obj_;
    int size_;
  };
  List<Entry>* emergency_stack_;

  Heap* heap_;

  void RelocateQueueHead();

  DISALLOW_COPY_AND_ASSIGN(PromotionQueue);
};


typedef void (*ScavengingCallback)(Map* map,
                                   HeapObject** slot,
                                   HeapObject* object);


// External strings table is a place where all external strings are
// registered.  We need to keep track of such strings to properly
// finalize them.
class ExternalStringTable {
 public:
  // Registers an external string.
  inline void AddString(String* string);

  inline void Iterate(ObjectVisitor* v);

  // Restores internal invariant and gets rid of collected strings.
  // Must be called after each Iterate() that modified the strings.
  void CleanUp();

  // Destroys all allocated memory.
  void TearDown();

 private:
  ExternalStringTable() { }

  friend class Heap;

  inline void Verify();

  inline void AddOldString(String* string);

  // Notifies the table that only a prefix of the new list is valid.
  inline void ShrinkNewStrings(int position);

  // To speed up scavenge collections new space string are kept
  // separate from old space strings.
  List<Object*> new_space_strings_;
  List<Object*> old_space_strings_;

  Heap* heap_;

  DISALLOW_COPY_AND_ASSIGN(ExternalStringTable);
};


enum ArrayStorageAllocationMode {
  DONT_INITIALIZE_ARRAY_ELEMENTS,
  INITIALIZE_ARRAY_ELEMENTS_WITH_HOLE
};

class Heap {
 public:
  // Configure heap size before setup. Return false if the heap has been
  // set up already.
  bool ConfigureHeap(int max_semispace_size,
                     intptr_t max_old_gen_size,
                     intptr_t max_executable_size);
  bool ConfigureHeapDefault();

  // Initializes the global object heap. If create_heap_objects is true,
  // also creates the basic non-mutable objects.
  // Returns whether it succeeded.
  bool SetUp(bool create_heap_objects);

  // Destroys all memory allocated by the heap.
  void TearDown();

  // Set the stack limit in the roots_ array.  Some architectures generate
  // code that looks here, because it is faster than loading from the static
  // jslimit_/real_jslimit_ variable in the StackGuard.
  void SetStackLimits();

  // Returns whether SetUp has been called.
  bool HasBeenSetUp();

  // Returns the maximum amount of memory reserved for the heap.  For
  // the young generation, we reserve 4 times the amount needed for a
  // semi space.  The young generation consists of two semi spaces and
  // we reserve twice the amount needed for those in order to ensure
  // that new space can be aligned to its size.
  intptr_t MaxReserved() {
    return 4 * reserved_semispace_size_ + max_old_generation_size_;
  }
  int MaxSemiSpaceSize() { return max_semispace_size_; }
  int ReservedSemiSpaceSize() { return reserved_semispace_size_; }
  int InitialSemiSpaceSize() { return initial_semispace_size_; }
  intptr_t MaxOldGenerationSize() { return max_old_generation_size_; }
  intptr_t MaxExecutableSize() { return max_executable_size_; }

  // Returns the capacity of the heap in bytes w/o growing. Heap grows when
  // more spaces are needed until it reaches the limit.
  intptr_t Capacity();

  // Returns the amount of memory currently committed for the heap.
  intptr_t CommittedMemory();

  // Returns the amount of executable memory currently committed for the heap.
  intptr_t CommittedMemoryExecutable();

  // Returns the available bytes in space w/o growing.
  // Heap doesn't guarantee that it can allocate an object that requires
  // all available bytes. Check MaxHeapObjectSize() instead.
  intptr_t Available();

  // Returns of size of all objects residing in the heap.
  intptr_t SizeOfObjects();

  // Return the starting address and a mask for the new space.  And-masking an
  // address with the mask will result in the start address of the new space
  // for all addresses in either semispace.
  Address NewSpaceStart() { return new_space_.start(); }
  uintptr_t NewSpaceMask() { return new_space_.mask(); }
  Address NewSpaceTop() { return new_space_.top(); }

  NewSpace* new_space() { return &new_space_; }
  OldSpace* old_pointer_space() { return old_pointer_space_; }
  OldSpace* old_data_space() { return old_data_space_; }
  OldSpace* code_space() { return code_space_; }
  MapSpace* map_space() { return map_space_; }
  CellSpace* cell_space() { return cell_space_; }
  LargeObjectSpace* lo_space() { return lo_space_; }
  PagedSpace* paged_space(int idx) {
    switch (idx) {
      case OLD_POINTER_SPACE:
        return old_pointer_space();
      case OLD_DATA_SPACE:
        return old_data_space();
      case MAP_SPACE:
        return map_space();
      case CELL_SPACE:
        return cell_space();
      case CODE_SPACE:
        return code_space();
      case NEW_SPACE:
      case LO_SPACE:
        UNREACHABLE();
    }
    return NULL;
  }

  bool always_allocate() { return always_allocate_scope_depth_ != 0; }
  Address always_allocate_scope_depth_address() {
    return reinterpret_cast<Address>(&always_allocate_scope_depth_);
  }
  bool linear_allocation() {
    return linear_allocation_scope_depth_ != 0;
  }

  Address* NewSpaceAllocationTopAddress() {
    return new_space_.allocation_top_address();
  }
  Address* NewSpaceAllocationLimitAddress() {
    return new_space_.allocation_limit_address();
  }

  // Uncommit unused semi space.
  bool UncommitFromSpace() { return new_space_.UncommitFromSpace(); }

  // Allocates and initializes a new JavaScript object based on a
  // constructor.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateJSObject(
      JSFunction* constructor, PretenureFlag pretenure = NOT_TENURED);

  MUST_USE_RESULT MaybeObject* AllocateJSModule(Context* context,
                                                ScopeInfo* scope_info);

  // Allocate a JSArray with no elements
  MUST_USE_RESULT MaybeObject* AllocateEmptyJSArray(
      ElementsKind elements_kind,
      PretenureFlag pretenure = NOT_TENURED) {
    return AllocateJSArrayAndStorage(elements_kind, 0, 0,
                                     DONT_INITIALIZE_ARRAY_ELEMENTS,
                                     pretenure);
  }

  // Allocate a JSArray with a specified length but elements that are left
  // uninitialized.
  MUST_USE_RESULT MaybeObject* AllocateJSArrayAndStorage(
      ElementsKind elements_kind,
      int length,
      int capacity,
      ArrayStorageAllocationMode mode = DONT_INITIALIZE_ARRAY_ELEMENTS,
      PretenureFlag pretenure = NOT_TENURED);

  // Allocate a JSArray with no elements
  MUST_USE_RESULT MaybeObject* AllocateJSArrayWithElements(
      FixedArrayBase* array_base,
      ElementsKind elements_kind,
      PretenureFlag pretenure = NOT_TENURED);

  // Allocates and initializes a new global object based on a constructor.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateGlobalObject(JSFunction* constructor);

  // Returns a deep copy of the JavaScript object.
  // Properties and elements are copied too.
  // Returns failure if allocation failed.
  MUST_USE_RESULT MaybeObject* CopyJSObject(JSObject* source);

  // Allocates the function prototype.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateFunctionPrototype(JSFunction* function);

  // Allocates a Harmony proxy or function proxy.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateJSProxy(Object* handler,
                                               Object* prototype);

  MUST_USE_RESULT MaybeObject* AllocateJSFunctionProxy(Object* handler,
                                                       Object* call_trap,
                                                       Object* construct_trap,
                                                       Object* prototype);

  // Reinitialize a JSReceiver into an (empty) JS object of respective type and
  // size, but keeping the original prototype.  The receiver must have at least
  // the size of the new object.  The object is reinitialized and behaves as an
  // object that has been freshly allocated.
  // Returns failure if an error occured, otherwise object.
  MUST_USE_RESULT MaybeObject* ReinitializeJSReceiver(JSReceiver* object,
                                                      InstanceType type,
                                                      int size);

  // Reinitialize an JSGlobalProxy based on a constructor.  The object
  // must have the same size as objects allocated using the
  // constructor.  The object is reinitialized and behaves as an
  // object that has been freshly allocated using the constructor.
  MUST_USE_RESULT MaybeObject* ReinitializeJSGlobalProxy(
      JSFunction* constructor, JSGlobalProxy* global);

  // Allocates and initializes a new JavaScript object based on a map.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateJSObjectFromMap(
      Map* map, PretenureFlag pretenure = NOT_TENURED);

  // Allocates a heap object based on the map.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this function does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* Allocate(Map* map, AllocationSpace space);

  // Allocates a JS Map in the heap.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this function does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateMap(
      InstanceType instance_type,
      int instance_size,
      ElementsKind elements_kind = TERMINAL_FAST_ELEMENTS_KIND);

  // Allocates a partial map for bootstrapping.
  MUST_USE_RESULT MaybeObject* AllocatePartialMap(InstanceType instance_type,
                                                  int instance_size);

  // Allocate a map for the specified function
  MUST_USE_RESULT MaybeObject* AllocateInitialMap(JSFunction* fun);

  // Allocates an empty code cache.
  MUST_USE_RESULT MaybeObject* AllocateCodeCache();

  // Allocates a serialized scope info.
  MUST_USE_RESULT MaybeObject* AllocateScopeInfo(int length);

  // Allocates an empty PolymorphicCodeCache.
  MUST_USE_RESULT MaybeObject* AllocatePolymorphicCodeCache();

  // Allocates a pre-tenured empty AccessorPair.
  MUST_USE_RESULT MaybeObject* AllocateAccessorPair();

  // Allocates an empty TypeFeedbackInfo.
  MUST_USE_RESULT MaybeObject* AllocateTypeFeedbackInfo();

  // Allocates an AliasedArgumentsEntry.
  MUST_USE_RESULT MaybeObject* AllocateAliasedArgumentsEntry(int slot);

  // Clear the Instanceof cache (used when a prototype changes).
  inline void ClearInstanceofCache();

  // For use during bootup.
  void RepairFreeListsAfterBoot();

  // Allocates and fully initializes a String.  There are two String
  // encodings: ASCII and two byte. One should choose between the three string
  // allocation functions based on the encoding of the string buffer used to
  // initialized the string.
  //   - ...FromAscii initializes the string from a buffer that is ASCII
  //     encoded (it does not check that the buffer is ASCII encoded) and the
  //     result will be ASCII encoded.
  //   - ...FromUTF8 initializes the string from a buffer that is UTF-8
  //     encoded.  If the characters are all single-byte characters, the
  //     result will be ASCII encoded, otherwise it will converted to two
  //     byte.
  //   - ...FromTwoByte initializes the string from a buffer that is two-byte
  //     encoded.  If the characters are all single-byte characters, the
  //     result will be converted to ASCII, otherwise it will be left as
  //     two-byte.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateStringFromAscii(
      Vector<const char> str,
      PretenureFlag pretenure = NOT_TENURED);
  MUST_USE_RESULT inline MaybeObject* AllocateStringFromUtf8(
      Vector<const char> str,
      PretenureFlag pretenure = NOT_TENURED);
  MUST_USE_RESULT MaybeObject* AllocateStringFromUtf8Slow(
      Vector<const char> str,
      int non_ascii_start,
      PretenureFlag pretenure = NOT_TENURED);
  MUST_USE_RESULT MaybeObject* AllocateStringFromTwoByte(
      Vector<const uc16> str,
      PretenureFlag pretenure = NOT_TENURED);

  // Allocates a symbol in old space based on the character stream.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this function does not perform a garbage collection.
  MUST_USE_RESULT inline MaybeObject* AllocateSymbol(Vector<const char> str,
                                                     int chars,
                                                     uint32_t hash_field);

  MUST_USE_RESULT inline MaybeObject* AllocateAsciiSymbol(
        Vector<const char> str,
        uint32_t hash_field);

  MUST_USE_RESULT inline MaybeObject* AllocateTwoByteSymbol(
        Vector<const uc16> str,
        uint32_t hash_field);

  MUST_USE_RESULT MaybeObject* AllocateInternalSymbol(
      unibrow::CharacterStream* buffer, int chars, uint32_t hash_field);

  MUST_USE_RESULT MaybeObject* AllocateExternalSymbol(
      Vector<const char> str,
      int chars);

  // Allocates and partially initializes a String.  There are two String
  // encodings: ASCII and two byte.  These functions allocate a string of the
  // given length and set its map and length fields.  The characters of the
  // string are uninitialized.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateRawAsciiString(
      int length,
      PretenureFlag pretenure = NOT_TENURED);
  MUST_USE_RESULT MaybeObject* AllocateRawTwoByteString(
      int length,
      PretenureFlag pretenure = NOT_TENURED);

  // Computes a single character string where the character has code.
  // A cache is used for ASCII codes.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed. Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* LookupSingleCharacterStringFromCode(
      uint16_t code);

  // Allocate a byte array of the specified length
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateByteArray(int length,
                                                 PretenureFlag pretenure);

  // Allocate a non-tenured byte array of the specified length
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateByteArray(int length);

  // Allocates an external array of the specified length and type.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateExternalArray(
      int length,
      ExternalArrayType array_type,
      void* external_pointer,
      PretenureFlag pretenure);

  // Allocate a tenured JS global property cell.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateJSGlobalPropertyCell(Object* value);

  // Allocates a fixed array initialized with undefined values
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateFixedArray(int length,
                                                  PretenureFlag pretenure);
  // Allocates a fixed array initialized with undefined values
  MUST_USE_RESULT MaybeObject* AllocateFixedArray(int length);

  // Allocates an uninitialized fixed array. It must be filled by the caller.
  //
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateUninitializedFixedArray(int length);

  // Make a copy of src and return it. Returns
  // Failure::RetryAfterGC(requested_bytes, space) if the allocation failed.
  MUST_USE_RESULT inline MaybeObject* CopyFixedArray(FixedArray* src);

  // Make a copy of src, set the map, and return the copy. Returns
  // Failure::RetryAfterGC(requested_bytes, space) if the allocation failed.
  MUST_USE_RESULT MaybeObject* CopyFixedArrayWithMap(FixedArray* src, Map* map);

  // Make a copy of src and return it. Returns
  // Failure::RetryAfterGC(requested_bytes, space) if the allocation failed.
  MUST_USE_RESULT inline MaybeObject* CopyFixedDoubleArray(
      FixedDoubleArray* src);

  // Make a copy of src, set the map, and return the copy. Returns
  // Failure::RetryAfterGC(requested_bytes, space) if the allocation failed.
  MUST_USE_RESULT MaybeObject* CopyFixedDoubleArrayWithMap(
      FixedDoubleArray* src, Map* map);

  // Allocates a fixed array initialized with the hole values.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateFixedArrayWithHoles(
      int length,
      PretenureFlag pretenure = NOT_TENURED);

  MUST_USE_RESULT MaybeObject* AllocateRawFixedDoubleArray(
      int length,
      PretenureFlag pretenure);

  // Allocates a fixed double array with uninitialized values. Returns
  // Failure::RetryAfterGC(requested_bytes, space) if the allocation failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateUninitializedFixedDoubleArray(
      int length,
      PretenureFlag pretenure = NOT_TENURED);

  // Allocates a fixed double array with hole values. Returns
  // Failure::RetryAfterGC(requested_bytes, space) if the allocation failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateFixedDoubleArrayWithHoles(
      int length,
      PretenureFlag pretenure = NOT_TENURED);

  // AllocateHashTable is identical to AllocateFixedArray except
  // that the resulting object has hash_table_map as map.
  MUST_USE_RESULT MaybeObject* AllocateHashTable(
      int length, PretenureFlag pretenure = NOT_TENURED);

  // Allocate a native (but otherwise uninitialized) context.
  MUST_USE_RESULT MaybeObject* AllocateNativeContext();

  // Allocate a global context.
  MUST_USE_RESULT MaybeObject* AllocateGlobalContext(JSFunction* function,
                                                     ScopeInfo* scope_info);

  // Allocate a module context.
  MUST_USE_RESULT MaybeObject* AllocateModuleContext(ScopeInfo* scope_info);

  // Allocate a function context.
  MUST_USE_RESULT MaybeObject* AllocateFunctionContext(int length,
                                                       JSFunction* function);

  // Allocate a catch context.
  MUST_USE_RESULT MaybeObject* AllocateCatchContext(JSFunction* function,
                                                    Context* previous,
                                                    String* name,
                                                    Object* thrown_object);
  // Allocate a 'with' context.
  MUST_USE_RESULT MaybeObject* AllocateWithContext(JSFunction* function,
                                                   Context* previous,
                                                   JSObject* extension);

  // Allocate a block context.
  MUST_USE_RESULT MaybeObject* AllocateBlockContext(JSFunction* function,
                                                    Context* previous,
                                                    ScopeInfo* info);

  // Allocates a new utility object in the old generation.
  MUST_USE_RESULT MaybeObject* AllocateStruct(InstanceType type);

  // Allocates a function initialized with a shared part.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateFunction(
      Map* function_map,
      SharedFunctionInfo* shared,
      Object* prototype,
      PretenureFlag pretenure = TENURED);

  // Arguments object size.
  static const int kArgumentsObjectSize =
      JSObject::kHeaderSize + 2 * kPointerSize;
  // Strict mode arguments has no callee so it is smaller.
  static const int kArgumentsObjectSizeStrict =
      JSObject::kHeaderSize + 1 * kPointerSize;
  // Indicies for direct access into argument objects.
  static const int kArgumentsLengthIndex = 0;
  // callee is only valid in non-strict mode.
  static const int kArgumentsCalleeIndex = 1;

  // Allocates an arguments object - optionally with an elements array.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateArgumentsObject(
      Object* callee, int length);

  // Same as NewNumberFromDouble, but may return a preallocated/immutable
  // number object (e.g., minus_zero_value_, nan_value_)
  MUST_USE_RESULT MaybeObject* NumberFromDouble(
      double value, PretenureFlag pretenure = NOT_TENURED);

  // Allocated a HeapNumber from value.
  MUST_USE_RESULT MaybeObject* AllocateHeapNumber(
      double value,
      PretenureFlag pretenure);
  // pretenure = NOT_TENURED
  MUST_USE_RESULT MaybeObject* AllocateHeapNumber(double value);

  // Converts an int into either a Smi or a HeapNumber object.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT inline MaybeObject* NumberFromInt32(
      int32_t value, PretenureFlag pretenure = NOT_TENURED);

  // Converts an int into either a Smi or a HeapNumber object.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT inline MaybeObject* NumberFromUint32(
      uint32_t value, PretenureFlag pretenure = NOT_TENURED);

  // Allocates a new foreign object.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateForeign(
      Address address, PretenureFlag pretenure = NOT_TENURED);

  // Allocates a new SharedFunctionInfo object.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateSharedFunctionInfo(Object* name);

  // Allocates a new JSMessageObject object.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note that this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateJSMessageObject(
      String* type,
      JSArray* arguments,
      int start_position,
      int end_position,
      Object* script,
      Object* stack_trace,
      Object* stack_frames);

  // Allocates a new cons string object.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateConsString(String* first,
                                                  String* second);

  // Allocates a new sub string object which is a substring of an underlying
  // string buffer stretching from the index start (inclusive) to the index
  // end (exclusive).
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateSubString(
      String* buffer,
      int start,
      int end,
      PretenureFlag pretenure = NOT_TENURED);

  // Allocate a new external string object, which is backed by a string
  // resource that resides outside the V8 heap.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* AllocateExternalStringFromAscii(
      const ExternalAsciiString::Resource* resource);
  MUST_USE_RESULT MaybeObject* AllocateExternalStringFromTwoByte(
      const ExternalTwoByteString::Resource* resource);

  // Finalizes an external string by deleting the associated external
  // data and clearing the resource pointer.
  inline void FinalizeExternalString(String* string);

  // Allocates an uninitialized object.  The memory is non-executable if the
  // hardware and OS allow.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this function does not perform a garbage collection.
  MUST_USE_RESULT inline MaybeObject* AllocateRaw(int size_in_bytes,
                                                  AllocationSpace space,
                                                  AllocationSpace retry_space);

  // Initialize a filler object to keep the ability to iterate over the heap
  // when shortening objects.
  void CreateFillerObjectAt(Address addr, int size);

  // Makes a new native code object
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed. On success, the pointer to the Code object is stored in the
  // self_reference. This allows generated code to reference its own Code
  // object by containing this pointer.
  // Please note this function does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* CreateCode(const CodeDesc& desc,
                                          Code::Flags flags,
                                          Handle<Object> self_reference,
                                          bool immovable = false);

  MUST_USE_RESULT MaybeObject* CopyCode(Code* code);

  // Copy the code and scope info part of the code object, but insert
  // the provided data as the relocation information.
  MUST_USE_RESULT MaybeObject* CopyCode(Code* code, Vector<byte> reloc_info);

  // Finds the symbol for string in the symbol table.
  // If not found, a new symbol is added to the table and returned.
  // Returns Failure::RetryAfterGC(requested_bytes, space) if allocation
  // failed.
  // Please note this function does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* LookupSymbol(Vector<const char> str);
  MUST_USE_RESULT MaybeObject* LookupAsciiSymbol(Vector<const char> str);
  MUST_USE_RESULT MaybeObject* LookupTwoByteSymbol(Vector<const uc16> str);
  MUST_USE_RESULT MaybeObject* LookupAsciiSymbol(const char* str) {
    return LookupSymbol(CStrVector(str));
  }
  MUST_USE_RESULT MaybeObject* LookupSymbol(String* str);
  MUST_USE_RESULT MaybeObject* LookupAsciiSymbol(Handle<SeqAsciiString> string,
                                                 int from,
                                                 int length);

  bool LookupSymbolIfExists(String* str, String** symbol);
  bool LookupTwoCharsSymbolIfExists(String* str, String** symbol);

  // Compute the matching symbol map for a string if possible.
  // NULL is returned if string is in new space or not flattened.
  Map* SymbolMapForString(String* str);

  // Tries to flatten a string before compare operation.
  //
  // Returns a failure in case it was decided that flattening was
  // necessary and failed.  Note, if flattening is not necessary the
  // string might stay non-flat even when not a failure is returned.
  //
  // Please note this function does not perform a garbage collection.
  MUST_USE_RESULT inline MaybeObject* PrepareForCompare(String* str);

  // Converts the given boolean condition to JavaScript boolean value.
  inline Object* ToBoolean(bool condition);

  // Code that should be run before and after each GC.  Includes some
  // reporting/verification activities when compiled with DEBUG set.
  void GarbageCollectionPrologue();
  void GarbageCollectionEpilogue();

  // Performs garbage collection operation.
  // Returns whether there is a chance that another major GC could
  // collect more garbage.
  bool CollectGarbage(AllocationSpace space,
                      GarbageCollector collector,
                      const char* gc_reason,
                      const char* collector_reason);

  // Performs garbage collection operation.
  // Returns whether there is a chance that another major GC could
  // collect more garbage.
  inline bool CollectGarbage(AllocationSpace space,
                             const char* gc_reason = NULL);

  static const int kNoGCFlags = 0;
  static const int kSweepPreciselyMask = 1;
  static const int kReduceMemoryFootprintMask = 2;
  static const int kAbortIncrementalMarkingMask = 4;

  // Making the heap iterable requires us to sweep precisely and abort any
  // incremental marking as well.
  static const int kMakeHeapIterableMask =
      kSweepPreciselyMask | kAbortIncrementalMarkingMask;

  // Performs a full garbage collection.  If (flags & kMakeHeapIterableMask) is
  // non-zero, then the slower precise sweeper is used, which leaves the heap
  // in a state where we can iterate over the heap visiting all objects.
  void CollectAllGarbage(int flags, const char* gc_reason = NULL);

  // Last hope GC, should try to squeeze as much as possible.
  void CollectAllAvailableGarbage(const char* gc_reason = NULL);

  // Check whether the heap is currently iterable.
  bool IsHeapIterable();

  // Ensure that we have swept all spaces in such a way that we can iterate
  // over all objects.  May cause a GC.
  void EnsureHeapIsIterable();

  // Notify the heap that a context has been disposed.
  int NotifyContextDisposed() {
    flush_monomorphic_ics_ = true;
    return ++contexts_disposed_;
  }

  // Utility to invoke the scavenger. This is needed in test code to
  // ensure correct callback for weak global handles.
  void PerformScavenge();

  inline void increment_scan_on_scavenge_pages() {
    scan_on_scavenge_pages_++;
    if (FLAG_gc_verbose) {
      PrintF("Scan-on-scavenge pages: %d\n", scan_on_scavenge_pages_);
    }
  }

  inline void decrement_scan_on_scavenge_pages() {
    scan_on_scavenge_pages_--;
    if (FLAG_gc_verbose) {
      PrintF("Scan-on-scavenge pages: %d\n", scan_on_scavenge_pages_);
    }
  }

  PromotionQueue* promotion_queue() { return &promotion_queue_; }

#ifdef DEBUG
  // Utility used with flag gc-greedy.
  void GarbageCollectionGreedyCheck();
#endif

  void AddGCPrologueCallback(
      GCPrologueCallback callback, GCType gc_type_filter);
  void RemoveGCPrologueCallback(GCPrologueCallback callback);

  void AddGCEpilogueCallback(
      GCEpilogueCallback callback, GCType gc_type_filter);
  void RemoveGCEpilogueCallback(GCEpilogueCallback callback);

  void SetGlobalGCPrologueCallback(GCCallback callback) {
    ASSERT((callback == NULL) ^ (global_gc_prologue_callback_ == NULL));
    global_gc_prologue_callback_ = callback;
  }
  void SetGlobalGCEpilogueCallback(GCCallback callback) {
    ASSERT((callback == NULL) ^ (global_gc_epilogue_callback_ == NULL));
    global_gc_epilogue_callback_ = callback;
  }

  // Heap root getters.  We have versions with and without type::cast() here.
  // You can't use type::cast during GC because the assert fails.
  // TODO(1490): Try removing the unchecked accessors, now that GC marking does
  // not corrupt the map.
#define ROOT_ACCESSOR(type, name, camel_name)                                  \
  type* name() {                                                               \
    return type::cast(roots_[k##camel_name##RootIndex]);                       \
  }                                                                            \
  type* raw_unchecked_##name() {                                               \
    return reinterpret_cast<type*>(roots_[k##camel_name##RootIndex]);          \
  }
  ROOT_LIST(ROOT_ACCESSOR)
#undef ROOT_ACCESSOR

// Utility type maps
#define STRUCT_MAP_ACCESSOR(NAME, Name, name)                                  \
    Map* name##_map() {                                                        \
      return Map::cast(roots_[k##Name##MapRootIndex]);                         \
    }
  STRUCT_LIST(STRUCT_MAP_ACCESSOR)
#undef STRUCT_MAP_ACCESSOR

#define SYMBOL_ACCESSOR(name, str) String* name() {                            \
    return String::cast(roots_[k##name##RootIndex]);                           \
  }
  SYMBOL_LIST(SYMBOL_ACCESSOR)
#undef SYMBOL_ACCESSOR

  // The hidden_symbol is special because it is the empty string, but does
  // not match the empty string.
  String* hidden_symbol() { return hidden_symbol_; }

  void set_native_contexts_list(Object* object) {
    native_contexts_list_ = object;
  }
  Object* native_contexts_list() { return native_contexts_list_; }

  // Number of mark-sweeps.
  unsigned int ms_count() { return ms_count_; }

  // Iterates over all roots in the heap.
  void IterateRoots(ObjectVisitor* v, VisitMode mode);
  // Iterates over all strong roots in the heap.
  void IterateStrongRoots(ObjectVisitor* v, VisitMode mode);
  // Iterates over all the other roots in the heap.
  void IterateWeakRoots(ObjectVisitor* v, VisitMode mode);

  // Iterate pointers to from semispace of new space found in memory interval
  // from start to end.
  void IterateAndMarkPointersToFromSpace(Address start,
                                         Address end,
                                         ObjectSlotCallback callback);

  // Returns whether the object resides in new space.
  inline bool InNewSpace(Object* object);
  inline bool InNewSpace(Address addr);
  inline bool InNewSpacePage(Address addr);
  inline bool InFromSpace(Object* object);
  inline bool InToSpace(Object* object);

  // Checks whether an address/object in the heap (including auxiliary
  // area and unused area).
  bool Contains(Address addr);
  bool Contains(HeapObject* value);

  // Checks whether an address/object in a space.
  // Currently used by tests, serialization and heap verification only.
  bool InSpace(Address addr, AllocationSpace space);
  bool InSpace(HeapObject* value, AllocationSpace space);

  // Finds out which space an object should get promoted to based on its type.
  inline OldSpace* TargetSpace(HeapObject* object);
  inline AllocationSpace TargetSpaceId(InstanceType type);

  // Sets the stub_cache_ (only used when expanding the dictionary).
  void public_set_code_stubs(UnseededNumberDictionary* value) {
    roots_[kCodeStubsRootIndex] = value;
  }

  // Support for computing object sizes for old objects during GCs. Returns
  // a function that is guaranteed to be safe for computing object sizes in
  // the current GC phase.
  HeapObjectCallback GcSafeSizeOfOldObjectFunction() {
    return gc_safe_size_of_old_object_;
  }

  // Sets the non_monomorphic_cache_ (only used when expanding the dictionary).
  void public_set_non_monomorphic_cache(UnseededNumberDictionary* value) {
    roots_[kNonMonomorphicCacheRootIndex] = value;
  }

  void public_set_empty_script(Script* script) {
    roots_[kEmptyScriptRootIndex] = script;
  }

  void public_set_store_buffer_top(Address* top) {
    roots_[kStoreBufferTopRootIndex] = reinterpret_cast<Smi*>(top);
  }

  // Update the next script id.
  inline void SetLastScriptId(Object* last_script_id);

  // Generated code can embed this address to get access to the roots.
  Object** roots_array_start() { return roots_; }

  Address* store_buffer_top_address() {
    return reinterpret_cast<Address*>(&roots_[kStoreBufferTopRootIndex]);
  }

  // Get address of native contexts list for serialization support.
  Object** native_contexts_list_address() {
    return &native_contexts_list_;
  }

#ifdef VERIFY_HEAP
  // Verify the heap is in its normal state before or after a GC.
  void Verify();
#endif

#ifdef DEBUG
  void Print();
  void PrintHandles();

  void OldPointerSpaceCheckStoreBuffer();
  void MapSpaceCheckStoreBuffer();
  void LargeObjectSpaceCheckStoreBuffer();

  // Report heap statistics.
  void ReportHeapStatistics(const char* title);
  void ReportCodeStatistics(const char* title);
#endif

  // Zapping is needed for verify heap, and always done in debug builds.
  static inline bool ShouldZapGarbage() {
#ifdef DEBUG
    return true;
#else
#ifdef VERIFY_HEAP
    return FLAG_verify_heap;
#else
    return false;
#endif
#endif
  }

  // Fill in bogus values in from space
  void ZapFromSpace();

  // Print short heap statistics.
  void PrintShortHeapStatistics();

  // Makes a new symbol object
  // Returns Failure::RetryAfterGC(requested_bytes, space) if the allocation
  // failed.
  // Please note this function does not perform a garbage collection.
  MUST_USE_RESULT MaybeObject* CreateSymbol(
      const char* str, int length, int hash);
  MUST_USE_RESULT MaybeObject* CreateSymbol(String* str);

  // Write barrier support for address[offset] = o.
  inline void RecordWrite(Address address, int offset);

  // Write barrier support for address[start : start + len[ = o.
  inline void RecordWrites(Address address, int start, int len);

  // Given an address occupied by a live code object, return that object.
  Object* FindCodeObject(Address a);

  // Invoke Shrink on shrinkable spaces.
  void Shrink();

  enum HeapState { NOT_IN_GC, SCAVENGE, MARK_COMPACT };
  inline HeapState gc_state() { return gc_state_; }

  inline bool IsInGCPostProcessing() { return gc_post_processing_depth_ > 0; }

#ifdef DEBUG
  bool IsAllocationAllowed() { return allocation_allowed_; }
  inline bool allow_allocation(bool enable);

  bool disallow_allocation_failure() {
    return disallow_allocation_failure_;
  }

  void TracePathToObjectFrom(Object* target, Object* root);
  void TracePathToObject(Object* target);
  void TracePathToGlobal();
#endif

  // Callback function passed to Heap::Iterate etc.  Copies an object if
  // necessary, the object might be promoted to an old space.  The caller must
  // ensure the precondition that the object is (a) a heap object and (b) in
  // the heap's from space.
  static inline void ScavengePointer(HeapObject** p);
  static inline void ScavengeObject(HeapObject** p, HeapObject* object);

  // Commits from space if it is uncommitted.
  void EnsureFromSpaceIsCommitted();

  // Support for partial snapshots.  After calling this we have a linear
  // space to write objects in each space.
  void ReserveSpace(int *sizes, Address* addresses);

  //
  // Support for the API.
  //

  bool CreateApiObjects();

  // Attempt to find the number in a small cache.  If we finds it, return
  // the string representation of the number.  Otherwise return undefined.
  Object* GetNumberStringCache(Object* number);

  // Update the cache with a new number-string pair.
  void SetNumberStringCache(Object* number, String* str);

  // Adjusts the amount of registered external memory.
  // Returns the adjusted value.
  inline intptr_t AdjustAmountOfExternalAllocatedMemory(
      intptr_t change_in_bytes);

  // Allocate uninitialized fixed array.
  MUST_USE_RESULT MaybeObject* AllocateRawFixedArray(int length);
  MUST_USE_RESULT MaybeObject* AllocateRawFixedArray(int length,
                                                     PretenureFlag pretenure);

  inline intptr_t PromotedTotalSize() {
    return PromotedSpaceSizeOfObjects() + PromotedExternalMemorySize();
  }

  // True if we have reached the allocation limit in the old generation that
  // should force the next GC (caused normally) to be a full one.
  inline bool OldGenerationPromotionLimitReached() {
    return PromotedTotalSize() > old_gen_promotion_limit_;
  }

  inline intptr_t OldGenerationSpaceAvailable() {
    return old_gen_allocation_limit_ - PromotedTotalSize();
  }

  inline intptr_t OldGenerationCapacityAvailable() {
    return max_old_generation_size_ - PromotedTotalSize();
  }

  static const intptr_t kMinimumPromotionLimit = 5 * Page::kPageSize;
  static const intptr_t kMinimumAllocationLimit =
      8 * (Page::kPageSize > MB ? Page::kPageSize : MB);

  intptr_t OldGenPromotionLimit(intptr_t old_gen_size) {
    const int divisor = FLAG_stress_compaction ? 10 : 3;
    intptr_t limit =
        Max(old_gen_size + old_gen_size / divisor, kMinimumPromotionLimit);
    limit += new_space_.Capacity();
    limit *= old_gen_limit_factor_;
    intptr_t halfway_to_the_max = (old_gen_size + max_old_generation_size_) / 2;
    return Min(limit, halfway_to_the_max);
  }

  intptr_t OldGenAllocationLimit(intptr_t old_gen_size) {
    const int divisor = FLAG_stress_compaction ? 8 : 2;
    intptr_t limit =
        Max(old_gen_size + old_gen_size / divisor, kMinimumAllocationLimit);
    limit += new_space_.Capacity();
    limit *= old_gen_limit_factor_;
    intptr_t halfway_to_the_max = (old_gen_size + max_old_generation_size_) / 2;
    return Min(limit, halfway_to_the_max);
  }

  // Implements the corresponding V8 API function.
  bool IdleNotification(int hint);

  // Declare all the root indices.
  enum RootListIndex {
#define ROOT_INDEX_DECLARATION(type, name, camel_name) k##camel_name##RootIndex,
    STRONG_ROOT_LIST(ROOT_INDEX_DECLARATION)
#undef ROOT_INDEX_DECLARATION

#define SYMBOL_INDEX_DECLARATION(name, str) k##name##RootIndex,
    SYMBOL_LIST(SYMBOL_INDEX_DECLARATION)
#undef SYMBOL_DECLARATION

    // Utility type maps
#define DECLARE_STRUCT_MAP(NAME, Name, name) k##Name##MapRootIndex,
    STRUCT_LIST(DECLARE_STRUCT_MAP)
#undef DECLARE_STRUCT_MAP

    kSymbolTableRootIndex,
    kStrongRootListLength = kSymbolTableRootIndex,
    kRootListLength
  };

  STATIC_CHECK(kUndefinedValueRootIndex == Internals::kUndefinedValueRootIndex);
  STATIC_CHECK(kNullValueRootIndex == Internals::kNullValueRootIndex);
  STATIC_CHECK(kTrueValueRootIndex == Internals::kTrueValueRootIndex);
  STATIC_CHECK(kFalseValueRootIndex == Internals::kFalseValueRootIndex);
  STATIC_CHECK(kempty_symbolRootIndex == Internals::kEmptySymbolRootIndex);

  MUST_USE_RESULT MaybeObject* NumberToString(
      Object* number, bool check_number_string_cache = true);
  MUST_USE_RESULT MaybeObject* Uint32ToString(
      uint32_t value, bool check_number_string_cache = true);

  Map* MapForExternalArrayType(ExternalArrayType array_type);
  RootListIndex RootIndexForExternalArrayType(
      ExternalArrayType array_type);

  void RecordStats(HeapStats* stats, bool take_snapshot = false);

  // Copy block of memory from src to dst. Size of block should be aligned
  // by pointer size.
  static inline void CopyBlock(Address dst, Address src, int byte_size);

  // Optimized version of memmove for blocks with pointer size aligned sizes and
  // pointer size aligned addresses.
  static inline void MoveBlock(Address dst, Address src, int byte_size);

  // Check new space expansion criteria and expand semispaces if it was hit.
  void CheckNewSpaceExpansionCriteria();

  inline void IncrementYoungSurvivorsCounter(int survived) {
    ASSERT(survived >= 0);
    young_survivors_after_last_gc_ = survived;
    survived_since_last_expansion_ += survived;
  }

  inline bool NextGCIsLikelyToBeFull() {
    if (FLAG_gc_global) return true;

    if (FLAG_stress_compaction && (gc_count_ & 1) != 0) return true;

    intptr_t total_promoted = PromotedTotalSize();

    intptr_t adjusted_promotion_limit =
        old_gen_promotion_limit_ - new_space_.Capacity();

    if (total_promoted >= adjusted_promotion_limit) return true;

    intptr_t adjusted_allocation_limit =
        old_gen_allocation_limit_ - new_space_.Capacity() / 5;

    if (PromotedSpaceSizeOfObjects() >= adjusted_allocation_limit) return true;

    return false;
  }


  void UpdateNewSpaceReferencesInExternalStringTable(
      ExternalStringTableUpdaterCallback updater_func);

  void UpdateReferencesInExternalStringTable(
      ExternalStringTableUpdaterCallback updater_func);

  void ProcessWeakReferences(WeakObjectRetainer* retainer);

  void VisitExternalResources(v8::ExternalResourceVisitor* visitor);

  // Helper function that governs the promotion policy from new space to
  // old.  If the object's old address lies below the new space's age
  // mark or if we've already filled the bottom 1/16th of the to space,
  // we try to promote this object.
  inline bool ShouldBePromoted(Address old_address, int object_size);

  int MaxObjectSizeInNewSpace() { return kMaxObjectSizeInNewSpace; }

  void ClearJSFunctionResultCaches();

  void ClearNormalizedMapCaches();

  GCTracer* tracer() { return tracer_; }

  // Returns the size of objects residing in non new spaces.
  intptr_t PromotedSpaceSizeOfObjects();

  double total_regexp_code_generated() { return total_regexp_code_generated_; }
  void IncreaseTotalRegexpCodeGenerated(int size) {
    total_regexp_code_generated_ += size;
  }

  // Returns maximum GC pause.
  int get_max_gc_pause() { return max_gc_pause_; }

  // Returns maximum size of objects alive after GC.
  intptr_t get_max_alive_after_gc() { return max_alive_after_gc_; }

  // Returns minimal interval between two subsequent collections.
  int get_min_in_mutator() { return min_in_mutator_; }

  MarkCompactCollector* mark_compact_collector() {
    return &mark_compact_collector_;
  }

  StoreBuffer* store_buffer() {
    return &store_buffer_;
  }

  Marking* marking() {
    return &marking_;
  }

  IncrementalMarking* incremental_marking() {
    return &incremental_marking_;
  }

  bool IsSweepingComplete() {
    return old_data_space()->IsSweepingComplete() &&
           old_pointer_space()->IsSweepingComplete();
  }

  bool AdvanceSweepers(int step_size) {
    bool sweeping_complete = old_data_space()->AdvanceSweeper(step_size);
    sweeping_complete &= old_pointer_space()->AdvanceSweeper(step_size);
    return sweeping_complete;
  }

  ExternalStringTable* external_string_table() {
    return &external_string_table_;
  }

  // Returns the current sweep generation.
  int sweep_generation() {
    return sweep_generation_;
  }

  inline Isolate* isolate();

  inline void CallGlobalGCPrologueCallback() {
    if (global_gc_prologue_callback_ != NULL) global_gc_prologue_callback_();
  }

  inline void CallGlobalGCEpilogueCallback() {
    if (global_gc_epilogue_callback_ != NULL) global_gc_epilogue_callback_();
  }

  inline bool OldGenerationAllocationLimitReached();

  inline void DoScavengeObject(Map* map, HeapObject** slot, HeapObject* obj) {
    scavenging_visitors_table_.GetVisitor(map)(map, slot, obj);
  }

  void QueueMemoryChunkForFree(MemoryChunk* chunk);
  void FreeQueuedChunks();

  // Completely clear the Instanceof cache (to stop it keeping objects alive
  // around a GC).
  inline void CompletelyClearInstanceofCache();

  // The roots that have an index less than this are always in old space.
  static const int kOldSpaceRoots = 0x20;

  uint32_t HashSeed() {
    uint32_t seed = static_cast<uint32_t>(hash_seed()->value());
    ASSERT(FLAG_randomize_hashes || seed == 0);
    return seed;
  }

  void SetArgumentsAdaptorDeoptPCOffset(int pc_offset) {
    ASSERT(arguments_adaptor_deopt_pc_offset() == Smi::FromInt(0));
    set_arguments_adaptor_deopt_pc_offset(Smi::FromInt(pc_offset));
  }

  void SetConstructStubDeoptPCOffset(int pc_offset) {
    ASSERT(construct_stub_deopt_pc_offset() == Smi::FromInt(0));
    set_construct_stub_deopt_pc_offset(Smi::FromInt(pc_offset));
  }

  void SetGetterStubDeoptPCOffset(int pc_offset) {
    ASSERT(getter_stub_deopt_pc_offset() == Smi::FromInt(0));
    set_getter_stub_deopt_pc_offset(Smi::FromInt(pc_offset));
  }

  void SetSetterStubDeoptPCOffset(int pc_offset) {
    ASSERT(setter_stub_deopt_pc_offset() == Smi::FromInt(0));
    set_setter_stub_deopt_pc_offset(Smi::FromInt(pc_offset));
  }

  // For post mortem debugging.
  void RememberUnmappedPage(Address page, bool compacted);

  // Global inline caching age: it is incremented on some GCs after context
  // disposal. We use it to flush inline caches.
  int global_ic_age() {
    return global_ic_age_;
  }

  void AgeInlineCaches() {
    global_ic_age_ = (global_ic_age_ + 1) & SharedFunctionInfo::ICAgeBits::kMax;
  }

  bool flush_monomorphic_ics() { return flush_monomorphic_ics_; }

  intptr_t amount_of_external_allocated_memory() {
    return amount_of_external_allocated_memory_;
  }

  // ObjectStats are kept in two arrays, counts and sizes. Related stats are
  // stored in a contiguous linear buffer. Stats groups are stored one after
  // another.
  enum {
    FIRST_CODE_KIND_SUB_TYPE = LAST_TYPE + 1,
    FIRST_FIXED_ARRAY_SUB_TYPE =
        FIRST_CODE_KIND_SUB_TYPE + Code::LAST_CODE_KIND + 1,
    OBJECT_STATS_COUNT =
        FIRST_FIXED_ARRAY_SUB_TYPE + LAST_FIXED_ARRAY_SUB_TYPE + 1
  };

  void RecordObjectStats(InstanceType type, int sub_type, size_t size) {
    ASSERT(type <= LAST_TYPE);
    if (sub_type < 0) {
      object_counts_[type]++;
      object_sizes_[type] += size;
    } else {
      if (type == CODE_TYPE) {
        ASSERT(sub_type <= Code::LAST_CODE_KIND);
        object_counts_[FIRST_CODE_KIND_SUB_TYPE + sub_type]++;
        object_sizes_[FIRST_CODE_KIND_SUB_TYPE + sub_type] += size;
      } else if (type == FIXED_ARRAY_TYPE) {
        ASSERT(sub_type <= LAST_FIXED_ARRAY_SUB_TYPE);
        object_counts_[FIRST_FIXED_ARRAY_SUB_TYPE + sub_type]++;
        object_sizes_[FIRST_FIXED_ARRAY_SUB_TYPE + sub_type] += size;
      }
    }
  }

  void CheckpointObjectStats();

  // We don't use a ScopedLock here since we want to lock the heap
  // only when FLAG_parallel_recompilation is true.
  class RelocationLock {
   public:
    explicit RelocationLock(Heap* heap) : heap_(heap) {
      if (FLAG_parallel_recompilation) {
        heap_->relocation_mutex_->Lock();
      }
    }
    ~RelocationLock() {
      if (FLAG_parallel_recompilation) {
        heap_->relocation_mutex_->Unlock();
      }
    }

   private:
    Heap* heap_;
  };

 private:
  Heap();

  // This can be calculated directly from a pointer to the heap; however, it is
  // more expedient to get at the isolate directly from within Heap methods.
  Isolate* isolate_;

  Object* roots_[kRootListLength];

  intptr_t code_range_size_;
  int reserved_semispace_size_;
  int max_semispace_size_;
  int initial_semispace_size_;
  intptr_t max_old_generation_size_;
  intptr_t max_executable_size_;

  // For keeping track of how much data has survived
  // scavenge since last new space expansion.
  int survived_since_last_expansion_;

  // For keeping track on when to flush RegExp code.
  int sweep_generation_;

  int always_allocate_scope_depth_;
  int linear_allocation_scope_depth_;

  // For keeping track of context disposals.
  int contexts_disposed_;

  int global_ic_age_;

  bool flush_monomorphic_ics_;

  int scan_on_scavenge_pages_;

#if defined(V8_TARGET_ARCH_X64)
  static const int kMaxObjectSizeInNewSpace = 1024*KB;
#else
  static const int kMaxObjectSizeInNewSpace = 512*KB;
#endif

  NewSpace new_space_;
  OldSpace* old_pointer_space_;
  OldSpace* old_data_space_;
  OldSpace* code_space_;
  MapSpace* map_space_;
  CellSpace* cell_space_;
  LargeObjectSpace* lo_space_;
  HeapState gc_state_;
  int gc_post_processing_depth_;

  // Returns the amount of external memory registered since last global gc.
  intptr_t PromotedExternalMemorySize();

  unsigned int ms_count_;  // how many mark-sweep collections happened
  unsigned int gc_count_;  // how many gc happened

  // For post mortem debugging.
  static const int kRememberedUnmappedPages = 128;
  int remembered_unmapped_pages_index_;
  Address remembered_unmapped_pages_[kRememberedUnmappedPages];

  // Total length of the strings we failed to flatten since the last GC.
  int unflattened_strings_length_;

#define ROOT_ACCESSOR(type, name, camel_name)                                  \
  inline void set_##name(type* value) {                                        \
    /* The deserializer makes use of the fact that these common roots are */   \
    /* never in new space and never on a page that is being compacted.    */   \
    ASSERT(k##camel_name##RootIndex >= kOldSpaceRoots || !InNewSpace(value));  \
    roots_[k##camel_name##RootIndex] = value;                                  \
  }
  ROOT_LIST(ROOT_ACCESSOR)
#undef ROOT_ACCESSOR

#ifdef DEBUG
  bool allocation_allowed_;

  // If the --gc-interval flag is set to a positive value, this
  // variable holds the value indicating the number of allocations
  // remain until the next failure and garbage collection.
  int allocation_timeout_;

  // Do we expect to be able to handle allocation failure at this
  // time?
  bool disallow_allocation_failure_;

  HeapDebugUtils* debug_utils_;
#endif  // DEBUG

  // Indicates that the new space should be kept small due to high promotion
  // rates caused by the mutator allocating a lot of long-lived objects.
  bool new_space_high_promotion_mode_active_;

  // Limit that triggers a global GC on the next (normally caused) GC.  This
  // is checked when we have already decided to do a GC to help determine
  // which collector to invoke.
  intptr_t old_gen_promotion_limit_;

  // Limit that triggers a global GC as soon as is reasonable.  This is
  // checked before expanding a paged space in the old generation and on
  // every allocation in large object space.
  intptr_t old_gen_allocation_limit_;

  // Sometimes the heuristics dictate that those limits are increased.  This
  // variable records that fact.
  int old_gen_limit_factor_;

  // Used to adjust the limits that control the timing of the next GC.
  intptr_t size_of_old_gen_at_last_old_space_gc_;

  // Limit on the amount of externally allocated memory allowed
  // between global GCs. If reached a global GC is forced.
  intptr_t external_allocation_limit_;

  // The amount of external memory registered through the API kept alive
  // by global handles
  intptr_t amount_of_external_allocated_memory_;

  // Caches the amount of external memory registered at the last global gc.
  intptr_t amount_of_external_allocated_memory_at_last_global_gc_;

  // Indicates that an allocation has failed in the old generation since the
  // last GC.
  int old_gen_exhausted_;

  Object* native_contexts_list_;

  StoreBufferRebuilder store_buffer_rebuilder_;

  struct StringTypeTable {
    InstanceType type;
    int size;
    RootListIndex index;
  };

  struct ConstantSymbolTable {
    const char* contents;
    RootListIndex index;
  };

  struct StructTable {
    InstanceType type;
    int size;
    RootListIndex index;
  };

  static const StringTypeTable string_type_table[];
  static const ConstantSymbolTable constant_symbol_table[];
  static const StructTable struct_table[];

  // The special hidden symbol which is an empty string, but does not match
  // any string when looked up in properties.
  String* hidden_symbol_;

  // GC callback function, called before and after mark-compact GC.
  // Allocations in the callback function are disallowed.
  struct GCPrologueCallbackPair {
    GCPrologueCallbackPair(GCPrologueCallback callback, GCType gc_type)
        : callback(callback), gc_type(gc_type) {
    }
    bool operator==(const GCPrologueCallbackPair& pair) const {
      return pair.callback == callback;
    }
    GCPrologueCallback callback;
    GCType gc_type;
  };
  List<GCPrologueCallbackPair> gc_prologue_callbacks_;

  struct GCEpilogueCallbackPair {
    GCEpilogueCallbackPair(GCEpilogueCallback callback, GCType gc_type)
        : callback(callback), gc_type(gc_type) {
    }
    bool operator==(const GCEpilogueCallbackPair& pair) const {
      return pair.callback == callback;
    }
    GCEpilogueCallback callback;
    GCType gc_type;
  };
  List<GCEpilogueCallbackPair> gc_epilogue_callbacks_;

  GCCallback global_gc_prologue_callback_;
  GCCallback global_gc_epilogue_callback_;

  // Support for computing object sizes during GC.
  HeapObjectCallback gc_safe_size_of_old_object_;
  static int GcSafeSizeOfOldObject(HeapObject* object);

  // Update the GC state. Called from the mark-compact collector.
  void MarkMapPointersAsEncoded(bool encoded) {
    ASSERT(!encoded);
    gc_safe_size_of_old_object_ = &GcSafeSizeOfOldObject;
  }

  // Checks whether a global GC is necessary
  GarbageCollector SelectGarbageCollector(AllocationSpace space,
                                          const char** reason);

  // Performs garbage collection
  // Returns whether there is a chance another major GC could
  // collect more garbage.
  bool PerformGarbageCollection(GarbageCollector collector,
                                GCTracer* tracer);


  inline void UpdateOldSpaceLimits();

  // Allocate an uninitialized object in map space.  The behavior is identical
  // to Heap::AllocateRaw(size_in_bytes, MAP_SPACE), except that (a) it doesn't
  // have to test the allocation space argument and (b) can reduce code size
  // (since both AllocateRaw and AllocateRawMap are inlined).
  MUST_USE_RESULT inline MaybeObject* AllocateRawMap();

  // Allocate an uninitialized object in the global property cell space.
  MUST_USE_RESULT inline MaybeObject* AllocateRawCell();

  // Initializes a JSObject based on its map.
  void InitializeJSObjectFromMap(JSObject* obj,
                                 FixedArray* properties,
                                 Map* map);

  bool CreateInitialMaps();
  bool CreateInitialObjects();

  // These five Create*EntryStub functions are here and forced to not be inlined
  // because of a gcc-4.4 bug that assigns wrong vtable entries.
  NO_INLINE(void CreateJSEntryStub());
  NO_INLINE(void CreateJSConstructEntryStub());

  void CreateFixedStubs();

  MaybeObject* CreateOddball(const char* to_string,
                             Object* to_number,
                             byte kind);

  // Allocate a JSArray with no elements
  MUST_USE_RESULT MaybeObject* AllocateJSArray(
      ElementsKind elements_kind,
      PretenureFlag pretenure = NOT_TENURED);

  // Allocate empty fixed array.
  MUST_USE_RESULT MaybeObject* AllocateEmptyFixedArray();

  // Allocate empty fixed double array.
  MUST_USE_RESULT MaybeObject* AllocateEmptyFixedDoubleArray();

  // Performs a minor collection in new generation.
  void Scavenge();

  static String* UpdateNewSpaceReferenceInExternalStringTableEntry(
      Heap* heap,
      Object** pointer);

  Address DoScavenge(ObjectVisitor* scavenge_visitor, Address new_space_front);
  static void ScavengeStoreBufferCallback(Heap* heap,
                                          MemoryChunk* page,
                                          StoreBufferEvent event);

  // Performs a major collection in the whole heap.
  void MarkCompact(GCTracer* tracer);

  // Code to be run before and after mark-compact.
  void MarkCompactPrologue();

  // Record statistics before and after garbage collection.
  void ReportStatisticsBeforeGC();
  void ReportStatisticsAfterGC();

  // Slow part of scavenge object.
  static void ScavengeObjectSlow(HeapObject** p, HeapObject* object);

  // Initializes a function with a shared part and prototype.
  // Note: this code was factored out of AllocateFunction such that
  // other parts of the VM could use it. Specifically, a function that creates
  // instances of type JS_FUNCTION_TYPE benefit from the use of this function.
  // Please note this does not perform a garbage collection.
  inline void InitializeFunction(
      JSFunction* function,
      SharedFunctionInfo* shared,
      Object* prototype);

  // Total RegExp code ever generated
  double total_regexp_code_generated_;

  GCTracer* tracer_;


  // Allocates a small number to string cache.
  MUST_USE_RESULT MaybeObject* AllocateInitialNumberStringCache();
  // Creates and installs the full-sized number string cache.
  void AllocateFullSizeNumberStringCache();
  // Get the length of the number to string cache based on the max semispace
  // size.
  int FullSizeNumberStringCacheLength();
  // Flush the number to string cache.
  void FlushNumberStringCache();

  void UpdateSurvivalRateTrend(int start_new_space_size);

  enum SurvivalRateTrend { INCREASING, STABLE, DECREASING, FLUCTUATING };

  static const int kYoungSurvivalRateHighThreshold = 90;
  static const int kYoungSurvivalRateLowThreshold = 10;
  static const int kYoungSurvivalRateAllowedDeviation = 15;

  int young_survivors_after_last_gc_;
  int high_survival_rate_period_length_;
  int low_survival_rate_period_length_;
  double survival_rate_;
  SurvivalRateTrend previous_survival_rate_trend_;
  SurvivalRateTrend survival_rate_trend_;

  void set_survival_rate_trend(SurvivalRateTrend survival_rate_trend) {
    ASSERT(survival_rate_trend != FLUCTUATING);
    previous_survival_rate_trend_ = survival_rate_trend_;
    survival_rate_trend_ = survival_rate_trend;
  }

  SurvivalRateTrend survival_rate_trend() {
    if (survival_rate_trend_ == STABLE) {
      return STABLE;
    } else if (previous_survival_rate_trend_ == STABLE) {
      return survival_rate_trend_;
    } else if (survival_rate_trend_ != previous_survival_rate_trend_) {
      return FLUCTUATING;
    } else {
      return survival_rate_trend_;
    }
  }

  bool IsStableOrIncreasingSurvivalTrend() {
    switch (survival_rate_trend()) {
      case STABLE:
      case INCREASING:
        return true;
      default:
        return false;
    }
  }

  bool IsStableOrDecreasingSurvivalTrend() {
    switch (survival_rate_trend()) {
      case STABLE:
      case DECREASING:
        return true;
      default:
        return false;
    }
  }

  bool IsIncreasingSurvivalTrend() {
    return survival_rate_trend() == INCREASING;
  }

  bool IsHighSurvivalRate() {
    return high_survival_rate_period_length_ > 0;
  }

  bool IsLowSurvivalRate() {
    return low_survival_rate_period_length_ > 0;
  }

  void SelectScavengingVisitorsTable();

  void StartIdleRound() {
    mark_sweeps_since_idle_round_started_ = 0;
    ms_count_at_last_idle_notification_ = ms_count_;
  }

  void FinishIdleRound() {
    mark_sweeps_since_idle_round_started_ = kMaxMarkSweepsInIdleRound;
    scavenges_since_last_idle_round_ = 0;
  }

  bool EnoughGarbageSinceLastIdleRound() {
    return (scavenges_since_last_idle_round_ >= kIdleScavengeThreshold);
  }

  // Estimates how many milliseconds a Mark-Sweep would take to complete.
  // In idle notification handler we assume that this function will return:
  // - a number less than 10 for small heaps, which are less than 8Mb.
  // - a number greater than 10 for large heaps, which are greater than 32Mb.
  int TimeMarkSweepWouldTakeInMs() {
    // Rough estimate of how many megabytes of heap can be processed in 1 ms.
    static const int kMbPerMs = 2;

    int heap_size_mb = static_cast<int>(SizeOfObjects() / MB);
    return heap_size_mb / kMbPerMs;
  }

  // Returns true if no more GC work is left.
  bool IdleGlobalGC();

  void AdvanceIdleIncrementalMarking(intptr_t step_size);

  void ClearObjectStats(bool clear_last_time_stats = false);

  static const int kInitialSymbolTableSize = 2048;
  static const int kInitialEvalCacheSize = 64;
  static const int kInitialNumberStringCacheSize = 256;

  // Object counts and used memory by InstanceType
  size_t object_counts_[OBJECT_STATS_COUNT];
  size_t object_counts_last_time_[OBJECT_STATS_COUNT];
  size_t object_sizes_[OBJECT_STATS_COUNT];
  size_t object_sizes_last_time_[OBJECT_STATS_COUNT];

  // Maximum GC pause.
  int max_gc_pause_;

  // Total time spent in GC.
  int total_gc_time_ms_;

  // Maximum size of objects alive after GC.
  intptr_t max_alive_after_gc_;

  // Minimal interval between two subsequent collections.
  int min_in_mutator_;

  // Size of objects alive after last GC.
  intptr_t alive_after_last_gc_;

  double last_gc_end_timestamp_;

  MarkCompactCollector mark_compact_collector_;

  StoreBuffer store_buffer_;

  Marking marking_;

  IncrementalMarking incremental_marking_;

  int number_idle_notifications_;
  unsigned int last_idle_notification_gc_count_;
  bool last_idle_notification_gc_count_init_;

  int mark_sweeps_since_idle_round_started_;
  int ms_count_at_last_idle_notification_;
  unsigned int gc_count_at_last_idle_gc_;
  int scavenges_since_last_idle_round_;

  static const int kMaxMarkSweepsInIdleRound = 7;
  static const int kIdleScavengeThreshold = 5;

  // Shared state read by the scavenge collector and set by ScavengeObject.
  PromotionQueue promotion_queue_;

  // Flag is set when the heap has been configured.  The heap can be repeatedly
  // configured through the API until it is set up.
  bool configured_;

  ExternalStringTable external_string_table_;

  VisitorDispatchTable<ScavengingCallback> scavenging_visitors_table_;

  MemoryChunk* chunks_queued_for_free_;

  Mutex* relocation_mutex_;

  friend class Factory;
  friend class GCTracer;
  friend class DisallowAllocationFailure;
  friend class AlwaysAllocateScope;
  friend class Page;
  friend class Isolate;
  friend class MarkCompactCollector;
  friend class MarkCompactMarkingVisitor;
  friend class MapCompact;

  DISALLOW_COPY_AND_ASSIGN(Heap);
};


class HeapStats {
 public:
  static const int kStartMarker = 0xDECADE00;
  static const int kEndMarker = 0xDECADE01;

  int* start_marker;                    //  0
  int* new_space_size;                  //  1
  int* new_space_capacity;              //  2
  intptr_t* old_pointer_space_size;          //  3
  intptr_t* old_pointer_space_capacity;      //  4
  intptr_t* old_data_space_size;             //  5
  intptr_t* old_data_space_capacity;         //  6
  intptr_t* code_space_size;                 //  7
  intptr_t* code_space_capacity;             //  8
  intptr_t* map_space_size;                  //  9
  intptr_t* map_space_capacity;              // 10
  intptr_t* cell_space_size;                 // 11
  intptr_t* cell_space_capacity;             // 12
  intptr_t* lo_space_size;                   // 13
  int* global_handle_count;             // 14
  int* weak_global_handle_count;        // 15
  int* pending_global_handle_count;     // 16
  int* near_death_global_handle_count;  // 17
  int* free_global_handle_count;        // 18
  intptr_t* memory_allocator_size;           // 19
  intptr_t* memory_allocator_capacity;       // 20
  int* objects_per_type;                // 21
  int* size_per_type;                   // 22
  int* os_error;                        // 23
  int* end_marker;                      // 24
};


class DisallowAllocationFailure {
 public:
  inline DisallowAllocationFailure();
  inline ~DisallowAllocationFailure();

#ifdef DEBUG
 private:
  bool old_state_;
#endif
};


class AlwaysAllocateScope {
 public:
  inline AlwaysAllocateScope();
  inline ~AlwaysAllocateScope();

 private:
  // Implicitly disable artificial allocation failures.
  DisallowAllocationFailure disallow_allocation_failure_;
};


// Visitor class to verify interior pointers in spaces that do not contain
// or care about intergenerational references. All heap object pointers have to
// point into the heap to a location that has a map pointer at its first word.
// Caveat: Heap::Contains is an approximation because it can return true for
// objects in a heap space but above the allocation pointer.
class VerifyPointersVisitor: public ObjectVisitor {
 public:
  inline void VisitPointers(Object** start, Object** end);
};


// Space iterator for iterating over all spaces of the heap.
// Returns each space in turn, and null when it is done.
class AllSpaces BASE_EMBEDDED {
 public:
  Space* next();
  AllSpaces() { counter_ = FIRST_SPACE; }
 private:
  int counter_;
};


// Space iterator for iterating over all old spaces of the heap: Old pointer
// space, old data space and code space.
// Returns each space in turn, and null when it is done.
class OldSpaces BASE_EMBEDDED {
 public:
  OldSpace* next();
  OldSpaces() { counter_ = OLD_POINTER_SPACE; }
 private:
  int counter_;
};


// Space iterator for iterating over all the paged spaces of the heap:
// Map space, old pointer space, old data space, code space and cell space.
// Returns each space in turn, and null when it is done.
class PagedSpaces BASE_EMBEDDED {
 public:
  PagedSpace* next();
  PagedSpaces() { counter_ = OLD_POINTER_SPACE; }
 private:
  int counter_;
};


// Space iterator for iterating over all spaces of the heap.
// For each space an object iterator is provided. The deallocation of the
// returned object iterators is handled by the space iterator.
class SpaceIterator : public Malloced {
 public:
  SpaceIterator();
  explicit SpaceIterator(HeapObjectCallback size_func);
  virtual ~SpaceIterator();

  bool has_next();
  ObjectIterator* next();

 private:
  ObjectIterator* CreateIterator();

  int current_space_;  // from enum AllocationSpace.
  ObjectIterator* iterator_;  // object iterator for the current space.
  HeapObjectCallback size_func_;
};


// A HeapIterator provides iteration over the whole heap. It
// aggregates the specific iterators for the different spaces as
// these can only iterate over one space only.
//
// HeapIterator can skip free list nodes (that is, de-allocated heap
// objects that still remain in the heap). As implementation of free
// nodes filtering uses GC marks, it can't be used during MS/MC GC
// phases. Also, it is forbidden to interrupt iteration in this mode,
// as this will leave heap objects marked (and thus, unusable).
class HeapObjectsFilter;

class HeapIterator BASE_EMBEDDED {
 public:
  enum HeapObjectsFiltering {
    kNoFiltering,
    kFilterUnreachable
  };

  HeapIterator();
  explicit HeapIterator(HeapObjectsFiltering filtering);
  ~HeapIterator();

  HeapObject* next();
  void reset();

 private:
  // Perform the initialization.
  void Init();
  // Perform all necessary shutdown (destruction) work.
  void Shutdown();
  HeapObject* NextObject();

  HeapObjectsFiltering filtering_;
  HeapObjectsFilter* filter_;
  // Space iterator for iterating all the spaces.
  SpaceIterator* space_iterator_;
  // Object iterator for the space currently being iterated.
  ObjectIterator* object_iterator_;
};


// Cache for mapping (map, property name) into field offset.
// Cleared at startup and prior to mark sweep collection.
class KeyedLookupCache {
 public:
  // Lookup field offset for (map, name). If absent, -1 is returned.
  int Lookup(Map* map, String* name);

  // Update an element in the cache.
  void Update(Map* map, String* name, int field_offset);

  // Clear the cache.
  void Clear();

  static const int kLength = 256;
  static const int kCapacityMask = kLength - 1;
  static const int kMapHashShift = 5;
  static const int kHashMask = -4;  // Zero the last two bits.
  static const int kEntriesPerBucket = 4;
  static const int kNotFound = -1;

  // kEntriesPerBucket should be a power of 2.
  STATIC_ASSERT((kEntriesPerBucket & (kEntriesPerBucket - 1)) == 0);
  STATIC_ASSERT(kEntriesPerBucket == -kHashMask);

 private:
  KeyedLookupCache() {
    for (int i = 0; i < kLength; ++i) {
      keys_[i].map = NULL;
      keys_[i].name = NULL;
      field_offsets_[i] = kNotFound;
    }
  }

  static inline int Hash(Map* map, String* name);

  // Get the address of the keys and field_offsets arrays.  Used in
  // generated code to perform cache lookups.
  Address keys_address() {
    return reinterpret_cast<Address>(&keys_);
  }

  Address field_offsets_address() {
    return reinterpret_cast<Address>(&field_offsets_);
  }

  struct Key {
    Map* map;
    String* name;
  };

  Key keys_[kLength];
  int field_offsets_[kLength];

  friend class ExternalReference;
  friend class Isolate;
  DISALLOW_COPY_AND_ASSIGN(KeyedLookupCache);
};


// Cache for mapping (map, property name) into descriptor index.
// The cache contains both positive and negative results.
// Descriptor index equals kNotFound means the property is absent.
// Cleared at startup and prior to any gc.
class DescriptorLookupCache {
 public:
  // Lookup descriptor index for (map, name).
  // If absent, kAbsent is returned.
  int Lookup(Map* source, String* name) {
    if (!StringShape(name).IsSymbol()) return kAbsent;
    int index = Hash(source, name);
    Key& key = keys_[index];
    if ((key.source == source) && (key.name == name)) return results_[index];
    return kAbsent;
  }

  // Update an element in the cache.
  void Update(Map* source, String* name, int result) {
    ASSERT(result != kAbsent);
    if (StringShape(name).IsSymbol()) {
      int index = Hash(source, name);
      Key& key = keys_[index];
      key.source = source;
      key.name = name;
      results_[index] = result;
    }
  }

  // Clear the cache.
  void Clear();

  static const int kAbsent = -2;

 private:
  DescriptorLookupCache() {
    for (int i = 0; i < kLength; ++i) {
      keys_[i].source = NULL;
      keys_[i].name = NULL;
      results_[i] = kAbsent;
    }
  }

  static int Hash(Object* source, String* name) {
    // Uses only lower 32 bits if pointers are larger.
    uint32_t source_hash =
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(source))
            >> kPointerSizeLog2;
    uint32_t name_hash =
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(name))
            >> kPointerSizeLog2;
    return (source_hash ^ name_hash) % kLength;
  }

  static const int kLength = 64;
  struct Key {
    Map* source;
    String* name;
  };

  Key keys_[kLength];
  int results_[kLength];

  friend class Isolate;
  DISALLOW_COPY_AND_ASSIGN(DescriptorLookupCache);
};


// A helper class to document/test C++ scopes where we do not
// expect a GC. Usage:
//
// /* Allocation not allowed: we cannot handle a GC in this scope. */
// { AssertNoAllocation nogc;
//   ...
// }
class AssertNoAllocation {
 public:
  inline AssertNoAllocation();
  inline ~AssertNoAllocation();

#ifdef DEBUG
 private:
  bool old_state_;
  bool active_;
#endif
};


class DisableAssertNoAllocation {
 public:
  inline DisableAssertNoAllocation();
  inline ~DisableAssertNoAllocation();

#ifdef DEBUG
 private:
  bool old_state_;
  bool active_;
#endif
};

// GCTracer collects and prints ONE line after each garbage collector
// invocation IFF --trace_gc is used.

class GCTracer BASE_EMBEDDED {
 public:
  class Scope BASE_EMBEDDED {
   public:
    enum ScopeId {
      EXTERNAL,
      MC_MARK,
      MC_SWEEP,
      MC_SWEEP_NEWSPACE,
      MC_EVACUATE_PAGES,
      MC_UPDATE_NEW_TO_NEW_POINTERS,
      MC_UPDATE_ROOT_TO_NEW_POINTERS,
      MC_UPDATE_OLD_TO_NEW_POINTERS,
      MC_UPDATE_POINTERS_TO_EVACUATED,
      MC_UPDATE_POINTERS_BETWEEN_EVACUATED,
      MC_UPDATE_MISC_POINTERS,
      MC_FLUSH_CODE,
      kNumberOfScopes
    };

    Scope(GCTracer* tracer, ScopeId scope)
        : tracer_(tracer),
        scope_(scope) {
      start_time_ = OS::TimeCurrentMillis();
    }

    ~Scope() {
      ASSERT(scope_ < kNumberOfScopes);  // scope_ is unsigned.
      tracer_->scopes_[scope_] += OS::TimeCurrentMillis() - start_time_;
    }

   private:
    GCTracer* tracer_;
    ScopeId scope_;
    double start_time_;
  };

  explicit GCTracer(Heap* heap,
                    const char* gc_reason,
                    const char* collector_reason);
  ~GCTracer();

  // Sets the collector.
  void set_collector(GarbageCollector collector) { collector_ = collector; }

  // Sets the GC count.
  void set_gc_count(unsigned int count) { gc_count_ = count; }

  // Sets the full GC count.
  void set_full_gc_count(int count) { full_gc_count_ = count; }

  void increment_promoted_objects_size(int object_size) {
    promoted_objects_size_ += object_size;
  }

 private:
  // Returns a string matching the collector.
  const char* CollectorString();

  // Returns size of object in heap (in MB).
  inline double SizeOfHeapObjects();

  // Timestamp set in the constructor.
  double start_time_;

  // Size of objects in heap set in constructor.
  intptr_t start_object_size_;

  // Size of memory allocated from OS set in constructor.
  intptr_t start_memory_size_;

  // Type of collector.
  GarbageCollector collector_;

  // A count (including this one, e.g. the first collection is 1) of the
  // number of garbage collections.
  unsigned int gc_count_;

  // A count (including this one) of the number of full garbage collections.
  int full_gc_count_;

  // Amounts of time spent in different scopes during GC.
  double scopes_[Scope::kNumberOfScopes];

  // Total amount of space either wasted or contained in one of free lists
  // before the current GC.
  intptr_t in_free_list_or_wasted_before_gc_;

  // Difference between space used in the heap at the beginning of the current
  // collection and the end of the previous collection.
  intptr_t allocated_since_last_gc_;

  // Amount of time spent in mutator that is time elapsed between end of the
  // previous collection and the beginning of the current one.
  double spent_in_mutator_;

  // Size of objects promoted during the current collection.
  intptr_t promoted_objects_size_;

  // Incremental marking steps counters.
  int steps_count_;
  double steps_took_;
  double longest_step_;
  int steps_count_since_last_gc_;
  double steps_took_since_last_gc_;

  Heap* heap_;

  const char* gc_reason_;
  const char* collector_reason_;
};


class RegExpResultsCache {
 public:
  enum ResultsCacheType { REGEXP_MULTIPLE_INDICES, STRING_SPLIT_SUBSTRINGS };

  // Attempt to retrieve a cached result.  On failure, 0 is returned as a Smi.
  // On success, the returned result is guaranteed to be a COW-array.
  static Object* Lookup(Heap* heap,
                        String* key_string,
                        Object* key_pattern,
                        ResultsCacheType type);
  // Attempt to add value_array to the cache specified by type.  On success,
  // value_array is turned into a COW-array.
  static void Enter(Heap* heap,
                    String* key_string,
                    Object* key_pattern,
                    FixedArray* value_array,
                    ResultsCacheType type);
  static void Clear(FixedArray* cache);
  static const int kRegExpResultsCacheSize = 0x100;

 private:
  static const int kArrayEntriesPerCacheEntry = 4;
  static const int kStringOffset = 0;
  static const int kPatternOffset = 1;
  static const int kArrayOffset = 2;
};


class TranscendentalCache {
 public:
  enum Type {ACOS, ASIN, ATAN, COS, EXP, LOG, SIN, TAN, kNumberOfCaches};
  static const int kTranscendentalTypeBits = 3;
  STATIC_ASSERT((1 << kTranscendentalTypeBits) >= kNumberOfCaches);

  // Returns a heap number with f(input), where f is a math function specified
  // by the 'type' argument.
  MUST_USE_RESULT inline MaybeObject* Get(Type type, double input);

  // The cache contains raw Object pointers.  This method disposes of
  // them before a garbage collection.
  void Clear();

 private:
  class SubCache {
    static const int kCacheSize = 512;

    explicit SubCache(Type t);

    MUST_USE_RESULT inline MaybeObject* Get(double input);

    inline double Calculate(double input);

    struct Element {
      uint32_t in[2];
      Object* output;
    };

    union Converter {
      double dbl;
      uint32_t integers[2];
    };

    inline static int Hash(const Converter& c) {
      uint32_t hash = (c.integers[0] ^ c.integers[1]);
      hash ^= static_cast<int32_t>(hash) >> 16;
      hash ^= static_cast<int32_t>(hash) >> 8;
      return (hash & (kCacheSize - 1));
    }

    Element elements_[kCacheSize];
    Type type_;
    Isolate* isolate_;

    // Allow access to the caches_ array as an ExternalReference.
    friend class ExternalReference;
    // Inline implementation of the cache.
    friend class TranscendentalCacheStub;
    // For evaluating value.
    friend class TranscendentalCache;

    DISALLOW_COPY_AND_ASSIGN(SubCache);
  };

  TranscendentalCache() {
    for (int i = 0; i < kNumberOfCaches; ++i) caches_[i] = NULL;
  }

  // Used to create an external reference.
  inline Address cache_array_address();

  // Instantiation
  friend class Isolate;
  // Inline implementation of the caching.
  friend class TranscendentalCacheStub;
  // Allow access to the caches_ array as an ExternalReference.
  friend class ExternalReference;

  SubCache* caches_[kNumberOfCaches];
  DISALLOW_COPY_AND_ASSIGN(TranscendentalCache);
};


// Abstract base class for checking whether a weak object should be retained.
class WeakObjectRetainer {
 public:
  virtual ~WeakObjectRetainer() {}

  // Return whether this object should be retained. If NULL is returned the
  // object has no references. Otherwise the address of the retained object
  // should be returned as in some GC situations the object has been moved.
  virtual Object* RetainAs(Object* object) = 0;
};


// Intrusive object marking uses least significant bit of
// heap object's map word to mark objects.
// Normally all map words have least significant bit set
// because they contain tagged map pointer.
// If the bit is not set object is marked.
// All objects should be unmarked before resuming
// JavaScript execution.
class IntrusiveMarking {
 public:
  static bool IsMarked(HeapObject* object) {
    return (object->map_word().ToRawValue() & kNotMarkedBit) == 0;
  }

  static void ClearMark(HeapObject* object) {
    uintptr_t map_word = object->map_word().ToRawValue();
    object->set_map_word(MapWord::FromRawValue(map_word | kNotMarkedBit));
    ASSERT(!IsMarked(object));
  }

  static void SetMark(HeapObject* object) {
    uintptr_t map_word = object->map_word().ToRawValue();
    object->set_map_word(MapWord::FromRawValue(map_word & ~kNotMarkedBit));
    ASSERT(IsMarked(object));
  }

  static Map* MapOfMarkedObject(HeapObject* object) {
    uintptr_t map_word = object->map_word().ToRawValue();
    return MapWord::FromRawValue(map_word | kNotMarkedBit).ToMap();
  }

  static int SizeOfMarkedObject(HeapObject* object) {
    return object->SizeFromMap(MapOfMarkedObject(object));
  }

 private:
  static const uintptr_t kNotMarkedBit = 0x1;
  STATIC_ASSERT((kHeapObjectTag & kNotMarkedBit) != 0);
};


#if defined(DEBUG) || defined(LIVE_OBJECT_LIST)
// Helper class for tracing paths to a search target Object from all roots.
// The TracePathFrom() method can be used to trace paths from a specific
// object to the search target object.
class PathTracer : public ObjectVisitor {
 public:
  enum WhatToFind {
    FIND_ALL,   // Will find all matches.
    FIND_FIRST  // Will stop the search after first match.
  };

  // For the WhatToFind arg, if FIND_FIRST is specified, tracing will stop
  // after the first match.  If FIND_ALL is specified, then tracing will be
  // done for all matches.
  PathTracer(Object* search_target,
             WhatToFind what_to_find,
             VisitMode visit_mode)
      : search_target_(search_target),
        found_target_(false),
        found_target_in_trace_(false),
        what_to_find_(what_to_find),
        visit_mode_(visit_mode),
        object_stack_(20),
        no_alloc() {}

  virtual void VisitPointers(Object** start, Object** end);

  void Reset();
  void TracePathFrom(Object** root);

  bool found() const { return found_target_; }

  static Object* const kAnyGlobalObject;

 protected:
  class MarkVisitor;
  class UnmarkVisitor;

  void MarkRecursively(Object** p, MarkVisitor* mark_visitor);
  void UnmarkRecursively(Object** p, UnmarkVisitor* unmark_visitor);
  virtual void ProcessResults();

  // Tags 0, 1, and 3 are used. Use 2 for marking visited HeapObject.
  static const int kMarkTag = 2;

  Object* search_target_;
  bool found_target_;
  bool found_target_in_trace_;
  WhatToFind what_to_find_;
  VisitMode visit_mode_;
  List<Object*> object_stack_;

  AssertNoAllocation no_alloc;  // i.e. no gc allowed.

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PathTracer);
};
#endif  // DEBUG || LIVE_OBJECT_LIST

} }  // namespace v8::internal

#endif  // V8_HEAP_H_
