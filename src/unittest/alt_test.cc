#include "buffer_cache/alt/page.hpp"
#include "serializer/config.hpp"
#include "unittest/gtest.hpp"
#include "unittest/mock_file.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

using alt::page_cache_t;
using alt::page_txn_t;

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

void run_Control() {
    mock_ser_t ser;
}

TEST(PageTest, Control) {
    run_in_thread_pool(run_Control, 4);
}

void run_CreateDestroy() {
    mock_ser_t mock;
    page_cache_t page_cache(mock.ser.get());
}

TEST(PageTest, CreateDestroy) {
    run_in_thread_pool(run_CreateDestroy, 4);
}

void run_OneTxn() {
    mock_ser_t mock;
    page_cache_t page_cache(mock.ser.get());
    page_txn_t txn(&page_cache);
}

TEST(PageTest, OneTxn) {
    run_in_thread_pool(run_OneTxn, 4);
}

void run_TwoIndependentTxn() {
    mock_ser_t mock;
    page_cache_t page_cache(mock.ser.get());
    page_txn_t txn1(&page_cache);
    page_txn_t txn2(&page_cache);
}

TEST(PageTest, TwoIndependentTxn) {
    run_in_thread_pool(run_TwoIndependentTxn, 4);
}

void run_TwoIndependentTxnSwitch() {
    mock_ser_t mock;
    page_cache_t page_cache(mock.ser.get());
    auto txn1 = make_scoped<page_txn_t>(&page_cache);
    page_txn_t txn2(&page_cache);
    txn1.reset();
}

TEST(PageTest, TwoIndependentTxnSwitch) {
    run_in_thread_pool(run_TwoIndependentTxnSwitch, 4);
}

void run_TwoSequentialTxnSwitch() {
    mock_ser_t mock;
    page_cache_t page_cache(mock.ser.get());
    auto txn1 = make_scoped<page_txn_t>(&page_cache);
    page_txn_t txn2(&page_cache, txn1.get());
    txn1.reset();
}

TEST(PageTest, TwoSequentialTxnSwitch) {
    run_in_thread_pool(run_TwoSequentialTxnSwitch, 4);
}



}  // namespace unittest
