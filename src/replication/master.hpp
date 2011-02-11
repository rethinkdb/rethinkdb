#ifndef __REPLICATION_MASTER_HPP__
#define __REPLICATION_MASTER_HPP__

#include "store.hpp"
#include "arch/arch.hpp"
#include "concurrency/mutex.hpp"
#include "containers/snag_ptr.hpp"
#include "containers/thick_list.hpp"
#include "replication/net_structs.hpp"
#include "replication/protocol.hpp"

namespace replication {

// TODO: Consider a good value for this port.
const int REPLICATION_PORT = 11212;

class master_exc_t : public std::runtime_error {
public:
    master_exc_t(const char *what_arg) : std::runtime_error(what_arg) { }
};


class master_t : public home_thread_mixin_t, public linux_tcp_listener_callback_t, public message_callback_t, public snag_pointee_mixin_t {
public:
    master_t() : message_contiguity_(), slave_(NULL), sources_(), listener_(REPLICATION_PORT) {
        listener_.set_callback(this);
    }

    ~master_t() {
        wait_until_ready_to_delete();
        destroy_existing_slave_conn_if_it_exists();
    }

    bool has_slave() { return slave_ != NULL; }

    void get_cas(store_key_t *key, castime_t castime);

    // Takes ownership of the data_provider_t *data parameter, and deletes it.
    void sarc(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas);

    void incr_decr(incr_decr_kind_t kind, store_key_t *key, uint64_t amount, castime_t castime);

    void append_prepend(append_prepend_kind_t kind, store_key_t *key, data_provider_t *data, castime_t castime);

    void delete_key(store_key_t *key, repli_timestamp timestamp);

    // Listener callback functions
    void on_tcp_listener_accept(boost::scoped_ptr<linux_tcp_conn_t>& conn);

    void hello(net_hello_t message) { hello_receive_var_.pulse(message); }
    void send(buffed_data_t<net_backfill_t>& message) { }
    void send(buffed_data_t<net_announce_t>& message) { }
    void send(buffed_data_t<net_get_cas_t>& message) { }
    void send(stream_pair<net_sarc_t>& message) { }
    void send(buffed_data_t<net_incr_t>& message) { }
    void send(buffed_data_t<net_decr_t>& message) { }
    void send(stream_pair<net_append_t>& message) { }
    void send(stream_pair<net_prepend_t>& message) { }
    void send(buffed_data_t<net_delete_t>& message) { }
    void send(buffed_data_t<net_nop_t>& message) { }
    void send(buffed_data_t<net_ack_t>& message) { }
    void send(buffed_data_t<net_shutting_down_t>& message) { }
    void send(buffed_data_t<net_goodbye_t>& message) { }
    void conn_closed() { }


private:
    // Spawns a coroutine.
    void send_data_with_ident(data_provider_t *data, uint32_t ident);

    template <class net_struct_type>
    void incr_decr_like(uint8_t msgcode, store_key_t *key, uint64_t amount, castime_t castime);

    template <class net_struct_type>
    void stereotypical(int msgcode, store_key_t *key, data_provider_t *data, net_struct_type netstruct);

    void send_hello(const mutex_acquisition_t& proof_of_acquisition);
    void receive_hello(const mutex_acquisition_t& proof_of_acquisition);

    void destroy_existing_slave_conn_if_it_exists();

    mutex_t message_contiguity_;
    boost::scoped_ptr<tcp_conn_t> slave_;
    thick_list<data_provider_t *, uint32_t> sources_;

    tcp_listener_t listener_;

    message_parser_t parser_;

    unicond_t<net_hello_t> hello_receive_var_;

    DISABLE_COPYING(master_t);
};





}  // namespace replication

#endif  // __REPLICATION_MASTER_HPP__
