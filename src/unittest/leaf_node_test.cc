#include <algorithm>
#include <vector>

#include "unittest/gtest.hpp"

#include "btree/node.hpp"
#include "btree/leaf_node.hpp"

namespace unittest {

// TODO: This is rather duplicative of fsck::check_value.
void expect_valid_value_shallowly(const btree_value *value) {
    EXPECT_EQ(0, value->metadata_flags & ~(MEMCACHED_FLAGS | MEMCACHED_CAS | MEMCACHED_EXPTIME | LARGE_VALUE));

    size_t size = value->value_size();

    if (value->is_large()) {
        EXPECT_GT(size, MAX_IN_NODE_VALUE_SIZE);

        // This isn't fsck, we can't follow large buf pointers.
    } else {
        EXPECT_LE(size, MAX_IN_NODE_VALUE_SIZE);
    }
}

// TODO: This is rather duplicative of fsck::check_subtree_leaf_node.
void verify(block_size_t block_size, const leaf_node_t *buf, int expected_free_space) {

    int end_of_pair_offsets = offsetof(leaf_node_t, pair_offsets) + buf->npairs * 2;
    ASSERT_LE(end_of_pair_offsets, buf->frontmost_offset);
    ASSERT_LE(buf->frontmost_offset, block_size.value());
    ASSERT_EQ(expected_free_space, buf->frontmost_offset - end_of_pair_offsets);

    std::vector<uint16_t> offsets(buf->pair_offsets, buf->pair_offsets + buf->npairs);
    std::sort(offsets.begin(), offsets.end());

    uint16_t expected = buf->frontmost_offset;
    for (std::vector<uint16_t>::const_iterator p = offsets.begin(), e = offsets.end(); p < e; ++p) {
        ASSERT_LE(expected, block_size.value());
        ASSERT_EQ(expected, *p);
        expected += leaf_node_handler::pair_size(leaf_node_handler::get_pair(buf, *p));
    }
    ASSERT_EQ(block_size.value(), expected);

    const btree_key *last_key = NULL;
    for (const uint16_t *p = buf->pair_offsets, *e = p + buf->npairs; p < e; ++p) {
        const btree_leaf_pair *pair = leaf_node_handler::get_pair(buf, *p);
        if (last_key != NULL) {
            const btree_key *next_key = &pair->key;
            EXPECT_LT(leaf_key_comp::compare(last_key, next_key), 0);
            last_key = next_key;
        }
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
    EXPECT_EQ(0, offsetof(btree_leaf_pair, key));

    btree_leaf_pair p;
    p.key.size = 173;
    EXPECT_EQ(174, reinterpret_cast<byte *>(p.value()) - reinterpret_cast<byte *>(&p));

    EXPECT_EQ(1, sizeof(btree_key));
    EXPECT_EQ(1, offsetof(btree_key, contents));
    EXPECT_EQ(2, sizeof(btree_value));
}

class Value {
public:
    Value(const std::string& data = "", mcflags_t mcflags = 0, cas_t cas = 0, bool has_cas = false, exptime_t exptime = 0)
        : mcflags_(mcflags), cas_(cas), exptime_(exptime),
          has_cas_(has_cas),
          data_(data) {
        EXPECT_LE(data_.size(), MAX_IN_NODE_VALUE_SIZE);
    }

    void WriteBtreeValue(btree_value *out) const {
        out->size = 0;
        out->metadata_flags = 0;

        ASSERT_LE(data_.size(), MAX_IN_NODE_VALUE_SIZE);
        out->value_size(data_.size());
        out->set_mcflags(mcflags_);
        out->set_exptime(exptime_);
        if (has_cas_) {
            out->set_cas(cas_);
        }
        memcpy(out->value(), data_.c_str(), data_.size());
    }

    static Value Make(const btree_value *v) {
        EXPECT_FALSE(v->is_large());
        return Value(std::string(v->value(), v->value() + v->value_size()),
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
        return sizeof(btree_value) + (mcflags_ ? sizeof(mcflags_t) : 0) + (has_cas_ ? sizeof(cas_t) : 0)
            + (exptime_ ? sizeof(exptime_t) : 0) + data_.size();
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
    explicit StackKey(const std::string& key) {
        EXPECT_LE(key.size(), MAX_KEY_SIZE);
        keyval.size = key.size();
        memcpy(keyval.contents, key.c_str(), key.size());
    }

    const btree_key *look() const {
        return &keyval;
    }

private:
    union {
        byte keyval_padding[sizeof(btree_key) + MAX_KEY_SIZE];
        btree_key keyval;
    };
};

class StackValue {
public:
    StackValue() {
        memset(val_padding, 0, sizeof(val_padding));
    }
    explicit StackValue(const Value& value) {
        value.WriteBtreeValue(&val);
    }
    const btree_value *look() const {
        return &val;
    }
    btree_value *look_write() {
        return &val;
    }
private:
    union {
        byte val_padding[sizeof(btree_value) + MAX_TOTAL_NODE_CONTENTS_SIZE];
        btree_value val;
    };
};

class LeafNodeGrinder {
public:
    LeafNodeGrinder(int block_size) : bs(block_size_t::unsafe_make(block_size)), expected(), expected_frontmost_offset(bs.value()), expected_npairs(0), node(reinterpret_cast<leaf_node_t *>(malloc(bs.value()))), initialized(false) {
    }

    ~LeafNodeGrinder() {
        free(node);
    }

    int expected_space() const {
        return expected_frontmost_offset - (offsetof(leaf_node_t, pair_offsets) + sizeof(*node->pair_offsets) * expected_npairs);
    }

    void init() {
        SCOPED_TRACE("init");
        leaf_node_handler::init(bs, node, repli_timestamp::invalid);
        initialized = true;
        validate();
    }

    void insert(const std::string& k, const Value& v) {
        SCOPED_TRACE("insert");
        ASSERT_TRUE(initialized);
        StackKey skey(k);
        StackValue sval(v);

        if (expected_space() < int((1 + k.size()) + v.full_size() + sizeof(*node->pair_offsets))) {
            ASSERT_FALSE(leaf_node_handler::insert(bs, node, skey.look(), sval.look(), repli_timestamp::invalid));
        } else {
            ASSERT_TRUE(leaf_node_handler::insert(bs, node, skey.look(), sval.look(), repli_timestamp::invalid));

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

        validate();
    }

    // Expects the key to be in the node.
    void remove(const std::string& k) {
        SCOPED_TRACE("remove");
        ASSERT_TRUE(initialized);
        expected_t::iterator p = expected.find(k);

        // There's a bug in the test if we're trying to remove a key that's not there.
        ASSERT_TRUE(p != expected.end());

        // The key should be in there.
        StackKey skey(k);
        StackValue sval;

        ASSERT_TRUE(leaf_node_handler::lookup(node, skey.look(), sval.look_write()));

        leaf_node_handler::remove(bs, node, skey.look());
        expected.erase(p);
        expected_frontmost_offset += (1 + k.size()) + sval.look()->full_size();
        -- expected_npairs;

        validate();
    }

    void validate() const {
        SCOPED_TRACE("validate");
        ASSERT_TRUE(initialized);
        ASSERT_EQ(expected_npairs, node->npairs);
        ASSERT_EQ(expected_frontmost_offset, node->frontmost_offset);

        verify(bs, node, expected_space());

        for (expected_t::const_iterator p = expected.begin(), e = expected.end(); p != e; ++p) {
            lookup(p->first, p->second);
        }

        // We're confident there are no bad keys because
        // expected_free_space matched up.
    }

private:
    void lookup(const std::string& k, const Value& expected) const {
        StackKey skey(k);
        StackValue sval;
        ASSERT_TRUE(leaf_node_handler::lookup(node, skey.look(), sval.look_write()));
        ASSERT_EQ(expected, Value::Make(sval.look()));
    }

    block_size_t bs;
    typedef std::map<std::string, Value> expected_t;
    expected_t expected;
    int expected_frontmost_offset;
    int expected_npairs;
    leaf_node_t *node;
    bool initialized;
};





block_size_t make_bs() {
    // TODO: Test weird block sizes.
    return block_size_t::unsafe_make(4096);
}

leaf_node_t *malloc_leaf_node(block_size_t bs) {
    return reinterpret_cast<leaf_node_t *>(malloc(bs.value()));
}

btree_key *malloc_key(const char *s) {
    size_t origlen = strlen(s);
    EXPECT_LE(origlen, MAX_KEY_SIZE);

    size_t len = std::min<size_t>(origlen, MAX_KEY_SIZE);
    btree_key *k = reinterpret_cast<btree_key *>(malloc(sizeof(btree_key) + len));
    k->size = len;
    memcpy(k->contents, s, len);
    return k;
}

btree_value *malloc_value(const char *s) {
    size_t origlen = strlen(s);
    EXPECT_LE(origlen, MAX_IN_NODE_VALUE_SIZE);

    size_t len = std::min<size_t>(origlen, MAX_IN_NODE_VALUE_SIZE);

    btree_value *v = reinterpret_cast<btree_value *>(malloc(sizeof(btree_value) + len));
    v->size = len;
    v->metadata_flags = 0;
    memcpy(v->value(), s, len);
    return v;
}

TEST(LeafNodeTest, Initialization) {
    LeafNodeGrinder gr(4096);

    gr.init();
}

void InsertRemoveHelper(const std::string& key, const char *value) {
    LeafNodeGrinder gr(4096);

    gr.init();
    gr.insert(key, Value(value));
    gr.remove(key);
}

TEST(LeafNodeTest, ElementaryInsertLookupRemove) {
    InsertRemoveHelper("the_key", "the_value");
}
TEST(LeafNodeTest, EmptyValueInsertLookupRemove) {
    InsertRemoveHelper("the_key", "");
}
TEST(LeafNodeTest, EmptyKeyInsertLookupRemove) {
    InsertRemoveHelper("", "the_value");
}
TEST(LeafNodeTest, EmptyKeyValueInsertLookupRemove) {
    InsertRemoveHelper("", "");
}







}  // namespace unittest

