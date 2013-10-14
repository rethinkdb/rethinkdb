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
    page_txn_t txn(&page_cache, NULL);
}

TEST(PageTest, OneTxn) {
    run_in_thread_pool(run_OneTxn, 4);
}

void run_TwoTxn() {
    mock_ser_t mock;
    page_cache_t page_cache(mock.ser.get());
    page_txn_t txn1(&page_cache, NULL);
    page_txn_t txn2(&page_cache, NULL);
}

TEST(PageTest, TwoTxn) {
    run_in_thread_pool(run_TwoTxn, 4);
}





}  // namespace unittest
