// Copyright (c) 2005, Google Inc.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
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

// ---
// Author: Sanjay Ghemawat
//
// This has the implementation details of malloc_hook that are needed
// to use malloc-hook inside the tcmalloc system.  It does not hold
// any of the client-facing calls that are used to add new hooks.

#ifndef _MALLOC_HOOK_INL_H_
#define _MALLOC_HOOK_INL_H_

#include <stddef.h>
#include <sys/types.h>
#include "base/atomicops.h"
#include "base/basictypes.h"
#include <google/malloc_hook.h>

namespace base { namespace internal {

// A simple atomic pointer class that can be initialized by the linker
// when you define a namespace-scope variable as:
//
//   AtomicPtr<Foo*> my_global = { &initial_value };
//
// This isn't suitable for a general atomic<> class because of the
// public access to data_.
template<typename PtrT>
class AtomicPtr {
 public:
  COMPILE_ASSERT(sizeof(PtrT) <= sizeof(AtomicWord),
                 PtrT_should_fit_in_AtomicWord);

  PtrT Get() const {
    // Depending on the system, Acquire_Load(AtomicWord*) may have
    // been defined to return an AtomicWord, Atomic32, or Atomic64.
    // We hide that implementation detail here with an explicit cast.
    // This prevents MSVC 2005, at least, from complaining (it has to
    // do with __wp64; AtomicWord is __wp64, but Atomic32/64 aren't).
    return reinterpret_cast<PtrT>(static_cast<AtomicWord>(
      base::subtle::Acquire_Load(&data_)));
  }

  // Sets the contained value to new_val and returns the old value,
  // atomically, with acquire and release semantics.
  // This is a full-barrier instruction.
  PtrT Exchange(PtrT new_val);

  // Atomically executes:
  //      result = data_
  //      if (data_ == old_val)
  //        data_ = new_val;
  //      return result;
  // This is a full-barrier instruction.
  PtrT CompareAndSwap(PtrT old_val, PtrT new_val);

  // Not private so that the class is an aggregate and can be
  // initialized by the linker. Don't access this directly.
  AtomicWord data_;
};

// These are initialized in malloc_hook.cc
extern AtomicPtr<MallocHook::NewHook>     new_hook_;
extern AtomicPtr<MallocHook::DeleteHook>  delete_hook_;
extern AtomicPtr<MallocHook::PreMmapHook> premmap_hook_;
extern AtomicPtr<MallocHook::MmapHook>    mmap_hook_;
extern AtomicPtr<MallocHook::MunmapHook>  munmap_hook_;
extern AtomicPtr<MallocHook::MremapHook>  mremap_hook_;
extern AtomicPtr<MallocHook::PreSbrkHook> presbrk_hook_;
extern AtomicPtr<MallocHook::SbrkHook>    sbrk_hook_;

} }  // namespace base::internal

inline MallocHook::NewHook MallocHook::GetNewHook() {
  return base::internal::new_hook_.Get();
}

inline void MallocHook::InvokeNewHook(const void* p, size_t s) {
  MallocHook::NewHook hook = MallocHook::GetNewHook();
  if (hook != NULL) (*hook)(p, s);
}

inline MallocHook::DeleteHook MallocHook::GetDeleteHook() {
  return base::internal::delete_hook_.Get();
}

inline void MallocHook::InvokeDeleteHook(const void* p) {
  MallocHook::DeleteHook hook = MallocHook::GetDeleteHook();
  if (hook != NULL) (*hook)(p);
}

inline MallocHook::PreMmapHook MallocHook::GetPreMmapHook() {
  return base::internal::premmap_hook_.Get();
}

inline void MallocHook::InvokePreMmapHook(const void* start,
                                          size_t size,
                                          int protection,
                                          int flags,
                                          int fd,
                                          off_t offset) {
  MallocHook::PreMmapHook hook = MallocHook::GetPreMmapHook();
  if (hook != NULL) (*hook)(start, size,
                            protection, flags,
                            fd, offset);
}

inline MallocHook::MmapHook MallocHook::GetMmapHook() {
  return base::internal::mmap_hook_.Get();
}

inline void MallocHook::InvokeMmapHook(const void* result,
                                       const void* start,
                                       size_t size,
                                       int protection,
                                       int flags,
                                       int fd,
                                       off_t offset) {
  MallocHook::MmapHook hook = MallocHook::GetMmapHook();
  if (hook != NULL) (*hook)(result,
                            start, size,
                            protection, flags,
                            fd, offset);
}

inline MallocHook::MunmapHook MallocHook::GetMunmapHook() {
  return base::internal::munmap_hook_.Get();
}

inline void MallocHook::InvokeMunmapHook(const void* p, size_t size) {
  MallocHook::MunmapHook hook = MallocHook::GetMunmapHook();
  if (hook != NULL) (*hook)(p, size);
}

inline MallocHook::MremapHook MallocHook::GetMremapHook() {
  return base::internal::mremap_hook_.Get();
}

inline void MallocHook::InvokeMremapHook(const void* result,
                                         const void* old_addr,
                                         size_t old_size,
                                         size_t new_size,
                                         int flags,
                                         const void* new_addr) {
  MallocHook::MremapHook hook = MallocHook::GetMremapHook();
  if (hook != NULL) (*hook)(result,
                            old_addr, old_size,
                            new_size, flags, new_addr);
}

inline MallocHook::PreSbrkHook MallocHook::GetPreSbrkHook() {
  return base::internal::presbrk_hook_.Get();
}

inline void MallocHook::InvokePreSbrkHook(ptrdiff_t increment) {
  MallocHook::PreSbrkHook hook = MallocHook::GetPreSbrkHook();
  if (hook != NULL && increment != 0) (*hook)(increment);
}

inline MallocHook::SbrkHook MallocHook::GetSbrkHook() {
  return base::internal::sbrk_hook_.Get();
}

inline void MallocHook::InvokeSbrkHook(const void* result,
                                       ptrdiff_t increment) {
  MallocHook::SbrkHook hook = MallocHook::GetSbrkHook();
  if (hook != NULL && increment != 0) (*hook)(result, increment);
}

#endif /* _MALLOC_HOOK_INL_H_ */
