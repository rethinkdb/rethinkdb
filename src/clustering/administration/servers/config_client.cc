// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/servers/config_client.hpp"

server_config_client_t::server_config_client_t(
        mailbox_manager_t *_mailbox_manager,
        clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t,
            cluster_directory_metadata_t> > > _directory_view,
        boost::shared_ptr<semilattice_readwrite_view_t<servers_semilattice_metadata_t> >
            _semilattice_view) :
    mailbox_manager(_mailbox_manager),
    directory_view(_directory_view),
    semilattice_view(_semilattice_view),
    server_id_to_peer_id_map(std::map<server_id_t, peer_id_t>()),
    peer_id_to_server_id_map(std::map<peer_id_t, server_id_t>()),
    name_to_server_id_map(std::multimap<name_string_t, server_id_t>()),
    server_id_to_name_map(std::map<server_id_t, name_string_t>()),
    directory_subs([this]() {
        this->recompute_server_id_to_peer_id_map();
        }),
    semilattice_subs([this]() {
        this->recompute_server_id_to_peer_id_map();
        this->recompute_name_to_server_id_map();
        })
{
    watchable_t< change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> >
        ::freeze_t freeze(directory_view);
    directory_subs.reset(directory_view, &freeze);
    semilattice_subs.reset(semilattice_view);
    recompute_server_id_to_peer_id_map();
    recompute_name_to_server_id_map();
}

std::set<server_id_t> server_config_client_t::get_servers_with_tag(
        const name_string_t &tag) {
    std::set<server_id_t> servers;
    servers_semilattice_metadata_t md = semilattice_view->get();
    for (auto it = md.servers.begin();
              it != md.servers.end();
            ++it) {
        if (it->second.is_deleted()) {
            continue;
        }
        if (it->second.get_ref().tags.get_ref().count(tag) == 1) {
            servers.insert(it->first);
        }
    }
    return servers;
}

bool server_config_client_t::change_server_name(
        const server_id_t &server_id,
        const name_string_t &server_name,
        const name_string_t &new_name,
        signal_t *interruptor,
        std::string *error_out) {
    this->assert_thread();

    if (server_name == new_name) {
        /* This is a no-op */
        return true;
    }

    bool new_name_in_use = false;
    name_to_server_id_map.apply_read(
        [&](const std::multimap<name_string_t, server_id_t> *map) {
            new_name_in_use = (map->count(new_name) != 0);
        });
    if (new_name_in_use) {
        *error_out = strprintf("Cannot rename server `%s` to `%s` because server `%s` "
            "already exists.", server_name.c_str(), new_name.c_str(), new_name.c_str());
        return false;
    }

    return do_change(
        server_id,
        server_name,
        "name",
        [&](const server_config_business_card_t &bc, 
                const mailbox_t<void(std::string)>::address_t &reply_addr) {
            send(mailbox_manager, bc.change_name_addr, new_name, reply_addr);
        },
        interruptor,
        error_out);
}

bool server_config_client_t::change_server_tags(
        const server_id_t &server_id,
        const name_string_t &server_name,
        const std::set<name_string_t> &new_tags,
        signal_t *interruptor,
        std::string *error_out) {
    return do_change(
        server_id,
        server_name,
        "tags",
        [&](const server_config_business_card_t &bc, 
                const mailbox_t<void(std::string)>::address_t &reply_addr) {
            send(mailbox_manager, bc.change_tags_addr, new_tags, reply_addr);
        },
        interruptor,
        error_out);
}

bool server_config_client_t::change_server_cache_size(
        const server_id_t &server_id,
        const name_string_t &server_name,   /* for error messages */
        const boost::optional<uint64_t> &new_cache_size_bytes,
        signal_t *interruptor,
        std::string *error_out) {
    return do_change(
        server_id,
        server_name,
        "cache size",
        [&](const server_config_business_card_t &bc, 
                const mailbox_t<void(std::string)>::address_t &reply_addr) {
            send(mailbox_manager, bc.change_cache_size_addr, new_cache_size_bytes,
                 reply_addr);
        },
        interruptor,
        error_out);
}

bool server_config_client_t::permanently_remove_server(const name_string_t &name,
        std::string *error_out) {
    assert_thread();

    server_id_t server_id = nil_uuid();
    bool name_collision = false;
    name_to_server_id_map.apply_read(
        [&](const std::multimap<name_string_t, server_id_t> *map) {
            if (map->count(name) > 1) {
                name_collision = true;
            } else {
                auto it = map->find(name);
                if (it != map->end()) {
                    server_id = it->second;
                }
            }
        });
    if (name_collision) {
        *error_out = strprintf("There are multiple servers called `%s`.", name.c_str());
        return false;
    }
    if (server_id == nil_uuid()) {
        *error_out = strprintf("Server `%s` does not exist.", name.c_str());
        return false;
    }
    
    bool is_visible;
    server_id_to_peer_id_map.apply_read(
        [&](const std::map<server_id_t, peer_id_t> *map) {
            is_visible = (map->count(server_id) != 0);
        });
    if (is_visible) {
        *error_out = strprintf("Server `%s` is currently connected to the cluster, so "
            "it should not be permanently removed.", name.c_str());
        return false;
    }

    servers_semilattice_metadata_t sl_metadata = semilattice_view->get();
    sl_metadata.servers.at(server_id).mark_deleted();
    semilattice_view->join(sl_metadata);
    return true;
}

bool server_config_client_t::do_change(
        const server_id_t &server_id,
        const name_string_t &server_name,
        const std::string &what_is_changing,
        const std::function<void(
            const server_config_business_card_t &,
            const mailbox_t<void(std::string)>::address_t &
            )> &sender,
        signal_t *interruptor,
        std::string *error_out) {

    /* We can produce this error message for several different reasons, so it's stored in
    a local variable instead of typed out multiple times. */
    std::string lost_contact_message = strprintf(
        "Cannot change the %s of server `%s` because we lost contact with it.",
        what_is_changing.c_str(), server_name.c_str());

    peer_id_t peer;
    server_id_to_peer_id_map.apply_read(
        [&](const std::map<server_id_t, peer_id_t> *map) {
            auto it = map->find(server_id);
            if (it != map->end()) {
                peer = it->second;
            }
        });
    if (peer.is_nil()) {
        *error_out = lost_contact_message;
        return false;
    }

    server_config_business_card_t bcard;
    bool found;
    directory_view->apply_read(
        [&](const change_tracking_map_t<peer_id_t, cluster_directory_metadata_t>
                *dir_metadata) {
            auto it = dir_metadata->get_inner().find(peer);
            if (it != dir_metadata->get_inner().end()) {
                guarantee(it->second.server_config_business_card, "We shouldn't be "
                    "trying to change configuration on a proxy.");
                bcard = it->second.server_config_business_card.get();
                found = true;
            } else {
                found = false;
            }
        });
    if (!found) {
        *error_out = lost_contact_message;
        return false;
    }

    promise_t<std::string> got_reply;
    mailbox_t<void(std::string)> ack_mailbox(
        mailbox_manager,
        [&](UNUSED signal_t *interruptor2, const std::string &msg) {
            got_reply.pulse(msg);
        });
    disconnect_watcher_t disconnect_watcher(
        mailbox_manager, bcard.change_tags_addr.get_peer());

    sender(bcard, ack_mailbox.get_address());

    wait_any_t waiter(got_reply.get_ready_signal(), &disconnect_watcher);
    wait_interruptible(&waiter, interruptor);

    if (!got_reply.get_ready_signal()->is_pulsed()) {
        *error_out = lost_contact_message;
        return false;
    }

    if (!got_reply.wait().empty()) {
        *error_out = strprintf("Error when trying to change the %s of server `%s`: %s",
            what_is_changing.c_str(), server_name.c_str(), got_reply.wait().c_str());
        return false;
    }

    try {
        /* Don't return until the change is visible in the semilattices */
        semilattice_view->sync_from(peer, interruptor);
    } catch (const sync_failed_exc_t &) {
        *error_out = lost_contact_message;
        return false;
    }

    return true;
}

void server_config_client_t::recompute_name_to_server_id_map() {
    std::multimap<name_string_t, server_id_t> new_map_n2m;
    std::map<server_id_t, name_string_t> new_map_m2n;
    servers_semilattice_metadata_t sl_metadata = semilattice_view->get();
    for (auto sl_it = sl_metadata.servers.begin();
              sl_it != sl_metadata.servers.end();
            ++sl_it) {
        if (sl_it->second.is_deleted()) {
            continue;
        }
        name_string_t name = sl_it->second.get_ref().name.get_ref();
        new_map_n2m.insert(std::make_pair(name, sl_it->first));
        auto res2 = new_map_m2n.insert(std::make_pair(sl_it->first, name));
        if (!res2.second) {
            crash("Two servers have the same server ID.");
        }
    }
    name_to_server_id_map.set_value(new_map_n2m);
    server_id_to_name_map.set_value(new_map_m2n);
}

void server_config_client_t::recompute_server_id_to_peer_id_map() {
    std::map<server_id_t, peer_id_t> new_map_s2p;
    std::map<peer_id_t, server_id_t> new_map_p2s;
    directory_view->apply_read(
        [&](const change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> *dir) {
            for (auto dir_it = dir->get_inner().begin();
                      dir_it != dir->get_inner().end();
                    ++dir_it) {
                new_map_s2p.insert(std::make_pair(
                    dir_it->second.server_id, dir_it->first));
                new_map_p2s.insert(std::make_pair(
                    dir_it->first, dir_it->second.server_id));
            }
    });
    server_id_to_peer_id_map.set_value(new_map_s2p);
    peer_id_to_server_id_map.set_value(new_map_p2s);
}

