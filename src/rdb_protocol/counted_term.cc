#include "rdb_protocol/counted_term.hpp"

#include "rdb_protocol/ql2.pb.h"

namespace ql {

protob_destructable_t::protob_destructable_t(intptr_t *refcount, Term *destructee)
    : refcount_(refcount), destructee_(destructee) { }

protob_destructable_t::protob_destructable_t(const protob_destructable_t &copyee)
    : refcount_(copyee.refcount_), destructee_(copyee.destructee_) {
    rassert(*copyee.refcount_ > 0);
    ++*refcount_;
}

protob_destructable_t::~protob_destructable_t() {
    rassert(*refcount_ > 0);
    --*refcount_;
    if (*refcount_ == 0) {
        delete destructee_;
    }
}

protob_destructable_t &protob_destructable_t::operator=(const protob_destructable_t &assignee) {
    protob_destructable_t tmp(assignee);
    swap(tmp);
    return *this;
}

void protob_destructable_t::swap(protob_destructable_t &other) {
    std::swap(refcount_, other.refcount_);
    std::swap(destructee_, other.destructee_);
}

protob_t<Term> make_counted_term() {
    Term *term = new Term;
    return protob_t<Term>(new intptr_t(1), term, term);
}

}  // namespace ql
