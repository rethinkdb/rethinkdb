#include "buffer_cache/alt/page.hpp"
#include "serializer/config.hpp"
#include "unittest/gtest.hpp"
#include "unittest/mock_file.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

using alt::alt_access_t;
using alt::current_page_acq_t;
using alt::page_acq_t;
using alt::page_cache_t;
using alt::page_txn_t;

void run_Nothing() {
}

TEST(PageTest, Nothing) {
    run_in_thread_pool(run_Nothing, 4);
}

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

void run_OneReadAcq() {
    mock_ser_t mock;
    page_cache_t page_cache(mock.ser.get());
    page_txn_t txn(&page_cache);
    current_page_acq_t acq(&txn, 0, alt_access_t::read);
    // Do nothing with the acq.
}

TEST(PageTest, OneReadAcq) {
    run_in_thread_pool(run_OneReadAcq, 4);
}

void run_OneWriteAcq() {
    mock_ser_t mock;
    page_cache_t page_cache(mock.ser.get());
    page_txn_t txn(&page_cache);
    current_page_acq_t acq(&txn, 0, alt_access_t::write);
    // Do nothing with the acq.
}

TEST(PageTest, OneWriteAcq) {
    run_in_thread_pool(run_OneWriteAcq, 4);
}

void run_OneWriteAcqWait() {
    mock_ser_t mock;
    page_cache_t page_cache(mock.ser.get());
    page_txn_t txn(&page_cache);
    current_page_acq_t acq(&txn, alt_access_t::write);
    debugf("acq constructed\n");
    page_acq_t page_acq;
    page_acq.init(acq.current_page_for_write());
    debugf("page_acq initted\n");
    ASSERT_TRUE(page_acq.buf_ready_signal()->is_pulsed());
    debugf("buf size: %" PRIu32 "\n", page_acq.get_buf_size());
    void *buf = page_acq.get_buf_write();
    debugf("page_acq got buf write\n");
    ASSERT_TRUE(buf != NULL);
}

#if 0
TEST(PageTest, OneWriteAcqWait) {
    run_in_thread_pool(run_OneWriteAcqWait, 4);
}
#endif


}  // namespace unittest
