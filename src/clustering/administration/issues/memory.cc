//Copyright 2010-2016 RethinkDB, all rights reserved.
#include "clustering/administration/issues/memory.hpp"
#include "clustering/administration/datum_adapter.hpp"

const datum_string_t memory_issue_t::memory_issue_type =
    datum_string_t("memory_error");
const uuid_u memory_issue_t::base_issue_id =
    str_to_uuid("e013d7af-dd63-4fc0-a021-d9b57591685b");

memory_issue_t::memory_issue_t() : issue_t(nil_uuid()) { }

memory_issue_t::memory_issue_t(const std::string &_message) :
    issue_t(from_hash(base_issue_id, _message)),
    message(_message) { }

bool memory_issue_t::build_info_and_description(
    const metadata_t &,
    server_config_client_t *server_config_client,
    table_meta_client_t *,
    admin_identifier_format_t identifier_format,
    ql::datum_t *info_out,
    datum_string_t *description_out) const {

    ql::datum_object_builder_t info_builder;
    ql::datum_array_builder_t servers_builder(ql::configured_limits_t::unlimited);
    std::string servers_string;

    for (auto const &server_id : reporting_server_ids) {
        ql::datum_t server_name_or_uuid;
        name_string_t server_name;
        if (!convert_connected_server_id_to_datum(server_id,
                                                  identifier_format,
                                                  server_config_client,
                                                  &server_name_or_uuid,
                                                  &server_name)) {
            continue;
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
        "The following server%s encountered a memory problem: %s. \n"
        "\n The message reported is: %s\n\n",
        reporting_server_ids.size() == 1 ? "" : "s",
        servers_string.c_str(),
        message.c_str()));
    return true;
}

RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(memory_issue_t,
                                    issue_id,
                                    reporting_server_ids,
                                    message);

RDB_IMPL_EQUALITY_COMPARABLE_3(memory_issue_t,
                               issue_id,
                               reporting_server_ids,
                               message);

std::vector<memory_issue_t> memory_issue_tracker_t::get_issues() {
    std::vector<memory_issue_t> issues;
    if (static_cast<bool>(error_message)) {
        issues.push_back(memory_issue_t(*error_message));
    }
    return issues;
}

void memory_issue_tracker_t::report_success() {
    assert_thread();
    error_message = boost::none;
}

void memory_issue_tracker_t::report_error(const std::string &message) {
    assert_thread();
    error_message = boost::make_optional(message);
}

void memory_issue_tracker_t::combine(
    std::vector<memory_issue_t> &&issues,
    std::vector<scoped_ptr_t<issue_t> > *issues_out) {
    std::map<std::string, memory_issue_t*> combined_issues;
    for (auto &issue : issues) {
        auto combined_it = combined_issues.find(issue.message);
        if (combined_it == combined_issues.end()) {
            combined_issues.insert(std::make_pair(issue.message, &issue));
        } else {
            rassert(issue.reporting_server_ids.size() == 1);
            combined_it->second->add_server(*issue.reporting_server_ids.begin());
        }
    }

    for (auto const &it : combined_issues) {
        issues_out->push_back(scoped_ptr_t<issue_t>(new memory_issue_t(*it.second)));
    }
}
