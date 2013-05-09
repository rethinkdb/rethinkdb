#ifndef RDB_PROTOCOL_TERMS_DATUM_TERMS_HPP_
#define RDB_PROTOCOL_TERMS_DATUM_TERMS_HPP_

#include "containers/counted.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/term.hpp"

namespace ql {
class env_t;

counted_t<term_t> make_datum_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_make_array_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_make_obj_term(env_t *env, protob_t<const Term> term);

} // namespace ql

#endif // RDB_PROTOCOL_TERMS_DATUM_TERMS_HPP_
