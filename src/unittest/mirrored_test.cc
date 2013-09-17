// Copyright 2010-2013 RethinkDB, all rights reserved.

#include "buffer_cache/buffer_cache.hpp"
#include "unittest/unittest_utils.hpp"
#include "serializer/config.hpp"
#include "serializer/translator.hpp"
#include "unittest/gtest.hpp"
#include "unittest/mock_file.hpp"
#include "unittest/server_test_helper.hpp"

namespace unittest {

class mirrored_tester_t : public server_test_helper_t {
protected:
    void run_tests(cache_t *cache) {
        // for now this test doesn't work as it should, so turn it off
        trace_call(test_read_ahead_checks_free_list, cache);
    }
private:
    void test_read_ahead_checks_free_list(cache_t *cache) {
        order_source_t order_source;
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

        // Also, since we are using fake_buf below, which doesn't contain the
        // block_id/block_sequence_id, we will get an assertion failure during mc_inner_buf
        // creation, but since creating it is a bug in the first place, it's fine to do so in this
        // test (although I'm definitely not proud of doing so).
        transaction_t t0(cache, rwi_write, 0, repli_timestamp_t::distant_past,
                         order_source.check_in("test_read_ahead_checks_free_list(t0)"), WRITE_DURABILITY_SOFT);
        block_id_t block_A, block_B;
        create_two_blocks(&t0, &block_A, &block_B);

        transaction_t t1(cache, rwi_write, 0, repli_timestamp_t::distant_past,
                         order_source.check_in("test_read_ahead_checks_free_list(t1)"), WRITE_DURABILITY_SOFT);

        buf_lock_t buf1_A(&t1, block_A, rwi_write);
        buf1_A.mark_deleted();
        buf1_A.release();

        // create a fake buffer (be careful with populating it with data
        scoped_malloc_t<ser_buffer_t> fake_buf = serializer->malloc();
        fake_buf->ser_header.block_id = serializer->translate_block_id(block_A);
        fake_buf->ser_header.block_sequence_id = 1;

        EXPECT_FALSE(cache->contains_block(block_A));
        cache->offer_read_ahead_buf(block_A,
                                    &fake_buf,
                                    counted_t<standard_block_token_t>(),
                                    repli_timestamp_t::distant_past);
        EXPECT_FALSE(cache->contains_block(block_A));
    }
};

TEST(MirroredTest, ReadAheadChecks) {
    mirrored_tester_t().run();
}

static const uint64_t value_A = 12345;
static const uint64_t value_B = 23456;

class durability_tester_t : public server_test_helper_t {
public:
    durability_tester_t() : block_id(NULL_BLOCK_ID) {}

    void run_tests(cache_t *cache) {
        {
            transaction_t t0(cache, rwi_write, 0, repli_timestamp_t::distant_past, order_token_t::ignore, WRITE_DURABILITY_SOFT);
            buf_lock_t buf(&t0);
            block_id = buf.get_block_id();
            *static_cast<uint64_t *>(buf.get_data_write()) = value_A;
        }

        {
            transaction_t t0(cache, rwi_write, 0, repli_timestamp_t::distant_past, order_token_t::ignore, WRITE_DURABILITY_HARD);
            {
                buf_lock_t buf(&t0, block_id, rwi_write);
                const uint64_t value = *static_cast<const uint64_t *>(buf.get_data_read());
                EXPECT_EQ(value_A, value);
                *static_cast<uint64_t *>(buf.get_data_write()) = value_B;
            }
        }

        // We snapshot the file immediately after the transaction has finished, to verify that it's actually written.
        snapshotted_file_opener_ = *this->mock_file_opener;
    }

    void check_snapshotted_file_contents() {
        standard_serializer_t log_serializer(standard_serializer_t::dynamic_config_t(),
                                             &snapshotted_file_opener_,
                                             &get_global_perfmon_collection());

        std::vector<standard_serializer_t *> serializers;
        serializers.push_back(&log_serializer);
        serializer_multiplexer_t::create(serializers, 1);
        serializer_multiplexer_t multiplexer(serializers);

        mirrored_cache_config_t cache_cfg;
        cache_cfg.flush_timer_ms = MILLION;
        cache_cfg.flush_dirty_size = BILLION;
        cache_cfg.max_size = GIGABYTE;
        cache_t cache(multiplexer.proxies[0], cache_cfg, &get_global_perfmon_collection());

        transaction_t t0(&cache, rwi_read, order_token_t::ignore);

        buf_lock_t buf(&t0, block_id, rwi_read);
        const uint64_t value = *static_cast<const uint64_t *>(buf.get_data_read());
        EXPECT_EQ(value_B, value);
    }

private:
    block_id_t block_id;
    mock_file_opener_t snapshotted_file_opener_;
};

TEST(MirroredTest, Durability) {
    durability_tester_t tester;
    tester.run();
    unittest::run_in_thread_pool(std::bind(&durability_tester_t::check_snapshotted_file_contents, &tester));
}

}  // namespace unittest

