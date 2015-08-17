// Copyright 2011 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/v8.h"

#if V8_TARGET_ARCH_X64

#include "src/x64/lithium-codegen-x64.h"
#include "src/x64/lithium-gap-resolver-x64.h"

namespace v8 {
namespace internal {

LGapResolver::LGapResolver(LCodeGen* owner)
    : cgen_(owner), moves_(32, owner->zone()) {}


void LGapResolver::Resolve(LParallelMove* parallel_move) {
  DCHECK(moves_.is_empty());
  // Build up a worklist of moves.
  BuildInitialMoveList(parallel_move);

  for (int i = 0; i < moves_.length(); ++i) {
    LMoveOperands move = moves_[i];
    // Skip constants to perform them last.  They don't block other moves
    // and skipping such moves with register destinations keeps those
    // registers free for the whole algorithm.
    if (!move.IsEliminated() && !move.source()->IsConstantOperand()) {
      PerformMove(i);
    }
  }

  // Perform the moves with constant sources.
  for (int i = 0; i < moves_.length(); ++i) {
    if (!moves_[i].IsEliminated()) {
      DCHECK(moves_[i].source()->IsConstantOperand());
      EmitMove(i);
    }
  }

  moves_.Rewind(0);
}


void LGapResolver::BuildInitialMoveList(LParallelMove* parallel_move) {
  // Perform a linear sweep of the moves to add them to the initial list of
  // moves to perform, ignoring any move that is redundant (the source is
  // the same as the destination, the destination is ignored and
  // unallocated, or the move was already eliminated).
  const ZoneList<LMoveOperands>* moves = parallel_move->move_operands();
  for (int i = 0; i < moves->length(); ++i) {
    LMoveOperands move = moves->at(i);
    if (!move.IsRedundant()) moves_.Add(move, cgen_->zone());
  }
  Verify();
}


void LGapResolver::PerformMove(int index) {
  // Each call to this function performs a move and deletes it from the move
  // graph.  We first recursively perform any move blocking this one.  We
  // mark a move as "pending" on entry to PerformMove in order to detect
  // cycles in the move graph.  We use operand swaps to resolve cycles,
  // which means that a call to PerformMove could change any source operand
  // in the move graph.

  DCHECK(!moves_[index].IsPending());
  DCHECK(!moves_[index].IsRedundant());

  // Clear this move's destination to indicate a pending move.  The actual
  // destination is saved in a stack-allocated local.  Recursion may allow
  // multiple moves to be pending.
  DCHECK(moves_[index].source() != NULL);  // Or else it will look eliminated.
  LOperand* destination = moves_[index].destination();
  moves_[index].set_destination(NULL);

  // Perform a depth-first traversal of the move graph to resolve
  // dependencies.  Any unperformed, unpending move with a source the same
  // as this one's destination blocks this one so recursively perform all
  // such moves.
  for (int i = 0; i < moves_.length(); ++i) {
    LMoveOperands other_move = moves_[i];
    if (other_move.Blocks(destination) && !other_move.IsPending()) {
      // Though PerformMove can change any source operand in the move graph,
      // this call cannot create a blocking move via a swap (this loop does
      // not miss any).  Assume there is a non-blocking move with source A
      // and this move is blocked on source B and there is a swap of A and
      // B.  Then A and B must be involved in the same cycle (or they would
      // not be swapped).  Since this move's destination is B and there is
      // only a single incoming edge to an operand, this move must also be
      // involved in the same cycle.  In that case, the blocking move will
      // be created but will be "pending" when we return from PerformMove.
      PerformMove(i);
    }
  }

  // We are about to resolve this move and don't need it marked as
  // pending, so restore its destination.
  moves_[index].set_destination(destination);

  // This move's source may have changed due to swaps to resolve cycles and
  // so it may now be the last move in the cycle.  If so remove it.
  if (moves_[index].source()->Equals(destination)) {
    moves_[index].Eliminate();
    return;
  }

  // The move may be blocked on a (at most one) pending move, in which case
  // we have a cycle.  Search for such a blocking move and perform a swap to
  // resolve it.
  for (int i = 0; i < moves_.length(); ++i) {
    LMoveOperands other_move = moves_[i];
    if (other_move.Blocks(destination)) {
      DCHECK(other_move.IsPending());
      EmitSwap(index);
      return;
    }
  }

  // This move is not blocked.
  EmitMove(index);
}


void LGapResolver::Verify() {
#ifdef ENABLE_SLOW_DCHECKS
  // No operand should be the destination for more than one move.
  for (int i = 0; i < moves_.length(); ++i) {
    LOperand* destination = moves_[i].destination();
    for (int j = i + 1; j < moves_.length(); ++j) {
      SLOW_DCHECK(!destination->Equals(moves_[j].destination()));
    }
  }
#endif
}


#define __ ACCESS_MASM(cgen_->masm())


void LGapResolver::EmitMove(int index) {
  LOperand* source = moves_[index].source();
  LOperand* destination = moves_[index].destination();

  // Dispatch on the source and destination operand kinds.  Not all
  // combinations are possible.
  if (source->IsRegister()) {
    Register src = cgen_->ToRegister(source);
    if (destination->IsRegister()) {
      Register dst = cgen_->ToRegister(destination);
      __ movp(dst, src);
    } else {
      DCHECK(destination->IsStackSlot());
      Operand dst = cgen_->ToOperand(destination);
      __ movp(dst, src);
    }

  } else if (source->IsStackSlot()) {
    Operand src = cgen_->ToOperand(source);
    if (destination->IsRegister()) {
      Register dst = cgen_->ToRegister(destination);
      __ movp(dst, src);
    } else {
      DCHECK(destination->IsStackSlot());
      Operand dst = cgen_->ToOperand(destination);
      __ movp(kScratchRegister, src);
      __ movp(dst, kScratchRegister);
    }

  } else if (source->IsConstantOperand()) {
    LConstantOperand* constant_source = LConstantOperand::cast(source);
    if (destination->IsRegister()) {
      Register dst = cgen_->ToRegister(destination);
      if (cgen_->IsSmiConstant(constant_source)) {
        __ Move(dst, cgen_->ToSmi(constant_source));
      } else if (cgen_->IsInteger32Constant(constant_source)) {
        int32_t constant = cgen_->ToInteger32(constant_source);
        // Do sign extension only for constant used as de-hoisted array key.
        // Others only need zero extension, which saves 2 bytes.
        if (cgen_->IsDehoistedKeyConstant(constant_source)) {
          __ Set(dst, constant);
        } else {
          __ Set(dst, static_cast<uint32_t>(constant));
        }
      } else {
        __ Move(dst, cgen_->ToHandle(constant_source));
      }
    } else if (destination->IsDoubleRegister()) {
      double v = cgen_->ToDouble(constant_source);
      uint64_t int_val = bit_cast<uint64_t, double>(v);
      XMMRegister dst = cgen_->ToDoubleRegister(destination);
      if (int_val == 0) {
        __ xorps(dst, dst);
      } else {
        __ Set(kScratchRegister, int_val);
        __ movq(dst, kScratchRegister);
      }
    } else {
      DCHECK(destination->IsStackSlot());
      Operand dst = cgen_->ToOperand(destination);
      if (cgen_->IsSmiConstant(constant_source)) {
        __ Move(dst, cgen_->ToSmi(constant_source));
      } else if (cgen_->IsInteger32Constant(constant_source)) {
        // Do sign extension to 64 bits when stored into stack slot.
        __ movp(dst, Immediate(cgen_->ToInteger32(constant_source)));
      } else {
        __ Move(kScratchRegister, cgen_->ToHandle(constant_source));
        __ movp(dst, kScratchRegister);
      }
    }

  } else if (source->IsDoubleRegister()) {
    XMMRegister src = cgen_->ToDoubleRegister(source);
    if (destination->IsDoubleRegister()) {
      __ movaps(cgen_->ToDoubleRegister(destination), src);
    } else {
      DCHECK(destination->IsDoubleStackSlot());
      __ movsd(cgen_->ToOperand(destination), src);
    }
  } else if (source->IsDoubleStackSlot()) {
    Operand src = cgen_->ToOperand(source);
    if (destination->IsDoubleRegister()) {
      __ movsd(cgen_->ToDoubleRegister(destination), src);
    } else {
      DCHECK(destination->IsDoubleStackSlot());
      __ movsd(xmm0, src);
      __ movsd(cgen_->ToOperand(destination), xmm0);
    }
  } else {
    UNREACHABLE();
  }

  moves_[index].Eliminate();
}


void LGapResolver::EmitSwap(int index) {
  LOperand* source = moves_[index].source();
  LOperand* destination = moves_[index].destination();

  // Dispatch on the source and destination operand kinds.  Not all
  // combinations are possible.
  if (source->IsRegister() && destination->IsRegister()) {
    // Swap two general-purpose registers.
    Register src = cgen_->ToRegister(source);
    Register dst = cgen_->ToRegister(destination);
    __ xchgq(dst, src);

  } else if ((source->IsRegister() && destination->IsStackSlot()) ||
             (source->IsStackSlot() && destination->IsRegister())) {
    // Swap a general-purpose register and a stack slot.
    Register reg =
        cgen_->ToRegister(source->IsRegister() ? source : destination);
    Operand mem =
        cgen_->ToOperand(source->IsRegister() ? destination : source);
    __ movp(kScratchRegister, mem);
    __ movp(mem, reg);
    __ movp(reg, kScratchRegister);

  } else if ((source->IsStackSlot() && destination->IsStackSlot()) ||
      (source->IsDoubleStackSlot() && destination->IsDoubleStackSlot())) {
    // Swap two stack slots or two double stack slots.
    Operand src = cgen_->ToOperand(source);
    Operand dst = cgen_->ToOperand(destination);
    __ movsd(xmm0, src);
    __ movp(kScratchRegister, dst);
    __ movsd(dst, xmm0);
    __ movp(src, kScratchRegister);

  } else if (source->IsDoubleRegister() && destination->IsDoubleRegister()) {
    // Swap two double registers.
    XMMRegister source_reg = cgen_->ToDoubleRegister(source);
    XMMRegister destination_reg = cgen_->ToDoubleRegister(destination);
    __ movaps(xmm0, source_reg);
    __ movaps(source_reg, destination_reg);
    __ movaps(destination_reg, xmm0);

  } else if (source->IsDoubleRegister() || destination->IsDoubleRegister()) {
    // Swap a double register and a double stack slot.
    DCHECK((source->IsDoubleRegister() && destination->IsDoubleStackSlot()) ||
           (source->IsDoubleStackSlot() && destination->IsDoubleRegister()));
    XMMRegister reg = cgen_->ToDoubleRegister(source->IsDoubleRegister()
                                                  ? source
                                                  : destination);
    LOperand* other = source->IsDoubleRegister() ? destination : source;
    DCHECK(other->IsDoubleStackSlot());
    Operand other_operand = cgen_->ToOperand(other);
    __ movsd(xmm0, other_operand);
    __ movsd(other_operand, reg);
    __ movaps(reg, xmm0);

  } else {
    // No other combinations are possible.
    UNREACHABLE();
  }

  // The swap of source and destination has executed a move from source to
  // destination.
  moves_[index].Eliminate();

  // Any unperformed (including pending) move with a source of either
  // this move's source or destination needs to have their source
  // changed to reflect the state of affairs after the swap.
  for (int i = 0; i < moves_.length(); ++i) {
    LMoveOperands other_move = moves_[i];
    if (other_move.Blocks(source)) {
      moves_[i].set_source(destination);
    } else if (other_move.Blocks(destination)) {
      moves_[i].set_source(source);
    }
  }
}

#undef __

} }  // namespace v8::internal

#endif  // V8_TARGET_ARCH_X64
