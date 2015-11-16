// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/issues/outdated_index.hpp"

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/datum_adapter.hpp"
#include "rdb_protocol/configured_limits.hpp"

const datum_string_t outdated_index_issue_t::outdated_index_issue_type =
    datum_string_t("outdated_index");
const uuid_u outdated_index_issue_t::base_issue_id =
    str_to_uuid("528f5239-096e-47a3-b263-7a06a256ecc3");

outdated_index_issue_t::outdated_index_issue_t() : issue_t(nil_uuid()) { }

outdated_index_issue_t::outdated_index_issue_t(
        outdated_index_issue_t::index_map_t _indexes) :
    issue_t(base_issue_id),
    indexes(std::move(_indexes)) { }

bool outdated_index_issue_t::build_info_and_description(
        const metadata_t &metadata,
        UNUSED server_config_client_t *server_config_client,
        table_meta_client_t *table_meta_client,
        admin_identifier_format_t identifier_format,
        ql::datum_t *info_out,
        datum_string_t *description_out) const {
    ql::datum_object_builder_t info_builder;
    ql::datum_array_builder_t tables_builder(ql::configured_limits_t::unlimited);
    std::string tables_str;
    for (auto const &pair : indexes) {
        ql::datum_t table_name_or_uuid;
        name_string_t table_name;
        ql::datum_t db_name_or_uuid;
        name_string_t db_name;
        if (!convert_table_id_to_datums(pair.first, identifier_format, metadata,
                table_meta_client, &table_name_or_uuid, &table_name, &db_name_or_uuid,
                &db_name)) {
            /* No point in showing an outdated_index issue for a deleted table. */
            continue;
        }
        ql::datum_array_builder_t indexes_builder(ql::configured_limits_t::unlimited);
        std::string indexes_str;
        for (auto const &index : pair.second) {
            indexes_builder.add(convert_string_to_datum(index));
            if (!indexes_str.empty()) {
                indexes_str += ", ";
            }
            indexes_str += "`" + index + "`";
        }
        ql::datum_object_builder_t table_builder;
        table_builder.overwrite("table", table_name_or_uuid);
        table_builder.overwrite("db", db_name_or_uuid);
        table_builder.overwrite("indexes", std::move(indexes_builder).to_datum());
        tables_builder.add(std::move(table_builder).to_datum());
        tables_str += strprintf("\nFor table `%s.%s`: %s",
            table_name.c_str(), db_name.c_str(), indexes_str.c_str());
    }
    info_builder.overwrite("tables", std::move(tables_builder).to_datum());
    *info_out = std::move(info_builder).to_datum();
    *description_out = datum_string_t(strprintf(
        "The cluster contains indexes that were created with a previous version of the "
        "query language which contained some bugs.  These should be remade to avoid "
        "relying on broken behavior.  See "
        "http://www.rethinkdb.com/docs/troubleshooting/#my-secondary-index-is-outdated "
        "for details.%s", tables_str.c_str()));
    /* If all the tables were deleted by the time we get here, don't show the user an
    issue */
    return !tables_str.empty();
}


outdated_index_issue_tracker_t::outdated_index_issue_tracker_t(
            table_meta_client_t *_table_meta_client) :
        table_meta_client(_table_meta_client) { }

void outdated_index_issue_tracker_t::log_outdated_indexes(
        multi_table_manager_t *multi_table_manager,
        const cluster_semilattice_metadata_t &metadata,
        signal_t *interruptor) {
    try {
        multi_table_manager->visit_tables(interruptor, access_t::read,
            [&] (const namespace_id_t &table_id,
                 multistore_ptr_t *,
                 table_manager_t *table_manager) {
                table_config_and_shards_t config =
                    table_manager->get_raft()->get_committed_state()->get().state.config;

                std::string indexes;
                for (auto const &sindex_pair : config.config.sindexes) {
                    if (sindex_pair.second.func_version != reql_version_t::LATEST) {
                        indexes += (indexes.empty() ? "'" : ", '") + sindex_pair.first + "'";
                    }
                }

                if (!indexes.empty()) {
                    name_string_t db_name;
                    if (!convert_database_id_to_datum(config.config.basic.database,
                                                      admin_identifier_format_t::name,
                                                      metadata,
                                                      nullptr,
                                                      &db_name)) {
                        db_name = name_string_t::guarantee_valid("__deleted_database__");
                    }

                    logWRN("Table %s.%s (%s) contains these outdated indexes which should be "
                           "recreated: %s",
                           db_name.c_str(),
                           config.config.basic.name.c_str(),
                           uuid_to_str(table_id).c_str(),
                           indexes.c_str());
                }
            });
    } catch (const interrupted_exc_t &) {
        // Do nothing
    }
}

std::vector<scoped_ptr_t<issue_t> > outdated_index_issue_tracker_t::get_issues(
        signal_t *interruptor) const {
    std::vector<scoped_ptr_t<issue_t> > res;
    outdated_index_issue_t::index_map_t index_map;

    std::map<namespace_id_t, table_config_and_shards_t> configs;
    std::map<namespace_id_t, table_basic_config_t> disconnected_configs;
    table_meta_client->list_configs(interruptor, &configs, &disconnected_configs);

    for (auto const &table_pair : configs) {
        std::set<std::string> indexes;
        for (auto const &sindex_pair : table_pair.second.config.sindexes) {
            if (sindex_pair.second.func_version != reql_version_t::LATEST) {
                indexes.insert(sindex_pair.first);
            }
        }

        if (!indexes.empty()) {
            index_map.insert(std::make_pair(table_pair.first, indexes));
        }
    }

    if (!index_map.empty()) {
        res.push_back(scoped_ptr_t<issue_t>(
            new outdated_index_issue_t(std::move(index_map))));
    }
    return res;
}
