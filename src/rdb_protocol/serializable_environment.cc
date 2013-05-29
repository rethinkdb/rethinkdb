#include "rdb_protocol/serializable_environment.hpp"

#include "containers/archive/boost_types.hpp"

namespace query_language {

RDB_IMPL_ME_SERIALIZABLE_2(term_info_t, type, deterministic);

RDB_IMPL_ME_SERIALIZABLE_2(type_checking_environment_t, scope, implicit_type);

RDB_IMPL_ME_SERIALIZABLE_2(scopes_t, type_env, implicit_attribute_value);



}  // namespace query_language
