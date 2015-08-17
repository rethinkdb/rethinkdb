// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_HYDROGEN_MARK_UNREACHABLE_H_
#define V8_HYDROGEN_MARK_UNREACHABLE_H_

#include "src/hydrogen.h"

namespace v8 {
namespace internal {


class HMarkUnreachableBlocksPhase : public HPhase {
 public:
  explicit HMarkUnreachableBlocksPhase(HGraph* graph)
      : HPhase("H_Mark unreachable blocks", graph) { }

  void Run();

 private:
  void MarkUnreachableBlocks();

  DISALLOW_COPY_AND_ASSIGN(HMarkUnreachableBlocksPhase);
};


} }  // namespace v8::internal

#endif  // V8_HYDROGEN_MARK_UNREACHABLE_H_
