#include "errors.hpp"
#include "unittest/gtest.hpp"
#include "unittest/server_test_helper.hpp"
#include "unittest/unittest_utils.hpp"
#include "serializer/log/log_serializer.hpp" // for ls_buf_data_t
#include "serializer/translator.hpp"
#include "buffer_cache/buffer_cache.hpp"

#include "buffer_cache/sequence_group.hpp"

namespace unittest {

struct mirrored_tester_t : public server_test_helper_t {
protected:
    void run_tests(cache_t *cache) {
        // for now this test doesn't work as it should, so turn it off
        trace_call(test_read_ahead_checks_free_list, cache);
    }
private:
    void test_read_ahead_checks_free_list(cache_t *cache) {
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
        sequence_group_t seq_group(1);
        seq_group.slice_groups[cache->get_slice_num()].fifo.rethread(cache->home_thread());

        // Also, since we are using fake_buf below, which doesn't contain the
        // block_id/block_sequence_id, we will get an assertion failure during mc_inner_buf
        // creation, but since creating it is a bug in the first place, it's fine to do so in this
        // test (although I'm definitely not proud of doing so).
        transaction_t t0(cache, &seq_group, rwi_write, 0, repli_timestamp_t::distant_past);
        block_id_t block_A, block_B;
        create_two_blocks(&t0, &block_A, &block_B);

        transaction_t t1(cache, &seq_group, rwi_write, 0, repli_timestamp_t::distant_past);

        buf_lock_t *buf1_A = acq(&t1, block_A, rwi_write);
        buf1_A->mark_deleted();
        delete buf1_A;

        // create a fake buffer (be careful with populating it with data
        void *fake_buf = serializer->malloc();
        ls_buf_data_t *ser_data = reinterpret_cast<ls_buf_data_t *>(fake_buf);
        ser_data--;
        ser_data->block_id = serializer->translate_block_id(block_A);
        ser_data->block_sequence_id = 1;

        EXPECT_FALSE(cache->contains_block(block_A));
        cache->offer_read_ahead_buf(block_A, ser_data + 1, boost::intrusive_ptr<standard_block_token_t>(), repli_timestamp_t::distant_past);
        EXPECT_FALSE(cache->contains_block(block_A));
    }
};

TEST(MirroredTest, all_tests) {
    mirrored_tester_t().run();
}

}  // namespace unittest

