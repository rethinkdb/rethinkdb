// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RPC_DIRECTORY_WRITE_MANAGER_TCC_
#define RPC_DIRECTORY_WRITE_MANAGER_TCC_

#include "rpc/directory/write_manager.hpp"

#include <set>

#include "arch/runtime/coroutines.hpp"
#include "rpc/connectivity/messages.hpp"

template<class metadata_t>
directory_write_manager_t<metadata_t>::directory_write_manager_t(
        message_service_t *sub,
        const clone_ptr_t<watchable_t<metadata_t> > &value) THROWS_NOTHING :
    message_service(sub),
    value_watchable(value),
    value_subscription(boost::bind(&directory_write_manager_t::on_change, this)),
    connectivity_subscription(this) {
    typename watchable_t<metadata_t>::freeze_t value_freeze(value_watchable);
    connectivity_service_t::peers_list_freeze_t connectivity_freeze(message_service->get_connectivity_service());
    guarantee(message_service->get_connectivity_service()->get_peers_list().empty());
    value_subscription.reset(value_watchable, &value_freeze);
    connectivity_subscription.reset(message_service->get_connectivity_service(), &connectivity_freeze);
}

template<class metadata_t>
directory_write_manager_t<metadata_t>::~directory_write_manager_t() { }

template<class metadata_t>
void directory_write_manager_t<metadata_t>::on_connect(peer_id_t peer) THROWS_NOTHING {
    typename watchable_t<metadata_t>::freeze_t freeze(value_watchable);
    coro_t::spawn_sometime(boost::bind(
        &directory_write_manager_t::send_initialization, this,
        peer,
        value_watchable->get(), metadata_fifo_source.get_state(),
        auto_drainer_t::lock_t(&drainer)));
}

template<class metadata_t>
void directory_write_manager_t<metadata_t>::on_change() THROWS_NOTHING {
    /* Acquire this lock to avoid the case where a new peer copies the metadata
    value after we change it but also gets an update sent. (That would lead to a
    crash on the receiving end because the receiving FIFO would get a duplicate
    update.) */
    connectivity_service_t::peers_list_freeze_t freeze(message_service->get_connectivity_service());
    fifo_enforcer_write_token_t metadata_fifo_token = metadata_fifo_source.enter_write();
    std::set<peer_id_t> peers = message_service->get_connectivity_service()->get_peers_list();
    for (std::set<peer_id_t>::iterator it = peers.begin(); it != peers.end(); it++) {
        coro_t::spawn_sometime(boost::bind(
            &directory_write_manager_t::send_update, this,
            *it,
            value_watchable->get(), metadata_fifo_token,
            auto_drainer_t::lock_t(&drainer)));
    }
}

template <class metadata_t>
class directory_write_manager_t<metadata_t>::initialization_writer_t : public send_message_write_callback_t {
public:
    initialization_writer_t(const metadata_t &_initial_value, fifo_enforcer_state_t _metadata_fifo_state) :
        initial_value(_initial_value), metadata_fifo_state(_metadata_fifo_state) { }
    ~initialization_writer_t() { }

    void write(write_stream_t *stream) {
        write_message_t msg;
        uint8_t code = 'I';
        msg << code;
        msg << initial_value;
        msg << metadata_fifo_state;
        int res = send_write_message(stream, &msg);
        if (res) {
            throw fake_archive_exc_t();
        }
    }
private:
    const metadata_t &initial_value;
    fifo_enforcer_state_t metadata_fifo_state;
};

template <class metadata_t>
class directory_write_manager_t<metadata_t>::update_writer_t : public send_message_write_callback_t {
public:
    update_writer_t(const metadata_t &_new_value, fifo_enforcer_write_token_t _metadata_fifo_token) :
        new_value(_new_value), metadata_fifo_token(_metadata_fifo_token) { }
    ~update_writer_t() { }

    void write(write_stream_t *stream) {
        write_message_t msg;
        uint8_t code = 'U';
        msg << code;
        msg << new_value;
        msg << metadata_fifo_token;
        int res = send_write_message(stream, &msg);
        if (res) {
            throw fake_archive_exc_t();
        }
    }
private:
    const metadata_t &new_value;
    fifo_enforcer_write_token_t metadata_fifo_token;
};

template<class metadata_t>
void directory_write_manager_t<metadata_t>::send_initialization(peer_id_t peer, const metadata_t &initial_value, fifo_enforcer_state_t metadata_fifo_state, auto_drainer_t::lock_t) THROWS_NOTHING {
    initialization_writer_t writer(initial_value, metadata_fifo_state);
    message_service->send_message(peer, &writer);
}

template<class metadata_t>
void directory_write_manager_t<metadata_t>::send_update(peer_id_t peer, const metadata_t &new_value, fifo_enforcer_write_token_t metadata_fifo_token, auto_drainer_t::lock_t) THROWS_NOTHING {
    update_writer_t writer(new_value, metadata_fifo_token);
    message_service->send_message(peer, &writer);
}

#endif  // RPC_DIRECTORY_WRITE_MANAGER_TCC_
