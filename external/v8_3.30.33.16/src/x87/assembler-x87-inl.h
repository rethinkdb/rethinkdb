// Copyright (c) 1994-2006 Sun Microsystems Inc.
// All Rights Reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// - Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// - Redistribution in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// - Neither the name of Sun Microsystems or the names of contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// The original source code covered by the above license above has been
// modified significantly by Google Inc.
// Copyright 2012 the V8 project authors. All rights reserved.

// A light-weight IA32 Assembler.

#ifndef V8_X87_ASSEMBLER_X87_INL_H_
#define V8_X87_ASSEMBLER_X87_INL_H_

#include "src/x87/assembler-x87.h"

#include "src/assembler.h"
#include "src/debug.h"

namespace v8 {
namespace internal {

bool CpuFeatures::SupportsCrankshaft() { return true; }


static const byte kCallOpcode = 0xE8;
static const int kNoCodeAgeSequenceLength = 5;


// The modes possibly affected by apply must be in kApplyMask.
void RelocInfo::apply(intptr_t delta, ICacheFlushMode icache_flush_mode) {
  bool flush_icache = icache_flush_mode != SKIP_ICACHE_FLUSH;
  if (IsRuntimeEntry(rmode_) || IsCodeTarget(rmode_)) {
    int32_t* p = reinterpret_cast<int32_t*>(pc_);
    *p -= delta;  // Relocate entry.
    if (flush_icache) CpuFeatures::FlushICache(p, sizeof(uint32_t));
  } else if (rmode_ == CODE_AGE_SEQUENCE) {
    if (*pc_ == kCallOpcode) {
      int32_t* p = reinterpret_cast<int32_t*>(pc_ + 1);
      *p -= delta;  // Relocate entry.
      if (flush_icache) CpuFeatures::FlushICache(p, sizeof(uint32_t));
    }
  } else if (rmode_ == JS_RETURN && IsPatchedReturnSequence()) {
    // Special handling of js_return when a break point is set (call
    // instruction has been inserted).
    int32_t* p = reinterpret_cast<int32_t*>(pc_ + 1);
    *p -= delta;  // Relocate entry.
    if (flush_icache) CpuFeatures::FlushICache(p, sizeof(uint32_t));
  } else if (rmode_ == DEBUG_BREAK_SLOT && IsPatchedDebugBreakSlotSequence()) {
    // Special handling of a debug break slot when a break point is set (call
    // instruction has been inserted).
    int32_t* p = reinterpret_cast<int32_t*>(pc_ + 1);
    *p -= delta;  // Relocate entry.
    if (flush_icache) CpuFeatures::FlushICache(p, sizeof(uint32_t));
  } else if (IsInternalReference(rmode_)) {
    // absolute code pointer inside code object moves with the code object.
    int32_t* p = reinterpret_cast<int32_t*>(pc_);
    *p += delta;  // Relocate entry.
    if (flush_icache) CpuFeatures::FlushICache(p, sizeof(uint32_t));
  }
}


Address RelocInfo::target_address() {
  DCHECK(IsCodeTarget(rmode_) || IsRuntimeEntry(rmode_));
  return Assembler::target_address_at(pc_, host_);
}


Address RelocInfo::target_address_address() {
  DCHECK(IsCodeTarget(rmode_) || IsRuntimeEntry(rmode_)
                              || rmode_ == EMBEDDED_OBJECT
                              || rmode_ == EXTERNAL_REFERENCE);
  return reinterpret_cast<Address>(pc_);
}


Address RelocInfo::constant_pool_entry_address() {
  UNREACHABLE();
  return NULL;
}


int RelocInfo::target_address_size() {
  return Assembler::kSpecialTargetSize;
}


void RelocInfo::set_target_address(Address target,
                                   WriteBarrierMode write_barrier_mode,
                                   ICacheFlushMode icache_flush_mode) {
  Assembler::set_target_address_at(pc_, host_, target, icache_flush_mode);
  Assembler::set_target_address_at(pc_, host_, target);
  DCHECK(IsCodeTarget(rmode_) || IsRuntimeEntry(rmode_));
  if (write_barrier_mode == UPDATE_WRITE_BARRIER && host() != NULL &&
      IsCodeTarget(rmode_)) {
    Object* target_code = Code::GetCodeFromTargetAddress(target);
    host()->GetHeap()->incremental_marking()->RecordWriteIntoCode(
        host(), this, HeapObject::cast(target_code));
  }
}


Object* RelocInfo::target_object() {
  DCHECK(IsCodeTarget(rmode_) || rmode_ == EMBEDDED_OBJECT);
  return Memory::Object_at(pc_);
}


Handle<Object> RelocInfo::target_object_handle(Assembler* origin) {
  DCHECK(IsCodeTarget(rmode_) || rmode_ == EMBEDDED_OBJECT);
  return Memory::Object_Handle_at(pc_);
}


void RelocInfo::set_target_object(Object* target,
                                  WriteBarrierMode write_barrier_mode,
                                  ICacheFlushMode icache_flush_mode) {
  DCHECK(IsCodeTarget(rmode_) || rmode_ == EMBEDDED_OBJECT);
  Memory::Object_at(pc_) = target;
  if (icache_flush_mode != SKIP_ICACHE_FLUSH) {
    CpuFeatures::FlushICache(pc_, sizeof(Address));
  }
  if (write_barrier_mode == UPDATE_WRITE_BARRIER &&
      host() != NULL &&
      target->IsHeapObject()) {
    host()->GetHeap()->incremental_marking()->RecordWrite(
        host(), &Memory::Object_at(pc_), HeapObject::cast(target));
  }
}


Address RelocInfo::target_reference() {
  DCHECK(rmode_ == RelocInfo::EXTERNAL_REFERENCE);
  return Memory::Address_at(pc_);
}


Address RelocInfo::target_runtime_entry(Assembler* origin) {
  DCHECK(IsRuntimeEntry(rmode_));
  return reinterpret_cast<Address>(*reinterpret_cast<int32_t*>(pc_));
}


void RelocInfo::set_target_runtime_entry(Address target,
                                         WriteBarrierMode write_barrier_mode,
                                         ICacheFlushMode icache_flush_mode) {
  DCHECK(IsRuntimeEntry(rmode_));
  if (target_address() != target) {
    set_target_address(target, write_barrier_mode, icache_flush_mode);
  }
}


Handle<Cell> RelocInfo::target_cell_handle() {
  DCHECK(rmode_ == RelocInfo::CELL);
  Address address = Memory::Address_at(pc_);
  return Handle<Cell>(reinterpret_cast<Cell**>(address));
}


Cell* RelocInfo::target_cell() {
  DCHECK(rmode_ == RelocInfo::CELL);
  return Cell::FromValueAddress(Memory::Address_at(pc_));
}


void RelocInfo::set_target_cell(Cell* cell,
                                WriteBarrierMode write_barrier_mode,
                                ICacheFlushMode icache_flush_mode) {
  DCHECK(rmode_ == RelocInfo::CELL);
  Address address = cell->address() + Cell::kValueOffset;
  Memory::Address_at(pc_) = address;
  if (icache_flush_mode != SKIP_ICACHE_FLUSH) {
    CpuFeatures::FlushICache(pc_, sizeof(Address));
  }
  if (write_barrier_mode == UPDATE_WRITE_BARRIER && host() != NULL) {
    // TODO(1550) We are passing NULL as a slot because cell can never be on
    // evacuation candidate.
    host()->GetHeap()->incremental_marking()->RecordWrite(
        host(), NULL, cell);
  }
}


Handle<Object> RelocInfo::code_age_stub_handle(Assembler* origin) {
  DCHECK(rmode_ == RelocInfo::CODE_AGE_SEQUENCE);
  DCHECK(*pc_ == kCallOpcode);
  return Memory::Object_Handle_at(pc_ + 1);
}


Code* RelocInfo::code_age_stub() {
  DCHECK(rmode_ == RelocInfo::CODE_AGE_SEQUENCE);
  DCHECK(*pc_ == kCallOpcode);
  return Code::GetCodeFromTargetAddress(
      Assembler::target_address_at(pc_ + 1, host_));
}


void RelocInfo::set_code_age_stub(Code* stub,
                                  ICacheFlushMode icache_flush_mode) {
  DCHECK(*pc_ == kCallOpcode);
  DCHECK(rmode_ == RelocInfo::CODE_AGE_SEQUENCE);
  Assembler::set_target_address_at(pc_ + 1, host_, stub->instruction_start(),
                                   icache_flush_mode);
}


Address RelocInfo::call_address() {
  DCHECK((IsJSReturn(rmode()) && IsPatchedReturnSequence()) ||
         (IsDebugBreakSlot(rmode()) && IsPatchedDebugBreakSlotSequence()));
  return Assembler::target_address_at(pc_ + 1, host_);
}


void RelocInfo::set_call_address(Address target) {
  DCHECK((IsJSReturn(rmode()) && IsPatchedReturnSequence()) ||
         (IsDebugBreakSlot(rmode()) && IsPatchedDebugBreakSlotSequence()));
  Assembler::set_target_address_at(pc_ + 1, host_, target);
  if (host() != NULL) {
    Object* target_code = Code::GetCodeFromTargetAddress(target);
    host()->GetHeap()->incremental_marking()->RecordWriteIntoCode(
        host(), this, HeapObject::cast(target_code));
  }
}


Object* RelocInfo::call_object() {
  return *call_object_address();
}


void RelocInfo::set_call_object(Object* target) {
  *call_object_address() = target;
}


Object** RelocInfo::call_object_address() {
  DCHECK((IsJSReturn(rmode()) && IsPatchedReturnSequence()) ||
         (IsDebugBreakSlot(rmode()) && IsPatchedDebugBreakSlotSequence()));
  return reinterpret_cast<Object**>(pc_ + 1);
}


void RelocInfo::WipeOut() {
  if (IsEmbeddedObject(rmode_) || IsExternalReference(rmode_)) {
    Memory::Address_at(pc_) = NULL;
  } else if (IsCodeTarget(rmode_) || IsRuntimeEntry(rmode_)) {
    // Effectively write zero into the relocation.
    Assembler::set_target_address_at(pc_, host_, pc_ + sizeof(int32_t));
  } else {
    UNREACHABLE();
  }
}


bool RelocInfo::IsPatchedReturnSequence() {
  return *pc_ == kCallOpcode;
}


bool RelocInfo::IsPatchedDebugBreakSlotSequence() {
  return !Assembler::IsNop(pc());
}


void RelocInfo::Visit(Isolate* isolate, ObjectVisitor* visitor) {
  RelocInfo::Mode mode = rmode();
  if (mode == RelocInfo::EMBEDDED_OBJECT) {
    visitor->VisitEmbeddedPointer(this);
    CpuFeatures::FlushICache(pc_, sizeof(Address));
  } else if (RelocInfo::IsCodeTarget(mode)) {
    visitor->VisitCodeTarget(this);
  } else if (mode == RelocInfo::CELL) {
    visitor->VisitCell(this);
  } else if (mode == RelocInfo::EXTERNAL_REFERENCE) {
    visitor->VisitExternalReference(this);
    CpuFeatures::FlushICache(pc_, sizeof(Address));
  } else if (RelocInfo::IsCodeAgeSequence(mode)) {
    visitor->VisitCodeAgeSequence(this);
  } else if (((RelocInfo::IsJSReturn(mode) &&
              IsPatchedReturnSequence()) ||
             (RelocInfo::IsDebugBreakSlot(mode) &&
              IsPatchedDebugBreakSlotSequence())) &&
             isolate->debug()->has_break_points()) {
    visitor->VisitDebugTarget(this);
  } else if (IsRuntimeEntry(mode)) {
    visitor->VisitRuntimeEntry(this);
  }
}


template<typename StaticVisitor>
void RelocInfo::Visit(Heap* heap) {
  RelocInfo::Mode mode = rmode();
  if (mode == RelocInfo::EMBEDDED_OBJECT) {
    StaticVisitor::VisitEmbeddedPointer(heap, this);
    CpuFeatures::FlushICache(pc_, sizeof(Address));
  } else if (RelocInfo::IsCodeTarget(mode)) {
    StaticVisitor::VisitCodeTarget(heap, this);
  } else if (mode == RelocInfo::CELL) {
    StaticVisitor::VisitCell(heap, this);
  } else if (mode == RelocInfo::EXTERNAL_REFERENCE) {
    StaticVisitor::VisitExternalReference(this);
    CpuFeatures::FlushICache(pc_, sizeof(Address));
  } else if (RelocInfo::IsCodeAgeSequence(mode)) {
    StaticVisitor::VisitCodeAgeSequence(heap, this);
  } else if (heap->isolate()->debug()->has_break_points() &&
             ((RelocInfo::IsJSReturn(mode) &&
              IsPatchedReturnSequence()) ||
             (RelocInfo::IsDebugBreakSlot(mode) &&
              IsPatchedDebugBreakSlotSequence()))) {
    StaticVisitor::VisitDebugTarget(heap, this);
  } else if (IsRuntimeEntry(mode)) {
    StaticVisitor::VisitRuntimeEntry(this);
  }
}



Immediate::Immediate(int x)  {
  x_ = x;
  rmode_ = RelocInfo::NONE32;
}


Immediate::Immediate(const ExternalReference& ext) {
  x_ = reinterpret_cast<int32_t>(ext.address());
  rmode_ = RelocInfo::EXTERNAL_REFERENCE;
}


Immediate::Immediate(Label* internal_offset) {
  x_ = reinterpret_cast<int32_t>(internal_offset);
  rmode_ = RelocInfo::INTERNAL_REFERENCE;
}


Immediate::Immediate(Handle<Object> handle) {
  AllowDeferredHandleDereference using_raw_address;
  // Verify all Objects referred by code are NOT in new space.
  Object* obj = *handle;
  if (obj->IsHeapObject()) {
    DCHECK(!HeapObject::cast(obj)->GetHeap()->InNewSpace(obj));
    x_ = reinterpret_cast<intptr_t>(handle.location());
    rmode_ = RelocInfo::EMBEDDED_OBJECT;
  } else {
    // no relocation needed
    x_ =  reinterpret_cast<intptr_t>(obj);
    rmode_ = RelocInfo::NONE32;
  }
}


Immediate::Immediate(Smi* value) {
  x_ = reinterpret_cast<intptr_t>(value);
  rmode_ = RelocInfo::NONE32;
}


Immediate::Immediate(Address addr) {
  x_ = reinterpret_cast<int32_t>(addr);
  rmode_ = RelocInfo::NONE32;
}


void Assembler::emit(uint32_t x) {
  *reinterpret_cast<uint32_t*>(pc_) = x;
  pc_ += sizeof(uint32_t);
}


void Assembler::emit(Handle<Object> handle) {
  AllowDeferredHandleDereference heap_object_check;
  // Verify all Objects referred by code are NOT in new space.
  Object* obj = *handle;
  DCHECK(!isolate()->heap()->InNewSpace(obj));
  if (obj->IsHeapObject()) {
    emit(reinterpret_cast<intptr_t>(handle.location()),
         RelocInfo::EMBEDDED_OBJECT);
  } else {
    // no relocation needed
    emit(reinterpret_cast<intptr_t>(obj));
  }
}


void Assembler::emit(uint32_t x, RelocInfo::Mode rmode, TypeFeedbackId id) {
  if (rmode == RelocInfo::CODE_TARGET && !id.IsNone()) {
    RecordRelocInfo(RelocInfo::CODE_TARGET_WITH_ID, id.ToInt());
  } else if (!RelocInfo::IsNone(rmode)
      && rmode != RelocInfo::CODE_AGE_SEQUENCE) {
    RecordRelocInfo(rmode);
  }
  emit(x);
}


void Assembler::emit(Handle<Code> code,
                     RelocInfo::Mode rmode,
                     TypeFeedbackId id) {
  AllowDeferredHandleDereference embedding_raw_address;
  emit(reinterpret_cast<intptr_t>(code.location()), rmode, id);
}


void Assembler::emit(const Immediate& x) {
  if (x.rmode_ == RelocInfo::INTERNAL_REFERENCE) {
    Label* label = reinterpret_cast<Label*>(x.x_);
    emit_code_relative_offset(label);
    return;
  }
  if (!RelocInfo::IsNone(x.rmode_)) RecordRelocInfo(x.rmode_);
  emit(x.x_);
}


void Assembler::emit_code_relative_offset(Label* label) {
  if (label->is_bound()) {
    int32_t pos;
    pos = label->pos() + Code::kHeaderSize - kHeapObjectTag;
    emit(pos);
  } else {
    emit_disp(label, Displacement::CODE_RELATIVE);
  }
}


void Assembler::emit_w(const Immediate& x) {
  DCHECK(RelocInfo::IsNone(x.rmode_));
  uint16_t value = static_cast<uint16_t>(x.x_);
  reinterpret_cast<uint16_t*>(pc_)[0] = value;
  pc_ += sizeof(uint16_t);
}


Address Assembler::target_address_at(Address pc,
                                     ConstantPoolArray* constant_pool) {
  return pc + sizeof(int32_t) + *reinterpret_cast<int32_t*>(pc);
}


void Assembler::set_target_address_at(Address pc,
                                      ConstantPoolArray* constant_pool,
                                      Address target,
                                      ICacheFlushMode icache_flush_mode) {
  int32_t* p = reinterpret_cast<int32_t*>(pc);
  *p = target - (pc + sizeof(int32_t));
  if (icache_flush_mode != SKIP_ICACHE_FLUSH) {
    CpuFeatures::FlushICache(p, sizeof(int32_t));
  }
}


Address Assembler::target_address_from_return_address(Address pc) {
  return pc - kCallTargetAddressOffset;
}


Address Assembler::break_address_from_return_address(Address pc) {
  return pc - Assembler::kPatchDebugBreakSlotReturnOffset;
}


Displacement Assembler::disp_at(Label* L) {
  return Displacement(long_at(L->pos()));
}


void Assembler::disp_at_put(Label* L, Displacement disp) {
  long_at_put(L->pos(), disp.data());
}


void Assembler::emit_disp(Label* L, Displacement::Type type) {
  Displacement disp(L, type);
  L->link_to(pc_offset());
  emit(static_cast<int>(disp.data()));
}


void Assembler::emit_near_disp(Label* L) {
  byte disp = 0x00;
  if (L->is_near_linked()) {
    int offset = L->near_link_pos() - pc_offset();
    DCHECK(is_int8(offset));
    disp = static_cast<byte>(offset & 0xFF);
  }
  L->link_to(pc_offset(), Label::kNear);
  *pc_++ = disp;
}


void Operand::set_modrm(int mod, Register rm) {
  DCHECK((mod & -4) == 0);
  buf_[0] = mod << 6 | rm.code();
  len_ = 1;
}


void Operand::set_sib(ScaleFactor scale, Register index, Register base) {
  DCHECK(len_ == 1);
  DCHECK((scale & -4) == 0);
  // Use SIB with no index register only for base esp.
  DCHECK(!index.is(esp) || base.is(esp));
  buf_[1] = scale << 6 | index.code() << 3 | base.code();
  len_ = 2;
}


void Operand::set_disp8(int8_t disp) {
  DCHECK(len_ == 1 || len_ == 2);
  *reinterpret_cast<int8_t*>(&buf_[len_++]) = disp;
}


void Operand::set_dispr(int32_t disp, RelocInfo::Mode rmode) {
  DCHECK(len_ == 1 || len_ == 2);
  int32_t* p = reinterpret_cast<int32_t*>(&buf_[len_]);
  *p = disp;
  len_ += sizeof(int32_t);
  rmode_ = rmode;
}

Operand::Operand(Register reg) {
  // reg
  set_modrm(3, reg);
}


Operand::Operand(int32_t disp, RelocInfo::Mode rmode) {
  // [disp/r]
  set_modrm(0, ebp);
  set_dispr(disp, rmode);
}


Operand::Operand(Immediate imm) {
  // [disp/r]
  set_modrm(0, ebp);
  set_dispr(imm.x_, imm.rmode_);
}
} }  // namespace v8::internal

#endif  // V8_X87_ASSEMBLER_X87_INL_H_
