#include "unittest/gtest.hpp"
#include "errors.hpp"
#include "unittest/server_test_helper.hpp"

namespace unittest {

struct mirrored_tester_t : public server_test_helper_t {
protected:
    void run_tests(thread_saver_t& saver UNUSED, cache_t *cache UNUSED) {
        // for now this test doesn't work as it should, so turn it off
        log_call(test_read_ahead_checks_free_list, saver, cache);
    }
private:
    void test_read_ahead_checks_free_list(thread_saver_t& saver, cache_t *cache) {
        // Scenario:
        //   t0:create+release(A)
        //   t1:acqw+mark_deleted+release(A)
        //   cache:read_ahead(A) (writeback hasn't flushed the data yet)
        //   assert that A is not in the cache
        //
        // If cache:read_ahead doesn't check whether A has been deleted, but not flushed,
        // it will create a new inner_buf for A with stale data and it will mess things
        // up later on allocate, because A is still in the free-list.
        //
        // Also, since we are using fake_buf below, which doesn't contain the block_id/transaction_id,
        // we will get an assertion failure during mc_inner_buf creation, but since creating it is a
        // bug in the first place, it's fine to do so in this test (although I'm definitely not proud
        // of doing so).
        transactor_t t0(saver, cache, rwi_write, 0, current_time());
        block_id_t block_A, block_B;
        create_two_blocks(t0, block_A, block_B);

        transactor_t t1(saver, cache, rwi_write, 0, current_time());

        buf_t *buf1_A = acq(t1, block_A, rwi_write);
        buf1_A->mark_deleted(false);
        buf1_A->release();

        // create a fake buffer (be careful with populating it with data
        void *fake_buf = serializer->malloc();
        buf_data_t *ser_data = ptr_cast<buf_data_t>(fake_buf);
        ser_data--;
        ser_data->block_id = serializer->translate_block_id(block_A);
        ser_data->transaction_id = 1;

        EXPECT_FALSE(cache->contains_block(block_A));
        cache->offer_read_ahead_buf(block_A, ser_data + 1, current_time());
        EXPECT_FALSE(cache->contains_block(block_A));
    }
};

TEST(MirroredTest, all_tests) {
    mirrored_tester_t().run();
}

}

