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

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.List;
import java.util.NoSuchElementException;

/**
 * Test {@link LiteralByteString} by setting up a reference string in {@link #setUp()}.
 * This class is designed to be extended for testing extensions of {@link LiteralByteString}
 * such as {@link BoundedByteString}, see {@link BoundedByteStringTest}.
 *
 * @author carlanton@google.com (Carl Haverl)
 */
public class LiteralByteStringTest extends TestCase {
  protected static final String UTF_8 = "UTF-8";

  protected String classUnderTest;
  protected byte[] referenceBytes;
  protected ByteString stringUnderTest;
  protected int expectedHashCode;

  @Override
  protected void setUp() throws Exception {
    classUnderTest = "LiteralByteString";
    referenceBytes = ByteStringTest.getTestBytes(1234, 11337766L);
    stringUnderTest = ByteString.copyFrom(referenceBytes);
    expectedHashCode = 331161852;
  }

  public void testExpectedType() {
    String actualClassName = getActualClassName(stringUnderTest);
    assertEquals(classUnderTest + " should match type exactly", classUnderTest, actualClassName);
  }

  protected String getActualClassName(Object object) {
    String actualClassName = object.getClass().getName();
    actualClassName = actualClassName.substring(actualClassName.lastIndexOf('.') + 1);
    return actualClassName;
  }

  public void testByteAt() {
    boolean stillEqual = true;
    for (int i = 0; stillEqual && i < referenceBytes.length; ++i) {
      stillEqual = (referenceBytes[i] == stringUnderTest.byteAt(i));
    }
    assertTrue(classUnderTest + " must capture the right bytes", stillEqual);
  }

  public void testByteIterator() {
    boolean stillEqual = true;
    ByteString.ByteIterator iter = stringUnderTest.iterator();
    for (int i = 0; stillEqual && i < referenceBytes.length; ++i) {
      stillEqual = (iter.hasNext() && referenceBytes[i] == iter.nextByte());
    }
    assertTrue(classUnderTest + " must capture the right bytes", stillEqual);
    assertFalse(classUnderTest + " must have exhausted the itertor", iter.hasNext());

    try {
      iter.nextByte();
      fail("Should have thrown an exception.");
    } catch (NoSuchElementException e) {
      // This is success
    }
  }

  public void testByteIterable() {
    boolean stillEqual = true;
    int j = 0;
    for (byte quantum : stringUnderTest) {
      stillEqual = (referenceBytes[j] == quantum);
      ++j;
    }
    assertTrue(classUnderTest + " must capture the right bytes as Bytes", stillEqual);
    assertEquals(classUnderTest + " iterable character count", referenceBytes.length, j);
  }

  public void testSize() {
    assertEquals(classUnderTest + " must have the expected size", referenceBytes.length,
        stringUnderTest.size());
  }

  public void testGetTreeDepth() {
    assertEquals(classUnderTest + " must have depth 0", 0, stringUnderTest.getTreeDepth());
  }

  public void testIsBalanced() {
    assertTrue(classUnderTest + " is technically balanced", stringUnderTest.isBalanced());
  }

  public void testCopyTo_ByteArrayOffsetLength() {
    int destinationOffset = 50;
    int length = 100;
    byte[] destination = new byte[destinationOffset + length];
    int sourceOffset = 213;
    stringUnderTest.copyTo(destination, sourceOffset, destinationOffset, length);
    boolean stillEqual = true;
    for (int i = 0; stillEqual && i < length; ++i) {
      stillEqual = referenceBytes[i + sourceOffset] == destination[i + destinationOffset];
    }
    assertTrue(classUnderTest + ".copyTo(4 arg) must give the expected bytes", stillEqual);
  }

  public void testCopyTo_ByteArrayOffsetLengthErrors() {
    int destinationOffset = 50;
    int length = 100;
    byte[] destination = new byte[destinationOffset + length];

    try {
      // Copy one too many bytes
      stringUnderTest.copyTo(destination, stringUnderTest.size() + 1 - length,
          destinationOffset, length);
      fail("Should have thrown an exception when copying too many bytes of a "
          + classUnderTest);
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal negative sourceOffset
      stringUnderTest.copyTo(destination, -1, destinationOffset, length);
      fail("Should have thrown an exception when given a negative sourceOffset in "
          + classUnderTest);
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal negative destinationOffset
      stringUnderTest.copyTo(destination, 0, -1, length);
      fail("Should have thrown an exception when given a negative destinationOffset in "
          + classUnderTest);
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal negative size
      stringUnderTest.copyTo(destination, 0, 0, -1);
      fail("Should have thrown an exception when given a negative size in "
          + classUnderTest);
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal too-large sourceOffset
      stringUnderTest.copyTo(destination, 2 * stringUnderTest.size(), 0, length);
      fail("Should have thrown an exception when the destinationOffset is too large in "
          + classUnderTest);
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }

    try {
      // Copy with illegal too-large destinationOffset
      stringUnderTest.copyTo(destination, 0, 2 * destination.length, length);
      fail("Should have thrown an exception when the destinationOffset is too large in "
          + classUnderTest);
    } catch (IndexOutOfBoundsException expected) {
      // This is success
    }
  }

  public void testCopyTo_ByteBuffer() {
    ByteBuffer myBuffer = ByteBuffer.allocate(referenceBytes.length);
    stringUnderTest.copyTo(myBuffer);
    assertTrue(classUnderTest + ".copyTo(ByteBuffer) must give back the same bytes",
        Arrays.equals(referenceBytes, myBuffer.array()));
  }

  public void testAsReadOnlyByteBuffer() {
    ByteBuffer byteBuffer = stringUnderTest.asReadOnlyByteBuffer();
    byte[] roundTripBytes = new byte[referenceBytes.length];
    assertTrue(byteBuffer.remaining() == referenceBytes.length);
    assertTrue(byteBuffer.isReadOnly());
    byteBuffer.get(roundTripBytes);
    assertTrue(classUnderTest + ".asReadOnlyByteBuffer() must give back the same bytes",
        Arrays.equals(referenceBytes, roundTripBytes));
  }

  public void testAsReadOnlyByteBufferList() {
    List<ByteBuffer> byteBuffers = stringUnderTest.asReadOnlyByteBufferList();
    int bytesSeen = 0;
    byte[] roundTripBytes = new byte[referenceBytes.length];
    for (ByteBuffer byteBuffer : byteBuffers) {
      int thisLength = byteBuffer.remaining();
      assertTrue(byteBuffer.isReadOnly());
      assertTrue(bytesSeen + thisLength <= referenceBytes.length);
      byteBuffer.get(roundTripBytes, bytesSeen, thisLength);
      bytesSeen += thisLength;
    }
    assertTrue(bytesSeen == referenceBytes.length);
    assertTrue(classUnderTest + ".asReadOnlyByteBufferTest() must give back the same bytes",
        Arrays.equals(referenceBytes, roundTripBytes));
  }

  public void testToByteArray() {
    byte[] roundTripBytes = stringUnderTest.toByteArray();
    assertTrue(classUnderTest + ".toByteArray() must give back the same bytes",
        Arrays.equals(referenceBytes, roundTripBytes));
  }

  public void testWriteTo() throws IOException {
    ByteArrayOutputStream bos = new ByteArrayOutputStream();
    stringUnderTest.writeTo(bos);
    byte[] roundTripBytes = bos.toByteArray();
    assertTrue(classUnderTest + ".writeTo() must give back the same bytes",
        Arrays.equals(referenceBytes, roundTripBytes));
  }
  
  public void testWriteTo_mutating() throws IOException {
    OutputStream os = new OutputStream() {
      @Override
      public void write(byte[] b, int off, int len) {
        for (int x = 0; x < len; ++x) {
          b[off + x] = (byte) 0;
        }
      }

      @Override
      public void write(int b) {
        // Purposefully left blank.
      }
    };

    stringUnderTest.writeTo(os);
    byte[] newBytes = stringUnderTest.toByteArray();
    assertTrue(classUnderTest + ".writeTo() must not grant access to underlying array",
        Arrays.equals(referenceBytes, newBytes));
  }

  public void testNewOutput() throws IOException {
    ByteArrayOutputStream bos = new ByteArrayOutputStream();
    ByteString.Output output = ByteString.newOutput();
    stringUnderTest.writeTo(output);
    assertEquals("Output Size returns correct result",
        output.size(), stringUnderTest.size());
    output.writeTo(bos);
    assertTrue("Output.writeTo() must give back the same bytes",
        Arrays.equals(referenceBytes, bos.toByteArray()));

    // write the output stream to itself! This should cause it to double
    output.writeTo(output);
    assertEquals("Writing an output stream to itself is successful",
        stringUnderTest.concat(stringUnderTest), output.toByteString());

    output.reset();
    assertEquals("Output.reset() resets the output", 0, output.size());
    assertEquals("Output.reset() resets the output",
        ByteString.EMPTY, output.toByteString());
    
  }

  public void testToString() throws UnsupportedEncodingException {
    String testString = "I love unicode \u1234\u5678 characters";
    LiteralByteString unicode = new LiteralByteString(testString.getBytes(UTF_8));
    String roundTripString = unicode.toString(UTF_8);
    assertEquals(classUnderTest + " unicode must match", testString, roundTripString);
  }

  public void testEquals() {
    assertEquals(classUnderTest + " must not equal null", false, stringUnderTest.equals(null));
    assertEquals(classUnderTest + " must equal self", stringUnderTest, stringUnderTest);
    assertFalse(classUnderTest + " must not equal the empty string",
        stringUnderTest.equals(ByteString.EMPTY));
    assertEquals(classUnderTest + " empty strings must be equal",
        new LiteralByteString(new byte[]{}), stringUnderTest.substring(55, 55));
    assertEquals(classUnderTest + " must equal another string with the same value",
        stringUnderTest, new LiteralByteString(referenceBytes));

    byte[] mungedBytes = new byte[referenceBytes.length];
    System.arraycopy(referenceBytes, 0, mungedBytes, 0, referenceBytes.length);
    mungedBytes[mungedBytes.length - 5] ^= 0xFF;
    assertFalse(classUnderTest + " must not equal every string with the same length",
        stringUnderTest.equals(new LiteralByteString(mungedBytes)));
  }

  public void testHashCode() {
    int hash = stringUnderTest.hashCode();
    assertEquals(classUnderTest + " must have expected hashCode", expectedHashCode, hash);
  }

  public void testPeekCachedHashCode() {
    assertEquals(classUnderTest + ".peekCachedHashCode() should return zero at first", 0,
        stringUnderTest.peekCachedHashCode());
    stringUnderTest.hashCode();
    assertEquals(classUnderTest + ".peekCachedHashCode should return zero at first",
        expectedHashCode, stringUnderTest.peekCachedHashCode());
  }

  public void testPartialHash() {
    // partialHash() is more strenuously tested elsewhere by testing hashes of substrings.
    // This test would fail if the expected hash were 1.  It's not.
    int hash = stringUnderTest.partialHash(stringUnderTest.size(), 0, stringUnderTest.size());
    assertEquals(classUnderTest + ".partialHash() must yield expected hashCode",
        expectedHashCode, hash);
  }

  public void testNewInput() throws IOException {
    InputStream input = stringUnderTest.newInput();
    assertEquals("InputStream.available() returns correct value",
        stringUnderTest.size(), input.available());
    boolean stillEqual = true;
    for (byte referenceByte : referenceBytes) {
      int expectedInt = (referenceByte & 0xFF);
      stillEqual = (expectedInt == input.read());
    }
    assertEquals("InputStream.available() returns correct value",
        0, input.available());
    assertTrue(classUnderTest + " must give the same bytes from the InputStream", stillEqual);
    assertEquals(classUnderTest + " InputStream must now be exhausted", -1, input.read());
  }

  public void testNewInput_skip() throws IOException {
    InputStream input = stringUnderTest.newInput();
    int stringSize = stringUnderTest.size();
    int nearEndIndex = stringSize * 2 / 3;
    long skipped1 = input.skip(nearEndIndex);
    assertEquals("InputStream.skip()", skipped1, nearEndIndex);
    assertEquals("InputStream.available()",
        stringSize - skipped1, input.available());
    assertTrue("InputStream.mark() is available", input.markSupported());
    input.mark(0);
    assertEquals("InputStream.skip(), read()",
        stringUnderTest.byteAt(nearEndIndex) & 0xFF, input.read());
    assertEquals("InputStream.available()",
                 stringSize - skipped1 - 1, input.available());
    long skipped2 = input.skip(stringSize);
    assertEquals("InputStream.skip() incomplete",
        skipped2, stringSize - skipped1 - 1);
    assertEquals("InputStream.skip(), no more input", 0, input.available());
    assertEquals("InputStream.skip(), no more input", -1, input.read());
    input.reset();
    assertEquals("InputStream.reset() succeded",
                 stringSize - skipped1, input.available());
    assertEquals("InputStream.reset(), read()",
        stringUnderTest.byteAt(nearEndIndex) & 0xFF, input.read());
  }

  public void testNewCodedInput() throws IOException {
    CodedInputStream cis = stringUnderTest.newCodedInput();
    byte[] roundTripBytes = cis.readRawBytes(referenceBytes.length);
    assertTrue(classUnderTest + " must give the same bytes back from the CodedInputStream",
        Arrays.equals(referenceBytes, roundTripBytes));
    assertTrue(classUnderTest + " CodedInputStream must now be exhausted", cis.isAtEnd());
  }

  /**
   * Make sure we keep things simple when concatenating with empty. See also
   * {@link ByteStringTest#testConcat_empty()}.
   */
  public void testConcat_empty() {
    assertSame(classUnderTest + " concatenated with empty must give " + classUnderTest,
        stringUnderTest.concat(ByteString.EMPTY), stringUnderTest);
    assertSame("empty concatenated with " + classUnderTest + " must give " + classUnderTest,
        ByteString.EMPTY.concat(stringUnderTest), stringUnderTest);
  }
}
