// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/counted_term.hpp"

#include "rdb_protocol/ql2.pb.h"

namespace ql {

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
