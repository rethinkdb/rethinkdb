#ifndef __PROTOCOL_REDIS_TYPES_H__
#define __PROTOCOL_REDIS_TYPES_H__

#include "serializer/types.hpp"
#include "buffer_cache/mirrored/mirrored.hpp"
#include "buffer_cache/blob.hpp"
#include "btree/node.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/variant.hpp"
#include <vector>
#include <inttypes.h>

//Redis result types. All redis commands return one of these types. Any redis parser
//should know how to serialize these
enum redis_status {
    OK,
    ERROR
};

struct status_result_struct {
    redis_status status;
    const char *msg;
};

typedef const boost::shared_ptr<status_result_struct> status_result;
typedef const boost::variant<unsigned, status_result> integer_result;
typedef const boost::variant<boost::shared_ptr<std::string>, status_result> bulk_result;
typedef const boost::variant<boost::shared_ptr<std::vector<std::string> >, status_result> multi_bulk_result;

//typedef const unsigned integer_result;
//typedef const boost::shared_ptr<std::string> bulk_result;
//typedef const boost::shared_ptr<std::vector<std::string> > multi_bulk_result;
//typedef const boost::variant<status_result, integer_result, bulk_result, multi_bulk_result> redis_result;

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
        assert(expiration_set());
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

    void clear() {
        // TODO Tricky, involves clearing the entire sub-btree
    }

    block_id_t &get_root() {
        return *reinterpret_cast<block_id_t *>(get_content());
    }

    uint32_t &get_sub_size() {
        return *reinterpret_cast<block_id_t *>(get_content() + sizeof(block_id_t));
    }
} __attribute__((__packed__));

// The value support is identical at this point 
typedef redis_hash_value_t redis_set_value_t;

template <>
class value_sizer_t<redis_value_t> {
public:
    value_sizer_t(block_size_t bs) : block_size_(bs) { }

    int size(const redis_value_t *value) const {
        switch(value->get_redis_type()) {
        case REDIS_STRING:
            return reinterpret_cast<const redis_string_value_t *>(this)->size(block_size_);
            break;
        case REDIS_LIST:
            break;
        case REDIS_HASH:
            return reinterpret_cast<const redis_hash_value_t *>(this)->size();
            break;
        case REDIS_SET:
            return reinterpret_cast<const redis_set_value_t *>(this)->size();
            break;
        case REDIS_SORTED_SET:
            break;
        }
        
        assert(NULL);    
        return 0;
    }

    bool fits(const redis_value_t *value, int length_available) const {
        int value_size = size(value);
        return value_size <= length_available;
    }

    int max_possible_size() const {
        return MAX_BTREE_VALUE_SIZE;
    }

    static block_magic_t btree_leaf_magic() {
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
class value_sizer_t<redis_nested_set_value_t> {
public:
    value_sizer_t(block_size_t bs) : block_size_(bs) { }

    int size(const redis_nested_set_value_t *value) const {
        (void) value;
        return 0;
    }

    bool fits(const redis_value_t *value, int length_available) const {
        (void) value;
        (void) length_available;
        return true;
    }

    int max_possible_size() const {
        return MAX_BTREE_VALUE_SIZE;
    }

    static block_magic_t btree_leaf_magic() {
        block_magic_t magic = { { 'n', 's', 't', 's' } };
        return magic;
    }

    block_size_t block_size() const { return block_size_; }

protected:
    block_size_t block_size_;
};

struct redis_nested_string_value_t {
    char content[0];
};

template <>
class value_sizer_t<redis_nested_string_value_t> {
public:
    value_sizer_t(block_size_t bs) : block_size_(bs) { }

    int size(const redis_nested_string_value_t *value) const {
        blob_t blob(const_cast<char *>(value->content), blob::btree_maxreflen);
        return blob.refsize(block_size_);
    }

    bool fits(const redis_nested_string_value_t *value, int length_available) const {
        int value_size = size(value);
        return value_size <= length_available;
    }

    int max_possible_size() const {
        return MAX_BTREE_VALUE_SIZE;
    }

    static block_magic_t btree_leaf_magic() {
        block_magic_t magic = { { 'n', 'e', 's', 't' } };
        return magic;
    }

    block_size_t block_size() const { return block_size_; }

protected:
    block_size_t block_size_;
};

#endif /*__PROTOCOL_REDIS_TYPES_H__*/

