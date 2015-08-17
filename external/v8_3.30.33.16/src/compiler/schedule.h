// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_COMPILER_SCHEDULE_H_
#define V8_COMPILER_SCHEDULE_H_

#include <iosfwd>
#include <vector>

#include "src/v8.h"

#include "src/compiler/node.h"
#include "src/compiler/opcodes.h"
#include "src/zone.h"

namespace v8 {
namespace internal {
namespace compiler {

class BasicBlock;
class BasicBlockInstrumentor;
class Graph;
class ConstructScheduleData;
class CodeGenerator;  // Because of a namespace bug in clang.

// A basic block contains an ordered list of nodes and ends with a control
// node. Note that if a basic block has phis, then all phis must appear as the
// first nodes in the block.
class BasicBlock FINAL : public ZoneObject {
 public:
  // Possible control nodes that can end a block.
  enum Control {
    kNone,    // Control not initialized yet.
    kGoto,    // Goto a single successor block.
    kBranch,  // Branch if true to first successor, otherwise second.
    kReturn,  // Return a value from this method.
    kThrow    // Throw an exception.
  };

  class Id {
   public:
    int ToInt() const { return static_cast<int>(index_); }
    size_t ToSize() const { return index_; }
    static Id FromSize(size_t index) { return Id(index); }
    static Id FromInt(int index) { return Id(static_cast<size_t>(index)); }

   private:
    explicit Id(size_t index) : index_(index) {}
    size_t index_;
  };

  static const int kInvalidRpoNumber = -1;
  class RpoNumber FINAL {
   public:
    int ToInt() const {
      DCHECK(IsValid());
      return index_;
    }
    size_t ToSize() const {
      DCHECK(IsValid());
      return static_cast<size_t>(index_);
    }
    bool IsValid() const { return index_ != kInvalidRpoNumber; }
    static RpoNumber FromInt(int index) { return RpoNumber(index); }
    static RpoNumber Invalid() { return RpoNumber(kInvalidRpoNumber); }

    bool IsNext(const RpoNumber other) const {
      DCHECK(IsValid());
      return other.index_ == this->index_ + 1;
    }

    bool operator==(RpoNumber other) const {
      return this->index_ == other.index_;
    }

   private:
    explicit RpoNumber(int32_t index) : index_(index) {}
    int32_t index_;
  };

  BasicBlock(Zone* zone, Id id);

  Id id() const { return id_; }

  // Predecessors and successors.
  typedef ZoneVector<BasicBlock*> Predecessors;
  Predecessors::iterator predecessors_begin() { return predecessors_.begin(); }
  Predecessors::iterator predecessors_end() { return predecessors_.end(); }
  Predecessors::const_iterator predecessors_begin() const {
    return predecessors_.begin();
  }
  Predecessors::const_iterator predecessors_end() const {
    return predecessors_.end();
  }
  size_t PredecessorCount() const { return predecessors_.size(); }
  BasicBlock* PredecessorAt(size_t index) { return predecessors_[index]; }
  void ClearPredecessors() { predecessors_.clear(); }
  void AddPredecessor(BasicBlock* predecessor);

  typedef ZoneVector<BasicBlock*> Successors;
  Successors::iterator successors_begin() { return successors_.begin(); }
  Successors::iterator successors_end() { return successors_.end(); }
  Successors::const_iterator successors_begin() const {
    return successors_.begin();
  }
  Successors::const_iterator successors_end() const {
    return successors_.end();
  }
  size_t SuccessorCount() const { return successors_.size(); }
  BasicBlock* SuccessorAt(size_t index) { return successors_[index]; }
  void ClearSuccessors() { successors_.clear(); }
  void AddSuccessor(BasicBlock* successor);

  // Nodes in the basic block.
  Node* NodeAt(size_t index) { return nodes_[index]; }
  size_t NodeCount() const { return nodes_.size(); }

  typedef NodeVector::iterator iterator;
  iterator begin() { return nodes_.begin(); }
  iterator end() { return nodes_.end(); }

  typedef NodeVector::const_iterator const_iterator;
  const_iterator begin() const { return nodes_.begin(); }
  const_iterator end() const { return nodes_.end(); }

  typedef NodeVector::reverse_iterator reverse_iterator;
  reverse_iterator rbegin() { return nodes_.rbegin(); }
  reverse_iterator rend() { return nodes_.rend(); }

  void AddNode(Node* node);
  template <class InputIterator>
  void InsertNodes(iterator insertion_point, InputIterator insertion_start,
                   InputIterator insertion_end) {
    nodes_.insert(insertion_point, insertion_start, insertion_end);
  }

  // Accessors.
  Control control() const { return control_; }
  void set_control(Control control);

  Node* control_input() const { return control_input_; }
  void set_control_input(Node* control_input);

  bool deferred() const { return deferred_; }
  void set_deferred(bool deferred) { deferred_ = deferred; }

  BasicBlock* dominator() const { return dominator_; }
  void set_dominator(BasicBlock* dominator);

  BasicBlock* loop_header() const { return loop_header_; }
  void set_loop_header(BasicBlock* loop_header);

  int32_t loop_depth() const { return loop_depth_; }
  void set_loop_depth(int32_t loop_depth);

  int32_t loop_end() const { return loop_end_; }
  void set_loop_end(int32_t loop_end);

  RpoNumber GetAoNumber() const { return RpoNumber::FromInt(ao_number_); }
  int32_t ao_number() const { return ao_number_; }
  void set_ao_number(int32_t ao_number) { ao_number_ = ao_number; }

  RpoNumber GetRpoNumber() const { return RpoNumber::FromInt(rpo_number_); }
  int32_t rpo_number() const { return rpo_number_; }
  void set_rpo_number(int32_t rpo_number);

  // Loop membership helpers.
  inline bool IsLoopHeader() const { return loop_end_ >= 0; }
  bool LoopContains(BasicBlock* block) const;

 private:
  int32_t ao_number_;        // assembly order number of the block.
  int32_t rpo_number_;       // special RPO number of the block.
  bool deferred_;            // true if the block contains deferred code.
  BasicBlock* dominator_;    // Immediate dominator of the block.
  BasicBlock* loop_header_;  // Pointer to dominating loop header basic block,
                             // NULL if none. For loop headers, this points to
                             // enclosing loop header.
  int32_t loop_depth_;       // loop nesting, 0 is top-level
  int32_t loop_end_;         // end of the loop, if this block is a loop header.

  Control control_;          // Control at the end of the block.
  Node* control_input_;      // Input value for control.
  NodeVector nodes_;         // nodes of this block in forward order.

  Successors successors_;
  Predecessors predecessors_;
  Id id_;

  DISALLOW_COPY_AND_ASSIGN(BasicBlock);
};

std::ostream& operator<<(std::ostream& os, const BasicBlock::Control& c);
std::ostream& operator<<(std::ostream& os, const BasicBlock::Id& id);
std::ostream& operator<<(std::ostream& os, const BasicBlock::RpoNumber& rpo);

typedef ZoneVector<BasicBlock*> BasicBlockVector;
typedef BasicBlockVector::iterator BasicBlockVectorIter;
typedef BasicBlockVector::reverse_iterator BasicBlockVectorRIter;

// A schedule represents the result of assigning nodes to basic blocks
// and ordering them within basic blocks. Prior to computing a schedule,
// a graph has no notion of control flow ordering other than that induced
// by the graph's dependencies. A schedule is required to generate code.
class Schedule FINAL : public ZoneObject {
 public:
  explicit Schedule(Zone* zone, size_t node_count_hint = 0);

  // Return the block which contains {node}, if any.
  BasicBlock* block(Node* node) const;

  bool IsScheduled(Node* node);
  BasicBlock* GetBlockById(BasicBlock::Id block_id);

  size_t BasicBlockCount() const { return all_blocks_.size(); }
  size_t RpoBlockCount() const { return rpo_order_.size(); }

  // Check if nodes {a} and {b} are in the same block.
  bool SameBasicBlock(Node* a, Node* b) const;

  // BasicBlock building: create a new block.
  BasicBlock* NewBasicBlock();

  // BasicBlock building: records that a node will later be added to a block but
  // doesn't actually add the node to the block.
  void PlanNode(BasicBlock* block, Node* node);

  // BasicBlock building: add a node to the end of the block.
  void AddNode(BasicBlock* block, Node* node);

  // BasicBlock building: add a goto to the end of {block}.
  void AddGoto(BasicBlock* block, BasicBlock* succ);

  // BasicBlock building: add a branch at the end of {block}.
  void AddBranch(BasicBlock* block, Node* branch, BasicBlock* tblock,
                 BasicBlock* fblock);

  // BasicBlock building: add a return at the end of {block}.
  void AddReturn(BasicBlock* block, Node* input);

  // BasicBlock building: add a throw at the end of {block}.
  void AddThrow(BasicBlock* block, Node* input);

  // BasicBlock mutation: insert a branch into the end of {block}.
  void InsertBranch(BasicBlock* block, BasicBlock* end, Node* branch,
                    BasicBlock* tblock, BasicBlock* fblock);

  // Exposed publicly for testing only.
  void AddSuccessorForTesting(BasicBlock* block, BasicBlock* succ) {
    return AddSuccessor(block, succ);
  }

  BasicBlockVector* rpo_order() { return &rpo_order_; }
  const BasicBlockVector* rpo_order() const { return &rpo_order_; }

  BasicBlock* start() { return start_; }
  BasicBlock* end() { return end_; }

  Zone* zone() const { return zone_; }

 private:
  friend class Scheduler;
  friend class CodeGenerator;
  friend class ScheduleVisualizer;
  friend class BasicBlockInstrumentor;

  void AddSuccessor(BasicBlock* block, BasicBlock* succ);
  void MoveSuccessors(BasicBlock* from, BasicBlock* to);

  void SetControlInput(BasicBlock* block, Node* node);
  void SetBlockForNode(BasicBlock* block, Node* node);

  Zone* zone_;
  BasicBlockVector all_blocks_;           // All basic blocks in the schedule.
  BasicBlockVector nodeid_to_block_;      // Map from node to containing block.
  BasicBlockVector rpo_order_;            // Reverse-post-order block list.
  BasicBlock* start_;
  BasicBlock* end_;

  DISALLOW_COPY_AND_ASSIGN(Schedule);
};

std::ostream& operator<<(std::ostream& os, const Schedule& s);

}  // namespace compiler
}  // namespace internal
}  // namespace v8

#endif  // V8_COMPILER_SCHEDULE_H_
