// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/v8.h"

#include "src/assembler.h"
#include "src/compiler/js-graph.h"
#include "src/compiler/node-properties-inl.h"
#include "src/compiler/typer.h"
#include "src/types.h"
#include "test/cctest/cctest.h"
#include "test/cctest/compiler/value-helper.h"

using namespace v8::internal;
using namespace v8::internal::compiler;

class JSCacheTesterHelper {
 protected:
  explicit JSCacheTesterHelper(Zone* zone)
      : main_graph_(zone),
        main_common_(zone),
        main_javascript_(zone),
        main_typer_(&main_graph_, MaybeHandle<Context>()),
        main_machine_() {}
  Graph main_graph_;
  CommonOperatorBuilder main_common_;
  JSOperatorBuilder main_javascript_;
  Typer main_typer_;
  MachineOperatorBuilder main_machine_;
};


// TODO(dcarney): JSConstantCacheTester inherits from JSGraph???
class JSConstantCacheTester : public HandleAndZoneScope,
                              public JSCacheTesterHelper,
                              public JSGraph {
 public:
  JSConstantCacheTester()
      : JSCacheTesterHelper(main_zone()),
        JSGraph(&main_graph_, &main_common_, &main_javascript_,
                &main_machine_) {
    main_graph_.SetStart(main_graph_.NewNode(common()->Start(0)));
    main_graph_.SetEnd(main_graph_.NewNode(common()->End()));
    main_typer_.Run();
  }

  Type* upper(Node* node) { return NodeProperties::GetBounds(node).upper; }

  Handle<Object> handle(Node* node) {
    CHECK_EQ(IrOpcode::kHeapConstant, node->opcode());
    return OpParameter<Unique<Object> >(node).handle();
  }

  Factory* factory() { return main_isolate()->factory(); }
};


TEST(ZeroConstant1) {
  JSConstantCacheTester T;

  Node* zero = T.ZeroConstant();

  CHECK_EQ(IrOpcode::kNumberConstant, zero->opcode());
  CHECK_EQ(zero, T.Constant(0));
  CHECK_NE(zero, T.Constant(-0.0));
  CHECK_NE(zero, T.Constant(1.0));
  CHECK_NE(zero, T.Constant(v8::base::OS::nan_value()));
  CHECK_NE(zero, T.Float64Constant(0));
  CHECK_NE(zero, T.Int32Constant(0));

  Type* t = T.upper(zero);

  CHECK(t->Is(Type::Number()));
  CHECK(t->Is(Type::Integral32()));
  CHECK(t->Is(Type::Signed32()));
  CHECK(t->Is(Type::Unsigned32()));
  CHECK(t->Is(Type::SignedSmall()));
  CHECK(t->Is(Type::UnsignedSmall()));
}


TEST(MinusZeroConstant) {
  JSConstantCacheTester T;

  Node* minus_zero = T.Constant(-0.0);
  Node* zero = T.ZeroConstant();

  CHECK_EQ(IrOpcode::kNumberConstant, minus_zero->opcode());
  CHECK_EQ(minus_zero, T.Constant(-0.0));
  CHECK_NE(zero, minus_zero);

  Type* t = T.upper(minus_zero);

  CHECK(t->Is(Type::Number()));
  CHECK(t->Is(Type::MinusZero()));
  CHECK(!t->Is(Type::Integral32()));
  CHECK(!t->Is(Type::Signed32()));
  CHECK(!t->Is(Type::Unsigned32()));
  CHECK(!t->Is(Type::SignedSmall()));
  CHECK(!t->Is(Type::UnsignedSmall()));

  double zero_value = OpParameter<double>(zero);
  double minus_zero_value = OpParameter<double>(minus_zero);

  CHECK_EQ(0.0, zero_value);
  CHECK_NE(-0.0, zero_value);
  CHECK_EQ(-0.0, minus_zero_value);
  CHECK_NE(0.0, minus_zero_value);
}


TEST(ZeroConstant2) {
  JSConstantCacheTester T;

  Node* zero = T.Constant(0);

  CHECK_EQ(IrOpcode::kNumberConstant, zero->opcode());
  CHECK_EQ(zero, T.ZeroConstant());
  CHECK_NE(zero, T.Constant(-0.0));
  CHECK_NE(zero, T.Constant(1.0));
  CHECK_NE(zero, T.Constant(v8::base::OS::nan_value()));
  CHECK_NE(zero, T.Float64Constant(0));
  CHECK_NE(zero, T.Int32Constant(0));

  Type* t = T.upper(zero);

  CHECK(t->Is(Type::Number()));
  CHECK(t->Is(Type::Integral32()));
  CHECK(t->Is(Type::Signed32()));
  CHECK(t->Is(Type::Unsigned32()));
  CHECK(t->Is(Type::SignedSmall()));
  CHECK(t->Is(Type::UnsignedSmall()));
}


TEST(OneConstant1) {
  JSConstantCacheTester T;

  Node* one = T.OneConstant();

  CHECK_EQ(IrOpcode::kNumberConstant, one->opcode());
  CHECK_EQ(one, T.Constant(1));
  CHECK_EQ(one, T.Constant(1.0));
  CHECK_NE(one, T.Constant(1.01));
  CHECK_NE(one, T.Constant(-1.01));
  CHECK_NE(one, T.Constant(v8::base::OS::nan_value()));
  CHECK_NE(one, T.Float64Constant(1.0));
  CHECK_NE(one, T.Int32Constant(1));

  Type* t = T.upper(one);

  CHECK(t->Is(Type::Number()));
  CHECK(t->Is(Type::Integral32()));
  CHECK(t->Is(Type::Signed32()));
  CHECK(t->Is(Type::Unsigned32()));
  CHECK(t->Is(Type::SignedSmall()));
  CHECK(t->Is(Type::UnsignedSmall()));
}


TEST(OneConstant2) {
  JSConstantCacheTester T;

  Node* one = T.Constant(1);

  CHECK_EQ(IrOpcode::kNumberConstant, one->opcode());
  CHECK_EQ(one, T.OneConstant());
  CHECK_EQ(one, T.Constant(1.0));
  CHECK_NE(one, T.Constant(1.01));
  CHECK_NE(one, T.Constant(-1.01));
  CHECK_NE(one, T.Constant(v8::base::OS::nan_value()));
  CHECK_NE(one, T.Float64Constant(1.0));
  CHECK_NE(one, T.Int32Constant(1));

  Type* t = T.upper(one);

  CHECK(t->Is(Type::Number()));
  CHECK(t->Is(Type::Integral32()));
  CHECK(t->Is(Type::Signed32()));
  CHECK(t->Is(Type::Unsigned32()));
  CHECK(t->Is(Type::SignedSmall()));
  CHECK(t->Is(Type::UnsignedSmall()));
}


TEST(Canonicalizations) {
  JSConstantCacheTester T;

  CHECK_EQ(T.ZeroConstant(), T.ZeroConstant());
  CHECK_EQ(T.UndefinedConstant(), T.UndefinedConstant());
  CHECK_EQ(T.TheHoleConstant(), T.TheHoleConstant());
  CHECK_EQ(T.TrueConstant(), T.TrueConstant());
  CHECK_EQ(T.FalseConstant(), T.FalseConstant());
  CHECK_EQ(T.NullConstant(), T.NullConstant());
  CHECK_EQ(T.ZeroConstant(), T.ZeroConstant());
  CHECK_EQ(T.OneConstant(), T.OneConstant());
  CHECK_EQ(T.NaNConstant(), T.NaNConstant());
}


TEST(NoAliasing) {
  JSConstantCacheTester T;

  Node* nodes[] = {T.UndefinedConstant(), T.TheHoleConstant(), T.TrueConstant(),
                   T.FalseConstant(),     T.NullConstant(),    T.ZeroConstant(),
                   T.OneConstant(),       T.NaNConstant(),     T.Constant(21),
                   T.Constant(22.2)};

  for (size_t i = 0; i < arraysize(nodes); i++) {
    for (size_t j = 0; j < arraysize(nodes); j++) {
      if (i != j) CHECK_NE(nodes[i], nodes[j]);
    }
  }
}


TEST(CanonicalizingNumbers) {
  JSConstantCacheTester T;

  FOR_FLOAT64_INPUTS(i) {
    Node* node = T.Constant(*i);
    for (int j = 0; j < 5; j++) {
      CHECK_EQ(node, T.Constant(*i));
    }
  }
}


TEST(NumberTypes) {
  JSConstantCacheTester T;

  FOR_FLOAT64_INPUTS(i) {
    double value = *i;
    Node* node = T.Constant(value);
    CHECK(T.upper(node)->Is(Type::Of(value, T.main_zone())));
  }
}


TEST(HeapNumbers) {
  JSConstantCacheTester T;

  FOR_FLOAT64_INPUTS(i) {
    double value = *i;
    Handle<Object> num = T.factory()->NewNumber(value);
    Handle<HeapNumber> heap = T.factory()->NewHeapNumber(value);
    Node* node1 = T.Constant(value);
    Node* node2 = T.Constant(num);
    Node* node3 = T.Constant(heap);
    CHECK_EQ(node1, node2);
    CHECK_EQ(node1, node3);
  }
}


TEST(OddballHandle) {
  JSConstantCacheTester T;

  CHECK_EQ(T.UndefinedConstant(), T.Constant(T.factory()->undefined_value()));
  CHECK_EQ(T.TheHoleConstant(), T.Constant(T.factory()->the_hole_value()));
  CHECK_EQ(T.TrueConstant(), T.Constant(T.factory()->true_value()));
  CHECK_EQ(T.FalseConstant(), T.Constant(T.factory()->false_value()));
  CHECK_EQ(T.NullConstant(), T.Constant(T.factory()->null_value()));
  CHECK_EQ(T.NaNConstant(), T.Constant(T.factory()->nan_value()));
}


TEST(OddballValues) {
  JSConstantCacheTester T;

  CHECK_EQ(*T.factory()->undefined_value(), *T.handle(T.UndefinedConstant()));
  CHECK_EQ(*T.factory()->the_hole_value(), *T.handle(T.TheHoleConstant()));
  CHECK_EQ(*T.factory()->true_value(), *T.handle(T.TrueConstant()));
  CHECK_EQ(*T.factory()->false_value(), *T.handle(T.FalseConstant()));
  CHECK_EQ(*T.factory()->null_value(), *T.handle(T.NullConstant()));
}


TEST(OddballTypes) {
  JSConstantCacheTester T;

  CHECK(T.upper(T.UndefinedConstant())->Is(Type::Undefined()));
  // TODO(dcarney): figure this out.
  // CHECK(T.upper(T.TheHoleConstant())->Is(Type::Internal()));
  CHECK(T.upper(T.TrueConstant())->Is(Type::Boolean()));
  CHECK(T.upper(T.FalseConstant())->Is(Type::Boolean()));
  CHECK(T.upper(T.NullConstant())->Is(Type::Null()));
  CHECK(T.upper(T.ZeroConstant())->Is(Type::Number()));
  CHECK(T.upper(T.OneConstant())->Is(Type::Number()));
  CHECK(T.upper(T.NaNConstant())->Is(Type::NaN()));
}


TEST(ExternalReferences) {
  // TODO(titzer): test canonicalization of external references.
}


static bool Contains(NodeVector* nodes, Node* n) {
  for (size_t i = 0; i < nodes->size(); i++) {
    if (nodes->at(i) == n) return true;
  }
  return false;
}


static void CheckGetCachedNodesContains(JSConstantCacheTester* T, Node* n) {
  NodeVector nodes(T->main_zone());
  T->GetCachedNodes(&nodes);
  CHECK(Contains(&nodes, n));
}


TEST(JSGraph_GetCachedNodes1) {
  JSConstantCacheTester T;
  CheckGetCachedNodesContains(&T, T.TrueConstant());
  CheckGetCachedNodesContains(&T, T.UndefinedConstant());
  CheckGetCachedNodesContains(&T, T.TheHoleConstant());
  CheckGetCachedNodesContains(&T, T.TrueConstant());
  CheckGetCachedNodesContains(&T, T.FalseConstant());
  CheckGetCachedNodesContains(&T, T.NullConstant());
  CheckGetCachedNodesContains(&T, T.ZeroConstant());
  CheckGetCachedNodesContains(&T, T.OneConstant());
  CheckGetCachedNodesContains(&T, T.NaNConstant());
}


TEST(JSGraph_GetCachedNodes_int32) {
  JSConstantCacheTester T;

  int32_t constants[] = {0,  1,  1,   1,   1,   2,   3,   4,  11, 12, 13,
                         14, 55, -55, -44, -33, -22, -11, 16, 16, 17, 17,
                         18, 18, 19,  19,  20,  20,  21,  21, 22, 23, 24,
                         25, 15, 30,  31,  45,  46,  47,  48};

  for (size_t i = 0; i < arraysize(constants); i++) {
    int count_before = T.graph()->NodeCount();
    NodeVector nodes_before(T.main_zone());
    T.GetCachedNodes(&nodes_before);
    Node* n = T.Int32Constant(constants[i]);
    if (n->id() < count_before) {
      // An old ID indicates a cached node. It should have been in the set.
      CHECK(Contains(&nodes_before, n));
    }
    // Old or new, it should be in the cached set afterwards.
    CheckGetCachedNodesContains(&T, n);
  }
}


TEST(JSGraph_GetCachedNodes_float64) {
  JSConstantCacheTester T;

  double constants[] = {0,   11.1, 12.2,  13,    14,   55.5, -55.5, -44.4,
                        -33, -22,  -11,   0,     11.1, 11.1, 12.3,  12.3,
                        11,  11,   -33.3, -33.3, -22,  -11};

  for (size_t i = 0; i < arraysize(constants); i++) {
    int count_before = T.graph()->NodeCount();
    NodeVector nodes_before(T.main_zone());
    T.GetCachedNodes(&nodes_before);
    Node* n = T.Float64Constant(constants[i]);
    if (n->id() < count_before) {
      // An old ID indicates a cached node. It should have been in the set.
      CHECK(Contains(&nodes_before, n));
    }
    // Old or new, it should be in the cached set afterwards.
    CheckGetCachedNodesContains(&T, n);
  }
}


TEST(JSGraph_GetCachedNodes_int64) {
  JSConstantCacheTester T;

  int32_t constants[] = {0,   11,  12, 13, 14, 55, -55, -44, -33,
                         -22, -11, 16, 16, 17, 17, 18,  18,  19,
                         19,  20,  20, 21, 21, 22, 23,  24,  25};

  for (size_t i = 0; i < arraysize(constants); i++) {
    int count_before = T.graph()->NodeCount();
    NodeVector nodes_before(T.main_zone());
    T.GetCachedNodes(&nodes_before);
    Node* n = T.Int64Constant(constants[i]);
    if (n->id() < count_before) {
      // An old ID indicates a cached node. It should have been in the set.
      CHECK(Contains(&nodes_before, n));
    }
    // Old or new, it should be in the cached set afterwards.
    CheckGetCachedNodesContains(&T, n);
  }
}


TEST(JSGraph_GetCachedNodes_number) {
  JSConstantCacheTester T;

  double constants[] = {0,   11.1, 12.2,  13,    14,   55.5, -55.5, -44.4,
                        -33, -22,  -11,   0,     11.1, 11.1, 12.3,  12.3,
                        11,  11,   -33.3, -33.3, -22,  -11};

  for (size_t i = 0; i < arraysize(constants); i++) {
    int count_before = T.graph()->NodeCount();
    NodeVector nodes_before(T.main_zone());
    T.GetCachedNodes(&nodes_before);
    Node* n = T.Constant(constants[i]);
    if (n->id() < count_before) {
      // An old ID indicates a cached node. It should have been in the set.
      CHECK(Contains(&nodes_before, n));
    }
    // Old or new, it should be in the cached set afterwards.
    CheckGetCachedNodesContains(&T, n);
  }
}


TEST(JSGraph_GetCachedNodes_external) {
  JSConstantCacheTester T;

  ExternalReference constants[] = {ExternalReference::address_of_min_int(),
                                   ExternalReference::address_of_min_int(),
                                   ExternalReference::address_of_min_int(),
                                   ExternalReference::address_of_one_half(),
                                   ExternalReference::address_of_one_half(),
                                   ExternalReference::address_of_min_int(),
                                   ExternalReference::address_of_the_hole_nan(),
                                   ExternalReference::address_of_one_half()};

  for (size_t i = 0; i < arraysize(constants); i++) {
    int count_before = T.graph()->NodeCount();
    NodeVector nodes_before(T.main_zone());
    T.GetCachedNodes(&nodes_before);
    Node* n = T.ExternalConstant(constants[i]);
    if (n->id() < count_before) {
      // An old ID indicates a cached node. It should have been in the set.
      CHECK(Contains(&nodes_before, n));
    }
    // Old or new, it should be in the cached set afterwards.
    CheckGetCachedNodesContains(&T, n);
  }
}


TEST(JSGraph_GetCachedNodes_together) {
  JSConstantCacheTester T;

  Node* constants[] = {
      T.TrueConstant(),
      T.UndefinedConstant(),
      T.TheHoleConstant(),
      T.TrueConstant(),
      T.FalseConstant(),
      T.NullConstant(),
      T.ZeroConstant(),
      T.OneConstant(),
      T.NaNConstant(),
      T.Int32Constant(0),
      T.Int32Constant(1),
      T.Int64Constant(-2),
      T.Int64Constant(-4),
      T.Float64Constant(0.9),
      T.Float64Constant(V8_INFINITY),
      T.Constant(0.99),
      T.Constant(1.11),
      T.ExternalConstant(ExternalReference::address_of_one_half())};

  NodeVector nodes(T.main_zone());
  T.GetCachedNodes(&nodes);

  for (size_t i = 0; i < arraysize(constants); i++) {
    CHECK(Contains(&nodes, constants[i]));
  }
}
