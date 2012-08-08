#include "rpc/directory/write_manager.hpp"

#include <set>

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
    rassert(message_service->get_connectivity_service()->get_peers_list().empty());
    value_subscription.reset(value_watchable, &value_freeze);
    connectivity_subscription.reset(message_service->get_connectivity_service(), &connectivity_freeze);
}

template<class metadata_t>
void directory_write_manager_t<metadata_t>::on_connect(peer_id_t peer) THROWS_NOTHING {
    typename watchable_t<metadata_t>::freeze_t freeze(value_watchable);
    coro_t::spawn_sometime(boost::bind(
        &directory_write_manager_t::send_initialization, this,
        peer,
        value_watchable->get(), metadata_fifo_source.get_state(),
        auto_drainer_t::lock_t(&drainer)
        ));
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
    for (std::set<peer_id_t>::iterator it = peers.begin(); it != peers.end(); ++it) {
        coro_t::spawn_sometime(boost::bind(
            &directory_write_manager_t::send_update, this,
            *it,
            value_watchable->get(), metadata_fifo_token,
            auto_drainer_t::lock_t(&drainer)
            ));
    }
}

template<class metadata_t>
void directory_write_manager_t<metadata_t>::send_initialization(peer_id_t peer, const metadata_t &initial_value, fifo_enforcer_source_t::state_t metadata_fifo_state, auto_drainer_t::lock_t) THROWS_NOTHING {
    message_service->send_message(peer,
        boost::bind(&directory_write_manager_t::write_initialization, _1, initial_value, metadata_fifo_state)
        );
}

template<class metadata_t>
void directory_write_manager_t<metadata_t>::send_update(peer_id_t peer, const metadata_t &new_value, fifo_enforcer_write_token_t metadata_fifo_token, auto_drainer_t::lock_t) THROWS_NOTHING {
    message_service->send_message(peer,
        boost::bind(&directory_write_manager_t::write_update, _1, new_value, metadata_fifo_token)
        );
}

template<class metadata_t>
void directory_write_manager_t<metadata_t>::write_initialization(write_stream_t *os, const metadata_t &initial_value, fifo_enforcer_source_t::state_t metadata_fifo_state) THROWS_NOTHING {
    write_message_t msg;
    uint8_t code = 'I';
    msg << code;
    msg << initial_value;
    msg << metadata_fifo_state;
    int res = send_write_message(os, &msg);
    if (res) {
        throw fake_archive_exc_t();
    }
}

template<class metadata_t>
void directory_write_manager_t<metadata_t>::write_update(write_stream_t *os, const metadata_t &new_value, fifo_enforcer_write_token_t metadata_fifo_token) THROWS_NOTHING {
    write_message_t msg;
    uint8_t code = 'U';
    msg << code;
    msg << new_value;
    msg << metadata_fifo_token;
    int res = send_write_message(os, &msg);
    if (res) {
        throw fake_archive_exc_t();
    }
}

