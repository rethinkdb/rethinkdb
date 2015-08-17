// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/compiler/verifier.h"

#include <deque>
#include <queue>
#include <sstream>
#include <string>

#include "src/bit-vector.h"
#include "src/compiler/generic-algorithm.h"
#include "src/compiler/generic-node-inl.h"
#include "src/compiler/generic-node.h"
#include "src/compiler/graph-inl.h"
#include "src/compiler/graph.h"
#include "src/compiler/node.h"
#include "src/compiler/node-properties-inl.h"
#include "src/compiler/node-properties.h"
#include "src/compiler/opcodes.h"
#include "src/compiler/operator.h"
#include "src/compiler/schedule.h"
#include "src/compiler/simplified-operator.h"
#include "src/ostreams.h"

namespace v8 {
namespace internal {
namespace compiler {


static bool IsDefUseChainLinkPresent(Node* def, Node* use) {
  Node::Uses uses = def->uses();
  for (Node::Uses::iterator it = uses.begin(); it != uses.end(); ++it) {
    if (*it == use) return true;
  }
  return false;
}


static bool IsUseDefChainLinkPresent(Node* def, Node* use) {
  Node::Inputs inputs = use->inputs();
  for (Node::Inputs::iterator it = inputs.begin(); it != inputs.end(); ++it) {
    if (*it == def) return true;
  }
  return false;
}


class Verifier::Visitor : public NullNodeVisitor {
 public:
  Visitor(Zone* z, Typing typed) : zone(z), typing(typed) {}

  // Fulfills the PreNodeCallback interface.
  void Pre(Node* node);

  Zone* zone;
  Typing typing;

 private:
  // TODO(rossberg): Get rid of these once we got rid of NodeProperties.
  Bounds bounds(Node* node) { return NodeProperties::GetBounds(node); }
  Node* ValueInput(Node* node, int i = 0) {
    return NodeProperties::GetValueInput(node, i);
  }
  FieldAccess Field(Node* node) {
    DCHECK(node->opcode() == IrOpcode::kLoadField ||
           node->opcode() == IrOpcode::kStoreField);
    return OpParameter<FieldAccess>(node);
  }
  ElementAccess Element(Node* node) {
    DCHECK(node->opcode() == IrOpcode::kLoadElement ||
           node->opcode() == IrOpcode::kStoreElement);
    return OpParameter<ElementAccess>(node);
  }
  void CheckNotTyped(Node* node) {
    if (NodeProperties::IsTyped(node)) {
      std::ostringstream str;
      str << "TypeError: node #" << node->opcode() << ":"
          << node->op()->mnemonic() << " should never have a type";
      V8_Fatal(__FILE__, __LINE__, str.str().c_str());
    }
  }
  void CheckUpperIs(Node* node, Type* type) {
    if (typing == TYPED && !bounds(node).upper->Is(type)) {
      std::ostringstream str;
      str << "TypeError: node #" << node->opcode() << ":"
          << node->op()->mnemonic() << " upper bound ";
      bounds(node).upper->PrintTo(str);
      str << " is not ";
      type->PrintTo(str);
      V8_Fatal(__FILE__, __LINE__, str.str().c_str());
    }
  }
  void CheckUpperMaybe(Node* node, Type* type) {
    if (typing == TYPED && !bounds(node).upper->Maybe(type)) {
      std::ostringstream str;
      str << "TypeError: node #" << node->opcode() << ":"
          << node->op()->mnemonic() << " upper bound ";
      bounds(node).upper->PrintTo(str);
      str << " must intersect ";
      type->PrintTo(str);
      V8_Fatal(__FILE__, __LINE__, str.str().c_str());
    }
  }
  void CheckValueInputIs(Node* node, int i, Type* type) {
    Node* input = ValueInput(node, i);
    if (typing == TYPED && !bounds(input).upper->Is(type)) {
      std::ostringstream str;
      str << "TypeError: node #" << node->opcode() << ":"
          << node->op()->mnemonic() << "(input @" << i << " = "
          << input->opcode() << ":" << input->op()->mnemonic()
          << ") upper bound ";
      bounds(input).upper->PrintTo(str);
      str << " is not ";
      type->PrintTo(str);
      V8_Fatal(__FILE__, __LINE__, str.str().c_str());
    }
  }
};


void Verifier::Visitor::Pre(Node* node) {
  int value_count = node->op()->ValueInputCount();
  int context_count = OperatorProperties::GetContextInputCount(node->op());
  int frame_state_count =
      OperatorProperties::GetFrameStateInputCount(node->op());
  int effect_count = node->op()->EffectInputCount();
  int control_count = node->op()->ControlInputCount();

  // Verify number of inputs matches up.
  int input_count = value_count + context_count + frame_state_count +
                    effect_count + control_count;
  CHECK_EQ(input_count, node->InputCount());

  // Verify that frame state has been inserted for the nodes that need it.
  if (OperatorProperties::HasFrameStateInput(node->op())) {
    Node* frame_state = NodeProperties::GetFrameStateInput(node);
    CHECK(frame_state->opcode() == IrOpcode::kFrameState ||
          // kFrameState uses undefined as a sentinel.
          (node->opcode() == IrOpcode::kFrameState &&
           frame_state->opcode() == IrOpcode::kHeapConstant));
    CHECK(IsDefUseChainLinkPresent(frame_state, node));
    CHECK(IsUseDefChainLinkPresent(frame_state, node));
  }

  // Verify all value inputs actually produce a value.
  for (int i = 0; i < value_count; ++i) {
    Node* value = NodeProperties::GetValueInput(node, i);
    CHECK(value->op()->ValueOutputCount() > 0);
    CHECK(IsDefUseChainLinkPresent(value, node));
    CHECK(IsUseDefChainLinkPresent(value, node));
  }

  // Verify all context inputs are value nodes.
  for (int i = 0; i < context_count; ++i) {
    Node* context = NodeProperties::GetContextInput(node);
    CHECK(context->op()->ValueOutputCount() > 0);
    CHECK(IsDefUseChainLinkPresent(context, node));
    CHECK(IsUseDefChainLinkPresent(context, node));
  }

  // Verify all effect inputs actually have an effect.
  for (int i = 0; i < effect_count; ++i) {
    Node* effect = NodeProperties::GetEffectInput(node);
    CHECK(effect->op()->EffectOutputCount() > 0);
    CHECK(IsDefUseChainLinkPresent(effect, node));
    CHECK(IsUseDefChainLinkPresent(effect, node));
  }

  // Verify all control inputs are control nodes.
  for (int i = 0; i < control_count; ++i) {
    Node* control = NodeProperties::GetControlInput(node, i);
    CHECK(control->op()->ControlOutputCount() > 0);
    CHECK(IsDefUseChainLinkPresent(control, node));
    CHECK(IsUseDefChainLinkPresent(control, node));
  }

  // Verify all successors are projections if multiple value outputs exist.
  if (node->op()->ValueOutputCount() > 1) {
    Node::Uses uses = node->uses();
    for (Node::Uses::iterator it = uses.begin(); it != uses.end(); ++it) {
      CHECK(!NodeProperties::IsValueEdge(it.edge()) ||
            (*it)->opcode() == IrOpcode::kProjection ||
            (*it)->opcode() == IrOpcode::kParameter);
    }
  }

  switch (node->opcode()) {
    case IrOpcode::kStart:
      // Start has no inputs.
      CHECK_EQ(0, input_count);
      // Type is a tuple.
      // TODO(rossberg): Multiple outputs are currently typed as Internal.
      CheckUpperIs(node, Type::Internal());
      break;
    case IrOpcode::kEnd:
      // End has no outputs.
      CHECK(node->op()->ValueOutputCount() == 0);
      CHECK(node->op()->EffectOutputCount() == 0);
      CHECK(node->op()->ControlOutputCount() == 0);
      // Type is empty.
      CheckNotTyped(node);
      break;
    case IrOpcode::kDead:
      // Dead is never connected to the graph.
      UNREACHABLE();
    case IrOpcode::kBranch: {
      // Branch uses are IfTrue and IfFalse.
      Node::Uses uses = node->uses();
      int count_true = 0, count_false = 0;
      for (Node::Uses::iterator it = uses.begin(); it != uses.end(); ++it) {
        CHECK((*it)->opcode() == IrOpcode::kIfTrue ||
              (*it)->opcode() == IrOpcode::kIfFalse);
        if ((*it)->opcode() == IrOpcode::kIfTrue) ++count_true;
        if ((*it)->opcode() == IrOpcode::kIfFalse) ++count_false;
      }
      CHECK(count_true == 1 && count_false == 1);
      // Type is empty.
      CheckNotTyped(node);
      break;
    }
    case IrOpcode::kIfTrue:
    case IrOpcode::kIfFalse:
      CHECK_EQ(IrOpcode::kBranch,
               NodeProperties::GetControlInput(node, 0)->opcode());
      // Type is empty.
      CheckNotTyped(node);
      break;
    case IrOpcode::kLoop:
    case IrOpcode::kMerge:
      CHECK_EQ(control_count, input_count);
      // Type is empty.
      CheckNotTyped(node);
      break;
    case IrOpcode::kReturn:
      // TODO(rossberg): check successor is End
      // Type is empty.
      CheckNotTyped(node);
      break;
    case IrOpcode::kThrow:
      // TODO(rossberg): what are the constraints on these?
      // Type is empty.
      CheckNotTyped(node);
      break;
    case IrOpcode::kTerminate:
      // Type is empty.
      CheckNotTyped(node);
      CHECK_EQ(1, control_count);
      CHECK_EQ(input_count, 1 + effect_count);
      break;

    // Common operators
    // ----------------
    case IrOpcode::kParameter: {
      // Parameters have the start node as inputs.
      CHECK_EQ(1, input_count);
      CHECK_EQ(IrOpcode::kStart,
               NodeProperties::GetValueInput(node, 0)->opcode());
      // Parameter has an input that produces enough values.
      int index = OpParameter<int>(node);
      Node* input = NodeProperties::GetValueInput(node, 0);
      // Currently, parameter indices start at -1 instead of 0.
      CHECK_GT(input->op()->ValueOutputCount(), index + 1);
      // Type can be anything.
      CheckUpperIs(node, Type::Any());
      break;
    }
    case IrOpcode::kInt32Constant:  // TODO(rossberg): rename Word32Constant?
      // Constants have no inputs.
      CHECK_EQ(0, input_count);
      // Type is a 32 bit integer, signed or unsigned.
      CheckUpperIs(node, Type::Integral32());
      break;
    case IrOpcode::kInt64Constant:
      // Constants have no inputs.
      CHECK_EQ(0, input_count);
      // Type is internal.
      // TODO(rossberg): Introduce proper Int64 type.
      CheckUpperIs(node, Type::Internal());
      break;
    case IrOpcode::kFloat32Constant:
    case IrOpcode::kFloat64Constant:
    case IrOpcode::kNumberConstant:
      // Constants have no inputs.
      CHECK_EQ(0, input_count);
      // Type is a number.
      CheckUpperIs(node, Type::Number());
      break;
    case IrOpcode::kHeapConstant:
      // Constants have no inputs.
      CHECK_EQ(0, input_count);
      // Type can be anything represented as a heap pointer.
      CheckUpperIs(node, Type::TaggedPtr());
      break;
    case IrOpcode::kExternalConstant:
      // Constants have no inputs.
      CHECK_EQ(0, input_count);
      // Type is considered internal.
      CheckUpperIs(node, Type::Internal());
      break;
    case IrOpcode::kProjection: {
      // Projection has an input that produces enough values.
      int index = static_cast<int>(OpParameter<size_t>(node->op()));
      Node* input = NodeProperties::GetValueInput(node, 0);
      CHECK_GT(input->op()->ValueOutputCount(), index);
      // Type can be anything.
      // TODO(rossberg): Introduce tuple types for this.
      // TODO(titzer): Convince rossberg not to.
      CheckUpperIs(node, Type::Any());
      break;
    }
    case IrOpcode::kSelect: {
      CHECK_EQ(0, effect_count);
      CHECK_EQ(0, control_count);
      CHECK_EQ(3, value_count);
      break;
    }
    case IrOpcode::kPhi: {
      // Phi input count matches parent control node.
      CHECK_EQ(0, effect_count);
      CHECK_EQ(1, control_count);
      Node* control = NodeProperties::GetControlInput(node, 0);
      CHECK_EQ(value_count, control->op()->ControlInputCount());
      CHECK_EQ(input_count, 1 + value_count);
      // Type must be subsumed by all input types.
      // TODO(rossberg): for now at least, narrowing does not really hold.
      /*
      for (int i = 0; i < value_count; ++i) {
        // TODO(rossberg, jarin): Figure out what to do about lower bounds.
        // CHECK(bounds(node).lower->Is(bounds(ValueInput(node, i)).lower));
        CHECK(bounds(ValueInput(node, i)).upper->Is(bounds(node).upper));
      }
      */
      break;
    }
    case IrOpcode::kEffectPhi: {
      // EffectPhi input count matches parent control node.
      CHECK_EQ(0, value_count);
      CHECK_EQ(1, control_count);
      Node* control = NodeProperties::GetControlInput(node, 0);
      CHECK_EQ(effect_count, control->op()->ControlInputCount());
      CHECK_EQ(input_count, 1 + effect_count);
      break;
    }
    case IrOpcode::kValueEffect:
      // TODO(rossberg): what are the constraints on these?
      break;
    case IrOpcode::kFinish: {
      // TODO(rossberg): what are the constraints on these?
      // Type must be subsumed by input type.
      if (typing == TYPED) {
        CHECK(bounds(ValueInput(node)).lower->Is(bounds(node).lower));
        CHECK(bounds(ValueInput(node)).upper->Is(bounds(node).upper));
      }
      break;
    }
    case IrOpcode::kFrameState:
      // TODO(jarin): what are the constraints on these?
      break;
    case IrOpcode::kStateValues:
      // TODO(jarin): what are the constraints on these?
      break;
    case IrOpcode::kCall:
      // TODO(rossberg): what are the constraints on these?
      break;

    // JavaScript operators
    // --------------------
    case IrOpcode::kJSEqual:
    case IrOpcode::kJSNotEqual:
    case IrOpcode::kJSStrictEqual:
    case IrOpcode::kJSStrictNotEqual:
    case IrOpcode::kJSLessThan:
    case IrOpcode::kJSGreaterThan:
    case IrOpcode::kJSLessThanOrEqual:
    case IrOpcode::kJSGreaterThanOrEqual:
    case IrOpcode::kJSUnaryNot:
      // Type is Boolean.
      CheckUpperIs(node, Type::Boolean());
      break;

    case IrOpcode::kJSBitwiseOr:
    case IrOpcode::kJSBitwiseXor:
    case IrOpcode::kJSBitwiseAnd:
    case IrOpcode::kJSShiftLeft:
    case IrOpcode::kJSShiftRight:
    case IrOpcode::kJSShiftRightLogical:
      // Type is 32 bit integral.
      CheckUpperIs(node, Type::Integral32());
      break;
    case IrOpcode::kJSAdd:
      // Type is Number or String.
      CheckUpperIs(node, Type::NumberOrString());
      break;
    case IrOpcode::kJSSubtract:
    case IrOpcode::kJSMultiply:
    case IrOpcode::kJSDivide:
    case IrOpcode::kJSModulus:
      // Type is Number.
      CheckUpperIs(node, Type::Number());
      break;

    case IrOpcode::kJSToBoolean:
      // Type is Boolean.
      CheckUpperIs(node, Type::Boolean());
      break;
    case IrOpcode::kJSToNumber:
      // Type is Number.
      CheckUpperIs(node, Type::Number());
      break;
    case IrOpcode::kJSToString:
      // Type is String.
      CheckUpperIs(node, Type::String());
      break;
    case IrOpcode::kJSToName:
      // Type is Name.
      CheckUpperIs(node, Type::Name());
      break;
    case IrOpcode::kJSToObject:
      // Type is Receiver.
      CheckUpperIs(node, Type::Receiver());
      break;

    case IrOpcode::kJSCreate:
      // Type is Object.
      CheckUpperIs(node, Type::Object());
      break;
    case IrOpcode::kJSLoadProperty:
    case IrOpcode::kJSLoadNamed:
      // Type can be anything.
      CheckUpperIs(node, Type::Any());
      break;
    case IrOpcode::kJSStoreProperty:
    case IrOpcode::kJSStoreNamed:
      // Type is empty.
      CheckNotTyped(node);
      break;
    case IrOpcode::kJSDeleteProperty:
    case IrOpcode::kJSHasProperty:
    case IrOpcode::kJSInstanceOf:
      // Type is Boolean.
      CheckUpperIs(node, Type::Boolean());
      break;
    case IrOpcode::kJSTypeOf:
      // Type is String.
      CheckUpperIs(node, Type::String());
      break;

    case IrOpcode::kJSLoadContext:
      // Type can be anything.
      CheckUpperIs(node, Type::Any());
      break;
    case IrOpcode::kJSStoreContext:
      // Type is empty.
      CheckNotTyped(node);
      break;
    case IrOpcode::kJSCreateFunctionContext:
    case IrOpcode::kJSCreateCatchContext:
    case IrOpcode::kJSCreateWithContext:
    case IrOpcode::kJSCreateBlockContext:
    case IrOpcode::kJSCreateModuleContext:
    case IrOpcode::kJSCreateGlobalContext: {
      // Type is Context, and operand is Internal.
      Node* context = NodeProperties::GetContextInput(node);
      // TODO(rossberg): This should really be Is(Internal), but the typer
      // currently can't do backwards propagation.
      CheckUpperMaybe(context, Type::Internal());
      if (typing == TYPED) CHECK(bounds(node).upper->IsContext());
      break;
    }

    case IrOpcode::kJSCallConstruct:
      // Type is Receiver.
      CheckUpperIs(node, Type::Receiver());
      break;
    case IrOpcode::kJSCallFunction:
    case IrOpcode::kJSCallRuntime:
    case IrOpcode::kJSYield:
    case IrOpcode::kJSDebugger:
      // Type can be anything.
      CheckUpperIs(node, Type::Any());
      break;

    // Simplified operators
    // -------------------------------
    case IrOpcode::kBooleanNot:
      // Boolean -> Boolean
      CheckValueInputIs(node, 0, Type::Boolean());
      CheckUpperIs(node, Type::Boolean());
      break;
    case IrOpcode::kBooleanToNumber:
      // Boolean -> Number
      CheckValueInputIs(node, 0, Type::Boolean());
      CheckUpperIs(node, Type::Number());
      break;
    case IrOpcode::kNumberEqual:
    case IrOpcode::kNumberLessThan:
    case IrOpcode::kNumberLessThanOrEqual:
      // (Number, Number) -> Boolean
      CheckValueInputIs(node, 0, Type::Number());
      CheckValueInputIs(node, 1, Type::Number());
      CheckUpperIs(node, Type::Boolean());
      break;
    case IrOpcode::kNumberAdd:
    case IrOpcode::kNumberSubtract:
    case IrOpcode::kNumberMultiply:
    case IrOpcode::kNumberDivide:
    case IrOpcode::kNumberModulus:
      // (Number, Number) -> Number
      CheckValueInputIs(node, 0, Type::Number());
      CheckValueInputIs(node, 1, Type::Number());
      // TODO(rossberg): activate once we retype after opcode changes.
      // CheckUpperIs(node, Type::Number());
      break;
    case IrOpcode::kNumberToInt32:
      // Number -> Signed32
      CheckValueInputIs(node, 0, Type::Number());
      CheckUpperIs(node, Type::Signed32());
      break;
    case IrOpcode::kNumberToUint32:
      // Number -> Unsigned32
      CheckValueInputIs(node, 0, Type::Number());
      CheckUpperIs(node, Type::Unsigned32());
      break;
    case IrOpcode::kStringEqual:
    case IrOpcode::kStringLessThan:
    case IrOpcode::kStringLessThanOrEqual:
      // (String, String) -> Boolean
      CheckValueInputIs(node, 0, Type::String());
      CheckValueInputIs(node, 1, Type::String());
      CheckUpperIs(node, Type::Boolean());
      break;
    case IrOpcode::kStringAdd:
      // (String, String) -> String
      CheckValueInputIs(node, 0, Type::String());
      CheckValueInputIs(node, 1, Type::String());
      CheckUpperIs(node, Type::String());
      break;
    case IrOpcode::kReferenceEqual: {
      // (Unique, Any) -> Boolean  and
      // (Any, Unique) -> Boolean
      if (typing == TYPED) {
        CHECK(bounds(ValueInput(node, 0)).upper->Is(Type::Unique()) ||
              bounds(ValueInput(node, 1)).upper->Is(Type::Unique()));
      }
      CheckUpperIs(node, Type::Boolean());
      break;
    }
    case IrOpcode::kObjectIsSmi:
      CheckValueInputIs(node, 0, Type::Any());
      CheckUpperIs(node, Type::Boolean());
      break;
    case IrOpcode::kObjectIsNonNegativeSmi:
      CheckValueInputIs(node, 0, Type::Any());
      CheckUpperIs(node, Type::Boolean());
      break;

    case IrOpcode::kChangeTaggedToInt32: {
      // Signed32 /\ Tagged -> Signed32 /\ UntaggedInt32
      // TODO(neis): Activate once ChangeRepresentation works in typer.
      // Type* from = Type::Intersect(Type::Signed32(), Type::Tagged());
      // Type* to = Type::Intersect(Type::Signed32(), Type::UntaggedInt32());
      // CheckValueInputIs(node, 0, from));
      // CheckUpperIs(node, to));
      break;
    }
    case IrOpcode::kChangeTaggedToUint32: {
      // Unsigned32 /\ Tagged -> Unsigned32 /\ UntaggedInt32
      // TODO(neis): Activate once ChangeRepresentation works in typer.
      // Type* from = Type::Intersect(Type::Unsigned32(), Type::Tagged());
      // Type* to =Type::Intersect(Type::Unsigned32(), Type::UntaggedInt32());
      // CheckValueInputIs(node, 0, from));
      // CheckUpperIs(node, to));
      break;
    }
    case IrOpcode::kChangeTaggedToFloat64: {
      // Number /\ Tagged -> Number /\ UntaggedFloat64
      // TODO(neis): Activate once ChangeRepresentation works in typer.
      // Type* from = Type::Intersect(Type::Number(), Type::Tagged());
      // Type* to = Type::Intersect(Type::Number(), Type::UntaggedFloat64());
      // CheckValueInputIs(node, 0, from));
      // CheckUpperIs(node, to));
      break;
    }
    case IrOpcode::kChangeInt32ToTagged: {
      // Signed32 /\ UntaggedInt32 -> Signed32 /\ Tagged
      // TODO(neis): Activate once ChangeRepresentation works in typer.
      // Type* from =Type::Intersect(Type::Signed32(), Type::UntaggedInt32());
      // Type* to = Type::Intersect(Type::Signed32(), Type::Tagged());
      // CheckValueInputIs(node, 0, from));
      // CheckUpperIs(node, to));
      break;
    }
    case IrOpcode::kChangeUint32ToTagged: {
      // Unsigned32 /\ UntaggedInt32 -> Unsigned32 /\ Tagged
      // TODO(neis): Activate once ChangeRepresentation works in typer.
      // Type* from=Type::Intersect(Type::Unsigned32(),Type::UntaggedInt32());
      // Type* to = Type::Intersect(Type::Unsigned32(), Type::Tagged());
      // CheckValueInputIs(node, 0, from));
      // CheckUpperIs(node, to));
      break;
    }
    case IrOpcode::kChangeFloat64ToTagged: {
      // Number /\ UntaggedFloat64 -> Number /\ Tagged
      // TODO(neis): Activate once ChangeRepresentation works in typer.
      // Type* from =Type::Intersect(Type::Number(), Type::UntaggedFloat64());
      // Type* to = Type::Intersect(Type::Number(), Type::Tagged());
      // CheckValueInputIs(node, 0, from));
      // CheckUpperIs(node, to));
      break;
    }
    case IrOpcode::kChangeBoolToBit: {
      // Boolean /\ TaggedPtr -> Boolean /\ UntaggedInt1
      // TODO(neis): Activate once ChangeRepresentation works in typer.
      // Type* from = Type::Intersect(Type::Boolean(), Type::TaggedPtr());
      // Type* to = Type::Intersect(Type::Boolean(), Type::UntaggedInt1());
      // CheckValueInputIs(node, 0, from));
      // CheckUpperIs(node, to));
      break;
    }
    case IrOpcode::kChangeBitToBool: {
      // Boolean /\ UntaggedInt1 -> Boolean /\ TaggedPtr
      // TODO(neis): Activate once ChangeRepresentation works in typer.
      // Type* from = Type::Intersect(Type::Boolean(), Type::UntaggedInt1());
      // Type* to = Type::Intersect(Type::Boolean(), Type::TaggedPtr());
      // CheckValueInputIs(node, 0, from));
      // CheckUpperIs(node, to));
      break;
    }

    case IrOpcode::kLoadField:
      // Object -> fieldtype
      // TODO(rossberg): activate once machine ops are typed.
      // CheckValueInputIs(node, 0, Type::Object());
      // CheckUpperIs(node, Field(node).type));
      break;
    case IrOpcode::kLoadElement:
      // Object -> elementtype
      // TODO(rossberg): activate once machine ops are typed.
      // CheckValueInputIs(node, 0, Type::Object());
      // CheckUpperIs(node, Element(node).type));
      break;
    case IrOpcode::kStoreField:
      // (Object, fieldtype) -> _|_
      // TODO(rossberg): activate once machine ops are typed.
      // CheckValueInputIs(node, 0, Type::Object());
      // CheckValueInputIs(node, 1, Field(node).type));
      CheckNotTyped(node);
      break;
    case IrOpcode::kStoreElement:
      // (Object, elementtype) -> _|_
      // TODO(rossberg): activate once machine ops are typed.
      // CheckValueInputIs(node, 0, Type::Object());
      // CheckValueInputIs(node, 1, Element(node).type));
      CheckNotTyped(node);
      break;

    // Machine operators
    // -----------------------
    case IrOpcode::kLoad:
    case IrOpcode::kStore:
    case IrOpcode::kWord32And:
    case IrOpcode::kWord32Or:
    case IrOpcode::kWord32Xor:
    case IrOpcode::kWord32Shl:
    case IrOpcode::kWord32Shr:
    case IrOpcode::kWord32Sar:
    case IrOpcode::kWord32Ror:
    case IrOpcode::kWord32Equal:
    case IrOpcode::kWord64And:
    case IrOpcode::kWord64Or:
    case IrOpcode::kWord64Xor:
    case IrOpcode::kWord64Shl:
    case IrOpcode::kWord64Shr:
    case IrOpcode::kWord64Sar:
    case IrOpcode::kWord64Ror:
    case IrOpcode::kWord64Equal:
    case IrOpcode::kInt32Add:
    case IrOpcode::kInt32AddWithOverflow:
    case IrOpcode::kInt32Sub:
    case IrOpcode::kInt32SubWithOverflow:
    case IrOpcode::kInt32Mul:
    case IrOpcode::kInt32MulHigh:
    case IrOpcode::kInt32Div:
    case IrOpcode::kInt32Mod:
    case IrOpcode::kInt32LessThan:
    case IrOpcode::kInt32LessThanOrEqual:
    case IrOpcode::kUint32Div:
    case IrOpcode::kUint32Mod:
    case IrOpcode::kUint32MulHigh:
    case IrOpcode::kUint32LessThan:
    case IrOpcode::kUint32LessThanOrEqual:
    case IrOpcode::kInt64Add:
    case IrOpcode::kInt64Sub:
    case IrOpcode::kInt64Mul:
    case IrOpcode::kInt64Div:
    case IrOpcode::kInt64Mod:
    case IrOpcode::kInt64LessThan:
    case IrOpcode::kInt64LessThanOrEqual:
    case IrOpcode::kUint64Div:
    case IrOpcode::kUint64Mod:
    case IrOpcode::kUint64LessThan:
    case IrOpcode::kFloat64Add:
    case IrOpcode::kFloat64Sub:
    case IrOpcode::kFloat64Mul:
    case IrOpcode::kFloat64Div:
    case IrOpcode::kFloat64Mod:
    case IrOpcode::kFloat64Sqrt:
    case IrOpcode::kFloat64Floor:
    case IrOpcode::kFloat64Ceil:
    case IrOpcode::kFloat64RoundTruncate:
    case IrOpcode::kFloat64RoundTiesAway:
    case IrOpcode::kFloat64Equal:
    case IrOpcode::kFloat64LessThan:
    case IrOpcode::kFloat64LessThanOrEqual:
    case IrOpcode::kTruncateInt64ToInt32:
    case IrOpcode::kTruncateFloat64ToFloat32:
    case IrOpcode::kTruncateFloat64ToInt32:
    case IrOpcode::kChangeInt32ToInt64:
    case IrOpcode::kChangeUint32ToUint64:
    case IrOpcode::kChangeInt32ToFloat64:
    case IrOpcode::kChangeUint32ToFloat64:
    case IrOpcode::kChangeFloat32ToFloat64:
    case IrOpcode::kChangeFloat64ToInt32:
    case IrOpcode::kChangeFloat64ToUint32:
    case IrOpcode::kLoadStackPointer:
      // TODO(rossberg): Check.
      break;
  }
}


void Verifier::Run(Graph* graph, Typing typing) {
  Visitor visitor(graph->zone(), typing);
  CHECK_NE(NULL, graph->start());
  CHECK_NE(NULL, graph->end());
  graph->VisitNodeInputsFromEnd(&visitor);
}


// -----------------------------------------------------------------------------

static bool HasDominatingDef(Schedule* schedule, Node* node,
                             BasicBlock* container, BasicBlock* use_block,
                             int use_pos) {
  BasicBlock* block = use_block;
  while (true) {
    while (use_pos >= 0) {
      if (block->NodeAt(use_pos) == node) return true;
      use_pos--;
    }
    block = block->dominator();
    if (block == NULL) break;
    use_pos = static_cast<int>(block->NodeCount()) - 1;
    if (node == block->control_input()) return true;
  }
  return false;
}


static bool Dominates(Schedule* schedule, Node* dominator, Node* dominatee) {
  BasicBlock* dom = schedule->block(dominator);
  BasicBlock* sub = schedule->block(dominatee);
  while (sub != NULL) {
    if (sub == dom) {
      return true;
    }
    sub = sub->dominator();
  }
  return false;
}


static void CheckInputsDominate(Schedule* schedule, BasicBlock* block,
                                Node* node, int use_pos) {
  for (int j = node->op()->ValueInputCount() - 1; j >= 0; j--) {
    BasicBlock* use_block = block;
    if (node->opcode() == IrOpcode::kPhi) {
      use_block = use_block->PredecessorAt(j);
      use_pos = static_cast<int>(use_block->NodeCount()) - 1;
    }
    Node* input = node->InputAt(j);
    if (!HasDominatingDef(schedule, node->InputAt(j), block, use_block,
                          use_pos)) {
      V8_Fatal(__FILE__, __LINE__,
               "Node #%d:%s in B%d is not dominated by input@%d #%d:%s",
               node->id(), node->op()->mnemonic(), block->id().ToInt(), j,
               input->id(), input->op()->mnemonic());
    }
  }
  // Ensure that nodes are dominated by their control inputs;
  // kEnd is an exception, as unreachable blocks resulting from kMerge
  // are not in the RPO.
  if (node->op()->ControlInputCount() == 1 &&
      node->opcode() != IrOpcode::kEnd) {
    Node* ctl = NodeProperties::GetControlInput(node);
    if (!Dominates(schedule, ctl, node)) {
      V8_Fatal(__FILE__, __LINE__,
               "Node #%d:%s in B%d is not dominated by control input #%d:%s",
               node->id(), node->op()->mnemonic(), block->id(), ctl->id(),
               ctl->op()->mnemonic());
    }
  }
}


void ScheduleVerifier::Run(Schedule* schedule) {
  const size_t count = schedule->BasicBlockCount();
  Zone tmp_zone(schedule->zone()->isolate());
  Zone* zone = &tmp_zone;
  BasicBlock* start = schedule->start();
  BasicBlockVector* rpo_order = schedule->rpo_order();

  // Verify the RPO order contains only blocks from this schedule.
  CHECK_GE(count, rpo_order->size());
  for (BasicBlockVector::iterator b = rpo_order->begin(); b != rpo_order->end();
       ++b) {
    CHECK_EQ((*b), schedule->GetBlockById((*b)->id()));
    // All predecessors and successors should be in rpo and in this schedule.
    for (BasicBlock::Predecessors::iterator j = (*b)->predecessors_begin();
         j != (*b)->predecessors_end(); ++j) {
      CHECK_GE((*j)->rpo_number(), 0);
      CHECK_EQ((*j), schedule->GetBlockById((*j)->id()));
    }
    for (BasicBlock::Successors::iterator j = (*b)->successors_begin();
         j != (*b)->successors_end(); ++j) {
      CHECK_GE((*j)->rpo_number(), 0);
      CHECK_EQ((*j), schedule->GetBlockById((*j)->id()));
    }
  }

  // Verify RPO numbers of blocks.
  CHECK_EQ(start, rpo_order->at(0));  // Start should be first.
  for (size_t b = 0; b < rpo_order->size(); b++) {
    BasicBlock* block = rpo_order->at(b);
    CHECK_EQ(static_cast<int>(b), block->rpo_number());
    BasicBlock* dom = block->dominator();
    if (b == 0) {
      // All blocks except start should have a dominator.
      CHECK_EQ(NULL, dom);
    } else {
      // Check that the immediate dominator appears somewhere before the block.
      CHECK_NE(NULL, dom);
      CHECK_LT(dom->rpo_number(), block->rpo_number());
    }
  }

  // Verify that all blocks reachable from start are in the RPO.
  BoolVector marked(static_cast<int>(count), false, zone);
  {
    ZoneQueue<BasicBlock*> queue(zone);
    queue.push(start);
    marked[start->id().ToSize()] = true;
    while (!queue.empty()) {
      BasicBlock* block = queue.front();
      queue.pop();
      for (size_t s = 0; s < block->SuccessorCount(); s++) {
        BasicBlock* succ = block->SuccessorAt(s);
        if (!marked[succ->id().ToSize()]) {
          marked[succ->id().ToSize()] = true;
          queue.push(succ);
        }
      }
    }
  }
  // Verify marked blocks are in the RPO.
  for (size_t i = 0; i < count; i++) {
    BasicBlock* block = schedule->GetBlockById(BasicBlock::Id::FromSize(i));
    if (marked[i]) {
      CHECK_GE(block->rpo_number(), 0);
      CHECK_EQ(block, rpo_order->at(block->rpo_number()));
    }
  }
  // Verify RPO blocks are marked.
  for (size_t b = 0; b < rpo_order->size(); b++) {
    CHECK(marked[rpo_order->at(b)->id().ToSize()]);
  }

  {
    // Verify the dominance relation.
    ZoneVector<BitVector*> dominators(zone);
    dominators.resize(count, NULL);

    // Compute a set of all the nodes that dominate a given node by using
    // a forward fixpoint. O(n^2).
    ZoneQueue<BasicBlock*> queue(zone);
    queue.push(start);
    dominators[start->id().ToSize()] =
        new (zone) BitVector(static_cast<int>(count), zone);
    while (!queue.empty()) {
      BasicBlock* block = queue.front();
      queue.pop();
      BitVector* block_doms = dominators[block->id().ToSize()];
      BasicBlock* idom = block->dominator();
      if (idom != NULL && !block_doms->Contains(idom->id().ToInt())) {
        V8_Fatal(__FILE__, __LINE__, "Block B%d is not dominated by B%d",
                 block->id().ToInt(), idom->id().ToInt());
      }
      for (size_t s = 0; s < block->SuccessorCount(); s++) {
        BasicBlock* succ = block->SuccessorAt(s);
        BitVector* succ_doms = dominators[succ->id().ToSize()];

        if (succ_doms == NULL) {
          // First time visiting the node. S.doms = B U B.doms
          succ_doms = new (zone) BitVector(static_cast<int>(count), zone);
          succ_doms->CopyFrom(*block_doms);
          succ_doms->Add(block->id().ToInt());
          dominators[succ->id().ToSize()] = succ_doms;
          queue.push(succ);
        } else {
          // Nth time visiting the successor. S.doms = S.doms ^ (B U B.doms)
          bool had = succ_doms->Contains(block->id().ToInt());
          if (had) succ_doms->Remove(block->id().ToInt());
          if (succ_doms->IntersectIsChanged(*block_doms)) queue.push(succ);
          if (had) succ_doms->Add(block->id().ToInt());
        }
      }
    }

    // Verify the immediateness of dominators.
    for (BasicBlockVector::iterator b = rpo_order->begin();
         b != rpo_order->end(); ++b) {
      BasicBlock* block = *b;
      BasicBlock* idom = block->dominator();
      if (idom == NULL) continue;
      BitVector* block_doms = dominators[block->id().ToSize()];

      for (BitVector::Iterator it(block_doms); !it.Done(); it.Advance()) {
        BasicBlock* dom =
            schedule->GetBlockById(BasicBlock::Id::FromInt(it.Current()));
        if (dom != idom &&
            !dominators[idom->id().ToSize()]->Contains(dom->id().ToInt())) {
          V8_Fatal(__FILE__, __LINE__,
                   "Block B%d is not immediately dominated by B%d",
                   block->id().ToInt(), idom->id().ToInt());
        }
      }
    }
  }

  // Verify phis are placed in the block of their control input.
  for (BasicBlockVector::iterator b = rpo_order->begin(); b != rpo_order->end();
       ++b) {
    for (BasicBlock::const_iterator i = (*b)->begin(); i != (*b)->end(); ++i) {
      Node* phi = *i;
      if (phi->opcode() != IrOpcode::kPhi) continue;
      // TODO(titzer): Nasty special case. Phis from RawMachineAssembler
      // schedules don't have control inputs.
      if (phi->InputCount() > phi->op()->ValueInputCount()) {
        Node* control = NodeProperties::GetControlInput(phi);
        CHECK(control->opcode() == IrOpcode::kMerge ||
              control->opcode() == IrOpcode::kLoop);
        CHECK_EQ((*b), schedule->block(control));
      }
    }
  }

  // Verify that all uses are dominated by their definitions.
  for (BasicBlockVector::iterator b = rpo_order->begin(); b != rpo_order->end();
       ++b) {
    BasicBlock* block = *b;

    // Check inputs to control for this block.
    Node* control = block->control_input();
    if (control != NULL) {
      CHECK_EQ(block, schedule->block(control));
      CheckInputsDominate(schedule, block, control,
                          static_cast<int>(block->NodeCount()) - 1);
    }
    // Check inputs for all nodes in the block.
    for (size_t i = 0; i < block->NodeCount(); i++) {
      Node* node = block->NodeAt(i);
      CheckInputsDominate(schedule, block, node, static_cast<int>(i) - 1);
    }
  }
}
}
}
}  // namespace v8::internal::compiler
