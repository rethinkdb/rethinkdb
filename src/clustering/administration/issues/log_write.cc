// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/issues/log_write.hpp"

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
    builder.overwrite(datum_string_t("server"),
                      ql::datum_t(datum_string_t(server_name)));
    builder.overwrite(datum_string_t("server_id"),
                      ql::datum_t(datum_string_t(uuid_to_str(affected_servers[0]))));
    builder.overwrite(datum_string_t("message"),
                      ql::datum_t(datum_string_t(message)));
    *info_out = std::move(builder).to_datum();

    *desc_out = datum_string_t(strprintf("Server %s encountered an error while writing "
                                         "to its log file: %s.",
                                         server_name.c_str(), message.c_str()));
}

log_write_issue_tracker_t::log_write_issue_tracker_t(local_issue_aggregator_t *_parent) :
    local_issue_tracker_t(_parent),
    active_entry(NULL) { }

log_write_issue_tracker_t::~log_write_issue_tracker_t() {
    guarantee(active_entry == NULL);
}

log_write_issue_tracker_t::entry_t::entry_t(log_write_issue_tracker_t *_parent) :
        parent(_parent) {
    parent->assert_thread();
    guarantee(parent->active_entry == NULL);
    parent->active_entry = this;
}

log_write_issue_tracker_t::entry_t::~entry_t() {
    parent->assert_thread();
    guarantee(parent->active_entry == this);
    parent->active_entry = NULL;
}

void log_write_issue_tracker_t::entry_t::set_message(const std::string &_message) {
    parent->assert_thread();
    if (message != _message) {
        message = _message;

        std::vector<local_issue_t> issues; // There can be only one log write issue
        issues.push_back(local_issue_t::make_log_write_issue(message));
        parent->update(issues);
    }
}
