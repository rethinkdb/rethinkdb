// Copyright 2010-2016 RethinkDB, all rights reserved.
#include "clustering/administration/tables/name_resolver.hpp"

#include "clustering/administration/artificial_reql_cluster_interface.hpp"

name_resolver_t::name_resolver_t(
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t>>
            cluster_semilattice_view,
        table_meta_client_t *table_meta_client,
        lifetime_t<artificial_reql_cluster_interface_t const &>
            artificial_reql_cluster_interface)
    : m_cluster_semilattice_view(cluster_semilattice_view),
      m_table_meta_client(table_meta_client),
      m_artificial_reql_cluster_interface(artificial_reql_cluster_interface) {
}

cluster_semilattice_metadata_t name_resolver_t::get_cluster_metadata() const noexcept {
    return m_cluster_semilattice_view->get();
}

boost::optional<name_string_t> name_resolver_t::database_id_to_name(
        database_id_t const &database_id) const noexcept {
    return database_id_to_name(database_id, get_cluster_metadata());
}

boost::optional<name_string_t> name_resolver_t::database_id_to_name(
        database_id_t const &database_id,
        cluster_semilattice_metadata_t const &cluster_metadata) const noexcept {
    if (artificial_reql_cluster_interface_t::database_id == database_id) {
        return artificial_reql_cluster_interface_t::database_name;
    }

    auto iterator = cluster_metadata.databases.databases.find(database_id);
    if (iterator != cluster_metadata.databases.databases.end() &&
            !iterator->second.is_deleted()) {
        return iterator->second.get_ref().name.get_ref();
    }

    return boost::none;
}

boost::optional<table_basic_config_t> name_resolver_t::table_id_to_basic_config(
        namespace_id_t const &table_id,
        boost::optional<database_id_t> const &database_id) const noexcept {
    if (!static_cast<bool>(database_id) ||
            artificial_reql_cluster_interface_t::database_id == database_id.get()) {
        for (auto const &table_backend :
                m_artificial_reql_cluster_interface.get_table_backends_map()) {
            if (table_backend.second.first != nullptr &&
                    table_backend.second.first->get_table_id() == table_id) {
                return table_basic_config_t{
                        table_backend.first,
                        artificial_reql_cluster_interface_t::database_id,
                        table_backend.second.first->get_primary_key_name()
                    };
            }
        }
    }

    if (m_table_meta_client != nullptr) {
        table_basic_config_t table_basic_config;
        try {
            m_table_meta_client->get_name(table_id, &table_basic_config);
        } catch (no_such_table_exc_t const &) {
            return boost::none;
        }

        if (static_cast<bool>(database_id) &&
                table_basic_config.database != database_id.get()) {
            return boost::none;
        }

        return table_basic_config;
    } else {
        return boost::none;
    }
}

resolved_id_optional_t<database_id_t> name_resolver_t::database_name_to_id(
        name_string_t const &database_name) const noexcept {
    return database_name_to_id(database_name, get_cluster_metadata());
}

resolved_id_optional_t<database_id_t> name_resolver_t::database_name_to_id(
        name_string_t const &database_name,
        cluster_semilattice_metadata_t const &cluster_metadata) const noexcept {
    if (artificial_reql_cluster_interface_t::database_name == database_name) {
        return resolved_id_optional_t<database_id_t>(
            artificial_reql_cluster_interface_t::database_id);
    }

    database_id_t database_id;
    size_t count = 0;
    for (auto const &database : cluster_metadata.databases.databases) {
        if (!database.second.is_deleted() &&
                database.second.get_ref().name.get_ref() == database_name) {
            database_id = database.first;
            ++count;
        }
    }

    switch (count) {
        case 0:
            return resolved_id_optional_t<database_id_t>(
                resolved_id_optional_t<database_id_t>::no_such_name_t());
        case 1:
            return resolved_id_optional_t<database_id_t>(database_id);
        default:
            return resolved_id_optional_t<database_id_t>(
                resolved_id_optional_t<database_id_t>::ambiguous_name_t());
    }
}

resolved_id_optional_t<namespace_id_t> name_resolver_t::table_name_to_id(
        database_id_t const &database_id,
        name_string_t const &table_name) const noexcept {
    if (artificial_reql_cluster_interface_t::database_id == database_id) {
        auto const &table_backends_map =
            m_artificial_reql_cluster_interface.get_table_backends_map();
        auto iterator = table_backends_map.find(table_name);
        if (iterator != table_backends_map.end() && iterator->second.first != nullptr) {
            return resolved_id_optional_t<namespace_id_t>(
                iterator->second.first->get_table_id());
        } else {
            return resolved_id_optional_t<namespace_id_t>(
                resolved_id_optional_t<database_id_t>::no_such_name_t());
        }
    }

    if (m_table_meta_client != nullptr) {
        namespace_id_t table_id;
        try {
            m_table_meta_client->find(database_id, table_name, &table_id);
        } catch (no_such_table_exc_t const &) {
            return resolved_id_optional_t<namespace_id_t>(
                resolved_id_optional_t<database_id_t>::no_such_name_t());
        } catch (ambiguous_table_exc_t const &) {
            return resolved_id_optional_t<namespace_id_t>(
                resolved_id_optional_t<database_id_t>::ambiguous_name_t());
        }
        return resolved_id_optional_t<database_id_t>(table_id);
    } else {
        return resolved_id_optional_t<namespace_id_t>(
            resolved_id_optional_t<database_id_t>::no_such_name_t());
    }
}
