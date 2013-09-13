// Copyright 2010-2012 RethinkDB, all rights reserved.
#include <map>

#include "btree/leaf_node.hpp"
#include "btree/node.hpp"
#include "containers/scoped.hpp"
#include "repli_timestamp.hpp"
#include "unittest/gtest.hpp"


struct short_value_t;

template <>
class value_sizer_t<short_value_t> : public value_sizer_t<void> {
public:
    explicit value_sizer_t<short_value_t>(block_size_t bs) : block_size_(bs) { }

    int size(const void *value) const {
        int x = *reinterpret_cast<const uint8_t *>(value);
        return 1 + x;
    }

    bool fits(const void *value, int length_available) const {
        return length_available > 0 && size(value) <= length_available;
    }

    int max_possible_size() const {
        return 256;
    }

    block_magic_t btree_leaf_magic() const {
        block_magic_t magic = { { 's', 'h', 'L', 'F' } };
        return magic;
    }

    block_size_t block_size() const { return block_size_; }

private:
    block_size_t block_size_;

    DISABLE_COPYING(value_sizer_t<short_value_t>);
};

namespace unittest {

class short_value_buffer_t {
public:
    explicit short_value_buffer_t(const short_value_t *v) {
        memcpy(data_, v, reinterpret_cast<const uint8_t *>(v)[0] + 1);
    }

    explicit short_value_buffer_t(const std::string& v) {
        rassert(v.size() <= 255);
        data_[0] = v.size();
        memcpy(data_ + 1, v.data(), v.size());
    }

    short_value_t *data() {
        return reinterpret_cast<short_value_t *>(data_);
    }

    std::string as_str() const {
        return std::string(data_ + 1, data_ + 1 + data_[0]);
    }

private:
    uint8_t data_[256];
};

class LeafNodeTracker {
public:
    LeafNodeTracker() : bs_(block_size_t::unsafe_make(4096)), sizer_(bs_), node_(bs_.value()),
                        tstamp_counter_(0) {
        leaf::init(&sizer_, node_.get());
        Print();
    }

    leaf_node_t *node() { return node_.get(); }

    bool Insert(const store_key_t& key, const std::string& value, repli_timestamp_t tstamp) {
        short_value_buffer_t v(value);

        if (leaf::is_full(&sizer_, node(), key.btree_key(), v.data())) {
            Print();

            Verify();
            return false;
        }

        leaf::insert(&sizer_, node(), key.btree_key(), v.data(), tstamp, key_modification_proof_t::real_proof());

        kv_[key] = value;

        Print();

        Verify();
        return true;
    }

    bool Insert(const store_key_t &key, const std::string &value) {
        return Insert(key, value, NextTimestamp());
    }

    void Remove(const store_key_t& key, repli_timestamp_t tstamp) {
        ASSERT_TRUE(ShouldHave(key));

        kv_.erase(key);

        leaf::remove(&sizer_, node(), key.btree_key(), tstamp, key_modification_proof_t::real_proof());

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
        bool can_level = leaf::level(&sizer_, nodecmp_value, node(), sibling->node(), replacement.btree_key());

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
            std::map<store_key_t, std::string>::iterator prev = p;
            --p;
            kv_.erase(prev);
        }

        ASSERT_EQ(key_to_unescaped_str(p->first), key_to_unescaped_str(median));
    }

    bool IsFull(const store_key_t& key, const std::string& value) {
        short_value_buffer_t value_buf(value);
        return leaf::is_full(&sizer_, node(), key.btree_key(), value_buf.data());
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
        // leaf::print(stdout, &sizer_, node_);
    }

    class verify_receptor_t : public leaf::entry_reception_callback_t {
    public:
        verify_receptor_t() : got_lost_deletions_(false) { }

        void lost_deletions() {
            ASSERT_FALSE(got_lost_deletions_);
            got_lost_deletions_ = true;
        }

        void deletion(UNUSED const btree_key_t *k, UNUSED repli_timestamp_t tstamp) {
            ASSERT_TRUE(false);
        }

        void key_value(const btree_key_t *k, const void *v_value, UNUSED repli_timestamp_t tstamp) {
            ASSERT_TRUE(got_lost_deletions_);
            const short_value_t *value = static_cast<const short_value_t *>(v_value);

            store_key_t k_buf(k);
            short_value_buffer_t v_buf(value);
            std::string v_str = v_buf.as_str();

            ASSERT_TRUE(kv_map_.find(k_buf) == kv_map_.end());
            kv_map_[k_buf] = v_str;
        }

        const std::map<store_key_t, std::string>& map() const { return kv_map_; }

    private:
        bool got_lost_deletions_;

        std::map<store_key_t, std::string> kv_map_;
    };

    void printmap(const std::map<store_key_t, std::string>& m) {
        for (std::map<store_key_t, std::string>::const_iterator p = m.begin(), q = m.end(); p != q; ++p) {
            printf("%s: %s;", key_to_debug_str(p->first).c_str(), p->second.c_str());
        }
    }


    void Verify() {
        // Of course, this will fail with rassert, not a gtest assertion.
        leaf::validate(&sizer_, node());

        verify_receptor_t receptor;
        repli_timestamp_t max_possible_tstamp = { tstamp_counter_ };
        leaf::dump_entries_since_time(&sizer_, node(), repli_timestamp_t::distant_past, max_possible_tstamp, &receptor);

        if (receptor.map() != kv_) {
            printf("receptor.map(): ");
            printmap(receptor.map());
            printf("\nkv_: ");
            printmap(kv_);
            printf("\n");
        }
        ASSERT_TRUE(receptor.map() == kv_);
    }

public:
    block_size_t bs_;
    value_sizer_t<short_value_t> sizer_;
    scoped_malloc_t<leaf_node_t> node_;

    uint64_t tstamp_counter_;

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

TEST(LeafNodeTest, RandomOutOfOrder) {
    for (int try_num = 0; try_num < 10; ++try_num) {
        LeafNodeTracker tracker;

        rng_t rng;

        const int num_keys = 10;
        store_key_t key_pool[num_keys];
        for (int i = 0; i < num_keys; ++i) {
            key_pool[i].set_size(rng.randint(160));
            char letter = 'a' + rng.randint(26);
            for (int j = 0; j < key_pool[i].size(); ++j) {
                key_pool[i].contents()[j] = letter;
            }
        }

        const int num_ops = 10000;
        for (int i = 0; i < num_ops; ++i) {
            const store_key_t &key = key_pool[rng.randint(num_keys)];
            repli_timestamp_t tstamp;
            tstamp.longtime = rng.randint(num_ops);

            if (rng.randint(2) == 1) {
                std::string value;
                int length = rng.randint(160);
                char letter = 'a' + rng.randint(26);
                for (int j = 0; j < length; ++j) {
                    value.push_back(letter);
                }
                /* If the key doesn't fit, that's OK; it will just not insert it
                and return `false`, which we ignore. */
                tracker.Insert(key, value, tstamp);
            } else {
                if (tracker.ShouldHave(key)) {
                    tracker.Remove(key);
                }
            }
        }
    }
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
