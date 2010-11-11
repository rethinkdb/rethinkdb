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
 * Author: Craig Silverstein
 *
 * These are some portability typedefs and defines to make it a bit
 * easier to compile this code under VC++.
 *
 * Several of these are taken from glib:
 *    http://developer.gnome.org/doc/API/glib/glib-windows-compatability-functions.html
 */

#ifndef GOOGLE_BASE_WINDOWS_H_
#define GOOGLE_BASE_WINDOWS_H_

// You should never include this file directly, but always include it
// from either config.h (MSVC) or mingw.h (MinGW/msys).
#if !defined(GOOGLE_PERFTOOLS_WINDOWS_CONFIG_H_) && \
    !defined(GOOGLE_PERFTOOLS_WINDOWS_MINGW_H_)
# error "port.h should only be included from config.h or mingw.h"
#endif

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN  /* We always want minimal includes */
#endif
#include <windows.h>
#include <io.h>              /* because we so often use open/close/etc */
#include <process.h>         /* for _getpid */
#include <stdarg.h>          /* for va_list */
#include <stdio.h>           /* need this to override stdio's (v)snprintf */

// 4018: signed/unsigned mismatch is common (and ok for signed_i < unsigned_i)
// 4244: otherwise we get problems when substracting two size_t's to an int
// 4288: VC++7 gets confused when a var is defined in a loop and then after it
// 4267: too many false positives for "conversion gives possible data loss"
// 4290: it's ok windows ignores the "throw" directive
// 4996: Yes, we're ok using "unsafe" functions like vsnprintf and getenv()
#ifdef _MSC_VER
#pragma warning(disable:4018 4244 4288 4267 4290 4996)
#endif

// ----------------------------------- BASIC TYPES

#ifndef HAVE_STDINT_H
#ifndef HAVE___INT64    /* we need to have all the __intX names */
# error  Do not know how to set up type aliases.  Edit port.h for your system.
#endif

typedef __int8 int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#endif  // #ifndef HAVE_STDINT_H

// I guess MSVC's <types.h> doesn't include ssize_t by default?
#ifdef _MSC_VER
typedef intptr_t ssize_t;
#endif

// ----------------------------------- THREADS

#ifndef HAVE_PTHREAD   // not true for MSVC, but may be true for MSYS
typedef DWORD pthread_t;
typedef DWORD pthread_key_t;
typedef LONG pthread_once_t;
enum { PTHREAD_ONCE_INIT = 0 };   // important that this be 0! for SpinLock
#define pthread_self  GetCurrentThreadId
#define pthread_equal(pthread_t_1, pthread_t_2)  ((pthread_t_1)==(pthread_t_2))

#ifdef __cplusplus
// This replaces maybe_threads.{h,cc}
extern pthread_key_t PthreadKeyCreate(void (*destr_fn)(void*));  // in port.cc
#define perftools_pthread_key_create(pkey, destr_fn)  \
  *(pkey) = PthreadKeyCreate(destr_fn)
inline void* perftools_pthread_getspecific(DWORD key) {
  DWORD err = GetLastError();
  void* rv = TlsGetValue(key);
  if (err) SetLastError(err);
  return rv;
}
#define perftools_pthread_setspecific(key, val) \
  TlsSetValue((key), (val))
// NOTE: this is Win2K and later.  For Win98 we could use a CRITICAL_SECTION...
#define perftools_pthread_once(once, init)  do {                \
  if (InterlockedCompareExchange(once, 1, 0) == 0) (init)();    \
} while (0)
#endif  // __cplusplus
#endif  // HAVE_PTHREAD

// __declspec(thread) isn't usable in a dll opened via LoadLibrary().
// But it doesn't work to LoadLibrary() us anyway, because of all the
// things we need to do before main()!  So this kind of TLS is safe for us.
#define __thread __declspec(thread)

// This code is obsolete, but I keep it around in case we are ever in
// an environment where we can't or don't want to use google spinlocks
// (from base/spinlock.{h,cc}).  In that case, uncommenting this out,
// and removing spinlock.cc from the build, should be enough to revert
// back to using native spinlocks.
#if 0
// Windows uses a spinlock internally for its mutexes, making our life easy!
// However, the Windows spinlock must always be initialized, making life hard,
// since we want LINKER_INITIALIZED.  We work around this by having the
// linker initialize a bool to 0, and check that before accessing the mutex.
// This replaces spinlock.{h,cc}, and all the stuff it depends on (atomicops)
#ifdef __cplusplus
class SpinLock {
 public:
  SpinLock() : initialize_token_(PTHREAD_ONCE_INIT) {}
  // Used for global SpinLock vars (see base/spinlock.h for more details).
  enum StaticInitializer { LINKER_INITIALIZED };
  explicit SpinLock(StaticInitializer) : initialize_token_(PTHREAD_ONCE_INIT) {
    perftools_pthread_once(&initialize_token_, InitializeMutex);
  }

  // It's important SpinLock not have a destructor: otherwise we run
  // into problems when the main thread has exited, but other threads
  // are still running and try to access a main-thread spinlock.  This
  // means we leak mutex_ (we should call DeleteCriticalSection()
  // here).  However, I've verified that all SpinLocks used in
  // perftools have program-long scope anyway, so the leak is
  // perfectly fine.  But be aware of this for the future!

  void Lock() {
    // You'd thionk this would be unnecessary, since we call
    // InitializeMutex() in our constructor.  But sometimes Lock() can
    // be called before our constructor is!  This can only happen in
    // global constructors, when this is a global.  If we live in
    // bar.cc, and some global constructor in foo.cc calls a routine
    // in bar.cc that calls this->Lock(), then Lock() may well run
    // before our global constructor does.  To protect against that,
    // we do this check.  For SpinLock objects created after main()
    // has started, this pthread_once call will always be a noop.
    perftools_pthread_once(&initialize_token_, InitializeMutex);
    EnterCriticalSection(&mutex_);
  }
  void Unlock() {
    LeaveCriticalSection(&mutex_);
  }

  // Used in assertion checks: assert(lock.IsHeld()) (see base/spinlock.h).
  inline bool IsHeld() const {
    // This works, but probes undocumented internals, so I've commented it out.
    // c.f. http://msdn.microsoft.com/msdnmag/issues/03/12/CriticalSections/
    //return mutex_.LockCount>=0 && mutex_.OwningThread==GetCurrentThreadId();
    return true;
  }
 private:
  void InitializeMutex() { InitializeCriticalSection(&mutex_); }

  pthread_once_t initialize_token_;
  CRITICAL_SECTION mutex_;
};

class SpinLockHolder {  // Acquires a spinlock for as long as the scope lasts
 private:
  SpinLock* lock_;
 public:
  inline explicit SpinLockHolder(SpinLock* l) : lock_(l) { l->Lock(); }
  inline ~SpinLockHolder() { lock_->Unlock(); }
};
#endif  // #ifdef __cplusplus

// This keeps us from using base/spinlock.h's implementation of SpinLock.
#define BASE_SPINLOCK_H_ 1

#endif  // #if 0

// This replaces testutil.{h,cc}
extern PERFTOOLS_DLL_DECL void RunInThread(void (*fn)());
extern PERFTOOLS_DLL_DECL void RunManyInThread(void (*fn)(), int count);
extern PERFTOOLS_DLL_DECL void RunManyInThreadWithId(void (*fn)(int), int count,
                                                     int stacksize);


// ----------------------------------- MMAP and other memory allocation

#ifndef HAVE_MMAP   // not true for MSVC, but may be true for msys
#define MAP_FAILED  0
#define MREMAP_FIXED  2  // the value in linux, though it doesn't really matter
// These, when combined with the mmap invariants below, yield the proper action
#define PROT_READ      PAGE_READWRITE
#define PROT_WRITE     PAGE_READWRITE
#define MAP_ANONYMOUS  MEM_RESERVE
#define MAP_PRIVATE    MEM_COMMIT
#define MAP_SHARED     MEM_RESERVE   // value of this #define is 100% arbitrary

// VirtualAlloc is only a replacement for mmap when certain invariants are kept
#define mmap(start, length, prot, flags, fd, offset)                          \
  ( (start) == NULL && (fd) == -1 && (offset) == 0 &&                         \
    (prot) == (PROT_READ|PROT_WRITE) && (flags) == (MAP_PRIVATE|MAP_ANONYMOUS)\
      ? VirtualAlloc(0, length, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)     \
      : NULL )

#define munmap(start, length)   (VirtualFree(start, 0, MEM_RELEASE) ? 0 : -1)
#endif  // HAVE_MMAP

// We could maybe use VirtualAlloc for sbrk as well, but no need
#define sbrk(increment)  ( (void*)-1 )   // sbrk returns -1 on failure


// ----------------------------------- STRING ROUTINES

// We can't just use _vsnprintf and _snprintf as drop-in-replacements,
// because they don't always NUL-terminate. :-(  We also can't use the
// name vsnprintf, since windows defines that (but not snprintf (!)).
extern PERFTOOLS_DLL_DECL int snprintf(char *str, size_t size,
                                       const char *format, ...);
extern PERFTOOLS_DLL_DECL int safe_vsnprintf(char *str, size_t size,
                                             const char *format, va_list ap);
#define vsnprintf(str, size, format, ap)  safe_vsnprintf(str, size, format, ap)

#define PRIx64  "I64x"
#define SCNx64  "I64x"
#define PRId64  "I64d"
#define SCNd64  "I64d"
#define PRIu64  "I64u"
#ifdef _WIN64
# define PRIuPTR "llu"
# define PRIxPTR "llx"
#else
# define PRIuPTR "lu"
# define PRIxPTR "lx"
#endif

// ----------------------------------- FILE IO
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif
#ifndef __MINGW32__
enum { STDIN_FILENO = 0, STDOUT_FILENO = 1, STDERR_FILENO = 2 };
#endif
#define getcwd    _getcwd
#define access    _access
#define open      _open
#define read      _read
#define write     _write
#define lseek     _lseek
#define close     _close
#define popen     _popen
#define pclose    _pclose
#define mkdir(dirname, mode)  _mkdir(dirname)
#ifndef O_RDONLY
#define O_RDONLY  _O_RDONLY
#endif

// ----------------------------------- SYSTEM/PROCESS
typedef int pid_t;
#define getpid  _getpid
#define getppid() (0)

// Handle case when poll is used to simulate sleep.
#define poll(r, w, t) \
  do {                \
    assert(r == 0);   \
    assert(w == 0);   \
    Sleep(t);         \
  } while(0)

extern PERFTOOLS_DLL_DECL int getpagesize();   // in port.cc

// ----------------------------------- OTHER

#define srandom  srand
#define random   rand
#define sleep(t) Sleep(t * 1000)

struct timespec {
  int tv_sec;
  int tv_nsec;
};

#define nanosleep(tm_ptr, ignored)  \
  Sleep((tm_ptr)->tv_sec * 1000 + (tm_ptr)->tv_nsec / 1000000)

#ifndef __MINGW32__
#define strtoq   _strtoi64
#define strtouq  _strtoui64
#define strtoll  _strtoi64
#define strtoull _strtoui64
#define atoll    _atoi64
#endif

#define __THROW throw()

// ----------------------------------- TCMALLOC-SPECIFIC

// tcmalloc.cc calls this so we can patch VirtualAlloc() et al.
extern PERFTOOLS_DLL_DECL void PatchWindowsFunctions();

// ----------------------------------- BUILD-SPECIFIC

// windows/port.h defines compatibility APIs for several .h files, which
// we therefore shouldn't be #including directly.  This hack keeps us from
// doing so.  TODO(csilvers): do something more principled.
#define GOOGLE_MAYBE_THREADS_H_ 1


#endif  /* _WIN32 */

#endif  /* GOOGLE_BASE_WINDOWS_H_ */
