#include "unittest/server_test_helper.hpp"
#include "unittest/gtest.hpp"

#include "server/nested_demo/redis_hash_values.hpp"
#include "server/nested_demo/redis_set_values.hpp"
#include "server/nested_demo/redis_sortedset_values.hpp"

namespace unittest {

struct redis_value_tester_t : public server_test_helper_t {
protected:
    void run_tests(cache_t *cache) {
        test_redis_hash_value(cache);
        test_redis_set_value(cache);
        test_redis_sortedset_value(cache);
    }
private:
    /*
     * All these tests are not very extensive as of Jul 28th 2011. They mostly
     * serve as a demo about how to use the extended redis value types.
     * TODO: Improve the tests!
     */
    
    void test_redis_hash_value(cache_t *cache) {
        // Initialize an empty hash value
        redis_hash_value_t hash_value;
        hash_value.nested_root = NULL_BLOCK_ID;
        hash_value.size = 0;

        // Perform some operations
        {
            on_thread_t thread(cache->home_thread());

            boost::shared_ptr<transaction_t> transaction(new transaction_t(cache, rwi_write, 1, repli_timestamp_t::invalid));
            value_sizer_t<redis_hash_value_t> sizer(cache->get_block_size());

            EXPECT_TRUE(hash_value.hset(&sizer, transaction, "field_1", "value_1"));
            EXPECT_TRUE(hash_value.hset(&sizer, transaction, "field_2", "value_2"));
            std::string large_value(1024 * 1024, 'X');
            EXPECT_TRUE(hash_value.hset(&sizer, transaction, "field_3", large_value));

            EXPECT_TRUE(hash_value.hexists(&sizer, transaction, "field_1"));
            EXPECT_TRUE(hash_value.hexists(&sizer, transaction, "field_2"));
            EXPECT_TRUE(hash_value.hexists(&sizer, transaction, "field_3"));
            EXPECT_TRUE(!hash_value.hexists(&sizer, transaction, "field_4"));

            EXPECT_EQ(hash_value.hlen(), 3);

            boost::shared_ptr<one_way_iterator_t<std::pair<std::string, std::string> > > iter = hash_value.hgetall(&sizer, transaction, cache->home_thread());
            EXPECT_EQ(iter->next(), (boost::optional<std::pair<std::string, std::string> >(std::pair<std::string, std::string>("field_1", "value_1"))));
            EXPECT_EQ(iter->next(), (boost::optional<std::pair<std::string, std::string> >(std::pair<std::string, std::string>("field_2", "value_2"))));
            EXPECT_EQ(iter->next(), (boost::optional<std::pair<std::string, std::string> >(std::pair<std::string, std::string>("field_3", large_value))));
            EXPECT_EQ(iter->next(), (boost::none));
            iter.reset(); // release locks

            hash_value.clear(&sizer, transaction, cache->home_thread());
            EXPECT_TRUE(!hash_value.hexists(&sizer, transaction, "field_1"));
            EXPECT_TRUE(!hash_value.hexists(&sizer, transaction, "field_2"));
            EXPECT_TRUE(!hash_value.hexists(&sizer, transaction, "field_3"));

            EXPECT_EQ(hash_value.hlen(), 0);
        }
    }

    void test_redis_set_value(cache_t *cache) {
        // Initialize an empty set value
        redis_set_value_t set_value;
        set_value.nested_root = NULL_BLOCK_ID;
        set_value.size = 0;

        // Perform some operations
        {
            on_thread_t thread(cache->home_thread());

            boost::shared_ptr<transaction_t> transaction(new transaction_t(cache, rwi_write, 1, repli_timestamp_t::invalid));
            value_sizer_t<redis_set_value_t> sizer(cache->get_block_size());

            EXPECT_TRUE(set_value.sadd(&sizer, transaction, "member_1"));
            EXPECT_TRUE(set_value.sadd(&sizer, transaction, "member_2"));
            EXPECT_TRUE(!set_value.sadd(&sizer, transaction, "member_1"));
            EXPECT_TRUE(!set_value.sadd(&sizer, transaction, "member_2"));

            EXPECT_TRUE(set_value.sismember(&sizer, transaction, "member_1"));
            EXPECT_TRUE(set_value.sismember(&sizer, transaction, "member_2"));
            EXPECT_TRUE(!set_value.sismember(&sizer, transaction, "member_3"));

            EXPECT_EQ(set_value.scard(), 2);

            boost::shared_ptr<one_way_iterator_t<std::string> > iter = set_value.smembers(&sizer, transaction, cache->home_thread());
            EXPECT_EQ(iter->next(), (boost::optional<std::string>(std::string("member_1"))));
            EXPECT_EQ(iter->next(), (boost::optional<std::string>(std::string("member_2"))));
            EXPECT_EQ(iter->next(), (boost::none));
            iter.reset(); // release locks

            EXPECT_TRUE(set_value.sadd(&sizer, transaction, "member_3"));
            EXPECT_TRUE(set_value.sismember(&sizer, transaction, "member_3"));
            EXPECT_TRUE(set_value.srem(&sizer, transaction, "member_3"));
            EXPECT_TRUE(!set_value.sismember(&sizer, transaction, "member_3"));

            set_value.clear(&sizer, transaction, cache->home_thread());
            EXPECT_TRUE(!set_value.sismember(&sizer, transaction, "member_1"));
            EXPECT_TRUE(!set_value.sismember(&sizer, transaction, "member_2"));

            EXPECT_EQ(set_value.scard(), 0);
        }
    }

    void test_redis_sortedset_value(cache_t *cache) {
        // Initialize an empty set value
        redis_sortedset_value_t sortedset_value;
        sortedset_value.member_score_nested_root = NULL_BLOCK_ID;
        sortedset_value.score_member_nested_root = NULL_BLOCK_ID;
        sortedset_value.size = 0;

        // Perform some operations
        {
            on_thread_t thread(cache->home_thread());

            boost::shared_ptr<transaction_t> transaction(new transaction_t(cache, rwi_write, 1, repli_timestamp_t::invalid));
            value_sizer_t<redis_sortedset_value_t> sizer(cache->get_block_size());

            EXPECT_TRUE(sortedset_value.zadd(&sizer, transaction, "member_1", -0.5f));
            EXPECT_TRUE(sortedset_value.zadd(&sizer, transaction, "member_3", 0.1f));
            EXPECT_TRUE(sortedset_value.zadd(&sizer, transaction, "member_2", 0.0f));
            EXPECT_TRUE(sortedset_value.zrank(&sizer, transaction, cache->home_thread(), "member_1").get() == 0);
            EXPECT_TRUE(sortedset_value.zrank(&sizer, transaction, cache->home_thread(), "member_2").get() == 1);
            EXPECT_TRUE(sortedset_value.zrank(&sizer, transaction, cache->home_thread(), "member_3").get() == 2);
            EXPECT_TRUE(sortedset_value.zscore(&sizer, transaction, "member_1").get() == -0.5f);
            EXPECT_TRUE(sortedset_value.zscore(&sizer, transaction, "member_2").get() == 0.0f);
            EXPECT_TRUE(sortedset_value.zscore(&sizer, transaction, "member_3").get() == 0.1f);
            EXPECT_TRUE(!sortedset_value.zadd(&sizer, transaction, "member_1", -0.1f)); // Should update the score
            EXPECT_TRUE(!sortedset_value.zadd(&sizer, transaction, "member_2", 0.0f));
            EXPECT_TRUE(!sortedset_value.zadd(&sizer, transaction, "member_3", 0.1f));
            EXPECT_TRUE(sortedset_value.zrank(&sizer, transaction, cache->home_thread(), "member_1").get() == 0);
            EXPECT_TRUE(sortedset_value.zrank(&sizer, transaction, cache->home_thread(), "member_2").get() == 1);
            EXPECT_TRUE(sortedset_value.zrank(&sizer, transaction, cache->home_thread(), "member_3").get() == 2);

            EXPECT_EQ(sortedset_value.zcard(), 3);

            boost::shared_ptr<one_way_iterator_t<std::pair<float, std::string> > > iter = sortedset_value.zrange(&sizer, transaction, cache->home_thread(), 0);
            EXPECT_EQ(iter->next(), (boost::optional<std::pair<float, std::string> >(std::pair<float, std::string>(-0.1f, "member_1"))));
            EXPECT_EQ(iter->next(), (boost::optional<std::pair<float, std::string> >(std::pair<float, std::string>(0.0f, "member_2"))));
            EXPECT_EQ(iter->next(), (boost::optional<std::pair<float, std::string> >(std::pair<float, std::string>(0.1f, "member_3"))));
            EXPECT_EQ(iter->next(), (boost::none));
            iter.reset(); // release locks

            // Make all members ambiguous when it comes to score (they should then be sorted by the member string)
            EXPECT_TRUE(!sortedset_value.zadd(&sizer, transaction, "member_1", 0.0));
            EXPECT_TRUE(!sortedset_value.zadd(&sizer, transaction, "member_3", 0.0));
            // Last two elements in the sorted set
            iter = sortedset_value.zrange(&sizer, transaction, cache->home_thread(), -2);
            EXPECT_EQ(iter->next(), (boost::optional<std::pair<float, std::string> >(std::pair<float, std::string>(0.0f, "member_2"))));
            EXPECT_EQ(iter->next(), (boost::optional<std::pair<float, std::string> >(std::pair<float, std::string>(0.0f, "member_3"))));
            EXPECT_EQ(iter->next(), (boost::none));
            iter.reset(); // release locks

            sortedset_value.clear(&sizer, transaction, cache->home_thread());
            EXPECT_EQ(sortedset_value.zcard(), 0);
        }
    }
};

TEST(RedisValueTest, all_tests) {
    redis_value_tester_t().run();
}
    
} /* namespace unittest */