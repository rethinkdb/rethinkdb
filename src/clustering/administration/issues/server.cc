// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/issues/server.hpp"
#include "clustering/administration/servers/machine_id_to_peer_id.hpp"
#include "clustering/administration/datum_adapter.hpp"

const datum_string_t server_down_issue_t::server_down_issue_type =
    datum_string_t("server_down");
const uuid_u server_down_issue_t::base_issue_id =
    str_to_uuid("377a1d56-db29-416d-97f7-4ce1efc6e97b");

const datum_string_t server_ghost_issue_t::server_ghost_issue_type =
    datum_string_t("server_ghost");
const uuid_u server_ghost_issue_t::base_issue_id =
    str_to_uuid("193df26a-eac7-4373-bf0a-12bbc0b869ed");

ql::datum_t build_server_issue_info(
        const std::string &server_name,
        const machine_id_t &server_id,
        const std::vector<std::string> &affected_server_names,
        const std::vector<machine_id_t> &affected_server_ids) {
    ql::datum_object_builder_t builder;
    ql::datum_array_builder_t server_array(ql::configured_limits_t::unlimited);
    ql::datum_array_builder_t server_id_array(ql::configured_limits_t::unlimited);

    for (auto s : affected_server_names) {
        server_array.add(convert_string_to_datum(s));
    }
    for (auto s : affected_server_ids) {
        server_id_array.add(convert_uuid_to_datum(s));
    }

    builder.overwrite("server", convert_string_to_datum(server_name));
    builder.overwrite("server_id", convert_uuid_to_datum(server_id));
    builder.overwrite("affected_servers", std::move(server_array).to_datum());
    builder.overwrite("affected_server_ids", std::move(server_id_array).to_datum());

    return std::move(builder).to_datum();
}

std::vector<std::string> look_up_servers(const issue_t::metadata_t &metadata,
                                         const std::vector<machine_id_t> &servers) {
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

server_down_issue_t::server_down_issue_t(const machine_id_t &_down_server_id) :
    local_issue_t(from_hash(base_issue_id, _down_server_id)),
    down_server_id(_down_server_id) { }

ql::datum_t server_down_issue_t::build_info(const metadata_t &metadata) const {
    const std::string name = get_server_name(metadata, down_server_id);
    const std::vector<std::string> affected_server_names =
        look_up_servers(metadata, affected_server_ids);
    return build_server_issue_info(name,
                                   down_server_id,
                                   affected_server_names,
                                   affected_server_ids);
}

datum_string_t server_down_issue_t::build_description(const ql::datum_t &info) const {
    return datum_string_t(strprintf(
        "Server '%s' is inaccessible from %s%s.",
        info.get_field("server").as_str().to_std().c_str(),
        affected_server_ids.size() == 1 ? "" : "these servers: ",
        servers_to_string(info.get_field("affected_servers")).c_str()));
}

server_ghost_issue_t::server_ghost_issue_t() { }

server_ghost_issue_t::server_ghost_issue_t(const machine_id_t &_ghost_server_id) :
    local_issue_t(from_hash(base_issue_id, _ghost_server_id)),
    ghost_server_id(_ghost_server_id) { }

ql::datum_t server_ghost_issue_t::build_info(const metadata_t &metadata) const {
    const std::string name = get_server_name(metadata, ghost_server_id);
    const std::vector<std::string> affected_server_names =
        look_up_servers(metadata, affected_server_ids);
    return build_server_issue_info(name,
                                   ghost_server_id,
                                   affected_server_names,
                                   affected_server_ids);
}

datum_string_t server_ghost_issue_t::build_description(const ql::datum_t &info) const {
    return datum_string_t(strprintf(
        "Server '%s' was declared dead, but is still connected to %s%s.",
        info.get_field("server").as_str().to_std().c_str(),
        affected_server_ids.size() == 1 ? "" : "these servers: ",
        servers_to_string(info.get_field("affected_servers")).c_str()));
}

server_issue_tracker_t::server_issue_tracker_t(
            local_issue_aggregator_t *parent,
            boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
                _cluster_sl_view,
            const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, machine_id_t> > >
                &_machine_to_peer) :
        down_issues(std::vector<server_down_issue_t>()),
        ghost_issues(std::vector<server_ghost_issue_t>()),
        down_subs(parent, down_issues.get_watchable(), &local_issues_t::server_down_issues),
        ghost_subs(parent, ghost_issues.get_watchable(), &local_issues_t::server_ghost_issues),
        cluster_sl_view(_cluster_sl_view),
        cluster_sl_subs(std::bind(&server_issue_tracker_t::recompute, this), cluster_sl_view),
        machine_to_peer(_machine_to_peer),
        machine_to_peer_subs(std::bind(&server_issue_tracker_t::recompute, this)) {
    watchable_t<change_tracking_map_t<peer_id_t, machine_id_t> >::freeze_t
        freeze(machine_to_peer);
    machine_to_peer_subs.reset(machine_to_peer, &freeze);
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
    std::vector<machine_id_t> down_servers;
    std::vector<machine_id_t> ghost_servers;

    cluster_semilattice_metadata_t metadata = cluster_sl_view->get();
    for (auto const &machine : metadata.machines.machines) {
        peer_id_t peer = machine_id_to_peer_id(machine.first,
                                               machine_to_peer->get().get_inner());
        if (!machine.second.is_deleted() && peer.is_nil()) {
            down_servers.push_back(machine.first);
        } else if (machine.second.is_deleted() && !peer.is_nil()) {
            ghost_servers.push_back(machine.first);
        }
    }

    down_issues.apply_atomic_op(
        [&] (std::vector<server_down_issue_t> *issues) -> bool {
            issues->clear();
            for (auto const &server : down_servers) {
                issues->push_back(server_down_issue_t(server));
            }
            return true;
        });
    ghost_issues.apply_atomic_op(
        [&] (std::vector<server_ghost_issue_t> *issues) -> bool {
            issues->clear();
            for (auto const &server : ghost_servers) {
                issues->push_back(server_ghost_issue_t(server));
            }
            return true;
        });
}

void server_issue_tracker_t::combine(
        local_issues_t *local_issues,
        std::vector<scoped_ptr_t<issue_t> > *issues_out) {
    // Combine down issues
    {
        std::map<machine_id_t, server_down_issue_t*> combined_down_issues;
        for (auto &down_issue : local_issues->server_down_issues) {
            auto combined_it = combined_down_issues.find(down_issue.down_server_id);
            if (combined_it == combined_down_issues.end()) {
                combined_down_issues.insert(std::make_pair(
                    down_issue.down_server_id,
                    &down_issue));
            } else {
                rassert(down_issue.affected_server_ids.size() == 0);
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
        std::map<machine_id_t, server_ghost_issue_t*> combined_ghost_issues;
        for (auto &ghost_issue : local_issues->server_ghost_issues) {
            auto combined_it = combined_ghost_issues.find(ghost_issue.ghost_server_id);
            if (combined_it == combined_ghost_issues.end()) {
                combined_ghost_issues.insert(std::make_pair(
                    ghost_issue.ghost_server_id,
                    &ghost_issue));
            } else {
                rassert(ghost_issue.affected_server_ids.size() == 0);
                combined_it->second->add_server(ghost_issue.affected_server_ids[0]);
            }
        }


        for (auto const &it : combined_ghost_issues) {
            issues_out->push_back(scoped_ptr_t<issue_t>(
                new server_ghost_issue_t(*it.second)));
        }
    }

}
