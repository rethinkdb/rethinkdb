#ifndef __BUF_PATCH_HPP__
#define	__BUF_PATCH_HPP__

class buf_patch_t;

#include "buffer_cache/types.hpp"
#include "serializer/types.hpp"

typedef uint32_t patch_counter_t;
typedef byte_t patch_operation_code_t;

class buf_patch_t {
public:
    virtual ~buf_patch_t() { }

    // Unserializes a patch an returns a buf_patch_t object
    // If *source is 0, it returns NULL
    static buf_patch_t* load_patch(char* source);

    // Serializes the patch to the given destination address
    void serialize(char* destination) const;
    uint16_t get_serialized_size() const;
    static uint16_t get_min_serialized_size();

    patch_counter_t get_patch_counter() const;
    ser_transaction_id_t get_transaction_id() const;
    void set_transaction_id(const ser_transaction_id_t transaction_id);
    block_id_t get_block_id() const;

    // This is used in buf_t
    virtual void apply_to_buf(char* buf_data) = 0;

    bool operator<(const buf_patch_t& p) const;
    
protected:    
    // These are for usage in subclasses
    buf_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const ser_transaction_id_t applies_to_transaction_id, const patch_operation_code_t operation_code);
    virtual void serialize_data(char* destination) const = 0;
    virtual uint16_t get_data_size() const = 0;

    static const patch_operation_code_t OPER_MEMCPY = 0;
    static const patch_operation_code_t OPER_MEMMOVE = 1;

private:
    block_id_t block_id;
    patch_counter_t patch_counter;
    ser_transaction_id_t applies_to_transaction_id;
    patch_operation_code_t operation_code;
};

class memcpy_patch_t : public buf_patch_t {
public:
    memcpy_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const ser_transaction_id_t applies_to_transaction_id, const uint16_t dest_offset, const char *src, const uint16_t n);
    memcpy_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const ser_transaction_id_t applies_to_transaction_id, const char* data, const uint16_t data_length);

    virtual ~memcpy_patch_t();

    virtual void apply_to_buf(char* buf_data);

protected:
    virtual void serialize_data(char* destination) const;
    virtual uint16_t get_data_size() const;

private:
    uint16_t dest_offset;
    uint16_t n;
    char* src_buf;
};

class memmove_patch_t : public buf_patch_t {
public:
    memmove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const ser_transaction_id_t applies_to_transaction_id, const uint16_t dest_offset, const uint16_t src_offset, const uint16_t n);
    memmove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const ser_transaction_id_t applies_to_transaction_id, const char* data, const uint16_t data_length);

    virtual void apply_to_buf(char* buf_data);

protected:
    virtual void serialize_data(char* destination) const;
    virtual uint16_t get_data_size() const;

private:
    uint16_t dest_offset;
    uint16_t src_offset;
    uint16_t n;
};

#endif	/* __BUF_PATCH_HPP__ */

