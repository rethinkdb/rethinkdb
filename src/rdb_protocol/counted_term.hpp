#ifndef RDB_PROTOCOL_COUNTED_TERM_HPP_
#define RDB_PROTOCOL_COUNTED_TERM_HPP_

#include <stdint.h>

#include "errors.hpp"

class Term;

namespace ql {

// A single-threaded shared pointer that points at a Term.
class counted_term_t {
public:
    // Makes a counted_term using an unowned Term on the heap.
    static counted_term_t make(Term *term);

    // Makes a counted_term using a _child_ of an existing counted_term_t.  Since the child is
    // really part of the parent, this adds a reference count to the _parent_, not the child.
    static counted_term_t make_child(counted_term_t parent, Term *child);

    counted_term_t(const counted_term_t &copyee);

    ~counted_term_t();

    counted_term_t &operator=(const counted_term_t &assignee);

    Term *get() const {
        rassert(pointee_ != NULL);
        return pointee_;
    }

    Term *operator->() const {
        rassert(pointee_ != NULL);
        return pointee_;
    }

    void swap(counted_term_t &other);

private:
    counted_term_t(Term *pointee, intptr_t *refcount, Term *destructee);

    // RSI should this be const?

    // The value we point at.  We never destruct this (directly).
    Term *pointee_;

    // The reference count, stored on the heap.
    intptr_t *refcount_;

    // The value we destruct, when the refcount hits zero.  This equals pointee_ or points to a
    // Term that owns pointee_.
    Term *destructee_;
};








}  // namespace ql

#endif  // RDB_PROTOCOL_COUNTED_TERM_HPP_
