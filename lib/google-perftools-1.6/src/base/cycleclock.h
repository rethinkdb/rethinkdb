// Copyright (c) 2004, Google Inc.
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

// ----------------------------------------------------------------------
// CycleClock
//    A CycleClock tells you the current time in Cycles.  The "time"
//    is actually time since power-on.  This is like time() but doesn't
//    involve a system call and is much more precise.
//
// NOTE: Not all cpu/platform/kernel combinations guarantee that this
// clock increments at a constant rate or is synchronized across all logical
// cpus in a system.
//
// Also, in some out of order CPU implementations, the CycleClock is not 
// serializing. So if you're trying to count at cycles granularity, your
// data might be inaccurate due to out of order instruction execution.
// ----------------------------------------------------------------------

#ifndef GOOGLE_BASE_CYCLECLOCK_H_
#define GOOGLE_BASE_CYCLECLOCK_H_

#include "base/basictypes.h"   // make sure we get the def for int64
#if defined(__MACH__) && defined(__APPLE__)
#include <mach/mach_time.h>
#elif defined(__ARM_ARCH_5T__)
#include <sys/time.h>
#endif

// NOTE: only i386 and x86_64 have been well tested.
// PPC, sparc, alpha, and ia64 are based on
//    http://peter.kuscsik.com/wordpress/?p=14
// with modifications by m3b.  cf
//    https://setisvn.ssl.berkeley.edu/svn/lib/fftw-3.0.1/kernel/cycle.h
struct CycleClock {
  // This should return the number of cycles since power-on.  Thread-safe.
  static inline int64 Now() {
#if defined(__MACH__) && defined(__APPLE__)
    // this goes at the top because we need ALL Macs, regardless
    // of architecture, to return the number of "mach time units"
    // that have passes since startup. See sysinfo.cc where
    // InitializeSystemInfo() sets the supposed cpu clock frequency of macs
    // to the number of mach time units per second, not actual
    // CPU clock frequency (which can change in the face of CPU
    // frequency scaling).  also note that when the Mac sleeps,
    // this counter pauses; it does not continue counting, nor resets
    // to zero.
    return mach_absolute_time();
#elif defined(__i386__)
    int64 ret;
    __asm__ volatile ("rdtsc" : "=A" (ret) );
    return ret;
#elif defined(__x86_64__) || defined(__amd64__)
    uint64 low, high;
    __asm__ volatile ("rdtsc" : "=a" (low), "=d" (high));
    return (high << 32) | low;
#elif defined(__powerpc__) || defined(__ppc__)
    // This returns a time-base, which is not always precisely a cycle-count.
    int64 tbl, tbu0, tbu1;
    asm("mftbu %0" : "=r" (tbu0));
    asm("mftb  %0" : "=r" (tbl));
    asm("mftbu %0" : "=r" (tbu1));
    tbl &= -static_cast<int64>(tbu0 == tbu1);
    // high 32 bits in tbu1; low 32 bits in tbl  (tbu0 is garbage)
    return (tbu1 << 32) | tbl;
#elif defined(__ARM_ARCH_5T__)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return static_cast<uint64>(tv.tv_sec) * 1000000 + tv.tv_usec;
#elif defined(__sparc__)
    int64 tick;
    asm(".byte 0x83, 0x41, 0x00, 0x00");
    asm("mov   %%g1, %0" : "=r" (tick));
    return tick;
#elif defined(__ia64__)
    int64 itc;
    asm("mov %0 = ar.itc" : "=r" (itc));
    return itc;
#elif defined(_MSC_VER) && defined(_M_IX86)
    _asm rdtsc
#else
    // We could define __alpha here as well, but it only has a 32-bit
    // timer (good for like 4 seconds), which isn't very useful.
#error You need to define CycleTimer for your O/S and CPU
#endif
  }
};


#endif  // GOOGLE_BASE_CYCLECLOCK_H_
