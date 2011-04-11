#include "unittest/gtest.hpp"
#include "errors.hpp"
#include "unittest/server_test_helper.hpp"
#include "buffer_cache/transactor.hpp"
#include "buffer_cache/co_functions.hpp"

namespace unittest {

struct snapshots_tester_t : public server_test_helper_t {
protected:
    void run_tests(thread_saver_t& saver, cache_t *cache) {
        // It's nice to see the progress of these tests, so we use log_call
        log_call(test_snapshot_acq_blocks_on_unfinished_create, saver, cache);
        log_call(test_snapshot_sees_changes_started_before_its_first_block_acq, saver, cache);
        log_call(test_snapshot_doesnt_see_later_changes_and_doesnt_block_them, saver, cache);
        log_call(test_snapshot_doesnt_block_or_get_blocked_on_txns_that_acq_first_block_later, saver, cache);
        log_call(test_snapshot_blocks_on_txns_that_acq_first_block_earlier, saver, cache);
        log_call(test_issue_194, saver, cache);
        log_call(test_cow_snapshots, saver, cache);
        log_call(test_double_cow_acq_release, saver, cache);
        log_call(test_cow_delete, saver, cache);
    }

private:
    static void test_snapshot_acq_blocks_on_unfinished_create(thread_saver_t &saver, cache_t *cache) {
        // t0:create(A), t1:snap(), t1:acq(A) blocks, t0:release(A), t1 unblocks, t1 sees the block.
        transactor_t t0(saver, cache, rwi_write, 0, current_time());
        transactor_t t1(saver, cache, rwi_read, 0, repli_timestamp::invalid);

        buf_t *buf0 = create(t0);
        snap(t1);
        bool blocked = false;
        buf_t *buf1 = acq_check_if_blocks_until_buf_released(t1, buf0, rwi_read, true, blocked);
        EXPECT_TRUE(blocked);
        EXPECT_TRUE(buf1 != NULL);
        if (buf1)
            buf1->release();
    }

    static void test_snapshot_sees_changes_started_before_its_first_block_acq(thread_saver_t &saver, cache_t *cache) {
        // t0:create+release(A,B), t1:snap(), t2:acqw(A), t2:change+release(A), t1:acq(A), t1 sees the A change, t1:release(A), t2:acqw(B), t2:change(B), t1:acq(B) blocks, t2:release(B), t1 unblocks, t1 sees the B change
        transactor_t t0(saver, cache, rwi_write, 0, current_time());

        block_id_t block_A, block_B;
        create_two_blocks(t0, block_A, block_B);

        transactor_t t1(saver, cache, rwi_read, 0, repli_timestamp::invalid);
        transactor_t t2(saver, cache, rwi_write, 0, current_time());

        snap(t1);

        buf_t *buf2_A = acq(t2, block_A, rwi_write);
        change_value(buf2_A, changed_value);
        buf2_A->release();

        buf_t *buf1_A = acq(t1, block_A, rwi_read);
        EXPECT_EQ(changed_value, get_value(buf1_A));
        buf1_A->release();

        buf_t *buf2_B = acq(t2, block_B, rwi_write);
        change_value(buf2_B, changed_value);

        bool blocked = false;
        buf_t *buf1_B = acq_check_if_blocks_until_buf_released(t2, buf2_B, rwi_read, true, blocked);
        EXPECT_TRUE(blocked);
        EXPECT_EQ(changed_value, get_value(buf1_B));
        buf1_B->release();
    }

    static void test_snapshot_doesnt_see_later_changes_and_doesnt_block_them(thread_saver_t &saver, cache_t *cache) {
        // t0:create+release(A), t1:snap(), t1:acq(A), t2:acqw(A) doesn't block, t2:change+release(A), t3:snap(), t3:acq(A), t1 doesn't see the change, t3 does see the change
        transactor_t t0(saver, cache, rwi_write, 0, current_time());

        block_id_t block_A, block_B;
        create_two_blocks(t0, block_A, block_B);

        transactor_t t1(saver, cache, rwi_read, 0, repli_timestamp::invalid);
        transactor_t t2(saver, cache, rwi_write, 0, current_time());
        transactor_t t3(saver, cache, rwi_read, 0, repli_timestamp::invalid);

        snap(t1);
        buf_t *buf1 = acq(t1, block_A, rwi_read);

        bool blocked = true;
        buf_t *buf2 = acq_check_if_blocks_until_buf_released(t2, buf1, rwi_write, false, blocked);
        EXPECT_FALSE(blocked);

        change_value(buf2, changed_value);

        snap(t3);
        buf_t *buf3 = acq_check_if_blocks_until_buf_released(t2, buf2, rwi_read, true, blocked);
        EXPECT_TRUE(blocked);

        EXPECT_EQ(init_value, get_value(buf1));
        EXPECT_EQ(changed_value, get_value(buf3));
        buf1->release();
        buf3->release();
    }

    static void test_snapshot_doesnt_block_or_get_blocked_on_txns_that_acq_first_block_later(thread_saver_t &saver, cache_t *cache) {
        // t0:create+release(A,B), t1:snap(), t1:acq(A), t2:acqw(A) doesn't block, t2:acqw(B), t1:acq(B) doesn't block
        transactor_t t0(saver, cache, rwi_write, 0, current_time());

        block_id_t block_A, UNUSED block_B;
        create_two_blocks(t0, block_A, block_B);

        transactor_t t1(saver, cache, rwi_read, 0, repli_timestamp::invalid);
        transactor_t t2(saver, cache, rwi_write, 0, current_time());

        snap(t1);
        buf_t *buf1_A = acq(t1, block_A, rwi_read);

        bool blocked = true;
        buf_t *buf2_A = acq_check_if_blocks_until_buf_released(t2, buf1_A, rwi_write, false, blocked);
        EXPECT_FALSE(blocked);

        buf_t *buf2_B = acq(t2, block_B, rwi_write);

        buf_t *buf1_B = acq_check_if_blocks_until_buf_released(t1, buf2_B, rwi_read, false, blocked);
        EXPECT_FALSE(blocked);

        buf1_A->release();
        buf2_A->release();
        buf1_B->release();
        buf2_B->release();
    }

    static void test_snapshot_blocks_on_txns_that_acq_first_block_earlier(thread_saver_t &saver, cache_t *cache) {
        // t0:create+release(A,B), t1:acqw(A), t1:acqw(B), t1:release(A), t2:snap(), t2:acq+release(A), t2:acq(B) blocks, t1:release(B), t2 unblocks
        transactor_t t0(saver, cache, rwi_write, 0, current_time());

        block_id_t block_A, block_B;
        create_two_blocks(t0, block_A, block_B);

        transactor_t t1(saver, cache, rwi_write, 0, current_time());
        transactor_t t2(saver, cache, rwi_read, 0, repli_timestamp::invalid);

        buf_t *buf1_A = acq(t1, block_A, rwi_write);
        buf_t *buf1_B = acq(t1, block_B, rwi_write);
        change_value(buf1_A, changed_value);
        change_value(buf1_B, changed_value);
        buf1_A->release();

        snap(t2);
        buf_t *buf2_A = acq(t1, block_A, rwi_read);
        EXPECT_EQ(changed_value, get_value(buf2_A));

        buf2_A->release();
        bool blocked = false;
        buf_t *buf2_B = acq_check_if_blocks_until_buf_released(t2, buf1_B, rwi_read, true, blocked);
        EXPECT_TRUE(blocked);
        EXPECT_EQ(changed_value, get_value(buf2_B));
        buf2_B->release();
    }

    static void test_issue_194(thread_saver_t &saver, cache_t *cache) {
        // issue 194 unit-test
        // t0:create+release(A,B), t1:acqw+release(A), t2:acqw(A), t3:snap(), t3:acq(A) blocks, t2:release(A), t1:acqw+release(B), t2:acqw(B), t2:change(B), t3:acq(B) blocks, t2:release(B), t3 unblocks and sees B change
        // (fails on t2:acqw(B) with assertion if issue 194 is not fixed)
        transactor_t t0(saver, cache, rwi_write, 0, current_time());

        block_id_t block_A, block_B;
        create_two_blocks(t0, block_A, block_B);

        transactor_t t1(saver, cache, rwi_write, 0, current_time());
        transactor_t t2(saver, cache, rwi_write, 0, current_time());
        transactor_t t3(saver, cache, rwi_read, 0, repli_timestamp::invalid);

        buf_t *buf1_A = acq(t1, block_A, rwi_write);
        buf1_A->release();

        buf_t *buf2_A = acq(t2, block_A, rwi_write);
        snap(t3);

        bool blocked = false;
        buf_t *buf3_A = acq_check_if_blocks_until_buf_released(t3, buf2_A, rwi_read, true, blocked);
        EXPECT_TRUE(blocked);

        buf_t *buf1_B = acq(t1, block_B, rwi_write);
        buf1_B->release();

        buf_t *buf2_B = acq(t2, block_B, rwi_write);    // if issue 194 is not fixed, expect assertion failure here

        buf3_A->release();

        change_value(buf2_B, changed_value);

        buf_t *buf3_B = acq_check_if_blocks_until_buf_released(t3, buf2_B, rwi_read, true, blocked);
        EXPECT_TRUE(blocked);
        buf3_B->release();
    }

    static void test_cow_snapshots(thread_saver_t &saver, cache_t *cache) {
        // t0:create+release(A,B), t3:acq_outdated_ok(A), t1:acqw(A) doesn't block, t1:change(A), t1:release(A), t2:acqw(A) doesn't block, t2:release(A), t3 doesn't see the change, t3:release(A)
        transactor_t t0(saver, cache, rwi_write, 0, current_time());

        block_id_t block_A, block_B;
        create_two_blocks(t0, block_A, block_B);

        transactor_t t1(saver, cache, rwi_write, 0, current_time());
        transactor_t t2(saver, cache, rwi_write, 0, current_time());
        transactor_t t3(saver, cache, rwi_read, 0, repli_timestamp::invalid);

        buf_t *buf3_A = acq(t3, block_A, rwi_read_outdated_ok);
        uint32_t old_value = get_value(buf3_A);

        bool blocked = true;
        buf_t *buf1_A = acq_check_if_blocks_until_buf_released(t1, buf3_A, rwi_write, false, blocked);
        EXPECT_FALSE(blocked);
        change_value(buf1_A, changed_value);
        buf1_A->release();

        acq_check_if_blocks_until_buf_released(t2, buf3_A, rwi_write, false, blocked)->release();
        EXPECT_FALSE(blocked);

        EXPECT_EQ(old_value, get_value(buf3_A));
        buf3_A->release();
    }

    static void test_double_cow_acq_release(thread_saver_t &saver, cache_t * cache) {
        // t0:create+release(A,B), t1:acq_outdated_ok(A), t2:acq_outdated_ok(A), [t3:acqw(A) doesn't block, t3:delete(A),] t1:release(A), t2:release(A)
        transactor_t t0(saver, cache, rwi_write, 0, current_time());

        block_id_t block_A, block_B;
        create_two_blocks(t0, block_A, block_B);

        transactor_t t1(saver, cache, rwi_read, 0, repli_timestamp::invalid);
        transactor_t t2(saver, cache, rwi_read, 0, repli_timestamp::invalid);

        buf_t *buf1_A = acq(t1, block_A, rwi_read_outdated_ok);
        buf_t *buf2_A = acq(t2, block_A, rwi_read_outdated_ok);

        buf1_A->release();
        buf2_A->release();
    }

    static void test_cow_delete(thread_saver_t &saver, cache_t * cache) {
        // t0:create+release(A,B), t1:acq_outdated_ok(A), t2:acq_outdated_ok(A), t3:acqw(A) doesn't block, t3:delete(A), t1:release(A), t2:release(A)
        transactor_t t0(saver, cache, rwi_write, 0, current_time());

        block_id_t block_A, block_B;
        create_two_blocks(t0, block_A, block_B);

        transactor_t t1(saver, cache, rwi_read, 0, repli_timestamp::invalid);
        transactor_t t2(saver, cache, rwi_read, 0, repli_timestamp::invalid);
        transactor_t t3(saver, cache, rwi_write, 0, current_time());

        buf_t *buf1_A = acq(t1, block_A, rwi_read_outdated_ok);
        buf_t *buf2_A = acq(t2, block_A, rwi_read_outdated_ok);

        const uint32_t old_value = get_value(buf1_A);
        EXPECT_EQ(old_value, get_value(buf2_A));

        bool blocked = true;
        buf_t *buf3_A = acq_check_if_blocks_until_buf_released(t3, buf1_A, rwi_write, false, blocked);
        EXPECT_FALSE(blocked);

        change_value(buf3_A, changed_value);
        buf3_A->mark_deleted();
        buf3_A->release();

        EXPECT_EQ(old_value, get_value(buf1_A));
        buf1_A->release();

        EXPECT_EQ(old_value, get_value(buf2_A));
        buf2_A->release();
    }

};

TEST(SnapshotsTest, all_tests) {
    snapshots_tester_t().run();
}

}  // namespace unittest
