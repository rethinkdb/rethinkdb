#ifndef __REPLICATION_MASTER_HPP__
#define __REPLICATION_MASTER_HPP__

#include "store.hpp"
#include "arch/arch.hpp"
#include "btree/backfill.hpp"
#include "concurrency/mutex.hpp"
#include "containers/snag_ptr.hpp"
#include "containers/thick_list.hpp"
#include "replication/net_structs.hpp"
#include "replication/protocol.hpp"

class btree_slice_dispatching_to_master_t;

namespace replication {

class master_exc_t : public std::runtime_error {
public:
    master_exc_t(const char *what_arg) : std::runtime_error(what_arg) { }
};


// master_t is a class that manages a connection to a slave.  It
// behaves somewhat like a set_store_t, in fact maybe it actually
// obeys the set_store_t interface.  Right now it is like a
// set_store_t that sends data to the slave.  You must send the
// messages on the master_t's thread.  Use a
// buffer_borrowing_data_provider_t whose side thread is the master's
// home_thread, and then send its side_data_provider_t to the
// master_t using spawn_on_thread.

class master_t : public home_thread_mixin_t, public linux_tcp_listener_callback_t, public message_callback_t, public snag_pointee_mixin_t {
public:
    master_t(thread_pool_t *thread_pool, int port)
        : timer_handler_(&thread_pool->threads[get_thread_id()]->timer_handler),
          stream_(NULL), listener_(port),
          next_timestamp_nop_timer_(NULL), latest_timestamp_(current_time()) {
        listener_.set_callback(this);
    }

    ~master_t() {
        wait_until_ready_to_delete();
        destroy_existing_slave_conn_if_it_exists();
    }

    // TODO: get rid of this, have master_t take a list of slices in
    // the constructor?  Or register them all in a single registration
    // function?  Tie the knot more cleanly, so that we don't have to
    // worry about slices registering themselves while other slices
    // send operations.
    void register_dispatcher(btree_slice_dispatching_to_master_t *dispatcher);

    bool has_slave() { return stream_ != NULL; }

    void get_cas(const store_key_t &key, castime_t castime);

    // Takes ownership of the data_provider_t *data parameter, and deletes it.
    void sarc(const store_key_t &key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas);

    void incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount, castime_t castime);

    void append_prepend(append_prepend_kind_t kind, const store_key_t &key, data_provider_t *data, castime_t castime);

    void delete_key(const store_key_t &key, repli_timestamp timestamp);

    // Listener callback functions
    void on_tcp_listener_accept(boost::scoped_ptr<linux_tcp_conn_t>& conn);

    void hello(UNUSED net_hello_t message) { debugf("Received hello from slave.\n"); }
    void send(UNUSED buffed_data_t<net_backfill_t>& message) { }
    void send(UNUSED buffed_data_t<net_announce_t>& message) { rassert(false, "slave sent announce"); }
    void send(UNUSED buffed_data_t<net_get_cas_t>& message) { rassert(false, "slave sent get_cas"); }
    void send(UNUSED stream_pair<net_sarc_t>& message) { rassert(false, "slave sent sarc"); }
    void send(UNUSED buffed_data_t<net_incr_t>& message) { rassert(false, "slave sent incr"); }
    void send(UNUSED buffed_data_t<net_decr_t>& message) { rassert(false, "slave sent decr"); }
    void send(UNUSED stream_pair<net_append_t>& message) { rassert(false, "slave sent append"); }
    void send(UNUSED stream_pair<net_prepend_t>& message) { rassert(false, "slave sent prepend"); }
    void send(UNUSED buffed_data_t<net_delete_t>& message) { rassert(false, "slave sent delete"); }
    void send(UNUSED buffed_data_t<net_nop_t>& message) { rassert(false, "slave sent nop"); }
    void send(UNUSED buffed_data_t<net_ack_t>& message) { }
    void send(UNUSED buffed_data_t<net_shutting_down_t>& message) { }
    void send(UNUSED buffed_data_t<net_goodbye_t>& message) { }
    void conn_closed() { destroy_existing_slave_conn_if_it_exists(); }

    void do_nop_rebound(repli_timestamp t);

    void consider_nop_dispatch_and_update_latest_timestamp(repli_timestamp timestamp);

    void do_backfill(repli_timestamp since_when);

    void send_backfill_atom_to_slave(backfill_atom_t atom);

private:
    // Spawns a coroutine.
    void send_data_with_ident(data_provider_t *data, uint32_t ident);

    template <class net_struct_type>
    void incr_decr_like(uint8_t msgcode, const store_key_t& key, uint64_t amount, castime_t castime);

    template <class net_struct_type>
    void stereotypical(int msgcode, const store_key_t &key, data_provider_t *data, net_struct_type netstruct);

    void destroy_existing_slave_conn_if_it_exists();

    // The thread-local timer handler, which we use to set timers.
    timer_handler_t *timer_handler_;

    // The stream to the slave, or NULL if there is no slave connected.
    repli_stream_t *stream_;

    // Listens for incoming slave connections.
    tcp_listener_t listener_;

    // If no actions come in, we periodically send nops to the slave
    // anyway, to keep up a heartbeat.  This is the timer token for
    // the next time we must send a nop (if we haven't already).  This
    // is NULL if and only if stream_ is NULL.
    timer_token_t *next_timestamp_nop_timer_;

    // The latest timestamp we've seen.  Every time we get a new
    // timestamp, we update this value and spawn
    // consider_nop_dispatch_and_update_latest_timestamp(the_new_timestamp),
    // which tells the slices to check in.
    repli_timestamp latest_timestamp_;

    // All the slices.
    std::vector<btree_slice_dispatching_to_master_t *> dispatchers_;

    DISABLE_COPYING(master_t);
};





}  // namespace replication

#endif  // __REPLICATION_MASTER_HPP__
