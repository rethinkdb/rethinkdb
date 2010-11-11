// Copyright (c) 2009, Google Inc.
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
//         Nabeel Mian
//
// Implements management of profile timers and the corresponding signal handler.

#include "config.h"
#include "profile-handler.h"

#if !(defined(__CYGWIN__) || defined(__CYGWIN32__))

#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

#include <list>
#include <string>

#include "base/dynamic_annotations.h"
#include "base/logging.h"
#include "base/spinlock.h"
#include "maybe_threads.h"

using std::list;
using std::string;

// This structure is used by ProfileHandlerRegisterCallback and
// ProfileHandlerUnregisterCallback as a handle to a registered callback.
struct ProfileHandlerToken {
  // Sets the callback and associated arg.
  ProfileHandlerToken(ProfileHandlerCallback cb, void* cb_arg)
      : callback(cb),
        callback_arg(cb_arg) {
  }

  // Callback function to be invoked on receiving a profile timer interrupt.
  ProfileHandlerCallback callback;
  // Argument for the callback function.
  void* callback_arg;
};

// This class manages profile timers and associated signal handler. This is a
// a singleton.
class ProfileHandler {
 public:
  // Registers the current thread with the profile handler. On systems which
  // have a separate interval timer for each thread, this function starts the
  // timer for the current thread.
  //
  // The function also attempts to determine whether or not timers are shared by
  // all threads in the process.  (With LinuxThreads, and with NPTL on some
  // Linux kernel versions, each thread has separate timers.)
  //
  // Prior to determining whether timers are shared, this function will
  // unconditionally start the timer.  However, if this function determines
  // that timers are shared, then it will stop the timer if no callbacks are
  // currently registered.
  void RegisterThread();

  // Registers a callback routine to receive profile timer ticks. The returned
  // token is to be used when unregistering this callback and must not be
  // deleted by the caller. Registration of the first callback enables the
  // SIGPROF handler (or SIGALRM if using ITIMER_REAL).
  ProfileHandlerToken* RegisterCallback(ProfileHandlerCallback callback,
                                        void* callback_arg);

  // Unregisters a previously registered callback. Expects the token returned
  // by the corresponding RegisterCallback routine. Unregistering the last
  // callback disables the SIGPROF handler (or SIGALRM if using ITIMER_REAL).
  void UnregisterCallback(ProfileHandlerToken* token)
      NO_THREAD_SAFETY_ANALYSIS;

  // Unregisters all the callbacks, stops the timer if shared, disables the
  // SIGPROF (or SIGALRM) handler and clears the timer_sharing_ state.
  void Reset();

  // Gets the current state of profile handler.
  void GetState(ProfileHandlerState* state);

  // Initializes and returns the ProfileHandler singleton.
  static ProfileHandler* Instance();

 private:
  ProfileHandler();
  ~ProfileHandler();

  // Largest allowed frequency.
  static const int32 kMaxFrequency = 4000;
  // Default frequency.
  static const int32 kDefaultFrequency = 100;

  // ProfileHandler singleton.
  static ProfileHandler* instance_;

  // pthread_once_t for one time initialization of ProfileHandler singleton.
  static pthread_once_t once_;

  // Initializes the ProfileHandler singleton via GoogleOnceInit.
  static void Init();

  // The number of SIGPROF (or SIGALRM for ITIMER_REAL) interrupts received.
  int64 interrupts_ GUARDED_BY(signal_lock_);

  // SIGPROF/SIGALRM interrupt frequency, read-only after construction.
  int32 frequency_;

  // ITIMER_PROF (which uses SIGPROF), or ITIMER_REAL (which uses SIGALRM)
  int timer_type_;

  // Counts the number of callbacks registered.
  int32 callback_count_ GUARDED_BY(control_lock_);

  // Whether or not the threading system provides interval timers that are
  // shared by all threads in a process.
  enum {
    // No timer initialization attempted yet.
    TIMERS_UNTOUCHED,
    // First thread has registered and set timer.
    TIMERS_ONE_SET,
    // Timers are shared by all threads.
    TIMERS_SHARED,
    // Timers are separate in each thread.
    TIMERS_SEPARATE
  } timer_sharing_ GUARDED_BY(control_lock_);

  // This lock serializes the registration of threads and protects the
  // callbacks_ list below.
  // Locking order:
  // In the context of a signal handler, acquire signal_lock_ to walk the
  // callback list. Otherwise, acquire control_lock_, disable the signal
  // handler and then acquire signal_lock_.
  SpinLock control_lock_ ACQUIRED_BEFORE(signal_lock_);
  SpinLock signal_lock_;

  // Holds the list of registered callbacks. We expect the list to be pretty
  // small. Currently, the cpu profiler (base/profiler) and thread module
  // (base/thread.h) are the only two components registering callbacks.
  // Following are the locking requirements for callbacks_:
  // For read-write access outside the SIGPROF handler:
  //  - Acquire control_lock_
  //  - Disable SIGPROF handler.
  //  - Acquire signal_lock_
  // For read-only access in the context of SIGPROF handler
  // (Read-write access is *not allowed* in the SIGPROF handler)
  //  - Acquire signal_lock_
  // For read-only access outside SIGPROF handler:
  //  - Acquire control_lock_
  typedef list<ProfileHandlerToken*> CallbackList;
  typedef CallbackList::iterator CallbackIterator;
  CallbackList callbacks_ GUARDED_BY(signal_lock_);

  // Starts the interval timer.  If the thread library shares timers between
  // threads, this function starts the shared timer. Otherwise, this will start
  // the timer in the current thread.
  void StartTimer() EXCLUSIVE_LOCKS_REQUIRED(control_lock_);

  // Stops the interval timer. If the thread library shares timers between
  // threads, this fucntion stops the shared timer. Otherwise, this will stop
  // the timer in the current thread.
  void StopTimer() EXCLUSIVE_LOCKS_REQUIRED(control_lock_);

  // Returns true if the profile interval timer is enabled in the current
  // thread.  This actually checks the kernel's interval timer setting.  (It is
  // used to detect whether timers are shared or separate.)
  bool IsTimerRunning() EXCLUSIVE_LOCKS_REQUIRED(control_lock_);

  // Sets the timer interrupt signal handler.
  void EnableHandler() EXCLUSIVE_LOCKS_REQUIRED(control_lock_);

  // Disables (ignores) the timer interrupt signal.
  void DisableHandler() EXCLUSIVE_LOCKS_REQUIRED(control_lock_);

  // SIGPROF/SIGALRM handler. Iterate over and call all the registered callbacks.
  static void SignalHandler(int sig, siginfo_t* sinfo, void* ucontext);

  DISALLOW_COPY_AND_ASSIGN(ProfileHandler);
};

ProfileHandler* ProfileHandler::instance_ = NULL;
pthread_once_t ProfileHandler::once_ = PTHREAD_ONCE_INIT;

const int32 ProfileHandler::kMaxFrequency;
const int32 ProfileHandler::kDefaultFrequency;

// If we are LD_PRELOAD-ed against a non-pthreads app, then
// pthread_once won't be defined.  We declare it here, for that
// case (with weak linkage) which will cause the non-definition to
// resolve to NULL.  We can then check for NULL or not in Instance.
extern "C" int pthread_once(pthread_once_t *, void (*)(void))
    ATTRIBUTE_WEAK;

void ProfileHandler::Init() {
  instance_ = new ProfileHandler();
}

ProfileHandler* ProfileHandler::Instance() {
  if (pthread_once) {
    pthread_once(&once_, Init);
  }
  if (instance_ == NULL) {
    // This will be true on systems that don't link in pthreads,
    // including on FreeBSD where pthread_once has a non-zero address
    // (but doesn't do anything) even when pthreads isn't linked in.
    Init();
    assert(instance_ != NULL);
  }
  return instance_;
}

ProfileHandler::ProfileHandler()
    : interrupts_(0),
      callback_count_(0),
      timer_sharing_(TIMERS_UNTOUCHED) {
  SpinLockHolder cl(&control_lock_);

  timer_type_ = (getenv("CPUPROFILE_REALTIME") ? ITIMER_REAL : ITIMER_PROF);

  // Get frequency of interrupts (if specified)
  char junk;
  const char* fr = getenv("CPUPROFILE_FREQUENCY");
  if (fr != NULL && (sscanf(fr, "%u%c", &frequency_, &junk) == 1) &&
      (frequency_ > 0)) {
    // Limit to kMaxFrequency
    frequency_ = (frequency_ > kMaxFrequency) ? kMaxFrequency : frequency_;
  } else {
    frequency_ = kDefaultFrequency;
  }

  // Ignore signals until we decide to turn profiling on.  (Paranoia;
  // should already be ignored.)
  DisableHandler();
}

ProfileHandler::~ProfileHandler() {
  Reset();
}

void ProfileHandler::RegisterThread() {
  SpinLockHolder cl(&control_lock_);

  // We try to detect whether timers are being shared by setting a
  // timer in the first call to this function, then checking whether
  // it's set in the second call.
  //
  // Note that this detection method requires that the first two calls
  // to RegisterThread must be made from different threads.  (Subsequent
  // calls will see timer_sharing_ set to either TIMERS_SEPARATE or
  // TIMERS_SHARED, and won't try to detect the timer sharing type.)
  //
  // Also note that if timer settings were inherited across new thread
  // creation but *not* shared, this approach wouldn't work.  That's
  // not an issue for any Linux threading implementation, and should
  // not be a problem for a POSIX-compliant threads implementation.
  switch (timer_sharing_) {
    case TIMERS_UNTOUCHED:
      StartTimer();
      timer_sharing_ = TIMERS_ONE_SET;
      break;
    case TIMERS_ONE_SET:
      // If the timer is running, that means that the main thread's
      // timer setup is seen in this (second) thread -- and therefore
      // that timers are shared.
      if (IsTimerRunning()) {
        timer_sharing_ = TIMERS_SHARED;
        // If callback is already registered, we have to keep the timer
        // running.  If not, we disable the timer here.
        if (callback_count_ == 0) {
          StopTimer();
        }
      } else {
        timer_sharing_ = TIMERS_SEPARATE;
        StartTimer();
      }
      break;
    case TIMERS_SHARED:
      // Nothing needed.
      break;
    case TIMERS_SEPARATE:
      StartTimer();
      break;
  }
}

ProfileHandlerToken* ProfileHandler::RegisterCallback(
    ProfileHandlerCallback callback, void* callback_arg) {
  ProfileHandlerToken* token = new ProfileHandlerToken(callback, callback_arg);

  SpinLockHolder cl(&control_lock_);
  DisableHandler();
  {
    SpinLockHolder sl(&signal_lock_);
    callbacks_.push_back(token);
  }
  // Start the timer if timer is shared and this is a first callback.
  if ((callback_count_ == 0) && (timer_sharing_ == TIMERS_SHARED)) {
    StartTimer();
  }
  ++callback_count_;
  EnableHandler();
  return token;
}

void ProfileHandler::UnregisterCallback(ProfileHandlerToken* token) {
  SpinLockHolder cl(&control_lock_);
  for (CallbackIterator it = callbacks_.begin(); it != callbacks_.end();
       ++it) {
    if ((*it) == token) {
      RAW_CHECK(callback_count_ > 0, "Invalid callback count");
      DisableHandler();
      {
        SpinLockHolder sl(&signal_lock_);
        delete *it;
        callbacks_.erase(it);
      }
      --callback_count_;
      if (callback_count_ > 0) {
        EnableHandler();
      } else if (timer_sharing_ == TIMERS_SHARED) {
        StopTimer();
      }
      return;
    }
  }
  // Unknown token.
  RAW_LOG(FATAL, "Invalid token");
}

void ProfileHandler::Reset() {
  SpinLockHolder cl(&control_lock_);
  DisableHandler();
  {
    SpinLockHolder sl(&signal_lock_);
    CallbackIterator it = callbacks_.begin();
    while (it != callbacks_.end()) {
      CallbackIterator tmp = it;
      ++it;
      delete *tmp;
      callbacks_.erase(tmp);
    }
  }
  callback_count_ = 0;
  if (timer_sharing_ == TIMERS_SHARED) {
    StopTimer();
  }
  timer_sharing_ = TIMERS_UNTOUCHED;
}

void ProfileHandler::GetState(ProfileHandlerState* state) {
  SpinLockHolder cl(&control_lock_);
  DisableHandler();
  {
    SpinLockHolder sl(&signal_lock_);  // Protects interrupts_.
    state->interrupts = interrupts_;
  }
  if (callback_count_ > 0) {
    EnableHandler();
  }
  state->frequency = frequency_;
  state->callback_count = callback_count_;
}

void ProfileHandler::StartTimer() {
  struct itimerval timer;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 1000000 / frequency_;
  timer.it_value = timer.it_interval;
  setitimer(timer_type_, &timer, 0);
}

void ProfileHandler::StopTimer() {
  struct itimerval timer;
  memset(&timer, 0, sizeof timer);
  setitimer(timer_type_, &timer, 0);
}

bool ProfileHandler::IsTimerRunning() {
  struct itimerval current_timer;
  RAW_CHECK(0 == getitimer(timer_type_, &current_timer), "getitimer");
  return (current_timer.it_value.tv_sec != 0 ||
          current_timer.it_value.tv_usec != 0);
}

void ProfileHandler::EnableHandler() {
  struct sigaction sa;
  sa.sa_sigaction = SignalHandler;
  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  const int signal_number = (timer_type_ == ITIMER_PROF ? SIGPROF : SIGALRM);
  RAW_CHECK(sigaction(signal_number, &sa, NULL) == 0, "sigprof (enable)");
}

void ProfileHandler::DisableHandler() {
  struct sigaction sa;
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = SA_RESTART;
  sigemptyset(&sa.sa_mask);
  const int signal_number = (timer_type_ == ITIMER_PROF ? SIGPROF : SIGALRM);
  RAW_CHECK(sigaction(signal_number, &sa, NULL) == 0, "sigprof (disable)");
}

void ProfileHandler::SignalHandler(int sig, siginfo_t* sinfo, void* ucontext) {
  int saved_errno = errno;
  RAW_CHECK(instance_ != NULL, "ProfileHandler is not initialized");
  {
    SpinLockHolder sl(&instance_->signal_lock_);
    ++instance_->interrupts_;
    for (CallbackIterator it = instance_->callbacks_.begin();
         it != instance_->callbacks_.end();
         ++it) {
      (*it)->callback(sig, sinfo, ucontext, (*it)->callback_arg);
    }
  }
  errno = saved_errno;
}

// The sole purpose of this class is to initialize the ProfileHandler singleton
// when the global static objects are created. Note that the main thread will
// be registered at this time.
class ProfileHandlerInitializer {
 public:
  ProfileHandlerInitializer() {
    ProfileHandler::Instance()->RegisterThread();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ProfileHandlerInitializer);
};
// ProfileHandlerInitializer singleton
static ProfileHandlerInitializer profile_handler_initializer;

extern "C" void ProfileHandlerRegisterThread() {
  ProfileHandler::Instance()->RegisterThread();
}

extern "C" ProfileHandlerToken* ProfileHandlerRegisterCallback(
    ProfileHandlerCallback callback, void* callback_arg) {
  return ProfileHandler::Instance()->RegisterCallback(callback, callback_arg);
}

extern "C" void ProfileHandlerUnregisterCallback(ProfileHandlerToken* token) {
  ProfileHandler::Instance()->UnregisterCallback(token);
}

extern "C" void ProfileHandlerReset() {
  return ProfileHandler::Instance()->Reset();
}

extern "C" void ProfileHandlerGetState(ProfileHandlerState* state) {
  ProfileHandler::Instance()->GetState(state);
}

#else  // OS_CYGWIN

// ITIMER_PROF doesn't work under cygwin.  ITIMER_REAL is available, but doesn't
// work as well for profiling, and also interferes with alarm().  Because of
// these issues, unless a specific need is identified, profiler support is
// disabled under Cygwin.
extern "C" void ProfileHandlerRegisterThread() {
}

extern "C" ProfileHandlerToken* ProfileHandlerRegisterCallback(
    ProfileHandlerCallback callback, void* callback_arg) {
  return NULL;
}

extern "C" void ProfileHandlerUnregisterCallback(ProfileHandlerToken* token) {
}

extern "C" void ProfileHandlerReset() {
}

extern "C" void ProfileHandlerGetState(ProfileHandlerState* state) {
}

#endif  // OS_CYGWIN
