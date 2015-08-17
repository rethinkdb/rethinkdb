// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_UNICODE_DECODER_H_
#define V8_UNICODE_DECODER_H_

#include <sys/types.h>
#include "src/globals.h"

namespace unibrow {

class Utf8DecoderBase {
 public:
  // Initialization done in subclass.
  inline Utf8DecoderBase();
  inline Utf8DecoderBase(uint16_t* buffer, unsigned buffer_length,
                         const uint8_t* stream, unsigned stream_length);
  inline unsigned Utf16Length() const { return utf16_length_; }

 protected:
  // This reads all characters and sets the utf16_length_.
  // The first buffer_length utf16 chars are cached in the buffer.
  void Reset(uint16_t* buffer, unsigned buffer_length, const uint8_t* stream,
             unsigned stream_length);
  static void WriteUtf16Slow(const uint8_t* stream, uint16_t* data,
                             unsigned length);
  const uint8_t* unbuffered_start_;
  unsigned utf16_length_;
  bool last_byte_of_buffer_unused_;

 private:
  DISALLOW_COPY_AND_ASSIGN(Utf8DecoderBase);
};

template <unsigned kBufferSize>
class Utf8Decoder : public Utf8DecoderBase {
 public:
  inline Utf8Decoder() {}
  inline Utf8Decoder(const char* stream, unsigned length);
  inline void Reset(const char* stream, unsigned length);
  inline unsigned WriteUtf16(uint16_t* data, unsigned length) const;

 private:
  uint16_t buffer_[kBufferSize];
};


Utf8DecoderBase::Utf8DecoderBase()
    : unbuffered_start_(NULL),
      utf16_length_(0),
      last_byte_of_buffer_unused_(false) {}


Utf8DecoderBase::Utf8DecoderBase(uint16_t* buffer, unsigned buffer_length,
                                 const uint8_t* stream,
                                 unsigned stream_length) {
  Reset(buffer, buffer_length, stream, stream_length);
}


template <unsigned kBufferSize>
Utf8Decoder<kBufferSize>::Utf8Decoder(const char* stream, unsigned length)
    : Utf8DecoderBase(buffer_, kBufferSize,
                      reinterpret_cast<const uint8_t*>(stream), length) {}


template <unsigned kBufferSize>
void Utf8Decoder<kBufferSize>::Reset(const char* stream, unsigned length) {
  Utf8DecoderBase::Reset(buffer_, kBufferSize,
                         reinterpret_cast<const uint8_t*>(stream), length);
}


template <unsigned kBufferSize>
unsigned Utf8Decoder<kBufferSize>::WriteUtf16(uint16_t* data,
                                              unsigned length) const {
  DCHECK(length > 0);
  if (length > utf16_length_) length = utf16_length_;
  // memcpy everything in buffer.
  unsigned buffer_length =
      last_byte_of_buffer_unused_ ? kBufferSize - 1 : kBufferSize;
  unsigned memcpy_length = length <= buffer_length ? length : buffer_length;
  v8::internal::MemCopy(data, buffer_, memcpy_length * sizeof(uint16_t));
  if (length <= buffer_length) return length;
  DCHECK(unbuffered_start_ != NULL);
  // Copy the rest the slow way.
  WriteUtf16Slow(unbuffered_start_, data + buffer_length,
                 length - buffer_length);
  return length;
}

class Latin1 {
 public:
  static const unsigned kMaxChar = 0xff;
  // Returns 0 if character does not convert to single latin-1 character
  // or if the character doesn't not convert back to latin-1 via inverse
  // operation (upper to lower, etc).
  static inline uint16_t ConvertNonLatin1ToLatin1(uint16_t);
};


uint16_t Latin1::ConvertNonLatin1ToLatin1(uint16_t c) {
  DCHECK(c > Latin1::kMaxChar);
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


}  // namespace unibrow

#endif  // V8_UNICODE_DECODER_H_
