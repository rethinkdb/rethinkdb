#ifndef RDB_PROTOCOL_COUNTED_TERM_HPP_
#define RDB_PROTOCOL_COUNTED_TERM_HPP_

#include <stdint.h>

#include "errors.hpp"

class Term;

namespace ql {

class protob_destructable_t {
protected:
    template <class T> friend class protob_t;
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

private:
    protob_t(const protob_destructable_t &destructable, T *pointee)
        : destructable_(destructable), pointee_(pointee) { }

    friend protob_t<Term> make_counted_term();

    // Used by make_counted_term.
    protob_t(intptr_t *refcount, Term *destructee, T *pointee)
        : destructable_(refcount, destructee), pointee_(pointee) { }

    protob_destructable_t destructable_;
    T *pointee_;
};

// Makes a protob_t<Term>.
protob_t<Term> make_counted_term();

}  // namespace ql

#endif  // RDB_PROTOCOL_COUNTED_TERM_HPP_
