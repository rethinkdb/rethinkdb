#include "slave.hpp"

#include <stdint.h>

#include "arch/linux/coroutines.hpp"
#include "logger.hpp"
#include "net_structs.hpp"
#include "server/key_value_store.hpp"

namespace replication {


// TODO unit test offsets


slave_t::slave_t(btree_key_value_store_t *internal_store, replication_config_t replication_config, failover_config_t failover_config)
    : failover_script_(failover_config.failover_script_path),
      timeout_(INITIAL_TIMEOUT),
      reconnection_timer_token_(NULL),
      failover_reset_control_(std::string("failover reset"), this),
      new_master_control_(std::string("new master"), this),
      internal_store_(internal_store),
      replication_config_(replication_config),
      failover_config_(failover_config),
      stream_(NULL),
      shutting_down_(false)
{
    coro_t::spawn(boost::bind(&run, this));
}

slave_t::~slave_t() {
    shutting_down_ = true;
    coro_t::move_to_thread(home_thread);
    kill_conn();

    /* cancel the timer */
    if (reconnection_timer_token_) {
        cancel_timer(reconnection_timer_token_);
    }
}

void slave_t::kill_conn() {
    {
        on_thread_t thread_switch(home_thread);

        debugf("Calling stream_->co_shutdown.\n");

        stream_->co_shutdown();

        debugf("stream_->co_shutdown has returned.\n");
        //    }

        //    {
        // TODO: why is this going to get_num_threads() - 2 instead of home_thread?  I don't get it.

        //        on_thread_t thread_switch(get_num_threads() - 2);

        delete stream_;
        stream_ = NULL;
    }
}

get_result_t slave_t::get_cas(const store_key_t &key) {
    if (respond_to_queries_) {
        return internal_store_->get_cas(key);
    } else {
        /* TODO: This is a hack. We can't give a valid result because the CAS we return will not be
        usable with the master. But currently there's no way to signal an error on gets. So we
        pretend the value was not found. Technically this is a lie, but screw it. */
        return get_result_t();
    }
}

set_result_t slave_t::sarc(const store_key_t &key, data_provider_t *data, mcflags_t flags, exptime_t exptime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas) {
    if (respond_to_queries_) {
        return internal_store_->sarc(key, data, flags, exptime, add_policy, replace_policy, old_cas);
    } else {
        return sr_not_allowed;
    }
}

incr_decr_result_t slave_t::incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount) {
    if (respond_to_queries_)
        return internal_store_->incr_decr(kind, key, amount);
    else
        return incr_decr_result_t::idr_not_allowed;
}

append_prepend_result_t slave_t::append_prepend(append_prepend_kind_t kind, const store_key_t &key, data_provider_t *data) {
    if (respond_to_queries_)
        return internal_store_->append_prepend(kind, key, data);
    else
        return apr_not_allowed;
}

delete_result_t slave_t::delete_key(const store_key_t &key) {
    if (respond_to_queries_)
        return internal_store_->delete_key(key);
    else
        return dr_not_allowed;
}

std::string slave_t::failover_reset() {
    // TODO don't say get_num_threads() - 2, what does this mean?
    on_thread_t thread_switch(get_num_threads() - 2);
    give_up_.reset();
    timeout_ = INITIAL_TIMEOUT;

    if (reconnection_timer_token_) {
        cancel_timer(reconnection_timer_token_);
        reconnection_timer_token_ = NULL;
    }

    if (stream_) kill_conn(); //this will cause a notify
    else coro_->notify();

    return std::string("Resetting failover\n");
}

 /* message_callback_t interface */
void slave_t::hello(net_hello_t message) {
    debugf("hello message received.\n");
}
void slave_t::send(buffed_data_t<net_backfill_t>& message) {
    rassert(false, "backfill message?  what?\n");
}
void slave_t::send(buffed_data_t<net_announce_t>& message) {
    debugf("announce message received.\n");
}
void slave_t::send(buffed_data_t<net_get_cas_t>& msg) {
    store_key_t key(msg->key_size, msg->key);

    // TODO this returns a get_result_t (with cross-thread messaging),
    // and we don't really care for that.
    internal_store_->get_cas(key, castime_t(msg->proposed_cas, msg->timestamp));
    debugf("get_cas message received and applied.\n");
}
void slave_t::send(stream_pair<net_sarc_t>& msg) {
    store_key_t key(msg->key_size, msg->keyvalue);

    internal_store_->sarc(key, msg.stream, msg->flags, msg->exptime, castime_t(msg->proposed_cas, msg->timestamp), add_policy_t(msg->add_policy), replace_policy_t(msg->replace_policy), msg->old_cas);
    debugf("sarc message received and applied.\n");
}
void slave_t::send(buffed_data_t<net_incr_t>& msg) {
    store_key_t key(msg->key_size, msg->key);

    internal_store_->incr_decr(incr_decr_INCR, key, msg->amount, castime_t(msg->proposed_cas, msg->timestamp));
    debugf("incr message received.\n");
}
void slave_t::send(buffed_data_t<net_decr_t>& msg) {
    store_key_t key(msg->key_size, msg->key);

    internal_store_->incr_decr(incr_decr_DECR, key, msg->amount, castime_t(msg->proposed_cas, msg->timestamp));
    debugf("decr message received and applied.\n");
}
void slave_t::send(stream_pair<net_append_t>& msg) {
    store_key_t key(msg->key_size, msg->keyvalue);

    internal_store_->append_prepend(append_prepend_APPEND, key, msg.stream, castime_t(msg->proposed_cas, msg->timestamp));
    debugf("append message received and applied.\n");
}
void slave_t::send(stream_pair<net_prepend_t>& msg) {
    store_key_t key(msg->key_size, msg->keyvalue);

    internal_store_->append_prepend(append_prepend_PREPEND, key, msg.stream, castime_t(msg->proposed_cas, msg->timestamp));
    debugf("prepend message received and applied.\n");
}
void slave_t::send(buffed_data_t<net_delete_t>& msg) {
    store_key_t key(msg->key_size, msg->key);

    internal_store_->delete_key(key, msg->timestamp);
    debugf("delete message received.\n");
}
void slave_t::send(buffed_data_t<net_nop_t>& message) {
    debugf("nop message received.\n");
    net_ack_t ackreply;
    ackreply.timestamp = message->timestamp;
    stream_->send(ackreply);
    debugf("sent ack reply\n");
}
void slave_t::send(buffed_data_t<net_ack_t>& message) {
    rassert("ack message received.. as slave?\n");
}
void slave_t::send(buffed_data_t<net_shutting_down_t>& message) {
    debugf("shutting_down message received.\n");
}
void slave_t::send(buffed_data_t<net_goodbye_t>& message) {
    debugf("goodbye message received.\n");
}
void slave_t::conn_closed() {
    debugf("conn_closed.\n");
    coro_->notify();
}

/* failover driving functions */
void slave_t::reconnect_timer_callback(void *ctx) {
    slave_t *self = static_cast<slave_t *>(ctx);
    self->coro_->notify();
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

    return (successful_reconnects.size() == MAX_RECONNECTS_PER_N_SECONDS && (successful_reconnects.back() - ticks_to_secs(get_ticks())) < N_SECONDS);
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
    slave->coro_ = coro_t::self();
    slave->failover.add_callback(slave);
    slave->respond_to_queries_ = false;
    bool first_connect = true;
    while (!slave->shutting_down_) {
        try {
            delete slave->stream_;
            slave->stream_ = NULL;

            logINF("Attempting to connect as slave to: %s:%d\n", slave->replication_config_.hostname, slave->replication_config_.port);
            {
                boost::scoped_ptr<tcp_conn_t> conn(new tcp_conn_t(slave->replication_config_.hostname, slave->replication_config_.port));
                slave->stream_ = new repli_stream_t(conn, slave);
            }
            slave->timeout_ = INITIAL_TIMEOUT;
            slave->give_up_.on_reconnect();

            if (!first_connect) {
                slave->failover.on_resume();
            }
            first_connect = false;
            logINF("Connected as slave to: %s:%d\n", slave->replication_config_.hostname, slave->replication_config_.port);


            // wait for things to fail
            coro_t::wait();

            slave->failover.on_failure();

            if (slave->shutting_down_) {
                break;
            }

        } catch (tcp_conn_t::connect_failed_exc_t& e) {
            //Presumably if the master doesn't even accept an initial
            //connection then this is a user error rather than some sort of
            //failure
            if (first_connect) {
                crash("Master at %s:%d is not responding :(. Perhaps you haven't brought it up yet. But what do I know, I'm just a database.\n", slave->replication_config_.hostname, slave->replication_config_.port);
            }
        }

        /* The connection has failed. Let's see what we should do */
        if (!slave->give_up_.give_up()) {
            slave->reconnection_timer_token_ = fire_timer_once(slave->timeout_, &slave_t::reconnect_timer_callback, slave);

            slave->timeout_ = std::min(slave->timeout_ * TIMEOUT_GROWTH_FACTOR, (long)TIMEOUT_CAP);

            coro_t::wait(); //waiting on a timer
            slave->reconnection_timer_token_ = NULL;
        } else {
            logINF("Master at %s:%d has failed %d times in the last %d seconds, "
                   "going rogue. To resume slave behavior send the command "
                   "\"rethinkdb failover reset\" (over telnet).\n",
                   slave->replication_config_.hostname, slave->replication_config_.port,
                   MAX_RECONNECTS_PER_N_SECONDS, N_SECONDS);
            coro_t::wait(); //the only thing that can save us now is a reset
        }
    }
}

std::string slave_t::new_master(int argc, char **argv) {
    guarantee(argc == 3); // TODO: Handle argc = 0.
    string host = string(argv[1]);
    if (host.length() >  MAX_HOSTNAME_LEN - 1)
        return std::string("That hostname is too long; use a shorter one.\n");

    /* redo the replication_config info */
    strcpy(replication_config_.hostname, host.c_str());
    replication_config_.port = atoi(argv[2]); //TODO this is ugly

    failover_reset();

    return std::string("New master set\n");
}

std::string slave_t::failover_reset_control_t::call(int argc, char **argv) {
    return slave->failover_reset();
}

std::string slave_t::new_master_control_t::call(int argc, char **argv) {
    return slave->new_master(argc, argv);
}

}  // namespace replication
