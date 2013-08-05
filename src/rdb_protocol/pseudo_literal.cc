#include "rdb_protocol/pseudo_literal.hpp"

namespace ql {
namespace pseudo {

const char *const literal_string = "literal";
const char *const value_key = "value";


void rcheck_literal_valid(const datum_t *lit) {
    for (auto it = lit->as_object().begin(); it != lit->as_object().end(); ++it) {
        if (it->first == datum_t::reql_type_string) {
            r_sanity_check(it->second->as_str() == literal_string);
        } else if (it->first == value_key) {
        } else {
            rfail_target(lit, base_exc_t::GENERIC,
                         "Invalid literal term with illegal key `%s`.",
                         it->first.c_str());
        }
    }
}

} // namespace pseudo
} // namespace ql
