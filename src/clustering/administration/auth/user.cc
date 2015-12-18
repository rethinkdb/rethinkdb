// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/user.hpp"

#include "arch/runtime/runtime_utils.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/tables/table_metadata.hpp"
#include "clustering/table_manager/table_meta_client.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/archive/versioned.hpp"

namespace auth {

user_t::user_t() {
}

user_t::user_t(admin_t)
    : m_password(boost::none),
      m_global_permissions(true, true, true, true) {
}

user_t::user_t(
        ql::datum_t const &datum,
        table_meta_client_t *table_meta_client,
        cluster_semilattice_metadata_t const &cluster_metadata,
        admin_identifier_format_t identifier_format) {
    ql::datum_t password = datum.get_field("password", ql::NOTHROW);
    if (!password.has()) {
        throw admin_op_exc_t(
            strprintf("Expected a field named `password`"), query_state_t::FAILED);
    }
    if (password.get_type() == ql::datum_t::R_STR) {
        set_password(std::move(password.as_str().to_std()));
    } else if (password.get_type() == ql::datum_t::R_BOOL) {
        if (!password.as_bool()) {
            set_password(boost::none);
        }
    } else {
        throw admin_op_exc_t(
            "Expected a string or boolean for `password`, got " + password.print(),
            query_state_t::FAILED);
    }

    ql::datum_t access = datum.get_field("access", ql::NOTHROW);
    if (!access.has()) {
        throw admin_op_exc_t(
            strprintf("Expected a field named `access`"), query_state_t::FAILED);
    }
    if (access.get_type() != ql::datum_t::R_OBJECT) {
        throw admin_op_exc_t(
            "Expected an object for `access`, got " + access.print(),
            query_state_t::FAILED);
    }

    set_global_permissions(access);

    for (size_t i = 0; i < access.obj_size(); ++i) {
        std::pair<datum_string_t, ql::datum_t> database = access.get_pair(i);

        std::string database_key = database.first.to_std();
        if (!database_key.empty() && database_key.front() == '$') {
            // This was either "$read", "$write", or "$config" which we skip
            continue;
        }

        database_id_t database_id;
        switch (identifier_format) {
            case admin_identifier_format_t::name:
                {
                    name_string_t database_name;
                    if (!database_name.assign_value(database_key)) {
                        throw admin_op_exc_t(
                            "`" + database_key + "` is not a valid database name",
                            query_state_t::FAILED);
                    }
                    admin_err_t admin_err;
                    if (!search_db_metadata_by_name(
                            cluster_metadata.databases,
                            database_name,
                            &database_id,
                            &admin_err)) {
                        throw admin_op_exc_t(admin_err);
                    }
                }
                break;
            case admin_identifier_format_t::uuid:
                if (!str_to_uuid(database_key, &database_id)) {
                    throw admin_op_exc_t(
                        "Expected a UUID; got " + datum.print(), query_state_t::FAILED);
                }
                auto iter = cluster_metadata.databases.databases.find(database_id);
                if (iter == cluster_metadata.databases.databases.end() ||
                        iter->second.is_deleted()) {
                    throw admin_op_exc_t(
                        strprintf(
                            "There is no database with UUID `%s`", database_key.c_str()),
                        query_state_t::FAILED);
                }
                break;
        }

        if (database.second.get_type() != ql::datum_t::R_OBJECT) {
            throw admin_op_exc_t(
                "Expected an object, got " + database.second.print(),
                query_state_t::FAILED);
        }

        set_database_permissions(database_id, database.second);

        for (size_t j = 0; j < database.second.obj_size(); ++j) {
            std::pair<datum_string_t, ql::datum_t> table = database.second.get_pair(j);

            std::string table_key = table.first.to_std();
            if (!table_key.empty() && table_key.front() == '$') {
                // This was either "$read", "$write", or "$config" which we skip
                continue;
            }

            namespace_id_t table_id;
            switch (identifier_format) {
                case admin_identifier_format_t::name:
                    {
                        name_string_t table_name;
                        if (!table_name.assign_value(table_key)) {
                            throw admin_op_exc_t(
                                "`" + table_key + "` is not a valid table name",
                                query_state_t::FAILED);
                        }
                        try {
                            table_meta_client->find(
                                database_id,
                                table_name,
                                &table_id,
                                nullptr);
                        } catch (const no_such_table_exc_t &) {
                            throw admin_op_exc_t(
                                strprintf(
                                    "Table `%s.%s` does not exist",
                                    database_key.c_str(),
                                    table_key.c_str()),
                                query_state_t::FAILED);
                        } catch (const ambiguous_table_exc_t &) {
                            throw admin_op_exc_t(
                                strprintf(
                                    "Table `%s.%s` is ambiguous; "
                                    "there are multiple tables with that name",
                                    database_key.c_str(),
                                    table_key.c_str()),
                                query_state_t::FAILED);
                        }
                    }
                    break;
                case admin_identifier_format_t::uuid:
                    if (!str_to_uuid(table_key, &table_id)) {
                        throw admin_op_exc_t(
                            "Expected a UUID; got " + datum.print(),
                            query_state_t::FAILED);
                    }
                    if (!table_meta_client->exists(table_id)) {
                        throw admin_op_exc_t(
                            strprintf(
                                "There is no table with UUID `%s`", table_key.c_str()),
                            query_state_t::FAILED);
                    }
                    break;
            }

            if (table.second.get_type() != ql::datum_t::R_OBJECT) {
                throw admin_op_exc_t(
                    "Expected an object, got " + table.second.print(),
                    query_state_t::FAILED);
            }

            set_table_permissions(table_id, table.second);
        }
    }
}

bool user_t::has_password() const {
    return static_cast<bool>(m_password);
}

boost::optional<password_t> const & user_t::get_password() const {
    return m_password;
}

void user_t::set_password(boost::optional<std::string> password) {
    if (static_cast<bool>(password)) {
        m_password = std::move(password.get());
    } else {
        m_password = boost::none;
    }
}

global_permissions_t user_t::get_global_permissions() const {
    return m_global_permissions;
}

void user_t::set_global_permissions(global_permissions_t permissions) {
    m_global_permissions = std::move(permissions);
}

permissions_t user_t::get_database_permissions(database_id_t const &database_id) const {
    auto iterator = m_database_permissions.find(database_id);
    if (iterator != m_database_permissions.end()) {
        return iterator->second;
    } else {
        return permissions_t(
            boost::indeterminate, boost::indeterminate, boost::indeterminate);
    }
}

void user_t::set_database_permissions(
        database_id_t const &database_id,
        permissions_t permissions) {
    m_database_permissions[database_id] = std::move(permissions);
}

permissions_t user_t::get_table_permissions(namespace_id_t const &table_id) const {
    auto iterator = m_table_permissions.find(table_id);
    if (iterator != m_table_permissions.end()) {
        return iterator->second;
    } else {
        return permissions_t(
            boost::indeterminate, boost::indeterminate, boost::indeterminate);
    }
}

void user_t::set_table_permissions(
        namespace_id_t const &table_id,
        permissions_t permissions) {
    m_table_permissions[table_id] = std::move(permissions);
}

bool user_t::has_connect_permission() const {
    return m_global_permissions.get_connect() || false;
}

ql::datum_t user_t::to_datum(
        std::map<namespace_id_t, table_basic_config_t> const &names,
        cluster_semilattice_metadata_t const &cluster_metadata,
        username_t const &username,
        admin_identifier_format_t identifier_format) const {
    ql::datum_object_builder_t builder;
    builder.overwrite("id", ql::datum_t(datum_string_t(username.to_string())));
    builder.overwrite("password", ql::datum_t::boolean(has_password()));

    ql::datum_object_builder_t access_builder;
    get_global_permissions().to_datum(&access_builder);

    for (auto const &database : cluster_metadata.databases.databases) {
        ql::datum_object_builder_t database_builder;
        get_database_permissions(database.first).to_datum(&database_builder);

        for (auto const &table : names) {
            if (table.second.database != database.first) {
                continue;
            }

            ql::datum_object_builder_t table_builder;
            get_table_permissions(table.first).to_datum(&table_builder);

            if (!table_builder.empty()) {
                switch (identifier_format) {
                    case admin_identifier_format_t::name:
                        database_builder.overwrite(
                            datum_string_t(table.second.name.str()),
                            std::move(table_builder).to_datum());
                        break;
                    case admin_identifier_format_t::uuid:
                        database_builder.overwrite(
                            datum_string_t(uuid_to_str(table.first)),
                            std::move(table_builder).to_datum());
                        break;
                }
            }
        }

        if (!database_builder.empty()) {
            switch (identifier_format) {
                case admin_identifier_format_t::name:
                    {
                        std::string database_name = "__deleted_database__";
                        if (!database.second.is_deleted()) {
                            database_name =
                                database.second.get_ref().name.get_ref().str();
                        }
                        access_builder.overwrite(
                            datum_string_t(database_name),
                            std::move(database_builder).to_datum());
                    }
                    break;
                case admin_identifier_format_t::uuid:
                    access_builder.overwrite(
                        datum_string_t(uuid_to_str(database.first)),
                        std::move(database_builder).to_datum());
                    break;
            }
        }
    }

    builder.overwrite("access", std::move(access_builder).to_datum());

    return std::move(builder).to_datum();
}

bool user_t::operator==(user_t const &rhs) const {
    return
        m_password == rhs.m_password &&
        m_global_permissions == rhs.m_global_permissions &&
        m_database_permissions == rhs.m_database_permissions &&
        m_table_permissions == rhs.m_table_permissions;
}

RDB_IMPL_SERIALIZABLE_4(
    user_t
  , m_password
  , m_global_permissions
  , m_database_permissions
  , m_table_permissions);
INSTANTIATE_SERIALIZABLE_SINCE_v1_13(user_t);

}  // namespace auth
