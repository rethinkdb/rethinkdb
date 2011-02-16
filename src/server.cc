#include <algorithm>
#include <math.h>
#include "server.hpp"
#include "db_thread_info.hpp"
#include "memcached/memcached.hpp"
#include "replication/master.hpp"
#include "diskinfo.hpp"

#ifdef TIMEBOMB_DAYS
namespace timebomb {

static const long seconds_in_an_hour = 3600;
static const long seconds_in_a_day = seconds_in_an_hour*24;
static const long timebomb_check_period_in_sec = seconds_in_an_hour * 12;

// Timebomb synchronization code is ugly: we don't want the timer to run when we have cancelled it,
// but it's hard to do, since timers are asynchronous and can execute while we are trying to destroy them.
// We could use a periodic timer, but then scheduling the last alarm precisely would be harder
// (or we would have to use a separate one-shot timer).
static spinlock_t timer_token_lock;
static volatile bool no_more_checking;

struct periodic_checker_t {
    periodic_checker_t(server_t *server, time_t creation_timestamp) : server(server), creation_timestamp(creation_timestamp), timer_token(NULL) {
        no_more_checking = false;
        check(this);
    }

    ~periodic_checker_t() {
        timer_token_lock.lock();
        no_more_checking = true;
        if (timer_token) {
            cancel_timer(const_cast<timer_token_t*>(timer_token));
        } 
        timer_token_lock.unlock();
    }

    static void check(periodic_checker_t *timebomb_checker) {
        timer_token_lock.lock();
        if (!no_more_checking) {
            timebomb_checker->timer_token = NULL;

            bool exploded = false;
            time_t time_now = time(NULL);

            double seconds_since_created = difftime(time_now, timebomb_checker->creation_timestamp);
            if (seconds_since_created < 0) {
                // time anomaly: database created in future (or we are in 2038)
                logERR("Error: Database creation timestamp is in the future.\n");
                exploded = true;
            } else if (seconds_since_created > double(TIMEBOMB_DAYS)*seconds_in_a_day) {
                // trial is over
                logERR("Thank you for evaluating %s. Trial period has expired. To continue using the software, please contact RethinkDB <support@rethinkdb.com>.\n", PRODUCT_NAME);
                exploded = true;
            } else {
                double days_since_created = seconds_since_created / seconds_in_a_day;
                int days_left = ceil(double(TIMEBOMB_DAYS) - days_since_created);
                if (days_left > 1) {
                    logWRN("This is a trial version of %s. It will expire in %d days.\n", PRODUCT_NAME, days_left);
                } else {
                    logWRN("This is a trial version of %s. It will expire today.\n", PRODUCT_NAME);
                }
                exploded = false;
            }
            if (exploded) {
                timebomb_checker->server->shutdown();
            } else {
                // schedule next check
                long seconds_left = ceil(double(TIMEBOMB_DAYS)*seconds_in_a_day - seconds_since_created) + 1;
                long seconds_till_check = std::min(seconds_left, timebomb_check_period_in_sec);
                timebomb_checker->timer_token = fire_timer_once(seconds_till_check * 1000, (void (*)(void*)) &check, timebomb_checker);
            }
        }
        timer_token_lock.unlock();
    }
private:
    server_t *server;
    time_t creation_timestamp;
    volatile timer_token_t *timer_token;
};
}
#endif

server_t::server_t(cmd_config_t *cmd_config, thread_pool_t *thread_pool)
    : cmd_config(cmd_config), thread_pool(thread_pool),
      conn_acceptor(cmd_config->port, &server_t::create_request_handler, (void*)this),
#ifdef REPLICATION_ENABLED
      replication_acceptor(NULL),
#endif
#ifdef TIMEBOMB_DAYS
      timebomb_checker(NULL),
#endif
      toggler(this) { }

void server_t::do_start() {
    assert_thread();
    do_start_loggers();
}

void server_t::do_start_loggers() {
    logger_start(this);
}

void server_t::on_logger_ready() {
    do_check_store();
}

void server_t::do_check_store() {
    /* Copy database filenames from private serializer configurations into a single vector of strings */
    std::vector<std::string> db_filenames;
    std::vector<log_serializer_private_dynamic_config_t>& serializer_private = cmd_config->store_dynamic_config.serializer_private;
    std::vector<log_serializer_private_dynamic_config_t>::iterator it;

    for (it = serializer_private.begin(); it != serializer_private.end(); ++it) {
        db_filenames.push_back((*it).db_filename);
    }
    btree_key_value_store_t::check_existing(db_filenames, this);
}

void server_t::on_store_check(bool ok) {
    if (ok && cmd_config->create_store && !cmd_config->force_create) {
        fail_due_to_user_error(
            "It looks like there already is a database here. RethinkDB will abort in case you "
            "didn't mean to overwrite it. Run with the '--force' flag to override this warning.");
    } else {
        if (!ok) {
            cmd_config->create_store = true;
        }
        do_start_store();
    }
}

void server_t::do_start_store() {
    assert_thread();
    
    store = new btree_key_value_store_t(&cmd_config->store_dynamic_config);
    
    bool done;
    if (cmd_config->create_store) {
        logINF("Creating new database...\n");
        done = store->start_new(this, &cmd_config->store_static_config);
    } else {
        logINF("Loading existing database...\n");
        done = store->start_existing(this);
    }


    if (done) on_store_ready();
}

void server_t::on_store_ready() {
    assert_thread();
    log_disk_info(cmd_config->store_dynamic_config.serializer_private);
    
    if (cmd_config->shutdown_after_creation) {
        logINF("Done creating database.\n");
        do_shutdown_store();
    } else {
        do_start_conn_acceptor();

#ifdef TIMEBOMB_DAYS
        timebomb_checker = new timebomb::periodic_checker_t(this, this->store->creation_timestamp);
#endif
    }
}

void server_t::do_start_conn_acceptor() {
    assert_thread();

    // We've now loaded everything. It's safe to print the config
    // specs (part of them depends on the information loaded from the
    // db file).
    cmd_config->print();
    
    if (!conn_acceptor.start()) {
        // If we got an error condition, shutdown myself
        do_shutdown();
    }
    else
        logINF("Server is now accepting memcached connections on port %d.\n", cmd_config->port);
    

#ifdef REPLICATION_ENABLED
    replication_acceptor = new conn_acceptor_t(cmd_config->port+1, &create_replication_master, (void*)store);
    replication_acceptor->start();
#endif
    
    interrupt_message.server = this;
    thread_pool->set_interrupt_message(&interrupt_message);
}

conn_handler_t *server_t::create_request_handler(conn_acceptor_t *acc, net_conn_t *conn, void *udata) {

    server_t *server = reinterpret_cast<server_t *>(udata);
    assert(acc == &server->conn_acceptor);
    return new txt_memcached_handler_t(&server->conn_acceptor, conn, server);
}

void server_t::shutdown() {
    /* This can be called from any thread! */
    
    thread_message_t *old_interrupt_msg = thread_pool->set_interrupt_message(NULL);
    
    /* If the interrupt message already was NULL, that means that either shutdown() was for
    some reason called before we finished starting up or shutdown() was called twice and this
    is the second time. */
    if (!old_interrupt_msg) return;
    
    if (continue_on_thread(home_thread, old_interrupt_msg))
        call_later_on_this_thread(old_interrupt_msg);
}

void server_t::do_shutdown() {
    logINF("Shutting down...\n");
    
    assert_thread();
#ifdef TIMEBOMB_DAYS
    if (timebomb_checker) {
        delete timebomb_checker;
        timebomb_checker = NULL;
    }
#endif
    do_shutdown_conn_acceptor();
}

void server_t::do_shutdown_conn_acceptor() {
#ifdef REPLICATION_ENABLED
    messages_out = 2;
    if (replication_acceptor->shutdown(this)) on_conn_acceptor_shutdown();
#else
    messages_out = 1;
#endif
    if (conn_acceptor.shutdown(this)) on_conn_acceptor_shutdown();
}

void server_t::on_conn_acceptor_shutdown() {
    assert_thread();
    messages_out--;
    if (messages_out == 0) {
#ifdef REPLICATION_ENABLED
        delete replication_acceptor;
#endif
        do_shutdown_store();
    }
}

void server_t::do_shutdown_store() {
    if (store->shutdown(this)) on_store_shutdown();
}

void server_t::on_store_shutdown() {
    assert_thread();
    delete store;
    store = NULL;
    do_shutdown_loggers();
}

void server_t::do_shutdown_loggers() {
    logger_shutdown(this);
}

void server_t::on_logger_shutdown() {
    assert_thread();
    do_message_flush();
}

/* The penultimate step of shutting down is to make sure that all messages
have reached their destinations so that they can be freed. The way we do this
is to send one final message to each core; when those messages all get back
we know that all messages have been processed properly. Otherwise, logger
shutdown messages would get "stuck" in the message hub when it shut down,
leading to memory leaks. */

struct flush_message_t :
    public thread_message_t
{
    bool returning;
    server_t *server;
    void on_thread_switch() {
        if (returning) {
            server->on_message_flush();
            delete this;
        } else {
            returning = true;
            if (continue_on_thread(server->home_thread, this)) on_thread_switch();
        }
    }
};

void server_t::do_message_flush() {
    messages_out = get_num_threads();
    for (int i = 0; i < get_num_threads(); i++) {
        flush_message_t *m = new flush_message_t();
        m->returning = false;
        m->server = this;
        if (continue_on_thread(i, m)) m->on_thread_switch();
    }
}

void server_t::on_message_flush() {
    messages_out--;
    if (messages_out == 0) {
        do_stop_threads();
    }
}

void server_t::do_stop_threads() {
    assert_thread();
    // This returns immediately, but will cause all of the threads to stop after we
    // return to the event queue.
    thread_pool->shutdown();
    delete this;
}

bool server_t::disable_gc(all_gc_disabled_callback_t *cb) {
    // The callback always gets called.

    return do_on_thread(home_thread, this, &server_t::do_disable_gc, cb);
}

bool server_t::do_disable_gc(all_gc_disabled_callback_t *cb) {
    assert_thread();

    return toggler.disable_gc(cb);
}

bool server_t::enable_gc(all_gc_enabled_callback_t *cb) {
    // The callback always gets called.

    return do_on_thread(home_thread, this, &server_t::do_enable_gc, cb);
}

bool server_t::do_enable_gc(all_gc_enabled_callback_t *cb) {
    assert_thread();

    return toggler.enable_gc(cb);
}


server_t::gc_toggler_t::gc_toggler_t(server_t *server) : state_(enabled), num_disabled_serializers_(0), server_(server) { }

bool server_t::gc_toggler_t::disable_gc(server_t::all_gc_disabled_callback_t *cb) {
    // We _always_ call the callback.

    if (state_ == enabled) {
        assert(callbacks_.size() == 0);

        int num_serializers = server_->cmd_config->store_dynamic_config.serializer_private.size();
        assert(num_serializers > 0);

        state_ = disabling;

        callbacks_.push_back(cb);

        num_disabled_serializers_ = 0;
        for (int i = 0; i < num_serializers; ++i) {
            standard_serializer_t::gc_disable_callback_t *this_as_callback = this;
            do_on_thread(server_->store->serializers[i]->home_thread, server_->store->serializers[i], &standard_serializer_t::disable_gc, this_as_callback);
        }

        return (state_ == disabled);
    } else if (state_ == disabling) {
        assert(callbacks_.size() > 0);
        callbacks_.push_back(cb);
        return false;
    } else if (state_ == disabled) {
        assert(state_ == disabled);
        cb->multiple_users_seen = true;
        cb->on_gc_disabled();
        return true;
    } else {
        assert(0);
        return false;  // Make compiler happy.
    }
}

bool server_t::gc_toggler_t::enable_gc(all_gc_enabled_callback_t *cb) {
    // Always calls the callback.

    int num_serializers = server_->cmd_config->store_dynamic_config.serializer_private.size();

    // The return value of serializer_t::enable_gc is always true.

    for (int i = 0; i < num_serializers; ++i) {
        do_on_thread(server_->store->serializers[i]->home_thread, server_->store->serializers[i], &standard_serializer_t::enable_gc);
    }

    if (state_ == enabled) {
        cb->multiple_users_seen = true;
    } else if (state_ == disabling) {
        state_ = enabled;

        // We tell the people requesting a disable that the disabling
        // process was completed, but that multiple people are using
        // it.
        for (callback_vector_t::iterator p = callbacks_.begin(), e = callbacks_.end(); p < e; ++p) {
            (*p)->multiple_users_seen = true;
            (*p)->on_gc_disabled();
        }

        callbacks_.clear();

        cb->multiple_users_seen = true;
    } else if (state_ == disabled) {
        state_ = enabled;
        cb->multiple_users_seen = false;
    }
    cb->on_gc_enabled();

    return true;
}

void server_t::gc_toggler_t::on_gc_disabled() {
    assert(state_ == disabling);

    int num_serializers = server_->cmd_config->store_dynamic_config.serializer_private.size();

    assert(num_disabled_serializers_ < num_serializers);

    num_disabled_serializers_++;
    if (num_disabled_serializers_ == num_serializers) {
        // We are no longer disabling, we're now disabled!

        bool multiple_disablers_seen = (callbacks_.size() > 1);

        for (callback_vector_t::iterator p = callbacks_.begin(), e = callbacks_.end(); p < e; ++p) {
            (*p)->multiple_users_seen = multiple_disablers_seen;
            (*p)->on_gc_disabled();
        }

        callbacks_.clear();

        state_ = disabled;
    }
}
