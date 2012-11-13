// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RPC_CONNECTIVITY_HEARTBEAT_HPP_
#define RPC_CONNECTIVITY_HEARTBEAT_HPP_

#include <map>

#include "rpc/connectivity/connectivity.hpp"
#include "rpc/connectivity/messages.hpp"
#include "concurrency/one_per_thread.hpp"
#include "arch/timer.hpp"

// This class implements a heartbeat on top of intra-cluster connections
class heartbeat_manager_t :
    private peers_list_callback_t,
    public message_handler_t,
    public home_thread_mixin_t {
public:
    explicit heartbeat_manager_t(message_service_t *_message_service);
    ~heartbeat_manager_t();

private:
    // TODO: make these configurable
    static const int64_t TIMER_INTERVAL_MS = 2000;
    static const uint32_t MAX_KEEPALIVES = 5;

    // Start heartbeat on a new peer connection
    void on_connect(peer_id_t peer_id);

    // Peer connection has gone down, stop heartbeat for it
    void on_disconnect(peer_id_t peer_id);

    // Send the heartbeat to all peer connections on this thread
    static void timer_callback(void *ctx);

    // Handle a heartbeat from a peer connection
    void on_message(peer_id_t source_peer, read_stream_t *stream);

    class heartbeat_writer_t : public send_message_write_callback_t {
    public:
        void write(UNUSED write_stream_t *stream) { }
    };

    message_service_t *message_service;
    heartbeat_writer_t writer;
    timer_token_t *timer_token;
    std::map<peer_id_t, uint32_t> connections;
    connectivity_service_t::peers_list_subscription_t connection_watcher;
};

#endif
