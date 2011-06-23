#include "unittest/gtest.hpp"
#include "unittest/server_test_helper.hpp"
#include "unittest/unittest_utils.hpp"
#include "buffer_cache/blob.hpp"

namespace unittest {

struct blob_tester_t : public server_test_helper_t {
protected:
    void run_tests(cache_t *cache) {
        small_value_test(cache);
    }

private:
    void small_value_test(cache_t *cache) {
        block_size_t block_size = cache->get_block_size();

        transaction_t txn(cache, rwi_write, 0, repli_timestamp_t::distant_past);

        char buf[251] = { 0 };

        blob_t blob(block_size, buf, 251);
        {
            ASSERT_EQ(0, blob.valuesize());
            ASSERT_EQ(1, blob.refsize(block_size));

            buffer_group_t bg;
            blob_acq_t bacq;
            blob.expose_region(&txn, rwi_write, 0, 0, &bg, &bacq);

            ASSERT_EQ(0, bg.get_size());
            ASSERT_EQ(0, bg.num_buffers());
        }

        blob.append_region(&txn, 1);
        {
            ASSERT_EQ(1, blob.valuesize());
            ASSERT_EQ(2, blob.refsize(block_size));

            buffer_group_t bg;
            blob_acq_t bacq;
            blob.expose_region(&txn, rwi_write, 0, 1, &bg, &bacq);

            ASSERT_EQ(1, bg.get_size());
            ASSERT_EQ(1, bg.num_buffers());
            ASSERT_EQ(1, bg.get_size());
        }
    }
};

TEST(BlobTest, all_tests) {
    blob_tester_t().run();
}

}  // namespace unittest
