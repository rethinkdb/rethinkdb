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

#include "scanner-character-streams.h"

#include "handles.h"
#include "unicode-inl.h"

namespace v8 {
namespace internal {

// ----------------------------------------------------------------------------
// BufferedUtf16CharacterStreams

BufferedUtf16CharacterStream::BufferedUtf16CharacterStream()
    : Utf16CharacterStream(),
      pushback_limit_(NULL) {
  // Initialize buffer as being empty. First read will fill the buffer.
  buffer_cursor_ = buffer_;
  buffer_end_ = buffer_;
}


BufferedUtf16CharacterStream::~BufferedUtf16CharacterStream() { }

void BufferedUtf16CharacterStream::PushBack(uc32 character) {
  if (character == kEndOfInput) {
    pos_--;
    return;
  }
  if (pushback_limit_ == NULL && buffer_cursor_ > buffer_) {
    // buffer_ is writable, buffer_cursor_ is const pointer.
    buffer_[--buffer_cursor_ - buffer_] = static_cast<uc16>(character);
    pos_--;
    return;
  }
  SlowPushBack(static_cast<uc16>(character));
}


void BufferedUtf16CharacterStream::SlowPushBack(uc16 character) {
  // In pushback mode, the end of the buffer contains pushback,
  // and the start of the buffer (from buffer start to pushback_limit_)
  // contains valid data that comes just after the pushback.
  // We NULL the pushback_limit_ if pushing all the way back to the
  // start of the buffer.

  if (pushback_limit_ == NULL) {
    // Enter pushback mode.
    pushback_limit_ = buffer_end_;
    buffer_end_ = buffer_ + kBufferSize;
    buffer_cursor_ = buffer_end_;
  }
  // Ensure that there is room for at least one pushback.
  ASSERT(buffer_cursor_ > buffer_);
  ASSERT(pos_ > 0);
  buffer_[--buffer_cursor_ - buffer_] = character;
  if (buffer_cursor_ == buffer_) {
    pushback_limit_ = NULL;
  } else if (buffer_cursor_ < pushback_limit_) {
    pushback_limit_ = buffer_cursor_;
  }
  pos_--;
}


bool BufferedUtf16CharacterStream::ReadBlock() {
  buffer_cursor_ = buffer_;
  if (pushback_limit_ != NULL) {
    // Leave pushback mode.
    buffer_end_ = pushback_limit_;
    pushback_limit_ = NULL;
    // If there were any valid characters left at the
    // start of the buffer, use those.
    if (buffer_cursor_ < buffer_end_) return true;
    // Otherwise read a new block.
  }
  unsigned length = FillBuffer(pos_, kBufferSize);
  buffer_end_ = buffer_ + length;
  return length > 0;
}


unsigned BufferedUtf16CharacterStream::SlowSeekForward(unsigned delta) {
  // Leave pushback mode (i.e., ignore that there might be valid data
  // in the buffer before the pushback_limit_ point).
  pushback_limit_ = NULL;
  return BufferSeekForward(delta);
}


// ----------------------------------------------------------------------------
// GenericStringUtf16CharacterStream


GenericStringUtf16CharacterStream::GenericStringUtf16CharacterStream(
    Handle<String> data,
    unsigned start_position,
    unsigned end_position)
    : string_(data),
      length_(end_position) {
  ASSERT(end_position >= start_position);
  buffer_cursor_ = buffer_;
  buffer_end_ = buffer_;
  pos_ = start_position;
}


GenericStringUtf16CharacterStream::~GenericStringUtf16CharacterStream() { }


unsigned GenericStringUtf16CharacterStream::BufferSeekForward(unsigned delta) {
  unsigned old_pos = pos_;
  pos_ = Min(pos_ + delta, length_);
  ReadBlock();
  return pos_ - old_pos;
}


unsigned GenericStringUtf16CharacterStream::FillBuffer(unsigned from_pos,
                                                      unsigned length) {
  if (from_pos >= length_) return 0;
  if (from_pos + length > length_) {
    length = length_ - from_pos;
  }
  String::WriteToFlat<uc16>(*string_, buffer_, from_pos, from_pos + length);
  return length;
}


// ----------------------------------------------------------------------------
// Utf8ToUtf16CharacterStream
Utf8ToUtf16CharacterStream::Utf8ToUtf16CharacterStream(const byte* data,
                                                       unsigned length)
    : BufferedUtf16CharacterStream(),
      raw_data_(data),
      raw_data_length_(length),
      raw_data_pos_(0),
      raw_character_position_(0) {
  ReadBlock();
}


Utf8ToUtf16CharacterStream::~Utf8ToUtf16CharacterStream() { }


unsigned Utf8ToUtf16CharacterStream::BufferSeekForward(unsigned delta) {
  unsigned old_pos = pos_;
  unsigned target_pos = pos_ + delta;
  SetRawPosition(target_pos);
  pos_ = raw_character_position_;
  ReadBlock();
  return pos_ - old_pos;
}


unsigned Utf8ToUtf16CharacterStream::FillBuffer(unsigned char_position,
                                                unsigned length) {
  static const unibrow::uchar kMaxUtf16Character = 0xffff;
  SetRawPosition(char_position);
  if (raw_character_position_ != char_position) {
    // char_position was not a valid position in the stream (hit the end
    // while spooling to it).
    return 0u;
  }
  unsigned i = 0;
  while (i < length - 1) {
    if (raw_data_pos_ == raw_data_length_) break;
    unibrow::uchar c = raw_data_[raw_data_pos_];
    if (c <= unibrow::Utf8::kMaxOneByteChar) {
      raw_data_pos_++;
    } else {
      c =  unibrow::Utf8::CalculateValue(raw_data_ + raw_data_pos_,
                                         raw_data_length_ - raw_data_pos_,
                                         &raw_data_pos_);
    }
    if (c > kMaxUtf16Character) {
      buffer_[i++] = unibrow::Utf16::LeadSurrogate(c);
      buffer_[i++] = unibrow::Utf16::TrailSurrogate(c);
    } else {
      buffer_[i++] = static_cast<uc16>(c);
    }
  }
  raw_character_position_ = char_position + i;
  return i;
}


static const byte kUtf8MultiByteMask = 0xC0;
static const byte kUtf8MultiByteCharStart = 0xC0;
static const byte kUtf8MultiByteCharFollower = 0x80;


#ifdef DEBUG
static bool IsUtf8MultiCharacterStart(byte first_byte) {
  return (first_byte & kUtf8MultiByteMask) == kUtf8MultiByteCharStart;
}
#endif


static bool IsUtf8MultiCharacterFollower(byte later_byte) {
  return (later_byte & kUtf8MultiByteMask) == kUtf8MultiByteCharFollower;
}


// Move the cursor back to point at the preceding UTF-8 character start
// in the buffer.
static inline void Utf8CharacterBack(const byte* buffer, unsigned* cursor) {
  byte character = buffer[--*cursor];
  if (character > unibrow::Utf8::kMaxOneByteChar) {
    ASSERT(IsUtf8MultiCharacterFollower(character));
    // Last byte of a multi-byte character encoding. Step backwards until
    // pointing to the first byte of the encoding, recognized by having the
    // top two bits set.
    while (IsUtf8MultiCharacterFollower(buffer[--*cursor])) { }
    ASSERT(IsUtf8MultiCharacterStart(buffer[*cursor]));
  }
}


// Move the cursor forward to point at the next following UTF-8 character start
// in the buffer.
static inline void Utf8CharacterForward(const byte* buffer, unsigned* cursor) {
  byte character = buffer[(*cursor)++];
  if (character > unibrow::Utf8::kMaxOneByteChar) {
    // First character of a multi-byte character encoding.
    // The number of most-significant one-bits determines the length of the
    // encoding:
    //  110..... - (0xCx, 0xDx) one additional byte (minimum).
    //  1110.... - (0xEx) two additional bytes.
    //  11110... - (0xFx) three additional bytes (maximum).
    ASSERT(IsUtf8MultiCharacterStart(character));
    // Additional bytes is:
    // 1 if value in range 0xC0 .. 0xDF.
    // 2 if value in range 0xE0 .. 0xEF.
    // 3 if value in range 0xF0 .. 0xF7.
    // Encode that in a single value.
    unsigned additional_bytes =
        ((0x3211u) >> (((character - 0xC0) >> 2) & 0xC)) & 0x03;
    *cursor += additional_bytes;
    ASSERT(!IsUtf8MultiCharacterFollower(buffer[1 + additional_bytes]));
  }
}


// This can't set a raw position between two surrogate pairs, since there
// is no position in the UTF8 stream that corresponds to that.  This assumes
// that the surrogate pair is correctly coded as a 4 byte UTF-8 sequence.  If
// it is illegally coded as two 3 byte sequences then there is no problem here.
void Utf8ToUtf16CharacterStream::SetRawPosition(unsigned target_position) {
  if (raw_character_position_ > target_position) {
    // Spool backwards in utf8 buffer.
    do {
      int old_pos = raw_data_pos_;
      Utf8CharacterBack(raw_data_, &raw_data_pos_);
      raw_character_position_--;
      ASSERT(old_pos - raw_data_pos_ <= 4);
      // Step back over both code units for surrogate pairs.
      if (old_pos - raw_data_pos_ == 4) raw_character_position_--;
    } while (raw_character_position_ > target_position);
    // No surrogate pair splitting.
    ASSERT(raw_character_position_ == target_position);
    return;
  }
  // Spool forwards in the utf8 buffer.
  while (raw_character_position_ < target_position) {
    if (raw_data_pos_ == raw_data_length_) return;
    int old_pos = raw_data_pos_;
    Utf8CharacterForward(raw_data_, &raw_data_pos_);
    raw_character_position_++;
    ASSERT(raw_data_pos_ - old_pos <= 4);
    if (raw_data_pos_ - old_pos == 4) raw_character_position_++;
  }
  // No surrogate pair splitting.
  ASSERT(raw_character_position_ == target_position);
}


// ----------------------------------------------------------------------------
// ExternalTwoByteStringUtf16CharacterStream

ExternalTwoByteStringUtf16CharacterStream::
    ~ExternalTwoByteStringUtf16CharacterStream() { }


ExternalTwoByteStringUtf16CharacterStream
    ::ExternalTwoByteStringUtf16CharacterStream(
        Handle<ExternalTwoByteString> data,
        int start_position,
        int end_position)
    : Utf16CharacterStream(),
      source_(data),
      raw_data_(data->GetTwoByteData(start_position)) {
  buffer_cursor_ = raw_data_,
  buffer_end_ = raw_data_ + (end_position - start_position);
  pos_ = start_position;
}

} }  // namespace v8::internal
