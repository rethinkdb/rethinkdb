#include "rdb_protocol/counted_term.hpp"

#include "rdb_protocol/ql2.pb.h"

namespace ql {

const uintptr_t PROTOB_TERM_TAG = 0;
const uintptr_t PROTOB_QUERY_TAG = 1;

// We have compile-time assertions about these types' sizeof values below.
struct protob_pointee_t {
    uintptr_t tag : 1;  // 0 means protob_term_t, 1 means protob_query_t.
    uintptr_t refcount : sizeof(uintptr_t) - 1;

    protob_pointee_t(uintptr_t _tag) : tag(_tag), refcount(0) { }
};

struct protob_term_t : public protob_pointee_t {
    protob_term_t(const Term &t) : protob_pointee_t(PROTOB_TERM_TAG), term(t) { }

    Term term;
};

struct protob_query_t : public protob_pointee_t {
    protob_query_t() : protob_pointee_t(PROTOB_QUERY_TAG) { }

    Query query;
};

protob_destructable_t::protob_destructable_t()
    : destructee_() { }

protob_destructable_t::protob_destructable_t(protob_pointee_t *destructee)
    : destructee_(destructee) {
    rassert(destructee_->refcount == 0);
    ++destructee_->refcount;
}

protob_destructable_t::protob_destructable_t(const protob_destructable_t &copyee)
    : destructee_(copyee.destructee_) {
    if (destructee_ != NULL) {
        rassert(destructee_->refcount > 0);
        destructee_->refcount += 1;
    }
}

protob_destructable_t::~protob_destructable_t() {
    if (destructee_ != NULL) {
        rassert(destructee_->refcount > 0);
        --destructee_->refcount;
        if (destructee_->refcount == 0) {
            if (destructee_->tag == PROTOB_TERM_TAG) {
                delete static_cast<protob_term_t *>(destructee_);
            } else {
                delete static_cast<protob_query_t *>(destructee_);
            }
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
    protob_term_t *term = new protob_term_t(t);
    return protob_t<Term>(term, &term->term);
}

protob_t<Term> make_counted_term() {
    return make_counted_term_copy(Term());
}

protob_t<Query> make_counted_query() {
    protob_query_t *query = new protob_query_t();
    return protob_t<Query>(query, &query->query);
}

}  // namespace ql
