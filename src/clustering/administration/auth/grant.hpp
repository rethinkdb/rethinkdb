// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_GRANT_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_GRANT_HPP

#include "rdb_protocol/table_common.hpp"

template <class> class semilattice_readwrite_view_t;
struct admin_err_t;

namespace auth {

class user_t;

bool grant(
        std::shared_ptr<semilattice_readwrite_view_t<auth_semilattice_metadata_t>>
            auth_semilattice_view,
        rdb_context_t *rdb_context,
        auth::user_context_t const &user_context,
        auth::username_t username,
        ql::datum_t permissions,
        signal_t *interruptor,
        std::function<auth::permissions_t &(auth::user_t &)> permission_selector_function,
        ql::datum_t *result_out,
        admin_err_t *error_out);

}  // namespace auth

#endif  // CLUSTERING_ADMINISTRATION_AUTH_GRANT_HPP
