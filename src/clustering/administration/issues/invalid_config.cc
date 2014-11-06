// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/issues/invalid_config.hpp"
#include "clustering/administration/datum_adapter.hpp"

const datum_string_t need_primary_issue_t::need_primary_issue_type =
    datum_string_t("table_needs_primary");
const uuid_u need_primary_issue_t::base_issue_id =
    str_to_uuid("da32f2a3-9ef7-4c68-a73a-11e6b15e2bdd");

const datum_string_t data_loss_issue_t::data_loss_issue_type =
    datum_string_t("data_lost");
const uuid_u data_loss_issue_t::base_issue_id =
    str_to_uuid("12ad8005-bdc0-4f31-aa43-b0b7ca6cc4b0");

invalid_config_issue_t::invalid_config_issue_t(
        const issue_id_t &issue_id, const namespace_id_t &_table_id)
    : issue_t(issue_id), table_id(_table_id) { }

ql::datum_t invalid_config_issue_t::build_info(
        const metadata_t &metadata) const {
    ql::datum_object_builder_t builder;
    auto it = metadata.rdb_namespaces->namespaces.find(table_id);
    if (it->second.is_deleted()) {
        /* There's no point in showing an issue for a deleted table */
        return ql::datum_t();
    }
    name_string_t table_name = it->second.get_ref().name.get_ref();
    database_id_t db_id = it->second.get_ref().database.get_ref();
    name_string_t db_name;
    auto jt = metadata.databases.databases.find(db_id);
    if (jt->second.is_deleted()) {
        db_name = name_string_t::guarantee_valid("__deleted_database__");
    } else {
        db_name = jt->second.get_ref().name.get_ref();
    }
    builder.overwrite("table", convert_name_to_datum(table_name));
    builder.overwrite("table_id", convert_uuid_to_datum(table_id));
    builder.overwrite("db", convert_name_to_datum(db_name));
    builder.overwrite("db_id", convert_uuid_to_datum(db_id));
    return std::move(builder).to_datum();
}

datum_string_t invalid_config_issue_t::build_description(const ql::datum_t &info) const {
    name_string_t table_name, db_name;
    std::string dummy_error;
    bool ok = convert_name_from_datum(
        info.get_field("table"), "", &table_name, &dummy_error);
    guarantee(ok);
    ok = convert_name_from_datum(info.get_field("db"), "", &db_name, &dummy_error);
    guarantee(ok);
    return build_description(table_name, db_name);
}

need_primary_issue_t::need_primary_issue_t(const namespace_id_t &_table_id) :
    invalid_config_issue_t(from_hash(base_issue_id, _table_id), _table_id) { }

datum_string_t need_primary_issue_t::build_description(
        const name_string_t &table_name, const name_string_t &db_name) const {
    return datum_string_t(strprintf("A server that was acting as a primary replica for "
        "table `%s.%s` has been permanently removed from the cluster. One or more "
        "shards now have no primary replica. Please assign primary replicas by writing "
        "to `rethinkdb.table_config` or generate a new configuration with "
        "`reconfigure()`.", db_name.c_str(), table_name.c_str()));
}

data_loss_issue_t::data_loss_issue_t(const namespace_id_t &_table_id) :
    invalid_config_issue_t(from_hash(base_issue_id, _table_id), _table_id) { }

datum_string_t data_loss_issue_t::build_description(
        const name_string_t &table_name, const name_string_t &db_name) const {
    return datum_string_t(strprintf("One or more servers that were acting as replicas "
        "for table `%s.%s` have been permanently removed from the cluster, and those "
        "replicas were holding all of the copies of some of the data for the table. "
        "Some data has probably been lost permanently. You can assign new replicas by "
        "writing to `rethinkdb.table_config` or generate a new configuration with "
        "`reconfigure()`; the parts of the table that were lost will become available "
        "for writes again, but they will be empty.", db_name.c_str(),
        table_name.c_str()));
}

invalid_config_issue_tracker_t::invalid_config_issue_tracker_t(
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
            _cluster_sl_view) :
    cluster_sl_view(_cluster_sl_view) { }

invalid_config_issue_tracker_t::~invalid_config_issue_tracker_t() { }

std::vector<scoped_ptr_t<issue_t> > invalid_config_issue_tracker_t::get_issues() const {
    std::vector<scoped_ptr_t<issue_t> > issues;
    cluster_semilattice_metadata_t md = cluster_sl_view->get();
    const_metadata_searcher_t<namespace_semilattice_metadata_t> searcher(
        &md.rdb_namespaces->namespaces);
    for (auto it = searcher.find_next(searcher.begin()); it != searcher.end();
            it = searcher.find_next(++it)) {
        const table_config_t &config =
            it->second.get_ref().replication_info.get_ref().config;
        bool need_primary = false, data_loss = false;
        for (const table_config_t::shard_t &shard : config.shards) {
            if (md.servers.servers.at(shard.director).is_deleted()) {
                need_primary = true;
            }
            size_t num_dead = 0;
            for (const server_id_t &sid : shard.replicas) {
                if (md.servers.servers.at(sid).is_deleted()) {
                    ++num_dead;
                }
            }
            if (num_dead == shard.replicas.size()) {
                data_loss = true;
            }
        }
        if (data_loss) {
            issues.push_back(make_scoped<data_loss_issue_t>(it->first));
        } else if (need_primary) {
            issues.push_back(make_scoped<need_primary_issue_t>(it->first));
        }
    }
    return issues;
}

