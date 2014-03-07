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

#ifndef V8_ALLOCATION_H_
#define V8_ALLOCATION_H_

#include "globals.h"

namespace v8 {
namespace internal {

// Called when allocation routines fail to allocate.
// This function should not return, but should terminate the current
// processing.
void FatalProcessOutOfMemory(const char* message);

// Superclass for classes managed with new & delete.
class Malloced {
 public:
  void* operator new(size_t size) { return New(size); }
  void  operator delete(void* p) { Delete(p); }

  static void FatalProcessOutOfMemory();
  static void* New(size_t size);
  static void Delete(void* p);
};


// A macro is used for defining the base class used for embedded instances.
// The reason is some compilers allocate a minimum of one word for the
// superclass. The macro prevents the use of new & delete in debug mode.
// In release mode we are not willing to pay this overhead.

#ifdef DEBUG
// Superclass for classes with instances allocated inside stack
// activations or inside other objects.
class Embedded {
 public:
  void* operator new(size_t size);
  void  operator delete(void* p);
};
#define BASE_EMBEDDED : public Embedded
#else
#define BASE_EMBEDDED
#endif


// Superclass for classes only using statics.
class AllStatic {
#ifdef DEBUG
 public:
  void* operator new(size_t size);
  void operator delete(void* p);
#endif
};


template <typename T>
T* NewArray(size_t size) {
  T* result = new T[size];
  if (result == NULL) Malloced::FatalProcessOutOfMemory();
  return result;
}


template <typename T>
void DeleteArray(T* array) {
  delete[] array;
}


// The normal strdup functions use malloc.  These versions of StrDup
// and StrNDup uses new and calls the FatalProcessOutOfMemory handler
// if allocation fails.
char* StrDup(const char* str);
char* StrNDup(const char* str, int n);


// Allocation policy for allocating in the C free store using malloc
// and free. Used as the default policy for lists.
class FreeStoreAllocationPolicy {
 public:
  INLINE(void* New(size_t size)) { return Malloced::New(size); }
  INLINE(static void Delete(void* p)) { Malloced::Delete(p); }
};


// Allocation policy for allocating in preallocated space.
// Used as an allocation policy for ScopeInfo when generating
// stack traces.
class PreallocatedStorage {
 public:
  explicit PreallocatedStorage(size_t size);
  size_t size() { return size_; }

 private:
  size_t size_;
  PreallocatedStorage* previous_;
  PreallocatedStorage* next_;

  void LinkTo(PreallocatedStorage* other);
  void Unlink();

  friend class Isolate;

  DISALLOW_IMPLICIT_CONSTRUCTORS(PreallocatedStorage);
};


struct PreallocatedStorageAllocationPolicy {
  INLINE(void* New(size_t size));
  INLINE(static void Delete(void* ptr));
};


} }  // namespace v8::internal

#endif  // V8_ALLOCATION_H_
