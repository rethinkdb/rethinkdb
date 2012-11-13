// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "rpc/connectivity/heartbeat.hpp"
#include "arch/io/arch.hpp"
#include "arch/runtime/runtime_utils.hpp"

// This class implements a heartbeat on top of intra-cluster connections 
heartbeat_manager_t::heartbeat_manager_t(message_service_t *_message_service) :
    message_service(_message_service),
    connection_watcher(this) {
    connectivity_service_t::peers_list_freeze_t freeze(message_service->get_connectivity_service());
    guarantee(message_service->get_connectivity_service()->get_peers_list().empty());
    connection_watcher.reset(message_service->get_connectivity_service(), &freeze);
}

heartbeat_manager_t::~heartbeat_manager_t() {
    // Assert empty?
}

// Start heartbeat on a new peer connection
void heartbeat_manager_t::on_connect(peer_id_t peer_id) {
    assert_thread();
    guarantee(connections.insert(std::make_pair(peer_id, 0)).second == true);

    if (timer_token == NULL) {
        // First connection on this thread, set up a timer
        rassert(connections.size() == 1);
        timer_token = add_timer(TIMER_INTERVAL_MS, &timer_callback, this);
    }
}

// Peer connection has gone down, stop heartbeat for it
void heartbeat_manager_t::on_disconnect(peer_id_t peer_id) {
    assert_thread();
    guarantee(connections.erase(peer_id) == 1);
    
    rassert(timer_token != NULL);
    if (connections.empty()) {
        cancel_timer(timer_token);
        timer_token = NULL;
    }
}

void heartbeat_manager_t::timer_callback(void *ctx) {
    // TODO: make sure we don't kill a reconnection of the one we're trying to kill
    //  not very likely, but could cause a momentary thrash
    ASSERT_FINITE_CORO_WAITING;
    heartbeat_manager_t *self = reinterpret_cast<heartbeat_manager_t*>(ctx);
    self->assert_thread();
    for (std::map<peer_id_t, uint32_t>::iterator i = self->connections.begin();
         i != self->connections.end(); ++i) {
        if (i->second >= MAX_KEEPALIVES) {
            coro_t::spawn_later_ordered(boost::bind(&message_service_t::kill_connection, self->message_service, i->first));
        } else {
            coro_t::spawn_later_ordered(boost::bind(&message_service_t::send_message, self->message_service, i->first, &self->writer));
            ++i->second;
        }
    }
}

// This can be called on any thread, but we want to handle it on thread 0, so we don't have any
//  race conditions on the connections map
void heartbeat_manager_t::on_message(peer_id_t source_peer, UNUSED read_stream_t *stream) {
    on_thread_t rethreader(home_thread());
    std::map<peer_id_t, uint32_t>::iterator i = connections.find(source_peer);
    guarantee(i != connections.end());
    i->second = 0;
}

