#include <algorithm>
#include <vector>

#include "unittest/gtest.hpp"
#include "unittest/server_test_helper.hpp"

#include "btree/node.hpp"
#include "logger.hpp"
#include "btree/buf_patches.hpp"
#include "btree/leaf_node.hpp"

#include "perfmon.hpp"          // for format(), oddly enough

namespace unittest {

repli_timestamp_t fake_timestamp = { -2 };

// TODO: Sperg out and make these tests much more brutal.

void expect_valid_value_shallowly(const memcached_value_t *value) {
    EXPECT_EQ(0, value->metadata_flags.flags & ~(MEMCACHED_FLAGS | MEMCACHED_CAS | MEMCACHED_EXPTIME));

    // size_t size = value->value_size();

    // TODO BLOB: Implement something equivalent for blobs.
    // if (value->is_large()) {
    //     EXPECT_GT(size, MAX_IN_NODE_VALUE_SIZE);

    //     // This isn't fsck, we can't follow large buf pointers.
    // } else {
    //     EXPECT_LE(size, MAX_IN_NODE_VALUE_SIZE);
    // }
}

void verify(value_sizer_t<memcached_value_t> *sizer, const leaf_node_t *buf, int expected_free_space) {

    int end_of_pair_offsets = offsetof(leaf_node_t, pair_offsets) + buf->npairs * 2;
    EXPECT_TRUE(buf->magic == leaf_node_t::expected_magic);
    ASSERT_LE(end_of_pair_offsets, buf->frontmost_offset);
    ASSERT_LE(buf->frontmost_offset, sizer->block_size().value());
    ASSERT_EQ(expected_free_space, buf->frontmost_offset - end_of_pair_offsets);

    std::vector<uint16_t> offsets(buf->pair_offsets, buf->pair_offsets + buf->npairs);
    std::sort(offsets.begin(), offsets.end());

    uint16_t expected = buf->frontmost_offset;
    for (std::vector<uint16_t>::const_iterator p = offsets.begin(), e = offsets.end(); p < e; ++p) {
        ASSERT_LE(expected, sizer->block_size().value());
        ASSERT_EQ(expected, *p);
        expected += leaf::pair_size<memcached_value_t>(sizer, leaf::get_pair<memcached_value_t>(buf, *p));
    }
    ASSERT_EQ(sizer->block_size().value(), expected);

    const btree_key_t *last_key = NULL;
    for (const uint16_t *p = buf->pair_offsets, *e = p + buf->npairs; p < e; ++p) {
        const btree_leaf_pair<memcached_value_t> *pair = leaf::get_pair<memcached_value_t>(buf, *p);
        const btree_key_t *next_key = &pair->key;

        if (last_key != NULL) {
            EXPECT_LT(leaf_key_comp::compare(last_key, next_key), 0);
        }

        last_key = next_key;
        expect_valid_value_shallowly(pair->value());
    }
}

TEST(LeafNodeTest, Offsets) {
    // Check leaf_node_t.
    EXPECT_EQ(0, offsetof(leaf_node_t, magic));
    EXPECT_EQ(4, offsetof(leaf_node_t, times));
    EXPECT_EQ(12, offsetof(leaf_node_t, npairs));
    EXPECT_EQ(14, offsetof(leaf_node_t, frontmost_offset));
    EXPECT_EQ(16, offsetof(leaf_node_t, pair_offsets));
    EXPECT_EQ(16, sizeof(leaf_node_t));


    // Check leaf_timestamps_t, since that's specifically part of leaf_node_t.
    EXPECT_EQ(0, offsetof(leaf_timestamps_t, last_modified));
    EXPECT_EQ(4, offsetof(leaf_timestamps_t, earlier));

    // Changing NUM_LEAF_NODE_EARLIER_TIMES changes the disk format.
    EXPECT_EQ(2, NUM_LEAF_NODE_EARLIER_TIMES);

    EXPECT_EQ(8, sizeof(leaf_timestamps_t));


    // Check btree_leaf_pair.
    EXPECT_EQ(0, offsetof(btree_leaf_pair<memcached_value_t>, key));

    btree_leaf_pair<memcached_value_t> p;
    p.key.size = 173;
    EXPECT_EQ(174, reinterpret_cast<char *>(p.value()) - reinterpret_cast<char *>(&p));

    EXPECT_EQ(1, sizeof(btree_key_t));
    EXPECT_EQ(1, offsetof(btree_key_t, contents));
    EXPECT_EQ(1, sizeof(memcached_value_t));

    EXPECT_EQ(1, sizeof(metadata_flags_t));

    EXPECT_EQ(4, sizeof(exptime_t));
    EXPECT_EQ(4, sizeof(mcflags_t));
    EXPECT_EQ(8, sizeof(cas_t));
}

class Value {
public:
    Value(const std::string& data = "", mcflags_t mcflags = 0, cas_t cas = 0, bool has_cas = false, exptime_t exptime = 0)
        : mcflags_(mcflags), cas_(cas), exptime_(exptime),
          has_cas_(has_cas),
          data_(data) {
        EXPECT_LE(data_.size(), MAX_IN_NODE_VALUE_SIZE);
    }

    void WriteBtreeValue(memcached_value_t *out, UNUSED block_size_t block_size) const {
        out->metadata_flags.flags = 0;

        ASSERT_LE(data_.size(), MAX_IN_NODE_VALUE_SIZE);
        if (has_cas_) {
            metadata_write(&out->metadata_flags, out->contents, mcflags_, exptime_, cas_);
        } else {
            metadata_write(&out->metadata_flags, out->contents, mcflags_, exptime_);
        }

        *reinterpret_cast<uint8_t *>(out->value_ref()) = data_.size();
        memcpy(out->value_ref() + 1, data_.data(), data_.size());
    }

    static Value Make(const memcached_value_t *v) {
        EXPECT_LE(*reinterpret_cast<const uint8_t *>(v->value_ref()), 250);
        return Value(std::string(v->value_ref() + 1, v->value_ref() + 1 + v->value_size()),
                     v->mcflags(), v->has_cas() ? v->cas() : 0, v->has_cas(), v->exptime());
    }

    bool operator==(const Value& rhs) const {
        return mcflags_ == rhs.mcflags_
            && exptime_ == rhs.exptime_
            && data_ == rhs.data_
            && has_cas_ == rhs.has_cas_
            && (has_cas_ ? cas_ == rhs.cas_ : true);
    }

    int full_size() const {
        return sizeof(memcached_value_t) + (mcflags_ ? sizeof(mcflags_t) : 0) + (has_cas_ ? sizeof(cas_t) : 0)
            + (exptime_ ? sizeof(exptime_t) : 0) + 1 + data_.size();
    }

    friend std::ostream& operator<<(std::ostream&, const Value&);

private:
    mcflags_t mcflags_;
    cas_t cas_;
    exptime_t exptime_;
    bool has_cas_;
    std::string data_;
};

std::ostream& operator<<(std::ostream& out, const Value& v) {
    out << "Value[mcflags=" << v.mcflags_;
    if (v.has_cas_) {
        out << " cas=" << v.cas_;
    }

    // TODO: string escape v.data_.
    out << " exptime=" << v.exptime_
        << " data='" << v.data_ << "']";
    return out;
}

class StackKey {
public:
    StackKey() {
        keyval.size = 0;
    }

    explicit StackKey(const std::string& key) {
        EXPECT_LE(key.size(), MAX_KEY_SIZE);
        keyval.size = key.size();
        memcpy(keyval.contents, key.c_str(), key.size());
    }

    const btree_key_t *look() const {
        return &keyval;
    }

    btree_key_t *look_write() {
        return &keyval;
    }

private:
    union {
        char keyval_padding[sizeof(btree_key_t) + MAX_KEY_SIZE];
        btree_key_t keyval;
    };
};

class StackValue {
public:
    StackValue() {
        memset(val_padding, 0, sizeof(val_padding));
    }
    StackValue(const Value& value, block_size_t block_size) {
        value.WriteBtreeValue(&val, block_size);
    }

    const memcached_value_t *look() const {
        return &val;
    }
    memcached_value_t *look_write() {
        return &val;
    }
private:
    union {
        char val_padding[MAX_BTREE_VALUE_SIZE];
        memcached_value_t val;
    };
};

class LeafNodeGrinder {
public:
    LeafNodeGrinder(transaction_t *txn) : sizer(txn->get_cache()->get_block_size()), bs(txn->get_cache()->get_block_size()), expected(), expected_frontmost_offset(bs.value()), expected_npairs(0), node_buf(txn->allocate()), initialized(false) {
        node = reinterpret_cast<leaf_node_t *>(node_buf->get_data_major_write());
    }

    ~LeafNodeGrinder() {
        node_buf->release();
    }

    static int space(int frontmont_offset, int npairs) {
        return frontmont_offset - (offsetof(leaf_node_t, pair_offsets) + sizeof(*((leaf_node_t *)0)->pair_offsets) * npairs);
    }

    int expected_space() const {
        return space(expected_frontmost_offset, expected_npairs);
    }

    int expected_used() const {
        return (bs.value() - offsetof(leaf_node_t, pair_offsets)) - expected_space();
    }

    void init() {
        SCOPED_TRACE("init");
        leaf::init(&sizer, reinterpret_cast<leaf_node_t *>(node_buf->get_data_major_write()), fake_timestamp);
        initialized = true;
        validate();
    }

    void insert_nocheck(const std::string& k, const Value& v) {
        SCOPED_TRACE(testing::Message() << "insert_nocheck k='" << k << "' v='" << v << "'");
        do_insert(k, v);
    }

    void insert(const std::string& k, const Value& v) {
        SCOPED_TRACE(testing::Message() << "insert k='" << k << "' v='" << v << "'");
        do_insert(k, v);
        validate();
    }

    void do_insert(const std::string& k, const Value& v) {
        ASSERT_TRUE(initialized);
        StackKey skey(k);
        StackValue sval(v, bs);

        if (expected_space() < int((1 + k.size()) + v.full_size() + sizeof(*node->pair_offsets))) {
            ASSERT_FALSE(leaf::insert<memcached_value_t>(&sizer, node_buf, skey.look(), sval.look(), fake_timestamp));
        } else {
            ASSERT_TRUE(leaf::insert<memcached_value_t>(&sizer, node_buf, skey.look(), sval.look(), fake_timestamp));

            std::pair<expected_t::iterator, bool> res = expected.insert(std::make_pair(k, v));
            if (res.second) {
                // The key didn't previously exist.
                expected_frontmost_offset -= (1 + k.size()) + v.full_size();
                expected_npairs++;
            } else {
                // The key previously existed.
                expected_frontmost_offset += res.first->second.full_size();
                expected_frontmost_offset -= v.full_size();
                res.first->second = v;
            }
        }
    }

    // Allows the key not to be in the node.
    void try_remove(const std::string& k) {
        SCOPED_TRACE("try_remove '" + k + "'");
        if (expected.find(k) != expected.end()) {
            do_remove(k);
            validate();
        }
    }

    // Expects the key to be in the node.
    void remove(const std::string& k) {
        SCOPED_TRACE("remove '" + k + "'");
        do_remove(k);
        validate();
    }

    void do_remove(const std::string& k) {
        ASSERT_TRUE(initialized);
        expected_t::iterator p = expected.find(k);

        // There's a bug in the test if we're trying to remove a key that's not there.
        ASSERT_TRUE(p != expected.end());

        // The key should be in there.
        StackKey skey(k);
        StackValue sval;

        ASSERT_TRUE(leaf::lookup(&sizer, node, skey.look(), sval.look_write()));

        leaf::remove(&sizer, node, skey.look());
        expected.erase(p);
        expected_frontmost_offset += (1 + k.size()) + sizer.size(sval.look());
        -- expected_npairs;
    }

    void validate() {
        SCOPED_TRACE("validate");
        ASSERT_TRUE(initialized);
        ASSERT_EQ(expected_npairs, node->npairs);
        ASSERT_EQ(expected_frontmost_offset, node->frontmost_offset);

        verify(&sizer, node, expected_space());

        for (expected_t::const_iterator p = expected.begin(), e = expected.end(); p != e; ++p) {
            SCOPED_TRACE("lookup '" + p->first + "'");
            lookup(p->first, p->second);
        }

        // We're confident there are no bad keys because
        // expected_free_space matched up.
    }

    void remove_all() {
        SCOPED_TRACE("remove_all");
        ASSERT_TRUE(initialized);

        std::vector<std::string> keys;
        for (expected_t::const_iterator p = expected.begin(), e = expected.end(); p != e; ++p) {
            keys.push_back(p->first);
        }

        for (std::vector<std::string>::iterator p = keys.begin(), e = keys.end(); p != e; ++p) {
            SCOPED_TRACE("removing '" + *p + "'");
            do_remove(*p);
        }

        validate();

        ASSERT_EQ(bs.value(), expected_frontmost_offset);
        ASSERT_EQ(0, expected_npairs);
    }

    void merge(const LeafNodeGrinder& lnode) {
        SCOPED_TRACE("merge");
        ASSERT_TRUE(initialized);

        ASSERT_EQ(bs.ser_value(), lnode.bs.ser_value());


        int next_expected_fo = bs.value() - ((bs.value() - expected_frontmost_offset) + (lnode.bs.value() - lnode.expected_frontmost_offset));
        int next_expected_npairs = expected_npairs + lnode.expected_npairs;

        // TODO: call merge under pathological conditions too, and
        // change merge so that it can refuse to merge.

        if (0 <= space(next_expected_fo, next_expected_npairs)) {
            // TODO: before we merge, add assertions that the keys fit
            // in two mutually exclusive intervals.

            StackKey skey;
            leaf::merge<memcached_value_t>(&sizer, lnode.node, node_buf, skey.look_write());

            for (expected_t::const_iterator p = lnode.expected.begin(), e = lnode.expected.end();
                 p != e;
                 ++p) {
                expected.insert(*p);
            }

            expected_frontmost_offset = next_expected_fo;
            expected_npairs = node->npairs;

            validate();
        }
    }

    void level(LeafNodeGrinder& sibling) {
        SCOPED_TRACE("level");
        ASSERT_TRUE(initialized);

        ASSERT_EQ(bs.ser_value(), sibling.bs.ser_value());

        StackKey key_to_replace;
        StackKey replacement_key;

        int cmp = leaf::nodecmp(node, sibling.node);

        int fo_sum = expected_frontmost_offset + sibling.expected_frontmost_offset;
        int npair_sum = expected_npairs + sibling.expected_npairs;

        leaf::level<memcached_value_t>(&sizer, node_buf, sibling.node_buf, key_to_replace.look_write(), replacement_key.look_write());

        // Sanity check that npairs and frontmost_offset are in sane ranges.

        ASSERT_LE(0, node->npairs);
        ASSERT_LE(0, sibling.node->npairs);

        // We move stuff *into* node.
        ASSERT_LE(expected_npairs, node->npairs);

        ASSERT_LE(node->frontmost_offset, bs.value());
        ASSERT_LE(sibling.node->frontmost_offset, bs.value());

        ASSERT_LE(0, space(node->frontmost_offset, node->npairs));
        ASSERT_LE(0, space(sibling.node->frontmost_offset, sibling.node->npairs));

        // Check that the size of our data is exactly the same.

        ASSERT_EQ(fo_sum, node->frontmost_offset + sibling.node->frontmost_offset);
        ASSERT_EQ(npair_sum, node->npairs + sibling.node->npairs);

        // Modify expecteds.  This test does not precisely calculate
        // how much leveling it expects to happen.

        int movement = node->npairs - expected_npairs;

        // We expect level to move things into node.
        ASSERT_LE(0, movement);
        ASSERT_LE(movement, sibling.expected_npairs);

        if (cmp < 0) {
            // We expect we moved things from the left end of sibling
            // onto the right end of node.

            for (int i = 0; i < movement; ++i) {
                // TODO: this could be slightly more efficient.
                expected_t::iterator p = sibling.expected.begin();
                expected.insert(*p);
                sibling.expected.erase(p);
            }
        } else {
            // We expect we moved things from the right end of sibling
            // onto the left end of node.

            for (int i = 0; i < movement; ++i) {
                expected_t::iterator p = sibling.expected.end();
                --p;
                expected.insert(*p);
                sibling.expected.erase(p);
            }
        }


        sibling.expected_frontmost_offset = sibling.node->frontmost_offset;
        sibling.expected_npairs = sibling.node->npairs;

        expected_frontmost_offset = node->frontmost_offset;
        expected_npairs = node->npairs;

        // TODO: We don't really check that leveling happens.
        // leaf::level could be a no-op and would pass
        // this test.

        validate();
        sibling.validate();
    }

    memcached_value_sizer_t sizer;
    block_size_t bs;
    typedef std::map<std::string, Value> expected_t;
    expected_t expected;
    int expected_frontmost_offset;
    int expected_npairs;
    buf_t *node_buf;
    leaf_node_t *node;
    bool initialized;

private:
    void lookup(const std::string& k, const Value& expected) {
        StackKey skey(k);
        StackValue sval;
        ASSERT_TRUE(leaf::lookup(&sizer, node, skey.look(), sval.look_write()));
        ASSERT_EQ(expected, Value::Make(sval.look()));
    }

    DISABLE_COPYING(LeafNodeGrinder);
};





block_size_t make_bs() {
    // TODO: Test weird block sizes.
    return block_size_t::unsafe_make(4096);
}

leaf_node_t *malloc_leaf_node(block_size_t bs) {
    return reinterpret_cast<leaf_node_t *>(malloc(bs.value()));
}

btree_key_t *malloc_key(const char *s) {
    size_t origlen = strlen(s);
    EXPECT_LE(origlen, MAX_KEY_SIZE);

    size_t len = std::min<size_t>(origlen, MAX_KEY_SIZE);
    btree_key_t *k = reinterpret_cast<btree_key_t *>(malloc(sizeof(btree_key_t) + len));
    k->size = len;
    memcpy(k->contents, s, len);
    return k;
}

memcached_value_t *malloc_value(const char *s) {
    size_t origlen = strlen(s);
    EXPECT_LE(origlen, MAX_IN_NODE_VALUE_SIZE);

    size_t len = std::min<size_t>(origlen, MAX_IN_NODE_VALUE_SIZE);

    memcached_value_t *v = reinterpret_cast<memcached_value_t *>(malloc(sizeof(memcached_value_t) + 1 + len));
    v->metadata_flags.flags = 0;
    *reinterpret_cast<uint8_t *>(v->value_ref()) = len;
    memcpy(v->value_ref() + 1, s, len);
    return v;
}

void fill_nonsense(LeafNodeGrinder& gr, const char *prefix, int max_space_filled) {
    std::string p(prefix);
    int i = 0;
    for (;;) {
        std::string key = p + strprintf("%d", i);
        Value value(strprintf("%d", i * i));
        if (gr.expected_used() + value.full_size() + int(key.size()) + 1 > max_space_filled) {
            break;
        }
        gr.insert(key, value);
        ++i;
    }
    gr.validate();
}

void fill_nonsense(LeafNodeGrinder& gr, const char *prefix) {
    fill_nonsense(gr, prefix, gr.expected_space());
}

class leaf_node_tester_t : public server_test_helper_t {
    void run_tests(cache_t *cache) {
        transaction_t txn(cache, rwi_write, 0, repli_timestamp_t::distant_past);
        test_initialization(&txn);
        test_insert_remove(&txn);
        test_merging(&txn);
        test_leveling(&txn);
    }

    void test_initialization(transaction_t *txn) {
        LeafNodeGrinder gr(txn);

        gr.init();
    }

    void InsertRemoveHelper(transaction_t *txn, const std::string& key, const char *value) {
        SCOPED_TRACE(std::string("InsertRemoveHelper(key='") + key + "' value='" + value + "'");

        LeafNodeGrinder gr(txn);

        gr.init();
        gr.insert(key, Value(value));
        gr.remove(key);
    }

    void test_insert_remove(transaction_t *txn) {
        InsertRemoveHelper(txn, "the_key", "the_value");
        InsertRemoveHelper(txn, "the_key", "");
        InsertRemoveHelper(txn, "", "the_value");
        InsertRemoveHelper(txn, "", "");
    }

    void test_merging(transaction_t *txn) {

        LeafNodeGrinder x(txn);
        LeafNodeGrinder y(txn);

        x.init();
        y.init();

        // Empty node merging.
        x.merge(y);

        fill_nonsense(x, "x");

        // Merge nothing into something
        x.merge(y);

        // Merge something into nothing
        y.merge(x);

        // Merge two full things.
        y.remove_all();
        fill_nonsense(y, "y");

        // TODO: This doesn't really try merging them.
        x.merge(y);

        x.remove_all();
        y.remove_all();

        fill_nonsense(x, "x", x.expected_space() / 3);
        fill_nonsense(y, "y", y.expected_space() / 3);

        y.merge(x);

    }

    void test_leveling(transaction_t *txn) {

        LeafNodeGrinder x(txn);
        LeafNodeGrinder y(txn);

        x.init();
        y.init();

        // Empty node leveling.
        x.level(y);

        fill_nonsense(x, "x");

        // Leveling nothing into something.
        x.level(y);

        // Leveling something into nothing.
        y.level(x);

        x.remove_all();
        y.remove_all();

        fill_nonsense(x, "x", x.expected_space() / 2);
        fill_nonsense(y, "y", y.expected_space() / 2);

        x.level(y);

    }
};

TEST(LeafNodeTest, all_tests) {
    leaf_node_tester_t().run();
}


}  // namespace unittest

