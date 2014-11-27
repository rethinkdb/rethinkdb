// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/servers/config_server.hpp"

#include "clustering/administration/main/cache_size.hpp"

server_config_server_t::server_config_server_t(
        mailbox_manager_t *_mailbox_manager,
        server_id_t _my_server_id,
        clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t,
            cluster_directory_metadata_t> > > _directory_view,
        boost::shared_ptr<semilattice_readwrite_view_t<servers_semilattice_metadata_t> >
            _semilattice_view) :
    mailbox_manager(_mailbox_manager),
    my_server_id(_my_server_id),
    my_name(name_string_t()),
    my_tags(std::set<name_string_t>()),
    cache_size_bytes(0),
    directory_view(_directory_view),
    semilattice_view(_semilattice_view),
    change_name_mailbox(mailbox_manager,
        std::bind(&server_config_server_t::on_change_name_request, this,
            ph::_1, ph::_2, ph::_3)),
    change_tags_mailbox(mailbox_manager,
        std::bind(&server_config_server_t::on_change_tags_request, this,
            ph::_1, ph::_2, ph::_3)),
    change_cache_size_mailbox(mailbox_manager,
        std::bind(&server_config_server_t::on_change_cache_size_request, this,
            ph::_1, ph::_2, ph::_3)),
    semilattice_subs([this]() {
        on_semilattice_change();
        })
{
    /* Find our entry in the servers semilattice map and determine our current name from
    it. */
    servers_semilattice_metadata_t sl_metadata = semilattice_view->get();
    auto it = sl_metadata.servers.find(my_server_id);
    guarantee(it != sl_metadata.servers.end(), "We should already have an entry in the "
        "semilattices");
    if (it->second.is_deleted()) {
        permanently_removed_cond.pulse();
    } else {
        my_name.set_value(it->second.get_ref().name.get_ref());
        my_tags.set_value(it->second.get_ref().tags.get_ref());
        cache_size_bytes.set_value(it->second.get_ref().cache_size_bytes.get_ref());

        std::string error;
        if (!validate_total_cache_size(cache_size_bytes.get(), &error)) {
            /* This is unlikely, but it could happen if the system parameters changed
            while the server was shut down (e.g. if we moved from a 64-bit system to a
            32-bit system). I'm not sure if that's actually possible, but let's play it
            safe. */
            logERR("%s", error.c_str());
            cache_size_bytes.set_value(get_default_total_cache_size());
        }
        log_total_cache_size(cache_size_bytes.get());
    }

    semilattice_subs.reset(semilattice_view);
}

server_config_business_card_t server_config_server_t::get_business_card() {
    server_config_business_card_t bcard;
    bcard.change_name_addr = change_name_mailbox.get_address();
    bcard.change_tags_addr = change_tags_mailbox.get_address();
    bcard.change_cache_size_addr = change_cache_size_mailbox.get_address();
    return bcard;
}

void server_config_server_t::on_change_name_request(
        UNUSED signal_t *interruptor,
        const name_string_t &new_name,
        mailbox_t<void(std::string)>::address_t ack_addr) {
    if (!permanently_removed_cond.is_pulsed() && new_name != my_name.get()) {
        logINF("Changed server's name from `%s` to `%s`.",
            my_name.get().c_str(), new_name.c_str());
        my_name.set_value(new_name);
        servers_semilattice_metadata_t metadata = semilattice_view->get();
        deletable_t<server_semilattice_metadata_t> *entry =
            &metadata.servers.at(my_server_id);
        if (!entry->is_deleted()) {
            entry->get_mutable()->name.set(new_name);
            semilattice_view->join(metadata);
        }
    }

    /* Send an acknowledgement to the server that initiated the request */
    send(mailbox_manager, ack_addr, std::string());
}

void server_config_server_t::on_change_tags_request(
        UNUSED signal_t *interruptor,
        const std::set<name_string_t> &new_tags,
        mailbox_t<void(std::string)>::address_t ack_addr) {
    if (!permanently_removed_cond.is_pulsed() && new_tags != my_tags.get()) {
        std::string tag_str;
        if (new_tags.empty()) {
            tag_str = "(nothing)";
        } else {
            for (const name_string_t &tag : new_tags) {
                if (!tag_str.empty()) {
                    tag_str += ", ";
                }
                tag_str += tag.str();
            }
        }
        logINF("Changed server's tags to: %s", tag_str.c_str());
        my_tags.set_value(new_tags);
        servers_semilattice_metadata_t metadata = semilattice_view->get();
        deletable_t<server_semilattice_metadata_t> *entry =
            &metadata.servers.at(my_server_id);
        if (!entry->is_deleted()) {
            entry->get_mutable()->tags.set(new_tags);
            semilattice_view->join(metadata);
        }
    }

    /* Send an acknowledgement to the server that initiated the request */
    send(mailbox_manager, ack_addr, std::string());
}

void server_config_server_t::on_change_cache_size_request(
        UNUSED signal_t *interruptor,
        uint64_t new_cache_size,
        mailbox_t<void(std::string)>::address_t ack_addr) {
    if (!permanently_removed_cond.is_pulsed() &&
            new_cache_size != cache_size_bytes.get()) {
        std::string error;
        if (!validate_total_cache_size(new_cache_size, &error)) {
            send(mailbox_manager, ack_addr, error);
            return;
        }
        log_total_cache_size(new_cache_size);
        cache_size_bytes.set_value(new_cache_size);
        servers_semilattice_metadata_t metadata = semilattice_view->get();
        deletable_t<server_semilattice_metadata_t> *entry =
            &metadata.servers.at(my_server_id);
        if (!entry->is_deleted()) {
            entry->get_mutable()->cache_size_bytes.set(new_cache_size);
            semilattice_view->join(metadata);
        }
    }
    send(mailbox_manager, ack_addr, std::string());
}

void server_config_server_t::on_semilattice_change() {
    ASSERT_FINITE_CORO_WAITING;
    servers_semilattice_metadata_t sl_metadata = semilattice_view->get();
    guarantee(sl_metadata.servers.count(my_server_id) == 1);

    /* Check if we've been permanently removed */
    if (sl_metadata.servers.at(my_server_id).is_deleted()) {
        if (!permanently_removed_cond.is_pulsed()) {
            logERR("This server has been permanently removed from the cluster. Please "
                "stop the process, erase its data files, and start a fresh RethinkDB "
                "instance.");
            my_name.set_value(name_string_t());
            permanently_removed_cond.pulse();
        }
    }
}

