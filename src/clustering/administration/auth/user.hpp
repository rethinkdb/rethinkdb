// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_USER_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_USER_HPP

#include <map>
#include <string>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "clustering/administration/auth/global_permissions.hpp"
#include "clustering/administration/auth/password.hpp"
#include "clustering/administration/auth/permissions.hpp"
#include "clustering/administration/auth/username.hpp"
#include "containers/uuid.hpp"
#include "rdb_protocol/context.hpp"
#include "rpc/serialize_macros.hpp"

class cluster_semilattice_metadata_t;
class table_basic_config_t;
class table_meta_client_t;

namespace auth {

// When passed to the constructor of `user_t` it will set the defaults for an admin user
class admin_t {
};

class user_t {
public:
    user_t();
    user_t(admin_t);
    user_t(
        ql::datum_t const &datum,
        table_meta_client_t *table_meta_client,
        cluster_semilattice_metadata_t const &cluster_metadata,
        admin_identifier_format_t identifier_format);

    bool has_password() const;
    boost::optional<password_t> const &get_password() const;
    void set_password(boost::optional<std::string> password);

    global_permissions_t get_global_permissions() const;
    void set_global_permissions(global_permissions_t permissions);

    permissions_t get_database_permissions(database_id_t const &database_id) const;
    void set_database_permissions(
        database_id_t const &database_id,
        permissions_t permissions);

    permissions_t get_table_permissions(namespace_id_t const &table_id) const;
    void set_table_permissions(
        namespace_id_t const &table_id,
        permissions_t permissions);

    bool has_read_permission(
        database_id_t const &database_id,
        namespace_id_t const &table_id) const;

    bool has_write_permission(
        database_id_t const &database_id,
        namespace_id_t const &table_id) const;

    bool has_config_permission() const;
    bool has_config_permission(
        database_id_t const &database_id) const;
    bool has_config_permission(
        database_id_t const &database_id,
        namespace_id_t const &table_id) const;

    bool has_connect_permission() const;

    ql::datum_t to_datum(
        std::map<namespace_id_t, table_basic_config_t> const &names,
        cluster_semilattice_metadata_t const &cluster_metadata,
        username_t const &username,
        admin_identifier_format_t identifier_format =
            admin_identifier_format_t::name) const;

    bool operator==(user_t const &rhs) const;

    RDB_DECLARE_ME_SERIALIZABLE(user_t);

private:
    boost::optional<password_t> m_password;
    global_permissions_t m_global_permissions;
    std::map<database_id_t, permissions_t> m_database_permissions;
    std::map<namespace_id_t, permissions_t> m_table_permissions;
};

}  // namespace auth

#endif  // CLUSTERING_ADMINISTRATION_AUTH_USER_HPP
