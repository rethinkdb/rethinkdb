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
outdated_index_issue_t::index_map_t merge_maps(
        const std::vector<outdated_index_issue_t::index_map_t> &maps) {
    outdated_index_issue_t::index_map_t res;
    for (auto it = maps.begin(); it != maps.end(); ++it) {
        for (auto jt = it->begin(); jt != it->end(); ++jt) {
            if (!jt->second.empty()) {
                res[jt->first].insert(jt->second.begin(), jt->second.end());
            }
        }
    }
    return res;
}

outdated_index_issue_t::outdated_index_issue_t() { }

outdated_index_issue_t::outdated_index_issue_t(
        const outdated_index_issue_t::index_map_t &_indexes) :
    local_issue_t(base_issue_id),
    indexes(_indexes) { }

ql::datum_t outdated_index_issue_t::build_info(const metadata_t &metadata) const {
    ql::datum_object_builder_t builder;
    ql::datum_array_builder_t tables(ql::configured_limits_t::unlimited);
    for (auto const &table : indexes) {
        namespaces_semilattice_metadata_t::namespace_map_t::const_iterator table_it;
        databases_semilattice_metadata_t::database_map_t::const_iterator db_it;

        // If we can't find the table, skip it
        if (!search_const_metadata_by_uuid(&metadata.rdb_namespaces.get()->namespaces,
                                           table.first, &table_it)) {
            continue;
        }

        std::string table_name = table_it->second.get_ref().name.get_ref().str();
        std::string db_name("__deleted_database__");
        database_id_t db_id = table_it->second.get_ref().database.get_ref();

        // Get the database name
        if (search_const_metadata_by_uuid(&metadata.databases.databases,
                                          table_it->second.get_ref().database.get_ref(),
                                          &db_it)) {
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
    return std::move(builder).to_datum();
}

datum_string_t outdated_index_issue_t::build_description(const ql::datum_t &info) const {
    std::string index_table;
    ql::datum_t tables = info.get_field("tables");
    for (size_t i = 0; i < tables.arr_size(); ++i) {
        ql::datum_t table_info = tables.get(i);
        ql::datum_t indexes_ = table_info.get_field("indexes");

        std::string index_str;
        for (size_t j = 0; j < indexes_.arr_size(); ++j) {
            index_str += strprintf("%s`%s`",
                                   j == 0 ? "" : ", ",
                                   indexes_.get(j).as_str().to_std().c_str());
        }

        index_table += strprintf("\nFor table %s.%s: %s.",
                                 table_info.get_field("db").as_str().to_std().c_str(),
                                 table_info.get_field("table").as_str().to_std().c_str(),
                                 index_str.c_str());
    }

    return datum_string_t(strprintf(
        "The cluster contains indexes that were created with a previous version of the "
        "query language which contained some bugs.  These should be remade to avoid "
        "relying on broken behavior.  See "
        "http://www.rethinkdb.com/docs/troubleshooting/#my-secondary-index-is-outdated "
        "for details.%s", index_table.c_str()));
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
        local_issue_aggregator_t *parent) :
    issues(std::vector<outdated_index_issue_t>()),
    subs(parent, issues.get_watchable(), &local_issues_t::outdated_index_issues),
    update_pool(1, &update_queue, this) { }

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
        one_per_thread_t<outdated_index_issue_t::index_map_t> *maps_in,
        std::vector<outdated_index_issue_t::index_map_t> *maps_out, int thread_id) {
    on_thread_t thread_switcher((threadnum_t(thread_id)));
    (*maps_out)[thread_id] = *maps_in->get();
}

void outdated_index_issue_tracker_t::recompute() {
    on_thread_t rethreader((threadnum_t(home_thread())));
    update_queue.give_value(outdated_index_dummy_value_t());
}

outdated_index_issue_tracker_t::~outdated_index_issue_tracker_t() {
    issues.apply_atomic_op(
        [] (std::vector<outdated_index_issue_t> *local_issues) -> bool {
            local_issues->clear();
            return true;
        });
}

void outdated_index_issue_tracker_t::coro_pool_callback(
        UNUSED outdated_index_dummy_value_t dummy,
        UNUSED signal_t *interruptor) {
    // Collect outdated indexes by table id from all threads
    std::vector<outdated_index_issue_t::index_map_t> all_maps(get_num_threads());
    pmap(get_num_threads(), std::bind(&copy_thread_map,
                                      &outdated_indexes,
                                      &all_maps,
                                      ph::_1));

    outdated_index_issue_t::index_map_t merged_map = merge_maps(all_maps);
    issues.apply_atomic_op(
        [&] (std::vector<outdated_index_issue_t> *local_issues) -> bool {
            local_issues->clear();
            if (!merged_map.empty()) {
                local_issues->push_back(outdated_index_issue_t(merged_map));
            }
            return true;
        });
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

void outdated_index_issue_tracker_t::combine(
        local_issues_t *local_issues,
        std::vector<scoped_ptr_t<issue_t> > *issues_out) {
    if (!local_issues->outdated_index_issues.empty()) {
        std::vector<outdated_index_issue_t::index_map_t> all_maps;
        for (auto const &issue : local_issues->outdated_index_issues) {
            all_maps.push_back(issue.indexes);           
        }
        issues_out->push_back(scoped_ptr_t<issue_t>(
            new outdated_index_issue_t(merge_maps(all_maps))));
    }
}
