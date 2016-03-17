#include "clustering/administration/issues/non_transitive.hpp"

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/servers/config_client.hpp"

bool non_transitive_issue_t::build_info_and_description(
    const metadata_t &,
    server_config_client_t *config_client,
    table_meta_client_t *,
    admin_identifier_format_t identifier_format,
    ql::datum_t *info_out,
    datum_string_t *description_out) const {
    ql::datum_object_builder_t info_builder;
    info_builder.overwrite("message",
                           convert_string_to_datum(message));

    ql::datum_array_builder_t servers_builder(ql::configured_limits_t::unlimited);
    for (auto const &server_id : reporting_server_ids) {
        ql::datum_t server_name_or_uuid;
        if (!convert_connected_server_id_to_datum(
                server_id,
                identifier_format,
                config_client,
                &server_name_or_uuid,
                nullptr)) {
            continue;
        }
        servers_builder.add(server_name_or_uuid);
    }
    info_builder.overwrite("servers", std::move(servers_builder).to_datum());
    *info_out = std::move(info_builder).to_datum();
    *description_out = datum_string_t(
        "There are currently servers that cannot see every server"
        " in the cluster. This may cause table availability issues.");
    return true;
}

const datum_string_t non_transitive_issue_t::non_transitive_issue_type =
    datum_string_t("non_transitive_error");
const uuid_u non_transitive_issue_t::base_type_id =
    str_to_uuid("ab2a4f6d-c623-4c11-9982-8e5cc653737d");

non_transitive_issue_tracker_t::non_transitive_issue_tracker_t(
    server_config_client_t *_server_config_client) :
    server_config_client(_server_config_client) { }

non_transitive_issue_tracker_t::~non_transitive_issue_tracker_t() { }

std::vector<scoped_ptr_t<issue_t> > non_transitive_issue_tracker_t::get_issues(
    signal_t *interruptor) const {
    std::vector<scoped_ptr_t<issue_t> > issues;

    const server_connectivity_t &server_connectivity =
        server_config_client->get_server_connectivity();

    bool is_transitive = true;
    int total_servers = server_connectivity.all_servers.size();

    non_transitive_issue_t issue("Server connectivity is non-transitive.");
    // Check if all the servers can see each other
    for (const auto &server_pair : server_connectivity.all_servers) {
        if (server_pair.second != total_servers * 2) {
            is_transitive = false;
            issue.add_server(server_pair.first);
        }
    }

    if (is_transitive == false) {
        ql::datum_t row;
        issues.push_back(make_scoped<non_transitive_issue_t>(issue));
    }

    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
    return std::move(issues);
}
