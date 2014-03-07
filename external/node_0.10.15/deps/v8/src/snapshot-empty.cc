// Copyright 2006-2008 the V8 project authors. All rights reserved.
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

// Used for building without snapshots.

#include "v8.h"

#include "snapshot.h"

namespace v8 {
namespace internal {

const byte Snapshot::data_[] = { 0 };
const byte* Snapshot::raw_data_ = NULL;
const int Snapshot::size_ = 0;
const int Snapshot::raw_size_ = 0;
const byte Snapshot::context_data_[] = { 0 };
const byte* Snapshot::context_raw_data_ = NULL;
const int Snapshot::context_size_ = 0;
const int Snapshot::context_raw_size_ = 0;

const int Snapshot::new_space_used_ = 0;
const int Snapshot::pointer_space_used_ = 0;
const int Snapshot::data_space_used_ = 0;
const int Snapshot::code_space_used_ = 0;
const int Snapshot::map_space_used_ = 0;
const int Snapshot::cell_space_used_ = 0;

const int Snapshot::context_new_space_used_ = 0;
const int Snapshot::context_pointer_space_used_ = 0;
const int Snapshot::context_data_space_used_ = 0;
const int Snapshot::context_code_space_used_ = 0;
const int Snapshot::context_map_space_used_ = 0;
const int Snapshot::context_cell_space_used_ = 0;

} }  // namespace v8::internal
