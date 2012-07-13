#ifndef BUFFER_CACHE_BUF_PATCH_HPP_
#define BUFFER_CACHE_BUF_PATCH_HPP_

#include <string>

/*
 * This file provides the basic buf_patch_t type as well as a few low-level binary
 * patch implementations (currently memmove and memcpy patches)
 */

class buf_patch_t;

#include "buffer_cache/types.hpp"
#include "containers/scoped.hpp"
#include "serializer/types.hpp"

typedef uint32_t patch_counter_t;
typedef int8_t patch_operation_code_t;

/*
 * As the buf_patch code is used in both extract and server, it should not crash
 * in case of a failed patch deserialization (i.e. loading a patch from disk).
 * Instead patches should emit a patch_deserialization_error_t exception.
 */
class patch_deserialization_error_t {
    // TODO: This isn't a std::exception?

    std::string message;
public:
    patch_deserialization_error_t(const char *file, int line, const char *msg);
    const char *c_str() const { return message.c_str(); }
};
#define guarantee_patch_format(cond, msg...) do {    \
        if (!(cond)) {                  \
            throw patch_deserialization_error_t(__FILE__, __LINE__, "" msg); \
        }                               \
    } while (0)


/*
 * A buf_patch_t is an in-memory representation for a patch. A patch describes
 * a specific change which can be applied to a buffer.
 * Each buffer patch has a patch counter as well as a block sequence id. The block sequence id
 * is used to determine to which version of a block the patch applies. Within
 * one version of a block, the patch counter explicitly encodes an ordering, which
 * is used to ensure that patches can be applied in the correct order
 * (even if they get serialized to disk in a different order).
 *
 * While buf_patch_t provides the general interface of a buffer patch and a few
 * universal methods, subclasses are required to implement an apply_to_buf method
 * (which executes the actual patch operation) as well as methods which handle
 * serializing and deserializing the subtype specific data.
 */
class buf_patch_t {
public:
    virtual ~buf_patch_t() { }

    // Unserializes a patch an returns a buf_patch_t object
    // If *(uint16_t*)source is 0, it returns NULL
    //
    // TODO: This allocates a patch, which you have to manually delete it.  Fix it.
    static buf_patch_t* load_patch(const char* source);

    // Serializes the patch to the given destination address
    void serialize(char* destination) const;

    inline uint16_t get_serialized_size() const {
        return sizeof(uint16_t) + sizeof(block_id_t) + sizeof(patch_counter_t) + sizeof(block_sequence_id_t) + sizeof(patch_operation_code_t) + get_data_size();
    }
    inline static uint16_t get_min_serialized_size() {
        return sizeof(uint16_t) + sizeof(block_id_t) + sizeof(patch_counter_t) + sizeof(block_sequence_id_t) + sizeof(patch_operation_code_t);
    }

    inline patch_counter_t get_patch_counter() const {
        return patch_counter;
    }
    inline block_sequence_id_t get_block_sequence_id() const {
        return applies_to_block_sequence_id;
    }
    inline void set_block_sequence_id(block_sequence_id_t block_sequence_id) {
        applies_to_block_sequence_id = block_sequence_id;
    }
    inline block_id_t get_block_id() const {
        return block_id;
    }

    // This is called from buf_lock_t
    virtual void apply_to_buf(char* buf_data, block_size_t block_size) = 0;

    bool applies_before(const buf_patch_t *p) const;

protected:
    virtual uint16_t get_data_size() const = 0;

    // These are for usage in subclasses
    buf_patch_t(block_id_t block_id, patch_counter_t patch_counter, patch_operation_code_t operation_code);
    virtual void serialize_data(char *destination) const = 0;

    static const patch_operation_code_t OPER_MEMCPY = 0;
    static const patch_operation_code_t OPER_MEMMOVE = 1;
    static const patch_operation_code_t OPER_LEAF_SHIFT_PAIRS = 2;
    static const patch_operation_code_t OPER_LEAF_INSERT_PAIR = 3;
    static const patch_operation_code_t OPER_LEAF_INSERT = 4;
    static const patch_operation_code_t OPER_LEAF_REMOVE = 5;
    static const patch_operation_code_t OPER_LEAF_ERASE_PRESENCE = 6;
    /* Assign an operation id to new subtypes here */
    /* Please note: you also have to "register" new operations in buf_patch_t::load_patch() */

private:
    block_id_t block_id;
    patch_counter_t patch_counter;
    block_sequence_id_t applies_to_block_sequence_id;
    patch_operation_code_t operation_code;

    DISABLE_COPYING(buf_patch_t);
};

struct dereferencing_buf_patch_compare_t {
    bool operator()(buf_patch_t *x, buf_patch_t *y) const {
        return x->applies_before(y);
    }
};

/* Binary patches */

/* memcpy_patch_t copies n bytes from src to the offset dest_offset of a buffer */
class memcpy_patch_t : public buf_patch_t {
public:
    memcpy_patch_t(block_id_t block_id, patch_counter_t patch_counter, uint16_t dest_offset, const char *src, uint16_t n);
    memcpy_patch_t(block_id_t block_id, patch_counter_t patch_counter, const char* data, uint16_t data_length);

    virtual ~memcpy_patch_t();

    virtual void apply_to_buf(char* buf_data, block_size_t bs);

protected:
    virtual void serialize_data(char* destination) const;
    virtual uint16_t get_data_size() const;

private:
    uint16_t dest_offset;
    scoped_array_t<char> src_buf;
};

/* memove_patch_t moves data from src_offset to dest_offset within a single buffer (with semantics equivalent to memmove()) */
class memmove_patch_t : public buf_patch_t {
public:
    memmove_patch_t(block_id_t block_id, patch_counter_t patch_counter, uint16_t dest_offset, uint16_t src_offset, uint16_t n);
    memmove_patch_t(block_id_t block_id, patch_counter_t patch_counter, const char* data, uint16_t data_length);

    virtual void apply_to_buf(char* buf_data, block_size_t bs);

    virtual uint16_t get_data_size() const;

protected:
    virtual void serialize_data(char* destination) const;

private:
    uint16_t dest_offset;
    uint16_t src_offset;
    uint16_t n;
};

#endif /* BUFFER_CACHE_BUF_PATCH_HPP_ */

