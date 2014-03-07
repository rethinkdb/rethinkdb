// Copyright 2008 the V8 project authors. All rights reserved.
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

#ifndef V8_V8_DEBUG_H_
#define V8_V8_DEBUG_H_

#include "v8.h"

/**
 * Debugger support for the V8 JavaScript engine.
 */
namespace v8 {

// Debug events which can occur in the V8 JavaScript engine.
enum DebugEvent {
  Break = 1,
  Exception = 2,
  NewFunction = 3,
  BeforeCompile = 4,
  AfterCompile  = 5,
  ScriptCollected = 6,
  BreakForCommand = 7
};


class V8_EXPORT Debug {
 public:
  /**
   * A client object passed to the v8 debugger whose ownership will be taken by
   * it. v8 is always responsible for deleting the object.
   */
  class ClientData {
   public:
    virtual ~ClientData() {}
  };


  /**
   * A message object passed to the debug message handler.
   */
  class Message {
   public:
    /**
     * Check type of message.
     */
    virtual bool IsEvent() const = 0;
    virtual bool IsResponse() const = 0;
    virtual DebugEvent GetEvent() const = 0;

    /**
     * Indicate whether this is a response to a continue command which will
     * start the VM running after this is processed.
     */
    virtual bool WillStartRunning() const = 0;

    /**
     * Access to execution state and event data. Don't store these cross
     * callbacks as their content becomes invalid. These objects are from the
     * debugger event that started the debug message loop.
     */
    virtual Handle<Object> GetExecutionState() const = 0;
    virtual Handle<Object> GetEventData() const = 0;

    /**
     * Get the debugger protocol JSON.
     */
    virtual Handle<String> GetJSON() const = 0;

    /**
     * Get the context active when the debug event happened. Note this is not
     * the current active context as the JavaScript part of the debugger is
     * running in its own context which is entered at this point.
     */
    virtual Handle<Context> GetEventContext() const = 0;

    /**
     * Client data passed with the corresponding request if any. This is the
     * client_data data value passed into Debug::SendCommand along with the
     * request that led to the message or NULL if the message is an event. The
     * debugger takes ownership of the data and will delete it even if there is
     * no message handler.
     */
    virtual ClientData* GetClientData() const = 0;

    virtual Isolate* GetIsolate() const = 0;

    virtual ~Message() {}
  };


  /**
   * An event details object passed to the debug event listener.
   */
  class EventDetails {
   public:
    /**
     * Event type.
     */
    virtual DebugEvent GetEvent() const = 0;

    /**
     * Access to execution state and event data of the debug event. Don't store
     * these cross callbacks as their content becomes invalid.
     */
    virtual Handle<Object> GetExecutionState() const = 0;
    virtual Handle<Object> GetEventData() const = 0;

    /**
     * Get the context active when the debug event happened. Note this is not
     * the current active context as the JavaScript part of the debugger is
     * running in its own context which is entered at this point.
     */
    virtual Handle<Context> GetEventContext() const = 0;

    /**
     * Client data passed with the corresponding callback when it was
     * registered.
     */
    virtual Handle<Value> GetCallbackData() const = 0;

    /**
     * Client data passed to DebugBreakForCommand function. The
     * debugger takes ownership of the data and will delete it even if
     * there is no message handler.
     */
    virtual ClientData* GetClientData() const = 0;

    virtual ~EventDetails() {}
  };

  /**
   * Debug event callback function.
   *
   * \param event_details object providing information about the debug event
   *
   * A EventCallback2 does not take possession of the event data,
   * and must not rely on the data persisting after the handler returns.
   */
  typedef void (*EventCallback2)(const EventDetails& event_details);

  /**
   * Debug message callback function.
   *
   * \param message the debug message handler message object
   *
   * A MessageHandler2 does not take possession of the message data,
   * and must not rely on the data persisting after the handler returns.
   */
  typedef void (*MessageHandler2)(const Message& message);

  /**
   * Debug host dispatch callback function.
   */
  typedef void (*HostDispatchHandler)();

  /**
   * Callback function for the host to ensure debug messages are processed.
   */
  typedef void (*DebugMessageDispatchHandler)();

  static bool SetDebugEventListener2(EventCallback2 that,
                                     Handle<Value> data = Handle<Value>());

  // Set a JavaScript debug event listener.
  static bool SetDebugEventListener(v8::Handle<v8::Object> that,
                                    Handle<Value> data = Handle<Value>());

  // Schedule a debugger break to happen when JavaScript code is run
  // in the given isolate. If no isolate is provided the default
  // isolate is used.
  static void DebugBreak(Isolate* isolate = NULL);

  // Remove scheduled debugger break in given isolate if it has not
  // happened yet. If no isolate is provided the default isolate is
  // used.
  static void CancelDebugBreak(Isolate* isolate = NULL);

  // Break execution of JavaScript in the given isolate (this method
  // can be invoked from a non-VM thread) for further client command
  // execution on a VM thread. Client data is then passed in
  // EventDetails to EventCallback2 at the moment when the VM actually
  // stops. If no isolate is provided the default isolate is used.
  static void DebugBreakForCommand(ClientData* data = NULL,
                                   Isolate* isolate = NULL);

  // Message based interface. The message protocol is JSON.
  static void SetMessageHandler2(MessageHandler2 handler);

  // If no isolate is provided the default isolate is
  // used.
  // TODO(dcarney): remove
  static void SendCommand(const uint16_t* command, int length,
                          ClientData* client_data = NULL,
                          Isolate* isolate = NULL);
  static void SendCommand(Isolate* isolate,
                          const uint16_t* command, int length,
                          ClientData* client_data = NULL);

  // Dispatch interface.
  static void SetHostDispatchHandler(HostDispatchHandler handler,
                                     int period = 100);

  /**
   * Register a callback function to be called when a debug message has been
   * received and is ready to be processed. For the debug messages to be
   * processed V8 needs to be entered, and in certain embedding scenarios this
   * callback can be used to make sure V8 is entered for the debug message to
   * be processed. Note that debug messages will only be processed if there is
   * a V8 break. This can happen automatically by using the option
   * --debugger-auto-break.
   * \param provide_locker requires that V8 acquires v8::Locker for you before
   *        calling handler
   */
  static void SetDebugMessageDispatchHandler(
      DebugMessageDispatchHandler handler, bool provide_locker = false);

 /**
  * Run a JavaScript function in the debugger.
  * \param fun the function to call
  * \param data passed as second argument to the function
  * With this call the debugger is entered and the function specified is called
  * with the execution state as the first argument. This makes it possible to
  * get access to information otherwise not available during normal JavaScript
  * execution e.g. details on stack frames. Receiver of the function call will
  * be the debugger context global object, however this is a subject to change.
  * The following example shows a JavaScript function which when passed to
  * v8::Debug::Call will return the current line of JavaScript execution.
  *
  * \code
  *   function frame_source_line(exec_state) {
  *     return exec_state.frame(0).sourceLine();
  *   }
  * \endcode
  */
  static Local<Value> Call(v8::Handle<v8::Function> fun,
                           Handle<Value> data = Handle<Value>());

  /**
   * Returns a mirror object for the given object.
   */
  static Local<Value> GetMirror(v8::Handle<v8::Value> obj);

 /**
  * Enable the V8 builtin debug agent. The debugger agent will listen on the
  * supplied TCP/IP port for remote debugger connection.
  * \param name the name of the embedding application
  * \param port the TCP/IP port to listen on
  * \param wait_for_connection whether V8 should pause on a first statement
  *   allowing remote debugger to connect before anything interesting happened
  */
  static bool EnableAgent(const char* name, int port,
                          bool wait_for_connection = false);

  /**
    * Disable the V8 builtin debug agent. The TCP/IP connection will be closed.
    */
  static void DisableAgent();

  /**
   * Makes V8 process all pending debug messages.
   *
   * From V8 point of view all debug messages come asynchronously (e.g. from
   * remote debugger) but they all must be handled synchronously: V8 cannot
   * do 2 things at one time so normal script execution must be interrupted
   * for a while.
   *
   * Generally when message arrives V8 may be in one of 3 states:
   * 1. V8 is running script; V8 will automatically interrupt and process all
   * pending messages (however auto_break flag should be enabled);
   * 2. V8 is suspended on debug breakpoint; in this state V8 is dedicated
   * to reading and processing debug messages;
   * 3. V8 is not running at all or has called some long-working C++ function;
   * by default it means that processing of all debug messages will be deferred
   * until V8 gets control again; however, embedding application may improve
   * this by manually calling this method.
   *
   * It makes sense to call this method whenever a new debug message arrived and
   * V8 is not already running. Method v8::Debug::SetDebugMessageDispatchHandler
   * should help with the former condition.
   *
   * Technically this method in many senses is equivalent to executing empty
   * script:
   * 1. It does nothing except for processing all pending debug messages.
   * 2. It should be invoked with the same precautions and from the same context
   * as V8 script would be invoked from, because:
   *   a. with "evaluate" command it can do whatever normal script can do,
   *   including all native calls;
   *   b. no other thread should call V8 while this method is running
   *   (v8::Locker may be used here).
   *
   * "Evaluate" debug command behavior currently is not specified in scope
   * of this method.
   */
  static void ProcessDebugMessages();

  /**
   * Debugger is running in its own context which is entered while debugger
   * messages are being dispatched. This is an explicit getter for this
   * debugger context. Note that the content of the debugger context is subject
   * to change.
   */
  static Local<Context> GetDebugContext();


  /**
   * Enable/disable LiveEdit functionality for the given Isolate
   * (default Isolate if not provided). V8 will abort if LiveEdit is
   * unexpectedly used. LiveEdit is enabled by default.
   */
  static void SetLiveEditEnabled(bool enable, Isolate* isolate = NULL);
};


}  // namespace v8


#undef EXPORT


#endif  // V8_V8_DEBUG_H_
