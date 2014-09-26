// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/issues/log_write.hpp"
#include "clustering/administration/datum_adapter.hpp"

const datum_string_t log_write_issue_t::log_write_issue_type =
    datum_string_t("log_write_error");
const uuid_u log_write_issue_t::base_issue_id =
    str_to_uuid("a0c0dc6d-aa7f-4119-a29d-4c717c343445");

log_write_issue_t::log_write_issue_t(const std::string &_message,
                                     const std::vector<machine_id_t> &_affected_servers) :
    issue_t(uuid_u::from_hash(base_issue_id, _affected_servers, _message)),
    message(_message),
    affected_servers(_affected_servers) { }

void log_write_issue_t::build_info_and_description(const metadata_t &metadata,
                                                   ql::datum_t *info_out,
                                                   datum_string_t *desc_out) const {
    // There should be no issue reduction performed on log write issues
    rassert(affected_servers.size() == 1);

    std::string server_name = get_server_name(metadata, affected_servers[0]);
    ql::datum_object_builder_t builder;
    builder.overwrite("server", convert_string_to_datum(server_name));
    builder.overwrite("server_id", convert_uuid_to_datum(affected_servers[0]));
    builder.overwrite("message", convert_string_to_datum(message));
    *info_out = std::move(builder).to_datum();

    *desc_out = datum_string_t(strprintf("Server %s encountered an error while writing "
                                         "to its log file: %s.",
                                         server_name.c_str(), message.c_str()));
}

log_write_issue_tracker_t::log_write_issue_tracker_t(local_issue_aggregator_t *_parent) :
    local_issue_tracker_t(_parent) { }

void log_write_issue_tracker_t::report_success() {
    assert_thread();
    if (error_message) {
        error_message.reset();
        update(std::vector<local_issue_t>());
    }
}

void log_write_issue_tracker_t::report_error(const std::string &message) {
    assert_thread();
    if (!error_message || error_message.get() != message) {
        error_message = message;

        std::vector<local_issue_t> issues;
        issues.push_back(local_issue_t::make_log_write_issue(error_message.get()));
        update(issues);
    }
}
