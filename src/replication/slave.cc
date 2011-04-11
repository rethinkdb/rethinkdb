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

slave_t::slave_t(btree_key_value_store_t *internal_store, replication_config_t replication_config, failover_config_t failover_config)
    : failover_script_(failover_config.failover_script_path),
      timeout_(INITIAL_TIMEOUT),
      failover_reset_control_(std::string("failover reset"), this),
      new_master_control_(std::string("new master"), this),
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

    slave->respond_to_queries_ = false;
    bool first_connect = true;
    while (!shutting_down) {
        try {

            multicond_t slave_multicond;

            logINF("Attempting to connect as slave to: %s:%d\n",
                slave->replication_config_.hostname, slave->replication_config_.port);

            boost::scoped_ptr<tcp_conn_t> conn(
                new tcp_conn_t(slave->replication_config_.hostname, slave->replication_config_.port));
            slave_stream_manager_t stream_mgr(&conn, slave->internal_store_, &slave_multicond);

            // No exception was thrown; it must have worked.
            slave->timeout_ = INITIAL_TIMEOUT;
            slave->give_up_.on_reconnect();
            first_connect = false;
            slave->failover.on_resume();
            logINF("Connected as slave to: %s:%d\n", slave->replication_config_.hostname, slave->replication_config_.port);

            // This makes it so that if we get a shutdown/reset command, the connection
            // will get closed.
            slave->pulse_to_interrupt_run_loop_.watch(&slave_multicond);

#ifdef REVERSE_BACKFILLING
            slave->failover.on_reverse_backfill_begin();
            // TODO: this is a fake timestamp!!! You _must_ fix this.
            repli_timestamp fake1 = { 0 };
            stream_mgr.reverse_side_backfill(fake1);
            slave->failover.on_reverse_backfill_end();
#endif

            slave->failover.on_backfill_begin();
            // TODO: another fake timestamp!!! You _must_ fix this.
            repli_timestamp fake2 = { 0 };
            stream_mgr.backfill(fake2);
            slave->failover.on_backfill_end();

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

