// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rpc/connectivity/connectivity.hpp"

RDB_IMPL_ME_SERIALIZABLE_1_SINCE_v1_13(peer_id_t, uuid);

// Universal serialization functions: you MUST NOT change their implementations.
// (You could find a way to remove these functions, though.)
void serialize_universal(write_message_t *wm, const peer_id_t &peer_id) {
    serialize_universal(wm, peer_id.get_uuid());
}
archive_result_t deserialize_universal(read_stream_t *s, peer_id_t *peer_id) {
    uuid_u uuid;
    archive_result_t res = deserialize_universal(s, &uuid);
    if (bad(res)) { return res; }
    *peer_id = peer_id_t(uuid);
    return archive_result_t::SUCCESS;
}

connectivity_service_t::peers_list_freeze_t::peers_list_freeze_t(connectivity_service_t *connectivity) :
    acq(connectivity->get_peers_list_lock()) { }

void connectivity_service_t::peers_list_freeze_t::assert_is_holding(connectivity_service_t *connectivity) {
    acq.assert_is_holding(connectivity->get_peers_list_lock());
}

connectivity_service_t::peers_list_subscription_t::peers_list_subscription_t(peers_list_callback_t *connect_disconnect_cb)
     : subs(connect_disconnect_cb) {
    guarantee(connect_disconnect_cb != NULL);
}

void connectivity_service_t::peers_list_subscription_t::reset() {
    subs.reset();
}

void connectivity_service_t::peers_list_subscription_t::reset(connectivity_service_t *connectivity, peers_list_freeze_t *proof) {
    proof->assert_is_holding(connectivity);
    subs.reset(connectivity->get_peers_list_publisher());
}

disconnect_watcher_t::disconnect_watcher_t(connectivity_service_t *connectivity, peer_id_t p)
    : subs(this), peer(p) {
    ASSERT_FINITE_CORO_WAITING;
    connectivity_service_t::peers_list_freeze_t freeze(connectivity);
    if (!connectivity->get_peer_connected(peer)) {
        pulse();
    } else {
        subs.reset(connectivity, &freeze);
    }
}

void disconnect_watcher_t::on_disconnect(peer_id_t p) {
    if (peer == p) {
        if (!is_pulsed()) {
            pulse();
        }
    }
}


void debug_print(printf_buffer_t *buf, const peer_id_t &peer_id) {
    debug_print(buf, peer_id.get_uuid());
}
