// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_COUNTED_TERM_HPP_
#define RDB_PROTOCOL_COUNTED_TERM_HPP_

#include <stdint.h>

#include "containers/counted.hpp"
#include "errors.hpp"
#include "rdb_protocol/ql2.pb.h"

// This defines the type protob_t.  Imagine you have a deeply nested protocol
// buffer object that is owned by same base protocol buffer object of type Term.
// You want to reference count the protocol buffer objects.  This means that the
// pointer to a sub-object needs own a pointer to the base Term object, so that
// once all pointers to subobjects have destructed, the base Term object is
// destroyed.  That's what protob_t (almost) does.
//
// protob_t also supports the base object type being Query, instead of Term.

namespace ql {

// A struct with a refcount and either a Term or a Query
struct protob_pointee_t : public slow_atomic_countable_t<protob_pointee_t> {
    protob_pointee_t() { }
    virtual ~protob_pointee_t() { }
};

struct protob_term_t : public protob_pointee_t {
    explicit protob_term_t(const Term &t) : term(t) { }

    Term term;
};

struct protob_query_t : public protob_pointee_t {
    protob_query_t() { }

    Query query;
};

template <class T>
class protob_t {
public:
    protob_t() : pointee_(NULL) { }

    template <class U>
    protob_t(const protob_t<U> &other)
        : destructable_(other.destructable_), pointee_(other.pointee_) { }

    protob_t(const protob_t &) = default;

    // Makes a protob_t pointer to some value that is a child of pointee_.  They
    // both share the same base Term object that will eventually get destroyed
    // when all its pointers are released.
    template <class U>
    protob_t<U> make_child(U *child) const {
        return protob_t<U>(destructable_, child);
    }

    T *get() const {
        rassert(pointee_ != NULL);
        return pointee_;
    }

    T &operator*() const { return *get(); }

    T *operator->() const {
        return get();
    }

    void swap(protob_t &other) {
        T *tmp = pointee_;
        pointee_ = other.pointee_;
        other.pointee_ = tmp;
        destructable_.swap(other.destructable_);
    }

    bool has() const {
        return pointee_ != NULL;
    }

private:
    template <class U>
    friend class protob_t;

    protob_t(const counted_t<protob_pointee_t> &destructable, T *pointee)
        : destructable_(destructable), pointee_(pointee) { }

    friend protob_t<Term> make_counted_term_copy(const Term &copyee);
    friend protob_t<Query> make_counted_query();

    // Used by make_counted_term_copy.
    protob_t(protob_pointee_t *term, T *pointee)
        : destructable_(term), pointee_(pointee) { }

    counted_t<protob_pointee_t> destructable_;
    T *pointee_;
};

// Makes a protob_t<Term> by deep-copying copyee!  Oof.
protob_t<Term> make_counted_term_copy(const Term &copyee);

// Makes a protob_t<Term> with default-constructed Term.
protob_t<Term> make_counted_term();

// Makes a protob_t<Query> with a default-constructed Query, which is
// reference-counted (with the base object being a Query!).
protob_t<Query> make_counted_query();

}  // namespace ql

#endif  // RDB_PROTOCOL_COUNTED_TERM_HPP_
