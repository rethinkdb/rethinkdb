// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/issues/server.hpp"

#include "clustering/administration/servers/name_client.hpp"
#include "clustering/administration/servers/name_server.hpp"
#include "clustering/administration/datum_adapter.hpp"

const datum_string_t server_down_issue_t::server_down_issue_type =
    datum_string_t("server_down");
const uuid_u server_down_issue_t::base_issue_id =
    str_to_uuid("377a1d56-db29-416d-97f7-4ce1efc6e97b");

const datum_string_t server_ghost_issue_t::server_ghost_issue_type =
    datum_string_t("server_ghost");
const uuid_u server_ghost_issue_t::base_issue_id =
    str_to_uuid("193df26a-eac7-4373-bf0a-12bbc0b869ed");

std::vector<std::string> look_up_servers(const issue_t::metadata_t &metadata,
                                         const std::vector<server_id_t> &servers) {
    std::vector<std::string> res;
    res.reserve(servers.size());
    for (auto const &server : servers) {
        res.push_back(issue_t::get_server_name(metadata, server));
    }
    return res;
}

std::string servers_to_string(const ql::datum_t &server_names) {
    std::string res;
    for (size_t i = 0; i < server_names.arr_size(); ++i) {
        res.append(strprintf("%s'%s'",
                             res.empty() ? "" : ", ",
                             server_names.get(i).as_str().to_std().c_str()));
    }
    return res;
}

server_down_issue_t::server_down_issue_t() { }

server_down_issue_t::server_down_issue_t(const server_id_t &_down_server_id) :
    local_issue_t(from_hash(base_issue_id, _down_server_id)),
    down_server_id(_down_server_id) { }

ql::datum_t server_down_issue_t::build_info(const metadata_t &metadata) const {
    /* If a disconnected server is deleted from `rethinkdb.server_config`, there is a
    brief window of time before the `server_down_issue_t` is destroyed. During that time,
    if the user reads from `rethinkdb.issues`, we don't want to show them an issue saying
    "__deleted_server__ is still connected". So we return an empty datum in this case. */
    auto it = metadata.servers.servers.find(down_server_id);
    if (it == metadata.servers.servers.end() || it->second.is_deleted()) {
        return ql::datum_t();
    }
    const std::string name = it->second.get_ref().name.get_ref().str();

    const std::vector<std::string> affected_server_names =
        look_up_servers(metadata, affected_server_ids);

    ql::datum_object_builder_t builder;
    ql::datum_array_builder_t server_array(ql::configured_limits_t::unlimited);
    ql::datum_array_builder_t server_id_array(ql::configured_limits_t::unlimited);

    for (const auto &s : affected_server_names) {
        server_array.add(convert_string_to_datum(s));
    }
    for (const auto &s : affected_server_ids) {
        server_id_array.add(convert_uuid_to_datum(s));
    }

    builder.overwrite("server", convert_string_to_datum(name));
    builder.overwrite("server_id", convert_uuid_to_datum(down_server_id));
    builder.overwrite("affected_servers", std::move(server_array).to_datum());
    builder.overwrite("affected_server_ids", std::move(server_id_array).to_datum());

    return std::move(builder).to_datum();
}

datum_string_t server_down_issue_t::build_description(const ql::datum_t &info) const {
    return datum_string_t(strprintf(
        "Server '%s' is inaccessible from %s%s.",
        info.get_field("server").as_str().to_std().c_str(),
        affected_server_ids.size() == 1 ? "" : "these servers: ",
        servers_to_string(info.get_field("affected_servers")).c_str()));
}

server_ghost_issue_t::server_ghost_issue_t() { }

server_ghost_issue_t::server_ghost_issue_t(const server_id_t &_ghost_server_id,
                                           const std::string &_hostname,
                                           int64_t _pid) :
    local_issue_t(from_hash(base_issue_id, _ghost_server_id)),
    ghost_server_id(_ghost_server_id), hostname(_hostname), pid(_pid) { }

ql::datum_t server_ghost_issue_t::build_info(const metadata_t &) const {
    ql::datum_object_builder_t builder;
    builder.overwrite("server_id", convert_uuid_to_datum(ghost_server_id));
    builder.overwrite("hostname", ql::datum_t(datum_string_t(hostname)));
    builder.overwrite("pid", ql::datum_t(static_cast<double>(pid)));
    return std::move(builder).to_datum();
}

datum_string_t server_ghost_issue_t::build_description(const ql::datum_t &) const {
    return datum_string_t(strprintf(
        "The server process with hostname `%s` and PID %" PRId64 " was deleted from the "
        "`rethinkdb.server_config` table, but the process is still running. Once a "
        "server has been deleted from `rethinkdb.server_config`, the data files "
        "for that RethinkDB instance cannot be used any more; you must start a new "
        "RethinkDB instance with an empty data directory.",
        hostname.c_str(), 
        pid));
}

server_issue_tracker_t::server_issue_tracker_t(
        local_issue_aggregator_t *parent,
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
            _cluster_sl_view,
        watchable_map_t<peer_id_t, cluster_directory_metadata_t> *_directory_view,
        server_name_client_t *_name_client,
        server_name_server_t *_name_server) :
    down_issues(std::vector<server_down_issue_t>()),
    ghost_issues(std::vector<server_ghost_issue_t>()),
    down_subs(parent, down_issues.get_watchable(),
        &local_issues_t::server_down_issues),
    ghost_subs(parent, ghost_issues.get_watchable(),
        &local_issues_t::server_ghost_issues),
    cluster_sl_view(_cluster_sl_view),
    directory_view(_directory_view),
    name_client(_name_client),
    name_server(_name_server),
    cluster_sl_subs(std::bind(&server_issue_tracker_t::recompute, this),
                    cluster_sl_view),
    directory_subs(directory_view,
                   std::bind(&server_issue_tracker_t::recompute, this),
                   false),
    name_client_subs(std::bind(&server_issue_tracker_t::recompute, this))
{
    watchable_t<std::map<server_id_t, peer_id_t> >::freeze_t freeze(
        name_client->get_server_id_to_peer_id_map());
    name_client_subs.reset(name_client->get_server_id_to_peer_id_map(), &freeze);
    recompute();   
}

server_issue_tracker_t::~server_issue_tracker_t() {
    // Clear any outstanding down/ghost issues
    down_issues.apply_atomic_op(
        [] (std::vector<server_down_issue_t> *issues) -> bool {
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
    debugf("recompute\n");
    std::vector<server_down_issue_t> down_list;
    std::vector<server_ghost_issue_t> ghost_list;
    for (auto const &pair : cluster_sl_view->get().servers.servers) {
        boost::optional<peer_id_t> peer_id =
            name_client->get_peer_id_for_server_id(pair.first);
        debugf("%s %d %d\n", uuid_to_str(pair.first).c_str(),
            int(static_cast<bool>(peer_id)),
            int(pair.second.is_deleted()));
        if (!pair.second.is_deleted() && !static_cast<bool>(peer_id)) {
            if (name_server->get_permanently_removed_signal()->is_pulsed()) {
                /* We are a ghost server. Ghost servers don't make disconnection reports.
                */
                continue;
            }
            debugf("make server down issue\n");
            down_list.push_back(server_down_issue_t(pair.first));
        } else if (pair.second.is_deleted() && static_cast<bool>(peer_id)) {
            directory_view->read_key(*peer_id,
                [&](const cluster_directory_metadata_t *md) {
                    if (md == nullptr) {
                        /* Race condition; the server appeared in `name_client`'s list of
                        servers but hasn't appeared in the directory yet. */
                        return;
                    }
                    debugf("make server ghost issue\n");
                    ghost_list.push_back(server_ghost_issue_t(
                        pair.first, md->hostname, md->pid));
                });
        }
    }
    down_issues.set_value(down_list);
    ghost_issues.set_value(ghost_list);
}

void server_issue_tracker_t::combine(
        local_issues_t *local_issues,
        std::vector<scoped_ptr_t<issue_t> > *issues_out) {
    // Combine down issues
    {
        std::map<server_id_t, server_down_issue_t*> combined_down_issues;
        for (auto &down_issue : local_issues->server_down_issues) {
            auto combined_it = combined_down_issues.find(down_issue.down_server_id);
            if (combined_it == combined_down_issues.end()) {
                combined_down_issues.insert(std::make_pair(
                    down_issue.down_server_id,
                    &down_issue));
            } else {
                rassert(down_issue.affected_server_ids.size() == 1);
                combined_it->second->add_server(down_issue.affected_server_ids[0]);
            }
        }

        for (auto const &it : combined_down_issues) {
            issues_out->push_back(scoped_ptr_t<issue_t>(
                new server_down_issue_t(*it.second)));
        }
    }

    // Combine ghost issues
    {
        std::set<server_id_t> ghost_issues_seen;
        for (auto &ghost_issue : local_issues->server_ghost_issues) {
            /* This is trivial, since we don't track affected servers for ghost issues.
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
