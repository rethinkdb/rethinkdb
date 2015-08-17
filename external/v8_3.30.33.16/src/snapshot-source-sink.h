// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_SNAPSHOT_SOURCE_SINK_H_
#define V8_SNAPSHOT_SOURCE_SINK_H_

#include "src/base/logging.h"
#include "src/utils.h"

namespace v8 {
namespace internal {


/**
 * Source to read snapshot and builtins files from.
 *
 * Note: Memory ownership remains with callee.
 */
class SnapshotByteSource FINAL {
 public:
  SnapshotByteSource(const byte* array, int length);
  ~SnapshotByteSource();

  bool HasMore() { return position_ < length_; }

  int Get() {
    DCHECK(position_ < length_);
    return data_[position_++];
  }

  int32_t GetUnalignedInt();

  void Advance(int by) { position_ += by; }

  void CopyRaw(byte* to, int number_of_bytes);

  inline int GetInt() {
    // This way of variable-length encoding integers does not suffer from branch
    // mispredictions.
    uint32_t answer = GetUnalignedInt();
    int bytes = (answer & 3) + 1;
    Advance(bytes);
    uint32_t mask = 0xffffffffu;
    mask >>= 32 - (bytes << 3);
    answer &= mask;
    answer >>= 2;
    return answer;
  }

  bool GetBlob(const byte** data, int* number_of_bytes);

  bool AtEOF();

  int position() { return position_; }

 private:
  const byte* data_;
  int length_;
  int position_;

  DISALLOW_COPY_AND_ASSIGN(SnapshotByteSource);
};


/**
 * Sink to write snapshot files to.
 *
 * Subclasses must implement actual storage or i/o.
 */
class SnapshotByteSink {
 public:
  SnapshotByteSink() {}
  explicit SnapshotByteSink(int initial_size) : data_(initial_size) {}

  ~SnapshotByteSink() {}

  void Put(byte b, const char* description) { data_.Add(b); }

  void PutSection(int b, const char* description) {
    DCHECK_LE(b, kMaxUInt8);
    Put(static_cast<byte>(b), description);
  }

  void PutInt(uintptr_t integer, const char* description);
  void PutRaw(byte* data, int number_of_bytes, const char* description);
  void PutBlob(byte* data, int number_of_bytes, const char* description);
  int Position() { return data_.length(); }

  const List<byte>& data() const { return data_; }

 private:
  List<byte> data_;
};

}  // namespace v8::internal
}  // namespace v8

#endif  // V8_SNAPSHOT_SOURCE_SINK_H_
