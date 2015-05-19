// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/issues/server.hpp"

#include "clustering/administration/servers/config_client.hpp"
#include "clustering/administration/servers/config_server.hpp"
#include "clustering/administration/datum_adapter.hpp"

#if 0

const datum_string_t server_disconnected_issue_t::server_disconnected_issue_type =
    datum_string_t("server_disconnected");
const uuid_u server_disconnected_issue_t::base_issue_id =
    str_to_uuid("377a1d56-db29-416d-97f7-4ce1efc6e97b");

const datum_string_t server_ghost_issue_t::server_ghost_issue_type =
    datum_string_t("server_ghost");
const uuid_u server_ghost_issue_t::base_issue_id =
    str_to_uuid("193df26a-eac7-4373-bf0a-12bbc0b869ed");

server_disconnected_issue_t::server_disconnected_issue_t() { }

server_disconnected_issue_t::server_disconnected_issue_t(
        const server_id_t &_disconnected_server_id) :
    local_issue_t(from_hash(base_issue_id, _disconnected_server_id)),
    disconnected_server_id(_disconnected_server_id) { }

bool server_disconnected_issue_t::build_info_and_description(
        UNUSED const metadata_t &metadata,
        server_config_client_t *server_config_client,
        UNUSED table_meta_client_t *table_meta_client,
        admin_identifier_format_t identifier_format,
        ql::datum_t *info_out,
        datum_string_t *description_out) const {
    ql::datum_t disconnected_server_name_or_uuid;
    name_string_t disconnected_server_name;
    if (!convert_server_id_to_datum(disconnected_server_id,
                                    identifier_format,
                                    server_config_client,
                                    &disconnected_server_name_or_uuid,
                                    &disconnected_server_name)) {
        /* If a disconnected server is deleted from `rethinkdb.server_config`, there is a
        brief window of time before the `server_disconnected_issue_t` is destroyed.
        During that time, if the user reads from `rethinkdb.current_issues`, we don't
        want to show them an issue saying "__deleted_server__ is still connected". So we
        return `false` in this case. */
        return false;
    }
    ql::datum_array_builder_t reporting_servers_builder(
        ql::configured_limits_t::unlimited);
    std::string reporting_servers_str;
    size_t num_reporting = 0;
    for (const server_id_t &id : reporting_server_ids) {
        ql::datum_t name_or_uuid;
        name_string_t name;
        if (!convert_server_id_to_datum(id, identifier_format, server_config_client,
                &name_or_uuid, &name)) {
            /* Ignore connectivity reports from servers that have been declared dead */
            continue;
        }
        reporting_servers_builder.add(name_or_uuid);
        if (!reporting_servers_str.empty()) {
            reporting_servers_str += ", ";
        }
        reporting_servers_str += "`" + name.str() + "`";
        ++num_reporting;
    }
    if (num_reporting == 0) {
        /* The servers making the reports have all been declared dead */
        return false;
    }
    ql::datum_object_builder_t info_builder;
    info_builder.overwrite("disconnected_server", disconnected_server_name_or_uuid);
    info_builder.overwrite("reporting_servers",
        std::move(reporting_servers_builder).to_datum());
    *info_out = std::move(info_builder).to_datum();
    *description_out = datum_string_t(strprintf(
        "Server `%s` is disconnected from %s%s. Here are some reasons why the server "
        "may be disconnected:\n\n"
        "- The server process has crashed.\n"
        "- The network connection between the two servers was interrupted.\n"
        "- The server's canonical address is not configured properly.\n"
        "- The server has experienced a hardware failure.\n"
        "\nIf the server's data has been permanently lost (for example, a disk failure) "
        "you can resolve this issue by deleting the server's entry from the "
        "`rethinkdb.server_config` system table. Once you've deleted the server's "
        "entry, its data and configuration will be discarded.",
        disconnected_server_name.c_str(),
        (num_reporting == 1 ? "" : "these servers: "),
        reporting_servers_str.c_str()));
    return true;
}

server_ghost_issue_t::server_ghost_issue_t() { }

server_ghost_issue_t::server_ghost_issue_t(const server_id_t &_ghost_server_id,
                                           const std::string &_hostname,
                                           int64_t _pid) :
    local_issue_t(from_hash(base_issue_id, _ghost_server_id)),
    ghost_server_id(_ghost_server_id), hostname(_hostname), pid(_pid) { }

bool server_ghost_issue_t::build_info_and_description(
        UNUSED const metadata_t &metadata,
        UNUSED server_config_client_t *server_config_client,
        UNUSED table_meta_client_t *table_meta_client,
        UNUSED admin_identifier_format_t identifier_format,
        ql::datum_t *info_out,
        datum_string_t *description_out) const {
    ql::datum_object_builder_t builder;
    builder.overwrite("server_id", convert_uuid_to_datum(ghost_server_id));
    builder.overwrite("hostname", ql::datum_t(datum_string_t(hostname)));
    builder.overwrite("pid", ql::datum_t(static_cast<double>(pid)));
    *info_out = std::move(builder).to_datum();
    *description_out = datum_string_t(strprintf(
        "The server process with hostname `%s` and PID %" PRId64 " was deleted from the "
        "`rethinkdb.server_config` table, but the process is still running. Once a "
        "server has been deleted from `rethinkdb.server_config`, the data files "
        "for that RethinkDB instance cannot be used any more; you must start a new "
        "RethinkDB instance with an empty data directory.",
        hostname.c_str(), 
        pid));
    return true;
}

server_issue_tracker_t::server_issue_tracker_t(
        local_issue_aggregator_t *parent,
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
            _cluster_sl_view,
        watchable_map_t<peer_id_t, cluster_directory_metadata_t> *_directory_view,
        server_config_client_t *_server_config_client,
        server_config_server_t *_server_config_server) :
    disconnected_issues(std::vector<server_disconnected_issue_t>()),
    ghost_issues(std::vector<server_ghost_issue_t>()),
    disconnected_subs(parent, disconnected_issues.get_watchable(),
        &local_issues_t::server_disconnected_issues),
    ghost_subs(parent, ghost_issues.get_watchable(),
        &local_issues_t::server_ghost_issues),
    cluster_sl_view(_cluster_sl_view),
    directory_view(_directory_view),
    server_config_client(_server_config_client),
    server_config_server(_server_config_server),
    cluster_sl_subs(std::bind(&server_issue_tracker_t::recompute, this),
                    cluster_sl_view),
    directory_subs(directory_view,
                   std::bind(&server_issue_tracker_t::recompute, this),
                   false),
    server_config_client_subs(std::bind(&server_issue_tracker_t::recompute, this))
{
    watchable_t<std::map<server_id_t, peer_id_t> >::freeze_t freeze(
        server_config_client->get_server_id_to_peer_id_map());
    server_config_client_subs.reset(
        server_config_client->get_server_id_to_peer_id_map(), &freeze);
    recompute();   
}

server_issue_tracker_t::~server_issue_tracker_t() {
    // Clear any outstanding disconnected/ghost issues
    disconnected_issues.apply_atomic_op(
        [] (std::vector<server_disconnected_issue_t> *issues) -> bool {
            issues->clear();
            return true;
        });
    ghost_issues.apply_atomic_op(
        [] (std::vector<server_ghost_issue_t> *issues) -> bool {
            issues->clear();
            return true;
        });
}

void server_issue_tracker_t::recompute() {
    std::vector<server_disconnected_issue_t> disconnected_list;
    std::vector<server_ghost_issue_t> ghost_list;
    for (auto const &pair : cluster_sl_view->get().servers.servers) {
        boost::optional<peer_id_t> peer_id =
            server_config_client->get_peer_id_for_server_id(pair.first);
        if (!pair.second.is_deleted() && !static_cast<bool>(peer_id)) {
            if (server_config_server->get_permanently_removed_signal()->is_pulsed()) {
                /* We are a ghost server. Ghost servers don't make disconnection reports.
                */
                continue;
            }
            disconnected_list.push_back(server_disconnected_issue_t(pair.first));
        } else if (pair.second.is_deleted() && static_cast<bool>(peer_id)) {
            directory_view->read_key(*peer_id,
                [&](const cluster_directory_metadata_t *md) {
                    if (md == nullptr) {
                        /* Race condition; the server appeared in
                        `server_config_client`'s list of servers but hasn't appeared in
                        the directory yet. */
                        return;
                    }
                    ghost_list.push_back(server_ghost_issue_t(
                        pair.first, md->hostname, md->pid));
                });
        }
    }
    disconnected_issues.set_value(disconnected_list);
    ghost_issues.set_value(ghost_list);
}

void server_issue_tracker_t::combine(
        local_issues_t *local_issues,
        std::vector<scoped_ptr_t<issue_t> > *issues_out) {
    // Combine disconnected issues
    {
        std::map<server_id_t, server_disconnected_issue_t*> combined_issues;
        for (auto &issue : local_issues->server_disconnected_issues) {
            auto combined_it = combined_issues.find(issue.disconnected_server_id);
            if (combined_it == combined_issues.end()) {
                combined_issues.insert(std::make_pair(
                    issue.disconnected_server_id, &issue));
            } else {
                rassert(issue.reporting_server_ids.size() == 1);
                combined_it->second->add_server(issue.reporting_server_ids[0]);
            }
        }

        for (auto const &it : combined_issues) {
            issues_out->push_back(scoped_ptr_t<issue_t>(
                new server_disconnected_issue_t(*it.second)));
        }
    }

    // Combine ghost issues
    {
        std::set<server_id_t> ghost_issues_seen;
        for (auto &ghost_issue : local_issues->server_ghost_issues) {
            /* This is trivial, since we don't track reporting servers for ghost issues.
            We assume hostname and PID are reported the same by every server, so we just
            show the first report and ignore the others. */
            if (ghost_issues_seen.count(ghost_issue.ghost_server_id) == 0) {
                issues_out->push_back(scoped_ptr_t<issue_t>(
                    new server_ghost_issue_t(ghost_issue)));
                ghost_issues_seen.insert(ghost_issue.ghost_server_id);
            }
        }
    }

}

#endif

