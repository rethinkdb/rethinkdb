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

bool log_write_issue_t::build_info_and_description(
        UNUSED const metadata_t &metadata,
        server_config_client_t *server_config_client,
        admin_identifier_format_t identifier_format,
        ql::datum_t *info_out,
        datum_string_t *description_out) const {
    ql::datum_object_builder_t info_builder;
    ql::datum_array_builder_t servers_builder(ql::configured_limits_t::unlimited);
    std::string servers_string;
    for (auto const &server_id : affected_server_ids) {
        ql::datum_t server_name_or_uuid;
        name_string_t server_name;
        if (!convert_server_id_to_datum(server_id, identifier_format,
                server_config_client, &server_name_or_uuid, &server_name)) {
            server_name_or_uuid = ql::datum_t("__deleted_server__");
            server_name = name_string_t::guarantee_valid("__deleted_server__");
        }
        servers_builder.add(server_name_or_uuid);
        if (!servers_string.empty()) {
            servers_string += ", ";
        }
        servers_string += server_name.str();
    }
    info_builder.overwrite("servers", std::move(servers_builder).to_datum());
    info_builder.overwrite("message", convert_string_to_datum(message));
    *info_out = std::move(info_builder).to_datum();
    *description_out = datum_string_t(strprintf(
        "The following server%s encountered an error ('%s') while writing log "
        "statements: %s.",
        (affected_server_ids.size() == 1 ? "" : "s"),
        message.c_str(),
        servers_string.c_str()));
    return true;
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
