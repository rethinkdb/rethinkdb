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
 * Author: Joi Sigurdsson
 *
 * Definition of PreamblePatcher
 */

#ifndef GOOGLE_PERFTOOLS_PREAMBLE_PATCHER_H_
#define GOOGLE_PERFTOOLS_PREAMBLE_PATCHER_H_

#include <windows.h>

// compatibility shim
#include "base/logging.h"
#define SIDESTEP_ASSERT(cond)  RAW_DCHECK(cond, #cond)
#define SIDESTEP_LOG(msg)      RAW_VLOG(1, msg)

// Maximum size of the preamble stub. We overwrite at least the first 5
// bytes of the function. Considering the worst case scenario, we need 4
// bytes + the max instruction size + 5 more bytes for our jump back to
// the original code. With that in mind, 32 is a good number :)
#define MAX_PREAMBLE_STUB_SIZE    (32)

namespace sidestep {

// Possible results of patching/unpatching
enum SideStepError {
  SIDESTEP_SUCCESS = 0,
  SIDESTEP_INVALID_PARAMETER,
  SIDESTEP_INSUFFICIENT_BUFFER,
  SIDESTEP_JUMP_INSTRUCTION,
  SIDESTEP_FUNCTION_TOO_SMALL,
  SIDESTEP_UNSUPPORTED_INSTRUCTION,
  SIDESTEP_NO_SUCH_MODULE,
  SIDESTEP_NO_SUCH_FUNCTION,
  SIDESTEP_ACCESS_DENIED,
  SIDESTEP_UNEXPECTED,
};

#define SIDESTEP_TO_HRESULT(error)                      \
  MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NULL, error)

// Implements a patching mechanism that overwrites the first few bytes of
// a function preamble with a jump to our hook function, which is then
// able to call the original function via a specially-made preamble-stub
// that imitates the action of the original preamble.
//
// NOTE:  This patching mechanism should currently only be used for
// non-production code, e.g. unit tests, because it is not threadsafe.
// See the TODO in preamble_patcher_with_stub.cc for instructions on what
// we need to do before using it in production code; it's fairly simple
// but unnecessary for now since we only intend to use it in unit tests.
//
// To patch a function, use either of the typesafe Patch() methods.  You
// can unpatch a function using Unpatch().
//
// Typical usage goes something like this:
// @code
// typedef int (*MyTypesafeFuncPtr)(int x);
// MyTypesafeFuncPtr original_func_stub;
// int MyTypesafeFunc(int x) { return x + 1; }
// int HookMyTypesafeFunc(int x) { return 1 + original_func_stub(x); }
// 
// void MyPatchInitializingFunction() {
//   original_func_stub = PreamblePatcher::Patch(
//              MyTypesafeFunc, HookMyTypesafeFunc);
//   if (!original_func_stub) {
//     // ... error handling ...
//   }
//
//   // ... continue - you have patched the function successfully ...
// }
// @endcode
//
// Note that there are a number of ways that this method of patching can
// fail.  The most common are:
//    - If there is a jump (jxx) instruction in the first 5 bytes of
//    the function being patched, we cannot patch it because in the
//    current implementation we do not know how to rewrite relative
//    jumps after relocating them to the preamble-stub.  Note that
//    if you really really need to patch a function like this, it
//    would be possible to add this functionality (but at some cost).
//    - If there is a return (ret) instruction in the first 5 bytes
//    we cannot patch the function because it may not be long enough
//    for the jmp instruction we use to inject our patch.
//    - If there is another thread currently executing within the bytes
//    that are copied to the preamble stub, it will crash in an undefined
//    way.
//
// If you get any other error than the above, you're either pointing the
// patcher at an invalid instruction (e.g. into the middle of a multi-
// byte instruction, or not at memory containing executable instructions)
// or, there may be a bug in the disassembler we use to find
// instruction boundaries.
//
// NOTE:  In optimized builds, when you have very trivial functions that
// the compiler can reason do not have side effects, the compiler may
// reuse the result of calling the function with a given parameter, which
// may mean if you patch the function in between your patch will never get
// invoked.  See preamble_patcher_test.cc for an example.
class PreamblePatcher {
 public:

  // This is a typesafe version of RawPatch(), identical in all other
  // ways than it takes a template parameter indicating the type of the
  // function being patched.
  //
  // @param T The type of the function you are patching. Usually
  // you will establish this type using a typedef, as in the following
  // example:
  // @code
  // typedef BOOL (WINAPI *MessageBoxPtr)(HWND, LPCTSTR, LPCTSTR, UINT);
  // MessageBoxPtr original = NULL;
  // PreamblePatcher::Patch(MessageBox, Hook_MessageBox, &original);
  // @endcode
  template <class T>
  static SideStepError Patch(T target_function,
                             T replacement_function,
                             T* original_function_stub) {
    // NOTE: casting from a function to a pointer is contra the C++
    //       spec.  It's not safe on IA64, but is on i386.  We use
    //       a C-style cast here to emphasize this is not legal C++.
    return RawPatch((void*)(target_function),
                    (void*)(replacement_function),
                    (void**)(original_function_stub));
  }

  // Patches a named function imported from the named module using
  // preamble patching.  Uses RawPatch() to do the actual patching
  // work.
  //
  // @param T The type of the function you are patching.  Must
  // exactly match the function you specify using module_name and
  // function_name.
  //
  // @param module_name The name of the module from which the function
  // is being imported.  Note that the patch will fail if this module
  // has not already been loaded into the current process.
  //
  // @param function_name The name of the function you wish to patch.
  //
  // @param replacement_function Your replacement function which
  // will be called whenever code tries to call the original function.
  //
  // @param original_function_stub Pointer to memory that should receive a
  // pointer that can be used (e.g. in the replacement function) to call the
  // original function, or NULL to indicate failure.
  //
  // @return One of the EnSideStepError error codes; only SIDESTEP_SUCCESS
  // indicates success.
  template <class T>
  static SideStepError Patch(LPCTSTR module_name,
                             LPCSTR function_name,
                             T replacement_function,
                             T* original_function_stub) {
    SIDESTEP_ASSERT(module_name && function_name);
    if (!module_name || !function_name) {
      SIDESTEP_ASSERT(false &&
                      "You must specify a module name and function name.");
      return SIDESTEP_INVALID_PARAMETER;
    }
    HMODULE module = ::GetModuleHandle(module_name);
    SIDESTEP_ASSERT(module != NULL);
    if (!module) {
      SIDESTEP_ASSERT(false && "Invalid module name.");
      return SIDESTEP_NO_SUCH_MODULE;
    }
    FARPROC existing_function = ::GetProcAddress(module, function_name);
    if (!existing_function) {
      SIDESTEP_ASSERT(
          false && "Did not find any function with that name in the module.");
      return SIDESTEP_NO_SUCH_FUNCTION;
    }
    // NOTE: casting from a function to a pointer is contra the C++
    //       spec.  It's not safe on IA64, but is on i386.  We use
    //       a C-style cast here to emphasize this is not legal C++.
    return RawPatch((void*)existing_function, (void*)replacement_function,
                    (void**)(original_function_stub));
  }

  // Patches a function by overwriting its first few bytes with
  // a jump to a different function.  This is the "worker" function
  // for each of the typesafe Patch() functions.  In most cases,
  // it is preferable to use the Patch() functions rather than
  // this one as they do more checking at compile time.
  //
  // @param target_function A pointer to the function that should be
  // patched.
  //
  // @param replacement_function A pointer to the function that should
  // replace the target function.  The replacement function must have
  // exactly the same calling convention and parameters as the original
  // function.
  //
  // @param original_function_stub Pointer to memory that should receive a
  // pointer that can be used (e.g. in the replacement function) to call the
  // original function, or NULL to indicate failure.
  //
  // @param original_function_stub Pointer to memory that should receive a
  // pointer that can be used (e.g. in the replacement function) to call the
  // original function, or NULL to indicate failure.
  //
  // @return One of the EnSideStepError error codes; only SIDESTEP_SUCCESS
  // indicates success.
  //
  // @note The preamble-stub (the memory pointed to by
  // *original_function_stub) is allocated on the heap, and (in
  // production binaries) never destroyed, resulting in a memory leak.  This
  // will be the case until we implement safe unpatching of a method.
  // However, it is quite difficult to unpatch a method (because other
  // threads in the process may be using it) so we are leaving it for now.
  // See however UnsafeUnpatch, which can be used for binaries where you
  // know only one thread is running, e.g. unit tests.
  static SideStepError RawPatch(void* target_function,
                                void* replacement_function,
                                void** original_function_stub);

  // Unpatches target_function and deletes the stub that previously could be
  // used to call the original version of the function.
  //
  // DELETES the stub that is passed to the function.
  //
  // @param target_function Pointer to the target function which was
  // previously patched, i.e. a pointer which value should match the value
  // of the symbol prior to patching it.
  //
  // @param replacement_function Pointer to the function target_function
  // was patched to.
  //
  // @param original_function_stub Pointer to the stub returned when
  // patching, that could be used to call the original version of the
  // patched function.  This function will also delete the stub, which after
  // unpatching is useless.
  //
  // If your original call was
  //    Patch(VirtualAlloc, MyVirtualAlloc, &origptr)
  // then to undo it you would call
  //    Unpatch(VirtualAlloc, MyVirtualAlloc, origptr);
  //
  // @return One of the EnSideStepError error codes; only SIDESTEP_SUCCESS
  // indicates success.
  static SideStepError Unpatch(void* target_function,
                               void* replacement_function,
                               void* original_function_stub);

  // A helper routine when patching, which follows jmp instructions at
  // function addresses, to get to the "actual" function contents.
  // This allows us to identify two functions that are at different
  // addresses but actually resolve to the same code.
  //
  // @param target_function Pointer to a function.
  //
  // @return Either target_function (the input parameter), or if
  // target_function's body consists entirely of a JMP instruction,
  // the address it JMPs to (or more precisely, the address at the end
  // of a chain of JMPs).
  template <class T>
  static T ResolveTarget(T target_function) {
    return (T)ResolveTargetImpl((unsigned char*)target_function, NULL);
  }

 private:
  // Patches a function by overwriting its first few bytes with
  // a jump to a different function.  This is similar to the RawPatch
  // function except that it uses the stub allocated by the caller
  // instead of allocating it.
  //
  // We call VirtualProtect to make the
  // target function writable at least for the duration of the call.
  //
  // @param target_function A pointer to the function that should be
  // patched.
  //
  // @param replacement_function A pointer to the function that should
  // replace the target function.  The replacement function must have
  // exactly the same calling convention and parameters as the original
  // function.
  //
  // @param preamble_stub A pointer to a buffer where the preamble stub
  // should be copied. The size of the buffer should be sufficient to
  // hold the preamble bytes.
  //
  // @param stub_size Size in bytes of the buffer allocated for the
  // preamble_stub
  //
  // @param bytes_needed Pointer to a variable that receives the minimum
  // number of bytes required for the stub.  Can be set to NULL if you're
  // not interested.
  //
  // @return An error code indicating the result of patching.
  static SideStepError RawPatchWithStubAndProtections(
      void* target_function,
      void *replacement_function,
      unsigned char* preamble_stub,
      unsigned long stub_size,
      unsigned long* bytes_needed);

  // A helper function used by RawPatchWithStubAndProtections -- it
  // does everything but the VirtualProtect work.  Defined in
  // preamble_patcher_with_stub.cc.
  //
  // @param target_function A pointer to the function that should be
  // patched.
  //
  // @param replacement_function A pointer to the function that should
  // replace the target function.  The replacement function must have
  // exactly the same calling convention and parameters as the original
  // function.
  //
  // @param preamble_stub A pointer to a buffer where the preamble stub
  // should be copied. The size of the buffer should be sufficient to
  // hold the preamble bytes.
  //
  // @param stub_size Size in bytes of the buffer allocated for the
  // preamble_stub
  //
  // @param bytes_needed Pointer to a variable that receives the minimum
  // number of bytes required for the stub.  Can be set to NULL if you're
  // not interested.
  //
  // @return An error code indicating the result of patching.
  static SideStepError RawPatchWithStub(void* target_function,
                                        void *replacement_function,
                                        unsigned char* preamble_stub,
                                        unsigned long stub_size,
                                        unsigned long* bytes_needed);


  // A helper routine when patching, which follows jmp instructions at
  // function addresses, to get to the "actual" function contents.
  // This allows us to identify two functions that are at different
  // addresses but actually resolve to the same code.
  //
  // @param target_function Pointer to a function.
  //
  // @param stop_before If, when following JMP instructions from
  // target_function, we get to the address stop, we return
  // immediately, the address that jumps to stop_before.
  //
  // @return Either target_function (the input parameter), or if
  // target_function's body consists entirely of a JMP instruction,
  // the address it JMPs to (or more precisely, the address at the end
  // of a chain of JMPs).
  static void* ResolveTargetImpl(unsigned char* target_function,
                                 unsigned char* stop_before);
};

};  // namespace sidestep

#endif  // GOOGLE_PERFTOOLS_PREAMBLE_PATCHER_H_
