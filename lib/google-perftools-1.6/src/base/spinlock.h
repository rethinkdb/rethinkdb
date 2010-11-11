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

//
// Fast spinlocks (at least on x86, a lock/unlock pair is approximately
// half the cost of a Mutex because the unlock just does a store instead
// of a compare-and-swap which is expensive).

// SpinLock is async signal safe.
// If used within a signal handler, all lock holders
// should block the signal even outside the signal handler.

#ifndef BASE_SPINLOCK_H_
#define BASE_SPINLOCK_H_

#include <config.h>
#include "base/basictypes.h"
#include "base/atomicops.h"
#include "base/dynamic_annotations.h"
#include "base/thread_annotations.h"

class LOCKABLE SpinLock {
 public:
  SpinLock() : lockword_(0) { }

  // Special constructor for use with static SpinLock objects.  E.g.,
  //
  //    static SpinLock lock(base::LINKER_INITIALIZED);
  //
  // When intialized using this constructor, we depend on the fact
  // that the linker has already initialized the memory appropriately.
  // A SpinLock constructed like this can be freely used from global
  // initializers without worrying about the order in which global
  // initializers run.
  explicit SpinLock(base::LinkerInitialized /*x*/) {
    // Does nothing; lockword_ is already initialized
  }

  // Acquire this SpinLock.
  // TODO(csilvers): uncomment the annotation when we figure out how to
  //                 support this macro with 0 args (see thread_annotations.h)
  inline void Lock() /*EXCLUSIVE_LOCK_FUNCTION()*/ {
    if (Acquire_CompareAndSwap(&lockword_, 0, 1) != 0) {
      SlowLock();
    }
    ANNOTATE_RWLOCK_ACQUIRED(this, 1);
  }

  // Acquire this SpinLock and return true if the acquisition can be
  // done without blocking, else return false.  If this SpinLock is
  // free at the time of the call, TryLock will return true with high
  // probability.
  inline bool TryLock() EXCLUSIVE_TRYLOCK_FUNCTION(true) {
    bool res = (Acquire_CompareAndSwap(&lockword_, 0, 1) == 0);
    if (res) {
      ANNOTATE_RWLOCK_ACQUIRED(this, 1);
    }
    return res;
  }

  // Release this SpinLock, which must be held by the calling thread.
  // TODO(csilvers): uncomment the annotation when we figure out how to
  //                 support this macro with 0 args (see thread_annotations.h)
  inline void Unlock() /*UNLOCK_FUNCTION()*/ {
    // This is defined in mutex.cc.
    extern void SubmitSpinLockProfileData(const void *, int64);

    int64 wait_timestamp = static_cast<uint32>(lockword_);
    ANNOTATE_RWLOCK_RELEASED(this, 1);
    Release_Store(&lockword_, 0);
    if (wait_timestamp != 1) {
      // Collect contentionz profile info, and speed the wakeup of any waiter.
      // The lockword_ value indicates when the waiter started waiting.
      SlowUnlock(wait_timestamp);
    }
  }

  // Report if we think the lock can be held by this thread.
  // When the lock is truly held by the invoking thread
  // we will always return true.
  // Indended to be used as CHECK(lock.IsHeld());
  inline bool IsHeld() const {
    return lockword_ != 0;
  }

  // The timestamp for contention lock profiling must fit into 31 bits.
  // as lockword_ is 32 bits and we loose an additional low-order bit due
  // to the statement "now |= 1" in SlowLock().
  // To select 31 bits from the 64-bit cycle counter, we shift right by
  // PROFILE_TIMESTAMP_SHIFT = 7.
  // Using these 31 bits, we reduce granularity of time measurement to
  // 256 cycles, and will loose track of wait time for waits greater than
  // 109 seconds on a 5 GHz machine, longer for faster clock cycles.
  // Waits this long should be very rare.
  enum { PROFILE_TIMESTAMP_SHIFT = 7 };

  static const base::LinkerInitialized LINKER_INITIALIZED;  // backwards compat
 private:
  // Lock-state:  0 means unlocked; 1 means locked with no waiters; values
  // greater than 1 indicate locked with waiters, where the value is the time
  // the first waiter started waiting and is used for contention profiling.
  volatile Atomic32 lockword_;

  void SlowLock();
  void SlowUnlock(int64 wait_timestamp);

  DISALLOW_COPY_AND_ASSIGN(SpinLock);
};

// Corresponding locker object that arranges to acquire a spinlock for
// the duration of a C++ scope.
class SCOPED_LOCKABLE SpinLockHolder {
 private:
  SpinLock* lock_;
 public:
  inline explicit SpinLockHolder(SpinLock* l) EXCLUSIVE_LOCK_FUNCTION(l)
      : lock_(l) {
    l->Lock();
  }
  // TODO(csilvers): uncomment the annotation when we figure out how to
  //                 support this macro with 0 args (see thread_annotations.h)
  inline ~SpinLockHolder() /*UNLOCK_FUNCTION()*/ { lock_->Unlock(); }
};
// Catch bug where variable name is omitted, e.g. SpinLockHolder (&lock);
#define SpinLockHolder(x) COMPILE_ASSERT(0, spin_lock_decl_missing_var_name)


#endif  // BASE_SPINLOCK_H_
