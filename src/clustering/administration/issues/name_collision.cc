// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/issues/name_collision.hpp"
#include "clustering/administration/datum_adapter.hpp"
#include "rdb_protocol/configured_limits.hpp"

const datum_string_t server_name_collision_issue_t::server_name_collision_issue_type =
    datum_string_t("server_name_collision");
const uuid_u server_name_collision_issue_t::base_issue_id =
    str_to_uuid("a8bf71c6-a178-43ea-b25d-73a13619c8f0");

const datum_string_t db_name_collision_issue_t::db_name_collision_issue_type =
    datum_string_t("db_name_collision");
const uuid_u db_name_collision_issue_t::base_issue_id =
    str_to_uuid("a6996d3c-418d-4ab1-85b5-455dcd8721d8");

const datum_string_t table_name_collision_issue_t::table_name_collision_issue_type =
    datum_string_t("table_name_collision");
const uuid_u table_name_collision_issue_t::base_issue_id =
    str_to_uuid("d610286a-a374-49e6-8d92-032794ed578f");

name_collision_issue_t::name_collision_issue_t(const issue_id_t &_issue_id,
                                               const name_string_t &_name,
                                               const std::vector<uuid_u> &_collided_ids) :
    issue_t(_issue_id),
    name(_name),
    collided_ids(_collided_ids) { }

void generic_build_info_and_description(
        const std::string &type,
        const name_string_t &name,
        const std::vector<uuid_u> &ids,
        ql::datum_t *info_out,
        datum_string_t *description_out) {
    ql::datum_object_builder_t builder;
    ql::datum_array_builder_t ids_builder(ql::configured_limits_t::unlimited);
    std::string ids_str;
    for (auto const &id : ids) {
        ids_builder.add(convert_uuid_to_datum(id));
        if (!ids_str.empty()) {
            ids_str += ", ";
        }
        ids_str += uuid_to_str(id);
    }
    builder.overwrite("name", convert_name_to_datum(name));
    builder.overwrite("ids", std::move(ids_builder).to_datum());
    *info_out = std::move(builder).to_datum();
    *description_out = datum_string_t(strprintf(
        "The following %s are all named '%s': %s.",
        type.c_str(), name.c_str(), ids_str.c_str()));
}

server_name_collision_issue_t::server_name_collision_issue_t(
        const name_string_t &_name,
        const std::vector<server_id_t> &_collided_ids) :
    name_collision_issue_t(from_hash(base_issue_id, _name),
                          _name,
                          _collided_ids) { }

bool server_name_collision_issue_t::build_info_and_description(
        UNUSED const metadata_t &metadata,
        UNUSED server_config_client_t *server_config_client,
        UNUSED admin_identifier_format_t identifier_format,
        ql::datum_t *info_out,
        datum_string_t *description_out) const {
    generic_build_info_and_description(
        "servers", name, collided_ids, info_out, description_out);
    return true;
}

db_name_collision_issue_t::db_name_collision_issue_t(
        const name_string_t &_name,
        const std::vector<database_id_t> &_collided_ids) :
    name_collision_issue_t(from_hash(base_issue_id, _name),
                          _name,
                          _collided_ids) { }

bool db_name_collision_issue_t::build_info_and_description(
        UNUSED const metadata_t &metadata,
        UNUSED server_config_client_t *server_config_client,
        UNUSED admin_identifier_format_t identifier_format,
        ql::datum_t *info_out,
        datum_string_t *description_out) const {
    generic_build_info_and_description(
        "databases", name, collided_ids, info_out, description_out);
    return true;
}

table_name_collision_issue_t::table_name_collision_issue_t(
        const name_string_t &_name,
        const database_id_t &_db_id,
        const std::vector<namespace_id_t> &_collided_ids) :
    name_collision_issue_t(from_hash(base_issue_id, _db_id, _name),
                           _name,
                           _collided_ids),
    db_id(_db_id) { }

bool table_name_collision_issue_t::build_info_and_description(
        const metadata_t &metadata,
        UNUSED server_config_client_t *server_config_client,
        admin_identifier_format_t identifier_format,
        ql::datum_t *info_out,
        datum_string_t *description_out) const {
    ql::datum_t db_name_or_uuid;
    name_string_t db_name;
    if (!convert_database_id_to_datum(db_id, identifier_format, metadata,
            &db_name_or_uuid, &db_name)) {
        db_name_or_uuid = ql::datum_t("__deleted_database__");
        db_name = name_string_t::guarantee_valid("__deleted_database__");
    }
    ql::datum_t partial_info;
    generic_build_info_and_description(
        strprintf("tables in database `%s`", db_name.c_str()), name, collided_ids,
        &partial_info, description_out);
    ql::datum_object_builder_t info_rebuilder(partial_info);
    info_rebuilder.overwrite("db", db_name_or_uuid);
    *info_out = std::move(info_rebuilder).to_datum();
    return true;
}

name_collision_issue_tracker_t::name_collision_issue_tracker_t(
            boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
                _cluster_sl_view) :
    cluster_sl_view(_cluster_sl_view) { }

name_collision_issue_tracker_t::~name_collision_issue_tracker_t() { }

template <typename issue_type, typename map_t>
void find_duplicates(const map_t &data,
                     std::vector<scoped_ptr_t<issue_t> > *issues_out) {
    std::map<name_string_t, std::vector<uuid_u> > name_counts;
    for (auto const &it : data) {
        if (!it.second.is_deleted()) {
            name_string_t name = it.second.get_ref().name.get_ref();
            name_counts[name].push_back(it.first);
        }
    }

    for (auto const &it : name_counts) {
        if (it.second.size() > 1) {
            issues_out->push_back(scoped_ptr_t<issue_t>(
                new issue_type(it.first, it.second)));
        }
    }
}

void find_table_duplicates(
        const namespaces_semilattice_metadata_t::namespace_map_t &data,
        std::vector<scoped_ptr_t<issue_t> > *issues_out) {
    std::map<std::pair<database_id_t, name_string_t>, std::vector<namespace_id_t> >
        name_counts;
    for (auto const &it : data) {
        if (!it.second.is_deleted()) {
            std::pair<database_id_t, name_string_t> info =
                std::make_pair(it.second.get_ref().database.get_ref(),
                               it.second.get_ref().name.get_ref());
            name_counts[info].push_back(it.first);
        }
    }

    for (auto const &it : name_counts) {
        if (it.second.size() > 1) {
            issues_out->push_back(scoped_ptr_t<issue_t>(
                new table_name_collision_issue_t(it.first.second, // Table name
                                                 it.first.first,  // Database
                                                 it.second)));    // IDs of tables
        }
    }
}

std::vector<scoped_ptr_t<issue_t> > name_collision_issue_tracker_t::get_issues() const {
    cluster_semilattice_metadata_t metadata = cluster_sl_view->get();
    std::vector<scoped_ptr_t<issue_t> > issues;

    find_duplicates<server_name_collision_issue_t>(metadata.servers.servers, &issues);
    find_duplicates<db_name_collision_issue_t>(metadata.databases.databases, &issues);
    find_table_duplicates(metadata.rdb_namespaces->namespaces, &issues);

    return issues;
}
