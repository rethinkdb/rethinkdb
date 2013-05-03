#include "rdb_protocol/counted_term.hpp"

#include "rdb_protocol/ql2.pb.h"

namespace ql {

// Contains an allocated Term (with a refcount) so that we don't need both a
// refcount pointer and a Term pointer inside protob_destructable_t.  Not to
// mention a heap-allocated refcount.  This saves memory.
struct protob_term_t {
    protob_term_t(intptr_t initial_refcount, const Term &t)
        : refcount(initial_refcount), term(t) { }

    intptr_t refcount;
    Term term;
};

protob_destructable_t::protob_destructable_t()
    : destructee_(NULL) { }

protob_destructable_t::protob_destructable_t(protob_term_t *destructee)
    : destructee_(destructee) { }

protob_destructable_t::protob_destructable_t(const protob_destructable_t &copyee)
    : destructee_(copyee.destructee_) {
    if (destructee_ != NULL) {
        rassert(destructee_->refcount > 0);
        ++destructee_->refcount;
    }
}

protob_destructable_t::~protob_destructable_t() {
    if (destructee_ != NULL) {
        rassert(destructee_->refcount > 0);
        --destructee_->refcount;
        if (destructee_->refcount == 0) {
            delete destructee_;
        }
    }
}

protob_destructable_t &
protob_destructable_t::operator=(const protob_destructable_t &assignee) {
    protob_destructable_t tmp(assignee);
    swap(tmp);
    return *this;
}

void protob_destructable_t::swap(protob_destructable_t &other) {
    std::swap(destructee_, other.destructee_);
}

protob_t<Term> make_counted_term_copy(const Term &t) {
    protob_term_t *term = new protob_term_t(1, t);
    return protob_t<Term>(term, &term->term);
}

protob_t<Term> make_counted_term() {
    return make_counted_term_copy(Term());
}

protob_t<Query> special_noncounting_query_protob(Query *query) {
    return protob_t<Query>(NULL, query);
}

}  // namespace ql
