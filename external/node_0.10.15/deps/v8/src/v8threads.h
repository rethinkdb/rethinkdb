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

#ifndef V8_V8THREADS_H_
#define V8_V8THREADS_H_

namespace v8 {
namespace internal {


class ThreadState {
 public:
  // Returns NULL after the last one.
  ThreadState* Next();

  enum List {FREE_LIST, IN_USE_LIST};

  void LinkInto(List list);
  void Unlink();

  // Id of thread.
  void set_id(ThreadId id) { id_ = id; }
  ThreadId id() { return id_; }

  // Should the thread be terminated when it is restored?
  bool terminate_on_restore() { return terminate_on_restore_; }
  void set_terminate_on_restore(bool terminate_on_restore) {
    terminate_on_restore_ = terminate_on_restore;
  }

  // Get data area for archiving a thread.
  char* data() { return data_; }

 private:
  explicit ThreadState(ThreadManager* thread_manager);
  ~ThreadState();

  void AllocateSpace();

  ThreadId id_;
  bool terminate_on_restore_;
  char* data_;
  ThreadState* next_;
  ThreadState* previous_;

  ThreadManager* thread_manager_;

  friend class ThreadManager;
};


// Defined in isolate.h.
class ThreadLocalTop;


class ThreadVisitor {
 public:
  // ThreadLocalTop may be only available during this call.
  virtual void VisitThread(Isolate* isolate, ThreadLocalTop* top) = 0;

 protected:
  virtual ~ThreadVisitor() {}
};


class ThreadManager {
 public:
  void Lock();
  void Unlock();

  void ArchiveThread();
  bool RestoreThread();
  void FreeThreadResources();
  bool IsArchived();

  void Iterate(ObjectVisitor* v);
  void IterateArchivedThreads(ThreadVisitor* v);
  bool IsLockedByCurrentThread() {
    return mutex_owner_.Equals(ThreadId::Current());
  }

  ThreadId CurrentId();

  void TerminateExecution(ThreadId thread_id);

  // Iterate over in-use states.
  ThreadState* FirstThreadStateInUse();
  ThreadState* GetFreeThreadState();

 private:
  ThreadManager();
  ~ThreadManager();

  void DeleteThreadStateList(ThreadState* anchor);

  void EagerlyArchiveThread();

  Mutex* mutex_;
  ThreadId mutex_owner_;
  ThreadId lazily_archived_thread_;
  ThreadState* lazily_archived_thread_state_;

  // In the following two lists there is always at least one object on the list.
  // The first object is a flying anchor that is only there to simplify linking
  // and unlinking.
  // Head of linked list of free states.
  ThreadState* free_anchor_;
  // Head of linked list of states in use.
  ThreadState* in_use_anchor_;

  Isolate* isolate_;

  friend class Isolate;
  friend class ThreadState;
};


// The ContextSwitcher thread is used to schedule regular preemptions to
// multiple running V8 threads. Generally it is necessary to call
// StartPreemption if there is more than one thread running. If not, a single
// JavaScript can take full control of V8 and not allow other threads to run.
class ContextSwitcher: public Thread {
 public:
  // Set the preemption interval for the ContextSwitcher thread.
  static void StartPreemption(int every_n_ms);

  // Stop sending preemption requests to threads.
  static void StopPreemption();

  // Preempted thread needs to call back to the ContextSwitcher to acknowledge
  // the handling of a preemption request.
  static void PreemptionReceived();

 private:
  ContextSwitcher(Isolate* isolate, int every_n_ms);

  Isolate* isolate() const { return isolate_; }

  void Run();

  bool keep_going_;
  int sleep_ms_;
  Isolate* isolate_;
};

} }  // namespace v8::internal

#endif  // V8_V8THREADS_H_
