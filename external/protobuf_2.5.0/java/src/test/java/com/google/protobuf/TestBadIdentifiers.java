// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

package com.google.protobuf;

import junit.framework.TestCase;

/**
 * Tests that proto2 api generation doesn't cause compile errors when
 * compiling protocol buffers that have names that would otherwise conflict
 * if not fully qualified (like @Deprecated and @Override).
 *
 * @author jonp@google.com (Jon Perlow)
 */
public class TestBadIdentifiers extends TestCase {

  public void testCompilation() {
    // If this compiles, it means the generation was correct.
    TestBadIdentifiersProto.Deprecated.newBuilder();
    TestBadIdentifiersProto.Override.newBuilder();
  }

  public void testGetDescriptor() {
    Descriptors.FileDescriptor fileDescriptor =
        TestBadIdentifiersProto.getDescriptor();
    String descriptorField = TestBadIdentifiersProto.Descriptor
        .getDefaultInstance().getDescriptor();
    Descriptors.Descriptor protoDescriptor = TestBadIdentifiersProto.Descriptor
        .getDefaultInstance().getDescriptorForType();
    String nestedDescriptorField = TestBadIdentifiersProto.Descriptor
        .NestedDescriptor.getDefaultInstance().getDescriptor();
    Descriptors.Descriptor nestedProtoDescriptor = TestBadIdentifiersProto
        .Descriptor.NestedDescriptor.getDefaultInstance()
        .getDescriptorForType();
  }
}
