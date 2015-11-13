// Copyright 2015 RethinkDB, all rights reserved
#ifndef PPRINT_SEXP_PPRINT_HPP_
#define PPRINT_SEXP_PPRINT_HPP_

#include "pprint.hpp"

namespace ql {
class raw_term_t;
}

namespace pprint {
    counted_t<const document_t> render_as_sexp(const ql::raw_term_t &t);
}

#endif // PPRINT_SEXP_PPRINT_HPP_
