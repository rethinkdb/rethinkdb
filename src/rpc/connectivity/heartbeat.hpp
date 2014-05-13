// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RPC_CONNECTIVITY_HEARTBEAT_HPP_
#define RPC_CONNECTIVITY_HEARTBEAT_HPP_

#include <map>

#include "arch/timer.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/one_per_thread.hpp"
#include "rpc/connectivity/connectivity.hpp"
#include "rpc/connectivity/messages.hpp"
#include "utils.hpp"


class heartbeat_manager_t : public message_handler_t, private timer_callback_t {
public:
    explicit heartbeat_manager_t(message_service_t *_message_service);
    ~heartbeat_manager_t();

    void begin_peer_heartbeat(const peer_id_t &peer_id);
    void end_peer_heartbeat(const peer_id_t &peer_id);

    void message_from_peer(const peer_id_t &source_peer);

    // Callback class which keeps track of traffic on the connection
    class heartbeat_keepalive_tracker_t {
    public:
        virtual ~heartbeat_keepalive_tracker_t() { }
        virtual bool check_and_reset_reads() = 0;
        virtual bool check_and_reset_writes() = 0;
    };

    void set_keepalive_tracker(const peer_id_t &peer_id, heartbeat_keepalive_tracker_t *tracker);

private:
    static const int64_t HEARTBEAT_INTERVAL_MS = 2000;
    static const uint32_t HEARTBEAT_TIMEOUT_INTERVALS = 5;

    void on_timer();

    // This is a stub, we do everything in message_from_peer instead
    void on_message(UNUSED peer_id_t source_peer, UNUSED cluster_version_t version,
                    UNUSED read_stream_t *stream) { }
    void send_message_wrapper(const peer_id_t source_peer, UNUSED auto_drainer_t::lock_t keepalive);
    void kill_connection_wrapper(const peer_id_t source_peer, UNUSED auto_drainer_t::lock_t keepalive);

    class heartbeat_writer_t : public send_message_write_callback_t {
    public:
        void write(cluster_version_t, UNUSED write_stream_t *stream) { }
    };

    struct per_thread_data_t {
        per_thread_data_t();
        ~per_thread_data_t();

        timer_token_t *timer_token;

        struct conn_data_t {
            conn_data_t();
            uint32_t outstanding;
            heartbeat_keepalive_tracker_t *tracker;
        };

        std::map<peer_id_t, conn_data_t> connections;
        auto_drainer_t drainer;
    };

    heartbeat_writer_t writer;
    message_service_t *message_service;
    one_per_thread_t<per_thread_data_t> thread_data;

    DISABLE_COPYING(heartbeat_manager_t);
};

#endif /* RPC_CONNECTIVITY_HEARTBEAT_HPP_ */
