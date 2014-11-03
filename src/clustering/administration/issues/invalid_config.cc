// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/issues/invalid_config.hpp"
#include "clustering/administration/datum_adapter.hpp"

const datum_string_t need_primary_issue_t::need_primary_issue_type =
    datum_string_t("table_needs_primary");
const uuid_u need_primary_issue_t::base_issue_id =
    str_to_uuid("da32f2a3-9ef7-4c68-a73a-11e6b15e2bdd");

const datum_string_t data_loss_issue_t::data_loss_issue_type =
    datum_string_t("table_needs_primary");
const uuid_u need_primary_issue_t::base_issue_id =
    str_to_uuid("12ad8005-bdc0-4f31-aa43-b0b7ca6cc4b0");

invalid_config_issue_t::invalid_config_issue_t(
        const issue_id_t &issue_id, const namespace_id_t &_table_id)
    : issue_t(issue_id), table_id(_table_id) { }

ql::datum_t invalid_config_issue_t::build_info(
        const metadata_t &metadata) const {
    ql::datum_object_builder_t builder;
    name_string_t name;
    database_id_t db;
    auto it = metadata.rdb_namespaces->namespaces.find(table_id);
    if (it == metadata.rdb_namespaces->namespaces.end() && it->second.is_deleted()) {
        name = name_string_t::guarantee_valid("__deleted_table__");
    } else {
        name = it->second.get_ref().name.get_ref().str();
    }
    builder.overwrite("table", convert_name_to_datum(name));
    builder.overwrite("table_id", convert_uuid_to_datum(table_id));
    return std::move(builder).to_datum();
}

datum_string_t invalid_config_issue_t::build_description(const ql::datum_t &info) const {
    name_string_t name;
    bool ok = convert_name_from_datum(info.get_field("table"), &name);
    guarantee(ok);
    return build_description(name);
}

need_primary_issue_t::need_primary_issue_t(const namespace_id_t &_table_id) :
    invalid_config_issue_t(from_hash(base_issue_id, _table_id), _table_id) { }

datum_string_t need_primary_issue_t::build_description(
        const name_string_t &table_name) const {
    return strprintf("A server that was acting as a primary for table `%s.%s`
}

data_loss_issue_t::data_loss_issue_t(const namespace_id_t &_table_id) :
    invalid_config_issue_t(from_hash(base_issue_id, _table_id), _table_id) { }
