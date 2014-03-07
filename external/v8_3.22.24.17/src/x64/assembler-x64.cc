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

#if V8_TARGET_ARCH_X64

#include "macro-assembler.h"
#include "serialize.h"

namespace v8 {
namespace internal {

// -----------------------------------------------------------------------------
// Implementation of CpuFeatures


#ifdef DEBUG
bool CpuFeatures::initialized_ = false;
#endif
uint64_t CpuFeatures::supported_ = CpuFeatures::kDefaultCpuFeatures;
uint64_t CpuFeatures::found_by_runtime_probing_only_ = 0;
uint64_t CpuFeatures::cross_compile_ = 0;

ExternalReference ExternalReference::cpu_features() {
  ASSERT(CpuFeatures::initialized_);
  return ExternalReference(&CpuFeatures::supported_);
}


void CpuFeatures::Probe() {
  ASSERT(supported_ == CpuFeatures::kDefaultCpuFeatures);
#ifdef DEBUG
  initialized_ = true;
#endif
  supported_ = kDefaultCpuFeatures;
  if (Serializer::enabled()) {
    supported_ |= OS::CpuFeaturesImpliedByPlatform();
    return;  // No features if we might serialize.
  }

  uint64_t probed_features = 0;
  CPU cpu;
  if (cpu.has_sse41()) {
    probed_features |= static_cast<uint64_t>(1) << SSE4_1;
  }
  if (cpu.has_sse3()) {
    probed_features |= static_cast<uint64_t>(1) << SSE3;
  }

  // SSE2 must be available on every x64 CPU.
  ASSERT(cpu.has_sse2());
  probed_features |= static_cast<uint64_t>(1) << SSE2;

  // CMOD must be available on every x64 CPU.
  ASSERT(cpu.has_cmov());
  probed_features |= static_cast<uint64_t>(1) << CMOV;

  // SAHF is not generally available in long mode.
  if (cpu.has_sahf()) {
    probed_features |= static_cast<uint64_t>(1) << SAHF;
  }

  uint64_t platform_features = OS::CpuFeaturesImpliedByPlatform();
  supported_ = probed_features | platform_features;
  found_by_runtime_probing_only_
      = probed_features & ~kDefaultCpuFeatures & ~platform_features;
}


// -----------------------------------------------------------------------------
// Implementation of RelocInfo

// Patch the code at the current PC with a call to the target address.
// Additional guard int3 instructions can be added if required.
void RelocInfo::PatchCodeWithCall(Address target, int guard_bytes) {
  int code_size = Assembler::kCallSequenceLength + guard_bytes;

  // Create a code patcher.
  CodePatcher patcher(pc_, code_size);

  // Add a label for checking the size of the code used for returning.
#ifdef DEBUG
  Label check_codesize;
  patcher.masm()->bind(&check_codesize);
#endif

  // Patch the code.
  patcher.masm()->movq(kScratchRegister, target, RelocInfo::NONE64);
  patcher.masm()->call(kScratchRegister);

  // Check that the size of the code generated is as expected.
  ASSERT_EQ(Assembler::kCallSequenceLength,
            patcher.masm()->SizeOfCodeGeneratedSince(&check_codesize));

  // Add the requested number of int3 instructions after the call.
  for (int i = 0; i < guard_bytes; i++) {
    patcher.masm()->int3();
  }
}


void RelocInfo::PatchCode(byte* instructions, int instruction_count) {
  // Patch the code at the current address with the supplied instructions.
  for (int i = 0; i < instruction_count; i++) {
    *(pc_ + i) = *(instructions + i);
  }

  // Indicate that code has changed.
  CPU::FlushICache(pc_, instruction_count);
}


// -----------------------------------------------------------------------------
// Register constants.

const int
    Register::kRegisterCodeByAllocationIndex[kMaxNumAllocatableRegisters] = {
  // rax, rbx, rdx, rcx, rdi, r8, r9, r11, r14, r15
  0, 3, 2, 1, 7, 8, 9, 11, 14, 15
};

const int Register::kAllocationIndexByRegisterCode[kNumRegisters] = {
  0, 3, 2, 1, -1, -1, -1, 4, 5, 6, -1, 7, -1, -1, 8, 9
};


// -----------------------------------------------------------------------------
// Implementation of Operand

Operand::Operand(Register base, int32_t disp) : rex_(0) {
  len_ = 1;
  if (base.is(rsp) || base.is(r12)) {
    // SIB byte is needed to encode (rsp + offset) or (r12 + offset).
    set_sib(times_1, rsp, base);
  }

  if (disp == 0 && !base.is(rbp) && !base.is(r13)) {
    set_modrm(0, base);
  } else if (is_int8(disp)) {
    set_modrm(1, base);
    set_disp8(disp);
  } else {
    set_modrm(2, base);
    set_disp32(disp);
  }
}


Operand::Operand(Register base,
                 Register index,
                 ScaleFactor scale,
                 int32_t disp) : rex_(0) {
  ASSERT(!index.is(rsp));
  len_ = 1;
  set_sib(scale, index, base);
  if (disp == 0 && !base.is(rbp) && !base.is(r13)) {
    // This call to set_modrm doesn't overwrite the REX.B (or REX.X) bits
    // possibly set by set_sib.
    set_modrm(0, rsp);
  } else if (is_int8(disp)) {
    set_modrm(1, rsp);
    set_disp8(disp);
  } else {
    set_modrm(2, rsp);
    set_disp32(disp);
  }
}


Operand::Operand(Register index,
                 ScaleFactor scale,
                 int32_t disp) : rex_(0) {
  ASSERT(!index.is(rsp));
  len_ = 1;
  set_modrm(0, rsp);
  set_sib(scale, index, rbp);
  set_disp32(disp);
}


Operand::Operand(const Operand& operand, int32_t offset) {
  ASSERT(operand.len_ >= 1);
  // Operand encodes REX ModR/M [SIB] [Disp].
  byte modrm = operand.buf_[0];
  ASSERT(modrm < 0xC0);  // Disallow mode 3 (register target).
  bool has_sib = ((modrm & 0x07) == 0x04);
  byte mode = modrm & 0xC0;
  int disp_offset = has_sib ? 2 : 1;
  int base_reg = (has_sib ? operand.buf_[1] : modrm) & 0x07;
  // Mode 0 with rbp/r13 as ModR/M or SIB base register always has a 32-bit
  // displacement.
  bool is_baseless = (mode == 0) && (base_reg == 0x05);  // No base or RIP base.
  int32_t disp_value = 0;
  if (mode == 0x80 || is_baseless) {
    // Mode 2 or mode 0 with rbp/r13 as base: Word displacement.
    disp_value = *BitCast<const int32_t*>(&operand.buf_[disp_offset]);
  } else if (mode == 0x40) {
    // Mode 1: Byte displacement.
    disp_value = static_cast<signed char>(operand.buf_[disp_offset]);
  }

  // Write new operand with same registers, but with modified displacement.
  ASSERT(offset >= 0 ? disp_value + offset > disp_value
                     : disp_value + offset < disp_value);  // No overflow.
  disp_value += offset;
  rex_ = operand.rex_;
  if (!is_int8(disp_value) || is_baseless) {
    // Need 32 bits of displacement, mode 2 or mode 1 with register rbp/r13.
    buf_[0] = (modrm & 0x3f) | (is_baseless ? 0x00 : 0x80);
    len_ = disp_offset + 4;
    Memory::int32_at(&buf_[disp_offset]) = disp_value;
  } else if (disp_value != 0 || (base_reg == 0x05)) {
    // Need 8 bits of displacement.
    buf_[0] = (modrm & 0x3f) | 0x40;  // Mode 1.
    len_ = disp_offset + 1;
    buf_[disp_offset] = static_cast<byte>(disp_value);
  } else {
    // Need no displacement.
    buf_[0] = (modrm & 0x3f);  // Mode 0.
    len_ = disp_offset;
  }
  if (has_sib) {
    buf_[1] = operand.buf_[1];
  }
}


bool Operand::AddressUsesRegister(Register reg) const {
  int code = reg.code();
  ASSERT((buf_[0] & 0xC0) != 0xC0);  // Always a memory operand.
  // Start with only low three bits of base register. Initial decoding doesn't
  // distinguish on the REX.B bit.
  int base_code = buf_[0] & 0x07;
  if (base_code == rsp.code()) {
    // SIB byte present in buf_[1].
    // Check the index register from the SIB byte + REX.X prefix.
    int index_code = ((buf_[1] >> 3) & 0x07) | ((rex_ & 0x02) << 2);
    // Index code (including REX.X) of 0x04 (rsp) means no index register.
    if (index_code != rsp.code() && index_code == code) return true;
    // Add REX.B to get the full base register code.
    base_code = (buf_[1] & 0x07) | ((rex_ & 0x01) << 3);
    // A base register of 0x05 (rbp) with mod = 0 means no base register.
    if (base_code == rbp.code() && ((buf_[0] & 0xC0) == 0)) return false;
    return code == base_code;
  } else {
    // A base register with low bits of 0x05 (rbp or r13) and mod = 0 means
    // no base register.
    if (base_code == rbp.code() && ((buf_[0] & 0xC0) == 0)) return false;
    base_code |= ((rex_ & 0x01) << 3);
    return code == base_code;
  }
}


// -----------------------------------------------------------------------------
// Implementation of Assembler.

#ifdef GENERATED_CODE_COVERAGE
static void InitCoverageLog();
#endif

Assembler::Assembler(Isolate* isolate, void* buffer, int buffer_size)
    : AssemblerBase(isolate, buffer, buffer_size),
      code_targets_(100),
      positions_recorder_(this) {
  // Clear the buffer in debug mode unless it was provided by the
  // caller in which case we can't be sure it's okay to overwrite
  // existing code in it.
#ifdef DEBUG
  if (own_buffer_) {
    memset(buffer_, 0xCC, buffer_size_);  // int3
  }
#endif

  reloc_info_writer.Reposition(buffer_ + buffer_size_, pc_);


#ifdef GENERATED_CODE_COVERAGE
  InitCoverageLog();
#endif
}


void Assembler::GetCode(CodeDesc* desc) {
  // Finalize code (at this point overflow() may be true, but the gap ensures
  // that we are still not overlapping instructions and relocation info).
  ASSERT(pc_ <= reloc_info_writer.pos());  // No overlap.
  // Set up code descriptor.
  desc->buffer = buffer_;
  desc->buffer_size = buffer_size_;
  desc->instr_size = pc_offset();
  ASSERT(desc->instr_size > 0);  // Zero-size code objects upset the system.
  desc->reloc_size =
      static_cast<int>((buffer_ + buffer_size_) - reloc_info_writer.pos());
  desc->origin = this;
}


void Assembler::Align(int m) {
  ASSERT(IsPowerOf2(m));
  int delta = (m - (pc_offset() & (m - 1))) & (m - 1);
  Nop(delta);
}


void Assembler::CodeTargetAlign() {
  Align(16);  // Preferred alignment of jump targets on x64.
}


bool Assembler::IsNop(Address addr) {
  Address a = addr;
  while (*a == 0x66) a++;
  if (*a == 0x90) return true;
  if (a[0] == 0xf && a[1] == 0x1f) return true;
  return false;
}


void Assembler::bind_to(Label* L, int pos) {
  ASSERT(!L->is_bound());  // Label may only be bound once.
  ASSERT(0 <= pos && pos <= pc_offset());  // Position must be valid.
  if (L->is_linked()) {
    int current = L->pos();
    int next = long_at(current);
    while (next != current) {
      // Relative address, relative to point after address.
      int imm32 = pos - (current + sizeof(int32_t));
      long_at_put(current, imm32);
      current = next;
      next = long_at(next);
    }
    // Fix up last fixup on linked list.
    int last_imm32 = pos - (current + sizeof(int32_t));
    long_at_put(current, last_imm32);
  }
  while (L->is_near_linked()) {
    int fixup_pos = L->near_link_pos();
    int offset_to_next =
        static_cast<int>(*reinterpret_cast<int8_t*>(addr_at(fixup_pos)));
    ASSERT(offset_to_next <= 0);
    int disp = pos - (fixup_pos + sizeof(int8_t));
    CHECK(is_int8(disp));
    set_byte_at(fixup_pos, disp);
    if (offset_to_next < 0) {
      L->link_to(fixup_pos + offset_to_next, Label::kNear);
    } else {
      L->UnuseNear();
    }
  }
  L->bind_to(pos);
}


void Assembler::bind(Label* L) {
  bind_to(L, pc_offset());
}


void Assembler::GrowBuffer() {
  ASSERT(buffer_overflow());
  if (!own_buffer_) FATAL("external code buffer is too small");

  // Compute new buffer size.
  CodeDesc desc;  // the new buffer
  if (buffer_size_ < 4*KB) {
    desc.buffer_size = 4*KB;
  } else {
    desc.buffer_size = 2*buffer_size_;
  }
  // Some internal data structures overflow for very large buffers,
  // they must ensure that kMaximalBufferSize is not too large.
  if ((desc.buffer_size > kMaximalBufferSize) ||
      (desc.buffer_size > isolate()->heap()->MaxOldGenerationSize())) {
    V8::FatalProcessOutOfMemory("Assembler::GrowBuffer");
  }

  // Set up new buffer.
  desc.buffer = NewArray<byte>(desc.buffer_size);
  desc.instr_size = pc_offset();
  desc.reloc_size =
      static_cast<int>((buffer_ + buffer_size_) - (reloc_info_writer.pos()));

  // Clear the buffer in debug mode. Use 'int3' instructions to make
  // sure to get into problems if we ever run uninitialized code.
#ifdef DEBUG
  memset(desc.buffer, 0xCC, desc.buffer_size);
#endif

  // Copy the data.
  intptr_t pc_delta = desc.buffer - buffer_;
  intptr_t rc_delta = (desc.buffer + desc.buffer_size) -
      (buffer_ + buffer_size_);
  OS::MemMove(desc.buffer, buffer_, desc.instr_size);
  OS::MemMove(rc_delta + reloc_info_writer.pos(),
              reloc_info_writer.pos(), desc.reloc_size);

  // Switch buffers.
  if (isolate() != NULL &&
      isolate()->assembler_spare_buffer() == NULL &&
      buffer_size_ == kMinimalBufferSize) {
    isolate()->set_assembler_spare_buffer(buffer_);
  } else {
    DeleteArray(buffer_);
  }
  buffer_ = desc.buffer;
  buffer_size_ = desc.buffer_size;
  pc_ += pc_delta;
  reloc_info_writer.Reposition(reloc_info_writer.pos() + rc_delta,
                               reloc_info_writer.last_pc() + pc_delta);

  // Relocate runtime entries.
  for (RelocIterator it(desc); !it.done(); it.next()) {
    RelocInfo::Mode rmode = it.rinfo()->rmode();
    if (rmode == RelocInfo::INTERNAL_REFERENCE) {
      intptr_t* p = reinterpret_cast<intptr_t*>(it.rinfo()->pc());
      if (*p != 0) {  // 0 means uninitialized.
        *p += pc_delta;
      }
    }
  }

  ASSERT(!buffer_overflow());
}


void Assembler::emit_operand(int code, const Operand& adr) {
  ASSERT(is_uint3(code));
  const unsigned length = adr.len_;
  ASSERT(length > 0);

  // Emit updated ModR/M byte containing the given register.
  ASSERT((adr.buf_[0] & 0x38) == 0);
  pc_[0] = adr.buf_[0] | code << 3;

  // Emit the rest of the encoded operand.
  for (unsigned i = 1; i < length; i++) pc_[i] = adr.buf_[i];
  pc_ += length;
}


// Assembler Instruction implementations.

void Assembler::arithmetic_op(byte opcode, Register reg, const Operand& op) {
  EnsureSpace ensure_space(this);
  emit_rex_64(reg, op);
  emit(opcode);
  emit_operand(reg, op);
}


void Assembler::arithmetic_op(byte opcode, Register reg, Register rm_reg) {
  EnsureSpace ensure_space(this);
  ASSERT((opcode & 0xC6) == 2);
  if (rm_reg.low_bits() == 4)  {  // Forces SIB byte.
    // Swap reg and rm_reg and change opcode operand order.
    emit_rex_64(rm_reg, reg);
    emit(opcode ^ 0x02);
    emit_modrm(rm_reg, reg);
  } else {
    emit_rex_64(reg, rm_reg);
    emit(opcode);
    emit_modrm(reg, rm_reg);
  }
}


void Assembler::arithmetic_op_16(byte opcode, Register reg, Register rm_reg) {
  EnsureSpace ensure_space(this);
  ASSERT((opcode & 0xC6) == 2);
  if (rm_reg.low_bits() == 4) {  // Forces SIB byte.
    // Swap reg and rm_reg and change opcode operand order.
    emit(0x66);
    emit_optional_rex_32(rm_reg, reg);
    emit(opcode ^ 0x02);
    emit_modrm(rm_reg, reg);
  } else {
    emit(0x66);
    emit_optional_rex_32(reg, rm_reg);
    emit(opcode);
    emit_modrm(reg, rm_reg);
  }
}


void Assembler::arithmetic_op_16(byte opcode,
                                 Register reg,
                                 const Operand& rm_reg) {
  EnsureSpace ensure_space(this);
  emit(0x66);
  emit_optional_rex_32(reg, rm_reg);
  emit(opcode);
  emit_operand(reg, rm_reg);
}


void Assembler::arithmetic_op_32(byte opcode, Register reg, Register rm_reg) {
  EnsureSpace ensure_space(this);
  ASSERT((opcode & 0xC6) == 2);
  if (rm_reg.low_bits() == 4) {  // Forces SIB byte.
    // Swap reg and rm_reg and change opcode operand order.
    emit_optional_rex_32(rm_reg, reg);
    emit(opcode ^ 0x02);  // E.g. 0x03 -> 0x01 for ADD.
    emit_modrm(rm_reg, reg);
  } else {
    emit_optional_rex_32(reg, rm_reg);
    emit(opcode);
    emit_modrm(reg, rm_reg);
  }
}


void Assembler::arithmetic_op_32(byte opcode,
                                 Register reg,
                                 const Operand& rm_reg) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(reg, rm_reg);
  emit(opcode);
  emit_operand(reg, rm_reg);
}


void Assembler::immediate_arithmetic_op(byte subcode,
                                        Register dst,
                                        Immediate src) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst);
  if (is_int8(src.value_)) {
    emit(0x83);
    emit_modrm(subcode, dst);
    emit(src.value_);
  } else if (dst.is(rax)) {
    emit(0x05 | (subcode << 3));
    emitl(src.value_);
  } else {
    emit(0x81);
    emit_modrm(subcode, dst);
    emitl(src.value_);
  }
}

void Assembler::immediate_arithmetic_op(byte subcode,
                                        const Operand& dst,
                                        Immediate src) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst);
  if (is_int8(src.value_)) {
    emit(0x83);
    emit_operand(subcode, dst);
    emit(src.value_);
  } else {
    emit(0x81);
    emit_operand(subcode, dst);
    emitl(src.value_);
  }
}


void Assembler::immediate_arithmetic_op_16(byte subcode,
                                           Register dst,
                                           Immediate src) {
  EnsureSpace ensure_space(this);
  emit(0x66);  // Operand size override prefix.
  emit_optional_rex_32(dst);
  if (is_int8(src.value_)) {
    emit(0x83);
    emit_modrm(subcode, dst);
    emit(src.value_);
  } else if (dst.is(rax)) {
    emit(0x05 | (subcode << 3));
    emitw(src.value_);
  } else {
    emit(0x81);
    emit_modrm(subcode, dst);
    emitw(src.value_);
  }
}


void Assembler::immediate_arithmetic_op_16(byte subcode,
                                           const Operand& dst,
                                           Immediate src) {
  EnsureSpace ensure_space(this);
  emit(0x66);  // Operand size override prefix.
  emit_optional_rex_32(dst);
  if (is_int8(src.value_)) {
    emit(0x83);
    emit_operand(subcode, dst);
    emit(src.value_);
  } else {
    emit(0x81);
    emit_operand(subcode, dst);
    emitw(src.value_);
  }
}


void Assembler::immediate_arithmetic_op_32(byte subcode,
                                           Register dst,
                                           Immediate src) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst);
  if (is_int8(src.value_)) {
    emit(0x83);
    emit_modrm(subcode, dst);
    emit(src.value_);
  } else if (dst.is(rax)) {
    emit(0x05 | (subcode << 3));
    emitl(src.value_);
  } else {
    emit(0x81);
    emit_modrm(subcode, dst);
    emitl(src.value_);
  }
}


void Assembler::immediate_arithmetic_op_32(byte subcode,
                                           const Operand& dst,
                                           Immediate src) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst);
  if (is_int8(src.value_)) {
    emit(0x83);
    emit_operand(subcode, dst);
    emit(src.value_);
  } else {
    emit(0x81);
    emit_operand(subcode, dst);
    emitl(src.value_);
  }
}


void Assembler::immediate_arithmetic_op_8(byte subcode,
                                          const Operand& dst,
                                          Immediate src) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst);
  ASSERT(is_int8(src.value_) || is_uint8(src.value_));
  emit(0x80);
  emit_operand(subcode, dst);
  emit(src.value_);
}


void Assembler::immediate_arithmetic_op_8(byte subcode,
                                          Register dst,
                                          Immediate src) {
  EnsureSpace ensure_space(this);
  if (!dst.is_byte_register()) {
    // Use 64-bit mode byte registers.
    emit_rex_64(dst);
  }
  ASSERT(is_int8(src.value_) || is_uint8(src.value_));
  emit(0x80);
  emit_modrm(subcode, dst);
  emit(src.value_);
}


void Assembler::shift(Register dst, Immediate shift_amount, int subcode) {
  EnsureSpace ensure_space(this);
  ASSERT(is_uint6(shift_amount.value_));  // illegal shift count
  if (shift_amount.value_ == 1) {
    emit_rex_64(dst);
    emit(0xD1);
    emit_modrm(subcode, dst);
  } else {
    emit_rex_64(dst);
    emit(0xC1);
    emit_modrm(subcode, dst);
    emit(shift_amount.value_);
  }
}


void Assembler::shift(Register dst, int subcode) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst);
  emit(0xD3);
  emit_modrm(subcode, dst);
}


void Assembler::shift_32(Register dst, int subcode) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst);
  emit(0xD3);
  emit_modrm(subcode, dst);
}


void Assembler::shift_32(Register dst, Immediate shift_amount, int subcode) {
  EnsureSpace ensure_space(this);
  ASSERT(is_uint5(shift_amount.value_));  // illegal shift count
  if (shift_amount.value_ == 1) {
    emit_optional_rex_32(dst);
    emit(0xD1);
    emit_modrm(subcode, dst);
  } else {
    emit_optional_rex_32(dst);
    emit(0xC1);
    emit_modrm(subcode, dst);
    emit(shift_amount.value_);
  }
}


void Assembler::bt(const Operand& dst, Register src) {
  EnsureSpace ensure_space(this);
  emit_rex_64(src, dst);
  emit(0x0F);
  emit(0xA3);
  emit_operand(src, dst);
}


void Assembler::bts(const Operand& dst, Register src) {
  EnsureSpace ensure_space(this);
  emit_rex_64(src, dst);
  emit(0x0F);
  emit(0xAB);
  emit_operand(src, dst);
}


void Assembler::call(Label* L) {
  positions_recorder()->WriteRecordedPositions();
  EnsureSpace ensure_space(this);
  // 1110 1000 #32-bit disp.
  emit(0xE8);
  if (L->is_bound()) {
    int offset = L->pos() - pc_offset() - sizeof(int32_t);
    ASSERT(offset <= 0);
    emitl(offset);
  } else if (L->is_linked()) {
    emitl(L->pos());
    L->link_to(pc_offset() - sizeof(int32_t));
  } else {
    ASSERT(L->is_unused());
    int32_t current = pc_offset();
    emitl(current);
    L->link_to(current);
  }
}


void Assembler::call(Address entry, RelocInfo::Mode rmode) {
  ASSERT(RelocInfo::IsRuntimeEntry(rmode));
  positions_recorder()->WriteRecordedPositions();
  EnsureSpace ensure_space(this);
  // 1110 1000 #32-bit disp.
  emit(0xE8);
  emit_runtime_entry(entry, rmode);
}


void Assembler::call(Handle<Code> target,
                     RelocInfo::Mode rmode,
                     TypeFeedbackId ast_id) {
  positions_recorder()->WriteRecordedPositions();
  EnsureSpace ensure_space(this);
  // 1110 1000 #32-bit disp.
  emit(0xE8);
  emit_code_target(target, rmode, ast_id);
}


void Assembler::call(Register adr) {
  positions_recorder()->WriteRecordedPositions();
  EnsureSpace ensure_space(this);
  // Opcode: FF /2 r64.
  emit_optional_rex_32(adr);
  emit(0xFF);
  emit_modrm(0x2, adr);
}


void Assembler::call(const Operand& op) {
  positions_recorder()->WriteRecordedPositions();
  EnsureSpace ensure_space(this);
  // Opcode: FF /2 m64.
  emit_optional_rex_32(op);
  emit(0xFF);
  emit_operand(0x2, op);
}


// Calls directly to the given address using a relative offset.
// Should only ever be used in Code objects for calls within the
// same Code object. Should not be used when generating new code (use labels),
// but only when patching existing code.
void Assembler::call(Address target) {
  positions_recorder()->WriteRecordedPositions();
  EnsureSpace ensure_space(this);
  // 1110 1000 #32-bit disp.
  emit(0xE8);
  Address source = pc_ + 4;
  intptr_t displacement = target - source;
  ASSERT(is_int32(displacement));
  emitl(static_cast<int32_t>(displacement));
}


void Assembler::clc() {
  EnsureSpace ensure_space(this);
  emit(0xF8);
}


void Assembler::cld() {
  EnsureSpace ensure_space(this);
  emit(0xFC);
}


void Assembler::cdq() {
  EnsureSpace ensure_space(this);
  emit(0x99);
}


void Assembler::cmovq(Condition cc, Register dst, Register src) {
  if (cc == always) {
    movq(dst, src);
  } else if (cc == never) {
    return;
  }
  // No need to check CpuInfo for CMOV support, it's a required part of the
  // 64-bit architecture.
  ASSERT(cc >= 0);  // Use mov for unconditional moves.
  EnsureSpace ensure_space(this);
  // Opcode: REX.W 0f 40 + cc /r.
  emit_rex_64(dst, src);
  emit(0x0f);
  emit(0x40 + cc);
  emit_modrm(dst, src);
}


void Assembler::cmovq(Condition cc, Register dst, const Operand& src) {
  if (cc == always) {
    movq(dst, src);
  } else if (cc == never) {
    return;
  }
  ASSERT(cc >= 0);
  EnsureSpace ensure_space(this);
  // Opcode: REX.W 0f 40 + cc /r.
  emit_rex_64(dst, src);
  emit(0x0f);
  emit(0x40 + cc);
  emit_operand(dst, src);
}


void Assembler::cmovl(Condition cc, Register dst, Register src) {
  if (cc == always) {
    movl(dst, src);
  } else if (cc == never) {
    return;
  }
  ASSERT(cc >= 0);
  EnsureSpace ensure_space(this);
  // Opcode: 0f 40 + cc /r.
  emit_optional_rex_32(dst, src);
  emit(0x0f);
  emit(0x40 + cc);
  emit_modrm(dst, src);
}


void Assembler::cmovl(Condition cc, Register dst, const Operand& src) {
  if (cc == always) {
    movl(dst, src);
  } else if (cc == never) {
    return;
  }
  ASSERT(cc >= 0);
  EnsureSpace ensure_space(this);
  // Opcode: 0f 40 + cc /r.
  emit_optional_rex_32(dst, src);
  emit(0x0f);
  emit(0x40 + cc);
  emit_operand(dst, src);
}


void Assembler::cmpb_al(Immediate imm8) {
  ASSERT(is_int8(imm8.value_) || is_uint8(imm8.value_));
  EnsureSpace ensure_space(this);
  emit(0x3c);
  emit(imm8.value_);
}


void Assembler::cpuid() {
  EnsureSpace ensure_space(this);
  emit(0x0F);
  emit(0xA2);
}


void Assembler::cqo() {
  EnsureSpace ensure_space(this);
  emit_rex_64();
  emit(0x99);
}


void Assembler::decq(Register dst) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst);
  emit(0xFF);
  emit_modrm(0x1, dst);
}


void Assembler::decq(const Operand& dst) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst);
  emit(0xFF);
  emit_operand(1, dst);
}


void Assembler::decl(Register dst) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst);
  emit(0xFF);
  emit_modrm(0x1, dst);
}


void Assembler::decl(const Operand& dst) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst);
  emit(0xFF);
  emit_operand(1, dst);
}


void Assembler::decb(Register dst) {
  EnsureSpace ensure_space(this);
  if (!dst.is_byte_register()) {
    // Register is not one of al, bl, cl, dl.  Its encoding needs REX.
    emit_rex_32(dst);
  }
  emit(0xFE);
  emit_modrm(0x1, dst);
}


void Assembler::decb(const Operand& dst) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst);
  emit(0xFE);
  emit_operand(1, dst);
}


void Assembler::enter(Immediate size) {
  EnsureSpace ensure_space(this);
  emit(0xC8);
  emitw(size.value_);  // 16 bit operand, always.
  emit(0);
}


void Assembler::hlt() {
  EnsureSpace ensure_space(this);
  emit(0xF4);
}


void Assembler::idivq(Register src) {
  EnsureSpace ensure_space(this);
  emit_rex_64(src);
  emit(0xF7);
  emit_modrm(0x7, src);
}


void Assembler::idivl(Register src) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(src);
  emit(0xF7);
  emit_modrm(0x7, src);
}


void Assembler::imul(Register src) {
  EnsureSpace ensure_space(this);
  emit_rex_64(src);
  emit(0xF7);
  emit_modrm(0x5, src);
}


void Assembler::imul(Register dst, Register src) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst, src);
  emit(0x0F);
  emit(0xAF);
  emit_modrm(dst, src);
}


void Assembler::imul(Register dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst, src);
  emit(0x0F);
  emit(0xAF);
  emit_operand(dst, src);
}


void Assembler::imul(Register dst, Register src, Immediate imm) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst, src);
  if (is_int8(imm.value_)) {
    emit(0x6B);
    emit_modrm(dst, src);
    emit(imm.value_);
  } else {
    emit(0x69);
    emit_modrm(dst, src);
    emitl(imm.value_);
  }
}


void Assembler::imull(Register dst, Register src) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0xAF);
  emit_modrm(dst, src);
}


void Assembler::imull(Register dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0xAF);
  emit_operand(dst, src);
}


void Assembler::imull(Register dst, Register src, Immediate imm) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst, src);
  if (is_int8(imm.value_)) {
    emit(0x6B);
    emit_modrm(dst, src);
    emit(imm.value_);
  } else {
    emit(0x69);
    emit_modrm(dst, src);
    emitl(imm.value_);
  }
}


void Assembler::incq(Register dst) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst);
  emit(0xFF);
  emit_modrm(0x0, dst);
}


void Assembler::incq(const Operand& dst) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst);
  emit(0xFF);
  emit_operand(0, dst);
}


void Assembler::incl(const Operand& dst) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst);
  emit(0xFF);
  emit_operand(0, dst);
}


void Assembler::incl(Register dst) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst);
  emit(0xFF);
  emit_modrm(0, dst);
}


void Assembler::int3() {
  EnsureSpace ensure_space(this);
  emit(0xCC);
}


void Assembler::j(Condition cc, Label* L, Label::Distance distance) {
  if (cc == always) {
    jmp(L);
    return;
  } else if (cc == never) {
    return;
  }
  EnsureSpace ensure_space(this);
  ASSERT(is_uint4(cc));
  if (L->is_bound()) {
    const int short_size = 2;
    const int long_size  = 6;
    int offs = L->pos() - pc_offset();
    ASSERT(offs <= 0);
    // Determine whether we can use 1-byte offsets for backwards branches,
    // which have a max range of 128 bytes.

    // We also need to check predictable_code_size() flag here, because on x64,
    // when the full code generator recompiles code for debugging, some places
    // need to be padded out to a certain size. The debugger is keeping track of
    // how often it did this so that it can adjust return addresses on the
    // stack, but if the size of jump instructions can also change, that's not
    // enough and the calculated offsets would be incorrect.
    if (is_int8(offs - short_size) && !predictable_code_size()) {
      // 0111 tttn #8-bit disp.
      emit(0x70 | cc);
      emit((offs - short_size) & 0xFF);
    } else {
      // 0000 1111 1000 tttn #32-bit disp.
      emit(0x0F);
      emit(0x80 | cc);
      emitl(offs - long_size);
    }
  } else if (distance == Label::kNear) {
    // 0111 tttn #8-bit disp
    emit(0x70 | cc);
    byte disp = 0x00;
    if (L->is_near_linked()) {
      int offset = L->near_link_pos() - pc_offset();
      ASSERT(is_int8(offset));
      disp = static_cast<byte>(offset & 0xFF);
    }
    L->link_to(pc_offset(), Label::kNear);
    emit(disp);
  } else if (L->is_linked()) {
    // 0000 1111 1000 tttn #32-bit disp.
    emit(0x0F);
    emit(0x80 | cc);
    emitl(L->pos());
    L->link_to(pc_offset() - sizeof(int32_t));
  } else {
    ASSERT(L->is_unused());
    emit(0x0F);
    emit(0x80 | cc);
    int32_t current = pc_offset();
    emitl(current);
    L->link_to(current);
  }
}


void Assembler::j(Condition cc, Address entry, RelocInfo::Mode rmode) {
  ASSERT(RelocInfo::IsRuntimeEntry(rmode));
  EnsureSpace ensure_space(this);
  ASSERT(is_uint4(cc));
  emit(0x0F);
  emit(0x80 | cc);
  emit_runtime_entry(entry, rmode);
}


void Assembler::j(Condition cc,
                  Handle<Code> target,
                  RelocInfo::Mode rmode) {
  EnsureSpace ensure_space(this);
  ASSERT(is_uint4(cc));
  // 0000 1111 1000 tttn #32-bit disp.
  emit(0x0F);
  emit(0x80 | cc);
  emit_code_target(target, rmode);
}


void Assembler::jmp(Label* L, Label::Distance distance) {
  EnsureSpace ensure_space(this);
  const int short_size = sizeof(int8_t);
  const int long_size = sizeof(int32_t);
  if (L->is_bound()) {
    int offs = L->pos() - pc_offset() - 1;
    ASSERT(offs <= 0);
    if (is_int8(offs - short_size) && !predictable_code_size()) {
      // 1110 1011 #8-bit disp.
      emit(0xEB);
      emit((offs - short_size) & 0xFF);
    } else {
      // 1110 1001 #32-bit disp.
      emit(0xE9);
      emitl(offs - long_size);
    }
  } else if (distance == Label::kNear) {
    emit(0xEB);
    byte disp = 0x00;
    if (L->is_near_linked()) {
      int offset = L->near_link_pos() - pc_offset();
      ASSERT(is_int8(offset));
      disp = static_cast<byte>(offset & 0xFF);
    }
    L->link_to(pc_offset(), Label::kNear);
    emit(disp);
  } else if (L->is_linked()) {
    // 1110 1001 #32-bit disp.
    emit(0xE9);
    emitl(L->pos());
    L->link_to(pc_offset() - long_size);
  } else {
    // 1110 1001 #32-bit disp.
    ASSERT(L->is_unused());
    emit(0xE9);
    int32_t current = pc_offset();
    emitl(current);
    L->link_to(current);
  }
}


void Assembler::jmp(Handle<Code> target, RelocInfo::Mode rmode) {
  EnsureSpace ensure_space(this);
  // 1110 1001 #32-bit disp.
  emit(0xE9);
  emit_code_target(target, rmode);
}


void Assembler::jmp(Address entry, RelocInfo::Mode rmode) {
  ASSERT(RelocInfo::IsRuntimeEntry(rmode));
  EnsureSpace ensure_space(this);
  ASSERT(RelocInfo::IsRuntimeEntry(rmode));
  emit(0xE9);
  emit_runtime_entry(entry, rmode);
}


void Assembler::jmp(Register target) {
  EnsureSpace ensure_space(this);
  // Opcode FF/4 r64.
  emit_optional_rex_32(target);
  emit(0xFF);
  emit_modrm(0x4, target);
}


void Assembler::jmp(const Operand& src) {
  EnsureSpace ensure_space(this);
  // Opcode FF/4 m64.
  emit_optional_rex_32(src);
  emit(0xFF);
  emit_operand(0x4, src);
}


void Assembler::lea(Register dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst, src);
  emit(0x8D);
  emit_operand(dst, src);
}


void Assembler::leal(Register dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst, src);
  emit(0x8D);
  emit_operand(dst, src);
}


void Assembler::load_rax(void* value, RelocInfo::Mode mode) {
  EnsureSpace ensure_space(this);
  emit(0x48);  // REX.W
  emit(0xA1);
  emitp(value, mode);
}


void Assembler::load_rax(ExternalReference ref) {
  load_rax(ref.address(), RelocInfo::EXTERNAL_REFERENCE);
}


void Assembler::leave() {
  EnsureSpace ensure_space(this);
  emit(0xC9);
}


void Assembler::movb(Register dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  if (!dst.is_byte_register()) {
    // Register is not one of al, bl, cl, dl.  Its encoding needs REX.
    emit_rex_32(dst, src);
  } else {
    emit_optional_rex_32(dst, src);
  }
  emit(0x8A);
  emit_operand(dst, src);
}


void Assembler::movb(Register dst, Immediate imm) {
  EnsureSpace ensure_space(this);
  if (!dst.is_byte_register()) {
    emit_rex_32(dst);
  }
  emit(0xB0 + dst.low_bits());
  emit(imm.value_);
}


void Assembler::movb(const Operand& dst, Register src) {
  EnsureSpace ensure_space(this);
  if (!src.is_byte_register()) {
    emit_rex_32(src, dst);
  } else {
    emit_optional_rex_32(src, dst);
  }
  emit(0x88);
  emit_operand(src, dst);
}


void Assembler::movw(const Operand& dst, Register src) {
  EnsureSpace ensure_space(this);
  emit(0x66);
  emit_optional_rex_32(src, dst);
  emit(0x89);
  emit_operand(src, dst);
}


void Assembler::movl(Register dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst, src);
  emit(0x8B);
  emit_operand(dst, src);
}


void Assembler::movl(Register dst, Register src) {
  EnsureSpace ensure_space(this);
  if (src.low_bits() == 4) {
    emit_optional_rex_32(src, dst);
    emit(0x89);
    emit_modrm(src, dst);
  } else {
    emit_optional_rex_32(dst, src);
    emit(0x8B);
    emit_modrm(dst, src);
  }
}


void Assembler::movl(const Operand& dst, Register src) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(src, dst);
  emit(0x89);
  emit_operand(src, dst);
}


void Assembler::movl(const Operand& dst, Immediate value) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst);
  emit(0xC7);
  emit_operand(0x0, dst);
  emit(value);
}


void Assembler::movl(Register dst, Immediate value) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst);
  emit(0xB8 + dst.low_bits());
  emit(value);
}


void Assembler::movq(Register dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst, src);
  emit(0x8B);
  emit_operand(dst, src);
}


void Assembler::movq(Register dst, Register src) {
  EnsureSpace ensure_space(this);
  if (src.low_bits() == 4) {
    emit_rex_64(src, dst);
    emit(0x89);
    emit_modrm(src, dst);
  } else {
    emit_rex_64(dst, src);
    emit(0x8B);
    emit_modrm(dst, src);
  }
}


void Assembler::movq(Register dst, Immediate value) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst);
  emit(0xC7);
  emit_modrm(0x0, dst);
  emit(value);  // Only 32-bit immediates are possible, not 8-bit immediates.
}


void Assembler::movq(const Operand& dst, Register src) {
  EnsureSpace ensure_space(this);
  emit_rex_64(src, dst);
  emit(0x89);
  emit_operand(src, dst);
}


void Assembler::movq(Register dst, void* value, RelocInfo::Mode rmode) {
  // This method must not be used with heap object references. The stored
  // address is not GC safe. Use the handle version instead.
  ASSERT(rmode > RelocInfo::LAST_GCED_ENUM);
  EnsureSpace ensure_space(this);
  emit_rex_64(dst);
  emit(0xB8 | dst.low_bits());
  emitp(value, rmode);
}


void Assembler::movq(Register dst, int64_t value, RelocInfo::Mode rmode) {
  // Non-relocatable values might not need a 64-bit representation.
  ASSERT(RelocInfo::IsNone(rmode));
  if (is_uint32(value)) {
    movl(dst, Immediate(static_cast<int32_t>(value)));
  } else if (is_int32(value)) {
    movq(dst, Immediate(static_cast<int32_t>(value)));
  } else {
    // Value cannot be represented by 32 bits, so do a full 64 bit immediate
    // value.
    EnsureSpace ensure_space(this);
    emit_rex_64(dst);
    emit(0xB8 | dst.low_bits());
    emitq(value);
  }
}


void Assembler::movq(Register dst, ExternalReference ref) {
  Address value = reinterpret_cast<Address>(ref.address());
  movq(dst, value, RelocInfo::EXTERNAL_REFERENCE);
}


void Assembler::movq(const Operand& dst, Immediate value) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst);
  emit(0xC7);
  emit_operand(0, dst);
  emit(value);
}


// Loads the ip-relative location of the src label into the target location
// (as a 32-bit offset sign extended to 64-bit).
void Assembler::movl(const Operand& dst, Label* src) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst);
  emit(0xC7);
  emit_operand(0, dst);
  if (src->is_bound()) {
    int offset = src->pos() - pc_offset() - sizeof(int32_t);
    ASSERT(offset <= 0);
    emitl(offset);
  } else if (src->is_linked()) {
    emitl(src->pos());
    src->link_to(pc_offset() - sizeof(int32_t));
  } else {
    ASSERT(src->is_unused());
    int32_t current = pc_offset();
    emitl(current);
    src->link_to(current);
  }
}


void Assembler::movq(Register dst, Handle<Object> value, RelocInfo::Mode mode) {
  AllowDeferredHandleDereference using_raw_address;
  // If there is no relocation info, emit the value of the handle efficiently
  // (possibly using less that 8 bytes for the value).
  if (RelocInfo::IsNone(mode)) {
    // There is no possible reason to store a heap pointer without relocation
    // info, so it must be a smi.
    ASSERT(value->IsSmi());
    movq(dst, reinterpret_cast<int64_t>(*value), RelocInfo::NONE64);
  } else {
    EnsureSpace ensure_space(this);
    ASSERT(value->IsHeapObject());
    ASSERT(!isolate()->heap()->InNewSpace(*value));
    emit_rex_64(dst);
    emit(0xB8 | dst.low_bits());
    emitp(value.location(), mode);
  }
}


void Assembler::movsxbq(Register dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst, src);
  emit(0x0F);
  emit(0xBE);
  emit_operand(dst, src);
}


void Assembler::movsxwq(Register dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst, src);
  emit(0x0F);
  emit(0xBF);
  emit_operand(dst, src);
}


void Assembler::movsxlq(Register dst, Register src) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst, src);
  emit(0x63);
  emit_modrm(dst, src);
}


void Assembler::movsxlq(Register dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst, src);
  emit(0x63);
  emit_operand(dst, src);
}


void Assembler::movzxbq(Register dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  // 32 bit operations zero the top 32 bits of 64 bit registers.  Therefore
  // there is no need to make this a 64 bit operation.
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0xB6);
  emit_operand(dst, src);
}


void Assembler::movzxbl(Register dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0xB6);
  emit_operand(dst, src);
}


void Assembler::movzxwq(Register dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0xB7);
  emit_operand(dst, src);
}


void Assembler::movzxwl(Register dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0xB7);
  emit_operand(dst, src);
}


void Assembler::movzxwl(Register dst, Register src) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0xB7);
  emit_modrm(dst, src);
}


void Assembler::repmovsb() {
  EnsureSpace ensure_space(this);
  emit(0xF3);
  emit(0xA4);
}


void Assembler::repmovsw() {
  EnsureSpace ensure_space(this);
  emit(0x66);  // Operand size override.
  emit(0xF3);
  emit(0xA4);
}


void Assembler::repmovsl() {
  EnsureSpace ensure_space(this);
  emit(0xF3);
  emit(0xA5);
}


void Assembler::repmovsq() {
  EnsureSpace ensure_space(this);
  emit(0xF3);
  emit_rex_64();
  emit(0xA5);
}


void Assembler::mul(Register src) {
  EnsureSpace ensure_space(this);
  emit_rex_64(src);
  emit(0xF7);
  emit_modrm(0x4, src);
}


void Assembler::neg(Register dst) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst);
  emit(0xF7);
  emit_modrm(0x3, dst);
}


void Assembler::negl(Register dst) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst);
  emit(0xF7);
  emit_modrm(0x3, dst);
}


void Assembler::neg(const Operand& dst) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst);
  emit(0xF7);
  emit_operand(3, dst);
}


void Assembler::nop() {
  EnsureSpace ensure_space(this);
  emit(0x90);
}


void Assembler::not_(Register dst) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst);
  emit(0xF7);
  emit_modrm(0x2, dst);
}


void Assembler::not_(const Operand& dst) {
  EnsureSpace ensure_space(this);
  emit_rex_64(dst);
  emit(0xF7);
  emit_operand(2, dst);
}


void Assembler::notl(Register dst) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst);
  emit(0xF7);
  emit_modrm(0x2, dst);
}


void Assembler::Nop(int n) {
  // The recommended muti-byte sequences of NOP instructions from the Intel 64
  // and IA-32 Architectures Software Developer's Manual.
  //
  // Length   Assembly                                Byte Sequence
  // 2 bytes  66 NOP                                  66 90H
  // 3 bytes  NOP DWORD ptr [EAX]                     0F 1F 00H
  // 4 bytes  NOP DWORD ptr [EAX + 00H]               0F 1F 40 00H
  // 5 bytes  NOP DWORD ptr [EAX + EAX*1 + 00H]       0F 1F 44 00 00H
  // 6 bytes  66 NOP DWORD ptr [EAX + EAX*1 + 00H]    66 0F 1F 44 00 00H
  // 7 bytes  NOP DWORD ptr [EAX + 00000000H]         0F 1F 80 00 00 00 00H
  // 8 bytes  NOP DWORD ptr [EAX + EAX*1 + 00000000H] 0F 1F 84 00 00 00 00 00H
  // 9 bytes  66 NOP DWORD ptr [EAX + EAX*1 +         66 0F 1F 84 00 00 00 00
  //          00000000H]                              00H

  EnsureSpace ensure_space(this);
  while (n > 0) {
    switch (n) {
      case 2:
        emit(0x66);
      case 1:
        emit(0x90);
        return;
      case 3:
        emit(0x0f);
        emit(0x1f);
        emit(0x00);
        return;
      case 4:
        emit(0x0f);
        emit(0x1f);
        emit(0x40);
        emit(0x00);
        return;
      case 6:
        emit(0x66);
      case 5:
        emit(0x0f);
        emit(0x1f);
        emit(0x44);
        emit(0x00);
        emit(0x00);
        return;
      case 7:
        emit(0x0f);
        emit(0x1f);
        emit(0x80);
        emit(0x00);
        emit(0x00);
        emit(0x00);
        emit(0x00);
        return;
      default:
      case 11:
        emit(0x66);
        n--;
      case 10:
        emit(0x66);
        n--;
      case 9:
        emit(0x66);
        n--;
      case 8:
        emit(0x0f);
        emit(0x1f);
        emit(0x84);
        emit(0x00);
        emit(0x00);
        emit(0x00);
        emit(0x00);
        emit(0x00);
        n -= 8;
    }
  }
}


void Assembler::pop(Register dst) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst);
  emit(0x58 | dst.low_bits());
}


void Assembler::pop(const Operand& dst) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst);
  emit(0x8F);
  emit_operand(0, dst);
}


void Assembler::popfq() {
  EnsureSpace ensure_space(this);
  emit(0x9D);
}


void Assembler::push(Register src) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(src);
  emit(0x50 | src.low_bits());
}


void Assembler::push(const Operand& src) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(src);
  emit(0xFF);
  emit_operand(6, src);
}


void Assembler::push(Immediate value) {
  EnsureSpace ensure_space(this);
  if (is_int8(value.value_)) {
    emit(0x6A);
    emit(value.value_);  // Emit low byte of value.
  } else {
    emit(0x68);
    emitl(value.value_);
  }
}


void Assembler::push_imm32(int32_t imm32) {
  EnsureSpace ensure_space(this);
  emit(0x68);
  emitl(imm32);
}


void Assembler::pushfq() {
  EnsureSpace ensure_space(this);
  emit(0x9C);
}


void Assembler::ret(int imm16) {
  EnsureSpace ensure_space(this);
  ASSERT(is_uint16(imm16));
  if (imm16 == 0) {
    emit(0xC3);
  } else {
    emit(0xC2);
    emit(imm16 & 0xFF);
    emit((imm16 >> 8) & 0xFF);
  }
}


void Assembler::setcc(Condition cc, Register reg) {
  if (cc > last_condition) {
    movb(reg, Immediate(cc == always ? 1 : 0));
    return;
  }
  EnsureSpace ensure_space(this);
  ASSERT(is_uint4(cc));
  if (!reg.is_byte_register()) {  // Use x64 byte registers, where different.
    emit_rex_32(reg);
  }
  emit(0x0F);
  emit(0x90 | cc);
  emit_modrm(0x0, reg);
}


void Assembler::shld(Register dst, Register src) {
  EnsureSpace ensure_space(this);
  emit_rex_64(src, dst);
  emit(0x0F);
  emit(0xA5);
  emit_modrm(src, dst);
}


void Assembler::shrd(Register dst, Register src) {
  EnsureSpace ensure_space(this);
  emit_rex_64(src, dst);
  emit(0x0F);
  emit(0xAD);
  emit_modrm(src, dst);
}


void Assembler::xchgq(Register dst, Register src) {
  EnsureSpace ensure_space(this);
  if (src.is(rax) || dst.is(rax)) {  // Single-byte encoding
    Register other = src.is(rax) ? dst : src;
    emit_rex_64(other);
    emit(0x90 | other.low_bits());
  } else if (dst.low_bits() == 4) {
    emit_rex_64(dst, src);
    emit(0x87);
    emit_modrm(dst, src);
  } else {
    emit_rex_64(src, dst);
    emit(0x87);
    emit_modrm(src, dst);
  }
}


void Assembler::xchgl(Register dst, Register src) {
  EnsureSpace ensure_space(this);
  if (src.is(rax) || dst.is(rax)) {  // Single-byte encoding
    Register other = src.is(rax) ? dst : src;
    emit_optional_rex_32(other);
    emit(0x90 | other.low_bits());
  } else if (dst.low_bits() == 4) {
    emit_optional_rex_32(dst, src);
    emit(0x87);
    emit_modrm(dst, src);
  } else {
    emit_optional_rex_32(src, dst);
    emit(0x87);
    emit_modrm(src, dst);
  }
}


void Assembler::store_rax(void* dst, RelocInfo::Mode mode) {
  EnsureSpace ensure_space(this);
  emit(0x48);  // REX.W
  emit(0xA3);
  emitp(dst, mode);
}


void Assembler::store_rax(ExternalReference ref) {
  store_rax(ref.address(), RelocInfo::EXTERNAL_REFERENCE);
}


void Assembler::testb(Register dst, Register src) {
  EnsureSpace ensure_space(this);
  if (src.low_bits() == 4) {
    emit_rex_32(src, dst);
    emit(0x84);
    emit_modrm(src, dst);
  } else {
    if (!dst.is_byte_register() || !src.is_byte_register()) {
      // Register is not one of al, bl, cl, dl.  Its encoding needs REX.
      emit_rex_32(dst, src);
    }
    emit(0x84);
    emit_modrm(dst, src);
  }
}


void Assembler::testb(Register reg, Immediate mask) {
  ASSERT(is_int8(mask.value_) || is_uint8(mask.value_));
  EnsureSpace ensure_space(this);
  if (reg.is(rax)) {
    emit(0xA8);
    emit(mask.value_);  // Low byte emitted.
  } else {
    if (!reg.is_byte_register()) {
      // Register is not one of al, bl, cl, dl.  Its encoding needs REX.
      emit_rex_32(reg);
    }
    emit(0xF6);
    emit_modrm(0x0, reg);
    emit(mask.value_);  // Low byte emitted.
  }
}


void Assembler::testb(const Operand& op, Immediate mask) {
  ASSERT(is_int8(mask.value_) || is_uint8(mask.value_));
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(rax, op);
  emit(0xF6);
  emit_operand(rax, op);  // Operation code 0
  emit(mask.value_);  // Low byte emitted.
}


void Assembler::testb(const Operand& op, Register reg) {
  EnsureSpace ensure_space(this);
  if (!reg.is_byte_register()) {
    // Register is not one of al, bl, cl, dl.  Its encoding needs REX.
    emit_rex_32(reg, op);
  } else {
    emit_optional_rex_32(reg, op);
  }
  emit(0x84);
  emit_operand(reg, op);
}


void Assembler::testl(Register dst, Register src) {
  EnsureSpace ensure_space(this);
  if (src.low_bits() == 4) {
    emit_optional_rex_32(src, dst);
    emit(0x85);
    emit_modrm(src, dst);
  } else {
    emit_optional_rex_32(dst, src);
    emit(0x85);
    emit_modrm(dst, src);
  }
}


void Assembler::testl(Register reg, Immediate mask) {
  // testl with a mask that fits in the low byte is exactly testb.
  if (is_uint8(mask.value_)) {
    testb(reg, mask);
    return;
  }
  EnsureSpace ensure_space(this);
  if (reg.is(rax)) {
    emit(0xA9);
    emit(mask);
  } else {
    emit_optional_rex_32(rax, reg);
    emit(0xF7);
    emit_modrm(0x0, reg);
    emit(mask);
  }
}


void Assembler::testl(const Operand& op, Immediate mask) {
  // testl with a mask that fits in the low byte is exactly testb.
  if (is_uint8(mask.value_)) {
    testb(op, mask);
    return;
  }
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(rax, op);
  emit(0xF7);
  emit_operand(rax, op);  // Operation code 0
  emit(mask);
}


void Assembler::testl(const Operand& op, Register reg) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(reg, op);
  emit(0x85);
  emit_operand(reg, op);
}


void Assembler::testq(const Operand& op, Register reg) {
  EnsureSpace ensure_space(this);
  emit_rex_64(reg, op);
  emit(0x85);
  emit_operand(reg, op);
}


void Assembler::testq(Register dst, Register src) {
  EnsureSpace ensure_space(this);
  if (src.low_bits() == 4) {
    emit_rex_64(src, dst);
    emit(0x85);
    emit_modrm(src, dst);
  } else {
    emit_rex_64(dst, src);
    emit(0x85);
    emit_modrm(dst, src);
  }
}


void Assembler::testq(Register dst, Immediate mask) {
  if (is_uint8(mask.value_)) {
    testb(dst, mask);
    return;
  }
  EnsureSpace ensure_space(this);
  if (dst.is(rax)) {
    emit_rex_64();
    emit(0xA9);
    emit(mask);
  } else {
    emit_rex_64(dst);
    emit(0xF7);
    emit_modrm(0, dst);
    emit(mask);
  }
}


// FPU instructions.


void Assembler::fld(int i) {
  EnsureSpace ensure_space(this);
  emit_farith(0xD9, 0xC0, i);
}


void Assembler::fld1() {
  EnsureSpace ensure_space(this);
  emit(0xD9);
  emit(0xE8);
}


void Assembler::fldz() {
  EnsureSpace ensure_space(this);
  emit(0xD9);
  emit(0xEE);
}


void Assembler::fldpi() {
  EnsureSpace ensure_space(this);
  emit(0xD9);
  emit(0xEB);
}


void Assembler::fldln2() {
  EnsureSpace ensure_space(this);
  emit(0xD9);
  emit(0xED);
}


void Assembler::fld_s(const Operand& adr) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(adr);
  emit(0xD9);
  emit_operand(0, adr);
}


void Assembler::fld_d(const Operand& adr) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(adr);
  emit(0xDD);
  emit_operand(0, adr);
}


void Assembler::fstp_s(const Operand& adr) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(adr);
  emit(0xD9);
  emit_operand(3, adr);
}


void Assembler::fstp_d(const Operand& adr) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(adr);
  emit(0xDD);
  emit_operand(3, adr);
}


void Assembler::fstp(int index) {
  ASSERT(is_uint3(index));
  EnsureSpace ensure_space(this);
  emit_farith(0xDD, 0xD8, index);
}


void Assembler::fild_s(const Operand& adr) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(adr);
  emit(0xDB);
  emit_operand(0, adr);
}


void Assembler::fild_d(const Operand& adr) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(adr);
  emit(0xDF);
  emit_operand(5, adr);
}


void Assembler::fistp_s(const Operand& adr) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(adr);
  emit(0xDB);
  emit_operand(3, adr);
}


void Assembler::fisttp_s(const Operand& adr) {
  ASSERT(IsEnabled(SSE3));
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(adr);
  emit(0xDB);
  emit_operand(1, adr);
}


void Assembler::fisttp_d(const Operand& adr) {
  ASSERT(IsEnabled(SSE3));
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(adr);
  emit(0xDD);
  emit_operand(1, adr);
}


void Assembler::fist_s(const Operand& adr) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(adr);
  emit(0xDB);
  emit_operand(2, adr);
}


void Assembler::fistp_d(const Operand& adr) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(adr);
  emit(0xDF);
  emit_operand(7, adr);
}


void Assembler::fabs() {
  EnsureSpace ensure_space(this);
  emit(0xD9);
  emit(0xE1);
}


void Assembler::fchs() {
  EnsureSpace ensure_space(this);
  emit(0xD9);
  emit(0xE0);
}


void Assembler::fcos() {
  EnsureSpace ensure_space(this);
  emit(0xD9);
  emit(0xFF);
}


void Assembler::fsin() {
  EnsureSpace ensure_space(this);
  emit(0xD9);
  emit(0xFE);
}


void Assembler::fptan() {
  EnsureSpace ensure_space(this);
  emit(0xD9);
  emit(0xF2);
}


void Assembler::fyl2x() {
  EnsureSpace ensure_space(this);
  emit(0xD9);
  emit(0xF1);
}


void Assembler::f2xm1() {
  EnsureSpace ensure_space(this);
  emit(0xD9);
  emit(0xF0);
}


void Assembler::fscale() {
  EnsureSpace ensure_space(this);
  emit(0xD9);
  emit(0xFD);
}


void Assembler::fninit() {
  EnsureSpace ensure_space(this);
  emit(0xDB);
  emit(0xE3);
}


void Assembler::fadd(int i) {
  EnsureSpace ensure_space(this);
  emit_farith(0xDC, 0xC0, i);
}


void Assembler::fsub(int i) {
  EnsureSpace ensure_space(this);
  emit_farith(0xDC, 0xE8, i);
}


void Assembler::fisub_s(const Operand& adr) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(adr);
  emit(0xDA);
  emit_operand(4, adr);
}


void Assembler::fmul(int i) {
  EnsureSpace ensure_space(this);
  emit_farith(0xDC, 0xC8, i);
}


void Assembler::fdiv(int i) {
  EnsureSpace ensure_space(this);
  emit_farith(0xDC, 0xF8, i);
}


void Assembler::faddp(int i) {
  EnsureSpace ensure_space(this);
  emit_farith(0xDE, 0xC0, i);
}


void Assembler::fsubp(int i) {
  EnsureSpace ensure_space(this);
  emit_farith(0xDE, 0xE8, i);
}


void Assembler::fsubrp(int i) {
  EnsureSpace ensure_space(this);
  emit_farith(0xDE, 0xE0, i);
}


void Assembler::fmulp(int i) {
  EnsureSpace ensure_space(this);
  emit_farith(0xDE, 0xC8, i);
}


void Assembler::fdivp(int i) {
  EnsureSpace ensure_space(this);
  emit_farith(0xDE, 0xF8, i);
}


void Assembler::fprem() {
  EnsureSpace ensure_space(this);
  emit(0xD9);
  emit(0xF8);
}


void Assembler::fprem1() {
  EnsureSpace ensure_space(this);
  emit(0xD9);
  emit(0xF5);
}


void Assembler::fxch(int i) {
  EnsureSpace ensure_space(this);
  emit_farith(0xD9, 0xC8, i);
}


void Assembler::fincstp() {
  EnsureSpace ensure_space(this);
  emit(0xD9);
  emit(0xF7);
}


void Assembler::ffree(int i) {
  EnsureSpace ensure_space(this);
  emit_farith(0xDD, 0xC0, i);
}


void Assembler::ftst() {
  EnsureSpace ensure_space(this);
  emit(0xD9);
  emit(0xE4);
}


void Assembler::fucomp(int i) {
  EnsureSpace ensure_space(this);
  emit_farith(0xDD, 0xE8, i);
}


void Assembler::fucompp() {
  EnsureSpace ensure_space(this);
  emit(0xDA);
  emit(0xE9);
}


void Assembler::fucomi(int i) {
  EnsureSpace ensure_space(this);
  emit(0xDB);
  emit(0xE8 + i);
}


void Assembler::fucomip() {
  EnsureSpace ensure_space(this);
  emit(0xDF);
  emit(0xE9);
}


void Assembler::fcompp() {
  EnsureSpace ensure_space(this);
  emit(0xDE);
  emit(0xD9);
}


void Assembler::fnstsw_ax() {
  EnsureSpace ensure_space(this);
  emit(0xDF);
  emit(0xE0);
}


void Assembler::fwait() {
  EnsureSpace ensure_space(this);
  emit(0x9B);
}


void Assembler::frndint() {
  EnsureSpace ensure_space(this);
  emit(0xD9);
  emit(0xFC);
}


void Assembler::fnclex() {
  EnsureSpace ensure_space(this);
  emit(0xDB);
  emit(0xE2);
}


void Assembler::sahf() {
  // TODO(X64): Test for presence. Not all 64-bit intel CPU's have sahf
  // in 64-bit mode. Test CpuID.
  EnsureSpace ensure_space(this);
  emit(0x9E);
}


void Assembler::emit_farith(int b1, int b2, int i) {
  ASSERT(is_uint8(b1) && is_uint8(b2));  // wrong opcode
  ASSERT(is_uint3(i));  // illegal stack offset
  emit(b1);
  emit(b2 + i);
}


// SSE operations.

void Assembler::andps(XMMRegister dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x54);
  emit_sse_operand(dst, src);
}


// SSE 2 operations.

void Assembler::movd(XMMRegister dst, Register src) {
  EnsureSpace ensure_space(this);
  emit(0x66);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x6E);
  emit_sse_operand(dst, src);
}


void Assembler::movd(Register dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0x66);
  emit_optional_rex_32(src, dst);
  emit(0x0F);
  emit(0x7E);
  emit_sse_operand(src, dst);
}


void Assembler::movq(XMMRegister dst, Register src) {
  EnsureSpace ensure_space(this);
  emit(0x66);
  emit_rex_64(dst, src);
  emit(0x0F);
  emit(0x6E);
  emit_sse_operand(dst, src);
}


void Assembler::movq(Register dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0x66);
  emit_rex_64(src, dst);
  emit(0x0F);
  emit(0x7E);
  emit_sse_operand(src, dst);
}


void Assembler::movq(XMMRegister dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  if (dst.low_bits() == 4) {
    // Avoid unnecessary SIB byte.
    emit(0xf3);
    emit_optional_rex_32(dst, src);
    emit(0x0F);
    emit(0x7e);
    emit_sse_operand(dst, src);
  } else {
    emit(0x66);
    emit_optional_rex_32(src, dst);
    emit(0x0F);
    emit(0xD6);
    emit_sse_operand(src, dst);
  }
}


void Assembler::movdqa(const Operand& dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0x66);
  emit_rex_64(src, dst);
  emit(0x0F);
  emit(0x7F);
  emit_sse_operand(src, dst);
}


void Assembler::movdqa(XMMRegister dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit(0x66);
  emit_rex_64(dst, src);
  emit(0x0F);
  emit(0x6F);
  emit_sse_operand(dst, src);
}


void Assembler::movdqu(const Operand& dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0xF3);
  emit_rex_64(src, dst);
  emit(0x0F);
  emit(0x7F);
  emit_sse_operand(src, dst);
}


void Assembler::movdqu(XMMRegister dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit(0xF3);
  emit_rex_64(dst, src);
  emit(0x0F);
  emit(0x6F);
  emit_sse_operand(dst, src);
}


void Assembler::extractps(Register dst, XMMRegister src, byte imm8) {
  ASSERT(IsEnabled(SSE4_1));
  ASSERT(is_uint8(imm8));
  EnsureSpace ensure_space(this);
  emit(0x66);
  emit_optional_rex_32(src, dst);
  emit(0x0F);
  emit(0x3A);
  emit(0x17);
  emit_sse_operand(src, dst);
  emit(imm8);
}


void Assembler::movsd(const Operand& dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0xF2);  // double
  emit_optional_rex_32(src, dst);
  emit(0x0F);
  emit(0x11);  // store
  emit_sse_operand(src, dst);
}


void Assembler::movsd(XMMRegister dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0xF2);  // double
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x10);  // load
  emit_sse_operand(dst, src);
}


void Assembler::movsd(XMMRegister dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit(0xF2);  // double
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x10);  // load
  emit_sse_operand(dst, src);
}


void Assembler::movaps(XMMRegister dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  if (src.low_bits() == 4) {
    // Try to avoid an unnecessary SIB byte.
    emit_optional_rex_32(src, dst);
    emit(0x0F);
    emit(0x29);
    emit_sse_operand(src, dst);
  } else {
    emit_optional_rex_32(dst, src);
    emit(0x0F);
    emit(0x28);
    emit_sse_operand(dst, src);
  }
}


void Assembler::movapd(XMMRegister dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  if (src.low_bits() == 4) {
    // Try to avoid an unnecessary SIB byte.
    emit(0x66);
    emit_optional_rex_32(src, dst);
    emit(0x0F);
    emit(0x29);
    emit_sse_operand(src, dst);
  } else {
    emit(0x66);
    emit_optional_rex_32(dst, src);
    emit(0x0F);
    emit(0x28);
    emit_sse_operand(dst, src);
  }
}


void Assembler::movss(XMMRegister dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit(0xF3);  // single
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x10);  // load
  emit_sse_operand(dst, src);
}


void Assembler::movss(const Operand& src, XMMRegister dst) {
  EnsureSpace ensure_space(this);
  emit(0xF3);  // single
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x11);  // store
  emit_sse_operand(dst, src);
}


void Assembler::cvttss2si(Register dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit(0xF3);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x2C);
  emit_operand(dst, src);
}


void Assembler::cvttss2si(Register dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0xF3);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x2C);
  emit_sse_operand(dst, src);
}


void Assembler::cvttsd2si(Register dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit(0xF2);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x2C);
  emit_operand(dst, src);
}


void Assembler::cvttsd2si(Register dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0xF2);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x2C);
  emit_sse_operand(dst, src);
}


void Assembler::cvttsd2siq(Register dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0xF2);
  emit_rex_64(dst, src);
  emit(0x0F);
  emit(0x2C);
  emit_sse_operand(dst, src);
}


void Assembler::cvtlsi2sd(XMMRegister dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit(0xF2);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x2A);
  emit_sse_operand(dst, src);
}


void Assembler::cvtlsi2sd(XMMRegister dst, Register src) {
  EnsureSpace ensure_space(this);
  emit(0xF2);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x2A);
  emit_sse_operand(dst, src);
}


void Assembler::cvtlsi2ss(XMMRegister dst, Register src) {
  EnsureSpace ensure_space(this);
  emit(0xF3);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x2A);
  emit_sse_operand(dst, src);
}


void Assembler::cvtqsi2sd(XMMRegister dst, Register src) {
  EnsureSpace ensure_space(this);
  emit(0xF2);
  emit_rex_64(dst, src);
  emit(0x0F);
  emit(0x2A);
  emit_sse_operand(dst, src);
}


void Assembler::cvtss2sd(XMMRegister dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0xF3);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x5A);
  emit_sse_operand(dst, src);
}


void Assembler::cvtss2sd(XMMRegister dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit(0xF3);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x5A);
  emit_sse_operand(dst, src);
}


void Assembler::cvtsd2ss(XMMRegister dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0xF2);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x5A);
  emit_sse_operand(dst, src);
}


void Assembler::cvtsd2si(Register dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0xF2);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x2D);
  emit_sse_operand(dst, src);
}


void Assembler::cvtsd2siq(Register dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0xF2);
  emit_rex_64(dst, src);
  emit(0x0F);
  emit(0x2D);
  emit_sse_operand(dst, src);
}


void Assembler::addsd(XMMRegister dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0xF2);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x58);
  emit_sse_operand(dst, src);
}


void Assembler::addsd(XMMRegister dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit(0xF2);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x58);
  emit_sse_operand(dst, src);
}


void Assembler::mulsd(XMMRegister dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0xF2);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x59);
  emit_sse_operand(dst, src);
}


void Assembler::mulsd(XMMRegister dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit(0xF2);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x59);
  emit_sse_operand(dst, src);
}


void Assembler::subsd(XMMRegister dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0xF2);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x5C);
  emit_sse_operand(dst, src);
}


void Assembler::divsd(XMMRegister dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0xF2);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x5E);
  emit_sse_operand(dst, src);
}


void Assembler::andpd(XMMRegister dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0x66);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x54);
  emit_sse_operand(dst, src);
}


void Assembler::orpd(XMMRegister dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0x66);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x56);
  emit_sse_operand(dst, src);
}


void Assembler::xorpd(XMMRegister dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0x66);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x57);
  emit_sse_operand(dst, src);
}


void Assembler::xorps(XMMRegister dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x57);
  emit_sse_operand(dst, src);
}


void Assembler::sqrtsd(XMMRegister dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0xF2);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0x51);
  emit_sse_operand(dst, src);
}


void Assembler::ucomisd(XMMRegister dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0x66);
  emit_optional_rex_32(dst, src);
  emit(0x0f);
  emit(0x2e);
  emit_sse_operand(dst, src);
}


void Assembler::ucomisd(XMMRegister dst, const Operand& src) {
  EnsureSpace ensure_space(this);
  emit(0x66);
  emit_optional_rex_32(dst, src);
  emit(0x0f);
  emit(0x2e);
  emit_sse_operand(dst, src);
}


void Assembler::cmpltsd(XMMRegister dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0xF2);
  emit_optional_rex_32(dst, src);
  emit(0x0F);
  emit(0xC2);
  emit_sse_operand(dst, src);
  emit(0x01);  // LT == 1
}


void Assembler::roundsd(XMMRegister dst, XMMRegister src,
                        Assembler::RoundingMode mode) {
  ASSERT(IsEnabled(SSE4_1));
  EnsureSpace ensure_space(this);
  emit(0x66);
  emit_optional_rex_32(dst, src);
  emit(0x0f);
  emit(0x3a);
  emit(0x0b);
  emit_sse_operand(dst, src);
  // Mask precision exeption.
  emit(static_cast<byte>(mode) | 0x8);
}


void Assembler::movmskpd(Register dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit(0x66);
  emit_optional_rex_32(dst, src);
  emit(0x0f);
  emit(0x50);
  emit_sse_operand(dst, src);
}


void Assembler::movmskps(Register dst, XMMRegister src) {
  EnsureSpace ensure_space(this);
  emit_optional_rex_32(dst, src);
  emit(0x0f);
  emit(0x50);
  emit_sse_operand(dst, src);
}


void Assembler::emit_sse_operand(XMMRegister reg, const Operand& adr) {
  Register ireg = { reg.code() };
  emit_operand(ireg, adr);
}


void Assembler::emit_sse_operand(XMMRegister dst, XMMRegister src) {
  emit(0xC0 | (dst.low_bits() << 3) | src.low_bits());
}


void Assembler::emit_sse_operand(XMMRegister dst, Register src) {
  emit(0xC0 | (dst.low_bits() << 3) | src.low_bits());
}


void Assembler::emit_sse_operand(Register dst, XMMRegister src) {
  emit(0xC0 | (dst.low_bits() << 3) | src.low_bits());
}


void Assembler::db(uint8_t data) {
  EnsureSpace ensure_space(this);
  emit(data);
}


void Assembler::dd(uint32_t data) {
  EnsureSpace ensure_space(this);
  emitl(data);
}


// Relocation information implementations.

void Assembler::RecordRelocInfo(RelocInfo::Mode rmode, intptr_t data) {
  ASSERT(!RelocInfo::IsNone(rmode));
  if (rmode == RelocInfo::EXTERNAL_REFERENCE) {
    // Don't record external references unless the heap will be serialized.
#ifdef DEBUG
    if (!Serializer::enabled()) {
      Serializer::TooLateToEnableNow();
    }
#endif
    if (!Serializer::enabled() && !emit_debug_code()) {
      return;
    }
  } else if (rmode == RelocInfo::CODE_AGE_SEQUENCE) {
    // Don't record psuedo relocation info for code age sequence mode.
    return;
  }
  RelocInfo rinfo(pc_, rmode, data, NULL);
  reloc_info_writer.Write(&rinfo);
}


void Assembler::RecordJSReturn() {
  positions_recorder()->WriteRecordedPositions();
  EnsureSpace ensure_space(this);
  RecordRelocInfo(RelocInfo::JS_RETURN);
}


void Assembler::RecordDebugBreakSlot() {
  positions_recorder()->WriteRecordedPositions();
  EnsureSpace ensure_space(this);
  RecordRelocInfo(RelocInfo::DEBUG_BREAK_SLOT);
}


void Assembler::RecordComment(const char* msg, bool force) {
  if (FLAG_code_comments || force) {
    EnsureSpace ensure_space(this);
    RecordRelocInfo(RelocInfo::COMMENT, reinterpret_cast<intptr_t>(msg));
  }
}


const int RelocInfo::kApplyMask = RelocInfo::kCodeTargetMask |
    1 << RelocInfo::RUNTIME_ENTRY |
    1 << RelocInfo::INTERNAL_REFERENCE |
    1 << RelocInfo::CODE_AGE_SEQUENCE;


bool RelocInfo::IsCodedSpecially() {
  // The deserializer needs to know whether a pointer is specially coded.  Being
  // specially coded on x64 means that it is a relative 32 bit address, as used
  // by branch instructions.
  return (1 << rmode_) & kApplyMask;
}

} }  // namespace v8::internal

#endif  // V8_TARGET_ARCH_X64
