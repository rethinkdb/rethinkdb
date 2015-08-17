// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_HYDROGEN_DCE_H_
#define V8_HYDROGEN_DCE_H_

#include "src/hydrogen.h"

namespace v8 {
namespace internal {


class HDeadCodeEliminationPhase : public HPhase {
 public:
  explicit HDeadCodeEliminationPhase(HGraph* graph)
      : HPhase("H_Dead code elimination", graph) { }

  void Run() {
    MarkLiveInstructions();
    RemoveDeadInstructions();
  }

 private:
  void MarkLive(HValue* instr, ZoneList<HValue*>* worklist);
  void PrintLive(HValue* ref, HValue* instr);
  void MarkLiveInstructions();
  void RemoveDeadInstructions();
};


} }  // namespace v8::internal

#endif  // V8_HYDROGEN_DCE_H_
