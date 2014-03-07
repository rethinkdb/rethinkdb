// Copyright 2007-2010 the V8 project authors. All rights reserved.
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

#ifndef V8_UNICODE_INL_H_
#define V8_UNICODE_INL_H_

#include "unicode.h"
#include "checks.h"
#include "platform.h"

namespace unibrow {

template <class T, int s> bool Predicate<T, s>::get(uchar code_point) {
  CacheEntry entry = entries_[code_point & kMask];
  if (entry.code_point_ == code_point) return entry.value_;
  return CalculateValue(code_point);
}

template <class T, int s> bool Predicate<T, s>::CalculateValue(
    uchar code_point) {
  bool result = T::Is(code_point);
  entries_[code_point & kMask] = CacheEntry(code_point, result);
  return result;
}

template <class T, int s> int Mapping<T, s>::get(uchar c, uchar n,
    uchar* result) {
  CacheEntry entry = entries_[c & kMask];
  if (entry.code_point_ == c) {
    if (entry.offset_ == 0) {
      return 0;
    } else {
      result[0] = c + entry.offset_;
      return 1;
    }
  } else {
    return CalculateValue(c, n, result);
  }
}

template <class T, int s> int Mapping<T, s>::CalculateValue(uchar c, uchar n,
    uchar* result) {
  bool allow_caching = true;
  int length = T::Convert(c, n, result, &allow_caching);
  if (allow_caching) {
    if (length == 1) {
      entries_[c & kMask] = CacheEntry(c, result[0] - c);
      return 1;
    } else {
      entries_[c & kMask] = CacheEntry(c, 0);
      return 0;
    }
  } else {
    return length;
  }
}


uint16_t Latin1::ConvertNonLatin1ToLatin1(uint16_t c) {
  ASSERT(c > Latin1::kMaxChar);
  switch (c) {
    // This are equivalent characters in unicode.
    case 0x39c:
    case 0x3bc:
      return 0xb5;
    // This is an uppercase of a Latin-1 character
    // outside of Latin-1.
    case 0x178:
      return 0xff;
  }
  return 0;
}


unsigned Utf8::EncodeOneByte(char* str, uint8_t c) {
  static const int kMask = ~(1 << 6);
  if (c <= kMaxOneByteChar) {
    str[0] = c;
    return 1;
  }
  str[0] = 0xC0 | (c >> 6);
  str[1] = 0x80 | (c & kMask);
  return 2;
}


unsigned Utf8::Encode(char* str, uchar c, int previous) {
  static const int kMask = ~(1 << 6);
  if (c <= kMaxOneByteChar) {
    str[0] = c;
    return 1;
  } else if (c <= kMaxTwoByteChar) {
    str[0] = 0xC0 | (c >> 6);
    str[1] = 0x80 | (c & kMask);
    return 2;
  } else if (c <= kMaxThreeByteChar) {
    if (Utf16::IsTrailSurrogate(c) &&
        Utf16::IsLeadSurrogate(previous)) {
      const int kUnmatchedSize = kSizeOfUnmatchedSurrogate;
      return Encode(str - kUnmatchedSize,
                    Utf16::CombineSurrogatePair(previous, c),
                    Utf16::kNoPreviousCharacter) - kUnmatchedSize;
    }
    str[0] = 0xE0 | (c >> 12);
    str[1] = 0x80 | ((c >> 6) & kMask);
    str[2] = 0x80 | (c & kMask);
    return 3;
  } else {
    str[0] = 0xF0 | (c >> 18);
    str[1] = 0x80 | ((c >> 12) & kMask);
    str[2] = 0x80 | ((c >> 6) & kMask);
    str[3] = 0x80 | (c & kMask);
    return 4;
  }
}


uchar Utf8::ValueOf(const byte* bytes, unsigned length, unsigned* cursor) {
  if (length <= 0) return kBadChar;
  byte first = bytes[0];
  // Characters between 0000 and 0007F are encoded as a single character
  if (first <= kMaxOneByteChar) {
    *cursor += 1;
    return first;
  }
  return CalculateValue(bytes, length, cursor);
}

unsigned Utf8::Length(uchar c, int previous) {
  if (c <= kMaxOneByteChar) {
    return 1;
  } else if (c <= kMaxTwoByteChar) {
    return 2;
  } else if (c <= kMaxThreeByteChar) {
    if (Utf16::IsTrailSurrogate(c) &&
        Utf16::IsLeadSurrogate(previous)) {
      return kSizeOfUnmatchedSurrogate - kBytesSavedByCombiningSurrogates;
    }
    return 3;
  } else {
    return 4;
  }
}

Utf8DecoderBase::Utf8DecoderBase()
  : unbuffered_start_(NULL),
    utf16_length_(0),
    last_byte_of_buffer_unused_(false) {}

Utf8DecoderBase::Utf8DecoderBase(uint16_t* buffer,
                                 unsigned buffer_length,
                                 const uint8_t* stream,
                                 unsigned stream_length) {
  Reset(buffer, buffer_length, stream, stream_length);
}

template<unsigned kBufferSize>
Utf8Decoder<kBufferSize>::Utf8Decoder(const char* stream, unsigned length)
  : Utf8DecoderBase(buffer_,
                    kBufferSize,
                    reinterpret_cast<const uint8_t*>(stream),
                    length) {
}

template<unsigned kBufferSize>
void Utf8Decoder<kBufferSize>::Reset(const char* stream, unsigned length) {
  Utf8DecoderBase::Reset(buffer_,
                         kBufferSize,
                         reinterpret_cast<const uint8_t*>(stream),
                         length);
}

template <unsigned kBufferSize>
unsigned Utf8Decoder<kBufferSize>::WriteUtf16(uint16_t* data,
                                              unsigned length) const {
  ASSERT(length > 0);
  if (length > utf16_length_) length = utf16_length_;
  // memcpy everything in buffer.
  unsigned buffer_length =
      last_byte_of_buffer_unused_ ? kBufferSize - 1 : kBufferSize;
  unsigned memcpy_length = length <= buffer_length  ? length : buffer_length;
  v8::internal::OS::MemCopy(data, buffer_, memcpy_length*sizeof(uint16_t));
  if (length <= buffer_length) return length;
  ASSERT(unbuffered_start_ != NULL);
  // Copy the rest the slow way.
  WriteUtf16Slow(unbuffered_start_,
                 data + buffer_length,
                 length - buffer_length);
  return length;
}

}  // namespace unibrow

#endif  // V8_UNICODE_INL_H_
