// iopyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/permissions_artificial_table_backend.hpp"

#include "errors.hpp"
#include <boost/algorithm/string/join.hpp>

#include "clustering/administration/artificial_reql_cluster_interface.hpp"
#include "clustering/administration/auth/username.hpp"
#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/tables/name_resolver.hpp"

namespace auth {

permissions_artificial_table_backend_t::permissions_artificial_table_backend_t(
        rdb_context_t *rdb_context,
        lifetime_t<name_resolver_t const &> name_resolver,
        boost::shared_ptr<semilattice_readwrite_view_t<auth_semilattice_metadata_t>>
            auth_semilattice_view,
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t>>
            cluster_semilattice_view,
        admin_identifier_format_t identifier_format)
    : base_artificial_table_backend_t(
        name_string_t::guarantee_valid("permissions"),
        rdb_context,
        name_resolver,
        auth_semilattice_view,
        cluster_semilattice_view),
      m_name_resolver(name_resolver),
      m_identifier_format(identifier_format) {
}

bool permissions_artificial_table_backend_t::read_all_rows_as_vector(
        UNUSED auth::user_context_t const &user_context,
        UNUSED signal_t *interruptor,
        std::vector<ql::datum_t> *rows_out,
        UNUSED admin_err_t *error_out) {
    rows_out->clear();
    on_thread_t on_thread(home_thread());

    cluster_semilattice_metadata_t cluster_metadata =
        m_name_resolver.get_cluster_metadata();

    // The "admin" user is faked here
    {
        ql::datum_t row;
        if (global_to_datum(
                username_t("admin"),
                permissions_t(true, true, true, true),
                &row)) {
            rows_out->push_back(std::move(row));
        }
    }

    {
        ql::datum_t row;
        if (database_to_datum(
                username_t("admin"),
                artificial_reql_cluster_interface_t::database_id,
                permissions_t(true, true, true),
                cluster_metadata,
                &row)) {
            rows_out->push_back(std::move(row));
        }
    }

    auth_semilattice_metadata_t auth_metadata = m_auth_semilattice_view->get();
    for (auto const &user : auth_metadata.m_users) {
        if (user.first.is_admin() || !static_cast<bool>(user.second.get_ref())) {
            // The "admin" user is faked above and not displayed from the metadata.
            continue;
        }

        ql::datum_t username = convert_string_to_datum(user.first.to_string());

        {
            ql::datum_t row;
            if (global_to_datum(
                    user.first,
                    user.second.get_ref()->get_global_permissions(),
                    &row)) {
                rows_out->push_back(std::move(row));
            }
        }

        for (auto const &database : user.second.get_ref()->get_database_permissions()) {
            ql::datum_t row;
            if (database_to_datum(
                    user.first,
                    database.first,
                    database.second,
                    cluster_metadata,
                    &row)) {
                rows_out->push_back(std::move(row));
            }
        }

        for (auto const &table : user.second.get_ref()->get_table_permissions()) {
            // `table_to_datum` will look up the database
            ql::datum_t row;
            if (table_to_datum(
                    user.first,
                    boost::none,
                    table.first,
                    table.second,
                    cluster_metadata,
                    &row)) {
                rows_out->push_back(std::move(row));
            }
        }
    }

    return true;
}

bool permissions_artificial_table_backend_t::read_row(
        UNUSED auth::user_context_t const &user_context,
        ql::datum_t primary_key,
        UNUSED signal_t *interruptor,
        ql::datum_t *row_out,
        UNUSED admin_err_t *error_out) {
    *row_out = ql::datum_t();
    on_thread_t on_thread(home_thread());

    cluster_semilattice_metadata_t cluster_metadata =
        m_name_resolver.get_cluster_metadata();

    username_t username;
    database_id_t database_id;
    namespace_id_t table_id;
    uint8_t array_size = parse_primary_key(
        primary_key,
        cluster_metadata,
        &username,
        &database_id,
        &table_id);
    if (array_size == 0) {
        return true;
    }

    if (username.is_admin()) {
        switch (array_size) {
            case 1:
                global_to_datum(
                    username,
                    permissions_t(true, true, true, true),
                    row_out);
                break;
            case 2:
                if (database_id == artificial_reql_cluster_interface_t::database_id) {
                    database_to_datum(
                        username,
                        database_id,
                        permissions_t(true, true, true),
                        cluster_metadata,
                        row_out);
                }
                break;
        };
        return true;
    }

    auth_semilattice_metadata_t auth_metadata = m_auth_semilattice_view->get();
    auto user = auth_metadata.m_users.find(username);
    if (user == auth_metadata.m_users.end() ||
            !static_cast<bool>(user->second.get_ref())) {
        return true;
    }

    // Note these functions will only set `row_out` on success.
    switch (array_size) {
        case 1:
            global_to_datum(
                username,
                user->second.get_ref()->get_global_permissions(),
                row_out);
            break;
        case 2:
            database_to_datum(
                username,
                database_id,
                user->second.get_ref()->get_database_permissions(database_id),
                cluster_metadata,
                row_out);
            break;
        case 3:
            table_to_datum(
                username,
                database_id,
                table_id,
                user->second.get_ref()->get_table_permissions(table_id),
                cluster_metadata,
                row_out);
            break;
    }

    return true;
}

bool permissions_artificial_table_backend_t::write_row(
        UNUSED auth::user_context_t const &user_context,
        ql::datum_t primary_key,
        bool pkey_was_autogenerated,
        ql::datum_t *new_value_inout,
        UNUSED signal_t *interruptor,
        admin_err_t *error_out) {
    on_thread_t on_thread(home_thread());

    if (pkey_was_autogenerated) {
        *error_out = admin_err_t{
            "You must specify a primary key.", query_state_t::FAILED};
        return false;
    }

    cluster_semilattice_metadata_t cluster_metadata = m_cluster_semilattice_view->get();

    username_t username_primary;
    database_id_t database_id_primary;
    namespace_id_t table_id_primary;
    uint8_t array_size = parse_primary_key(
        primary_key,
        cluster_metadata,
        &username_primary,
        &database_id_primary,
        &table_id_primary,
        error_out);
    if (array_size == 0) {
        return false;
    }

    if (username_primary.is_admin()) {
        *error_out = admin_err_t{
            "The permissions of the user `" + username_primary.to_string() +
            "` can't be modified.",
            query_state_t::FAILED};
        return false;
    }

    auth_semilattice_metadata_t auth_metadata = m_auth_semilattice_view->get();
    auto user = auth_metadata.m_users.find(username_primary);
    if (user == auth_metadata.m_users.end() ||
            !static_cast<bool>(user->second.get_ref())) {
        *error_out = admin_err_t{
            "No user named `" + username_primary.to_string() + "`.",
            query_state_t::FAILED};
        return false;
    }

    if (new_value_inout->has()) {
        std::set<std::string> keys;
        for (size_t i = 0; i < new_value_inout->obj_size(); ++i) {
            keys.insert(new_value_inout->get_pair(i).first.to_std());
        }
        keys.erase("id");

        ql::datum_t username = new_value_inout->get_field("user", ql::NOTHROW);
        if (username.has()) {
            keys.erase("user");
            if (username_t(username.as_str().to_std()) != username_primary) {
                *error_out = admin_err_t{
                    "The key `user` does not match the primary key.",
                    query_state_t::FAILED};
                return false;
            }
        }

        if (array_size > 1) {
            ql::datum_t database = new_value_inout->get_field("database", ql::NOTHROW);
            if (database.has()) {
                keys.erase("database");

                switch (m_identifier_format) {
                    case admin_identifier_format_t::name:
                        {
                            name_string_t database_name;
                            if (!convert_name_from_datum(
                                    database,
                                    "database name",
                                    &database_name,
                                    error_out)) {
                                return false;
                            }


                            if (m_name_resolver.database_id_to_name(
                                        database_id_primary, cluster_metadata
                                    ).get() != database_name) {
                                *error_out = admin_err_t{
                                    "The key `database` does not match the primary key.",
                                    query_state_t::FAILED};
                                return false;
                            }
                        }
                        break;
                    case admin_identifier_format_t::uuid:
                        {
                            database_id_t database_id_secondary;
                            if (!convert_uuid_from_datum(
                                    database, &database_id_secondary, error_out)) {
                                return false;
                            }

                            if (database_id_primary != database_id_secondary) {
                                *error_out = admin_err_t{
                                    "The key `database` does not match the primary key.",
                                    query_state_t::FAILED};
                                return false;
                            }
                        }
                        break;
                }
            }
        }

        if (array_size > 2) {
            ql::datum_t table = new_value_inout->get_field("table", ql::NOTHROW);
            if (table.has()) {
                keys.erase("table");

                switch (m_identifier_format) {
                    case admin_identifier_format_t::name:
                        {
                            name_string_t table_name;
                            if (!convert_name_from_datum(
                                    table,
                                    "table name",
                                    &table_name,
                                    error_out)) {
                                return false;
                            }

                            boost::optional<table_basic_config_t> table_basic_config =
                                m_name_resolver.table_id_to_basic_config(
                                    table_id_primary, database_id_primary);
                            if (!static_cast<bool>(table_basic_config) ||
                                    table_basic_config->name != table_name) {
                                *error_out = admin_err_t{
                                    "The key `table` does not match the primary key.",
                                    query_state_t::FAILED};
                                return false;
                            }
                        }
                        break;
                    case admin_identifier_format_t::uuid:
                        {
                            namespace_id_t table_id_secondary;
                            if (!convert_uuid_from_datum(
                                    table, &table_id_secondary, error_out)) {
                                return false;
                            }

                            if (table_id_primary != table_id_secondary) {
                                *error_out = admin_err_t{
                                    "The key `table` does not match the primary key.",
                                    query_state_t::FAILED};
                                return false;
                            }
                        }
                        break;
                }
            }
        }

        bool is_indeterminate = false;

        ql::datum_t permissions = new_value_inout->get_field("permissions", ql::NOTHROW);
        if (permissions.has()) {
            keys.erase("permissions");

            try {
                switch (array_size) {
                    case 1:
                        user->second.apply_write(
                            [&](boost::optional<auth::user_t> *inner_user) {
                                auto &permissions_ref =
                                    inner_user->get().get_global_permissions();

                                permissions_ref.merge(permissions);
                                is_indeterminate = permissions_ref.is_indeterminate();
                            });
                        break;
                    case 2:
                        user->second.apply_write(
                            [&](boost::optional<auth::user_t> *inner_user) {
                                auto &permissions_ref =
                                    inner_user->get().get_database_permissions(
                                        database_id_primary);

                                permissions_ref.merge(permissions);
                                is_indeterminate = permissions_ref.is_indeterminate();
                            });
                        break;
                    case 3:
                        user->second.apply_write(
                            [&](boost::optional<auth::user_t> *inner_user) {
                                auto &permissions_ref =
                                    inner_user->get().get_table_permissions(
                                        table_id_primary);

                                permissions_ref.merge(permissions);
                                is_indeterminate = permissions_ref.is_indeterminate();
                            });
                        break;
                }
            } catch (admin_op_exc_t const &admin_op_exc) {
                *error_out = admin_op_exc.to_admin_err();
                return false;
            }
        } else {
            *error_out = admin_err_t{
                "Expected a field `permissions`.", query_state_t::FAILED};
            return false;
        }

        if (!keys.empty()) {
            *error_out = admin_err_t{
                "Unexpected key(s) `" + boost::algorithm::join(keys, "`, `") + "`.",
                query_state_t::FAILED};
            return false;
        }

        // Updating the permissions to indeterminate is considered equal to a deletion
        if (is_indeterminate) {
            *new_value_inout = ql::datum_t();
        }
    } else {
        switch (array_size) {
            case 1:
                user->second.apply_write([](boost::optional<auth::user_t> *inner_user) {
                    inner_user->get().set_global_permissions(
                        permissions_t(
                            boost::indeterminate,
                            boost::indeterminate,
                            boost::indeterminate,
                            boost::indeterminate));
                });
                break;
            case 2:
                user->second.apply_write([&](boost::optional<auth::user_t> *inner_user) {
                    inner_user->get().set_database_permissions(
                        database_id_primary,
                        permissions_t(
                            boost::indeterminate,
                            boost::indeterminate,
                            boost::indeterminate));
                });
                break;
            case 3:
                user->second.apply_write([&](boost::optional<auth::user_t> *inner_user) {
                    inner_user->get().set_table_permissions(
                        table_id_primary,
                        permissions_t(
                            boost::indeterminate,
                            boost::indeterminate,
                            boost::indeterminate));
                });
                break;
        }
    }

    m_auth_semilattice_view->join(auth_metadata);

    return true;
}

uint8_t permissions_artificial_table_backend_t::parse_primary_key(
        ql::datum_t const &primary_key,
        cluster_semilattice_metadata_t const &cluster_metadata,
        username_t *username_out,
        database_id_t *database_id_out,
        namespace_id_t *table_id_out,
        admin_err_t *admin_err_out) {
    if (primary_key.get_type() != ql::datum_t::R_ARRAY ||
            primary_key.arr_size() < 1 ||
            primary_key.arr_size() > 3) {
        if (admin_err_out != nullptr) {
            *admin_err_out = admin_err_t{
                "Expected an array of one to three items in the primary key, got " +
                    primary_key.print() + ".",
                query_state_t::FAILED};
        }
        return 0;
    }

    ql::datum_t username = primary_key.get(0, ql::NOTHROW);
    if (username.get_type() != ql::datum_t::R_STR) {
        if (admin_err_out != nullptr) {
            *admin_err_out = admin_err_t{
                "Expected a string as the username, got " + username.print() + ".",
                query_state_t::FAILED};
        }
        return 0;
    }
    *username_out = username_t(username.as_str().to_std());

    if (primary_key.arr_size() > 1) {
        ql::datum_t database = primary_key.get(1, ql::NOTHROW);
        if (database.get_type() != ql::datum_t::R_STR ||
                !str_to_uuid(database.as_str().to_std(), database_id_out)) {
            if (admin_err_out != nullptr) {
                *admin_err_out = admin_err_t{
                    "Expected a UUID as the database, got " + database.print() + ".",
                    query_state_t::FAILED};
            }
            return 0;
        }

        boost::optional<name_string_t> database_name =
            m_name_resolver.database_id_to_name(*database_id_out, cluster_metadata);
        if (!static_cast<bool>(database_name)) {
            if (admin_err_out != nullptr) {
                *admin_err_out = admin_err_t{
                    strprintf(
                        "No database with UUID `%s` exists.",
                        uuid_to_str(*database_id_out).c_str()),
                    query_state_t::FAILED};
            }
            return 0;
        }
    }

    if (primary_key.arr_size() > 2) {
        ql::datum_t table = primary_key.get(2, ql::NOTHROW);
        if (table.get_type() != ql::datum_t::R_STR ||
                !str_to_uuid(table.as_str().to_std(), table_id_out)) {
            if (admin_err_out != nullptr) {
                *admin_err_out = admin_err_t{
                    "Expected a UUID as the table, got " + table.print() + ".",
                    query_state_t::FAILED};
            }
            return 0;
        }

        boost::optional<table_basic_config_t> table_basic_config =
            m_name_resolver.table_id_to_basic_config(*table_id_out);
        if (!static_cast<bool>(table_basic_config)) {
            if (admin_err_out != nullptr) {
                *admin_err_out = admin_err_t{
                    strprintf(
                        "No table with UUID `%s` exists.",
                        uuid_to_str(*table_id_out).c_str()),
                    query_state_t::FAILED};
            }
            return 0;
        }

        if (table_basic_config->database != *database_id_out) {
            if (admin_err_out != nullptr) {
                *admin_err_out = admin_err_t{
                    strprintf(
                        "No table with UUID `%s` exists.",
                        uuid_to_str(*table_id_out).c_str()),
                    query_state_t::FAILED};
            }
            return 0;
        }
    }

    return primary_key.arr_size();
}

bool permissions_artificial_table_backend_t::global_to_datum(
        username_t const &username,
        permissions_t const &permissions,
        ql::datum_t *datum_out) {
    ql::datum_t permissions_datum = permissions.to_datum();
    if (permissions_datum.get_type() != ql::datum_t::R_NULL) {
        ql::datum_object_builder_t builder;

        ql::datum_array_builder_t id_builder(ql::configured_limits_t::unlimited);
        id_builder.add(convert_string_to_datum(username.to_string()));

        builder.overwrite("id", std::move(id_builder).to_datum());
        builder.overwrite("permissions", std::move(permissions_datum));
        builder.overwrite("user", convert_string_to_datum(username.to_string()));

        *datum_out = std::move(builder).to_datum();
        return true;
    }

    return false;
}

bool permissions_artificial_table_backend_t::database_to_datum(
        username_t const &username,
        database_id_t const &database_id,
        permissions_t const &permissions,
        cluster_semilattice_metadata_t const &cluster_metadata,
        ql::datum_t *datum_out) {
    ql::datum_t permissions_datum = permissions.to_datum();
    if (permissions_datum.get_type() != ql::datum_t::R_NULL) {
        ql::datum_object_builder_t builder;

        ql::datum_array_builder_t id_builder(ql::configured_limits_t::unlimited);
        id_builder.add(convert_string_to_datum(username.to_string()));
        id_builder.add(convert_uuid_to_datum(database_id));

        ql::datum_t database_name_or_uuid;
        switch (m_identifier_format) {
            case admin_identifier_format_t::name:
                {
                    boost::optional<name_string_t> database_name =
                        m_name_resolver.database_id_to_name(
                            database_id, cluster_metadata);
                    database_name_or_uuid = ql::datum_t(database_name.get_value_or(
                        name_string_t::guarantee_valid("__deleted_database__")).str());
                }
                break;
            case admin_identifier_format_t::uuid:
                database_name_or_uuid = ql::datum_t(uuid_to_str(database_id));
                break;
        }

        builder.overwrite("database", std::move(database_name_or_uuid));
        builder.overwrite("id", std::move(id_builder).to_datum());
        builder.overwrite("permissions", std::move(permissions_datum));
        builder.overwrite("user", convert_string_to_datum(username.to_string()));

        *datum_out = std::move(builder).to_datum();
        return true;
    }

    return false;
}

bool permissions_artificial_table_backend_t::table_to_datum(
        username_t const &username,
        boost::optional<database_id_t> const &database_id,
        namespace_id_t const &table_id,
        permissions_t const &permissions,
        cluster_semilattice_metadata_t const &cluster_metadata,
        ql::datum_t *datum_out) {
    boost::optional<table_basic_config_t> table_basic_config =
        m_name_resolver.table_id_to_basic_config(table_id, database_id);
    if (!static_cast<bool>(table_basic_config)) {
        return false;
    }

    ql::datum_t permissions_datum = permissions.to_datum();
    if (permissions_datum.get_type() != ql::datum_t::R_NULL) {
        ql::datum_object_builder_t builder;

        ql::datum_array_builder_t id_builder(ql::configured_limits_t::unlimited);
        id_builder.add(convert_string_to_datum(username.to_string()));
        id_builder.add(convert_uuid_to_datum(table_basic_config->database));
        id_builder.add(convert_uuid_to_datum(table_id));

        ql::datum_t database_name_or_uuid;
        ql::datum_t table_name_or_uuid;
        switch (m_identifier_format) {
            case admin_identifier_format_t::name:
                {
                    boost::optional<name_string_t> database_name =
                        m_name_resolver.database_id_to_name(
                            table_basic_config->database, cluster_metadata);
                    database_name_or_uuid = ql::datum_t(database_name.get_value_or(
                        name_string_t::guarantee_valid("__deleted_database__")).str());
                    table_name_or_uuid = ql::datum_t(table_basic_config->name.str());
                }
                break;
            case admin_identifier_format_t::uuid:
                database_name_or_uuid =
                    ql::datum_t(uuid_to_str(table_basic_config->database));
                table_name_or_uuid = ql::datum_t(uuid_to_str(table_id));
                break;
        }

        builder.overwrite("database", std::move(database_name_or_uuid));
        builder.overwrite("id", std::move(id_builder).to_datum());
        builder.overwrite("permissions", std::move(permissions_datum));
        builder.overwrite("table", std::move(table_name_or_uuid));
        builder.overwrite("user", convert_string_to_datum(username.to_string()));

        *datum_out = std::move(builder).to_datum();
        return true;
    }

    return false;
}

}  // namespace auth
