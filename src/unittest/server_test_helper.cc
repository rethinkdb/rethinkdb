// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/server_test_helper.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/io/disk.hpp"
#include "arch/runtime/coroutines.hpp"
#include "arch/timing.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "buffer_cache/global_page_repl.hpp"
#include "unittest/mock_file.hpp"
#include "unittest/unittest_utils.hpp"
#include "serializer/config.hpp"
#include "serializer/log/log_serializer.hpp"
#include "serializer/translator.hpp"

namespace unittest {

const uint32_t server_test_helper_t::init_value = 0x12345678;
const uint32_t server_test_helper_t::changed_value = 0x87654321;

server_test_helper_t::server_test_helper_t()
    : serializer(NULL), mock_file_opener(NULL) { }

server_test_helper_t::~server_test_helper_t() { }

void server_test_helper_t::run() {
    unittest::run_in_thread_pool(boost::bind(&server_test_helper_t::setup_server_and_run_tests, this));
}

void server_test_helper_t::setup_server_and_run_tests() {
    mock_file_opener_t file_opener;
    standard_serializer_t::create(
            &file_opener,
            standard_serializer_t::static_config_t());
    standard_serializer_t log_serializer(
            standard_serializer_t::dynamic_config_t(),
            &file_opener,
            &get_global_perfmon_collection());

    std::vector<standard_serializer_t *> serializers;
    serializers.push_back(&log_serializer);
    serializer_multiplexer_t::create(serializers, 1);
    serializer_multiplexer_t multiplexer(serializers);

    this->mock_file_opener = &file_opener;
    this->serializer = multiplexer.proxies[0];

    run_serializer_tests();

    this->serializer = NULL;
    this->mock_file_opener = NULL;
}

void server_test_helper_t::run_serializer_tests() {
    cache_t::create(this->serializer);

    global_page_repl_t global_page_repl;

    mirrored_cache_config_t cache_cfg;
    cache_cfg.flush_timer_ms = MILLION;
    cache_cfg.flush_dirty_size = BILLION;
    cache_cfg.max_size = GIGABYTE;
    cache_t cache(&global_page_repl, this->serializer, cache_cfg, &get_global_perfmon_collection());

    run_tests(&cache);
}

void server_test_helper_t::snap(transaction_t *txn) {
    txn->snapshot();
}

void server_test_helper_t::change_value(buf_lock_t *buf, uint32_t value) {
    *static_cast<uint32_t *>(buf->get_data_write()) = value;
}

uint32_t server_test_helper_t::get_value(buf_lock_t *buf) {
    return *static_cast<const uint32_t *>(buf->get_data_read());
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
            rassert(acq_coro.signaled, "Waiting acquire did not complete following release. May be a bug in the test.");
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
