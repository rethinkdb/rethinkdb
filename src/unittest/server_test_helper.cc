#include <boost/bind.hpp>

#include "unittest/server_test_helper.hpp"
#include "server/cmd_args.hpp"
#include "serializer/log/log_serializer.hpp"
#include "serializer/translator.hpp"
#include "btree/slice.hpp"

void server_test_helper_t::run() {
    struct starter_t : public thread_message_t {
        server_test_helper_t *server_test;
        starter_t(server_test_helper_t *server_test) : server_test(server_test) { }
        void on_thread_switch() {
            coro_t::spawn(boost::bind(&server_test_helper_t::setup_server_and_run_tests, server_test));
        }
    } starter(this);

    thread_pool.run(&starter);
}

void server_test_helper_t::setup_server_and_run_tests() {
    temp_file_t db_file("/tmp/rdb_unittest.XXXXXX");

    {
        cmd_config_t config;
        config.store_dynamic_config.cache.max_dirty_size = config.store_dynamic_config.cache.max_size / 10;

        // Set ridiculously high flush_* values, so that flush lock doesn't block the txn creation
        config.store_dynamic_config.cache.flush_timer_ms = 1000000;
        config.store_dynamic_config.cache.flush_dirty_size = 1000000000;

        log_serializer_private_dynamic_config_t ser_config;
        ser_config.db_filename = db_file;

        log_serializer_t::create(&config.store_dynamic_config.serializer, &ser_config, &config.store_static_config.serializer);
        log_serializer_t serializer(&config.store_dynamic_config.serializer, &ser_config);

        std::vector<serializer_t *> serializers;
        serializers.push_back(&serializer);
        serializer_multiplexer_t::create(serializers, 1);
        serializer_multiplexer_t multiplexer(serializers);

        btree_slice_t::create(multiplexer.proxies[0], &config.store_static_config.cache);
        btree_slice_t slice(multiplexer.proxies[0], &config.store_dynamic_config.cache);

        cache_t *cache = slice.cache();

        thread_saver_t saver;

        run_tests(saver, cache);
    }
    log_call(thread_pool.shutdown);
}
