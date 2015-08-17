// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_BASE_PLATFORM_CONDITION_VARIABLE_H_
#define V8_BASE_PLATFORM_CONDITION_VARIABLE_H_

#include "src/base/lazy-instance.h"
#include "src/base/platform/mutex.h"

namespace v8 {
namespace base {

// Forward declarations.
class ConditionVariableEvent;
class TimeDelta;

// -----------------------------------------------------------------------------
// ConditionVariable
//
// This class is a synchronization primitive that can be used to block a thread,
// or multiple threads at the same time, until:
// - a notification is received from another thread,
// - a timeout expires, or
// - a spurious wakeup occurs
// Any thread that intends to wait on a ConditionVariable has to acquire a lock
// on a Mutex first. The |Wait()| and |WaitFor()| operations atomically release
// the mutex and suspend the execution of the calling thread. When the condition
// variable is notified, the thread is awakened, and the mutex is reacquired.

class ConditionVariable FINAL {
 public:
  ConditionVariable();
  ~ConditionVariable();

  // If any threads are waiting on this condition variable, calling
  // |NotifyOne()| unblocks one of the waiting threads.
  void NotifyOne();

  // Unblocks all threads currently waiting for this condition variable.
  void NotifyAll();

  // |Wait()| causes the calling thread to block until the condition variable is
  // notified or a spurious wakeup occurs. Atomically releases the mutex, blocks
  // the current executing thread, and adds it to the list of threads waiting on
  // this condition variable. The thread will be unblocked when |NotifyAll()| or
  // |NotifyOne()| is executed. It may also be unblocked spuriously. When
  // unblocked, regardless of the reason, the lock on the mutex is reacquired
  // and |Wait()| exits.
  void Wait(Mutex* mutex);

  // Atomically releases the mutex, blocks the current executing thread, and
  // adds it to the list of threads waiting on this condition variable. The
  // thread will be unblocked when |NotifyAll()| or |NotifyOne()| is executed,
  // or when the relative timeout |rel_time| expires. It may also be unblocked
  // spuriously. When unblocked, regardless of the reason, the lock on the mutex
  // is reacquired and |WaitFor()| exits. Returns true if the condition variable
  // was notified prior to the timeout.
  bool WaitFor(Mutex* mutex, const TimeDelta& rel_time) WARN_UNUSED_RESULT;

  // The implementation-defined native handle type.
#if V8_OS_POSIX
  typedef pthread_cond_t NativeHandle;
#elif V8_OS_WIN
  struct Event;
  class NativeHandle FINAL {
   public:
    NativeHandle() : waitlist_(NULL), freelist_(NULL) {}
    ~NativeHandle();

    Event* Pre() WARN_UNUSED_RESULT;
    void Post(Event* event, bool result);

    Mutex* mutex() { return &mutex_; }
    Event* waitlist() { return waitlist_; }

   private:
    Event* waitlist_;
    Event* freelist_;
    Mutex mutex_;

    DISALLOW_COPY_AND_ASSIGN(NativeHandle);
  };
#endif

  NativeHandle& native_handle() {
    return native_handle_;
  }
  const NativeHandle& native_handle() const {
    return native_handle_;
  }

 private:
  NativeHandle native_handle_;

  DISALLOW_COPY_AND_ASSIGN(ConditionVariable);
};


// POD ConditionVariable initialized lazily (i.e. the first time Pointer() is
// called).
// Usage:
//   static LazyConditionVariable my_condvar =
//       LAZY_CONDITION_VARIABLE_INITIALIZER;
//
//   void my_function() {
//     LockGuard<Mutex> lock_guard(&my_mutex);
//     my_condvar.Pointer()->Wait(&my_mutex);
//   }
typedef LazyStaticInstance<
    ConditionVariable, DefaultConstructTrait<ConditionVariable>,
    ThreadSafeInitOnceTrait>::type LazyConditionVariable;

#define LAZY_CONDITION_VARIABLE_INITIALIZER LAZY_STATIC_INSTANCE_INITIALIZER

} }  // namespace v8::base

#endif  // V8_BASE_PLATFORM_CONDITION_VARIABLE_H_
