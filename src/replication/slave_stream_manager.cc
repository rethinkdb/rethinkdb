#include "replication/slave_stream_manager.hpp"
#include "replication/backfill_sender.hpp"
#include "replication/backfill_out.hpp"

namespace replication {

slave_stream_manager_t::slave_stream_manager_t(
        boost::scoped_ptr<tcp_conn_t> *conn,
        btree_key_value_store_t *kvs,
        cond_t *cond) :
    backfill_receiver_t(&backfill_storer_),
    stream_(NULL),
    cond_(NULL),
    kvs_(kvs),
    backfill_storer_(kvs),
    interrupted_by_external_event_(false) {

    stream_ = new repli_stream_t(*conn, this);

    cond_ = cond;
    if (cond_->is_pulsed()) {
        on_signal_pulsed();
    } else {
        cond_->add_waiter(this);
    }
}

slave_stream_manager_t::~slave_stream_manager_t() {
    assert_thread();
    shutdown_cond_.wait();
}

void slave_stream_manager_t::backfill(repli_timestamp since_when) {

    cond_t c;

    net_backfill_t bf;
    bf.timestamp = since_when;
    if (stream_) stream_->send(&bf);

    backfill_done_cond_.watch(&c);   // Stop when the backfill finishes
    cond_link_t abort_if_connection_closed(&shutdown_cond_, &c);   // Stop if the connection is closed
    c.wait();
}

void slave_stream_manager_t::reverse_side_backfill(repli_timestamp since_when) {

    assert_thread();

    debugf("Doing reverse_side_backfill.\n");

    backfill_sender_t sender(&stream_);

    cond_t mc;
    mc.pulse();   // So that backfill_and_realtime_stream() returns as soon as the backfill part is over
    backfill_and_realtime_stream(kvs_, since_when, &sender, &mc);
}

 /* message_callback_t interface */
void slave_stream_manager_t::hello(UNUSED net_hello_t message) {
    debugf("hello message received.\n");
}

void slave_stream_manager_t::send(UNUSED scoped_malloc<net_backfill_t>& message) {
    // TODO: Kill connection instead of crashing server, when master
    // sends garbage.
    rassert(false, "backfill message?  what?\n");
}

void slave_stream_manager_t::conn_closed() {
    assert_thread();

    /* Do this first-thing so that nothing tries to use the closed stream. The
    repli_stream_t destructor may block, which is why we set stream_ to NULL before
    we call the destructor. */
    repli_stream_t *stream_copy = stream_;
    stream_ = NULL;
    delete stream_copy;

    // If the connection closed spontaneously, then notify the cond_t so that
    // the run loop gets unstuck. cond_ could be NULL if we didn't finish our
    // constructor yet.
    if (!interrupted_by_external_event_ && cond_) {
        cond_->remove_waiter(this);   // So on_cond_pulsed() doesn't get called
        cond_->pulse();
    }

    shutdown_cond_.pulse();
}

void slave_stream_manager_t::on_signal_pulsed() {
    assert_thread();

    interrupted_by_external_event_ = true;
    stream_->shutdown();   // Will cause conn_closed() to be called
}

}
