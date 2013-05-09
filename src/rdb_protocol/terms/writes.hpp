#ifndef RDB_PROTOCOL_TERMS_WRITES_HPP_
#define RDB_PROTOCOL_TERMS_WRITES_HPP_

#include "containers/counted.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/term.hpp"

namespace ql {
class env_t;

counted_t<term_t> make_insert_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_replace_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_foreach_term(env_t *env, protob_t<const Term> term);

} //namespace ql

#endif // RDB_PROTOCOL_TERMS_WRITES_HPP_
