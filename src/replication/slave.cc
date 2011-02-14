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
      timer_token_(NULL),
      failover_reset_control_(std::string("failover reset"), this),
      new_master_control_(std::string("new master"), this),
      internal_store_(internal_store),
      replication_config_(replication_config),
      failover_config_(failover_config),
      conn_(NULL),
      shutting_down_(false)
{
    coro_t::spawn(boost::bind(&run, this));
}

slave_t::~slave_t() {
    shutting_down_ = true;
    coro_t::move_to_thread(home_thread);
    kill_conn();

    /* cancel the timer */
    if (timer_token_) {
        cancel_timer(timer_token_);
    }
}

void slave_t::kill_conn() {
    struct : public message_parser_t::message_parser_shutdown_callback_t, public cond_t {
            void on_parser_shutdown() { pulse(); }
    } parser_shutdown_cb;

    {
        on_thread_t thread_switch(home_thread);

        if (!parser_.shutdown(&parser_shutdown_cb))
            parser_shutdown_cb.wait();
    }

    { //on_thread_t scope
        on_thread_t thread_switch(get_num_threads() - 2);

        delete conn_;
        conn_ = NULL;
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
    on_thread_t thread_switch(get_num_threads() - 2);
    give_up_.reset();
    timeout_ = INITIAL_TIMEOUT;

    if (timer_token_) {
        cancel_timer(timer_token_);
        timer_token_ = NULL;
    }

    if (conn_) kill_conn(); //this will cause a notify
    else coro_->notify();

    return std::string("Reseting failover\n");
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
    store_key_t key;
    key.size = msg->key_size;
    memcpy(key.contents, msg->key, key.size);

    // TODO this returns a get_result_t (with cross-thread messaging),
    // and we don't really care for that.
    internal_store_->get_cas(key, castime_t(msg->proposed_cas, msg->timestamp));
    debugf("get_cas message received and applied.\n");
}
void slave_t::send(stream_pair<net_sarc_t>& msg) {
    store_key_t key;
    key.size = msg->key_size;
    memcpy(key.contents, msg->keyvalue, key.size);

    internal_store_->sarc(key, msg.stream, msg->flags, msg->exptime, castime_t(msg->proposed_cas, msg->timestamp), add_policy_t(msg->add_policy), replace_policy_t(msg->replace_policy), msg->old_cas);
    debugf("sarc message received.\n");
}
void slave_t::send(buffed_data_t<net_incr_t>& message) {
    debugf("incr message received.\n");
}
void slave_t::send(buffed_data_t<net_decr_t>& message) {
    debugf("decr message received.\n");
}
void slave_t::send(stream_pair<net_append_t>& message) {
    debugf("append message received.\n");
}
void slave_t::send(stream_pair<net_prepend_t>& message) {
    debugf("prepend message received.\n");
}
void slave_t::send(buffed_data_t<net_delete_t>& message) {
    debugf("delete message received.\n");
}
void slave_t::send(buffed_data_t<net_nop_t>& message) {
    debugf("nop message received.\n");
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
    slave_t *self = static_cast<slave_t*>(ctx);
    self->coro_->notify();
}

/* give_up_t interface: the give_up_t struct keeps track of the last N times
 * the server failed. If it's failing to frequently then it will tell us to
 * give up on the server and stop trying to reconnect */
void slave_t::give_up_t::on_reconnect() {
    succesful_reconnects.push(ticks_to_secs(get_ticks()));
    limit_to(MAX_RECONNECTS_PER_N_SECONDS);
}

bool slave_t::give_up_t::give_up() {
    limit_to(MAX_RECONNECTS_PER_N_SECONDS);

    return (succesful_reconnects.size() == MAX_RECONNECTS_PER_N_SECONDS && (succesful_reconnects.back() - ticks_to_secs(get_ticks())) < N_SECONDS);
}

void slave_t::give_up_t::reset() {
    limit_to(0);
}

void slave_t::give_up_t::limit_to(unsigned int limit) {
    while (succesful_reconnects.size() > limit)
        succesful_reconnects.pop();
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
            delete slave->conn_;
            slave->conn_ = NULL;

            logINF("Attempting to connect as slave to: %s:%d\n", slave->replication_config_.hostname, slave->replication_config_.port);
            slave->conn_ = new tcp_conn_t(slave->replication_config_.hostname, slave->replication_config_.port);
            slave->timeout_ = INITIAL_TIMEOUT;
            slave->give_up_.on_reconnect();

            slave->parser_.parse_messages(slave->conn_, slave);
            if (!first_connect) {
                // Call on_resume if we've failed before.
                slave->failover.on_resume();
            }
            first_connect = false;
            logINF("Connected as slave to: %s:%d\n", slave->replication_config_.hostname, slave->replication_config_.port);


            coro_t::wait(); //wait for things to fail
            slave->failover.on_failure();
            if (slave->shutting_down_) break;

        } catch (tcp_conn_t::connect_failed_exc_t& e) {
            //Presumably if the master doesn't even accept an initial
            //connection then this is a user error rather than some sort of
            //failure
            if (first_connect)
                crash("Master at %s:%d is not responding :(. Perhaps you haven't brought it up yet. But what do I know, I'm just a database.\n", slave->replication_config_.hostname, slave->replication_config_.port); 
        }

        /* The connection has failed. Let's see what se should do */
        if (!slave->give_up_.give_up()) {
            slave->timer_token_ = fire_timer_once(slave->timeout_, &slave->reconnect_timer_callback, slave);

            slave->timeout_ *= TIMEOUT_GROWTH_FACTOR;
            if (slave->timeout_ > TIMEOUT_CAP) slave->timeout_ = TIMEOUT_CAP;

            coro_t::wait(); //waiting on a timer
            slave->timer_token_ = NULL;
        } else {
            logINF("Master at %s:%d has failed %d times in the last %d seconds," \
                    "going rogue. To resume slave behavior send the command" \
                    "\"rethinkdb failover reset\" (over telnet).\n",
                    slave->replication_config_.hostname, slave->replication_config_.port,
                    MAX_RECONNECTS_PER_N_SECONDS, N_SECONDS); 
            coro_t::wait(); //the only thing that can save us now is a reset
        }
    }
}

std::string slave_t::new_master(std::string args) {
    std::string host = args.substr(0, args.find(' '));
    if (host.length() >  MAX_HOSTNAME_LEN - 1)
        return std::string("That hostname is soo long, use a shorter one\n");

    /* redo the replication_config info */
    strcpy(replication_config_.hostname, host.c_str());
    replication_config_.port = atoi(strip_spaces(args.substr(args.find(' ') + 1)).c_str()); //TODO this is ugly

    failover_reset();
    
    return std::string("New master set\n");
}

std::string slave_t::failover_reset_control_t::call(std::string) {
    return slave->failover_reset();
}

std::string slave_t::new_master_control_t::call(std::string args) {
    return slave->new_master(args);
}

}  // namespace replication
