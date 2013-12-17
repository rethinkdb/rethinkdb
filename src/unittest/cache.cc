#include "arch/runtime/coroutines.hpp"
#include "arch/timing.hpp"
#include "buffer_cache/alt/page.hpp"
#include "buffer_cache/alt/alt.hpp"
#include "concurrency/auto_drainer.hpp"
#include "serializer/config.hpp"
#include "unittest/gtest.hpp"
#include "unittest/mock_file.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

using alt::alt_cache_t;
using alt::alt_txn_t;

// RSI this is copy pasted from unittest/page_test.cc put it somewhere general
struct mock_ser_t {
    mock_file_opener_t opener;
    scoped_ptr_t<standard_serializer_t> ser;

    mock_ser_t()
        : opener() {
        standard_serializer_t::create(&opener,
                                      standard_serializer_t::static_config_t());
        ser = make_scoped<standard_serializer_t>(log_serializer_t::dynamic_config_t(),
                                                 &opener,
                                                 &get_global_perfmon_collection());
    }
};

void run_CacheCreateDestroy() {
    mock_ser_t mock;
    alt_cache_t cache(mock.ser.get());
}

TEST(Cache, CreateDestroy) {
    run_in_thread_pool(run_CacheCreateDestroy, 4);
}

void run_MakeTxns() {
    mock_ser_t mock;
    alt_cache_t cache(mock.ser.get());

    alt_txn_t wtxn1(&cache, write_durability_t::HARD);
    alt_txn_t wtxn2(&cache, write_durability_t::SOFT);

    alt_txn_t rtxn1(&cache);
    alt_txn_t rtxn2(&cache);
}

void delete_txn(alt_txn_t *txn) {
    delete txn;
}

void run_TxnPredecessors() {
    mock_ser_t mock;
    alt_cache_t cache(mock.ser.get());

    auto wtxn1 = new alt_txn_t(&cache, write_durability_t::HARD);
    coro_t::spawn(std::bind(&delete_txn, wtxn1));
    alt_txn_t wtxn2(&cache, write_durability_t::SOFT, wtxn1);

    auto rtxn1 = new alt_txn_t(&cache);
    coro_t::spawn(std::bind(&delete_txn, rtxn1));
    alt_txn_t rtxn2(&cache, rtxn1);
}

TEST(Cache, TxnPredecessors) {
    run_in_thread_pool(run_TxnPredecessors, 4);
}

} //namespace unittest 
