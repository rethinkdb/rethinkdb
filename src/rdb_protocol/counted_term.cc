#include "rdb_protocol/counted_term.hpp"

#include "rdb_protocol/ql2.pb.h"

namespace ql {

void deref_term_t::operator=(const Term &assignee) const {
    *pointee = assignee;
}


counted_term_t::counted_term_t(Term *pointee, intptr_t *refcount, Term *destructee)
    : pointee_(pointee), refcount_(refcount), destructee_(destructee) { }

counted_term_t counted_term_t::make(Term *term) {
    return counted_term_t(term, new intptr_t(1), term);
}

counted_term_t counted_term_t::make_child(counted_term_t parent, Term *child) {
    rassert(*parent.refcount_ > 0);
    ++*parent.refcount_;
    return counted_term_t(child, parent.refcount_, parent.destructee_);
}

counted_term_t::counted_term_t(const counted_term_t &copyee)
    : pointee_(copyee.pointee_), refcount_(copyee.refcount_), destructee_(copyee.destructee_) {
    rassert(*copyee.refcount_ > 0);
    ++*refcount_;
}

counted_term_t::~counted_term_t() {
    rassert(*refcount_ > 0);
    --*refcount_;
    if (*refcount_ == 0) {
        delete destructee_;
    }
}

counted_term_t &counted_term_t::operator=(const counted_term_t &assignee) {
    counted_term_t tmp(assignee);
    swap(tmp);
    return *this;
}

void counted_term_t::swap(counted_term_t &other) {
    std::swap(pointee_, other.pointee_);
    std::swap(refcount_, other.refcount_);
    std::swap(destructee_, other.destructee_);
}

}  // namespace ql
