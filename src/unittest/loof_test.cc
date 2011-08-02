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
    }
    ~LoofTracker() { delete[] node_; }

    bool Insert(const std::string& key, const std::string& value) {
        btree_key_buffer_t k(key.begin(), key.end());
        short_value_buffer_t v(value);

        if (loof::is_full(&sizer_, node_, k.key(), v.data())) {
            return false;
        }

        repli_timestamp_t tstamp;
        loof::insert(&sizer_, node_, k.key(), v.data(), tstamp);

        kv_[key] = value;
    }

    repli_timestamp_t NextTimestamp() {
        tstamp_counter_ ++;
        repli_timestamp_t ret;
        ret.time = tstamp_counter_;
        return ret;
    }


private:
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



}  // namespace unittest
