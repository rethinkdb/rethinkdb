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

#ifndef V8_DEOPTIMIZER_H_
#define V8_DEOPTIMIZER_H_

#include "v8.h"

#include "allocation.h"
#include "macro-assembler.h"
#include "zone-inl.h"


namespace v8 {
namespace internal {

class FrameDescription;
class TranslationIterator;
class DeoptimizingCodeListNode;
class DeoptimizedFrameInfo;

class HeapNumberMaterializationDescriptor BASE_EMBEDDED {
 public:
  HeapNumberMaterializationDescriptor(Address slot_address, double val)
      : slot_address_(slot_address), val_(val) { }

  Address slot_address() const { return slot_address_; }
  double value() const { return val_; }

 private:
  Address slot_address_;
  double val_;
};


class ArgumentsObjectMaterializationDescriptor BASE_EMBEDDED {
 public:
  ArgumentsObjectMaterializationDescriptor(Address slot_address, int argc)
      : slot_address_(slot_address), arguments_length_(argc) { }

  Address slot_address() const { return slot_address_; }
  int arguments_length() const { return arguments_length_; }

 private:
  Address slot_address_;
  int arguments_length_;
};


class OptimizedFunctionVisitor BASE_EMBEDDED {
 public:
  virtual ~OptimizedFunctionVisitor() {}

  // Function which is called before iteration of any optimized functions
  // from given native context.
  virtual void EnterContext(Context* context) = 0;

  virtual void VisitFunction(JSFunction* function) = 0;

  // Function which is called after iteration of all optimized functions
  // from given native context.
  virtual void LeaveContext(Context* context) = 0;
};


class Deoptimizer;


class DeoptimizerData {
 public:
  DeoptimizerData();
  ~DeoptimizerData();

#ifdef ENABLE_DEBUGGER_SUPPORT
  void Iterate(ObjectVisitor* v);
#endif

 private:
  MemoryChunk* eager_deoptimization_entry_code_;
  MemoryChunk* lazy_deoptimization_entry_code_;
  Deoptimizer* current_;

#ifdef ENABLE_DEBUGGER_SUPPORT
  DeoptimizedFrameInfo* deoptimized_frame_info_;
#endif

  // List of deoptimized code which still have references from active stack
  // frames. These code objects are needed by the deoptimizer when deoptimizing
  // a frame for which the code object for the function function has been
  // changed from the code present when deoptimizing was done.
  DeoptimizingCodeListNode* deoptimizing_code_list_;

  friend class Deoptimizer;

  DISALLOW_COPY_AND_ASSIGN(DeoptimizerData);
};


class Deoptimizer : public Malloced {
 public:
  enum BailoutType {
    EAGER,
    LAZY,
    OSR,
    // This last bailout type is not really a bailout, but used by the
    // debugger to deoptimize stack frames to allow inspection.
    DEBUGGER
  };

  int output_count() const { return output_count_; }

  // Number of created JS frames. Not all created frames are necessarily JS.
  int jsframe_count() const { return jsframe_count_; }

  static Deoptimizer* New(JSFunction* function,
                          BailoutType type,
                          unsigned bailout_id,
                          Address from,
                          int fp_to_sp_delta,
                          Isolate* isolate);
  static Deoptimizer* Grab(Isolate* isolate);

#ifdef ENABLE_DEBUGGER_SUPPORT
  // The returned object with information on the optimized frame needs to be
  // freed before another one can be generated.
  static DeoptimizedFrameInfo* DebuggerInspectableFrame(JavaScriptFrame* frame,
                                                        int jsframe_index,
                                                        Isolate* isolate);
  static void DeleteDebuggerInspectableFrame(DeoptimizedFrameInfo* info,
                                             Isolate* isolate);
#endif

  // Makes sure that there is enough room in the relocation
  // information of a code object to perform lazy deoptimization
  // patching. If there is not enough room a new relocation
  // information object is allocated and comments are added until it
  // is big enough.
  static void EnsureRelocSpaceForLazyDeoptimization(Handle<Code> code);

  // Deoptimize the function now. Its current optimized code will never be run
  // again and any activations of the optimized code will get deoptimized when
  // execution returns.
  static void DeoptimizeFunction(JSFunction* function);

  // Iterate over all the functions which share the same code object
  // and make them use unoptimized version.
  static void ReplaceCodeForRelatedFunctions(JSFunction* function, Code* code);

  // Deoptimize all functions in the heap.
  static void DeoptimizeAll();

  static void DeoptimizeGlobalObject(JSObject* object);

  static void VisitAllOptimizedFunctionsForContext(
      Context* context, OptimizedFunctionVisitor* visitor);

  static void VisitAllOptimizedFunctionsForGlobalObject(
      JSObject* object, OptimizedFunctionVisitor* visitor);

  static void VisitAllOptimizedFunctions(OptimizedFunctionVisitor* visitor);

  // The size in bytes of the code required at a lazy deopt patch site.
  static int patch_size();

  // Patch all stack guard checks in the unoptimized code to
  // unconditionally call replacement_code.
  static void PatchStackCheckCode(Code* unoptimized_code,
                                  Code* check_code,
                                  Code* replacement_code);

  // Patch stack guard check at instruction before pc_after in
  // the unoptimized code to unconditionally call replacement_code.
  static void PatchStackCheckCodeAt(Code* unoptimized_code,
                                    Address pc_after,
                                    Code* check_code,
                                    Code* replacement_code);

  // Change all patched stack guard checks in the unoptimized code
  // back to a normal stack guard check.
  static void RevertStackCheckCode(Code* unoptimized_code,
                                   Code* check_code,
                                   Code* replacement_code);

  // Change all patched stack guard checks in the unoptimized code
  // back to a normal stack guard check.
  static void RevertStackCheckCodeAt(Code* unoptimized_code,
                                     Address pc_after,
                                     Code* check_code,
                                     Code* replacement_code);

  ~Deoptimizer();

  void MaterializeHeapObjects(JavaScriptFrameIterator* it);
#ifdef ENABLE_DEBUGGER_SUPPORT
  void MaterializeHeapNumbersForDebuggerInspectableFrame(
      Address parameters_top,
      uint32_t parameters_size,
      Address expressions_top,
      uint32_t expressions_size,
      DeoptimizedFrameInfo* info);
#endif

  static void ComputeOutputFrames(Deoptimizer* deoptimizer);

  static Address GetDeoptimizationEntry(int id, BailoutType type);
  static int GetDeoptimizationId(Address addr, BailoutType type);
  static int GetOutputInfo(DeoptimizationOutputData* data,
                           BailoutId node_id,
                           SharedFunctionInfo* shared);

  // Code generation support.
  static int input_offset() { return OFFSET_OF(Deoptimizer, input_); }
  static int output_count_offset() {
    return OFFSET_OF(Deoptimizer, output_count_);
  }
  static int output_offset() { return OFFSET_OF(Deoptimizer, output_); }

  static int has_alignment_padding_offset() {
    return OFFSET_OF(Deoptimizer, has_alignment_padding_);
  }

  static int GetDeoptimizedCodeCount(Isolate* isolate);

  static const int kNotDeoptimizationEntry = -1;

  // Generators for the deoptimization entry code.
  class EntryGenerator BASE_EMBEDDED {
   public:
    EntryGenerator(MacroAssembler* masm, BailoutType type)
        : masm_(masm), type_(type) { }
    virtual ~EntryGenerator() { }

    void Generate();

   protected:
    MacroAssembler* masm() const { return masm_; }
    BailoutType type() const { return type_; }

    virtual void GeneratePrologue() { }

   private:
    MacroAssembler* masm_;
    Deoptimizer::BailoutType type_;
  };

  class TableEntryGenerator : public EntryGenerator {
   public:
    TableEntryGenerator(MacroAssembler* masm, BailoutType type,  int count)
        : EntryGenerator(masm, type), count_(count) { }

   protected:
    virtual void GeneratePrologue();

   private:
    int count() const { return count_; }

    int count_;
  };

  int ConvertJSFrameIndexToFrameIndex(int jsframe_index);

 private:
  static const int kNumberOfEntries = 16384;

  Deoptimizer(Isolate* isolate,
              JSFunction* function,
              BailoutType type,
              unsigned bailout_id,
              Address from,
              int fp_to_sp_delta,
              Code* optimized_code);
  void DeleteFrameDescriptions();

  void DoComputeOutputFrames();
  void DoComputeOsrOutputFrame();
  void DoComputeJSFrame(TranslationIterator* iterator, int frame_index);
  void DoComputeArgumentsAdaptorFrame(TranslationIterator* iterator,
                                      int frame_index);
  void DoComputeConstructStubFrame(TranslationIterator* iterator,
                                   int frame_index);
  void DoComputeAccessorStubFrame(TranslationIterator* iterator,
                                  int frame_index,
                                  bool is_setter_stub_frame);
  void DoTranslateCommand(TranslationIterator* iterator,
                          int frame_index,
                          unsigned output_offset);
  // Translate a command for OSR.  Updates the input offset to be used for
  // the next command.  Returns false if translation of the command failed
  // (e.g., a number conversion failed) and may or may not have updated the
  // input offset.
  bool DoOsrTranslateCommand(TranslationIterator* iterator,
                             int* input_offset);

  unsigned ComputeInputFrameSize() const;
  unsigned ComputeFixedSize(JSFunction* function) const;

  unsigned ComputeIncomingArgumentSize(JSFunction* function) const;
  unsigned ComputeOutgoingArgumentSize() const;

  Object* ComputeLiteral(int index) const;

  void AddArgumentsObject(intptr_t slot_address, int argc);
  void AddArgumentsObjectValue(intptr_t value);
  void AddDoubleValue(intptr_t slot_address, double value);

  static MemoryChunk* CreateCode(BailoutType type);
  static void GenerateDeoptimizationEntries(
      MacroAssembler* masm, int count, BailoutType type);

  // Weak handle callback for deoptimizing code objects.
  static void HandleWeakDeoptimizedCode(
      v8::Persistent<v8::Value> obj, void* data);
  static Code* FindDeoptimizingCodeFromAddress(Address addr);
  static void RemoveDeoptimizingCode(Code* code);

  // Fill the input from from a JavaScript frame. This is used when
  // the debugger needs to inspect an optimized frame. For normal
  // deoptimizations the input frame is filled in generated code.
  void FillInputFrame(Address tos, JavaScriptFrame* frame);

  Isolate* isolate_;
  JSFunction* function_;
  Code* optimized_code_;
  unsigned bailout_id_;
  BailoutType bailout_type_;
  Address from_;
  int fp_to_sp_delta_;
  int has_alignment_padding_;

  // Input frame description.
  FrameDescription* input_;
  // Number of output frames.
  int output_count_;
  // Number of output js frames.
  int jsframe_count_;
  // Array of output frame descriptions.
  FrameDescription** output_;

  List<Object*> deferred_arguments_objects_values_;
  List<ArgumentsObjectMaterializationDescriptor> deferred_arguments_objects_;
  List<HeapNumberMaterializationDescriptor> deferred_heap_numbers_;

  static const int table_entry_size_;

  friend class FrameDescription;
  friend class DeoptimizingCodeListNode;
  friend class DeoptimizedFrameInfo;
};


class FrameDescription {
 public:
  FrameDescription(uint32_t frame_size,
                   JSFunction* function);

  void* operator new(size_t size, uint32_t frame_size) {
    // Subtracts kPointerSize, as the member frame_content_ already supplies
    // the first element of the area to store the frame.
    return malloc(size + frame_size - kPointerSize);
  }

  void operator delete(void* pointer, uint32_t frame_size) {
    free(pointer);
  }

  void operator delete(void* description) {
    free(description);
  }

  uint32_t GetFrameSize() const {
    ASSERT(static_cast<uint32_t>(frame_size_) == frame_size_);
    return static_cast<uint32_t>(frame_size_);
  }

  JSFunction* GetFunction() const { return function_; }

  unsigned GetOffsetFromSlotIndex(int slot_index);

  intptr_t GetFrameSlot(unsigned offset) {
    return *GetFrameSlotPointer(offset);
  }

  double GetDoubleFrameSlot(unsigned offset) {
    intptr_t* ptr = GetFrameSlotPointer(offset);
#if V8_TARGET_ARCH_MIPS
    // Prevent gcc from using load-double (mips ldc1) on (possibly)
    // non-64-bit aligned double. Uses two lwc1 instructions.
    union conversion {
      double d;
      uint32_t u[2];
    } c;
    c.u[0] = *reinterpret_cast<uint32_t*>(ptr);
    c.u[1] = *(reinterpret_cast<uint32_t*>(ptr) + 1);
    return c.d;
#else
    return *reinterpret_cast<double*>(ptr);
#endif
  }

  void SetFrameSlot(unsigned offset, intptr_t value) {
    *GetFrameSlotPointer(offset) = value;
  }

  intptr_t GetRegister(unsigned n) const {
    ASSERT(n < ARRAY_SIZE(registers_));
    return registers_[n];
  }

  double GetDoubleRegister(unsigned n) const {
    ASSERT(n < ARRAY_SIZE(double_registers_));
    return double_registers_[n];
  }

  void SetRegister(unsigned n, intptr_t value) {
    ASSERT(n < ARRAY_SIZE(registers_));
    registers_[n] = value;
  }

  void SetDoubleRegister(unsigned n, double value) {
    ASSERT(n < ARRAY_SIZE(double_registers_));
    double_registers_[n] = value;
  }

  intptr_t GetTop() const { return top_; }
  void SetTop(intptr_t top) { top_ = top; }

  intptr_t GetPc() const { return pc_; }
  void SetPc(intptr_t pc) { pc_ = pc; }

  intptr_t GetFp() const { return fp_; }
  void SetFp(intptr_t fp) { fp_ = fp; }

  intptr_t GetContext() const { return context_; }
  void SetContext(intptr_t context) { context_ = context; }

  Smi* GetState() const { return state_; }
  void SetState(Smi* state) { state_ = state; }

  void SetContinuation(intptr_t pc) { continuation_ = pc; }

  StackFrame::Type GetFrameType() const { return type_; }
  void SetFrameType(StackFrame::Type type) { type_ = type; }

  // Get the incoming arguments count.
  int ComputeParametersCount();

  // Get a parameter value for an unoptimized frame.
  Object* GetParameter(int index);

  // Get the expression stack height for a unoptimized frame.
  unsigned GetExpressionCount();

  // Get the expression stack value for an unoptimized frame.
  Object* GetExpression(int index);

  static int registers_offset() {
    return OFFSET_OF(FrameDescription, registers_);
  }

  static int double_registers_offset() {
    return OFFSET_OF(FrameDescription, double_registers_);
  }

  static int frame_size_offset() {
    return OFFSET_OF(FrameDescription, frame_size_);
  }

  static int pc_offset() {
    return OFFSET_OF(FrameDescription, pc_);
  }

  static int state_offset() {
    return OFFSET_OF(FrameDescription, state_);
  }

  static int continuation_offset() {
    return OFFSET_OF(FrameDescription, continuation_);
  }

  static int frame_content_offset() {
    return OFFSET_OF(FrameDescription, frame_content_);
  }

 private:
  static const uint32_t kZapUint32 = 0xbeeddead;

  // Frame_size_ must hold a uint32_t value.  It is only a uintptr_t to
  // keep the variable-size array frame_content_ of type intptr_t at
  // the end of the structure aligned.
  uintptr_t frame_size_;  // Number of bytes.
  JSFunction* function_;
  intptr_t registers_[Register::kNumRegisters];
  double double_registers_[DoubleRegister::kNumAllocatableRegisters];
  intptr_t top_;
  intptr_t pc_;
  intptr_t fp_;
  intptr_t context_;
  StackFrame::Type type_;
  Smi* state_;
#ifdef DEBUG
  Code::Kind kind_;
#endif

  // Continuation is the PC where the execution continues after
  // deoptimizing.
  intptr_t continuation_;

  // This must be at the end of the object as the object is allocated larger
  // than it's definition indicate to extend this array.
  intptr_t frame_content_[1];

  intptr_t* GetFrameSlotPointer(unsigned offset) {
    ASSERT(offset < frame_size_);
    return reinterpret_cast<intptr_t*>(
        reinterpret_cast<Address>(this) + frame_content_offset() + offset);
  }

  int ComputeFixedSize();
};


class TranslationBuffer BASE_EMBEDDED {
 public:
  explicit TranslationBuffer(Zone* zone) : contents_(256, zone) { }

  int CurrentIndex() const { return contents_.length(); }
  void Add(int32_t value, Zone* zone);

  Handle<ByteArray> CreateByteArray();

 private:
  ZoneList<uint8_t> contents_;
};


class TranslationIterator BASE_EMBEDDED {
 public:
  TranslationIterator(ByteArray* buffer, int index)
      : buffer_(buffer), index_(index) {
    ASSERT(index >= 0 && index < buffer->length());
  }

  int32_t Next();

  bool HasNext() const { return index_ < buffer_->length(); }

  void Skip(int n) {
    for (int i = 0; i < n; i++) Next();
  }

 private:
  ByteArray* buffer_;
  int index_;
};


class Translation BASE_EMBEDDED {
 public:
  enum Opcode {
    BEGIN,
    JS_FRAME,
    CONSTRUCT_STUB_FRAME,
    GETTER_STUB_FRAME,
    SETTER_STUB_FRAME,
    ARGUMENTS_ADAPTOR_FRAME,
    REGISTER,
    INT32_REGISTER,
    UINT32_REGISTER,
    DOUBLE_REGISTER,
    STACK_SLOT,
    INT32_STACK_SLOT,
    UINT32_STACK_SLOT,
    DOUBLE_STACK_SLOT,
    LITERAL,
    ARGUMENTS_OBJECT,

    // A prefix indicating that the next command is a duplicate of the one
    // that follows it.
    DUPLICATE
  };

  Translation(TranslationBuffer* buffer, int frame_count, int jsframe_count,
              Zone* zone)
      : buffer_(buffer),
        index_(buffer->CurrentIndex()),
        zone_(zone) {
    buffer_->Add(BEGIN, zone);
    buffer_->Add(frame_count, zone);
    buffer_->Add(jsframe_count, zone);
  }

  int index() const { return index_; }

  // Commands.
  void BeginJSFrame(BailoutId node_id, int literal_id, unsigned height);
  void BeginArgumentsAdaptorFrame(int literal_id, unsigned height);
  void BeginConstructStubFrame(int literal_id, unsigned height);
  void BeginGetterStubFrame(int literal_id);
  void BeginSetterStubFrame(int literal_id);
  void StoreRegister(Register reg);
  void StoreInt32Register(Register reg);
  void StoreUint32Register(Register reg);
  void StoreDoubleRegister(DoubleRegister reg);
  void StoreStackSlot(int index);
  void StoreInt32StackSlot(int index);
  void StoreUint32StackSlot(int index);
  void StoreDoubleStackSlot(int index);
  void StoreLiteral(int literal_id);
  void StoreArgumentsObject(int args_index, int args_length);
  void MarkDuplicate();

  Zone* zone() const { return zone_; }

  static int NumberOfOperandsFor(Opcode opcode);

#if defined(OBJECT_PRINT) || defined(ENABLE_DISASSEMBLER)
  static const char* StringFor(Opcode opcode);
#endif

  // A literal id which refers to the JSFunction itself.
  static const int kSelfLiteralId = -239;

 private:
  TranslationBuffer* buffer_;
  int index_;
  Zone* zone_;
};


// Linked list holding deoptimizing code objects. The deoptimizing code objects
// are kept as weak handles until they are no longer activated on the stack.
class DeoptimizingCodeListNode : public Malloced {
 public:
  explicit DeoptimizingCodeListNode(Code* code);
  ~DeoptimizingCodeListNode();

  DeoptimizingCodeListNode* next() const { return next_; }
  void set_next(DeoptimizingCodeListNode* next) { next_ = next; }
  Handle<Code> code() const { return code_; }

 private:
  // Global (weak) handle to the deoptimizing code object.
  Handle<Code> code_;

  // Next pointer for linked list.
  DeoptimizingCodeListNode* next_;
};


class SlotRef BASE_EMBEDDED {
 public:
  enum SlotRepresentation {
    UNKNOWN,
    TAGGED,
    INT32,
    UINT32,
    DOUBLE,
    LITERAL
  };

  SlotRef()
      : addr_(NULL), representation_(UNKNOWN) { }

  SlotRef(Address addr, SlotRepresentation representation)
      : addr_(addr), representation_(representation) { }

  explicit SlotRef(Object* literal)
      : literal_(literal), representation_(LITERAL) { }

  Handle<Object> GetValue() {
    switch (representation_) {
      case TAGGED:
        return Handle<Object>(Memory::Object_at(addr_));

      case INT32: {
        int value = Memory::int32_at(addr_);
        if (Smi::IsValid(value)) {
          return Handle<Object>(Smi::FromInt(value));
        } else {
          return Isolate::Current()->factory()->NewNumberFromInt(value);
        }
      }

      case UINT32: {
        uint32_t value = Memory::uint32_at(addr_);
        if (value <= static_cast<uint32_t>(Smi::kMaxValue)) {
          return Handle<Object>(Smi::FromInt(static_cast<int>(value)));
        } else {
          return Isolate::Current()->factory()->NewNumber(
            static_cast<double>(value));
        }
      }

      case DOUBLE: {
        double value = Memory::double_at(addr_);
        return Isolate::Current()->factory()->NewNumber(value);
      }

      case LITERAL:
        return literal_;

      default:
        UNREACHABLE();
        return Handle<Object>::null();
    }
  }

  static Vector<SlotRef> ComputeSlotMappingForArguments(
      JavaScriptFrame* frame,
      int inlined_frame_index,
      int formal_parameter_count);

 private:
  Address addr_;
  Handle<Object> literal_;
  SlotRepresentation representation_;

  static Address SlotAddress(JavaScriptFrame* frame, int slot_index) {
    if (slot_index >= 0) {
      const int offset = JavaScriptFrameConstants::kLocal0Offset;
      return frame->fp() + offset - (slot_index * kPointerSize);
    } else {
      const int offset = JavaScriptFrameConstants::kLastParameterOffset;
      return frame->fp() + offset - ((slot_index + 1) * kPointerSize);
    }
  }

  static SlotRef ComputeSlotForNextArgument(TranslationIterator* iterator,
                                            DeoptimizationInputData* data,
                                            JavaScriptFrame* frame);

  static void ComputeSlotsForArguments(
      Vector<SlotRef>* args_slots,
      TranslationIterator* iterator,
      DeoptimizationInputData* data,
      JavaScriptFrame* frame);
};


#ifdef ENABLE_DEBUGGER_SUPPORT
// Class used to represent an unoptimized frame when the debugger
// needs to inspect a frame that is part of an optimized frame. The
// internally used FrameDescription objects are not GC safe so for use
// by the debugger frame information is copied to an object of this type.
// Represents parameters in unadapted form so their number might mismatch
// formal parameter count.
class DeoptimizedFrameInfo : public Malloced {
 public:
  DeoptimizedFrameInfo(Deoptimizer* deoptimizer,
                       int frame_index,
                       bool has_arguments_adaptor,
                       bool has_construct_stub);
  virtual ~DeoptimizedFrameInfo();

  // GC support.
  void Iterate(ObjectVisitor* v);

  // Return the number of incoming arguments.
  int parameters_count() { return parameters_count_; }

  // Return the height of the expression stack.
  int expression_count() { return expression_count_; }

  // Get the frame function.
  JSFunction* GetFunction() {
    return function_;
  }

  // Check if this frame is preceded by construct stub frame.  The bottom-most
  // inlined frame might still be called by an uninlined construct stub.
  bool HasConstructStub() {
    return has_construct_stub_;
  }

  // Get an incoming argument.
  Object* GetParameter(int index) {
    ASSERT(0 <= index && index < parameters_count());
    return parameters_[index];
  }

  // Get an expression from the expression stack.
  Object* GetExpression(int index) {
    ASSERT(0 <= index && index < expression_count());
    return expression_stack_[index];
  }

  int GetSourcePosition() {
    return source_position_;
  }

 private:
  // Set an incoming argument.
  void SetParameter(int index, Object* obj) {
    ASSERT(0 <= index && index < parameters_count());
    parameters_[index] = obj;
  }

  // Set an expression on the expression stack.
  void SetExpression(int index, Object* obj) {
    ASSERT(0 <= index && index < expression_count());
    expression_stack_[index] = obj;
  }

  JSFunction* function_;
  bool has_construct_stub_;
  int parameters_count_;
  int expression_count_;
  Object** parameters_;
  Object** expression_stack_;
  int source_position_;

  friend class Deoptimizer;
};
#endif

} }  // namespace v8::internal

#endif  // V8_DEOPTIMIZER_H_
