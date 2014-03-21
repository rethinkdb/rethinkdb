#include "rpc/connectivity/heartbeat.hpp"

#include <functional>

#include "logger.hpp"

heartbeat_manager_t::heartbeat_manager_t(message_service_t *_message_service) :
    message_service(_message_service) {
    // Do nothing
}

heartbeat_manager_t::~heartbeat_manager_t() {
    // Do nothing
}

heartbeat_manager_t::per_thread_data_t::per_thread_data_t() :
    timer_token(NULL) { }

heartbeat_manager_t::per_thread_data_t::~per_thread_data_t() {
    if (timer_token != NULL) {
        cancel_timer(timer_token);
        timer_token = NULL;
    }
}

heartbeat_manager_t::per_thread_data_t::conn_data_t::conn_data_t() :
    outstanding(0),
    tracker(NULL) {
    // Do nothing
}


void heartbeat_manager_t::begin_peer_heartbeat(const peer_id_t &peer_id) {
    per_thread_data_t *data = thread_data.get();
    guarantee(data->connections.insert(std::make_pair(peer_id, per_thread_data_t::conn_data_t())).second == true);

    if (data->timer_token == NULL) {
        rassert(data->connections.size() == 1);
        data->timer_token = add_timer(HEARTBEAT_INTERVAL_MS, this);
    }
}

void heartbeat_manager_t::end_peer_heartbeat(const peer_id_t &peer_id) {
    per_thread_data_t *data = thread_data.get();
    guarantee(data->connections.erase(peer_id) == 1);

    rassert(data->timer_token != NULL);
    if (data->connections.empty()) {
        cancel_timer(data->timer_token);
        data->timer_token = NULL;
    }
}

void heartbeat_manager_t::set_keepalive_tracker(const peer_id_t &peer, heartbeat_keepalive_tracker_t *tracker) {
    per_thread_data_t *data = thread_data.get();
    std::map<peer_id_t, per_thread_data_t::conn_data_t>::iterator it = data->connections.find(peer);

    if (it != data->connections.end()) {
        it->second.tracker = tracker;
    }
}

void heartbeat_manager_t::on_timer() {
    ASSERT_FINITE_CORO_WAITING;
    heartbeat_manager_t *self = this;
    per_thread_data_t *data = self->thread_data.get();
    for (std::map<peer_id_t, per_thread_data_t::conn_data_t>::iterator it = data->connections.begin();
         it != data->connections.end(); ++it) {
        bool read_done = false;
        bool write_done = false;

        if (it->second.tracker != NULL) {
            read_done = it->second.tracker->check_and_reset_reads();
            write_done = it->second.tracker->check_and_reset_writes();
        }

        if (it->second.outstanding >= HEARTBEAT_TIMEOUT_INTERVALS) {
            const std::string peer_str(uuid_to_str(it->first.get_uuid()).c_str());
            logERR("Heartbeat timeout, killing connection to peer: %s.", peer_str.c_str());
            coro_t::spawn_later_ordered(std::bind(&heartbeat_manager_t::kill_connection_wrapper,
                                                  self,
                                                  it->first,
                                                  auto_drainer_t::lock_t(&data->drainer)));
        } else if (!write_done) {
            // Only send a heartbeat if nothing was sent since the last timer
            coro_t::spawn_later_ordered(std::bind(&heartbeat_manager_t::send_message_wrapper,
                                                  self,
                                                  it->first,
                                                  auto_drainer_t::lock_t(&data->drainer)));
        }

        if (read_done) {
            // If we've read data from the socket since the last timer, clear the outstanding acks
            it->second.outstanding = 0;
        }

        ++it->second.outstanding;
    }
}

void heartbeat_manager_t::kill_connection_wrapper(const peer_id_t peer, UNUSED auto_drainer_t::lock_t keepalive) {
    message_service->kill_connection(peer);
}

void heartbeat_manager_t::send_message_wrapper(const peer_id_t peer, UNUSED auto_drainer_t::lock_t keepalive) {
    message_service->send_message(peer, &writer);
}
