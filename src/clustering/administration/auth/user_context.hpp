// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_USER_CONTEXT_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_USER_CONTEXT_HPP

#include <set>
#include <string>

#include "errors.hpp"
#include <boost/variant.hpp>

#include "clustering/administration/auth/permissions.hpp"
#include "clustering/administration/auth/username.hpp"
#include "containers/uuid.hpp"
#include "rpc/serialize_macros.hpp"

namespace auth {

class user_context_t
{
public:
    user_context_t();
    user_context_t(permissions_t permissions);
    user_context_t(username_t username, bool read_only = false);

    bool is_admin() const;

    void require_read_permission(
            rdb_context_t *rdb_context,
            database_id_t const &database_id,
            namespace_id_t const &table_id) const;

    void require_write_permission(
            rdb_context_t *rdb_context,
            database_id_t const &database_id,
            namespace_id_t const &table_id) const;

    void require_config_permission(rdb_context_t *rdb_context) const;
    void require_config_permission(
            rdb_context_t *rdb_context,
            database_id_t const &database) const;
    void require_config_permission(
            rdb_context_t *rdb_context,
            database_id_t const &database_id,
            namespace_id_t const &table_id) const;
    void require_config_permission(
            rdb_context_t *rdb_context,
            database_id_t const &database_id,
            std::set<namespace_id_t> const &table_ids) const;

    void require_connect_permission(rdb_context_t *rdb_context) const;

    RDB_DECLARE_ME_SERIALIZABLE(user_context_t);

private:
    boost::variant<permissions_t, username_t> m_context;
    bool m_read_only;
};

}  // namespace auth

#endif  // CLUSTERING_ADMINISTRATION_AUTH_USER_CONTEXT_HPP
