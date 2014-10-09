// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/issues/log_write.hpp"
#include "clustering/administration/datum_adapter.hpp"

const datum_string_t log_write_issue_t::log_write_issue_type =
    datum_string_t("log_write_error");
const uuid_u log_write_issue_t::base_issue_id =
    str_to_uuid("a0c0dc6d-aa7f-4119-a29d-4c717c343445");

log_write_issue_t::log_write_issue_t() { }

log_write_issue_t::log_write_issue_t(const std::string &_message) :
    local_issue_t(from_hash(base_issue_id, _message)),
    message(_message) { }

ql::datum_t log_write_issue_t::build_info(const metadata_t &metadata) const {
    ql::datum_object_builder_t builder;
    ql::datum_array_builder_t server_names(ql::configured_limits_t::unlimited);
    ql::datum_array_builder_t server_ids(ql::configured_limits_t::unlimited);

    for (auto const &server_id : affected_server_ids) {
        server_names.add(convert_string_to_datum(get_server_name(metadata, server_id)));
        server_ids.add(convert_uuid_to_datum(server_id));
    }

    builder.overwrite("servers", std::move(server_names).to_datum());
    builder.overwrite("server_ids", std::move(server_ids).to_datum());
    builder.overwrite("message", convert_string_to_datum(message));
    return std::move(builder).to_datum();
}

datum_string_t log_write_issue_t::build_description(const ql::datum_t &info) const {
    ql::datum_t servers = info.get_field("servers");
    std::string servers_string;
    for (size_t i = 0; i < servers.arr_size(); ++i) {
        servers_string += strprintf("%s%s",
            servers_string.empty() ? "" : ", ",
            servers.get(i).as_str().to_std().c_str());
    }

    return datum_string_t(strprintf(
        "The following server%s encountered an error ('%s') while "
        "writing log statements: %s.",
        servers.arr_size() == 1 ? "" : "s",
        info.get_field("message").as_str().to_std().c_str(),
        servers_string.c_str()));
}

log_write_issue_tracker_t::log_write_issue_tracker_t(local_issue_aggregator_t *parent) :
    issues(std::vector<log_write_issue_t>()),
    subs(parent, issues.get_watchable(), &local_issues_t::log_write_issues) { }

log_write_issue_tracker_t::~log_write_issue_tracker_t() {
    // Clear any log write issue
    report_success();
}

void log_write_issue_tracker_t::do_update() {
    issues.apply_atomic_op(
        [&](std::vector<log_write_issue_t> *local_issues) -> bool {
            local_issues->clear();
            if (error_message) {
                local_issues->push_back(log_write_issue_t(error_message.get()));
            }
            return true;
        });
}

void log_write_issue_tracker_t::report_success() {
    assert_thread();
    if (error_message) {
        error_message.reset();
        do_update();
    }
}

void log_write_issue_tracker_t::report_error(const std::string &message) {
    assert_thread();
    if (!error_message || error_message.get() != message) {
        error_message = message;
        do_update();
    }
}

void log_write_issue_tracker_t::combine(
        local_issues_t *local_issues,
        std::vector<scoped_ptr_t<issue_t> > *issues_out) {
    std::map<std::string, log_write_issue_t*> combined_issues;
    for (auto &issue : local_issues->log_write_issues) {
        auto combined_it = combined_issues.find(issue.message);
        if (combined_it == combined_issues.end()) {
            combined_issues.insert(std::make_pair(issue.message, &issue));
        } else {
            rassert(issue.affected_server_ids.size() == 1);
            combined_it->second->add_server(issue.affected_server_ids[0]);
        }
    }

    for (auto const &it : combined_issues) {
        issues_out->push_back(scoped_ptr_t<issue_t>(new log_write_issue_t(*it.second)));
    }
}
