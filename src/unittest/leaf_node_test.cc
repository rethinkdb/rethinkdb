// Copyright 2010-2015 RethinkDB, all rights reserved.
#include <map>

#include "btree/leaf_node.hpp"
#include "btree/node.hpp"
#include "containers/scoped.hpp"
#include "repli_timestamp.hpp"
#include "unittest/gtest.hpp"
#include "unittest/unittest_utils.hpp"
#include "utils.hpp"
#include "unittest/btree_utils.hpp"


namespace unittest {

class LeafNodeTracker {
public:
    LeafNodeTracker()
        : bs_(max_block_size_t::unsafe_make(4096)),
          sizer_(bs_),
          node_(bs_.value()),
          tstamp_counter_(0),
          maximum_existing_tstamp_(repli_timestamp_t::distant_past) {
        leaf::init(&sizer_, node_.get());
        Print();
    }

    leaf_node_t *node() { return node_.get(); }
    value_sizer_t *sizer() { return &sizer_; }

    bool Insert(
            const store_key_t &key,
            const std::string &value,
            repli_timestamp_t tstamp) {
        short_value_buffer_t v(value);

        if (leaf::is_full(&sizer_, node(), key.btree_key(), v.data())) {
            Print();

            Verify();
            return false;
        }

        leaf::insert(
            &sizer_,
            node(),
            key.btree_key(),
            v.data(),
            tstamp,
            maximum_existing_tstamp_,
            key_modification_proof_t::real_proof());

        maximum_existing_tstamp_ =
            superceding_recency(maximum_existing_tstamp_, tstamp);

        kv_[key] = value;

        Print();

        Verify();
        return true;
    }

    bool Insert(const store_key_t &key, const std::string &value) {
        return Insert(key, value, NextTimestamp());
    }

    void Remove(
            const store_key_t &key,
            repli_timestamp_t tstamp) {
        ASSERT_TRUE(ShouldHave(key));

        kv_.erase(key);

        leaf::remove(
            &sizer_,
            node(),
            key.btree_key(),
            tstamp,
            maximum_existing_tstamp_,
            key_modification_proof_t::real_proof());

        maximum_existing_tstamp_ =
            superceding_recency(maximum_existing_tstamp_, tstamp);

        Verify();

        Print();
    }

    void Remove(const store_key_t &key) {
        Remove(key, NextTimestamp());
    }

    void Merge(LeafNodeTracker *lnode) {
        SCOPED_TRACE("Merge");

        ASSERT_EQ(bs_.ser_value(), lnode->bs_.ser_value());

        leaf::merge(&sizer_, lnode->node(), node());

        int old_kv_size = kv_.size();
        for (std::map<store_key_t, std::string>::iterator p = lnode->kv_.begin(), e = lnode->kv_.end(); p != e; ++p) {
            kv_[p->first] = p->second;
        }

        ASSERT_EQ(kv_.size(), old_kv_size + lnode->kv_.size());

        lnode->kv_.clear();

        {
            SCOPED_TRACE("mergee verify");
            Verify();
        }
        {
            SCOPED_TRACE("lnode verify");
            lnode->Verify();
        }
    }

    void Level(int nodecmp_value, LeafNodeTracker *sibling, bool *could_level_out) {
        // Assertions can cause us to exit the function early, so give
        // the output parameter an initialized value.
        *could_level_out = false;
        ASSERT_EQ(bs_.ser_value(), sibling->bs_.ser_value());

        store_key_t replacement;
        bool can_level = leaf::level(&sizer_, nodecmp_value, node(), sibling->node(),
                                     replacement.btree_key(), nullptr);

        if (can_level) {
            ASSERT_TRUE(!sibling->kv_.empty());
            if (nodecmp_value < 0) {
                // Copy keys from front of sibling until and including replacement key.

                std::map<store_key_t, std::string>::iterator p = sibling->kv_.begin();
                while (p != sibling->kv_.end() && p->first < replacement) {
                    kv_[p->first] = p->second;
                    std::map<store_key_t, std::string>::iterator prev = p;
                    ++p;
                    sibling->kv_.erase(prev);
                }
                ASSERT_TRUE(p != sibling->kv_.end());
                ASSERT_EQ(key_to_unescaped_str(p->first), key_to_unescaped_str(replacement));
                kv_[p->first] = p->second;
                sibling->kv_.erase(p);
            } else {
                // Copy keys from end of sibling until but not including replacement key.

                std::map<store_key_t, std::string>::iterator p = sibling->kv_.end();
                --p;
                while (p != sibling->kv_.begin() && p->first > replacement) {
                    kv_[p->first] = p->second;
                    std::map<store_key_t, std::string>::iterator prev = p;
                    --p;
                    sibling->kv_.erase(prev);
                }

                ASSERT_EQ(key_to_unescaped_str(p->first), key_to_unescaped_str(replacement));
            }
        }

        *could_level_out = can_level;

        Verify();
        sibling->Verify();
    }

    void Split(LeafNodeTracker *right) {
        ASSERT_EQ(bs_.ser_value(), right->bs_.ser_value());

        ASSERT_TRUE(leaf::is_empty(right->node()));

        store_key_t median;
        leaf::split(&sizer_, node(), right->node(), median.btree_key());

        std::map<store_key_t, std::string>::iterator p = kv_.end();
        --p;
        while (p->first > median && p != kv_.begin()) {
            right->kv_[p->first] = p->second;
            kv_.erase(p--);
        }

        Verify();
        right->Verify();
    }

    bool IsFull(const store_key_t& key, const std::string& value) {
        short_value_buffer_t value_buf(value);
        return leaf::is_full(&sizer_, node(), key.btree_key(), value_buf.data());
    }

    bool IsUnderfull() {
        return leaf::is_underfull(&sizer_, node());
    }

    bool ShouldHave(const store_key_t& key) {
        return kv_.end() != kv_.find(key);
    }

    repli_timestamp_t NextTimestamp() {
        ++tstamp_counter_;
        repli_timestamp_t ret;
        ret.longtime = tstamp_counter_;
        return ret;
    }

    // This only prints if we enable printing.
    void Print() {
        // leaf::print(stdout, &sizer_, node());
    }

    void printmap(const std::map<store_key_t, std::string>& m) {
        for (std::map<store_key_t, std::string>::const_iterator p = m.begin(), q = m.end(); p != q; ++p) {
            printf("%s: %s;", key_to_debug_str(p->first).c_str(), p->second.c_str());
        }
    }

    void Verify() {
        // Of course, this will fail with rassert, not a gtest assertion.
        leaf::validate(&sizer_, node());

        std::map<store_key_t, std::string> leaf_guts;
        repli_timestamp_t max_possible_tstamp = { tstamp_counter_ };
        leaf::visit_entries(&sizer_, node(), max_possible_tstamp,
            [&](const btree_key_t *key, repli_timestamp_t, const void *value)
                    -> continue_bool_t {
                if (value != nullptr) {
                    store_key_t k(key);
                    short_value_buffer_t v(static_cast<const short_value_t *>(value));
                    auto res = leaf_guts.insert(std::make_pair(k, v.as_str()));
                    EXPECT_TRUE(res.second);
                }
                return continue_bool_t::CONTINUE;
            });

        if (leaf_guts != kv_) {
            printf("leaf_guts: ");
            printmap(leaf_guts);
            printf("\nkv_: ");
            printmap(kv_);
            printf("\n");
        }
        ASSERT_TRUE(leaf_guts == kv_);
    }

private:
    max_block_size_t bs_;
    short_value_sizer_t sizer_;
    scoped_malloc_t<leaf_node_t> node_;

    uint64_t tstamp_counter_;
    repli_timestamp_t maximum_existing_tstamp_;

    std::map<store_key_t, std::string> kv_;


    DISABLE_COPYING(LeafNodeTracker);
};

TEST(LeafNodeTest, Offsets) {
    ASSERT_EQ(0u, offsetof(leaf_node_t, magic));
    ASSERT_EQ(4u, offsetof(leaf_node_t, num_pairs));
    ASSERT_EQ(6u, offsetof(leaf_node_t, live_size));
    ASSERT_EQ(8u, offsetof(leaf_node_t, frontmost));
    ASSERT_EQ(10u, offsetof(leaf_node_t, tstamp_cutpoint));
    ASSERT_EQ(12u, offsetof(leaf_node_t, pair_offsets));
    ASSERT_EQ(12u, sizeof(leaf_node_t));
}

TEST(LeafNodeTest, Reinserts) {
    LeafNodeTracker tracker;
    std::string v = "aa";
    store_key_t k("key");
    for (; v[0] <= 'z'; ++v[0]) {
        v[1] = 'a';
        for (; v[1] <= 'z'; ++v[1]) {
            tracker.Insert(k, v);
        }
    }
}

TEST(LeafNodeTest, TenInserts) {
    LeafNodeTracker tracker;

    ASSERT_LT(leaf::MANDATORY_TIMESTAMPS, 10);

    const int num_keys = 10;
    const char *ks[num_keys] = { "the_relatively_long_key_that_is_relatively_long,_eh?__or_even_longer",
                           "some_other_relatively_long_key_that_...whatever.",
                           "another_relatively_long_key",
                           "a_short_key",
                           "", /* an empty string key */
                           "grohl",
                           "cobain",
                           "reznor",
                           "marley",
                           "domino" };

    for (int i = 0; i < 26 * 26; ++i) {
        std::string v;
        v += ('a' + (i / 26));
        v += ('a' + (i % 26));

        for (int j = 0; j < num_keys; ++j) {
            tracker.Insert(store_key_t(ks[j]), v);
        }
    }
}

TEST(LeafNodeTest, InsertRemove) {
    LeafNodeTracker tracker;

    const int num_keys = 10;
    const char *ks[num_keys] = { "the_relatively_long_key_that_is_relatively_long,_eh?__or_even_longer",
                           "some_other_relatively_long_key_that_...whatever.",
                           "another_relatively_long_key",
                           "a_short_key",
                           "", /* an empty string key */
                           "grohl",
                           "cobain",
                           "reznor",
                           "marley",
                           "domino" };

    rng_t rng;
    for (int i = 0; i < 26 * 26; ++i) {
        std::string v;
        v += ('a' + (i / 26));
        v += ('a' + (i % 26));

        for (int j = 0; j < num_keys; ++j) {
            if (rng.randint(2) == 1) {
                tracker.Insert(store_key_t(ks[j]), v);
            } else {
                if (tracker.ShouldHave(store_key_t(ks[j]))) {
                    tracker.Remove(store_key_t(ks[j]));
                }
            }
        }
    }
}

scoped_ptr_t<LeafNodeTracker> test_random_out_of_order(
        int num_keys,
        int num_ops,
        bool random_tstamps,
        store_key_t low_key = store_key_t::min(),
        store_key_t high_key = store_key_t::max()) {

    scoped_ptr_t<LeafNodeTracker> tracker(new LeafNodeTracker());

    rng_t rng;

    std::vector<store_key_t> key_pool(num_keys);
    for (int i = 0; i < num_keys; ++i) {
        do {
            key_pool[i] = store_key_t(random_letter_string(&rng, 0, 159));
        } while (key_pool[i].compare(low_key) < 0 || key_pool[i].compare(high_key) >= 0);
    }

    for (int i = 0; i < num_ops; ++i) {
        const store_key_t &key = key_pool[rng.randint(num_keys)];
        repli_timestamp_t tstamp;
        if (random_tstamps) {
            tstamp.longtime = rng.randint(num_ops);
        } else {
            tstamp.longtime = 0;
        }

        if (rng.randint(2) == 0) {
            if (tracker->ShouldHave(key)) {
                tracker->Remove(key);
            }
        } else {
            std::string value = random_letter_string(&rng, 0, 159);
            /* If the key doesn't fit, that's OK; it will just not insert it
            and return `false`, which we ignore. */
            tracker->Insert(key, value, tstamp);
        }
    }

    return tracker;
}

TEST(LeafNodeTest, RandomOutOfOrder) {
    for (int try_num = 0; try_num < 10; ++try_num) {
        test_random_out_of_order(10, 20000, true);
    }
}

TEST(LeafNodeTest, RandomOutOfOrderLowTstamp) {
    for (int try_num = 0; try_num < 10; ++try_num) {
        // In contrast to RandomOutOfOrder, we use a fixed tstamp of 0
        // to test some corner cases better.
        test_random_out_of_order(50, 20000, false);
    }
}

void make_node_underfull(LeafNodeTracker *tracker, rng_t *rng) {
    leaf_node_t *node = tracker->node();
    while (!tracker->IsUnderfull() ||
           (node->num_pairs > 0 && rng->randint(2) == 0)) {
        int chosen = rng->randint(node->num_pairs);
        auto pair = *leaf_node_t::iterator(node, chosen);

        // We might hit a removal entry; skip those.
        if (tracker->ShouldHave(store_key_t(pair.first))) {
            tracker->Remove(store_key_t(pair.first));
        }
    }
}

TEST(LeafNodeTest, RandomMerging) {
    rng_t rng;

    for (int try_num = 0; try_num < 100; ++try_num) {
        // choose a random split key with some space on the ends for values
        store_key_t split_key;
        while (split_key.size() == 0 ||
               split_key.contents()[0] == 'a' ||
               split_key.contents()[0] == 'z') {
            split_key = store_key_t(random_letter_string(&rng, 4, 8));
        }

        bool zero_timestamps = (try_num % 2) == 0;

        scoped_ptr_t<LeafNodeTracker> left = test_random_out_of_order(
                40, 200, zero_timestamps, store_key_t::min(), split_key);
        scoped_ptr_t<LeafNodeTracker> right = test_random_out_of_order(
                40, 200, zero_timestamps, split_key, store_key_t::max());

        make_node_underfull(left.get(), &rng);
        make_node_underfull(right.get(), &rng);

        right->Merge(left.get());
    }
}

TEST(LeafNodeTest, RandomSplitting) {
    rng_t rng;

    for (int try_num = 0; try_num < 100; ++try_num) {
        bool zero_timestamps = (try_num % 2) == 0;

        scoped_ptr_t<LeafNodeTracker> node = test_random_out_of_order(
                40, 200, zero_timestamps);

        // The node might not be full yet; add some values until it is.
        while (true) {
            store_key_t key(random_letter_string(&rng, 0, 160));
            std::string value = random_letter_string(&rng, 0, 160);
            if (node->IsFull(key, value)) {
                break;
            } else {
                node->Insert(key, value);
            }
        }

        LeafNodeTracker right;
        node->Split(&right);
    }
}

TEST(LeafNodeTest, DeletionTimestamp) {
    LeafNodeTracker tracker;

    rng_t rng;

    /* The parameters for the initial setup phase are tuned so that the call to
    `erase_deletions()` usually affects at least one deletion entry. */

    std::vector<store_key_t> keys;
    const int num_ops = 50;
    for (int i = 0; i < num_ops; ++i) {
        if (rng.randint(2) == 1 || keys.size() == 0) {
            store_key_t key(std::string(rng.randint(10), 'a' + rng.randint(26)));
            keys.push_back(key);
            std::string value(rng.randint(10), 'a' + rng.randint(26));
            tracker.Insert(key, value);
        } else {
            const store_key_t &key = keys.at(rng.randint(keys.size()));
            if (tracker.ShouldHave(key) && rng.randint(4) != 0) {
                tracker.Remove(key);
            } else {
                std::string value(rng.randint(10), 'a' + rng.randint(26));
                tracker.Insert(key, value);
            }
        }
    }

    repli_timestamp_t max_ts = tracker.NextTimestamp();
    repli_timestamp_t min_del_ts = leaf::min_deletion_timestamp(
        tracker.sizer(), tracker.node(), max_ts);
    ASSERT_LT(min_del_ts.longtime, max_ts.longtime - 10);
    min_del_ts.longtime += (max_ts.longtime - min_del_ts.longtime) / 2;
    leaf::erase_deletions(tracker.sizer(), tracker.node(), min_del_ts);

    tracker.Verify();
}

TEST(LeafNodeTest, ZeroZeroMerging) {
    LeafNodeTracker left;
    LeafNodeTracker right;

    right.Merge(&left);
}

TEST(LeafNodeTest, ZeroOneMerging) {
    LeafNodeTracker left;
    LeafNodeTracker right;

    right.Insert(store_key_t("b"), "B");

    right.Merge(&left);
}

TEST(LeafNodeTest, OneZeroMerging) {
    LeafNodeTracker left;
    LeafNodeTracker right;

    left.Insert(store_key_t("a"), "A");

    right.Merge(&left);
}


TEST(LeafNodeTest, OneOneMerging) {
    LeafNodeTracker left;
    LeafNodeTracker right;

    left.Insert(store_key_t("a"), "A");
    right.Insert(store_key_t("b"), "B");

    right.Merge(&left);
}

TEST(LeafNodeTest, SimpleMerging) {
    LeafNodeTracker left;
    LeafNodeTracker right;

    // We use the largest value that will underflow.
    //
    // key_cost = 251, max_possible_size() = 256, sizeof(uint16_t) = 2, sizeof(repli_timestamp) = 8.
    //
    // 4084 - 12 = 4072.  4072 / 2 = 2036.  2036 - (251 + 256 + 2
    // + 8) = 2036 - 517 = 1519.  So 1518 is the max possible
    // mandatory_cost.  (See the is_underfull implementation.)
    //
    // With 5*8 mandatory timestamp bytes and 12 bytes per entry,
    // that gives us 1478 / 12 as the loop boundary value that
    // will underflow.  We get 12 byte entries if entries run from
    // a000 to a999.  But if we allow two-digit entries, that
    // frees up 2 bytes per entry, so add 200, giving 1678.  If we
    // allow one-digit entries, that gives us 20 more bytes to
    // use, giving 1698 / 12 as the loop boundary.  That's an odd
    // way to look at the arithmetic, but if you don't like that,
    // you can go cry to your mommy.

    for (int i = 0; i < 1698 / 12; ++i) {
        left.Insert(store_key_t(strprintf("a%d", i)), strprintf("A%d", i));
        right.Insert(store_key_t(strprintf("b%d", i)), strprintf("B%d", i));
    }

    right.Merge(&left);
}

TEST(LeafNodeTest, MergingWithRemoves) {
    LeafNodeTracker left;
    LeafNodeTracker right;

    for (int i = 0; i < (1698 * 5 / 6) / 12; ++i) {
        left.Insert(store_key_t(strprintf("a%d", i)), strprintf("A%d", i));
        right.Insert(store_key_t(strprintf("b%d", i)), strprintf("B%d", i));
        if (i % 5 == 0) {
            left.Remove(store_key_t(strprintf("a%d", i / 5)));
            right.Remove(store_key_t(strprintf("b%d", i / 5)));
        }
    }

    right.Merge(&left);
}

TEST(LeafNodeTest, MergingWithHugeEntries) {
    LeafNodeTracker left;
    LeafNodeTracker right;

    ASSERT_EQ(10, leaf::DELETION_RESERVE_FRACTION);

    // This test overflows the deletion reserve fraction with three
    // huge deletes.  One of them will not be merged.

    for (int i = 0; i < 4; ++i) {
        left.Insert(store_key_t(std::string(250, 'a' + i)), std::string(255, 'A' + i));
        right.Insert(store_key_t(std::string(250, 'n' + i)), std::string(255, 'N' + i));
    }

    for (int i = 0; i < 3; ++i) {
        left.Remove(store_key_t(std::string(250, 'a' + i)));
        right.Remove(store_key_t(std::string(250, 'n' + 1 + i)));
    }

    right.Merge(&left);
}

// A regression test for https://github.com/rethinkdb/rethinkdb/issues/4769
TEST(LeafNodeTest, MergingRegression4769) {
    LeafNodeTracker left;
    LeafNodeTracker right;

    // Fill the nodes with a bunch of deletion entries
    for (int i = 0; i < 100; ++i) {
        left.Insert(store_key_t(strprintf("a%d", i)), strprintf("A%d", i));
        right.Insert(store_key_t(strprintf("b%d", i)), strprintf("B%d", i));
        left.Remove(store_key_t(strprintf("a%d", i)));
        right.Remove(store_key_t(strprintf("b%d", i)));
    }

    // Then insert a bunch of keys to push some of them over the timestamp
    // cut-off point.
    for (int i = 0; i < 20; ++i) {
        left.Insert(store_key_t(strprintf("c%d", i)), strprintf("C%d", i));
        right.Insert(store_key_t(strprintf("d%d", i)), strprintf("D%d", i));
    }

    right.Merge(&left);
}


TEST(LeafNodeTest, LevelingLeftToRight) {
    LeafNodeTracker left;
    LeafNodeTracker right;

    // 4084 - 12 = 4072.  This is the maximum mandatory cost before
    // the node gets too big.  With 5*4 mandatory timestamp bytes and
    // 12 bytes per entry, that gives us 4052 / 12 as the last loop
    // boundary value that won't overflow.  We get 200 + 20 extra
    // bytes if we allow 90 two-digit and 10 one-digit key/values.  So
    // 4272 / 12 will be the last loop boundary value that won't
    // overflow.

    for (int i = 0; i < 4272 / 12; ++i) {
        left.Insert(store_key_t(strprintf("a%d", i)), strprintf("A%d", i));
    }

    right.Insert(store_key_t("b0"), "B0");

    bool could_level;
    right.Level(1, &left, &could_level);
    ASSERT_TRUE(could_level);
}

TEST(LeafNodeTest, LevelingLeftToZero) {
    LeafNodeTracker left;
    LeafNodeTracker right;

    for (int i = 0; i < 4272 / 12; ++i) {
        left.Insert(store_key_t(strprintf("a%d", i)), strprintf("A%d", i));
    }

    bool could_level;
    right.Level(1, &left, &could_level);
    ASSERT_TRUE(could_level);
}

TEST(LeafNodeTest, LevelingRightToLeft) {
    LeafNodeTracker left;
    LeafNodeTracker right;
    for (int i = 0; i < 4272 / 12; ++i) {
        right.Insert(store_key_t(strprintf("b%d", i)), strprintf("B%d", i));
    }

    left.Insert(store_key_t("a0"), "A0");

    bool could_level;
    left.Level(-1, &right, &could_level);
    ASSERT_TRUE(could_level);
}


TEST(LeafNodeTest, LevelingRightToZero) {
    LeafNodeTracker left;
    LeafNodeTracker right;
    for (int i = 0; i < 4272 / 12; ++i) {
        right.Insert(store_key_t(strprintf("b%d", i)), strprintf("B%d", i));
    }

    bool could_level;
    left.Level(-1, &right, &could_level);
    ASSERT_TRUE(could_level);
}

TEST(LeafNodeTest, Splitting) {
    LeafNodeTracker left;
    for (int i = 0; i < 4272 / 12; ++i) {
        left.Insert(store_key_t(strprintf("a%d", i)), strprintf("A%d", i));
    }

    LeafNodeTracker right;

    left.Split(&right);
}

TEST(LeafNodeTest, Fullness) {
    LeafNodeTracker node;
    int i;
    for (i = 0; i < 4272 / 12; ++i) {
        node.Insert(store_key_t(strprintf("a%d", i)), strprintf("A%d", i));
    }

    ASSERT_TRUE(node.IsFull(store_key_t(strprintf("a%d", i)), strprintf("A%d", i)));
}

}  // namespace unittest
