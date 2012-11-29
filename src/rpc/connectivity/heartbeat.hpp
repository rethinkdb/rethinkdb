// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RPC_CONNECTIVITY_HEARTBEAT_HPP_
#define RPC_CONNECTIVITY_HEARTBEAT_HPP_

#include <map>

#include "arch/io/arch.hpp"
#include "concurrency/one_per_thread.hpp"
#include "rpc/connectivity/connectivity.hpp"
#include "rpc/connectivity/messages.hpp"
#include "utils.hpp"


class heartbeat_manager_t : public message_handler_t {
public:
    explicit heartbeat_manager_t(message_service_t *_message_service);
    ~heartbeat_manager_t();

    void begin_peer_heartbeat(const peer_id_t &peer_id);
    void end_peer_heartbeat(const peer_id_t &peer_id);

    void message_from_peer(const peer_id_t &source_peer);

private:
    static const int64_t HEARTBEAT_INTERVAL_MS = 2000;
    static const uint32_t HEARTBEAT_TIMEOUT_INTERVALS = 5;

    static void timer_callback(void *ctx);

    // This is a stub, we do everything in message_from_peer instead
    void on_message(UNUSED peer_id_t source_peer, UNUSED read_stream_t *stream) { }

    class heartbeat_writer_t : public send_message_write_callback_t {
    public:
        void write(UNUSED write_stream_t *stream) { }
    };

    struct per_thread_data_t {
        per_thread_data_t();
        ~per_thread_data_t();

        timer_token_t *timer_token;
        std::map<peer_id_t, uint32_t> connections;
    };

    heartbeat_writer_t writer;
    message_service_t *message_service;
    one_per_thread_t<per_thread_data_t> thread_data;

    DISABLE_COPYING(heartbeat_manager_t);
};

#endif /* RPC_CONNECTIVITY_HEARTBEAT_HPP_ */
