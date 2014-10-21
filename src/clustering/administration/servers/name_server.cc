// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/servers/name_server.hpp"

server_name_server_t::server_name_server_t(
        mailbox_manager_t *_mailbox_manager,
        machine_id_t _my_machine_id,
        clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t,
            cluster_directory_metadata_t> > > _directory_view,
        boost::shared_ptr<semilattice_readwrite_view_t<machines_semilattice_metadata_t> >
            _semilattice_view) :
    mailbox_manager(_mailbox_manager),
    my_machine_id(_my_machine_id),
    directory_view(_directory_view),
    semilattice_view(_semilattice_view),
    rename_mailbox(mailbox_manager,
        std::bind(&server_name_server_t::on_rename_request, this,
            ph::_1, ph::_2, ph::_3)),
    retag_mailbox(mailbox_manager,
        std::bind(&server_name_server_t::on_retag_request, this,
            ph::_1, ph::_2, ph::_3)),
    semilattice_subs([this]() {
        on_semilattice_change();
        })
{
    /* Find our entry in the machines semilattice map and determine our current name from
    it. */
    machines_semilattice_metadata_t sl_metadata = semilattice_view->get();
    auto it = sl_metadata.machines.find(my_machine_id);
    guarantee(it != sl_metadata.machines.end(), "We should already have an entry in the "
        "semilattices");
    if (it->second.is_deleted()) {
        permanently_removed_cond.pulse();
    } else {
        my_name = it->second.get_ref().name.get_ref();
    }

    semilattice_subs.reset(semilattice_view);
}

server_name_business_card_t server_name_server_t::get_business_card() {
    server_name_business_card_t bcard;
    bcard.rename_addr = rename_mailbox.get_address();
    bcard.retag_addr = retag_mailbox.get_address();
    return bcard;
}

void server_name_server_t::on_rename_request(UNUSED signal_t *interruptor,
                                             const name_string_t &new_name,
                                             mailbox_t<void()>::address_t ack_addr) {
    if (!permanently_removed_cond.is_pulsed() && new_name != my_name) {
        logINF("Changed server's name from `%s` to `%s`.",
            my_name.c_str(), new_name.c_str());
        my_name = new_name;
        machines_semilattice_metadata_t metadata = semilattice_view->get();
        deletable_t<machine_semilattice_metadata_t> *entry =
            &metadata.machines.at(my_machine_id);
        if (!entry->is_deleted()) {
            entry->get_mutable()->name.set(new_name);
            semilattice_view->join(metadata);
        }
    }

    /* Send an acknowledgement to the server that initiated the request */
    send(mailbox_manager, ack_addr);
}

void server_name_server_t::on_retag_request(UNUSED signal_t *interruptor,
                                            const std::set<name_string_t> &new_tags,
                                            mailbox_t<void()>::address_t ack_addr) {
    if (!permanently_removed_cond.is_pulsed() && new_tags != my_tags) {
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
        my_tags = new_tags;
        machines_semilattice_metadata_t metadata = semilattice_view->get();
        deletable_t<machine_semilattice_metadata_t> *entry =
            &metadata.machines.at(my_machine_id);
        if (!entry->is_deleted()) {
            entry->get_mutable()->tags.set(new_tags);
            semilattice_view->join(metadata);
        }
    }

    /* Send an acknowledgement to the server that initiated the request */
    send(mailbox_manager, ack_addr);
}

void server_name_server_t::on_semilattice_change() {
    ASSERT_FINITE_CORO_WAITING;
    machines_semilattice_metadata_t sl_metadata = semilattice_view->get();
    guarantee(sl_metadata.machines.count(my_machine_id) == 1);

    /* Check if we've been permanently removed */
    if (sl_metadata.machines.at(my_machine_id).is_deleted()) {
        if (!permanently_removed_cond.is_pulsed()) {
            logERR("This server has been permanently removed from the cluster. Please "
                "stop the process, erase its data files, and start a fresh RethinkDB "
                "instance.");
            my_name = name_string_t();
            permanently_removed_cond.pulse();
        }
    }
}

