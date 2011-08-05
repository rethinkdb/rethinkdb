#include <map>
#include "unittest/gtest.hpp"


#include "btree/loof_node.hpp"


struct short_value_t;

template <>
class value_sizer_t<short_value_t> {
public:
    value_sizer_t<short_value_t>(block_size_t bs) : block_size_(bs) { }

    int size(const short_value_t *value) const {
        int x = *reinterpret_cast<const uint8_t *>(value);
        return 1 + x;
    }

    bool fits(const short_value_t *value, int length_available) const {
        return length_available > 0 && size(value) <= length_available;
    }

    int max_possible_size() const {
        return 256;
    }

    static block_magic_t btree_leaf_magic() {
        block_magic_t magic = { { 's', 'h', 'L', 'F' } };
        return magic;
    }

    block_size_t block_size() const { return block_size_; }

private:
    block_size_t block_size_;

    DISABLE_COPYING(value_sizer_t<short_value_t>);
};

using loof::loof_t;

namespace unittest {

class short_value_buffer_t {
public:
    short_value_buffer_t(const std::string& v) {
        rassert(v.size() < 255);
        data_[0] = v.size();
        memcpy(data_ + 1, v.data(), v.size());
    }

    short_value_t *data() {
        return reinterpret_cast<short_value_t *>(data_);
    }

private:
    uint8_t data_[256];
};

class LoofTracker {
public:
    LoofTracker() : bs_(block_size_t::unsafe_make(4096)), sizer_(bs_), node_(reinterpret_cast<loof_t *>(new char[bs_.value()])),
                    tstamp_counter_(0) {
        loof::init(&sizer_, node_);
        Print();
    }
    ~LoofTracker() { delete[] node_; }

    bool Insert(const std::string& key, const std::string& value) {
        // printf("\n\nInserting %s\n\n", key.c_str());
        btree_key_buffer_t k(key.begin(), key.end());
        short_value_buffer_t v(value);

        if (loof::is_full(&sizer_, node_, k.key(), v.data())) {
            Print();

            Verify();
            return false;
        }

        repli_timestamp_t tstamp = NextTimestamp();
        loof::insert(&sizer_, node_, k.key(), v.data(), tstamp);

        kv_[key] = value;

        Print();

        Verify();
        return true;
    }

    void Remove(const std::string& key) {
        // printf("\n\nRemoving %s\n\n", key.c_str());
        btree_key_buffer_t k(key.begin(), key.end());

        ASSERT_TRUE(ShouldHave(key));

        kv_.erase(key);

        repli_timestamp_t tstamp = NextTimestamp();
        loof::remove(&sizer_, node_, k.key(), tstamp);

        Verify();

        Print();
    }

    void Merge(LoofTracker& lnode) {
        ASSERT_EQ(bs_.ser_value(), lnode.bs_.ser_value());

        btree_key_buffer_t buf;
        loof::merge(&sizer_, lnode.node_, node_, buf.key());

        int old_kv_size = kv_.size();
        for (std::map<std::string, std::string>::iterator p = lnode.kv_.begin(), e = lnode.kv_.end(); p != e; ++p) {
            kv_[p->first] = p->second;
        }

        ASSERT_EQ(kv_.size(), old_kv_size + lnode.kv_.size());

        lnode.kv_.clear();

        Verify();
        lnode.Verify();
    }

    bool ShouldHave(const std::string& key) {
        return kv_.end() != kv_.find(key);
    }

    repli_timestamp_t NextTimestamp() {
        tstamp_counter_ ++;
        repli_timestamp_t ret;
        ret.time = tstamp_counter_;
        return ret;
    }

    // This only prints if we enable printing.
    void Print() {
        // loof::print(stdout, &sizer_, node_);
    }

    void Verify() {
        if (!kv_.empty()) {
            ASSERT_EQ(tstamp_counter_, reinterpret_cast<repli_timestamp_t *>(loof::get_at_offset(node_, node_->frontmost))->time);
        }

        // Of course, this will fail with rassert, not a gtest assertion.
        loof::validate(&sizer_, node_);
    }

public:
    block_size_t bs_;
    value_sizer_t<short_value_t> sizer_;
    loof_t *node_;

    int tstamp_counter_;

    std::map<std::string, std::string> kv_;


    DISABLE_COPYING(LoofTracker);
};

TEST(LoofTest, Offsets) {
    ASSERT_EQ(0, offsetof(loof_t, magic));
    ASSERT_EQ(4, offsetof(loof_t, num_pairs));
    ASSERT_EQ(6, offsetof(loof_t, live_size));
    ASSERT_EQ(8, offsetof(loof_t, frontmost));
    ASSERT_EQ(10, offsetof(loof_t, tstamp_cutpoint));
    ASSERT_EQ(12, offsetof(loof_t, pair_offsets));
    ASSERT_EQ(12, sizeof(loof_t));
}

TEST(LoofTest, Reinserts) {
    LoofTracker tracker;
    std::string v = "aa";
    std::string k = "key";
    for (; v[0] <= 'z'; ++v[0]) {
        v[1] = 'a';
        for (; v[1] <= 'z'; ++v[1]) {
            // printf("inserting %s\n", v.c_str());
            tracker.Insert(k, v);
        }
    }
}

TEST(LoofTest, TenInserts) {
    LoofTracker tracker;

    ASSERT_LT(loof::MANDATORY_TIMESTAMPS, 10);

    const int num_keys = 10;
    std::string ks[num_keys] = { "the_relatively_long_key_that_is_relatively_long,_eh?__or_even_longer",
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
            tracker.Insert(ks[j], v);
        }
    }
}

TEST(LoofTest, InsertRemove) {
    LoofTracker tracker;

    srand(12345);

    const int num_keys = 10;
    std::string ks[num_keys] = { "the_relatively_long_key_that_is_relatively_long,_eh?__or_even_longer",
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
            if (rand() % 2 == 1) {
                tracker.Insert(ks[j], v);
            } else {
                if (tracker.ShouldHave(ks[j])) {
                    tracker.Remove(ks[j]);
                }
            }
        }
    }
}

TEST(LoofTest, MinimalMerging) {
    LoofTracker left;
    LoofTracker right;

    left.Insert("a", "A");
    right.Insert("b", "B");

    right.Merge(left);
}

TEST(LoofTest, SimpleMerging) {

    LoofTracker left;
    LoofTracker right;

    // We use the largest value that will underflow.
    //
    // key_cost = 251, max_possible_size() = 256, sizeof(uint16_t) = 2, sizeof(repli_timestamp) = 4.
    //
    // 4084 - 12 = 4072.  4072 / 2 = 2036.  2036 - (251 + 256 + 2
    // + 4) = 2036 - 513 = 1523.  So 1522 is the max possible
    // mandatory_cost.  (See the is_underfull implementation.)
    //
    // With 5*4 mandatory timestamp bytes and 12 bytes per entry,
    // that gives us 1502 / 12 as the loop boundary value that
    // will underflow.  We get 12 byte entries if entries run from
    // a000 to a999.  But if we allow two-digit entries, that
    // frees up 2 bytes per entry, so add 200, giving 1702.  If we
    // allow one-digit entries, that gives us 20 more bytes to
    // use, giving 1722 / 12 as the loop boundary.  That's an odd
    // way to look at the arithmetic, but if you don't like that,
    // you can go cry to your mommy.

    for (int i = 0; i < 1722 / 12; ++i) {
        left.Insert(strprintf("a%d", i), strprintf("A%d", i));
        right.Insert(strprintf("b%d", i), strprintf("B%d", i));
    }

    right.Merge(left);
}

TEST(LoofTest, MergingWithRemoves) {
    LoofTracker left;
    LoofTracker right;

    for (int i = 0; i < 21 /* 1000 / 12 */; ++i) {
        left.Insert(strprintf("a%d", i), strprintf("A%d", i));
        right.Insert(strprintf("b%d", i), strprintf("B%d", i));
        if (i % 5 == 0) {
            left.Remove(strprintf("a%d", i / 5));
            right.Remove(strprintf("b%d", i / 5));
        }
    }

    right.Merge(left);
}


}  // namespace unittest
