#ifndef __BUF_PATCH_HPP__
#define	__BUF_PATCH_HPP__

/*
 * This file provides the basic buf_patch_t type as well as a few low-level binary
 * patch implementations (currently memmove and memcpy patches)
 */

class buf_patch_t;

#include "buffer_cache/types.hpp"
#include "serializer/types.hpp"

typedef uint32_t patch_counter_t;
typedef byte patch_operation_code_t;

/*
 * A buf_patch_t is an in-memory representation for a patch. A patch describes
 * a specific change which can be applied to a buffer.
 * Each buffer patch has a patch counter as well as a transaction id. The transaction id
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

    // Unserializes a patch and returns a buf_patch_t object
    // If *(uint16_t*)source is 0, it returns NULL
    static void load_patch(const char* source, buf_patch_t **patch_out);

    // Serializes the patch to the given destination address
    void serialize(char* destination) const;

    inline uint16_t get_serialized_size() const {
        return sizeof(uint16_t) + sizeof(block_id) + sizeof(patch_counter) + sizeof(applies_to_transaction_id) + sizeof(operation_code) + get_data_size();
    }
    inline static uint16_t get_min_serialized_size() {
        return sizeof(uint16_t) + sizeof(block_id) + sizeof(patch_counter) + sizeof(applies_to_transaction_id) + sizeof(operation_code);
    }

    inline patch_counter_t get_patch_counter() const {
        return patch_counter;
    }
    inline ser_transaction_id_t get_transaction_id() const {
        return applies_to_transaction_id;
    }
    inline void set_transaction_id(const ser_transaction_id_t transaction_id) {
        applies_to_transaction_id = transaction_id;
    }
    inline block_id_t get_block_id() const {
        return block_id;
    }

    virtual size_t get_affected_data_size() const = 0;

    // This is called from buf_t
    virtual void apply_to_buf(char* buf_data) = 0;

    bool operator<(const buf_patch_t& p) const;
    
protected:    
    // These are for usage in subclasses
    buf_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const patch_operation_code_t operation_code);
    virtual void serialize_data(char* destination) const = 0;
    virtual uint16_t get_data_size() const = 0;

    static const patch_operation_code_t OPER_MEMCPY = 0;
    static const patch_operation_code_t OPER_MEMMOVE = 1;
    static const patch_operation_code_t OPER_LEAF_SHIFT_PAIRS = 2;
    static const patch_operation_code_t OPER_LEAF_INSERT_PAIR = 3;
    static const patch_operation_code_t OPER_LEAF_INSERT = 4;
    static const patch_operation_code_t OPER_LEAF_REMOVE = 5;
    /* Assign an operation id to new subtypes here */
    /* Please note: you also have to "register" new operations in buf_patch_t::load_patch() */

private:
    block_id_t block_id;
    patch_counter_t patch_counter;
    ser_transaction_id_t applies_to_transaction_id;
    patch_operation_code_t operation_code;
};


/* Binary patches */

/* memcpy_patch_t copies n bytes from src to the offset dest_offset of a buffer */
class memcpy_patch_t : public buf_patch_t {
public:
    memcpy_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const uint16_t dest_offset, const char *src, const uint16_t n);
    memcpy_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const uint16_t data_length);

    virtual ~memcpy_patch_t();

    virtual void apply_to_buf(char* buf_data);

    virtual size_t get_affected_data_size() const;

protected:
    virtual void serialize_data(char* destination) const;
    virtual uint16_t get_data_size() const;

private:
    uint16_t dest_offset;
    uint16_t n;
    char* src_buf;
};

/* memove_patch_t moves data from src_offset to dest_offset within a single buffer (with semantics equivalent to memmove()) */
class memmove_patch_t : public buf_patch_t {
public:
    memmove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const uint16_t dest_offset, const uint16_t src_offset, const uint16_t n);
    memmove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const uint16_t data_length);

    virtual void apply_to_buf(char* buf_data);

    virtual size_t get_affected_data_size() const;

protected:
    virtual void serialize_data(char* destination) const;
    virtual uint16_t get_data_size() const;

private:
    uint16_t dest_offset;
    uint16_t src_offset;
    uint16_t n;
};

#endif	/* __BUF_PATCH_HPP__ */

