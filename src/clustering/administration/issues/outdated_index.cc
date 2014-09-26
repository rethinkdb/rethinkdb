// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/issues/outdated_index.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/datum_adapter.hpp"
#include "rdb_protocol/configured_limits.hpp"

const datum_string_t outdated_index_issue_t::outdated_index_issue_type =
    datum_string_t("outdated_index");
const uuid_u outdated_index_issue_t::base_issue_id =
    str_to_uuid("528f5239-096e-47a3-b263-7a06a256ecc3");

// Merge two maps of outdated indexes by table id
local_issue_t::outdated_index_map_t merge_maps(
        const std::vector<local_issue_t::outdated_index_map_t> &maps) {
    local_issue_t::outdated_index_map_t res;
    for (auto it = maps.begin(); it != maps.end(); ++it) {
        for (auto jt = it->begin(); jt != it->end(); ++jt) {
            if (!jt->second.empty()) {
                res[jt->first].insert(jt->second.begin(), jt->second.end());
            }
        }
    }
    return res;
}

outdated_index_issue_t::outdated_index_issue_t(
        const local_issue_t::outdated_index_map_t &_indexes) :
    issue_t(base_issue_id),
    indexes(_indexes) { }

void outdated_index_issue_t::build_info_and_description(const metadata_t &metadata,
                                                        ql::datum_t *info_out,
                                                        datum_string_t *desc_out) const {
    build_info(metadata, info_out);
    build_description(desc_out);
}

void outdated_index_issue_t::build_description(datum_string_t *desc_out) const {
    std::string index_table;
    for (auto it : indexes) {
        std::string index_str;
        for (auto jt = it.second.begin(); jt != it.second.end(); ++jt) {
            index_str += strprintf("%s%s",
                                   jt == it.second.begin() ? "" : ", ", jt->c_str());
        }

        index_table += strprintf("\n%s\t%s",
                                 uuid_to_str(it.first).c_str(), index_str.c_str());
    }

    // TODO: use table names rather than UUIDs here
    *desc_out = datum_string_t(strprintf(
        "The cluster contains indexes that were created with a previous version of the "
        "query language which contained some bugs.  These should be remade to avoid "
        "relying on broken behavior.  See <url> for details.\n"
        "\tTable                               \tIndexes%s", index_table.c_str()));
}

void outdated_index_issue_t::build_info(const metadata_t &metadata,
                                        ql::datum_t *info_out) const {
    ql::datum_object_builder_t builder;
    ql::datum_array_builder_t tables(ql::configured_limits_t::unlimited);
    for (auto const &table : indexes) {
        std::string table_name("__deleted_table__");
        std::string db_name("__deleted_database__");
        uuid_u db_id = nil_uuid();

        // Get the table name and db_id
        auto const table_it = metadata.rdb_namespaces->namespaces.find(table.first);
        if (table_it != metadata.rdb_namespaces->namespaces.end() &&
            !table_it->second.is_deleted()) {
            table_name = table_it->second.get_ref().name.get_ref().str();
            db_id = table_it->second.get_ref().database.get_ref();
        }

        // Get the database name
        auto const db_it = metadata.databases.databases.find(db_id);
        if (db_it != metadata.databases.databases.end() &&
            !db_it->second.is_deleted()) {
            db_name = db_it->second.get_ref().name.get_ref().str();
        }

        ql::datum_object_builder_t item;
        item.overwrite("table", convert_string_to_datum(table_name));
        item.overwrite("table_id", convert_uuid_to_datum(table.first));
        item.overwrite("db", convert_string_to_datum(db_name));
        item.overwrite("db_id", convert_uuid_to_datum(db_id));

        ql::datum_array_builder_t index_list(ql::configured_limits_t::unlimited);
        for (auto const &index : table.second) {
            index_list.add(convert_string_to_datum(index));
        }
        item.overwrite("indexes", std::move(index_list).to_datum());

        tables.add(std::move(item).to_datum());
    }
    builder.overwrite("tables", std::move(tables).to_datum());
    *info_out = std::move(builder).to_datum();
}

class outdated_index_report_impl_t :
        public outdated_index_report_t,
        public home_thread_mixin_t {
public:
    outdated_index_report_impl_t(outdated_index_issue_tracker_t *_parent,
                                 namespace_id_t _ns_id) :
        parent(_parent),
        ns_id(_ns_id) { }

    ~outdated_index_report_impl_t() { }

    void set_outdated_indexes(std::set<std::string> &&indexes) {
        assert_thread();
        coro_t::spawn_sometime(
            std::bind(&outdated_index_issue_tracker_t::log_outdated_indexes,
                      parent, ns_id, indexes, auto_drainer_t::lock_t(&drainer)));
        (*parent->outdated_indexes.get())[ns_id] = std::move(indexes);
        parent->recompute();
    }

    void index_dropped(const std::string &index_name) {
        assert_thread();
        (*parent->outdated_indexes.get())[ns_id].erase(index_name);
        parent->recompute();
    }

    void index_renamed(const std::string &old_name,
                       const std::string &new_name) {
        assert_thread();
        std::set<std::string> &ns_set = (*parent->outdated_indexes.get())[ns_id];

        if (ns_set.find(old_name) != ns_set.end()) {
            ns_set.erase(old_name);
            ns_set.insert(new_name);
        }
        parent->recompute();
    }

    void destroy() {
        assert_thread();
        parent->outdated_indexes.get()->erase(ns_id);
        parent->destroy_report(this);
        parent->recompute();
    }

private:
    outdated_index_issue_tracker_t *parent;
    namespace_id_t ns_id;
    auto_drainer_t drainer;

    DISABLE_COPYING(outdated_index_report_impl_t);
};

outdated_index_issue_tracker_t::outdated_index_issue_tracker_t(
        local_issue_aggregator_t *_parent) :
    local_issue_tracker_t(_parent),
    update_pool(1, &update_queue, this) { }

outdated_index_issue_tracker_t::~outdated_index_issue_tracker_t() { }

outdated_index_report_t *outdated_index_issue_tracker_t::create_report(
        const namespace_id_t &ns_id) {
    outdated_index_report_impl_t *new_report = new outdated_index_report_impl_t(this, ns_id);
    index_reports.get()->insert(new_report);
    return new_report;
}

void outdated_index_issue_tracker_t::destroy_report(outdated_index_report_impl_t *report) {
    guarantee(index_reports.get()->erase(report) == 1);
}

void copy_thread_map(
        one_per_thread_t<local_issue_t::outdated_index_map_t> *maps_in,
        std::vector<local_issue_t::outdated_index_map_t> *maps_out, int thread_id) {
    on_thread_t thread_switcher((threadnum_t(thread_id)));
    (*maps_out)[thread_id] = *maps_in->get();
}

void outdated_index_issue_tracker_t::recompute() {
    on_thread_t rethreader((threadnum_t(home_thread())));
    update_queue.give_value(outdated_index_dummy_value_t());
}

void outdated_index_issue_tracker_t::coro_pool_callback(
        UNUSED outdated_index_dummy_value_t dummy,
        UNUSED signal_t *interruptor) {
    // Collect outdated indexes by table id from all threads
    std::vector<local_issue_t::outdated_index_map_t> all_maps(get_num_threads());
    pmap(get_num_threads(), std::bind(&copy_thread_map,
                                      &outdated_indexes,
                                      &all_maps,
                                      ph::_1));

    // Set this as the new issue
    local_issue_t new_issue = local_issue_t::make_outdated_index_issue(merge_maps(all_maps));
    std::vector<local_issue_t> issues;
    issues.push_back(std::move(new_issue));
    update(issues);
}

void outdated_index_issue_tracker_t::log_outdated_indexes(
        namespace_id_t ns_id,
        std::set<std::string> indexes,
        UNUSED auto_drainer_t::lock_t keepalive) {
    on_thread_t rethreader(home_thread());
    if (indexes.size() > 0 &&
        logged_namespaces.find(ns_id) == logged_namespaces.end()) {

        std::string index_list;
        for (auto const &s : indexes) {
            index_list += (index_list.empty() ? "'" : ", '") + s + "'";
        }

        logWRN("Namespace %s contains these outdated indexes which should be "
               "recreated: %s", uuid_to_str(ns_id).c_str(), index_list.c_str());

        logged_namespaces.insert(ns_id);
    }
}
