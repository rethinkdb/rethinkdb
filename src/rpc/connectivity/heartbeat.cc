#include "rpc/connectivity/heartbeat.hpp"
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

void heartbeat_manager_t::begin_peer_heartbeat(const peer_id_t &peer_id) {
    per_thread_data_t *data = thread_data.get();
    guarantee(data->connections.insert(std::make_pair(peer_id, 0)).second == true);

    if (data->timer_token == NULL) {
        rassert(data->connections.size() == 1);
        data->timer_token = add_timer(HEARTBEAT_INTERVAL_MS, &timer_callback, this);
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

void heartbeat_manager_t::timer_callback(void *ctx) {
    ASSERT_FINITE_CORO_WAITING;
    heartbeat_manager_t *self = reinterpret_cast<heartbeat_manager_t*>(ctx);
    per_thread_data_t *data = self->thread_data.get();
    for (std::map<peer_id_t, uint32_t>::iterator it = data->connections.begin();
         it != data->connections.end(); ++it) {
        if (it->second >= HEARTBEAT_TIMEOUT_INTERVALS) {
            const std::string peer_str(uuid_to_str(it->first.get_uuid()).c_str());
            logERR("Heartbeat timeout, killing connection to peer: %s.", peer_str.c_str());
            coro_t::spawn_later_ordered(boost::bind(&message_service_t::kill_connection, self->message_service, it->first));
        } else {
            coro_t::spawn_later_ordered(boost::bind(&message_service_t::send_message, self->message_service, it->first, &self->writer));
        }
        ++it->second;
    }
}

void heartbeat_manager_t::message_from_peer(const peer_id_t &source_peer) {
    per_thread_data_t *data = thread_data.get();
    std::map<peer_id_t, uint32_t>::iterator it = data->connections.find(source_peer);

    if (it != data->connections.end()) {
        it->second = 0;
    }
}

