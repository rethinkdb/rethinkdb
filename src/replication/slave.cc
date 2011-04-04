#include "slave.hpp"

#include <stdint.h>

#include "arch/linux/coroutines.hpp"
#include "logger.hpp"
#include "net_structs.hpp"
#include "server/key_value_store.hpp"
#include "replication/backfill.hpp"

namespace replication {

slave_stream_manager_t::slave_stream_manager_t(
        boost::scoped_ptr<tcp_conn_t> *conn,
        queueing_store_t *is,
        multicond_t *multicond) :
    stream_(new repli_stream_t(*conn, this)),
    internal_store_(is),
    multicond_(multicond),
    interrupted_by_external_event_(false) {

    multicond_->add_waiter(this);
}

slave_stream_manager_t::~slave_stream_manager_t() {
    shutdown_cond_.wait();
}

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
}

// TODO unit test offsets


slave_t::slave_t(btree_key_value_store_t *internal_store, replication_config_t replication_config, failover_config_t failover_config)
    : failover_script_(failover_config.failover_script_path),
      timeout_(INITIAL_TIMEOUT),
      failover_reset_control_(std::string("failover reset"), this),
      new_master_control_(std::string("new master"), this),
      internal_store_(new queueing_store_t(internal_store)),
      replication_config_(replication_config),
      failover_config_(failover_config)
{
    coro_t::spawn(boost::bind(&run, this));
}

slave_t::~slave_t() {
    coro_t::move_to_thread(home_thread);

    *shutting_down_ = true;

    /* If there is an open connection to the master, this will kill it. If we gave up
    on connecting to the master, this will cause us to resume trying to connect. If we
    are in a timeout before retrying the connection, this will cut the timout short. */
    pulse_to_interrupt_run_loop_.pulse_if_non_null();
}

struct not_allowed_visitor_t : public boost::static_visitor<mutation_result_t> {
    mutation_result_t operator()(UNUSED const get_cas_mutation_t& m) const {
        /* TODO: This is a hack. We can't give a valid result because the CAS we return will
        not be usable with the master. But currently there's no way to signal an error on
        gets. So we pretend the value was not found. Technically this is a lie, but screw
        it. */
        return get_result_t();
    }
    mutation_result_t operator()(UNUSED const sarc_mutation_t& m) const {
        return sr_not_allowed;
    }
    mutation_result_t operator()(UNUSED const incr_decr_mutation_t& m) const {
        return incr_decr_result_t(incr_decr_result_t::idr_not_allowed);
    }
    mutation_result_t operator()(UNUSED const append_prepend_mutation_t& m) const {
        return apr_not_allowed;
    }
    mutation_result_t operator()(UNUSED const delete_mutation_t& m) const {
        return dr_not_allowed;
    }
};

mutation_result_t slave_t::change(const mutation_t &m) {

    if (respond_to_queries_) {
        return internal_store_->bypass_change(m);
    } else {
        /* Construct a "failure" response of the right sort */
        return boost::apply_visitor(not_allowed_visitor_t(), m.mutation);
    }
}

std::string slave_t::failover_reset() {
    // TODO don't say get_num_threads() - 2, what does this mean?
    on_thread_t thread_switch(get_num_threads() - 2);
    give_up_.reset();
    timeout_ = INITIAL_TIMEOUT;

    /* If there is an open connection to the master, this will kill it. If we gave up
    on connecting to the master, this will cause us to resume trying to connect. If we
    are in a timeout before retrying the connection, this will cut the timout short. */
    pulse_to_interrupt_run_loop_.pulse_if_non_null();

    return std::string("Resetting failover\n");
}

/* give_up_t interface: the give_up_t struct keeps track of the last N times
 * the server failed. If it's failing to frequently then it will tell us to
 * give up on the server and stop trying to reconnect */
void slave_t::give_up_t::on_reconnect() {
    successful_reconnects.push(ticks_to_secs(get_ticks()));
    limit_to(MAX_RECONNECTS_PER_N_SECONDS);
}

bool slave_t::give_up_t::give_up() {
    limit_to(MAX_RECONNECTS_PER_N_SECONDS);

    return (successful_reconnects.size() == MAX_RECONNECTS_PER_N_SECONDS &&
            (successful_reconnects.back() - ticks_to_secs(get_ticks())) < N_SECONDS);
}

void slave_t::give_up_t::reset() {
    limit_to(0);
}

void slave_t::give_up_t::limit_to(unsigned int limit) {
    while (successful_reconnects.size() > limit)
        successful_reconnects.pop();
}

/* failover callback */
void slave_t::on_failure() {
    respond_to_queries_ = true;
}

void slave_t::on_resume() {
    respond_to_queries_ = false;
}

void run(slave_t *slave) {

    bool shutting_down = false;
    slave->shutting_down_ = &shutting_down;

    slave->failover.add_callback(slave);
    slave->respond_to_queries_ = false;
    bool first_connect = true;
    while (!shutting_down) {
        try {

            multicond_t slave_multicond;

            logINF("Attempting to connect as slave to: %s:%d\n",
                slave->replication_config_.hostname, slave->replication_config_.port);

            boost::scoped_ptr<tcp_conn_t> conn(
                new tcp_conn_t(slave->replication_config_.hostname, slave->replication_config_.port));
            slave_stream_manager_t stream_mgr(&conn, slave->internal_store_.get(), &slave_multicond);

            // No exception was thrown; it must have worked.
            slave->timeout_ = INITIAL_TIMEOUT;
            slave->give_up_.on_reconnect();
            first_connect = false;
            slave->failover.on_resume();
            logINF("Connected as slave to: %s:%d\n", slave->replication_config_.hostname, slave->replication_config_.port);

            // This makes it so that if we get a shutdown/reset command, the connection
            // will get closed.
            slave->pulse_to_interrupt_run_loop_.watch(&slave_multicond);

            // TODO: this is a fake timestamp!!! You _must_ fix this.
            repli_timestamp fake1 = { 0 };
            stream_mgr.reverse_side_backfill(fake1);

            // TODO: another fake timestamp!!! You _must_ fix this.
            repli_timestamp fake2 = { 0 };
            net_backfill_t bf;
            bf.timestamp = fake2;
            if (stream_mgr.stream_) stream_mgr.stream_->send(&bf);

            debugf("slave_t: Waiting for things to fail...\n");
            slave_multicond.wait();
            debugf("slave_t: Things failed.\n");

            if (shutting_down) {
                break;
            }

            slave->failover.on_failure();

        } catch (tcp_conn_t::connect_failed_exc_t& e) {
            //Presumably if the master doesn't even accept an initial
            //connection then this is a user error rather than some sort of
            //failure
            if (first_connect) {
                crash("Master at %s:%d is not responding :(. Perhaps you haven't brought it up "
                    "yet. But what do I know, I'm just a database.\n",
                    slave->replication_config_.hostname, slave->replication_config_.port);
            }
        }

        /* The connection has failed. Let's see what we should do */
        if (!slave->give_up_.give_up()) {
            int timeout = slave->timeout_;
            slave->timeout_ = std::min(slave->timeout_ * TIMEOUT_GROWTH_FACTOR, (long)TIMEOUT_CAP);

            multicond_t c;
            pulse_after_time(&c, timeout);
            slave->pulse_to_interrupt_run_loop_.watch(&c);
            c.wait();

        } else {
            logINF("Master at %s:%d has failed %d times in the last %d seconds, "
                   "going rogue. To resume slave behavior send the command "
                   "\"rethinkdb failover reset\" (over telnet).\n",
                   slave->replication_config_.hostname, slave->replication_config_.port,
                   MAX_RECONNECTS_PER_N_SECONDS, N_SECONDS);

            multicond_t c;
            slave->pulse_to_interrupt_run_loop_.watch(&c);
            c.wait();
        }
    }
}

std::string slave_t::new_master(int argc, char **argv) {
    guarantee(argc == 3); // TODO: Handle argc = 0.
    std::string host = argv[1];
    if (host.length() >  MAX_HOSTNAME_LEN - 1)
        return "That hostname is too long; use a shorter one.\n";

    /* redo the replication_config info */
    strcpy(replication_config_.hostname, host.c_str());
    replication_config_.port = atoi(argv[2]); //TODO this is ugly

    failover_reset();

    return "New master set\n";
}

// TODO: instead of UNUSED, ensure that these parameters are empty.
std::string slave_t::failover_reset_control_t::call(UNUSED int argc, UNUSED char **argv) {
    return slave->failover_reset();
}

std::string slave_t::new_master_control_t::call(UNUSED int argc, UNUSED char **argv) {
    return slave->new_master(argc, argv);
}

}  // namespace replication

