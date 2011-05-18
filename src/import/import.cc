#include "arch/arch.hpp"
#include "import/import.hpp"
#include <math.h>
#include "db_thread_info.hpp"
#include "memcached/memcached.hpp"
#include "diskinfo.hpp"
#include "concurrency/cond_var.hpp"
#include "logger.hpp"
#include "replication/master.hpp"
#include "replication/slave.hpp"
#include "replication/load_balancer.hpp"
#include "gated_store.hpp"
#include "concurrency/promise.hpp"

namespace import {

void usage() {
}

/* cmd_config_t parse_cmd_args(int argc, char *argv[]) {
    return cmd_config_t()
} */

void import_main(cmd_config_t *cmd_config, thread_pool_t *thread_pool) {
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
        struct : public btree_key_value_store_t::check_callback_t, public promise_t<bool> {
            void on_store_check(bool ok) { pulse(ok); }
        } check_cb;
        btree_key_value_store_t::check_existing(db_filenames, &check_cb);
        bool existing = check_cb.wait();
        if (!existing) {
            logINF("Creating database...\n");
            btree_key_value_store_t::create(&cmd_config->store_dynamic_config,
                                            &cmd_config->store_static_config);
            logINF("Done creating.\n");
        }


        order_source_pigeoncoop_t pigeoncoop(MEMCACHE_START_BUCKET);

        /* Start key-value store */
        logINF("Loading database...\n");
        btree_key_value_store_t store(&cmd_config->store_dynamic_config);

        logINF("Importing memcached commands..\n");
        order_source_t order_source(&pigeoncoop);
        import_memcache(cmd_config->memcached_file, &store, &order_source);

        logINF("Waiting for running operations to finish...\n");

        logINF("Waiting for changes to flush to disk...\n");
        // Store destructor called here
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

} //namespace import

/* int run_import(int argc, char *argv[]) {
    return 0;
} */

