/* Copyright (c) 2007, Google Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ---
 * Author: Joi Sigurdsson
 *
 * Implementation of PreamblePatcher
 */

#include "preamble_patcher.h"

#include "mini_disassembler.h"

// Definitions of assembly statements we need
#define ASM_JMP32REL 0xE9
#define ASM_INT3 0xCC

namespace sidestep {

SideStepError PreamblePatcher::RawPatchWithStub(
    void* target_function,
    void *replacement_function,
    unsigned char* preamble_stub,
    unsigned long stub_size,
    unsigned long* bytes_needed) {
  if ((NULL == target_function) ||
      (NULL == replacement_function) ||
      (NULL == preamble_stub)) {
    SIDESTEP_ASSERT(false &&
                    "Invalid parameters - either pTargetFunction or "
                    "pReplacementFunction or pPreambleStub were NULL.");
    return SIDESTEP_INVALID_PARAMETER;
  }

  // TODO(V7:joi) Siggi and I just had a discussion and decided that both
  // patching and unpatching are actually unsafe.  We also discussed a
  // method of making it safe, which is to freeze all other threads in the
  // process, check their thread context to see if their eip is currently
  // inside the block of instructions we need to copy to the stub, and if so
  // wait a bit and try again, then unfreeze all threads once we've patched.
  // Not implementing this for now since we're only using SideStep for unit
  // testing, but if we ever use it for production code this is what we
  // should do.
  //
  // NOTE: Stoyan suggests we can write 8 or even 10 bytes atomically using
  // FPU instructions, and on newer processors we could use cmpxchg8b or
  // cmpxchg16b. So it might be possible to do the patching/unpatching
  // atomically and avoid having to freeze other threads.  Note though, that
  // doing it atomically does not help if one of the other threads happens
  // to have its eip in the middle of the bytes you change while you change
  // them.

  // First, deal with a special case that we see with functions that
  // point into an IAT table (including functions linked statically
  // into the application): these function already starts with
  // ASM_JMP32REL.  For instance, malloc() might be implemented as a
  // JMP to __malloc().  In that case, we replace the destination of
  // the JMP (__malloc), rather than the JMP itself (malloc).  This
  // way we get the correct behavior no matter how malloc gets called.
  void *new_target = ResolveTarget(target_function);
  if (new_target != target_function) {   // we're in the IAT case
    // I'd like to just say "target = new_target", but I can't,
    // because the new target will need to have its protections set.
    return RawPatchWithStubAndProtections(new_target, replacement_function,
                                          preamble_stub, stub_size,
                                          bytes_needed);
  }
  unsigned char* target = reinterpret_cast<unsigned char*>(new_target);

  // Let's disassemble the preamble of the target function to see if we can
  // patch, and to see how much of the preamble we need to take.  We need 5
  // bytes for our jmp instruction, so let's find the minimum number of
  // instructions to get 5 bytes.
  MiniDisassembler disassembler;
  unsigned int preamble_bytes = 0;
  while (preamble_bytes < 5) {
    InstructionType instruction_type =
        disassembler.Disassemble(target + preamble_bytes, preamble_bytes);
    if (IT_JUMP == instruction_type) {
      SIDESTEP_ASSERT(false &&
                      "Unable to patch because there is a jump instruction "
                      "in the first 5 bytes.");
      return SIDESTEP_JUMP_INSTRUCTION;
    } else if (IT_RETURN == instruction_type) {
      SIDESTEP_ASSERT(false &&
                      "Unable to patch because function is too short");
      return SIDESTEP_FUNCTION_TOO_SMALL;
    } else if (IT_GENERIC != instruction_type) {
      SIDESTEP_ASSERT(false &&
                      "Disassembler encountered unsupported instruction "
                      "(either unused or unknown");
      return SIDESTEP_UNSUPPORTED_INSTRUCTION;
    }
  }

  if (NULL != bytes_needed)
    *bytes_needed = preamble_bytes + 5;

  // Inv: cbPreamble is the number of bytes (at least 5) that we need to take
  // from the preamble to have whole instructions that are 5 bytes or more
  // in size total. The size of the stub required is cbPreamble + size of
  // jmp (5)
  if (preamble_bytes + 5 > stub_size) {
    SIDESTEP_ASSERT(false);
    return SIDESTEP_INSUFFICIENT_BUFFER;
  }

  // First, copy the preamble that we will overwrite.
  memcpy(reinterpret_cast<void*>(preamble_stub),
         reinterpret_cast<void*>(target), preamble_bytes);

  // Now, make a jmp instruction to the rest of the target function (minus the
  // preamble bytes we moved into the stub) and copy it into our preamble-stub.
  // find address to jump to, relative to next address after jmp instruction
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4244)
#endif
  int relative_offset_to_target_rest
      = ((reinterpret_cast<unsigned char*>(target) + preamble_bytes) -
         (preamble_stub + preamble_bytes + 5));
#ifdef _MSC_VER
#pragma warning(pop)
#endif
  // jmp (Jump near, relative, displacement relative to next instruction)
  preamble_stub[preamble_bytes] = ASM_JMP32REL;
  // copy the address
  memcpy(reinterpret_cast<void*>(preamble_stub + preamble_bytes + 1),
         reinterpret_cast<void*>(&relative_offset_to_target_rest), 4);

  // Inv: preamble_stub points to assembly code that will execute the
  // original function by first executing the first cbPreamble bytes of the
  // preamble, then jumping to the rest of the function.

  // Overwrite the first 5 bytes of the target function with a jump to our
  // replacement function.
  // (Jump near, relative, displacement relative to next instruction)
  target[0] = ASM_JMP32REL;

  // Find offset from instruction after jmp, to the replacement function.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4244)
#endif
  int offset_to_replacement_function =
      reinterpret_cast<unsigned char*>(replacement_function) -
      reinterpret_cast<unsigned char*>(target) - 5;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
  // complete the jmp instruction
  memcpy(reinterpret_cast<void*>(target + 1),
         reinterpret_cast<void*>(&offset_to_replacement_function), 4);
  // Set any remaining bytes that were moved to the preamble-stub to INT3 so
  // as not to cause confusion (otherwise you might see some strange
  // instructions if you look at the disassembly, or even invalid
  // instructions). Also, by doing this, we will break into the debugger if
  // some code calls into this portion of the code.  If this happens, it
  // means that this function cannot be patched using this patcher without
  // further thought.
  if (preamble_bytes > 5) {
    memset(reinterpret_cast<void*>(target + 5), ASM_INT3, preamble_bytes - 5);
  }

  // Inv: The memory pointed to by target_function now points to a relative
  // jump instruction that jumps over to the preamble_stub.  The preamble
  // stub contains the first stub_size bytes of the original target
  // function's preamble code, followed by a relative jump back to the next
  // instruction after the first cbPreamble bytes.

  return SIDESTEP_SUCCESS;
}

};  // namespace sidestep
