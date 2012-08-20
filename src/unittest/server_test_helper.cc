#include <boost/bind.hpp>

#include "arch/arch.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "mock/unittest_utils.hpp"
#include "serializer/config.hpp"
#include "serializer/log/log_serializer.hpp"
#include "serializer/translator.hpp"
#include "unittest/server_test_helper.hpp"

namespace unittest {

const int server_test_helper_t::init_value = 0x12345678;
const int server_test_helper_t::changed_value = 0x87654321;

server_test_helper_t::server_test_helper_t()
    : serializer(NULL), thread_pool(new thread_pool_t(1, false)) { }

// Destructor defined in the .cc so that thread_pool_t isn't an incomplete type.
server_test_helper_t::~server_test_helper_t() { }

void server_test_helper_t::run() {
    mock::run_in_thread_pool(boost::bind(&server_test_helper_t::setup_server_and_run_tests, this));
}

void server_test_helper_t::setup_server_and_run_tests() {
    mock::temp_file_t db_file("/tmp/rdb_unittest.XXXXXX");

    scoped_ptr_t<io_backender_t> io_backender;
    make_io_backender(aio_default, &io_backender);

    {
        standard_serializer_t::create(
            io_backender.get(),
            standard_serializer_t::private_dynamic_config_t(db_file.name()),
            standard_serializer_t::static_config_t());
        standard_serializer_t log_serializer(
            standard_serializer_t::dynamic_config_t(),
            io_backender.get(),
            standard_serializer_t::private_dynamic_config_t(db_file.name()),
            &get_global_perfmon_collection());

        std::vector<standard_serializer_t *> serializers;
        serializers.push_back(&log_serializer);
        serializer_multiplexer_t::create(serializers, 1);
        serializer_multiplexer_t multiplexer(serializers);

        this->serializer = multiplexer.proxies[0];

        run_serializer_tests();
    }

    trace_call_0(thread_pool->shutdown_thread_pool);
}

void server_test_helper_t::run_serializer_tests() {
    mirrored_cache_static_config_t cache_static_cfg;
    cache_t::create(this->serializer, &cache_static_cfg);
    mirrored_cache_config_t cache_cfg;
    cache_cfg.flush_timer_ms = 1000000;
    cache_cfg.flush_dirty_size = 1000000000;
    cache_cfg.max_size = GIGABYTE;
    cache_t cache(this->serializer, &cache_cfg, &get_global_perfmon_collection());

    nap(200);   // to let patch_disk_storage do writeback.sync();

    run_tests(&cache);
}

void server_test_helper_t::snap(transaction_t *txn) {
    txn->snapshot();
}

void server_test_helper_t::change_value(buf_lock_t *buf, uint32_t value) {
    buf->set_data(const_cast<void *>(buf->get_data_read()), &value, sizeof(value));
}

uint32_t server_test_helper_t::get_value(buf_lock_t *buf) {
    return *reinterpret_cast<const uint32_t *>(buf->get_data_read());
}

bool server_test_helper_t::acq_check_if_blocks_until_buf_released(buf_lock_t *newly_acquired_block, transaction_t *txn, buf_lock_t *already_acquired_block, access_t acquire_mode, bool do_release) {
    acquiring_coro_t acq_coro(newly_acquired_block, txn, already_acquired_block->get_block_id(), acquire_mode);

    coro_t::spawn(boost::bind(&acquiring_coro_t::run, &acq_coro));
    nap(500);
    bool result = !acq_coro.signaled;

    if (do_release) {
        already_acquired_block->release();
        if (result) {
            nap(500);
            rassertf(acq_coro.signaled, "Waiting acquire did not complete following release. May be a bug in the test.");
        }
    }

    rassert(newly_acquired_block->is_acquired());

    return result;
}

void server_test_helper_t::create_two_blocks(transaction_t *txn, block_id_t *block_A, block_id_t *block_B) {
    buf_lock_t buf_A(txn);
    buf_lock_t buf_B(txn);
    *block_A = buf_A.get_block_id();
    *block_B = buf_B.get_block_id();
    change_value(&buf_A, init_value);
    change_value(&buf_B, init_value);
}

void server_test_helper_t::acquiring_coro_t::run() {
    buf_lock_t tmp(txn, block_id, mode);
    result->swap(tmp);
    signaled = true;
}

}  // namespace unittest
