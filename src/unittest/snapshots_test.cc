#include "errors.hpp"
#include "unittest/gtest.hpp"
#include "unittest/server_test_helper.hpp"
#include "mock/unittest_utils.hpp"
#include "buffer_cache/buffer_cache.hpp"

namespace unittest {

class snapshots_tester_t : public server_test_helper_t {
public:
    snapshots_tester_t() { }
private:
    void run_tests(cache_t *cache) {
        // It's nice to see the progress of these tests, so we use trace_call
        trace_call(test_snapshot_acq_blocks_on_unfinished_create, cache);
        trace_call(test_snapshot_sees_changes_started_before_its_first_block_acq, cache);
        trace_call(test_snapshot_doesnt_see_later_changes_and_doesnt_block_them, cache);
        trace_call(test_snapshot_doesnt_block_or_get_blocked_on_txns_that_acq_first_block_later, cache);
        trace_call(test_snapshot_blocks_on_txns_that_acq_first_block_earlier, cache);
        trace_call(test_issue_194, cache);
        trace_call(test_cow_snapshots, cache);
        trace_call(test_double_cow_acq_release, cache);
        trace_call(test_cow_delete, cache);
    }

    static void test_snapshot_acq_blocks_on_unfinished_create(cache_t *cache) {
        // t0:create(A), t1:snap(), t1:acq(A) blocks, t0:release(A), t1 unblocks, t1 sees the block.
        order_source_t order_source;
        transaction_t t0(cache, rwi_write, 0, repli_timestamp_t::distant_past,
                         order_source.check_in("test_snapshot_acq_blocks_on_unfinished_create(t0)"));
        transaction_t t1(cache, rwi_read, 0, repli_timestamp_t::invalid,
                         order_source.check_in("test_snapshot_acq_blocks_on_unfinished_create(t1)").with_read_mode());

        buf_lock_t buf0(&t0);
        buf_lock_t buf1;
        snap(&t1);
        EXPECT_TRUE(acq_check_if_blocks_until_buf_released(&buf1, &t1, &buf0, rwi_read, true));
    }

    static void test_snapshot_sees_changes_started_before_its_first_block_acq(cache_t *cache) {
        // t0:create+release(A,B), t1:snap(), t2:acqw(A), t2:change+release(A), t1:acq(A), t1 sees the A change, t1:release(A), t2:acqw(B), t2:change(B), t1:acq(B) blocks, t2:release(B), t1 unblocks, t1 sees the B change
        order_source_t order_source;
        transaction_t t0(cache, rwi_write, 0, repli_timestamp_t::distant_past,
                         order_source.check_in("test_snapshot_sees_changes_started_before_its_first_block_acq(t0)"));

        block_id_t block_A, block_B;
        create_two_blocks(&t0, &block_A, &block_B);

        transaction_t t1(cache, rwi_read, 0, repli_timestamp_t::invalid,
                         order_source.check_in("test_snapshot_sees_changes_started_before_its_first_block_acq(t1)").with_read_mode());
        transaction_t t2(cache, rwi_write, 0, repli_timestamp_t::distant_past,
                         order_source.check_in("test_snapshot_sees_changes_started_before_its_first_block_acq(t2)"));

        snap(&t1);

        buf_lock_t buf2_A(&t2, block_A, rwi_write);
        change_value(&buf2_A, changed_value);
        buf2_A.release();

        buf_lock_t buf1_A(&t1, block_A, rwi_read);
        EXPECT_EQ(changed_value, get_value(&buf1_A));
        buf1_A.release();

        buf_lock_t buf2_B(&t2, block_B, rwi_write);
        change_value(&buf2_B, changed_value);

        buf_lock_t buf1_B;
        EXPECT_TRUE(acq_check_if_blocks_until_buf_released(&buf1_B, &t2, &buf2_B, rwi_read, true));
        EXPECT_EQ(changed_value, get_value(&buf1_B));
    }

    static void test_snapshot_doesnt_see_later_changes_and_doesnt_block_them(cache_t *cache) {
        // t0:create+release(A), t1:snap(), t1:acq(A), t2:acqw(A) doesn't block, t2:change+release(A), t3:snap(), t3:acq(A), t1 doesn't see the change, t3 does see the change
        order_source_t order_source;
        transaction_t t0(cache, rwi_write, 0, repli_timestamp_t::distant_past,
                         order_source.check_in("test_snapshot_doesnt_see_later_changes_and_doesnt_block_them(t0)"));

        block_id_t block_A, block_B;
        create_two_blocks(&t0, &block_A, &block_B);

        transaction_t t1(cache, rwi_read, 0, repli_timestamp_t::invalid,
                         order_source.check_in("test_snapshot_doesnt_see_later_changes_and_doesnt_block_them(t1)").with_read_mode());
        transaction_t t2(cache, rwi_write, 0, repli_timestamp_t::distant_past,
                         order_source.check_in("test_snapshot_doesnt_see_later_changes_and_doesnt_block_them(t2)"));
        transaction_t t3(cache, rwi_read, 0, repli_timestamp_t::invalid,
                         order_source.check_in("test_snapshot_doesnt_see_later_changes_and_doesnt_block_them(t3)").with_read_mode());

        snap(&t1);
        buf_lock_t buf1(&t1, block_A, rwi_read);

        buf_lock_t buf2;
        EXPECT_FALSE(acq_check_if_blocks_until_buf_released(&buf2, &t2, &buf1, rwi_write, false));

        change_value(&buf2, changed_value);

        snap(&t3);
        buf_lock_t buf3;
        EXPECT_TRUE(acq_check_if_blocks_until_buf_released(&buf3, &t2, &buf2, rwi_read, true));

        EXPECT_EQ(init_value, get_value(&buf1));
        EXPECT_EQ(changed_value, get_value(&buf3));
    }

    static void test_snapshot_doesnt_block_or_get_blocked_on_txns_that_acq_first_block_later(cache_t *cache) {
        // t0:create+release(A,B), t1:snap(), t1:acq(A), t2:acqw(A) doesn't block, t2:acqw(B), t1:acq(B) doesn't block
        order_source_t order_source;
        transaction_t t0(cache, rwi_write, 0, repli_timestamp_t::distant_past,
                         order_source.check_in("test_snapshot_doesnt_block_or_get_blocked_on_txns_that_acq_first_block_later(t0)"));

        block_id_t block_A, UNUSED block_B;
        create_two_blocks(&t0, &block_A, &block_B);

        transaction_t t1(cache, rwi_read, 0, repli_timestamp_t::invalid,
                         order_source.check_in("test_snapshot_doesnt_block_or_get_blocked_on_txns_that_acq_first_block_later(t1)").with_read_mode());
        transaction_t t2(cache, rwi_write, 0, repli_timestamp_t::distant_past,
                         order_source.check_in("test_snapshot_doesnt_block_or_get_blocked_on_txns_that_acq_first_block_later(t2)"));

        snap(&t1);
        buf_lock_t buf1_A(&t1, block_A, rwi_read);
        buf_lock_t buf2_A;
        EXPECT_FALSE(acq_check_if_blocks_until_buf_released(&buf2_A, &t2, &buf1_A, rwi_write, false));

        buf_lock_t buf1_B(&t2, block_B, rwi_write);
        buf_lock_t buf2_B;
        EXPECT_FALSE(acq_check_if_blocks_until_buf_released(&buf2_B, &t1, &buf1_B, rwi_read, false));
    }

    static void test_snapshot_blocks_on_txns_that_acq_first_block_earlier(cache_t *cache) {
        // t0:create+release(A,B), t1:acqw(A), t1:acqw(B), t1:release(A), t2:snap(), t2:acq+release(A), t2:acq(B) blocks, t1:release(B), t2 unblocks
        order_source_t order_source;
        transaction_t t0(cache, rwi_write, 0, repli_timestamp_t::distant_past,
                         order_source.check_in("test_snapshot_blocks_on_txns_that_acq_first_block_earlier(t0)"));

        block_id_t block_A, block_B;
        create_two_blocks(&t0, &block_A, &block_B);

        transaction_t t1(cache, rwi_write, 0, repli_timestamp_t::distant_past,
                         order_source.check_in("test_snapshot_blocks_on_txns_that_acq_first_block_earlier(t1)"));
        transaction_t t2(cache, rwi_read, 0, repli_timestamp_t::invalid,
                         order_source.check_in("test_snapshot_blocks_on_txns_that_acq_first_block_earlier(t2)").with_read_mode());

        buf_lock_t buf1_A(&t1, block_A, rwi_write);
        buf_lock_t buf1_B(&t1, block_B, rwi_write);
        change_value(&buf1_A, changed_value);
        change_value(&buf1_B, changed_value);
        buf1_A.release();

        snap(&t2);
        buf_lock_t buf2_A(&t1, block_A, rwi_read);
        EXPECT_EQ(changed_value, get_value(&buf2_A));
        buf2_A.release();

        buf_lock_t buf2_B;
        EXPECT_TRUE(acq_check_if_blocks_until_buf_released(&buf2_B, &t2, &buf1_B, rwi_read, true));
        EXPECT_EQ(changed_value, get_value(&buf2_B));
    }

    static void test_issue_194(cache_t *cache) {
        // issue 194 unit-test
        // t0:create+release(A,B), t1:acqw+release(A), t2:acqw(A), t3:snap(), t3:acq(A) blocks, t2:release(A), t1:acqw+release(B), t2:acqw(B), t2:change(B), t3:acq(B) blocks, t2:release(B), t3 unblocks and sees B change
        // (fails on t2:acqw(B) with assertion if issue 194 is not fixed)
        order_source_t order_source;
        transaction_t t0(cache, rwi_write, 0, repli_timestamp_t::distant_past,
                         order_source.check_in("test_issue_194(t0)"));

        block_id_t block_A, block_B;
        create_two_blocks(&t0, &block_A, &block_B);

        transaction_t t1(cache, rwi_write, 0, repli_timestamp_t::distant_past,
                         order_source.check_in("test_issue_194(t1)"));
        transaction_t t2(cache, rwi_write, 0, repli_timestamp_t::distant_past,
                         order_source.check_in("test_issue_194(t2)"));
        transaction_t t3(cache, rwi_read, 0, repli_timestamp_t::invalid,
                         order_source.check_in("test_issue_194(t3)").with_read_mode());

        buf_lock_t buf1_A(&t1, block_A, rwi_write);
        buf1_A.release();

        buf_lock_t buf2_A(&t2, block_A, rwi_write);
        snap(&t3);

        buf_lock_t buf3_A;
        EXPECT_TRUE(acq_check_if_blocks_until_buf_released(&buf3_A, &t3, &buf2_A, rwi_read, true));

        buf_lock_t buf1_B(&t1, block_B, rwi_write);
        buf1_B.release();

        buf_lock_t buf2_B(&t2, block_B, rwi_write);    // if issue 194 is not fixed, expect assertion failure here
        buf3_A.release();

        change_value(&buf2_B, changed_value);

        buf_lock_t buf3_B;
        EXPECT_TRUE(acq_check_if_blocks_until_buf_released(&buf3_B, &t3, &buf2_B, rwi_read, true));
    }

    static void test_cow_snapshots(cache_t *cache) {
        // t0:create+release(A,B), t3:acq_outdated_ok(A), t1:acqw(A) doesn't block, t1:change(A), t1:release(A), t2:acqw(A) doesn't block, t2:release(A), t3 doesn't see the change, t3:release(A)
        order_source_t order_source;
        transaction_t t0(cache, rwi_write, 0, repli_timestamp_t::distant_past,
                         order_source.check_in("test_cow_snapshot(t0)"));

        block_id_t block_A, block_B;
        create_two_blocks(&t0, &block_A, &block_B);

        order_token_t t3_order_token = order_source.check_in("test_cow_snapshot(t3)").with_read_mode();
        transaction_t t1(cache, rwi_write, 0, repli_timestamp_t::distant_past,
                         order_source.check_in("test_cow_snapshot(t1)"));
        transaction_t t2(cache, rwi_write, 0, repli_timestamp_t::distant_past,
                         order_source.check_in("test_cow_snapshot(t2)"));
        transaction_t t3(cache, rwi_read, 0, repli_timestamp_t::invalid, t3_order_token);

        buf_lock_t buf3_A(&t3, block_A, rwi_read_outdated_ok);
        uint32_t old_value = get_value(&buf3_A);

        buf_lock_t buf1_A;
        EXPECT_FALSE(acq_check_if_blocks_until_buf_released(&buf1_A, &t1, &buf3_A, rwi_write, false));
        change_value(&buf1_A, changed_value);
        buf1_A.release();

        buf_lock_t buf2_A;
        EXPECT_FALSE(acq_check_if_blocks_until_buf_released(&buf2_A, &t2, &buf3_A, rwi_write, false));
        buf2_A.release();

        EXPECT_EQ(old_value, get_value(&buf3_A));
    }

    static void test_double_cow_acq_release(cache_t * cache) {
        // t0:create+release(A,B), t1:acq_outdated_ok(A), t2:acq_outdated_ok(A), [t3:acqw(A) doesn't block, t3:delete(A),] t1:release(A), t2:release(A)
        order_source_t order_source;
        transaction_t t0(cache, rwi_write, 0, repli_timestamp_t::distant_past,
                         order_source.check_in("test_double_cow_acq_release(t0)"));

        block_id_t block_A, block_B;
        create_two_blocks(&t0, &block_A, &block_B);

        transaction_t t1(cache, rwi_read, 0, repli_timestamp_t::invalid,
                         order_source.check_in("test_double_cow_acq_release(t1)").with_read_mode());
        transaction_t t2(cache, rwi_read, 0, repli_timestamp_t::invalid,
                         order_source.check_in("test_double_cow_acq_release(t2)").with_read_mode());

        buf_lock_t buf1_A(&t1, block_A, rwi_read_outdated_ok);
        buf_lock_t buf2_A(&t2, block_A, rwi_read_outdated_ok);
    }

    static void test_cow_delete(cache_t * cache) {
        // t0:create+release(A,B), t1:acq_outdated_ok(A), t2:acq_outdated_ok(A), t3:acqw(A) doesn't block, t3:delete(A), t1:release(A), t2:release(A)
        order_source_t order_source;
        transaction_t t0(cache, rwi_write, 0, repli_timestamp_t::distant_past,
                         order_source.check_in("test_cow_delete(t0)"));

        block_id_t block_A, block_B;
        create_two_blocks(&t0, &block_A, &block_B);

        transaction_t t1(cache, rwi_read, 0, repli_timestamp_t::invalid,
                         order_source.check_in("test_cow_delete(t1)").with_read_mode());
        transaction_t t2(cache, rwi_read, 0, repli_timestamp_t::invalid,
                         order_source.check_in("test_cow_delete(t2)").with_read_mode());
        transaction_t t3(cache, rwi_write, 0, repli_timestamp_t::distant_past,
                         order_source.check_in("test_cow_delete(t3)"));

        buf_lock_t buf1_A(&t1, block_A, rwi_read_outdated_ok);
        buf_lock_t buf2_A(&t2, block_A, rwi_read_outdated_ok);

        const uint32_t old_value = get_value(&buf1_A);
        EXPECT_EQ(old_value, get_value(&buf2_A));

        buf_lock_t buf3_A;
        EXPECT_FALSE(acq_check_if_blocks_until_buf_released(&buf3_A, &t3, &buf1_A, rwi_write, false));

        change_value(&buf3_A, changed_value);
        buf3_A.mark_deleted();
        buf3_A.release();

        EXPECT_EQ(old_value, get_value(&buf1_A));
        buf1_A.release();

        EXPECT_EQ(old_value, get_value(&buf2_A));
    }

    DISABLE_COPYING(snapshots_tester_t);
};

// Making mock cache do proper snapshotting seems deceivingly simple (just copy the blocks when you are snapshotting),
// but unfortunately it's not (I tried).  There are some pretty tricky cases with snapshotting that these tests are
// catching, and implementing that logic in the mock cache is more trouble than it's worth.
#ifndef MOCK_CACHE_CHECK
TEST(SnapshotsTest, all_tests) {
    snapshots_tester_t().run();
}
#endif

}  // namespace unittest
