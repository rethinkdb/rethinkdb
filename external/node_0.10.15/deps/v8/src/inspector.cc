// Copyright 2011 the V8 project authors. All rights reserved.
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


#include "v8.h"
#include "inspector.h"


namespace v8 {
namespace internal {

#ifdef INSPECTOR

//============================================================================
// The Inspector.

void Inspector::DumpObjectType(FILE* out, Object* obj, bool print_more) {
  // Dump the object pointer.
  OS::FPrint(out, "%p:", reinterpret_cast<void*>(obj));
  if (obj->IsHeapObject()) {
    HeapObject* hobj = HeapObject::cast(obj);
    OS::FPrint(out, " size %d :", hobj->Size());
  }

  // Dump each object classification that matches this object.
#define FOR_EACH_TYPE(type)   \
  if (obj->Is##type()) {      \
    OS::FPrint(out, " %s", #type);    \
  }
  OBJECT_TYPE_LIST(FOR_EACH_TYPE)
  HEAP_OBJECT_TYPE_LIST(FOR_EACH_TYPE)
#undef FOR_EACH_TYPE
}


#endif  // INSPECTOR

} }  // namespace v8::internal

