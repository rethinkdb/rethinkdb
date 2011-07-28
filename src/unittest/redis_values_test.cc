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
        // TODO! Convert all the rasserts and outputs into unit test assertions
        {
            on_thread_t thread(cache->home_thread());

            boost::shared_ptr<transaction_t> transaction(new transaction_t(cache, rwi_write, 1, repli_timestamp_t::invalid));
            value_sizer_t<redis_hash_value_t> sizer(cache->get_block_size());

            rassert(hash_value.hset(&sizer, transaction, "field_1", "value_1"));
            rassert(hash_value.hset(&sizer, transaction, "field_2", "value_2"));
            std::string large_value(1024 * 1024, 'X');
            rassert(hash_value.hset(&sizer, transaction, "field_3", large_value));

            rassert(hash_value.hexists(&sizer, transaction, "field_1"));
            rassert(hash_value.hexists(&sizer, transaction, "field_2"));
            rassert(hash_value.hexists(&sizer, transaction, "field_3"));
            rassert(!hash_value.hexists(&sizer, transaction, "field_4"));

            fprintf(stderr, "Hash length: %d\n", (int)hash_value.hlen());

            boost::shared_ptr<one_way_iterator_t<std::pair<std::string, std::string> > > iter = hash_value.hgetall(&sizer, transaction, cache->home_thread());
            fprintf(stderr, "\nHash contents:\n");
            while (true) {
                boost::optional<std::pair<std::string, std::string> > next = iter->next();
                if (next) {
                    fprintf(stderr, "\t%s -> %s\n", next->first.c_str(), next->second.c_str());
                } else {
                    break;
                }
            }
            iter.reset();

            hash_value.clear(&sizer, transaction, cache->home_thread());
            rassert(!hash_value.hexists(&sizer, transaction, "field_1"));
            rassert(!hash_value.hexists(&sizer, transaction, "field_2"));
            rassert(!hash_value.hexists(&sizer, transaction, "field_3"));

            fprintf(stderr, "Hash length after clear(): %d\n", (int)hash_value.hlen());
        }
    }

    void test_redis_set_value(cache_t *cache) {
        // Initialize an empty set value
        redis_set_value_t set_value;
        set_value.nested_root = NULL_BLOCK_ID;
        set_value.size = 0;

        // Perform some operations
        // TODO! Convert all the rasserts and outputs into unit test assertions
        {
            on_thread_t thread(cache->home_thread());

            boost::shared_ptr<transaction_t> transaction(new transaction_t(cache, rwi_write, 1, repli_timestamp_t::invalid));
            value_sizer_t<redis_set_value_t> sizer(cache->get_block_size());

            guarantee(set_value.sadd(&sizer, transaction, "member_1"));
            guarantee(set_value.sadd(&sizer, transaction, "member_2"));
            rassert(!set_value.sadd(&sizer, transaction, "member_1"));
            rassert(!set_value.sadd(&sizer, transaction, "member_2"));

            rassert(set_value.sismember(&sizer, transaction, "member_1"));
            rassert(set_value.sismember(&sizer, transaction, "member_2"));
            rassert(!set_value.sismember(&sizer, transaction, "member_3"));

            fprintf(stderr, "Set cardinality: %d\n", (int)set_value.scard());

            boost::shared_ptr<one_way_iterator_t<std::string> > iter = set_value.smembers(&sizer, transaction, cache->home_thread());
            fprintf(stderr, "\nSet contents:\n");
            while (true) {
                boost::optional<std::string> next = iter->next();
                if (next) {
                    fprintf(stderr, "\t%s \n", next->c_str());
                } else {
                    break;
                }
            }
            iter.reset();

            guarantee(set_value.sadd(&sizer, transaction, "member_3"));
            rassert(set_value.sismember(&sizer, transaction, "member_3"));
            guarantee(set_value.srem(&sizer, transaction, "member_3"));
            rassert(!set_value.sismember(&sizer, transaction, "member_3"));

            set_value.clear(&sizer, transaction, cache->home_thread());
            rassert(!set_value.sismember(&sizer, transaction, "member_1"));
            rassert(!set_value.sismember(&sizer, transaction, "member_2"));

            fprintf(stderr, "Set cardinality after clear(): %d\n", (int)set_value.scard());
        }
    }

    void test_redis_sortedset_value(cache_t *cache) {
        // Initialize an empty set value
        redis_sortedset_value_t sortedset_value;
        sortedset_value.member_score_nested_root = NULL_BLOCK_ID;
        sortedset_value.score_member_nested_root = NULL_BLOCK_ID;
        sortedset_value.size = 0;

        // Perform some operations
        // TODO! Convert all the rasserts and outputs into unit test assertions
        {
            on_thread_t thread(cache->home_thread());

            boost::shared_ptr<transaction_t> transaction(new transaction_t(cache, rwi_write, 1, repli_timestamp_t::invalid));
            value_sizer_t<redis_sortedset_value_t> sizer(cache->get_block_size());

            guarantee(sortedset_value.zadd(&sizer, transaction, "member_1", -0.5f));
            guarantee(sortedset_value.zadd(&sizer, transaction, "member_3", 0.1f));
            guarantee(sortedset_value.zadd(&sizer, transaction, "member_2", 0.0f));
            guarantee(sortedset_value.zrank(&sizer, transaction, cache->home_thread(), "member_1").get() == 0);
            guarantee(sortedset_value.zrank(&sizer, transaction, cache->home_thread(), "member_2").get() == 1);
            guarantee(sortedset_value.zrank(&sizer, transaction, cache->home_thread(), "member_3").get() == 2);
            guarantee(sortedset_value.zscore(&sizer, transaction, "member_1").get() == -0.5f);
            guarantee(sortedset_value.zscore(&sizer, transaction, "member_2").get() == 0.0f);
            guarantee(sortedset_value.zscore(&sizer, transaction, "member_3").get() == 0.1f);
            guarantee(!sortedset_value.zadd(&sizer, transaction, "member_1", -0.1f)); // Should update the score
            guarantee(!sortedset_value.zadd(&sizer, transaction, "member_2", 0.0f));
            guarantee(!sortedset_value.zadd(&sizer, transaction, "member_3", 0.1f));
            guarantee(sortedset_value.zrank(&sizer, transaction, cache->home_thread(), "member_1").get() == 0);
            guarantee(sortedset_value.zrank(&sizer, transaction, cache->home_thread(), "member_2").get() == 1);
            guarantee(sortedset_value.zrank(&sizer, transaction, cache->home_thread(), "member_3").get() == 2);

            fprintf(stderr, "Sorted set cardinality: %d\n", (int)sortedset_value.zcard());

            boost::shared_ptr<one_way_iterator_t<std::pair<float, std::string> > > iter = sortedset_value.zrange(&sizer, transaction, cache->home_thread(), 0);
            fprintf(stderr, "\nSorted set contents:\n");
            while (true) {
                boost::optional<std::pair<float, std::string> > next = iter->next();
                if (next) {
                    fprintf(stderr, "\t%f %s \n", next->first, next->second.c_str());
                } else {
                    break;
                }
            }
            iter.reset();


            // Make all members ambiguous when it comes to score (they should then be sorted by the member string)
            guarantee(!sortedset_value.zadd(&sizer, transaction, "member_1", 0.0));
            guarantee(!sortedset_value.zadd(&sizer, transaction, "member_3", 0.0));
            iter = sortedset_value.zrange(&sizer, transaction, cache->home_thread(), -2);
            fprintf(stderr, "\nLast two elements int sorted set:\n");
            while (true) {
                boost::optional<std::pair<float, std::string> > next = iter->next();
                if (next) {
                    fprintf(stderr, "\t%f %s \n", next->first, next->second.c_str());
                } else {
                    break;
                }
            }
            iter.reset();

            sortedset_value.clear(&sizer, transaction, cache->home_thread());
            fprintf(stderr, "Set cardinality after clear(): %d\n", (int)sortedset_value.zcard());
        }
    }
};

TEST(RedisValueTest, all_tests) {
    redis_value_tester_t().run();
}
    
} /* namespace unittest */