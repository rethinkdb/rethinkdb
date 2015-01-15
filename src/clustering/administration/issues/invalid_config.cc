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

const datum_string_t write_acks_issue_t::write_acks_issue_type =
    datum_string_t("write_acks");
const uuid_u write_acks_issue_t::base_issue_id =
    str_to_uuid("086417d4-ce13-4d38-84c7-52871f7f1bd7");

invalid_config_issue_t::invalid_config_issue_t(
        const issue_id_t &issue_id, const namespace_id_t &_table_id)
    : issue_t(issue_id), table_id(_table_id) { }

bool invalid_config_issue_t::build_info_and_description(
        const metadata_t &metadata,
        UNUSED server_config_client_t *server_config_client,
        table_meta_client_t *table_meta_client,
        admin_identifier_format_t identifier_format,
        ql::datum_t *info_out,
        datum_string_t *description_out) const {
    ql::datum_t table_name_or_uuid;
    name_string_t table_name;
    ql::datum_t db_name_or_uuid;
    name_string_t db_name;
    if (!convert_table_id_to_datums(table_id, identifier_format, metadata,
            table_meta_client, &table_name_or_uuid, &table_name, &db_name_or_uuid,
            &db_name)) {
        /* There's no point in showing an issue for a deleted table */
        return false;
    }
    ql::datum_object_builder_t builder;
    builder.overwrite("table", table_name_or_uuid);
    builder.overwrite("db", db_name_or_uuid);
    *info_out = std::move(builder).to_datum();
    *description_out = build_description(table_name, db_name);
    return true;
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

write_acks_issue_t::write_acks_issue_t(const namespace_id_t &_table_id) :
    invalid_config_issue_t(from_hash(base_issue_id, _table_id), _table_id) { }

datum_string_t write_acks_issue_t::build_description(
        const name_string_t &table_name, const name_string_t &db_name) const {
    return datum_string_t(strprintf("One or more servers that were acting as replicas "
        "for table `%s.%s` have been permanently removed from the cluster, and as a "
        "result the write ack settings you specified in `rethinkdb.table_config` are no "
        "longer satisfiable. This usually happens because different shards have "
        "different numbers of replicas; the 'majority' write ack setting applies the "
        "same threshold to every shard, but it computes the threshold based on the "
        "shard with the most replicas. Fix this by changing the replica assignments in "
        "`rethinkdb.table_config` or by changing the write ack setting to 'single'.",
        db_name.c_str(), table_name.c_str()));
}

invalid_config_issue_tracker_t::invalid_config_issue_tracker_t(
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
            _cluster_sl_view) :
    cluster_sl_view(_cluster_sl_view) { }

invalid_config_issue_tracker_t::~invalid_config_issue_tracker_t() { }

std::vector<scoped_ptr_t<issue_t> > invalid_config_issue_tracker_t::get_issues() const {
    std::vector<scoped_ptr_t<issue_t> > issues;
    // RSI(raft): Check for invalid config issues
    return issues;
}

