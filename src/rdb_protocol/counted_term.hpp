#ifndef RDB_PROTOCOL_COUNTED_TERM_HPP_
#define RDB_PROTOCOL_COUNTED_TERM_HPP_

#include <stdint.h>

#include "errors.hpp"

class Query;
class Term;

namespace ql {

class protob_destructable_t {
protected:
    template <class T> friend class protob_t;
    protob_destructable_t();
    protob_destructable_t(intptr_t *refcount, Term *destructee);
    protob_destructable_t(const protob_destructable_t &copyee);
    ~protob_destructable_t();

    protob_destructable_t &operator=(const protob_destructable_t &assignee);

    void swap(protob_destructable_t &other);

    intptr_t *refcount_;
    Term *destructee_;
};

template <class T>
class protob_t {
public:
    protob_t() : pointee_(NULL) { }

    template <class U>
    protob_t(const protob_t<U> &other)
        : destructable_(other.destructable_), pointee_(other.pointee_) { }

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

    protob_t(const protob_destructable_t &destructable, T *pointee)
        : destructable_(destructable), pointee_(pointee) { }

    friend protob_t<Term> make_counted_term_copy(const Term &copyee);
    friend protob_t<Query> special_noncounting_query_protob(Query *query);

    // Used by make_counted_term_copy.
    protob_t(intptr_t *refcount, Term *destructee, T *pointee)
        : destructable_(refcount, destructee), pointee_(pointee) { }

    protob_destructable_t destructable_;
    T *pointee_;
};

// Makes a protob_t<Term> by deep-copying copyee!  Oof.
protob_t<Term> make_counted_term_copy(const Term &copyee);

// Makes a protob_t<Term> with default-constructed Term.
protob_t<Term> make_counted_term();

// Makes a protob_t<Query> that doesn't do any reference counting.
protob_t<Query> special_noncounting_query_protob(Query *query);

}  // namespace ql

#endif  // RDB_PROTOCOL_COUNTED_TERM_HPP_
