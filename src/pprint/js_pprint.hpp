// Copyright 2015 RethinkDB, all rights reserved
#ifndef PPRINT_JS_PPRINT_HPP_
#define PPRINT_JS_PPRINT_HPP_

#include "pprint.hpp"

class Term;

namespace pprint {
    counted_t<const document_t> render_as_javascript(Term *t);
}

#endif // PPRINT_JS_PPRINT_HPP_
