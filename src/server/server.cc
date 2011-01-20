#include "server.hpp"
#include "db_thread_info.hpp"
#include "memcached/memcached.hpp"
#include "diskinfo.hpp"
#include "concurrency/cond_var.hpp"
#include "logger.hpp"
#include "server/cmd_args.hpp"

int run_server(int argc, char *argv[]) {

    // Parse command line arguments
    cmd_config_t config = parse_cmd_args(argc, argv);

    // Open the log file, if necessary.
    if (config.log_file_name[0]) {
        log_file = fopen(config.log_file_name, "w");
    }

    // Initial thread message to start server
    struct server_starter_t :
        public thread_message_t
    {
        cmd_config_t *cmd_config;
        thread_pool_t *thread_pool;
        void on_thread_switch() {
            coro_t::spawn(&server_main, cmd_config, thread_pool);
        }
    } starter;
    starter.cmd_config = &config;

    // Run the server.
    thread_pool_t thread_pool(config.n_workers);
    starter.thread_pool = &thread_pool;
    thread_pool.run(&starter);

    logINF("Server is shut down.\n");

    // Close the log file if necessary.
    if (config.log_file_name[0]) {
        fclose(log_file);
        log_file = stderr;
    }

    return 0;
}

void server_main(cmd_config_t *cmd_config, thread_pool_t *thread_pool) {

    /* Create a server object. It only exists so we can give pointers to it to other things. */
    server_t server(cmd_config, thread_pool);

    {
        /* Start logger */
        log_controller_t log_controller;
    
        /* Copy database filenames from private serializer configurations into a single vector of strings */
        std::vector<std::string> db_filenames;
        std::vector<log_serializer_private_dynamic_config_t>& serializer_private = cmd_config->store_dynamic_config.serializer_private;
        std::vector<log_serializer_private_dynamic_config_t>::iterator it;
    
        for (it = serializer_private.begin(); it != serializer_private.end(); ++it) {
            db_filenames.push_back((*it).db_filename);
        }
    
        /* Check to see if there is an existing database */
        struct : public btree_key_value_store_t::check_callback_t, public value_cond_t<bool> {
            void on_store_check(bool ok) { pulse(ok); }
        } check_cb;
        btree_key_value_store_t::check_existing(db_filenames, &check_cb);
        bool existing = check_cb.wait();
        if (existing && cmd_config->create_store && !cmd_config->force_create) {
            fail_due_to_user_error(
                "It looks like there already is a database here. RethinkDB will abort in case you "
                "didn't mean to overwrite it. Run with the '--force' flag to override this warning.");
        } else {
            if (!existing) {
                cmd_config->create_store = true;
            }
        }
    
        /* Start key-value store */
        btree_key_value_store_t store(&cmd_config->store_dynamic_config);
    
        struct : public btree_key_value_store_t::ready_callback_t, public cond_t {
            void on_store_ready() { pulse(); }
        } store_ready_cb;
        if (cmd_config->create_store) {
            logINF("Creating new database...\n");
            if (!store.start_new(&store_ready_cb, &cmd_config->store_static_config)) store_ready_cb.wait();
        } else {
            logINF("Loading existing database...\n");
            if (!store.start_existing(&store_ready_cb)) store_ready_cb.wait();
        }

        server.store = &store;   /* So things can access it */

        /* Record information about disk drives to log file */
        log_disk_info(cmd_config->store_dynamic_config.serializer_private);

        if (!cmd_config->shutdown_after_creation) {

            /* Start connection acceptor */
            struct : public conn_acceptor_t::handler_t {
                server_t *parent;
                void handle(tcp_conn_t *conn) {
                    serve_memcache(conn, parent);
                }
            } handler;
            handler.parent = &server;

            try {
                conn_acceptor_t conn_acceptor(cmd_config->port, &handler);

                logINF("Server is now accepting memcache connections on port %d.\n", cmd_config->port);

                /* Wait for an order to shut down */
                struct : public thread_message_t, public cond_t {
                    void on_thread_switch() { pulse(); }
                } interrupt_cond;
                thread_pool->set_interrupt_message(&interrupt_cond);
                interrupt_cond.wait();

                logINF("Shutting down... this may take time if there is a lot of unsaved data.\n");

            } catch (conn_acceptor_t::address_in_use_exc_t) {
                logERR("Port %d is already in use -- aborting.\n", cmd_config->port);
            }

        } else {
        
            logINF("Shutting down...\n");
        }

        /* Shut down key-value store */
        struct : public btree_key_value_store_t::shutdown_callback_t, public cond_t {
            void on_store_shutdown() { pulse(); }
        } store_shutdown_cb;
        if (!store.shutdown(&store_shutdown_cb)) store_shutdown_cb.wait();
    }

    /* The penultimate step of shutting down is to make sure that all messages
    have reached their destinations so that they can be freed. The way we do this
    is to send one final message to each core; when those messages all get back
    we know that all messages have been processed properly. Otherwise, logger
    shutdown messages would get "stuck" in the message hub when it shut down,
    leading to memory leaks. */
    for (int i = 0; i < get_num_threads(); i++) {
        on_thread_t thread_switcher(i);
    }

    /* Finally tell the thread pool to stop. TODO: Eventually, the thread pool should stop
    automatically when server_main() returns. */
    thread_pool->shutdown();
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
        rassert(callbacks_.size() == 0);

        int num_serializers = server_->cmd_config->store_dynamic_config.serializer_private.size();
        rassert(num_serializers > 0);

        state_ = disabling;

        callbacks_.push_back(cb);

        num_disabled_serializers_ = 0;
        for (int i = 0; i < num_serializers; ++i) {
            standard_serializer_t::gc_disable_callback_t *this_as_callback = this;
            do_on_thread(server_->store->serializers[i]->home_thread, server_->store->serializers[i], &standard_serializer_t::disable_gc, this_as_callback);
        }

        return (state_ == disabled);
    } else if (state_ == disabling) {
        rassert(callbacks_.size() > 0);
        callbacks_.push_back(cb);
        return false;
    } else if (state_ == disabled) {
        rassert(state_ == disabled);
        cb->multiple_users_seen = true;
        cb->on_gc_disabled();
        return true;
    } else {
        rassert(0);
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
    rassert(state_ == disabling);

    int num_serializers = server_->cmd_config->store_dynamic_config.serializer_private.size();

    rassert(num_disabled_serializers_ < num_serializers);

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
