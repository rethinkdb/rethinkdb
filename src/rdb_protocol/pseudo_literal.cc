// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/pseudo_literal.hpp"

#include "utils.hpp"

namespace ql {
namespace pseudo {

const char *const literal_string = "LITERAL";
const char *const value_key = "value";

void rcheck_literal_valid(const datum_t *lit) {
    for (size_t i = 0; i < lit->obj_size(); ++i) {
        auto pair = lit->get_pair(i);
        if (pair.first == datum_t::reql_type_string) {
            r_sanity_check(pair.second.as_str() == literal_string);
        } else if (pair.first == value_key) {
        } else {
            rfail_target(lit, base_exc_t::LOGIC,
                         "Invalid literal term with illegal key `%s`.",
                         pair.first.to_std().c_str());
        }
    }
}

} // namespace pseudo
} // namespace ql
