#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/administration/datum_adapter.hpp"
#include "rdb_protocol/pseudo_time.hpp"

void issue_t::to_datum(const metadata_t &metadata,
                       ql::datum_t *datum_out) const {
    ql::datum_t info;
    datum_string_t description;
    build_info_and_description(metadata, &info, &description);

    ql::datum_object_builder_t builder;
    builder.overwrite("id", convert_uuid_to_datum(issue_id));
    builder.overwrite("type", ql::datum_t(get_name()));
    builder.overwrite("critical", ql::datum_t::boolean(is_critical()));
    builder.overwrite("time", ql::pseudo::time_now());
    builder.overwrite("description", ql::datum_t(description));
    builder.overwrite("info", info);
    *datum_out = std::move(builder).to_datum();
}

const std::string issue_t::get_server_name(const issue_t::metadata_t &metadata,
                                           const machine_id_t &server_id) {
    auto machine_it = metadata.machines.machines.find(server_id);
    if (machine_it == metadata.machines.machines.end() ||
        machine_it->second.is_deleted()) {
        return std::string("<deleted>");
    }
    return machine_it->second.get_ref().name.get_ref().str();
}


issue_tracker_t::issue_tracker_t(issue_multiplexer_t *_parent) : parent(_parent) {
    parent->assert_thread();
    parent->trackers.insert(this);
}

issue_tracker_t::~issue_tracker_t() {
    parent->assert_thread();
    parent->trackers.erase(this);
}

issue_multiplexer_t::issue_multiplexer_t(
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
            _cluster_sl_view) :
    cluster_sl_view(_cluster_sl_view) { }

std::vector<scoped_ptr_t<issue_t> > issue_multiplexer_t::all_issues() const {
    std::vector<scoped_ptr_t<issue_t> > all_issues;

    for (auto const &tracker : trackers) {
        std::vector<scoped_ptr_t<issue_t> > issues = tracker->get_issues(); 
        std::move(issues.begin(), issues.end(), std::back_inserter(all_issues));
    }

    return all_issues;
}

void issue_multiplexer_t::get_issue_ids(std::vector<ql::datum_t> *ids_out) const {
    assert_thread();
    ids_out->clear();

    std::vector<scoped_ptr_t<issue_t> > issues = all_issues(); 

    for (auto const &issue : issues) {
        ids_out->push_back(convert_uuid_to_datum(issue->get_id()));
    }
}

void issue_multiplexer_t::get_issue(const uuid_u &issue_id,
                                    ql::datum_t *row_out) const {
    assert_thread();
    std::vector<scoped_ptr_t<issue_t> > issues = all_issues();

    for (auto const &issue : issues) {
        if (issue->get_id() == issue_id) {
            issue->to_datum(cluster_sl_view->get(), row_out);
            return;
        }
    }

    *row_out = ql::datum_t();
}
