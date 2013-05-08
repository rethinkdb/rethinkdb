#ifndef RDB_PROTOCOL_TERMS_OBJ_HPP_
#define RDB_PROTOCOL_TERMS_OBJ_HPP_

#include "containers/counted.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/term.hpp"

namespace ql {
class env_t;

counted_t<term_t> make_getattr_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_contains_term(env_t *env, protob_t<const Term> term);

} //namespace ql

#endif // RDB_PROTOCOL_TERMS_OBJ_HPP_
