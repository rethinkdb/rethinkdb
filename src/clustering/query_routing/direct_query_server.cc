// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/query_routing/direct_query_server.hpp"

#include "protocol_api.hpp"
#include "store_view.hpp"

direct_query_server_t::direct_query_server_t(
        mailbox_manager_t *mm,
        store_view_t *svs_) :
    mailbox_manager(mm),
    svs(svs_),
    read_mailbox(mm, std::bind(&direct_query_server_t::on_read, this,
                               ph::_1, ph::_2, ph::_3))
    { }

direct_query_bcard_t direct_query_server_t::get_bcard() {
    return direct_query_bcard_t(read_mailbox.get_address());
}

void direct_query_server_t::on_read(
        signal_t *interruptor,
        const read_t &read,
        const mailbox_addr_t<void(read_response_t)> &cont) {

    try {
        /* Leave the token empty. We're not actually interested in ordering here. */
        read_token_t token;

#ifndef NDEBUG
        trivial_metainfo_checker_callback_t metainfo_checker_callback;
        metainfo_checker_t metainfo_checker(&metainfo_checker_callback, svs->get_region());
#endif

        read_response_t response;
        svs->read(DEBUG_ONLY(metainfo_checker, )
                  read,
                  &response,
                  &token,
                  interruptor);
        send(mailbox_manager, cont, response);
    } catch (const interrupted_exc_t &) {
        /* ignore */
    }
}
