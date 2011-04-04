#include "replication/slave_stream_manager.hpp"

namespace replication {

slave_stream_manager_t::slave_stream_manager_t(
        boost::scoped_ptr<tcp_conn_t> *conn,
        queueing_store_t *is,
        multicond_t *multicond) :
    stream_(new repli_stream_t(*conn, this)),
    internal_store_(is),
    multicond_(multicond),
    backfilling_(false),
    interrupted_by_external_event_(false) {

    multicond_->add_waiter(this);
}

slave_stream_manager_t::~slave_stream_manager_t() {
    shutdown_cond_.wait();
}

void slave_stream_manager_t::backfill(repli_timestamp since_when) {
    rassert(!backfilling_);
    net_backfill_t bf;
    bf.timestamp = since_when;
    if (stream_) stream_->send(&bf);
    backfilling_ = true;
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

void slave_stream_manager_t::send(scoped_malloc<net_backfill_complete_t>& message) {
    internal_store_->time_barrier(message->time_barrier_timestamp);
    internal_store_->backfill_complete();
    backfilling_ = false;
    debugf("Received a BACKFILL_COMPLETE message.\n");
}

void slave_stream_manager_t::send(UNUSED scoped_malloc<net_announce_t>& message) {
    debugf("announce message received.\n");
}

void slave_stream_manager_t::send(scoped_malloc<net_get_cas_t>& msg) {
    // TODO this returns a get_result_t (with cross-thread messaging),
    // and we don't really care for that.
    get_cas_mutation_t mut;
    mut.key.assign(msg->key_size, msg->key);
    internal_store_->handover(new mutation_t(mut), castime_t(msg->proposed_cas, msg->timestamp));
}

void slave_stream_manager_t::send(stream_pair<net_sarc_t>& msg) {
    sarc_mutation_t mut;
    mut.key.assign(msg->key_size, msg->keyvalue);
    mut.data = msg.stream;
    mut.flags = msg->flags;
    mut.exptime = msg->exptime;
    mut.add_policy = add_policy_t(msg->add_policy);
    mut.replace_policy = replace_policy_t(msg->replace_policy);
    mut.old_cas = msg->old_cas;

    internal_store_->handover(new mutation_t(mut), castime_t(msg->proposed_cas, msg->timestamp));
}

void slave_stream_manager_t::send(stream_pair<net_backfill_set_t>& msg) {
    rassert(backfilling_);

    sarc_mutation_t mut;
    mut.key.assign(msg->key_size, msg->keyvalue);
    mut.data = msg.stream;
    mut.flags = msg->flags;
    mut.exptime = msg->exptime;
    mut.add_policy = add_policy_yes;
    mut.replace_policy = replace_policy_yes;
    mut.old_cas = NO_CAS_SUPPLIED;

    // TODO: We need this operation to force the cas to be set.
    internal_store_->backfill_handover(new mutation_t(mut), castime_t(msg->cas_or_zero, msg->timestamp));
}

void slave_stream_manager_t::send(scoped_malloc<net_incr_t>& msg) {
    incr_decr_mutation_t mut;
    mut.key.assign(msg->key_size, msg->key);
    mut.kind = incr_decr_INCR;
    mut.amount = msg->amount;
    internal_store_->handover(new mutation_t(mut), castime_t(msg->proposed_cas, msg->timestamp));
}

void slave_stream_manager_t::send(scoped_malloc<net_decr_t>& msg) {
    incr_decr_mutation_t mut;
    mut.key.assign(msg->key_size, msg->key);
    mut.kind = incr_decr_DECR;
    mut.amount = msg->amount;
    internal_store_->handover(new mutation_t(mut), castime_t(msg->proposed_cas, msg->timestamp));
}

void slave_stream_manager_t::send(stream_pair<net_append_t>& msg) {
    append_prepend_mutation_t mut;
    mut.key.assign(msg->key_size, msg->keyvalue);
    mut.data = msg.stream;
    mut.kind = append_prepend_APPEND;
    internal_store_->handover(new mutation_t(mut), castime_t(msg->proposed_cas, msg->timestamp));
}

void slave_stream_manager_t::send(stream_pair<net_prepend_t>& msg) {
    append_prepend_mutation_t mut;
    mut.key.assign(msg->key_size, msg->keyvalue);
    mut.data = msg.stream;
    mut.kind = append_prepend_PREPEND;
    internal_store_->handover(new mutation_t(mut), castime_t(msg->proposed_cas, msg->timestamp));
}

void slave_stream_manager_t::send(scoped_malloc<net_delete_t>& msg) {
    delete_mutation_t mut;
    mut.key.assign(msg->key_size, msg->key);
    // TODO: where does msg->timestamp go???  IS THIS RIGHT?? WHO KNOWS.
    internal_store_->handover(new mutation_t(mut), castime_t(NO_CAS_SUPPLIED /* This isn't even used, why is it a parameter. */, msg->timestamp));
}

void slave_stream_manager_t::send(scoped_malloc<net_backfill_delete_t>& msg) {
    rassert(backfilling_);

    delete_mutation_t mut;
    mut.key.assign(msg->key_size, msg->key);
    // NO_CAS_SUPPLIED is not used in any way for deletions, and the
    // timestamp is part of the "->change" interface in a way not
    // relevant to slaves -- it's used when putting deletions into the
    // delete queue.
    internal_store_->backfill_handover(new mutation_t(mut), castime_t(NO_CAS_SUPPLIED, repli_timestamp::invalid));
}

void slave_stream_manager_t::send(scoped_malloc<net_nop_t>& message) {
    net_ack_t ackreply;
    ackreply.timestamp = message->timestamp;
    stream_->send(ackreply);
    internal_store_->time_barrier(message->timestamp);
}

void slave_stream_manager_t::send(UNUSED scoped_malloc<net_ack_t>& message) {
    // TODO: Kill connection when master sends garbage.
    rassert("ack message received.. as slave?\n");
}

void slave_stream_manager_t::conn_closed() {

    if (backfilling_) {
        logWRN("%s The data on this slave is now in an inconsistent state. To put the data "
            "into a consistent state again, start the server again as a slave of the same "
            "master and allow the backfill operation to run to completion.\n",
            interrupted_by_external_event_ ?
                "You are interrupting the slave in the middle of a backfill operation." :
                "Lost connection to the master in the middle of a backfill operation.");
        internal_store_->backfill_complete();
    }

    // If the connection closed spontaneously, then notify the multicond_t so that
    // the run loop gets unstuck.
    if (!interrupted_by_external_event_) {
        multicond_->remove_waiter(this);   // So on_multicond_pulsed() doesn't get called
        multicond_->pulse();
    }

    delete stream_;
    stream_ = NULL;
    shutdown_cond_.pulse();
}

void slave_stream_manager_t::on_multicond_pulsed() {
    interrupted_by_external_event_ = true;
    stream_->shutdown();   // Will cause conn_closed() to be called
    shutdown_cond_.wait();
}

}
