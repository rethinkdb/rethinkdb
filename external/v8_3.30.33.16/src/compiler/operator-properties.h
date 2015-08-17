// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_COMPILER_OPERATOR_PROPERTIES_H_
#define V8_COMPILER_OPERATOR_PROPERTIES_H_

namespace v8 {
namespace internal {
namespace compiler {

class Operator;

class OperatorProperties {
 public:
  static inline bool HasContextInput(const Operator* op);
  static inline bool HasFrameStateInput(const Operator* op);

  static inline int GetContextInputCount(const Operator* op);
  static inline int GetFrameStateInputCount(const Operator* op);
  static inline int GetTotalInputCount(const Operator* op);

  static inline bool IsBasicBlockBegin(const Operator* op);
};

}  // namespace compiler
}  // namespace internal
}  // namespace v8

#endif  // V8_COMPILER_OPERATOR_PROPERTIES_H_
