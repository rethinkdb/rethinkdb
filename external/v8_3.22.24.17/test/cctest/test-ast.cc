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

#include <stdlib.h>

#include "v8.h"

#include "ast.h"
#include "cctest.h"

using namespace v8::internal;

TEST(List) {
  v8::internal::V8::Initialize(NULL);
  List<AstNode*>* list = new List<AstNode*>(0);
  CHECK_EQ(0, list->length());

  Isolate* isolate = CcTest::i_isolate();
  Zone zone(isolate);
  AstNodeFactory<AstNullVisitor> factory(isolate, &zone);
  AstNode* node = factory.NewEmptyStatement(RelocInfo::kNoPosition);
  list->Add(node);
  CHECK_EQ(1, list->length());
  CHECK_EQ(node, list->at(0));
  CHECK_EQ(node, list->last());

  const int kElements = 100;
  for (int i = 0; i < kElements; i++) {
    list->Add(node);
  }
  CHECK_EQ(1 + kElements, list->length());

  list->Clear();
  CHECK_EQ(0, list->length());
  delete list;
}
