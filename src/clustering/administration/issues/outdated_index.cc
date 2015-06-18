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

outdated_index_issue_t::outdated_index_issue_t() : issue_t(nil_uuid()) { }

outdated_index_issue_t::outdated_index_issue_t(
        const outdated_index_issue_t::index_map_t &_indexes) :
    issue_t(base_issue_id),
    indexes(_indexes) { }

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

RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(outdated_index_issue_t,
    issue_id, reporting_server_ids, indexes);
RDB_IMPL_EQUALITY_COMPARABLE_3(outdated_index_issue_t,
    issue_id, reporting_server_ids, indexes);

class outdated_index_report_impl_t :
        public outdated_index_report_t,
        public home_thread_mixin_t {
public:
    outdated_index_report_impl_t(outdated_index_issue_tracker_t *_parent,
                                 namespace_id_t _ns_id) :
        parent(_parent),
        ns_id(_ns_id) { }

    ~outdated_index_report_impl_t() {
        assert_thread();
        parent->outdated_indexes.get()->erase(ns_id);
        parent->remove_report(this);
    }

    void set_outdated_indexes(std::set<std::string> &&indexes) {
        assert_thread();
        coro_t::spawn_sometime(
            std::bind(&outdated_index_issue_tracker_t::log_outdated_indexes,
                      parent, ns_id, indexes, auto_drainer_t::lock_t(&drainer)));
        (*parent->outdated_indexes.get())[ns_id] = std::move(indexes);
    }

    void index_dropped(const std::string &index_name) {
        assert_thread();
        (*parent->outdated_indexes.get())[ns_id].erase(index_name);
    }

    void indexes_renamed(const std::map<std::string, std::string> &name_changes) {
        assert_thread();
        std::set<std::string> &ns_set = (*parent->outdated_indexes.get())[ns_id];
        for (const auto &pair : name_changes) {
            ns_set.erase(pair.first);
        }
        for (const auto &pair : name_changes) {
            ns_set.insert(pair.second);
        }
    }

private:
    outdated_index_issue_tracker_t *parent;
    namespace_id_t ns_id;
    auto_drainer_t drainer;

    DISABLE_COPYING(outdated_index_report_impl_t);
};

scoped_ptr_t<outdated_index_report_t> outdated_index_issue_tracker_t::create_report(
        const namespace_id_t &ns_id) {
    scoped_ptr_t<outdated_index_report_t> new_report(
        new outdated_index_report_impl_t(this, ns_id));
    index_reports.get()->insert(new_report.get());
    return new_report;
}

void copy_thread_map(
        one_per_thread_t<outdated_index_issue_t::index_map_t> *maps_in,
        std::vector<outdated_index_issue_t::index_map_t> *maps_out, int thread_id) {
    on_thread_t thread_switcher((threadnum_t(thread_id)));
    (*maps_out)[thread_id] = *maps_in->get();
}

std::vector<outdated_index_issue_t> outdated_index_issue_tracker_t::get_issues() {
    std::vector<outdated_index_issue_t::index_map_t> all_maps(get_num_threads());

    pmap(get_num_threads(), std::bind(&copy_thread_map,
                                      &outdated_indexes,
                                      &all_maps,
                                      ph::_1));

    outdated_index_issue_t::index_map_t merged_map = merge_maps(all_maps);

    std::vector<outdated_index_issue_t> issues;
    if (!merged_map.empty()) {
        issues.push_back(outdated_index_issue_t(merged_map));
    }
    return issues;
}

void outdated_index_issue_tracker_t::remove_report(outdated_index_report_impl_t *report) {
    guarantee(index_reports.get()->erase(report) == 1);
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
        std::vector<outdated_index_issue_t> &&issues,
        std::vector<scoped_ptr_t<issue_t> > *issues_out) {
    if (!issues.empty()) {
        std::vector<outdated_index_issue_t::index_map_t> all_maps;
        for (auto const &issue : issues) {
            all_maps.push_back(issue.indexes);
        }
        issues_out->push_back(scoped_ptr_t<issue_t>(
            new outdated_index_issue_t(merge_maps(all_maps))));
    }
}
