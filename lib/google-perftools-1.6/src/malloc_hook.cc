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
// Author: Sanjay Ghemawat <opensource@google.com>

#include <config.h>

// Disable the glibc prototype of mremap(), as older versions of the
// system headers define this function with only four arguments,
// whereas newer versions allow an optional fifth argument:
#ifdef HAVE_MMAP
# define mremap glibc_mremap
# include <sys/mman.h>
# undef mremap
#endif

#include <algorithm>
#include "base/basictypes.h"
#include "base/logging.h"
#include "malloc_hook-inl.h"
#include <google/malloc_hook.h>

// This #ifdef should almost never be set.  Set NO_TCMALLOC_SAMPLES if
// you're porting to a system where you really can't get a stacktrace.
#ifdef NO_TCMALLOC_SAMPLES
  // We use #define so code compiles even if you #include stacktrace.h somehow.
# define GetStackTrace(stack, depth, skip)  (0)
#else
# include <google/stacktrace.h>
#endif

// __THROW is defined in glibc systems.  It means, counter-intuitively,
// "This function will never throw an exception."  It's an optional
// optimization tool, but we may need to use it to match glibc prototypes.
#ifndef __THROW    // I guess we're not on a glibc system
# define __THROW   // __THROW is just an optimization, so ok to make it ""
#endif

using std::copy;


// Declarations of five default weak hook functions, that can be overridden by
// linking-in a strong definition (as heap-checker.cc does).  These are extern
// "C" so that they don't trigger gold's --detect-odr-violations warning, which
// only looks at C++ symbols.
//
// These default hooks let some other library we link in
// to define strong versions of InitialMallocHook_New, InitialMallocHook_MMap,
// InitialMallocHook_PreMMap, InitialMallocHook_PreSbrk, and
// InitialMallocHook_Sbrk to have a chance to hook into the very first
// invocation of an allocation function call, mmap, or sbrk.
//
// These functions are declared here as weak, and defined later, rather than a
// more straightforward simple weak definition, as a workround for an icc
// compiler issue ((Intel reference 290819).  This issue causes icc to resolve
// weak symbols too early, at compile rather than link time.  By declaring it
// (weak) here, then defining it below after its use, we can avoid the problem.
//
extern "C" {

ATTRIBUTE_WEAK
void InitialMallocHook_New(const void* ptr, size_t size);

ATTRIBUTE_WEAK
void InitialMallocHook_PreMMap(const void* start,
                               size_t size,
                               int protection,
                               int flags,
                               int fd,
                               off_t offset);

ATTRIBUTE_WEAK
void InitialMallocHook_MMap(const void* result,
                            const void* start,
                            size_t size,
                            int protection,
                            int flags,
                            int fd,
                            off_t offset);

ATTRIBUTE_WEAK
void InitialMallocHook_PreSbrk(ptrdiff_t increment);

ATTRIBUTE_WEAK
void InitialMallocHook_Sbrk(const void* result, ptrdiff_t increment);

}  // extern "C"

namespace base { namespace internal {
template<typename PtrT>
PtrT AtomicPtr<PtrT>::Exchange(PtrT new_val) {
  base::subtle::MemoryBarrier();  // Release semantics.
  // Depending on the system, NoBarrier_AtomicExchange(AtomicWord*)
  // may have been defined to return an AtomicWord, Atomic32, or
  // Atomic64.  We hide that implementation detail here with an
  // explicit cast.  This prevents MSVC 2005, at least, from complaining.
  PtrT old_val = reinterpret_cast<PtrT>(static_cast<AtomicWord>(
      base::subtle::NoBarrier_AtomicExchange(
          &data_,
          reinterpret_cast<AtomicWord>(new_val))));
  base::subtle::MemoryBarrier();  // And acquire semantics.
  return old_val;
}

template<typename PtrT>
PtrT AtomicPtr<PtrT>::CompareAndSwap(PtrT old_val, PtrT new_val) {
  base::subtle::MemoryBarrier();  // Release semantics.
  PtrT retval = reinterpret_cast<PtrT>(static_cast<AtomicWord>(
      base::subtle::NoBarrier_CompareAndSwap(
          &data_,
          reinterpret_cast<AtomicWord>(old_val),
          reinterpret_cast<AtomicWord>(new_val))));
  base::subtle::MemoryBarrier();  // And acquire semantics.
  return retval;
}

AtomicPtr<MallocHook::NewHook>    new_hook_ = {
  reinterpret_cast<AtomicWord>(InitialMallocHook_New) };
AtomicPtr<MallocHook::DeleteHook> delete_hook_ = { 0 };
AtomicPtr<MallocHook::PreMmapHook> premmap_hook_ = {
  reinterpret_cast<AtomicWord>(InitialMallocHook_PreMMap) };
AtomicPtr<MallocHook::MmapHook>   mmap_hook_ = {
  reinterpret_cast<AtomicWord>(InitialMallocHook_MMap) };
AtomicPtr<MallocHook::MunmapHook> munmap_hook_ = { 0 };
AtomicPtr<MallocHook::MremapHook> mremap_hook_ = { 0 };
AtomicPtr<MallocHook::PreSbrkHook> presbrk_hook_ = {
  reinterpret_cast<AtomicWord>(InitialMallocHook_PreSbrk) };
AtomicPtr<MallocHook::SbrkHook>   sbrk_hook_ = {
  reinterpret_cast<AtomicWord>(InitialMallocHook_Sbrk) };

} }  // namespace base::internal

using base::internal::new_hook_;
using base::internal::delete_hook_;
using base::internal::premmap_hook_;
using base::internal::mmap_hook_;
using base::internal::munmap_hook_;
using base::internal::mremap_hook_;
using base::internal::presbrk_hook_;
using base::internal::sbrk_hook_;


// These are available as C bindings as well as C++, hence their
// definition outside the MallocHook class.
extern "C"
MallocHook_NewHook MallocHook_SetNewHook(MallocHook_NewHook hook) {
  return new_hook_.Exchange(hook);
}

extern "C"
MallocHook_DeleteHook MallocHook_SetDeleteHook(MallocHook_DeleteHook hook) {
  return delete_hook_.Exchange(hook);
}

extern "C"
MallocHook_PreMmapHook MallocHook_SetPreMmapHook(MallocHook_PreMmapHook hook) {
  return premmap_hook_.Exchange(hook);
}

extern "C"
MallocHook_MmapHook MallocHook_SetMmapHook(MallocHook_MmapHook hook) {
  return mmap_hook_.Exchange(hook);
}

extern "C"
MallocHook_MunmapHook MallocHook_SetMunmapHook(MallocHook_MunmapHook hook) {
  return munmap_hook_.Exchange(hook);
}

extern "C"
MallocHook_MremapHook MallocHook_SetMremapHook(MallocHook_MremapHook hook) {
  return mremap_hook_.Exchange(hook);
}

extern "C"
MallocHook_PreSbrkHook MallocHook_SetPreSbrkHook(MallocHook_PreSbrkHook hook) {
  return presbrk_hook_.Exchange(hook);
}

extern "C"
MallocHook_SbrkHook MallocHook_SetSbrkHook(MallocHook_SbrkHook hook) {
  return sbrk_hook_.Exchange(hook);
}


// The definitions of weak default malloc hooks (New, MMap, and Sbrk)
// that self deinstall on their first call.  This is entirely for
// efficiency: the default version of these functions will be called a
// maximum of one time.  If these functions were a no-op instead, they'd
// be called every time, costing an extra function call per malloc.
//
// However, this 'delete self' isn't safe in general -- it's possible
// that this function will be called via a daisy chain.  That is,
// someone else might do
//    old_hook = MallocHook::SetNewHook(&myhook);
//    void myhook(void* ptr, size_t size) {
//       do_my_stuff();
//       old_hook(ptr, size);   // daisy-chain the hooks
//    }
// If old_hook is InitialMallocHook_New(), then this is broken code! --
// after the first run it'll deregister not only InitialMallocHook_New()
// but also myhook.  To protect against that, InitialMallocHook_New()
// makes sure it's the 'top-level' hook before doing the deregistration.
// This means the daisy-chain case will be less efficient because the
// hook will be called, and do an if check, for every new.  Alas.
// TODO(csilvers): add support for removing a hook from the middle of a chain.

void InitialMallocHook_New(const void* ptr, size_t size) {
  // Set new_hook to NULL iff its previous value was InitialMallocHook_New
  new_hook_.CompareAndSwap(&InitialMallocHook_New, NULL);
}

void InitialMallocHook_PreMMap(const void* start,
                               size_t size,
                               int protection,
                               int flags,
                               int fd,
                               off_t offset) {
  premmap_hook_.CompareAndSwap(&InitialMallocHook_PreMMap, NULL);
}

void InitialMallocHook_MMap(const void* result,
                            const void* start,
                            size_t size,
                            int protection,
                            int flags,
                            int fd,
                            off_t offset) {
  mmap_hook_.CompareAndSwap(&InitialMallocHook_MMap, NULL);
}

void InitialMallocHook_PreSbrk(ptrdiff_t increment) {
  presbrk_hook_.CompareAndSwap(&InitialMallocHook_PreSbrk, NULL);
}

void InitialMallocHook_Sbrk(const void* result, ptrdiff_t increment) {
  sbrk_hook_.CompareAndSwap(&InitialMallocHook_Sbrk, NULL);
}

DEFINE_ATTRIBUTE_SECTION_VARS(google_malloc);
DECLARE_ATTRIBUTE_SECTION_VARS(google_malloc);
  // actual functions are in debugallocation.cc or tcmalloc.cc
DEFINE_ATTRIBUTE_SECTION_VARS(malloc_hook);
DECLARE_ATTRIBUTE_SECTION_VARS(malloc_hook);
  // actual functions are in this file, malloc_hook.cc, and low_level_alloc.cc

#define ADDR_IN_ATTRIBUTE_SECTION(addr, name) \
  (reinterpret_cast<uintptr_t>(ATTRIBUTE_SECTION_START(name)) <= \
     reinterpret_cast<uintptr_t>(addr) && \
   reinterpret_cast<uintptr_t>(addr) < \
     reinterpret_cast<uintptr_t>(ATTRIBUTE_SECTION_STOP(name)))

// Return true iff 'caller' is a return address within a function
// that calls one of our hooks via MallocHook:Invoke*.
// A helper for GetCallerStackTrace.
static inline bool InHookCaller(const void* caller) {
  return ADDR_IN_ATTRIBUTE_SECTION(caller, google_malloc) ||
         ADDR_IN_ATTRIBUTE_SECTION(caller, malloc_hook);
  // We can use one section for everything except tcmalloc_or_debug
  // due to its special linkage mode, which prevents merging of the sections.
}

#undef ADDR_IN_ATTRIBUTE_SECTION

static bool checked_sections = false;

static inline void CheckInHookCaller() {
  if (!checked_sections) {
    INIT_ATTRIBUTE_SECTION_VARS(google_malloc);
    if (ATTRIBUTE_SECTION_START(google_malloc) ==
        ATTRIBUTE_SECTION_STOP(google_malloc)) {
      RAW_LOG(ERROR, "google_malloc section is missing, "
                     "thus InHookCaller is broken!");
    }
    INIT_ATTRIBUTE_SECTION_VARS(malloc_hook);
    if (ATTRIBUTE_SECTION_START(malloc_hook) ==
        ATTRIBUTE_SECTION_STOP(malloc_hook)) {
      RAW_LOG(ERROR, "malloc_hook section is missing, "
                     "thus InHookCaller is broken!");
    }
    checked_sections = true;
  }
}

// We can improve behavior/compactness of this function
// if we pass a generic test function (with a generic arg)
// into the implementations for GetStackTrace instead of the skip_count.
extern "C" int MallocHook_GetCallerStackTrace(void** result, int max_depth,
                                              int skip_count) {
#if defined(NO_TCMALLOC_SAMPLES)
  return 0;
#elif !defined(HAVE_ATTRIBUTE_SECTION_START)
  // Fall back to GetStackTrace and good old but fragile frame skip counts.
  // Note: this path is inaccurate when a hook is not called directly by an
  // allocation function but is daisy-chained through another hook,
  // search for MallocHook::(Get|Set|Invoke)* to find such cases.
  return GetStackTrace(result, max_depth, skip_count + int(DEBUG_MODE));
  // due to -foptimize-sibling-calls in opt mode
  // there's no need for extra frame skip here then
#else
  CheckInHookCaller();
  // MallocHook caller determination via InHookCaller works, use it:
  static const int kMaxSkip = 32 + 6 + 3;
    // Constant tuned to do just one GetStackTrace call below in practice
    // and not get many frames that we don't actually need:
    // currently max passsed max_depth is 32,
    // max passed/needed skip_count is 6
    // and 3 is to account for some hook daisy chaining.
  static const int kStackSize = kMaxSkip + 1;
  void* stack[kStackSize];
  int depth = GetStackTrace(stack, kStackSize, 1);  // skip this function frame
  if (depth == 0)   // silenty propagate cases when GetStackTrace does not work
    return 0;
  for (int i = 0; i < depth; ++i) {  // stack[0] is our immediate caller
    if (InHookCaller(stack[i])) {
      RAW_VLOG(10, "Found hooked allocator at %d: %p <- %p",
                   i, stack[i], stack[i+1]);
      i += 1;  // skip hook caller frame
      depth -= i;  // correct depth
      if (depth > max_depth) depth = max_depth;
      copy(stack + i, stack + i + depth, result);
      if (depth < max_depth  &&  depth + i == kStackSize) {
        // get frames for the missing depth
        depth +=
          GetStackTrace(result + depth, max_depth - depth, 1 + kStackSize);
      }
      return depth;
    }
  }
  RAW_LOG(WARNING, "Hooked allocator frame not found, returning empty trace");
    // If this happens try increasing kMaxSkip
    // or else something must be wrong with InHookCaller,
    // e.g. for every section used in InHookCaller
    // all functions in that section must be inside the same library.
  return 0;
#endif
}

// On Linux/x86, we override mmap/munmap/mremap/sbrk
// and provide support for calling the related hooks.
//
// We define mmap() and mmap64(), which somewhat reimplements libc's mmap
// syscall stubs.  Unfortunately libc only exports the stubs via weak symbols
// (which we're overriding with our mmap64() and mmap() wrappers) so we can't
// just call through to them.


#if defined(__linux) && \
    (defined(__i386__) || defined(__x86_64__) || defined(__PPC__))
#include <unistd.h>
#include <syscall.h>
#include <sys/mman.h>
#include <errno.h>
#include "base/linux_syscall_support.h"

// The x86-32 case and the x86-64 case differ:
// 32b has a mmap2() syscall, 64b does not.
// 64b and 32b have different calling conventions for mmap().
#if defined(__x86_64__) || defined(__PPC64__)

static inline void* do_mmap64(void *start, size_t length,
                              int prot, int flags,
                              int fd, __off64_t offset) __THROW {
  return (void *)syscall(SYS_mmap, start, length, prot, flags, fd, offset);
}

#elif defined(__i386__) || defined(__PPC__)

static inline void* do_mmap64(void *start, size_t length,
                              int prot, int flags,
                              int fd, __off64_t offset) __THROW {
  void *result;

  // Try mmap2() unless it's not supported
  static bool have_mmap2 = true;
  if (have_mmap2) {
    static int pagesize = 0;
    if (!pagesize) pagesize = getpagesize();

    // Check that the offset is page aligned
    if (offset & (pagesize - 1)) {
      result = MAP_FAILED;
      errno = EINVAL;
      goto out;
    }

    result = (void *)syscall(SYS_mmap2, 
                             start, length, prot, flags, fd, offset / pagesize);
    if (result != MAP_FAILED || errno != ENOSYS)  goto out;

    // We don't have mmap2() after all - don't bother trying it in future
    have_mmap2 = false;
  }

  if (((off_t)offset) != offset) {
    // If we're trying to map a 64-bit offset, fail now since we don't
    // have 64-bit mmap() support.
    result = MAP_FAILED;
    errno = EINVAL;
    goto out;
  }

  {
    // Fall back to old 32-bit offset mmap() call
    // Old syscall interface cannot handle six args, so pass in an array
    int32 args[6] = { (int32) start, length, prot, flags, fd, (off_t) offset };
    result = (void *)syscall(SYS_mmap, args);
  }
 out:
  return result;
}

# endif  // defined(__x86_64__)

// We use do_mmap64 abstraction to put MallocHook::InvokeMmapHook
// calls right into mmap and mmap64, so that the stack frames in the caller's
// stack are at the same offsets for all the calls of memory allocating
// functions.

// Put all callers of MallocHook::Invoke* in this module into
// malloc_hook section,
// so that MallocHook::GetCallerStackTrace can function accurately:

// Make sure mmap doesn't get #define'd away by <sys/mman.h>
#undef mmap

extern "C" {
  void* mmap64(void *start, size_t length, int prot, int flags,
               int fd, __off64_t offset  ) __THROW
    ATTRIBUTE_SECTION(malloc_hook);
  void* mmap(void *start, size_t length,int prot, int flags,
             int fd, off_t offset) __THROW
    ATTRIBUTE_SECTION(malloc_hook);
  int munmap(void* start, size_t length) __THROW
    ATTRIBUTE_SECTION(malloc_hook);
  void* mremap(void* old_addr, size_t old_size, size_t new_size,
               int flags, ...) __THROW
    ATTRIBUTE_SECTION(malloc_hook);
  void* sbrk(ptrdiff_t increment) __THROW
    ATTRIBUTE_SECTION(malloc_hook);
}

extern "C" void* mmap64(void *start, size_t length, int prot, int flags,
                        int fd, __off64_t offset) __THROW {
  MallocHook::InvokePreMmapHook(start, length, prot, flags, fd, offset);
  void *result = do_mmap64(start, length, prot, flags, fd, offset);
  MallocHook::InvokeMmapHook(result, start, length, prot, flags, fd, offset);
  return result;
}

#if !defined(__USE_FILE_OFFSET64) || !defined(__REDIRECT_NTH)

extern "C" void* mmap(void *start, size_t length, int prot, int flags,
                      int fd, off_t offset) __THROW {
  MallocHook::InvokePreMmapHook(start, length, prot, flags, fd, offset);
  void *result = do_mmap64(start, length, prot, flags, fd,
                           static_cast<size_t>(offset)); // avoid sign extension
  MallocHook::InvokeMmapHook(result, start, length, prot, flags, fd, offset);
  return result;
}

#endif  // !defined(__USE_FILE_OFFSET64) || !defined(__REDIRECT_NTH)

extern "C" int munmap(void* start, size_t length) __THROW {
  MallocHook::InvokeMunmapHook(start, length);
  return syscall(SYS_munmap, start, length);
}

extern "C" void* mremap(void* old_addr, size_t old_size, size_t new_size,
                        int flags, ...) __THROW {
  va_list ap;
  va_start(ap, flags);
  void *new_address = va_arg(ap, void *);
  va_end(ap);
  void* result = sys_mremap(old_addr, old_size, new_size, flags, new_address);
  MallocHook::InvokeMremapHook(result, old_addr, old_size, new_size, flags,
                               new_address);
  return result;
}

// libc's version:
extern "C" void* __sbrk(ptrdiff_t increment);

extern "C" void* sbrk(ptrdiff_t increment) __THROW {
  MallocHook::InvokePreSbrkHook(increment);
  void *result = __sbrk(increment);
  MallocHook::InvokeSbrkHook(result, increment);
  return result;
}

/*static*/void* MallocHook::UnhookedMMap(void *start, size_t length, int prot,
                                         int flags, int fd, off_t offset) {
  return do_mmap64(start, length, prot, flags, fd, offset);
}

/*static*/int MallocHook::UnhookedMUnmap(void *start, size_t length) {
  return sys_munmap(start, length);
}

#else   // defined(__linux) &&
        // (defined(__i386__) || defined(__x86_64__) || defined(__PPC__))

/*static*/void* MallocHook::UnhookedMMap(void *start, size_t length, int prot,
                                         int flags, int fd, off_t offset) {
  return mmap(start, length, prot, flags, fd, offset);
}

/*static*/int MallocHook::UnhookedMUnmap(void *start, size_t length) {
  return munmap(start, length);
}

#endif  // defined(__linux) &&
        // (defined(__i386__) || defined(__x86_64__) || defined(__PPC__))
