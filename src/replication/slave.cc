#include "slave.hpp"

#include <stdint.h>

#include "arch/linux/coroutines.hpp"
#include "logger.hpp"
#include "net_structs.hpp"
#include "server/key_value_store.hpp"

namespace replication {


// TODO unit test offsets


slave_t::slave_t(btree_key_value_store_t *internal_store_, replication_config_t replication_config_, failover_config_t failover_config_)
    : failover_script(failover_config_.failover_script_path),
      timeout(INITIAL_TIMEOUT),
      timer_token(NULL),
      failover_reset_control(std::string("failover_reset"), this),
      new_master_control(std::string("new_master"), this),
      internal_store(internal_store_),
      replication_config(replication_config_),
      failover_config(failover_config_),
      conn(NULL),
      shutting_down(false)
{
    coro_t::spawn(boost::bind(&run, this));
}

slave_t::~slave_t() {
    shutting_down = true;
    coro_t::move_to_thread(home_thread);
    kill_conn();
    
    /* cancel the timer */
    if(timer_token) cancel_timer(timer_token);
}

void slave_t::kill_conn() {
    struct : public message_parser_t::message_parser_shutdown_callback_t, public cond_t {
            void on_parser_shutdown() { pulse(); }
    } parser_shutdown_cb;

    {
        on_thread_t thread_switch(home_thread);

        if (!parser.shutdown(&parser_shutdown_cb))
            parser_shutdown_cb.wait();
    }

    { //on_thread_t scope
        on_thread_t thread_switch(get_num_threads() - 2);
        if (conn) {
            delete conn;
            conn = NULL;
        }
    }
}

struct not_allowed_visitor_t : public boost::static_visitor<mutation_result_t> {
    mutation_result_t operator()(UNUSED const get_cas_mutation_t& m) const {
        /* TODO: This is a hack. We can't give a valid result because the CAS we return will
        not be usable with the master. But currently there's no way to signal an error on
        gets. So we pretend the value was not found. Technically this is a lie, but screw
        it. */
        return get_result_t();
    }
    mutation_result_t operator()(UNUSED const set_mutation_t& m) const {
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

    if (respond_to_queries) {
        return internal_store->change(m);

    } else {
        /* Construct a "failure" response of the right sort */
        return boost::apply_visitor(not_allowed_visitor_t(), m.mutation);
    }
}

std::string slave_t::failover_reset() {
    on_thread_t thread_switch(get_num_threads() - 2);
    give_up.reset();
    timeout = INITIAL_TIMEOUT;

    if (timer_token) {
        cancel_timer(timer_token);
        timer_token = NULL;
    }

    if (conn) kill_conn(); //this will cause a notify
    else coro->notify();

    return std::string("Reseting failover\n");
}

// TODO: What are these functions, and why are they empty?

/* message_callback_t interface */
void slave_t::hello(UNUSED net_hello_t message) { }
void slave_t::send(UNUSED buffed_data_t<net_backfill_t>& message) { }
void slave_t::send(UNUSED buffed_data_t<net_announce_t>& message) { }
void slave_t::send(UNUSED buffed_data_t<net_get_cas_t>& message) { }
void slave_t::send(UNUSED stream_pair<net_set_t>& message) { }
void slave_t::send(UNUSED stream_pair<net_add_t>& message) { }
void slave_t::send(UNUSED stream_pair<net_replace_t>& message) { }
void slave_t::send(UNUSED stream_pair<net_cas_t>& message) { }
void slave_t::send(UNUSED buffed_data_t<net_incr_t>& message) { }
void slave_t::send(UNUSED buffed_data_t<net_decr_t>& message) { }
void slave_t::send(UNUSED stream_pair<net_append_t>& message) { }
void slave_t::send(UNUSED stream_pair<net_prepend_t>& message) { }
void slave_t::send(UNUSED buffed_data_t<net_delete_t>& message) { }
void slave_t::send(UNUSED buffed_data_t<net_nop_t>& message) { }
void slave_t::send(UNUSED buffed_data_t<net_ack_t>& message) { }
void slave_t::send(UNUSED buffed_data_t<net_shutting_down_t>& message) { }
void slave_t::send(UNUSED buffed_data_t<net_goodbye_t>& message) { }
void slave_t::conn_closed() {
    coro->notify();
}

/* failover driving functions */
void slave_t::reconnect_timer_callback(void *ctx) {
    slave_t *self = static_cast<slave_t*>(ctx);
    self->coro->notify();
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
    respond_to_queries = true;
}

void slave_t::on_resume() {
    respond_to_queries = false;
}

void run(slave_t *slave) {
    slave->coro = coro_t::self();
    slave->failover.add_callback(slave);
    slave->respond_to_queries = false;
    bool first_connect = true;
    while (!slave->shutting_down) {
        try {
            if (slave->conn) { 
                delete slave->conn;
                slave->conn = NULL;
            }
            coro_t::move_to_thread(get_num_threads() - 2);
            logINF("Attempting to connect as slave to: %s:%d\n", slave->replication_config.hostname, slave->replication_config.port);
            slave->conn = new tcp_conn_t(slave->replication_config.hostname, slave->replication_config.port);
            slave->timeout = INITIAL_TIMEOUT;
            slave->give_up.on_reconnect();

            slave->parser.parse_messages(slave->conn, slave);
            if (!first_connect) //UGLY :( but it's either this or code duplication
                slave->failover.on_resume(); //Call on_resume if we've failed before
            first_connect = false;
            logINF("Connected as slave to: %s:%d\n", slave->replication_config.hostname, slave->replication_config.port);


            coro_t::wait(); //wait for things to fail
            slave->failover.on_failure();
            if (slave->shutting_down) break;

        } catch (tcp_conn_t::connect_failed_exc_t& e) {
            //Presumably if the master doesn't even accept an initial
            //connection then this is a user error rather than some sort of
            //failure
            if (first_connect)
                crash("Master at %s:%d is not responding :(. Perhaps you haven't brought it up yet. But what do I know I'm just a database\n", slave->replication_config.hostname, slave->replication_config.port); 
        }

        /* The connection has failed. Let's see what se should do */
        if (!slave->give_up.give_up()) {
            slave->timer_token = fire_timer_once(slave->timeout, &slave->reconnect_timer_callback, slave);

            slave->timeout *= TIMEOUT_GROWTH_FACTOR;
            if (slave->timeout > TIMEOUT_CAP) slave->timeout = TIMEOUT_CAP;

            coro_t::wait(); //waiting on a timer
            slave->timer_token = NULL;
        } else {
            logINF("Master at %s:%d has failed %d times in the last %d seconds," \
                    "going rogue. To resume slave behavior send the command" \
                    "\"rethinkdb failover reset\" (over telnet)\n",
                    slave->replication_config.hostname, slave->replication_config.port,
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
    strcpy(replication_config.hostname, host.c_str());
    replication_config.port = atoi(argv[2]); //TODO this is ugly

    failover_reset();
    
    return std::string("New master set\n");
}

// TODO: instead of UNUSED, ensure that these parameters are empty.
std::string slave_t::failover_reset_control_t::call(UNUSED int argc, UNUSED char **argv) {
    return slave->failover_reset();
}

std::string slave_t::new_master_control_t::call(UNUSED int argc, UNUSED char **argv) {
    return slave->new_master(argc, argv);
}

}  // namespace replication
