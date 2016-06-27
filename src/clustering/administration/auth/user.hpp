// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_AUTH_USER_HPP
#define CLUSTERING_ADMINISTRATION_AUTH_USER_HPP

#include <map>
#include <string>

#include "errors.hpp"
#include <boost/optional.hpp>

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
    user_t(password_t password, admin_t);
    user_t(password_t password, permissions_t global_permissions);
    explicit user_t(ql::datum_t const &datum);

    void merge(ql::datum_t const &datum);

    password_t const &get_password() const;
    void set_password(password_t password);

    permissions_t const &get_global_permissions() const;
    permissions_t &get_global_permissions();
    void set_global_permissions(permissions_t permissions);

    std::map<database_id_t, permissions_t> const &get_database_permissions() const;
    permissions_t get_database_permissions(database_id_t const &database_id) const;
    permissions_t &get_database_permissions(database_id_t const &database_id);
    void set_database_permissions(
        database_id_t const &database_id,
        permissions_t permissions);

    std::map<namespace_id_t, permissions_t> const &get_table_permissions() const;
    permissions_t get_table_permissions(namespace_id_t const &table_id) const;
    permissions_t &get_table_permissions(namespace_id_t const &table_id);
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

    bool operator==(user_t const &rhs) const;

    RDB_DECLARE_ME_SERIALIZABLE(user_t);

private:
    password_t m_password;
    permissions_t m_global_permissions;
    std::map<database_id_t, permissions_t> m_database_permissions;
    std::map<namespace_id_t, permissions_t> m_table_permissions;
};

}  // namespace auth

#endif  // CLUSTERING_ADMINISTRATION_AUTH_USER_HPP
