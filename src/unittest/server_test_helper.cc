#include <boost/bind.hpp>

#include "unittest/server_test_helper.hpp"
#include "unittest/unittest_utils.hpp"
#include "server/cmd_args.hpp"
#include "serializer/log/log_serializer.hpp"
#include "serializer/translator.hpp"
#include "btree/slice.hpp"

const int server_test_helper_t::init_value = 0x12345678;
const int server_test_helper_t::changed_value = 0x87654321;

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
        ser_config.db_filename = db_file.name();

        log_serializer_t::create(&config.store_dynamic_config.serializer, &ser_config, &config.store_static_config.serializer);
        log_serializer_t log_serializer(&config.store_dynamic_config.serializer, &ser_config);

        std::vector<serializer_t *> serializers;
        serializers.push_back(&log_serializer);
        serializer_multiplexer_t::create(serializers, 1);
        serializer_multiplexer_t multiplexer(serializers);

        this->serializer = multiplexer.proxies[0];
        btree_slice_t::create(this->serializer, &config.store_static_config.cache);
        btree_slice_t slice(this->serializer, &config.store_dynamic_config.cache, MEGABYTE);

        cache_t *cache = slice.cache();

        thread_saver_t saver;

        nap(100);   // to let patch_disk_storage do writeback.sync();

        run_tests(saver, cache);
    }
    trace_call(thread_pool.shutdown);
}

// Static helper functions
buf_t *server_test_helper_t::create(transactor_t& txor) {
    return txor.get()->allocate();
}

void server_test_helper_t::snap(transactor_t& txor) {
    txor.get()->snapshot();
}

buf_t * server_test_helper_t::acq(transactor_t& txor, block_id_t block_id, access_t mode) {
    thread_saver_t saver;
    return co_acquire_block(saver, txor.get(), block_id, mode);
}

void server_test_helper_t::change_value(buf_t *buf, uint32_t value) {
    buf->set_data(const_cast<void *>(buf->get_data_read()), &value, sizeof(value));
}

uint32_t server_test_helper_t::get_value(buf_t *buf) {
    return *((uint32_t*) buf->get_data_read());
}

buf_t *server_test_helper_t::acq_check_if_blocks_until_buf_released(transactor_t& acquiring_txor, buf_t *already_acquired_block, access_t acquire_mode, bool do_release, bool &blocked) {
    acquiring_coro_t acq_coro(acquiring_txor, already_acquired_block->get_block_id(), acquire_mode);

    coro_t::spawn(boost::bind(&acquiring_coro_t::run, &acq_coro));
    nap(500);
    blocked = !acq_coro.signaled;

    if (do_release) {
        already_acquired_block->release();
        if (blocked) {
            nap(500);
            rassert(acq_coro.signaled, "Buf release must have unblocked the coroutine trying to acquire the buf. May be a bug in the test.");
        }
    }

    return acq_coro.result;
}

void server_test_helper_t::create_two_blocks(transactor_t &txor, block_id_t &block_A, block_id_t &block_B) {
    buf_t *buf_A = create(txor);
    buf_t *buf_B = create(txor);
    block_A = buf_A->get_block_id();
    block_B = buf_B->get_block_id();
    change_value(buf_A, init_value);
    change_value(buf_B, init_value);
    buf_A->release();
    buf_B->release();
}
