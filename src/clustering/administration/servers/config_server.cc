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
    actual_cache_size_bytes(0),
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
        update_actual_cache_size(it->second.get_ref().cache_size_bytes.get_ref());
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
        boost::optional<uint64_t> new_cache_size,
        mailbox_t<void(std::string)>::address_t ack_addr) {
    servers_semilattice_metadata_t metadata = semilattice_view->get();
    deletable_t<server_semilattice_metadata_t> *entry =
        &metadata.servers.at(my_server_id);
    if (!permanently_removed_cond.is_pulsed() && !entry->is_deleted() &&
            new_cache_size != entry->get_ref().cache_size_bytes.get_ref()) {
        std::string error;
        if (static_cast<bool>(new_cache_size) &&
                *new_cache_size > get_max_total_cache_size()) {
            send(mailbox_manager, ack_addr,
                strprintf("The proposed cache size of %" PRIu64 " MB is larger than the "
                    "maximum legal value for this platform (%" PRIu64 " MB).",
                    *new_cache_size, get_max_total_cache_size()));
            return;
        }
        update_actual_cache_size(new_cache_size);
        entry->get_mutable()->cache_size_bytes.set(new_cache_size);
        semilattice_view->join(metadata);
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

void server_config_server_t::update_actual_cache_size(
        const boost::optional<uint64_t> &setting) {
    uint64_t actual_size;
    if (!static_cast<bool>(setting)) {
        actual_size = get_default_total_cache_size();
        logINF("Automatically using cache size of %" PRIu64 " MB",
            actual_size / static_cast<uint64_t>(MEGABYTE));
    } else {
        if (*setting > get_max_total_cache_size()) {
            /* Usually this won't happen, because something else will reject the value
            before it gets to here. However, this can happen if a large cache size is
            recorded on disk and then the same data files are opened on a platform with a
            lower limit. I'm not sure if it's currently possible to open 64-bit metadata
            files on 32-bit platform, but better safe than sorry. */
            logWRN("The cache size is set to %" PRIu64 " MB, which is larger than the "
                "maximum legal value for this platform (%" PRIu64 " MB). The maximum "
                "legal value will be used as the actual cache size.",
                *setting, get_max_total_cache_size());
            actual_size = get_max_total_cache_size();
        } else {
            actual_size = *setting;
            logINF("Cache size is set to %" PRIu64 " MB",
                actual_size / static_cast<uint64_t>(MEGABYTE));
        }
    }
    log_warnings_for_cache_size(actual_size);
    actual_cache_size_bytes.set_value(actual_size);
}

