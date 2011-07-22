#ifndef __SERVER_NESTED_DEMO_REDIS_UTILS_HPP_
#define	__SERVER_NESTED_DEMO_REDIS_UTILS_HPP_

#include "btree/operations.hpp"

/* nested string value type */
// TODO! Replace this by a version that uses blobs!
struct redis_nested_string_value_t {
    int length;
    char contents[];

public:
    int inline_size(UNUSED block_size_t bs) const {
        return sizeof(length) + length;
    }

    int64_t value_size() const {
        return length;
    }

    const char *value_ref() const { return contents; }
    char *value_ref() { return contents; }
};
template <>
class value_sizer_t<redis_nested_string_value_t> {
public:
    value_sizer_t<redis_nested_string_value_t>(block_size_t bs) : block_size_(bs) { }

    int size(const redis_nested_string_value_t *value) const {
        return value->inline_size(block_size_);
    }

    bool fits(UNUSED const redis_nested_string_value_t *value, UNUSED int length_available) const {
        // TODO!
        return true;
    }

    int max_possible_size() const {
        // TODO?
        return MAX_BTREE_VALUE_SIZE;
    }

    block_magic_t btree_leaf_magic() const {
        block_magic_t magic = { { 'l', 'e', 'a', 'f' } };  // TODO!
        return magic;
    }

    block_size_t block_size() const { return block_size_; }

protected:
    // The block size.  It's convenient for leaf node code and for
    // some subclasses, too.
    block_size_t block_size_;
};


/* nested empty value type */
struct redis_nested_empty_value_t {
    int inline_size(UNUSED block_size_t bs) const {
        return 0;
    }

    int64_t value_size() const {
        return 0;
    }

    const char *value_ref() const { return NULL; }
    char *value_ref() { return NULL; }
};
template <>
class value_sizer_t<redis_nested_empty_value_t> {
public:
    value_sizer_t<redis_nested_empty_value_t>(block_size_t bs) : block_size_(bs) { }

    int size(const redis_nested_empty_value_t *value) const {
        return value->inline_size(block_size_);
    }

    bool fits(UNUSED const redis_nested_empty_value_t *value, UNUSED int length_available) const {
        return true;
    }

    int max_possible_size() const {
        return 0;
    }

    block_magic_t btree_leaf_magic() const {
        block_magic_t magic = { { 'l', 'e', 'a', 'f' } }; // TODO!
        return magic;
    }

    block_size_t block_size() const { return block_size_; }

protected:
    // The block size.  It's convenient for leaf node code and for
    // some subclasses, too.
    block_size_t block_size_;
};

namespace redis_utils {
    /* Constructs a btree_key_t from an std::string and puts it into out_buf */
    void construct_key(const std::string &key, scoped_malloc<btree_key_t> *out_buf);

    /* Convenience accessor functions for nested trees */
    void find_nested_keyvalue_location_for_write(block_size_t block_size, boost::shared_ptr<transaction_t> &transaction, const std::string &field, repli_timestamp_t ts, keyvalue_location_t<redis_nested_string_value_t> *kv_location, scoped_malloc<btree_key_t> *btree_key, const block_id_t nested_root);
    void find_nested_keyvalue_location_for_read(block_size_t block_size, boost::shared_ptr<transaction_t> &transaction, const std::string &field, keyvalue_location_t<redis_nested_string_value_t> *kv_location, const block_id_t nested_root);

    /* Conversion between int and a lexicographical string representation of those */
    const size_t LEX_INT_SIZE = sizeof(int);
    void to_lex_int(const int i, char* buf);
    int from_lex_int(char* buf);

    // TODO! (later) lex. float key representation
    const size_t LEX_FLOAT_SIZE = 0;
    void to_lex_float(const float f, UNUSED char* buf);
    float from_lex_float(char* buf);
} /* namespace redis_utils */

#endif	/* __SERVER_NESTED_DEMO_REDIS_UTILS_HPP_ */

