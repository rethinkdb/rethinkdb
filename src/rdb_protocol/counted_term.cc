#include "rdb_protocol/counted_term.hpp"

#include "rdb_protocol/ql2.pb.h"
#include "utils.hpp"  // SAMRSI not needed

namespace ql {

const uintptr_t PROTOB_TERM_TAG = 0;
const uintptr_t PROTOB_QUERY_TAG = 1;

// SAMRSI: Change the guarantees in this file back to rasserts.

// We have compile-time assertions about these types' sizeof values below.
struct protob_pointee_t {
    uintptr_t tag : 1;  // 0 means protob_term_t, 1 means protob_query_t.
    uintptr_t destructed : 1;  // SAMRSI: Get rid of this.  And adjust refcount's
                               // bitfield width.
    uintptr_t refcount : sizeof(uintptr_t) * CHAR_BIT - 2;

    protob_pointee_t(uintptr_t _tag) : tag(_tag), destructed(0), refcount(0) { }
};

struct protob_term_t : public protob_pointee_t {
    protob_term_t(const Term &t) : protob_pointee_t(PROTOB_TERM_TAG), term(t) { }
    ~protob_term_t() {
        guarantee(destructed == 0);
        guarantee(tag == PROTOB_TERM_TAG);
        guarantee(refcount == 0);
        destructed = 1;
    }

    Term term;
};

struct protob_query_t : public protob_pointee_t {
    protob_query_t() : protob_pointee_t(PROTOB_QUERY_TAG) { }
    ~protob_query_t() {
        guarantee(destructed == 0);
        guarantee(tag == PROTOB_QUERY_TAG);
        guarantee(refcount == 0);
        destructed = 1;
    }

    Query query;
};

protob_destructable_t::protob_destructable_t()
    : destructee_(NULL) { }

protob_destructable_t::protob_destructable_t(protob_pointee_t *destructee)
    : destructee_(destructee) {
    CT_ASSERT(sizeof(protob_pointee_t) == sizeof(uintptr_t));
    CT_ASSERT(sizeof(protob_term_t) == sizeof(uintptr_t) + sizeof(Term));
    CT_ASSERT(sizeof(protob_query_t) == sizeof(uintptr_t) + sizeof(Query));

    guarantee(destructee_->refcount == 0);
    ++destructee_->refcount;
}

protob_destructable_t::protob_destructable_t(const protob_destructable_t &copyee)
    : destructee_(copyee.destructee_) {
    if (destructee_ != NULL) {
        guarantee(destructee_->refcount > 0);
        destructee_->refcount += 1;
    }
}

protob_destructable_t::~protob_destructable_t() {
    if (destructee_ != NULL) {
        guarantee(destructee_->refcount > 0);
        --destructee_->refcount;
        if (destructee_->refcount == 0) {
            if (destructee_->tag == PROTOB_TERM_TAG) {
                delete static_cast<protob_term_t *>(destructee_);
            } else {
                delete static_cast<protob_query_t *>(destructee_);
            }
            destructee_ = NULL;
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
