// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_DATAFLOW_H_
#define V8_DATAFLOW_H_

#include "src/v8.h"

#include "src/allocation.h"
#include "src/ast.h"
#include "src/compiler.h"
#include "src/zone-inl.h"

namespace v8 {
namespace internal {

class BitVector : public ZoneObject {
 public:
  // Iterator for the elements of this BitVector.
  class Iterator BASE_EMBEDDED {
   public:
    explicit Iterator(BitVector* target)
        : target_(target),
          current_index_(0),
          current_value_(target->data_[0]),
          current_(-1) {
      DCHECK(target->data_length_ > 0);
      Advance();
    }
    ~Iterator() {}

    bool Done() const { return current_index_ >= target_->data_length_; }
    void Advance();

    int Current() const {
      DCHECK(!Done());
      return current_;
    }

   private:
    uintptr_t SkipZeroBytes(uintptr_t val) {
      while ((val & 0xFF) == 0) {
        val >>= 8;
        current_ += 8;
      }
      return val;
    }
    uintptr_t SkipZeroBits(uintptr_t val) {
      while ((val & 0x1) == 0) {
        val >>= 1;
        current_++;
      }
      return val;
    }

    BitVector* target_;
    int current_index_;
    uintptr_t current_value_;
    int current_;

    friend class BitVector;
  };

  static const int kDataBits = kPointerSize * 8;
  static const int kDataBitShift = kPointerSize == 8 ? 6 : 5;
  static const uintptr_t kOne = 1;  // This saves some static_casts.

  BitVector(int length, Zone* zone)
      : length_(length),
        data_length_(SizeFor(length)),
        data_(zone->NewArray<uintptr_t>(data_length_)) {
    DCHECK(length > 0);
    Clear();
  }

  BitVector(const BitVector& other, Zone* zone)
      : length_(other.length()),
        data_length_(SizeFor(length_)),
        data_(zone->NewArray<uintptr_t>(data_length_)) {
    CopyFrom(other);
  }

  static int SizeFor(int length) { return 1 + ((length - 1) / kDataBits); }

  void CopyFrom(const BitVector& other) {
    DCHECK(other.length() <= length());
    for (int i = 0; i < other.data_length_; i++) {
      data_[i] = other.data_[i];
    }
    for (int i = other.data_length_; i < data_length_; i++) {
      data_[i] = 0;
    }
  }

  bool Contains(int i) const {
    DCHECK(i >= 0 && i < length());
    uintptr_t block = data_[i / kDataBits];
    return (block & (kOne << (i % kDataBits))) != 0;
  }

  void Add(int i) {
    DCHECK(i >= 0 && i < length());
    data_[i / kDataBits] |= (kOne << (i % kDataBits));
  }

  void Remove(int i) {
    DCHECK(i >= 0 && i < length());
    data_[i / kDataBits] &= ~(kOne << (i % kDataBits));
  }

  void Union(const BitVector& other) {
    DCHECK(other.length() == length());
    for (int i = 0; i < data_length_; i++) {
      data_[i] |= other.data_[i];
    }
  }

  bool UnionIsChanged(const BitVector& other) {
    DCHECK(other.length() == length());
    bool changed = false;
    for (int i = 0; i < data_length_; i++) {
      uintptr_t old_data = data_[i];
      data_[i] |= other.data_[i];
      if (data_[i] != old_data) changed = true;
    }
    return changed;
  }

  void Intersect(const BitVector& other) {
    DCHECK(other.length() == length());
    for (int i = 0; i < data_length_; i++) {
      data_[i] &= other.data_[i];
    }
  }

  bool IntersectIsChanged(const BitVector& other) {
    DCHECK(other.length() == length());
    bool changed = false;
    for (int i = 0; i < data_length_; i++) {
      uintptr_t old_data = data_[i];
      data_[i] &= other.data_[i];
      if (data_[i] != old_data) changed = true;
    }
    return changed;
  }

  void Subtract(const BitVector& other) {
    DCHECK(other.length() == length());
    for (int i = 0; i < data_length_; i++) {
      data_[i] &= ~other.data_[i];
    }
  }

  void Clear() {
    for (int i = 0; i < data_length_; i++) {
      data_[i] = 0;
    }
  }

  bool IsEmpty() const {
    for (int i = 0; i < data_length_; i++) {
      if (data_[i] != 0) return false;
    }
    return true;
  }

  bool Equals(const BitVector& other) {
    for (int i = 0; i < data_length_; i++) {
      if (data_[i] != other.data_[i]) return false;
    }
    return true;
  }

  int Count() const;

  int length() const { return length_; }

#ifdef DEBUG
  void Print();
#endif

 private:
  const int length_;
  const int data_length_;
  uintptr_t* const data_;

  DISALLOW_COPY_AND_ASSIGN(BitVector);
};


class GrowableBitVector BASE_EMBEDDED {
 public:
  class Iterator BASE_EMBEDDED {
   public:
    Iterator(const GrowableBitVector* target, Zone* zone)
        : it_(target->bits_ == NULL ? new (zone) BitVector(1, zone)
                                    : target->bits_) {}
    bool Done() const { return it_.Done(); }
    void Advance() { it_.Advance(); }
    int Current() const { return it_.Current(); }

   private:
    BitVector::Iterator it_;
  };

  GrowableBitVector() : bits_(NULL) {}
  GrowableBitVector(int length, Zone* zone)
      : bits_(new (zone) BitVector(length, zone)) {}

  bool Contains(int value) const {
    if (!InBitsRange(value)) return false;
    return bits_->Contains(value);
  }

  void Add(int value, Zone* zone) {
    EnsureCapacity(value, zone);
    bits_->Add(value);
  }

  void Union(const GrowableBitVector& other, Zone* zone) {
    for (Iterator it(&other, zone); !it.Done(); it.Advance()) {
      Add(it.Current(), zone);
    }
  }

  void Clear() {
    if (bits_ != NULL) bits_->Clear();
  }

 private:
  static const int kInitialLength = 1024;

  bool InBitsRange(int value) const {
    return bits_ != NULL && bits_->length() > value;
  }

  void EnsureCapacity(int value, Zone* zone) {
    if (InBitsRange(value)) return;
    int new_length = bits_ == NULL ? kInitialLength : bits_->length();
    while (new_length <= value) new_length *= 2;
    BitVector* new_bits = new (zone) BitVector(new_length, zone);
    if (bits_ != NULL) new_bits->CopyFrom(*bits_);
    bits_ = new_bits;
  }

  BitVector* bits_;
};

}  // namespace internal
}  // namespace v8

#endif  // V8_DATAFLOW_H_
