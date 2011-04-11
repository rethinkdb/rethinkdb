#include "replication/slave_stream_manager.hpp"

namespace replication {

slave_stream_manager_t::slave_stream_manager_t(
        boost::scoped_ptr<tcp_conn_t> *conn,
        btree_key_value_store_t *kvs,
        multicond_t *multicond) :
    backfill_receiver_t(kvs),
    stream_(NULL),
    multicond_(NULL),
    interrupted_by_external_event_(false) {

    /* This is complicated. "new repli_stream_t()" could call our conn_closed() method.
    "multicond_->add_waiter()" could call our on_multicond_pulsed() method. on_multicond_pulsed()
    accesses stream_ and conn_closed() accesses multicond_. Solution is to set multicond_ to NULL,
    call "new repli_stream_t()", then set multicond_ to its final value, then call add_waiter(). */

    stream_ = new repli_stream_t(*conn, this);

    multicond_ = multicond;
    multicond_->add_waiter(this);
}

slave_stream_manager_t::~slave_stream_manager_t() {
    shutdown_cond_.wait();
}

void slave_stream_manager_t::backfill(repli_timestamp since_when) {
    net_backfill_t bf;
    bf.timestamp = since_when;
    if (stream_) stream_->send(&bf);
}

#ifdef REVERSE_BACKFILLING
void slave_stream_manager_t::reverse_side_backfill(repli_timestamp since_when) {
    // TODO: This rather duplicates some code in master_t::do_backfill.

    assert_thread();

    debugf("Doing reverse_side_backfill.\n");

    do_backfill_cb cb(home_thread, &stream_);

    internal_store_->inner()->spawn_backfill(since_when, &cb);

    debugf("reverse_side_backfill waiting...\n");
    repli_timestamp minimum_timestamp = cb.wait();
    debugf("reverse_side_backfill done waiting.\n");
    if (stream_) {
        net_backfill_complete_t msg;
        msg.time_barrier_timestamp = minimum_timestamp;
        stream_->send(&msg);
    }
}
#endif

 /* message_callback_t interface */
void slave_stream_manager_t::hello(UNUSED net_hello_t message) {
    debugf("hello message received.\n");
}

void slave_stream_manager_t::send(UNUSED scoped_malloc<net_backfill_t>& message) {
    // TODO: Kill connection instead of crashing server, when master
    // sends garbage.
    rassert(false, "backfill message?  what?\n");
}

void slave_stream_manager_t::send(UNUSED scoped_malloc<net_ack_t>& message) {
    // TODO: Kill connection when master sends garbage.
    crash("ack is obsolete\n");
}

void slave_stream_manager_t::conn_closed() {

    /* Do this first-thing so that nothing tries to use the closed stream. The
    repli_stream_t destructor may block, which is why we set stream_ to NULL before
    we call the destructor. */
    repli_stream_t *stream_copy = stream_;
    stream_ = NULL;
    delete stream_copy;

    // If the connection closed spontaneously, then notify the multicond_t so that
    // the run loop gets unstuck. multicond_ could be NULL if we didn't finish our
    // constructor yet.
    if (!interrupted_by_external_event_ && multicond_) {
        multicond_->remove_waiter(this);   // So on_multicond_pulsed() doesn't get called
        multicond_->pulse();
    }

    shutdown_cond_.pulse();
}

void slave_stream_manager_t::on_multicond_pulsed() {
    interrupted_by_external_event_ = true;
    stream_->shutdown();   // Will cause conn_closed() to be called
    shutdown_cond_.wait();
}

}
