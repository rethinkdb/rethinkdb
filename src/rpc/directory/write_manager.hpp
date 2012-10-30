// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RPC_DIRECTORY_WRITE_MANAGER_HPP_
#define RPC_DIRECTORY_WRITE_MANAGER_HPP_

#include "concurrency/auto_drainer.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/watchable.hpp"
#include "rpc/connectivity/connectivity.hpp"

class message_service_t;

template<class metadata_t>
class directory_write_manager_t : private peers_list_callback_t {
public:
    directory_write_manager_t(
        message_service_t *message_service,
        const clone_ptr_t<watchable_t<metadata_t> > &value) THROWS_NOTHING;
    ~directory_write_manager_t();

private:
    void on_connect(peer_id_t peer) THROWS_NOTHING;
    void on_disconnect(UNUSED peer_id_t p) { }
    void on_change() THROWS_NOTHING;

    void send_initialization(peer_id_t peer, const metadata_t &initial_value, fifo_enforcer_state_t metadata_fifo_state, auto_drainer_t::lock_t keepalive) THROWS_NOTHING;
    void send_update(peer_id_t peer, const metadata_t &new_value, fifo_enforcer_write_token_t metadata_fifo_token, auto_drainer_t::lock_t keepalive) THROWS_NOTHING;

    class initialization_writer_t;
    class update_writer_t;

    message_service_t *const message_service;
    clone_ptr_t<watchable_t<metadata_t> > value_watchable;
    fifo_enforcer_source_t metadata_fifo_source;
    auto_drainer_t drainer;
    typename watchable_t<metadata_t>::subscription_t value_subscription;
    connectivity_service_t::peers_list_subscription_t connectivity_subscription;
};

#endif /* RPC_DIRECTORY_WRITE_MANAGER_HPP_ */
