#ifndef RDB_PROTOCOL_TERMS_DB_TABLE_HPP_
#define RDB_PROTOCOL_TERMS_DB_TABLE_HPP_

#include "containers/counted.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/term.hpp"

namespace ql {
class env_t;

counted_t<term_t> make_db_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_table_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_get_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_get_all_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_db_create_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_db_drop_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_db_list_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_table_create_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_table_drop_term(env_t *env, protob_t<const Term> term);
counted_t<term_t> make_table_list_term(env_t *env, protob_t<const Term> term);

} // namespace ql

#endif // RDB_PROTOCOL_TERMS_DB_TABLE_HPP_
