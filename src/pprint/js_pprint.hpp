// Copyright 2015 RethinkDB, all rights reserved
#ifndef PPRINT_JS_PPRINT_HPP_
#define PPRINT_JS_PPRINT_HPP_

#include "pprint/pprint.hpp"

namespace ql {
class raw_term_t;
}

namespace pprint {
std::string pretty_print_as_js(size_t width, const ql::raw_term_t &t);
}

#endif // PPRINT_JS_PPRINT_HPP_
