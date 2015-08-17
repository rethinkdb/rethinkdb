// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_COMPILER_AST_LOOP_ASSIGNMENT_ANALYZER_H_
#define V8_COMPILER_AST_LOOP_ASSIGNMENT_ANALYZER_H_

#include "src/ast.h"
#include "src/bit-vector.h"
#include "src/v8.h"
#include "src/zone-containers.h"

namespace v8 {
namespace internal {

class Variable;
class Scope;

namespace compiler {

// The result of analyzing loop assignments.
class LoopAssignmentAnalysis : public ZoneObject {
 public:
  BitVector* GetVariablesAssignedInLoop(IterationStatement* loop) {
    for (size_t i = 0; i < list_.size(); i++) {
      // TODO(turbofan): hashmap or binary search for loop assignments.
      if (list_[i].first == loop) return list_[i].second;
    }
    UNREACHABLE();  // should never ask for loops that aren't here!
    return NULL;
  }

  int GetAssignmentCountForTesting(Scope* scope, Variable* var);

 private:
  friend class AstLoopAssignmentAnalyzer;
  explicit LoopAssignmentAnalysis(Zone* zone) : list_(zone) {}
  ZoneVector<std::pair<IterationStatement*, BitVector*>> list_;
};


// The class that performs loop assignment analysis by walking the AST.
class AstLoopAssignmentAnalyzer : public AstVisitor {
 public:
  AstLoopAssignmentAnalyzer(Zone* zone, CompilationInfo* info);

  LoopAssignmentAnalysis* Analyze();

#define DECLARE_VISIT(type) virtual void Visit##type(type* node);
  AST_NODE_LIST(DECLARE_VISIT)
#undef DECLARE_VISIT

  static int GetVariableIndex(Scope* scope, Variable* var);

 private:
  CompilationInfo* info_;
  ZoneDeque<BitVector*> loop_stack_;
  LoopAssignmentAnalysis* result_;

  CompilationInfo* info() { return info_; }

  void Enter(IterationStatement* loop);
  void Exit(IterationStatement* loop);

  void VisitIfNotNull(AstNode* node) {
    if (node != NULL) Visit(node);
  }

  void AnalyzeAssignment(Variable* var);

  DEFINE_AST_VISITOR_SUBCLASS_MEMBERS();
  DISALLOW_COPY_AND_ASSIGN(AstLoopAssignmentAnalyzer);
};
}
}
}  // namespace v8::internal::compiler

#endif  // V8_COMPILER_AST_LOOP_ASSIGNMENT_ANALYZER_H_
