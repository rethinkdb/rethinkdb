/* Copyright (c) 2006, Google Inc.
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
 * Author: Sanjay Ghemawat
 */

#include <config.h>
#include <time.h>       /* For nanosleep() */
#ifdef HAVE_SCHED_H
#include <sched.h>      /* For sched_yield() */
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>     /* For read() */
#endif
#include <fcntl.h>      /* for open(), O_RDONLY */
#include <string.h>     /* for strncmp */
#include <errno.h>
#include "base/spinlock.h"
#include "base/cycleclock.h"
#include "base/sysinfo.h"   /* for NumCPUs() */

// We can do contention-profiling of SpinLocks, but the code is in
// mutex.cc, which is not always linked in with spinlock.  Hence we
// provide this weak definition, which is used if mutex.cc isn't linked in.
ATTRIBUTE_WEAK extern void SubmitSpinLockProfileData(const void *, int64);
void SubmitSpinLockProfileData(const void *, int64) {}

static int adaptive_spin_count = 0;

const base::LinkerInitialized SpinLock::LINKER_INITIALIZED =
    base::LINKER_INITIALIZED;

// The OS-specific header included below must provide two calls:
// Wait until *w becomes zero, atomically set it to 1 and return.
//    static void SpinLockWait(volatile Atomic32 *w);
// 
// Hint that a thread waiting in SpinLockWait() could now make progress.  May
// do nothing.  This call may not read or write *w; it must use only the
// address.
//    static void SpinLockWake(volatile Atomic32 *w);
#if defined(_WIN32)
#include "base/spinlock_win32-inl.h"
#elif defined(__linux__)
#include "base/spinlock_linux-inl.h"
#else
#include "base/spinlock_posix-inl.h"
#endif

namespace {
struct SpinLock_InitHelper {
  SpinLock_InitHelper() {
    // On multi-cpu machines, spin for longer before yielding
    // the processor or sleeping.  Reduces idle time significantly.
    if (NumCPUs() > 1) {
      adaptive_spin_count = 1000;
    }
  }
};

// Hook into global constructor execution:
// We do not do adaptive spinning before that,
// but nothing lock-intensive should be going on at that time.
static SpinLock_InitHelper init_helper;

}  // unnamed namespace


void SpinLock::SlowLock() {
  int c = adaptive_spin_count;

  // Spin a few times in the hope that the lock holder releases the lock
  while ((c > 0) && (lockword_ != 0)) {
    c--;
  }

  if (lockword_ == 1) {
    int32 now = (CycleClock::Now() >> PROFILE_TIMESTAMP_SHIFT);
    // Don't loose the lock: make absolutely sure "now" is not zero
    now |= 1;
    // Atomically replace the value of lockword_ with "now" if
    // lockword_ is 1, thereby remembering the first timestamp to
    // be recorded.
    base::subtle::NoBarrier_CompareAndSwap(&lockword_, 1, now);
    // base::subtle::NoBarrier_CompareAndSwap() returns:
    //   0: the lock is/was available; nothing stored
    //   1: our timestamp was stored
    //   > 1: an older timestamp is already in lockword_; nothing stored
  }

  SpinLockWait(&lockword_);  // wait until lock acquired; OS specific
}

void SpinLock::SlowUnlock(int64 wait_timestamp) {
  SpinLockWake(&lockword_);  // wake waiter if necessary; OS specific 

  // Collect contentionz profile info.  Subtract one from wait_timestamp as
  // antidote to "now |= 1;" in SlowLock().
  SubmitSpinLockProfileData(this, wait_timestamp - 1);
}
