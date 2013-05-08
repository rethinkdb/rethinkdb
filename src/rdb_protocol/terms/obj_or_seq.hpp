#ifndef RDB_PROTOCOL_TERMS_OBJ_OR_SEQ_HPP_
#define RDB_PROTOCOL_TERMS_OBJ_OR_SEQ_HPP_

#include "containers/counted.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/term.hpp"

namespace ql {
class env_t;

counted_t<term_t> make_pluck_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_without_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_merge_term(env_t *env, protob_t<const Term> term);


} //namespace ql


#endif // RDB_PROTOCOL_TERMS_OBJ_OR_SEQ_HPP_
