#include "rdb_protocol/serializable_environment.hpp"

#include "containers/archive/boost_types.hpp"

namespace query_language {

RDB_IMPL_ME_SERIALIZABLE_2(term_info_t, type, deterministic);

RDB_IMPL_ME_SERIALIZABLE_0(scopes_t);



}  // namespace query_language
