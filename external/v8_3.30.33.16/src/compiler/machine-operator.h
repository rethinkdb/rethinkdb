// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_COMPILER_MACHINE_OPERATOR_H_
#define V8_COMPILER_MACHINE_OPERATOR_H_

#include "src/base/flags.h"
#include "src/compiler/machine-type.h"

namespace v8 {
namespace internal {
namespace compiler {

// Forward declarations.
struct MachineOperatorGlobalCache;
class Operator;


// Supported write barrier modes.
enum WriteBarrierKind { kNoWriteBarrier, kFullWriteBarrier };

std::ostream& operator<<(std::ostream& os, WriteBarrierKind);


// A Load needs a MachineType.
typedef MachineType LoadRepresentation;


// A Store needs a MachineType and a WriteBarrierKind in order to emit the
// correct write barrier.
class StoreRepresentation FINAL {
 public:
  StoreRepresentation(MachineType machine_type,
                      WriteBarrierKind write_barrier_kind)
      : machine_type_(machine_type), write_barrier_kind_(write_barrier_kind) {}

  MachineType machine_type() const { return machine_type_; }
  WriteBarrierKind write_barrier_kind() const { return write_barrier_kind_; }

 private:
  MachineType machine_type_;
  WriteBarrierKind write_barrier_kind_;
};

bool operator==(StoreRepresentation, StoreRepresentation);
bool operator!=(StoreRepresentation, StoreRepresentation);

size_t hash_value(StoreRepresentation);

std::ostream& operator<<(std::ostream&, StoreRepresentation);

StoreRepresentation const& StoreRepresentationOf(Operator const*);


// Interface for building machine-level operators. These operators are
// machine-level but machine-independent and thus define a language suitable
// for generating code to run on architectures such as ia32, x64, arm, etc.
class MachineOperatorBuilder FINAL : public ZoneObject {
 public:
  // Flags that specify which operations are available. This is useful
  // for operations that are unsupported by some back-ends.
  enum Flag {
    kNoFlags = 0u,
    kFloat64Floor = 1u << 0,
    kFloat64Ceil = 1u << 1,
    kFloat64RoundTruncate = 1u << 2,
    kFloat64RoundTiesAway = 1u << 3,
    kInt32DivIsSafe = 1u << 4,
    kInt32ModIsSafe = 1u << 5,
    kUint32DivIsSafe = 1u << 6,
    kUint32ModIsSafe = 1u << 7
  };
  typedef base::Flags<Flag, unsigned> Flags;

  explicit MachineOperatorBuilder(MachineType word = kMachPtr,
                                  Flags supportedOperators = kNoFlags);

  const Operator* Word32And();
  const Operator* Word32Or();
  const Operator* Word32Xor();
  const Operator* Word32Shl();
  const Operator* Word32Shr();
  const Operator* Word32Sar();
  const Operator* Word32Ror();
  const Operator* Word32Equal();

  const Operator* Word64And();
  const Operator* Word64Or();
  const Operator* Word64Xor();
  const Operator* Word64Shl();
  const Operator* Word64Shr();
  const Operator* Word64Sar();
  const Operator* Word64Ror();
  const Operator* Word64Equal();

  const Operator* Int32Add();
  const Operator* Int32AddWithOverflow();
  const Operator* Int32Sub();
  const Operator* Int32SubWithOverflow();
  const Operator* Int32Mul();
  const Operator* Int32MulHigh();
  const Operator* Int32Div();
  const Operator* Int32Mod();
  const Operator* Int32LessThan();
  const Operator* Int32LessThanOrEqual();
  const Operator* Uint32Div();
  const Operator* Uint32LessThan();
  const Operator* Uint32LessThanOrEqual();
  const Operator* Uint32Mod();
  const Operator* Uint32MulHigh();
  bool Int32DivIsSafe() const { return flags_ & kInt32DivIsSafe; }
  bool Int32ModIsSafe() const { return flags_ & kInt32ModIsSafe; }
  bool Uint32DivIsSafe() const { return flags_ & kUint32DivIsSafe; }
  bool Uint32ModIsSafe() const { return flags_ & kUint32ModIsSafe; }

  const Operator* Int64Add();
  const Operator* Int64Sub();
  const Operator* Int64Mul();
  const Operator* Int64Div();
  const Operator* Int64Mod();
  const Operator* Int64LessThan();
  const Operator* Int64LessThanOrEqual();
  const Operator* Uint64Div();
  const Operator* Uint64LessThan();
  const Operator* Uint64Mod();

  // These operators change the representation of numbers while preserving the
  // value of the number. Narrowing operators assume the input is representable
  // in the target type and are *not* defined for other inputs.
  // Use narrowing change operators only when there is a static guarantee that
  // the input value is representable in the target value.
  const Operator* ChangeFloat32ToFloat64();
  const Operator* ChangeFloat64ToInt32();   // narrowing
  const Operator* ChangeFloat64ToUint32();  // narrowing
  const Operator* ChangeInt32ToFloat64();
  const Operator* ChangeInt32ToInt64();
  const Operator* ChangeUint32ToFloat64();
  const Operator* ChangeUint32ToUint64();

  // These operators truncate numbers, both changing the representation of
  // the number and mapping multiple input values onto the same output value.
  const Operator* TruncateFloat64ToFloat32();
  const Operator* TruncateFloat64ToInt32();  // JavaScript semantics.
  const Operator* TruncateInt64ToInt32();

  // Floating point operators always operate with IEEE 754 round-to-nearest.
  const Operator* Float64Add();
  const Operator* Float64Sub();
  const Operator* Float64Mul();
  const Operator* Float64Div();
  const Operator* Float64Mod();
  const Operator* Float64Sqrt();

  // Floating point comparisons complying to IEEE 754.
  const Operator* Float64Equal();
  const Operator* Float64LessThan();
  const Operator* Float64LessThanOrEqual();

  // Floating point rounding.
  const Operator* Float64Floor();
  const Operator* Float64Ceil();
  const Operator* Float64RoundTruncate();
  const Operator* Float64RoundTiesAway();
  bool HasFloat64Floor() { return flags_ & kFloat64Floor; }
  bool HasFloat64Ceil() { return flags_ & kFloat64Ceil; }
  bool HasFloat64RoundTruncate() { return flags_ & kFloat64RoundTruncate; }
  bool HasFloat64RoundTiesAway() { return flags_ & kFloat64RoundTiesAway; }

  // load [base + index]
  const Operator* Load(LoadRepresentation rep);

  // store [base + index], value
  const Operator* Store(StoreRepresentation rep);

  // Access to the machine stack.
  const Operator* LoadStackPointer();

  // Target machine word-size assumed by this builder.
  bool Is32() const { return word() == kRepWord32; }
  bool Is64() const { return word() == kRepWord64; }
  MachineType word() const { return word_; }

// Pseudo operators that translate to 32/64-bit operators depending on the
// word-size of the target machine assumed by this builder.
#define PSEUDO_OP_LIST(V) \
  V(Word, And)            \
  V(Word, Or)             \
  V(Word, Xor)            \
  V(Word, Shl)            \
  V(Word, Shr)            \
  V(Word, Sar)            \
  V(Word, Ror)            \
  V(Word, Equal)          \
  V(Int, Add)             \
  V(Int, Sub)             \
  V(Int, Mul)             \
  V(Int, Div)             \
  V(Int, Mod)             \
  V(Int, LessThan)        \
  V(Int, LessThanOrEqual) \
  V(Uint, Div)            \
  V(Uint, LessThan)       \
  V(Uint, Mod)
#define PSEUDO_OP(Prefix, Suffix)                                \
  const Operator* Prefix##Suffix() {                             \
    return Is32() ? Prefix##32##Suffix() : Prefix##64##Suffix(); \
  }
  PSEUDO_OP_LIST(PSEUDO_OP)
#undef PSEUDO_OP
#undef PSEUDO_OP_LIST

 private:
  const MachineOperatorGlobalCache& cache_;
  const MachineType word_;
  const Flags flags_;
  DISALLOW_COPY_AND_ASSIGN(MachineOperatorBuilder);
};


DEFINE_OPERATORS_FOR_FLAGS(MachineOperatorBuilder::Flags)
}  // namespace compiler
}  // namespace internal
}  // namespace v8

#endif  // V8_COMPILER_MACHINE_OPERATOR_H_
