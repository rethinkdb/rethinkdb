// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/compiler/change-lowering.h"

#include "src/code-factory.h"
#include "src/compiler/diamond.h"
#include "src/compiler/js-graph.h"
#include "src/compiler/linkage.h"
#include "src/compiler/machine-operator.h"

namespace v8 {
namespace internal {
namespace compiler {

ChangeLowering::~ChangeLowering() {}


Reduction ChangeLowering::Reduce(Node* node) {
  Node* control = graph()->start();
  switch (node->opcode()) {
    case IrOpcode::kChangeBitToBool:
      return ChangeBitToBool(node->InputAt(0), control);
    case IrOpcode::kChangeBoolToBit:
      return ChangeBoolToBit(node->InputAt(0));
    case IrOpcode::kChangeFloat64ToTagged:
      return ChangeFloat64ToTagged(node->InputAt(0), control);
    case IrOpcode::kChangeInt32ToTagged:
      return ChangeInt32ToTagged(node->InputAt(0), control);
    case IrOpcode::kChangeTaggedToFloat64:
      return ChangeTaggedToFloat64(node->InputAt(0), control);
    case IrOpcode::kChangeTaggedToInt32:
      return ChangeTaggedToUI32(node->InputAt(0), control, kSigned);
    case IrOpcode::kChangeTaggedToUint32:
      return ChangeTaggedToUI32(node->InputAt(0), control, kUnsigned);
    case IrOpcode::kChangeUint32ToTagged:
      return ChangeUint32ToTagged(node->InputAt(0), control);
    default:
      return NoChange();
  }
  UNREACHABLE();
  return NoChange();
}


Node* ChangeLowering::HeapNumberValueIndexConstant() {
  STATIC_ASSERT(HeapNumber::kValueOffset % kPointerSize == 0);
  const int heap_number_value_offset =
      ((HeapNumber::kValueOffset / kPointerSize) * (machine()->Is64() ? 8 : 4));
  return jsgraph()->IntPtrConstant(heap_number_value_offset - kHeapObjectTag);
}


Node* ChangeLowering::SmiMaxValueConstant() {
  const int smi_value_size = machine()->Is32() ? SmiTagging<4>::SmiValueSize()
                                               : SmiTagging<8>::SmiValueSize();
  return jsgraph()->Int32Constant(
      -(static_cast<int>(0xffffffffu << (smi_value_size - 1)) + 1));
}


Node* ChangeLowering::SmiShiftBitsConstant() {
  const int smi_shift_size = machine()->Is32() ? SmiTagging<4>::SmiShiftSize()
                                               : SmiTagging<8>::SmiShiftSize();
  return jsgraph()->IntPtrConstant(smi_shift_size + kSmiTagSize);
}


Node* ChangeLowering::AllocateHeapNumberWithValue(Node* value, Node* control) {
  // The AllocateHeapNumberStub does not use the context, so we can safely pass
  // in Smi zero here.
  Callable callable = CodeFactory::AllocateHeapNumber(isolate());
  CallDescriptor* descriptor = linkage()->GetStubCallDescriptor(
      callable.descriptor(), 0, CallDescriptor::kNoFlags);
  Node* target = jsgraph()->HeapConstant(callable.code());
  Node* context = jsgraph()->ZeroConstant();
  Node* effect = graph()->NewNode(common()->ValueEffect(1), value);
  Node* heap_number = graph()->NewNode(common()->Call(descriptor), target,
                                       context, effect, control);
  Node* store = graph()->NewNode(
      machine()->Store(StoreRepresentation(kMachFloat64, kNoWriteBarrier)),
      heap_number, HeapNumberValueIndexConstant(), value, heap_number, control);
  return graph()->NewNode(common()->Finish(1), heap_number, store);
}


Node* ChangeLowering::ChangeSmiToInt32(Node* value) {
  value = graph()->NewNode(machine()->WordSar(), value, SmiShiftBitsConstant());
  if (machine()->Is64()) {
    value = graph()->NewNode(machine()->TruncateInt64ToInt32(), value);
  }
  return value;
}


Node* ChangeLowering::LoadHeapNumberValue(Node* value, Node* control) {
  return graph()->NewNode(machine()->Load(kMachFloat64), value,
                          HeapNumberValueIndexConstant(), graph()->start(),
                          control);
}


Reduction ChangeLowering::ChangeBitToBool(Node* val, Node* control) {
  Diamond d(graph(), common(), val);
  d.Chain(control);
  MachineType machine_type = static_cast<MachineType>(kTypeBool | kRepTagged);
  return Replace(d.Phi(machine_type, jsgraph()->TrueConstant(),
                       jsgraph()->FalseConstant()));
}


Reduction ChangeLowering::ChangeBoolToBit(Node* val) {
  return Replace(
      graph()->NewNode(machine()->WordEqual(), val, jsgraph()->TrueConstant()));
}


Reduction ChangeLowering::ChangeFloat64ToTagged(Node* val, Node* control) {
  return Replace(AllocateHeapNumberWithValue(val, control));
}


Reduction ChangeLowering::ChangeInt32ToTagged(Node* val, Node* control) {
  if (machine()->Is64()) {
    return Replace(
        graph()->NewNode(machine()->Word64Shl(),
                         graph()->NewNode(machine()->ChangeInt32ToInt64(), val),
                         SmiShiftBitsConstant()));
  }

  Node* add = graph()->NewNode(machine()->Int32AddWithOverflow(), val, val);
  Node* ovf = graph()->NewNode(common()->Projection(1), add);

  Diamond d(graph(), common(), ovf, BranchHint::kFalse);
  d.Chain(control);
  Node* heap_number = AllocateHeapNumberWithValue(
      graph()->NewNode(machine()->ChangeInt32ToFloat64(), val), d.if_true);
  Node* smi = graph()->NewNode(common()->Projection(0), add);
  return Replace(d.Phi(kMachAnyTagged, heap_number, smi));
}


Reduction ChangeLowering::ChangeTaggedToUI32(Node* val, Node* control,
                                             Signedness signedness) {
  STATIC_ASSERT(kSmiTag == 0);
  STATIC_ASSERT(kSmiTagMask == 1);

  Node* tag = graph()->NewNode(machine()->WordAnd(), val,
                               jsgraph()->IntPtrConstant(kSmiTagMask));

  Diamond d(graph(), common(), tag, BranchHint::kFalse);
  d.Chain(control);
  const Operator* op = (signedness == kSigned)
                           ? machine()->ChangeFloat64ToInt32()
                           : machine()->ChangeFloat64ToUint32();
  Node* load = graph()->NewNode(op, LoadHeapNumberValue(val, d.if_true));
  Node* number = ChangeSmiToInt32(val);

  return Replace(
      d.Phi((signedness == kSigned) ? kMachInt32 : kMachUint32, load, number));
}


Reduction ChangeLowering::ChangeTaggedToFloat64(Node* val, Node* control) {
  STATIC_ASSERT(kSmiTag == 0);
  STATIC_ASSERT(kSmiTagMask == 1);

  Node* tag = graph()->NewNode(machine()->WordAnd(), val,
                               jsgraph()->IntPtrConstant(kSmiTagMask));
  Diamond d(graph(), common(), tag, BranchHint::kFalse);
  d.Chain(control);
  Node* load = LoadHeapNumberValue(val, d.if_true);
  Node* number = graph()->NewNode(machine()->ChangeInt32ToFloat64(),
                                  ChangeSmiToInt32(val));

  return Replace(d.Phi(kMachFloat64, load, number));
}


Reduction ChangeLowering::ChangeUint32ToTagged(Node* val, Node* control) {
  STATIC_ASSERT(kSmiTag == 0);
  STATIC_ASSERT(kSmiTagMask == 1);

  Node* cmp = graph()->NewNode(machine()->Uint32LessThanOrEqual(), val,
                               SmiMaxValueConstant());
  Diamond d(graph(), common(), cmp, BranchHint::kTrue);
  d.Chain(control);
  Node* smi = graph()->NewNode(
      machine()->WordShl(),
      machine()->Is64()
          ? graph()->NewNode(machine()->ChangeUint32ToUint64(), val)
          : val,
      SmiShiftBitsConstant());

  Node* heap_number = AllocateHeapNumberWithValue(
      graph()->NewNode(machine()->ChangeUint32ToFloat64(), val), d.if_false);

  return Replace(d.Phi(kMachAnyTagged, smi, heap_number));
}


Isolate* ChangeLowering::isolate() const { return jsgraph()->isolate(); }


Graph* ChangeLowering::graph() const { return jsgraph()->graph(); }


CommonOperatorBuilder* ChangeLowering::common() const {
  return jsgraph()->common();
}


MachineOperatorBuilder* ChangeLowering::machine() const {
  return jsgraph()->machine();
}

}  // namespace compiler
}  // namespace internal
}  // namespace v8
