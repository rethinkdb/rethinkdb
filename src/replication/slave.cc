#include "slave.hpp"

#include <stdint.h>

#include "arch/linux/coroutines.hpp"
#include "logger.hpp"
#include "net_structs.hpp"
#include "server/key_value_store.hpp"
#include "replication/backfill.hpp"
#include "replication/slave_stream_manager.hpp"

namespace replication {

// TODO unit test offsets

slave_t::slave_t(btree_key_value_store_t *internal_store, replication_config_t replication_config,
        failover_config_t failover_config, failover_t *failover) :
    failover_(failover),
    timeout_(INITIAL_TIMEOUT),
    failover_reset_control_(this),
    new_master_control_(this),
    internal_store_(internal_store),
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

    pulsed_when_run_loop_done_.wait();
}

void slave_t::failover_reset() {
    on_thread_t thread_switch(home_thread);
    give_up_.reset();
    timeout_ = INITIAL_TIMEOUT;

    /* If there is an open connection to the master, this will kill it. If we gave up
    on connecting to the master, this will cause us to resume trying to connect. If we
    are in a timeout before retrying the connection, this will cut the timout short. */
    pulse_to_interrupt_run_loop_.pulse_if_non_null();
}

void slave_t::new_master(std::string host, int port) {
    on_thread_t thread_switcher(home_thread);

    /* redo the replication_config info */
    strcpy(replication_config_.hostname, host.c_str());
    replication_config_.port = port;

    failover_reset();
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

void run(slave_t *slave) {

    bool shutting_down = false;
    slave->shutting_down_ = &shutting_down;

    /* Determine if we were connected to the master at the time that we last shut down. We figure
    that if the last timestamp (get_replication_clock()) is the same as the last timestamp that
    we are up to date with the master on (get_last_sync()), then we were connected to the master at
    the time that we shut down. If this is our first time starting up, then both will be 0, so they
    will be equal anyway, which produces the correct behavior because the way we act when we first
    connect is the same as the way we act when we reconnect after we went down. */
    bool were_connected_before =
        slave->internal_store_->get_replication_clock() == slave->internal_store_->get_last_sync();

    if (!were_connected_before) {
        logINF("We didn't have contact with the master at the time that we last shut down, so "
            "we will now resume accepting writes.\n");
        slave->failover_->on_failure();
    }

    while (!shutting_down) {
        try {

            cond_t slave_cond;

            logINF("Attempting to connect as slave to: %s:%d\n",
                slave->replication_config_.hostname, slave->replication_config_.port);

            boost::scoped_ptr<tcp_conn_t> conn(
                new tcp_conn_t(slave->replication_config_.hostname, slave->replication_config_.port));
            slave_stream_manager_t stream_mgr(&conn, slave->internal_store_, &slave_cond);

            // No exception was thrown; it must have worked.
            slave->timeout_ = INITIAL_TIMEOUT;
            slave->give_up_.on_reconnect();
            slave->failover_->on_resume();
            logINF("Connected as slave to: %s:%d\n", slave->replication_config_.hostname, slave->replication_config_.port);

            // This makes it so that if we get a shutdown/reset command, the connection
            // will get closed.
            slave->pulse_to_interrupt_run_loop_.watch(&slave_cond);

            // last_sync is the latest timestamp that we didn't get all the master's changes for
            repli_timestamp last_sync = slave->internal_store_->get_last_sync();
            debugf("Last sync: %d\n", last_sync.time);

            if (!were_connected_before) {
                slave->failover_->on_reverse_backfill_begin();
                // We use last_sync.next() so that we don't send any of the master's own changes back
                // to it. This makes sense in conjunction with incrementing the replication clock when
                // we lose contact with the master.
                stream_mgr.reverse_side_backfill(last_sync.next());
                slave->failover_->on_reverse_backfill_end();
            }

            slave->failover_->on_backfill_begin();
            stream_mgr.backfill(last_sync);
            slave->failover_->on_backfill_end();

            debugf("slave_t: Waiting for things to fail...\n");
            slave_cond.wait();
            debugf("slave_t: Things failed.\n");

            if (shutting_down) {
                break;
            }

            /* Make sure that any sets we run are assigned a timestamp later than the latest
            timestamp we got from the master. */
            repli_timestamp_t rc = slave->internal_store_->get_replication_clock();
            debugf("Incrementing clock from %d to %d\n", rc.time, rc.time+1);
            slave->internal_store_->set_replication_clock(rc.next());

            slave->failover_->on_failure();

            were_connected_before = false;

        } catch (tcp_conn_t::connect_failed_exc_t& e) {
            // If the master was down when we last shut down, it's not so remarkable that it
            // would still be down when we come back up. But if that's not the case and it's
            // our first time connecting, we blame the failure to connect on user error.
            if (were_connected_before) {
                crash("Master at %s:%d is not responding :(. Perhaps you haven't brought it up "
                    "yet. But what do I know, I'm just a database.\n",
                    slave->replication_config_.hostname, slave->replication_config_.port);
            }
        }

        /* The connection has failed. Let's see what we should do */
        if (slave->failover_config_.no_rogue || !slave->give_up_.give_up()) {
            int timeout = slave->timeout_;
            slave->timeout_ = std::min(slave->timeout_ * TIMEOUT_GROWTH_FACTOR, (long)TIMEOUT_CAP);

            cond_t c;
            call_with_delay(timeout, boost::bind(&cond_t::pulse, &c), &c);
            slave->pulse_to_interrupt_run_loop_.watch(&c);
            c.wait();

        } else {
            logINF("Master at %s:%d has failed %d times in the last %d seconds. "
                   "The slave is now going rogue; it will stop trying to reconnect to the master "
                   "and it will accept writes. To resume normal slave behavior, send the command "
                   "\"rethinkdb failover-reset\" (over telnet).\n",
                   slave->replication_config_.hostname, slave->replication_config_.port,
                   MAX_RECONNECTS_PER_N_SECONDS, N_SECONDS);

            cond_t c;
            slave->pulse_to_interrupt_run_loop_.watch(&c);
            c.wait();
        }
    }

    slave->pulsed_when_run_loop_done_.pulse();
}

}  // namespace replication

