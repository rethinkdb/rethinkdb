// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_COMPILER_SOURCE_POSITION_H_
#define V8_COMPILER_SOURCE_POSITION_H_

#include "src/assembler.h"
#include "src/compiler/node-aux-data.h"

namespace v8 {
namespace internal {
namespace compiler {

// Encapsulates encoding and decoding of sources positions from which Nodes
// originated.
class SourcePosition FINAL {
 public:
  explicit SourcePosition(int raw = kUnknownPosition) : raw_(raw) {}

  static SourcePosition Unknown() { return SourcePosition(kUnknownPosition); }
  bool IsUnknown() const { return raw() == kUnknownPosition; }

  static SourcePosition Invalid() { return SourcePosition(kInvalidPosition); }
  bool IsInvalid() const { return raw() == kInvalidPosition; }

  int raw() const { return raw_; }

 private:
  static const int kInvalidPosition = -2;
  static const int kUnknownPosition = RelocInfo::kNoPosition;
  STATIC_ASSERT(kInvalidPosition != kUnknownPosition);
  int raw_;
};


inline bool operator==(const SourcePosition& lhs, const SourcePosition& rhs) {
  return lhs.raw() == rhs.raw();
}

inline bool operator!=(const SourcePosition& lhs, const SourcePosition& rhs) {
  return !(lhs == rhs);
}


class SourcePositionTable FINAL {
 public:
  class Scope {
   public:
    Scope(SourcePositionTable* source_positions, SourcePosition position)
        : source_positions_(source_positions),
          prev_position_(source_positions->current_position_) {
      Init(position);
    }
    Scope(SourcePositionTable* source_positions, Node* node)
        : source_positions_(source_positions),
          prev_position_(source_positions->current_position_) {
      Init(source_positions_->GetSourcePosition(node));
    }
    ~Scope() { source_positions_->current_position_ = prev_position_; }

   private:
    void Init(SourcePosition position) {
      if (!position.IsUnknown() || prev_position_.IsInvalid()) {
        source_positions_->current_position_ = position;
      }
    }

    SourcePositionTable* source_positions_;
    SourcePosition prev_position_;
    DISALLOW_COPY_AND_ASSIGN(Scope);
  };

  explicit SourcePositionTable(Graph* graph);
  ~SourcePositionTable() {
    if (decorator_ != NULL) RemoveDecorator();
  }

  void AddDecorator();
  void RemoveDecorator();

  SourcePosition GetSourcePosition(Node* node) const;

 private:
  class Decorator;

  Graph* graph_;
  Decorator* decorator_;
  SourcePosition current_position_;
  NodeAuxData<SourcePosition> table_;

  DISALLOW_COPY_AND_ASSIGN(SourcePositionTable);
};

}  // namespace compiler
}  // namespace internal
}  // namespace v8

#endif
