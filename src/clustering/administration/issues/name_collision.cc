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

ql::datum_t build_server_db_info(const name_string_t &name,
                                 const std::vector<uuid_u> &ids) {
    ql::datum_object_builder_t builder;
    ql::datum_array_builder_t ids_builder(ql::configured_limits_t::unlimited);
    for (auto const &id : ids) {
        ids_builder.add(convert_uuid_to_datum(id));
    }
    builder.overwrite("name", convert_name_to_datum(name));
    builder.overwrite("ids", std::move(ids_builder).to_datum());
    return std::move(builder).to_datum();
}

datum_string_t build_server_db_description(const std::string &type,
                                           const name_string_t &name,
                                           const std::vector<machine_id_t> &ids) {
    std::string ids_str;
    for (auto it = ids.begin(); it != ids.end(); ++it) {
        ids_str += strprintf("%s%s",
                             it == ids.begin() ? "" : ", ", uuid_to_str(*it).c_str());
    }
    return datum_string_t(strprintf("The following %s are all named '%s': %s.",
                                    type.c_str(), name.c_str(), ids_str.c_str()));
}

server_name_collision_issue_t::server_name_collision_issue_t(
        const name_string_t &_name,
        const std::vector<machine_id_t> &_collided_ids) :
    name_collision_issue_t(from_hash(base_issue_id, _name),
                          _name,
                          _collided_ids) { }

ql::datum_t server_name_collision_issue_t::build_info(
        UNUSED const metadata_t &metadata) const {
    return build_server_db_info(name, collided_ids);
}

datum_string_t server_name_collision_issue_t::build_description(
        UNUSED const ql::datum_t &info) const {
    return build_server_db_description("servers", name, collided_ids);
}

db_name_collision_issue_t::db_name_collision_issue_t(
        const name_string_t &_name,
        const std::vector<database_id_t> &_collided_ids) :
    name_collision_issue_t(from_hash(base_issue_id, _name),
                          _name,
                          _collided_ids) { }

ql::datum_t db_name_collision_issue_t::build_info(
        UNUSED const metadata_t &metadata) const {
    return build_server_db_info(name, collided_ids);
}

datum_string_t db_name_collision_issue_t::build_description(
        UNUSED const ql::datum_t &info) const {
    return build_server_db_description("databases", name, collided_ids);
}

table_name_collision_issue_t::table_name_collision_issue_t(
        const name_string_t &_name,
        const database_id_t &_db_id,
        const std::vector<namespace_id_t> &_collided_ids) :
    name_collision_issue_t(from_hash(base_issue_id, _db_id, _name),
                           _name,
                           _collided_ids),
    db_id(_db_id) { }

ql::datum_t table_name_collision_issue_t::build_info(const metadata_t &metadata) const {
    std::string db_name("__deleted_database__");
    auto const db_it = metadata.databases.databases.find(db_id);
    if (db_it != metadata.databases.databases.end() &&
        !db_it->second.is_deleted()) {
        db_name = db_it->second.get_ref().name.get_ref().str();
    }

    ql::datum_array_builder_t ids_builder(ql::configured_limits_t::unlimited);
    for (auto const &id : collided_ids) {
        ids_builder.add(convert_uuid_to_datum(id));
    }

    ql::datum_object_builder_t builder;
    builder.overwrite("name", convert_name_to_datum(name));
    builder.overwrite("db", convert_string_to_datum(db_name));
    builder.overwrite("db_id", convert_uuid_to_datum(db_id));
    builder.overwrite("ids", std::move(ids_builder).to_datum());
    return std::move(builder).to_datum();
}

datum_string_t table_name_collision_issue_t::build_description(const ql::datum_t &info) const {
    std::string ids_str;
    for (auto id : collided_ids) {
        ids_str += strprintf("%s%s",
                             ids_str.empty() ? "" : ", ",
                             uuid_to_str(id).c_str());
    }

    return datum_string_t(strprintf(
        "The following tables in database '%s' are all named '%s': %s.",
        info.get_field("db").as_str().to_std().c_str(),
        name.c_str(),
        ids_str.c_str()));
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

    find_duplicates<server_name_collision_issue_t>(metadata.machines.machines, &issues);
    find_duplicates<db_name_collision_issue_t>(metadata.databases.databases, &issues);
    find_table_duplicates(metadata.rdb_namespaces->namespaces, &issues);

    return issues;
}
