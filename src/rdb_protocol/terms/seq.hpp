#ifndef RDB_PROTOCOL_TERMS_SEQ_HPP_
#define RDB_PROTOCOL_TERMS_SEQ_HPP_

#include "containers/counted.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/term.hpp"

namespace ql {
class env_t;

counted_t<term_t> make_between_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_reduce_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_map_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_filter_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_concatmap_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_count_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_union_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_zip_term(env_t *env, protob_t<const Term> term);

} //namespace ql

#endif // RDB_PROTOCOL_TERMS_SEQ_HPP_
