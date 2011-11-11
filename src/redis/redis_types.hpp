#ifndef __PROTOCOL_REDIS_TYPES_H__
#define __PROTOCOL_REDIS_TYPES_H__

#include <vector>
#include <inttypes.h>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>

#include "serializer/types.hpp"
#include "buffer_cache/blob.hpp"
#include "btree/node.hpp"

enum redis_value_type {
    REDIS_STRING,
    REDIS_LIST,
    REDIS_HASH,
    REDIS_SET,
    REDIS_SORTED_SET,
};

//Base class of all redis value types. Contains metadata relevant to all redis value types.
struct redis_value_t {
    // Bits 0-2 reserved for type field. Bit 3 indicates expiration. Bit 4-7 reserved for the various value types.
    uint8_t flags;
    uint32_t metadata_values[0];

    static const uint8_t EXPIRATION_FLAG_MASK = (1 << 4);
    static const uint8_t TYPE_FLAGS_MASK = static_cast<uint8_t>(7 << 5);

    redis_value_type get_redis_type() const {
        //The first three bits encode the redis type
        //for a total of 8 possible types. Redis only
        //currently supports 5 types for now though.
        return static_cast<redis_value_type>(flags >> 5);
    }

    void set_redis_type(redis_value_type type) {
        flags = ((flags | TYPE_FLAGS_MASK) ^ TYPE_FLAGS_MASK) | (type << 5);
    }

    bool expiration_set() const {
        //The 4th bit indicates if the this key has an
        //expiration time set
        return static_cast<bool>(flags & EXPIRATION_FLAG_MASK);
    }

    // Warning, the caller must already have made space for the expiration time value
    void set_expiration(uint32_t expire_time) {
        flags |= EXPIRATION_FLAG_MASK;
        metadata_values[0] = expire_time;
    }

    // Warning, the caller is responsible for ensuring that the expiration time is valid
    uint32_t get_expiration() const {
        rassert(expiration_set());
        return metadata_values[0];
    }

    //Warning, the caller is responsible for unallocating the space thus freed
    bool void_expiration() {
        bool was_set = expiration_set();
        flags = (flags | EXPIRATION_FLAG_MASK) ^ EXPIRATION_FLAG_MASK;
        return was_set;
    }

    size_t get_metadata_size() const {
        return sizeof(redis_value_t) + sizeof(uint32_t)*expiration_set();
    }

    char *get_content() {
       return reinterpret_cast<char *>(this + get_metadata_size());
    }

    char *get_content() const {
       return const_cast<char *>(reinterpret_cast<const char *>(this + get_metadata_size()));
    }
} __attribute__((__packed__));

struct redis_string_value_t : redis_value_t {
    int size(const block_size_t &bs) const {
        blob_t blob(get_content(), blob::btree_maxreflen);
        return get_metadata_size() + blob.refsize(bs);
    }

    void clear(transaction_t *txn) {
        blob_t blob(get_content(), blob::btree_maxreflen);
        blob.clear(txn);
    }
} __attribute__((__packed__));

struct redis_hash_value_t : redis_value_t {
    int size() const {
        return get_metadata_size() + sizeof(block_id_t) + sizeof(uint32_t);
    }

    void clear(transaction_t *txn) {
        (void)txn;
        // TODO Tricky, involves clearing the entire sub-btree
    }

    block_id_t &get_root() {
        return *reinterpret_cast<block_id_t *>(get_content());
    }

    uint32_t &get_sub_size() {
        return *reinterpret_cast<uint32_t *>(get_content() + sizeof(block_id_t));
    }
} __attribute__((__packed__));

// The value support is identical at this point 
typedef redis_hash_value_t redis_set_value_t;

// Yes this is coppied from counted.hpp
struct sub_ref_t {
    uint32_t count; // the size of this sub tree
    block_id_t node_id; // The root block of the sub tree
} __attribute__((__packed__));


struct redis_list_value_t : redis_value_t {
    int size() const {
        return get_metadata_size() + sizeof(sub_ref_t);
    }

    sub_ref_t *get_ref() {
        return reinterpret_cast<sub_ref_t *>(get_content());
    }

    void clear(transaction_t *txn);
} __attribute__((__packed__));

struct redis_sorted_set_value_t : redis_value_t {
    // The sorted set value consists of two root blocks, one for each of the two indicies
    // They are, in order, the index by member name, and the index by score

    int size() const {
        return get_metadata_size() + 2 * sizeof(block_id_t);
    }

    uint32_t &get_sub_size() {
        return *reinterpret_cast<uint32_t *>(get_content());
    }

    block_id_t &get_member_index_root() {
        return *reinterpret_cast<block_id_t *>(get_content() + sizeof(uint32_t));
    }

    block_id_t &get_score_index_root() {
        return *reinterpret_cast<block_id_t *>(get_content() + sizeof(uint32_t) + sizeof(block_id_t));
    }

    void clear(transaction_t *txn);
} __attribute__((__packed__));

template <>
class value_sizer_t<redis_value_t> : public value_sizer_t<void> {
public:
    value_sizer_t(block_size_t bs) : block_size_(bs) { }
    ~value_sizer_t() {;}

    int size(const void *void_value) const {
        const redis_value_t *value = reinterpret_cast<const redis_value_t *>(void_value);
        switch(value->get_redis_type()) {
        case REDIS_STRING:
            return reinterpret_cast<const redis_string_value_t *>(this)->size(block_size_);
            break;
        case REDIS_LIST:
            return reinterpret_cast<const redis_list_value_t *>(this)->size();
            break;
        case REDIS_HASH:
            return reinterpret_cast<const redis_hash_value_t *>(this)->size();
            break;
        case REDIS_SET:
            return reinterpret_cast<const redis_set_value_t *>(this)->size();
            break;
        case REDIS_SORTED_SET:
            return reinterpret_cast<const redis_set_value_t *>(this)->size();
            break;
        default:
            unreachable();
            break;
        }
        
        rassert(NULL);    
        return 0;
    }

    bool fits(const void *void_value, int length_available) const {
        const redis_value_t *value = reinterpret_cast<const redis_value_t *>(void_value);
        int value_size = size(value);
        return value_size <= length_available;
    }

    bool deep_fsck(block_getter_t *getter, const void *value, int length_available, std::string *msg_out) const {
        (void)getter;
        (void)value;
        (void)length_available;
        (void)msg_out;

        //TODO real implementation
        return true;
    }

    int max_possible_size() const {
        return MAX_BTREE_VALUE_SIZE;
    }

    block_magic_t btree_leaf_magic() const {
        return value_sizer_t<redis_value_t>::leaf_magic();
    }

    static block_magic_t leaf_magic() {
        block_magic_t magic = { { 'r', 'd', 'i', 's' } };
        return magic;
    }

    block_size_t block_size() const { return block_size_; }

protected:
    block_size_t block_size_;
};

struct redis_nested_set_value_t {
    // This type is actually completely empty
};

template <>
class value_sizer_t<redis_nested_set_value_t> : public value_sizer_t<void> {
public:
    value_sizer_t(block_size_t bs) : block_size_(bs) { }
    ~value_sizer_t() {;}

    int size(const void *value) const {
        (void) value;
        return 0;
    }

    bool fits(const void *value, int length_available) const {
        (void) value;
        (void) length_available;
        return true;
    }

    int max_possible_size() const {
        return MAX_BTREE_VALUE_SIZE;
    }

    bool deep_fsck(block_getter_t *getter, const void *value, int length_available, std::string *msg_out) const {
        (void)getter;
        (void)value;
        (void)length_available;
        (void)msg_out;

        //TODO real implementation
        return true;
    }

    block_magic_t btree_leaf_magic() const {
        return value_sizer_t<redis_nested_set_value_t>::leaf_magic();
    }

    static block_magic_t leaf_magic() {
        block_magic_t magic = { { 'n', 's', 't', 's' } };
        return magic;
    }

    block_size_t block_size() const { return block_size_; }

protected:
    block_size_t block_size_;
};

struct redis_nested_string_value_t {
    char content[0];

    char *get_content() {
        return const_cast<char *>(content);
    }
};

template <>
class value_sizer_t<redis_nested_string_value_t> : public value_sizer_t<void> {
public:
    value_sizer_t(block_size_t bs) : block_size_(bs) { }
    ~value_sizer_t() {;}

    int size(const void *value) const {
        const redis_nested_string_value_t *actual_value = reinterpret_cast<const redis_nested_string_value_t *>(value); 
        blob_t blob(const_cast<char *>(actual_value->content), blob::btree_maxreflen);
        return blob.refsize(block_size_);
    }

    bool fits(const void *value, int length_available) const {
        const redis_nested_string_value_t *actual_value = reinterpret_cast<const redis_nested_string_value_t *>(value); 
        int value_size = size(actual_value);
        return value_size <= length_available;
    }


    bool deep_fsck(block_getter_t *getter, const void *value, int length_available, std::string *msg_out) const {
        (void)getter;
        (void)value;
        (void)length_available;
        (void)msg_out;

        //TODO real implementation
        return true;
    }

    int max_possible_size() const {
        return MAX_BTREE_VALUE_SIZE;
    }

    block_magic_t btree_leaf_magic() const {
        return value_sizer_t<redis_nested_string_value_t>::leaf_magic();
    }

    static block_magic_t leaf_magic() {
        block_magic_t magic = { { 'n', 'e', 's', 't' } };
        return magic;
    }


    block_size_t block_size() const { return block_size_; }

protected:
    block_size_t block_size_;
};

struct redis_nested_sorted_set_value_t {
    float score;
    char content[0];
};

template <>
class value_sizer_t<redis_nested_sorted_set_value_t> : public value_sizer_t<void> {
public:
    value_sizer_t(block_size_t bs) : block_size_(bs) { }
    ~value_sizer_t() {;}

    int size(const void *void_value) const {
        const redis_nested_sorted_set_value_t *value = reinterpret_cast<const redis_nested_sorted_set_value_t *>(void_value); 
        blob_t blob(const_cast<char *>(value->content), blob::btree_maxreflen);
        return sizeof(redis_nested_sorted_set_value_t) + blob.refsize(block_size_);
    }

    bool fits(const void *value, int length_available) const {
        int value_size = size(value);
        return value_size <= length_available;
    }

    bool deep_fsck(block_getter_t *getter, const void *value, int length_available, std::string *msg_out) const {
        (void)getter;
        (void)value;
        (void)length_available;
        (void)msg_out;

        //TODO real implementation
        return true;
    }

    int max_possible_size() const {
        return blob::btree_maxreflen + sizeof(redis_nested_sorted_set_value_t);
    }

    block_magic_t btree_leaf_magic() const {
        return value_sizer_t<redis_nested_sorted_set_value_t>::leaf_magic();
    }

    static block_magic_t leaf_magic() {
        block_magic_t magic = { { 'n', 's', 's', 'v' } };
        return magic;
    }

    block_size_t block_size() const { return block_size_; }

protected:
    block_size_t block_size_;
};


#endif /*__PROTOCOL_REDIS_TYPES_H__*/

