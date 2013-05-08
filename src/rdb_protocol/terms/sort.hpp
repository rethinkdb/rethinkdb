#ifndef RDB_PROTOCOL_TERMS_SORT_HPP_
#define RDB_PROTOCOL_TERMS_SORT_HPP_

#include "containers/counted.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/term.hpp"

namespace ql {
class env_t;

counted_t<term_t> make_orderby_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_distinct_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_asc_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_desc_term(env_t *env, protob_t<const Term> term);

} //namespace ql

#endif // RDB_PROTOCOL_TERMS_SORT_HPP_
