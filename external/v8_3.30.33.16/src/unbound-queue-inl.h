// Copyright 2010 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_UNBOUND_QUEUE_INL_H_
#define V8_UNBOUND_QUEUE_INL_H_

#include "src/unbound-queue.h"

namespace v8 {
namespace internal {

template<typename Record>
struct UnboundQueue<Record>::Node: public Malloced {
  explicit Node(const Record& value)
      : value(value), next(NULL) {
  }

  Record value;
  Node* next;
};


template<typename Record>
UnboundQueue<Record>::UnboundQueue() {
  first_ = new Node(Record());
  divider_ = last_ = reinterpret_cast<base::AtomicWord>(first_);
}


template<typename Record>
UnboundQueue<Record>::~UnboundQueue() {
  while (first_ != NULL) DeleteFirst();
}


template<typename Record>
void UnboundQueue<Record>::DeleteFirst() {
  Node* tmp = first_;
  first_ = tmp->next;
  delete tmp;
}


template<typename Record>
bool UnboundQueue<Record>::Dequeue(Record* rec) {
  if (divider_ == base::Acquire_Load(&last_)) return false;
  Node* next = reinterpret_cast<Node*>(divider_)->next;
  *rec = next->value;
  base::Release_Store(&divider_, reinterpret_cast<base::AtomicWord>(next));
  return true;
}


template<typename Record>
void UnboundQueue<Record>::Enqueue(const Record& rec) {
  Node*& next = reinterpret_cast<Node*>(last_)->next;
  next = new Node(rec);
  base::Release_Store(&last_, reinterpret_cast<base::AtomicWord>(next));

  while (first_ != reinterpret_cast<Node*>(base::Acquire_Load(&divider_))) {
    DeleteFirst();
  }
}


template<typename Record>
bool UnboundQueue<Record>::IsEmpty() const {
  return base::NoBarrier_Load(&divider_) == base::NoBarrier_Load(&last_);
}


template<typename Record>
Record* UnboundQueue<Record>::Peek() const {
  if (divider_ == base::Acquire_Load(&last_)) return NULL;
  Node* next = reinterpret_cast<Node*>(divider_)->next;
  return &next->value;
}

} }  // namespace v8::internal

#endif  // V8_UNBOUND_QUEUE_INL_H_
