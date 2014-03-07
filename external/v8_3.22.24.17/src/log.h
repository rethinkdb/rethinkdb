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

#ifndef V8_LOG_H_
#define V8_LOG_H_

#include "allocation.h"
#include "objects.h"
#include "platform.h"
#include "platform/elapsed-timer.h"

namespace v8 {
namespace internal {

// Logger is used for collecting logging information from V8 during
// execution. The result is dumped to a file.
//
// Available command line flags:
//
//  --log
// Minimal logging (no API, code, or GC sample events), default is off.
//
// --log-all
// Log all events to the file, default is off.  This is the same as combining
// --log-api, --log-code, --log-gc, and --log-regexp.
//
// --log-api
// Log API events to the logfile, default is off.  --log-api implies --log.
//
// --log-code
// Log code (create, move, and delete) events to the logfile, default is off.
// --log-code implies --log.
//
// --log-gc
// Log GC heap samples after each GC that can be processed by hp2ps, default
// is off.  --log-gc implies --log.
//
// --log-regexp
// Log creation and use of regular expressions, Default is off.
// --log-regexp implies --log.
//
// --logfile <filename>
// Specify the name of the logfile, default is "v8.log".
//
// --prof
// Collect statistical profiling information (ticks), default is off.  The
// tick profiler requires code events, so --prof implies --log-code.

// Forward declarations.
class CodeEventListener;
class CompilationInfo;
class CpuProfiler;
class Isolate;
class Log;
class PositionsRecorder;
class Profiler;
class Semaphore;
class Ticker;
struct TickSample;

#undef LOG
#define LOG(isolate, Call)                          \
  do {                                              \
    v8::internal::Logger* logger =                  \
        (isolate)->logger();                        \
    if (logger->is_logging())                       \
      logger->Call;                                 \
  } while (false)

#define LOG_CODE_EVENT(isolate, Call)               \
  do {                                              \
    v8::internal::Logger* logger =                  \
        (isolate)->logger();                        \
    if (logger->is_logging_code_events())           \
      logger->Call;                                 \
  } while (false)


#define LOG_EVENTS_AND_TAGS_LIST(V)                                     \
  V(CODE_CREATION_EVENT,            "code-creation")                    \
  V(CODE_MOVE_EVENT,                "code-move")                        \
  V(CODE_DELETE_EVENT,              "code-delete")                      \
  V(CODE_MOVING_GC,                 "code-moving-gc")                   \
  V(SHARED_FUNC_MOVE_EVENT,         "sfi-move")                         \
  V(SNAPSHOT_POSITION_EVENT,        "snapshot-pos")                     \
  V(SNAPSHOT_CODE_NAME_EVENT,       "snapshot-code-name")               \
  V(TICK_EVENT,                     "tick")                             \
  V(REPEAT_META_EVENT,              "repeat")                           \
  V(BUILTIN_TAG,                    "Builtin")                          \
  V(CALL_DEBUG_BREAK_TAG,           "CallDebugBreak")                   \
  V(CALL_DEBUG_PREPARE_STEP_IN_TAG, "CallDebugPrepareStepIn")           \
  V(CALL_IC_TAG,                    "CallIC")                           \
  V(CALL_INITIALIZE_TAG,            "CallInitialize")                   \
  V(CALL_MEGAMORPHIC_TAG,           "CallMegamorphic")                  \
  V(CALL_MISS_TAG,                  "CallMiss")                         \
  V(CALL_NORMAL_TAG,                "CallNormal")                       \
  V(CALL_PRE_MONOMORPHIC_TAG,       "CallPreMonomorphic")               \
  V(KEYED_CALL_DEBUG_BREAK_TAG,     "KeyedCallDebugBreak")              \
  V(KEYED_CALL_DEBUG_PREPARE_STEP_IN_TAG,                               \
    "KeyedCallDebugPrepareStepIn")                                      \
  V(KEYED_CALL_IC_TAG,              "KeyedCallIC")                      \
  V(KEYED_CALL_INITIALIZE_TAG,      "KeyedCallInitialize")              \
  V(KEYED_CALL_MEGAMORPHIC_TAG,     "KeyedCallMegamorphic")             \
  V(KEYED_CALL_MISS_TAG,            "KeyedCallMiss")                    \
  V(KEYED_CALL_NORMAL_TAG,          "KeyedCallNormal")                  \
  V(KEYED_CALL_PRE_MONOMORPHIC_TAG, "KeyedCallPreMonomorphic")          \
  V(CALLBACK_TAG,                   "Callback")                         \
  V(EVAL_TAG,                       "Eval")                             \
  V(FUNCTION_TAG,                   "Function")                         \
  V(HANDLER_TAG,                    "Handler")                          \
  V(KEYED_LOAD_IC_TAG,              "KeyedLoadIC")                      \
  V(KEYED_LOAD_POLYMORPHIC_IC_TAG,  "KeyedLoadPolymorphicIC")           \
  V(KEYED_EXTERNAL_ARRAY_LOAD_IC_TAG, "KeyedExternalArrayLoadIC")       \
  V(KEYED_STORE_IC_TAG,             "KeyedStoreIC")                     \
  V(KEYED_STORE_POLYMORPHIC_IC_TAG, "KeyedStorePolymorphicIC")          \
  V(KEYED_EXTERNAL_ARRAY_STORE_IC_TAG, "KeyedExternalArrayStoreIC")     \
  V(LAZY_COMPILE_TAG,               "LazyCompile")                      \
  V(LOAD_IC_TAG,                    "LoadIC")                           \
  V(LOAD_POLYMORPHIC_IC_TAG,        "LoadPolymorphicIC")                \
  V(REG_EXP_TAG,                    "RegExp")                           \
  V(SCRIPT_TAG,                     "Script")                           \
  V(STORE_IC_TAG,                   "StoreIC")                          \
  V(STORE_POLYMORPHIC_IC_TAG,       "StorePolymorphicIC")               \
  V(STUB_TAG,                       "Stub")                             \
  V(NATIVE_FUNCTION_TAG,            "Function")                         \
  V(NATIVE_LAZY_COMPILE_TAG,        "LazyCompile")                      \
  V(NATIVE_SCRIPT_TAG,              "Script")
// Note that 'NATIVE_' cases for functions and scripts are mapped onto
// original tags when writing to the log.


class JitLogger;
class LowLevelLogger;
class Sampler;

class Logger {
 public:
#define DECLARE_ENUM(enum_item, ignore) enum_item,
  enum LogEventsAndTags {
    LOG_EVENTS_AND_TAGS_LIST(DECLARE_ENUM)
    NUMBER_OF_LOG_EVENTS
  };
#undef DECLARE_ENUM

  // Acquires resources for logging if the right flags are set.
  bool SetUp(Isolate* isolate);

  // Sets the current code event handler.
  void SetCodeEventHandler(uint32_t options,
                           JitCodeEventHandler event_handler);

  Sampler* sampler();

  // Frees resources acquired in SetUp.
  // When a temporary file is used for the log, returns its stream descriptor,
  // leaving the file open.
  FILE* TearDown();

  // Emits an event with a string value -> (name, value).
  void StringEvent(const char* name, const char* value);

  // Emits an event with an int value -> (name, value).
  void IntEvent(const char* name, int value);
  void IntPtrTEvent(const char* name, intptr_t value);

  // Emits an event with an handle value -> (name, location).
  void HandleEvent(const char* name, Object** location);

  // Emits memory management events for C allocated structures.
  void NewEvent(const char* name, void* object, size_t size);
  void DeleteEvent(const char* name, void* object);

  // Static versions of the above, operate on current isolate's logger.
  // Used in TRACK_MEMORY(TypeName) defined in globals.h
  static void NewEventStatic(const char* name, void* object, size_t size);
  static void DeleteEventStatic(const char* name, void* object);

  // Emits an event with a tag, and some resource usage information.
  // -> (name, tag, <rusage information>).
  // Currently, the resource usage information is a process time stamp
  // and a real time timestamp.
  void ResourceEvent(const char* name, const char* tag);

  // Emits an event that an undefined property was read from an
  // object.
  void SuspectReadEvent(Name* name, Object* obj);

  // Emits an event when a message is put on or read from a debugging queue.
  // DebugTag lets us put a call-site specific label on the event.
  void DebugTag(const char* call_site_tag);
  void DebugEvent(const char* event_type, Vector<uint16_t> parameter);


  // ==== Events logged by --log-api. ====
  void ApiNamedSecurityCheck(Object* key);
  void ApiIndexedSecurityCheck(uint32_t index);
  void ApiNamedPropertyAccess(const char* tag, JSObject* holder, Object* name);
  void ApiIndexedPropertyAccess(const char* tag,
                                JSObject* holder,
                                uint32_t index);
  void ApiObjectAccess(const char* tag, JSObject* obj);
  void ApiEntryCall(const char* name);


  // ==== Events logged by --log-code. ====
  void addCodeEventListener(CodeEventListener* listener);
  void removeCodeEventListener(CodeEventListener* listener);
  bool hasCodeEventListener(CodeEventListener* listener);


  // Emits a code event for a callback function.
  void CallbackEvent(Name* name, Address entry_point);
  void GetterCallbackEvent(Name* name, Address entry_point);
  void SetterCallbackEvent(Name* name, Address entry_point);
  // Emits a code create event.
  void CodeCreateEvent(LogEventsAndTags tag,
                       Code* code, const char* source);
  void CodeCreateEvent(LogEventsAndTags tag,
                       Code* code, Name* name);
  void CodeCreateEvent(LogEventsAndTags tag,
                       Code* code,
                       SharedFunctionInfo* shared,
                       CompilationInfo* info,
                       Name* name);
  void CodeCreateEvent(LogEventsAndTags tag,
                       Code* code,
                       SharedFunctionInfo* shared,
                       CompilationInfo* info,
                       Name* source, int line, int column);
  void CodeCreateEvent(LogEventsAndTags tag, Code* code, int args_count);
  void CodeMovingGCEvent();
  // Emits a code create event for a RegExp.
  void RegExpCodeCreateEvent(Code* code, String* source);
  // Emits a code move event.
  void CodeMoveEvent(Address from, Address to);
  // Emits a code delete event.
  void CodeDeleteEvent(Address from);
  // Emits a code line info add event with Postion type.
  void CodeLinePosInfoAddPositionEvent(void* jit_handler_data,
                                       int pc_offset,
                                       int position);
  // Emits a code line info add event with StatementPostion type.
  void CodeLinePosInfoAddStatementPositionEvent(void* jit_handler_data,
                                                int pc_offset,
                                                int position);
  // Emits a code line info start to record event
  void CodeStartLinePosInfoRecordEvent(PositionsRecorder* pos_recorder);
  // Emits a code line info finish record event.
  // It's the callee's responsibility to dispose the parameter jit_handler_data.
  void CodeEndLinePosInfoRecordEvent(Code* code, void* jit_handler_data);

  void SharedFunctionInfoMoveEvent(Address from, Address to);

  void CodeNameEvent(Address addr, int pos, const char* code_name);
  void SnapshotPositionEvent(Address addr, int pos);

  // ==== Events logged by --log-gc. ====
  // Heap sampling events: start, end, and individual types.
  void HeapSampleBeginEvent(const char* space, const char* kind);
  void HeapSampleEndEvent(const char* space, const char* kind);
  void HeapSampleItemEvent(const char* type, int number, int bytes);
  void HeapSampleJSConstructorEvent(const char* constructor,
                                    int number, int bytes);
  void HeapSampleJSRetainersEvent(const char* constructor,
                                         const char* event);
  void HeapSampleJSProducerEvent(const char* constructor,
                                 Address* stack);
  void HeapSampleStats(const char* space, const char* kind,
                       intptr_t capacity, intptr_t used);

  void SharedLibraryEvent(const char* library_path,
                          uintptr_t start,
                          uintptr_t end);
  void SharedLibraryEvent(const wchar_t* library_path,
                          uintptr_t start,
                          uintptr_t end);

  // ==== Events logged by --log-timer-events. ====
  enum StartEnd { START, END };

  void CodeDeoptEvent(Code* code);

  void TimerEvent(StartEnd se, const char* name);

  static void EnterExternal(Isolate* isolate);
  static void LeaveExternal(Isolate* isolate);

  class TimerEventScope {
   public:
    TimerEventScope(Isolate* isolate, const char* name)
        : isolate_(isolate), name_(name) {
      if (FLAG_log_internal_timer_events) LogTimerEvent(START);
    }

    ~TimerEventScope() {
      if (FLAG_log_internal_timer_events) LogTimerEvent(END);
    }

    void LogTimerEvent(StartEnd se);

    static const char* v8_recompile_synchronous;
    static const char* v8_recompile_concurrent;
    static const char* v8_compile_full_code;
    static const char* v8_execute;
    static const char* v8_external;

   private:
    Isolate* isolate_;
    const char* name_;
  };

  // ==== Events logged by --log-regexp ====
  // Regexp compilation and execution events.

  void RegExpCompileEvent(Handle<JSRegExp> regexp, bool in_cache);

  // Log an event reported from generated code
  void LogRuntime(Vector<const char> format, JSArray* args);

  bool is_logging() {
    return is_logging_;
  }

  bool is_logging_code_events() {
    return is_logging() || jit_logger_ != NULL;
  }

  // Stop collection of profiling data.
  // When data collection is paused, CPU Tick events are discarded.
  void StopProfiler();

  void LogExistingFunction(Handle<SharedFunctionInfo> shared,
                           Handle<Code> code);
  // Logs all compiled functions found in the heap.
  void LogCompiledFunctions();
  // Logs all accessor callbacks found in the heap.
  void LogAccessorCallbacks();
  // Used for logging stubs found in the snapshot.
  void LogCodeObjects();

  // Converts tag to a corresponding NATIVE_... if the script is native.
  INLINE(static LogEventsAndTags ToNativeByScript(LogEventsAndTags, Script*));

  // Profiler's sampling interval (in milliseconds).
#if defined(ANDROID)
  // Phones and tablets have processors that are much slower than desktop
  // and laptop computers for which current heuristics are tuned.
  static const int kSamplingIntervalMs = 5;
#else
  static const int kSamplingIntervalMs = 1;
#endif

  // Callback from Log, stops profiling in case of insufficient resources.
  void LogFailure();

 private:
  explicit Logger(Isolate* isolate);
  ~Logger();

  // Emits the profiler's first message.
  void ProfilerBeginEvent();

  // Emits callback event messages.
  void CallbackEventInternal(const char* prefix,
                             Name* name,
                             Address entry_point);

  // Internal configurable move event.
  void MoveEventInternal(LogEventsAndTags event, Address from, Address to);

  // Emits the source code of a regexp. Used by regexp events.
  void LogRegExpSource(Handle<JSRegExp> regexp);

  // Used for logging stubs found in the snapshot.
  void LogCodeObject(Object* code_object);

  // Helper method. It resets name_buffer_ and add tag name into it.
  void InitNameBuffer(LogEventsAndTags tag);

  // Emits a profiler tick event. Used by the profiler thread.
  void TickEvent(TickSample* sample, bool overflow);

  void ApiEvent(const char* name, ...);

  // Logs a StringEvent regardless of whether FLAG_log is true.
  void UncheckedStringEvent(const char* name, const char* value);

  // Logs an IntEvent regardless of whether FLAG_log is true.
  void UncheckedIntEvent(const char* name, int value);
  void UncheckedIntPtrTEvent(const char* name, intptr_t value);

  Isolate* isolate_;

  // The sampler used by the profiler and the sliding state window.
  Ticker* ticker_;

  // When the statistical profile is active, profiler_
  // points to a Profiler, that handles collection
  // of samples.
  Profiler* profiler_;

  // An array of log events names.
  const char* const* log_events_;

  // Internal implementation classes with access to
  // private members.
  friend class EventLog;
  friend class Isolate;
  friend class TimeLog;
  friend class Profiler;
  template <StateTag Tag> friend class VMState;
  friend class LoggerTestHelper;

  bool is_logging_;
  Log* log_;
  LowLevelLogger* ll_logger_;
  JitLogger* jit_logger_;
  List<CodeEventListener*> listeners_;

  // Guards against multiple calls to TearDown() that can happen in some tests.
  // 'true' between SetUp() and TearDown().
  bool is_initialized_;

  ElapsedTimer timer_;

  friend class CpuProfiler;
};


class CodeEventListener {
 public:
  virtual ~CodeEventListener() {}

  virtual void CodeCreateEvent(Logger::LogEventsAndTags tag,
                               Code* code,
                               const char* comment) = 0;
  virtual void CodeCreateEvent(Logger::LogEventsAndTags tag,
                               Code* code,
                               Name* name) = 0;
  virtual void CodeCreateEvent(Logger::LogEventsAndTags tag,
                               Code* code,
                               SharedFunctionInfo* shared,
                               CompilationInfo* info,
                               Name* name) = 0;
  virtual void CodeCreateEvent(Logger::LogEventsAndTags tag,
                               Code* code,
                               SharedFunctionInfo* shared,
                               CompilationInfo* info,
                               Name* source,
                               int line, int column) = 0;
  virtual void CodeCreateEvent(Logger::LogEventsAndTags tag,
                               Code* code,
                               int args_count) = 0;
  virtual void CallbackEvent(Name* name, Address entry_point) = 0;
  virtual void GetterCallbackEvent(Name* name, Address entry_point) = 0;
  virtual void SetterCallbackEvent(Name* name, Address entry_point) = 0;
  virtual void RegExpCodeCreateEvent(Code* code, String* source) = 0;
  virtual void CodeMoveEvent(Address from, Address to) = 0;
  virtual void CodeDeleteEvent(Address from) = 0;
  virtual void SharedFunctionInfoMoveEvent(Address from, Address to) = 0;
  virtual void CodeMovingGCEvent() = 0;
};


class CodeEventLogger : public CodeEventListener {
 public:
  CodeEventLogger();
  virtual ~CodeEventLogger();

  virtual void CodeCreateEvent(Logger::LogEventsAndTags tag,
                               Code* code,
                               const char* comment);
  virtual void CodeCreateEvent(Logger::LogEventsAndTags tag,
                               Code* code,
                               Name* name);
  virtual void CodeCreateEvent(Logger::LogEventsAndTags tag,
                               Code* code,
                               int args_count);
  virtual void CodeCreateEvent(Logger::LogEventsAndTags tag,
                               Code* code,
                               SharedFunctionInfo* shared,
                               CompilationInfo* info,
                               Name* name);
  virtual void CodeCreateEvent(Logger::LogEventsAndTags tag,
                               Code* code,
                               SharedFunctionInfo* shared,
                               CompilationInfo* info,
                               Name* source,
                               int line, int column);
  virtual void RegExpCodeCreateEvent(Code* code, String* source);

  virtual void CallbackEvent(Name* name, Address entry_point) { }
  virtual void GetterCallbackEvent(Name* name, Address entry_point) { }
  virtual void SetterCallbackEvent(Name* name, Address entry_point) { }
  virtual void SharedFunctionInfoMoveEvent(Address from, Address to) { }
  virtual void CodeMovingGCEvent() { }

 private:
  class NameBuffer;

  virtual void LogRecordedBuffer(Code* code,
                                 SharedFunctionInfo* shared,
                                 const char* name,
                                 int length) = 0;

  NameBuffer* name_buffer_;
};


} }  // namespace v8::internal


#endif  // V8_LOG_H_
