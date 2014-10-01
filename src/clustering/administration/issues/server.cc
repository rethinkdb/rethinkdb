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
    for (auto server : servers) {
        res.push_back(issue_t::get_server_name(metadata, server));
    }
    return res;
}

std::string servers_to_string(const ql::datum_t &server_names) {
    std::string res;
    for (size_t i = 0; i < server_names.arr_size(); ++i) {
        res.append(strprintf("%s%s",
                             res.empty() ? "" : ", ",
                             server_names.get(i).as_str().to_std().c_str()));
    }
    return res;
}

server_down_issue_t::server_down_issue_t(const machine_id_t &_down_server_id,
                                         const std::vector<machine_id_t> &_affected_server_ids) :
    issue_t(from_hash(base_issue_id, _down_server_id)),
    down_server_id(_down_server_id),
    affected_server_ids(_affected_server_ids) { }

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
        "Server %s is inaccessible from %s%s.",
        info.get_field("server").as_str().to_std().c_str(),
        affected_server_ids.size() == 1 ? "" : "these servers: ",
        servers_to_string(info.get_field("server_ids")).c_str()));
}

server_ghost_issue_t::server_ghost_issue_t(const machine_id_t &_ghost_server_id,
                                           const std::vector<machine_id_t> &_affected_server_ids) :
    issue_t(from_hash(base_issue_id, _ghost_server_id)),
    ghost_server_id(_ghost_server_id),
    affected_server_ids(_affected_server_ids) { }

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
        "Server %s was declared dead, but is still connected to %s%s.",
        info.get_field("server").as_str().to_std().c_str(),
        affected_server_ids.size() == 1 ? "" : "these servers: ",
        servers_to_string(info.get_field("server_ids")).c_str()));
}

server_issue_tracker_t::server_issue_tracker_t(
            local_issue_aggregator_t *_parent,
            boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
                _cluster_sl_view,
            const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t, machine_id_t> > >
                &_machine_to_peer) :
        local_issue_tracker_t(_parent),
        cluster_sl_view(_cluster_sl_view),
        machine_to_peer(_machine_to_peer),
        machine_to_peer_subs(std::bind(&server_issue_tracker_t::recompute, this)) {
    watchable_t<change_tracking_map_t<peer_id_t, machine_id_t> >::freeze_t
        freeze(machine_to_peer);
    machine_to_peer_subs.reset(machine_to_peer, &freeze);
    recompute();   
}

server_issue_tracker_t::~server_issue_tracker_t() { }

void server_issue_tracker_t::recompute() {
    std::vector<local_issue_t> res;
    cluster_semilattice_metadata_t metadata = cluster_sl_view->get();
    for (auto const &machine : metadata.machines.machines) {
        peer_id_t peer = machine_id_to_peer_id(machine.first, machine_to_peer->get().get_inner());
        if (!machine.second.is_deleted() && peer.is_nil()) {
            res.push_back(local_issue_t::make_server_down_issue(machine.first));
        } else if (machine.second.is_deleted() && !peer.is_nil()) {
            res.push_back(local_issue_t::make_server_ghost_issue(machine.first));
        }
    }
    update(res);
}
