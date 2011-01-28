#ifndef __BUF_PATCH_HPP__
#define	__BUF_PATCH_HPP__

class buf_patch_t;

#include "buffer_cache/types.hpp"

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
    size_t get_serialized_size() const;

    patch_counter_t get_patch_counter() const;

    // This is used in buf_t
    virtual void apply_to_buf(char* buf_data) = 0;
    
protected:    
    // These are for usage in subclasses
    buf_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const patch_operation_code_t operation_code);
    virtual void serialize_data(char* destination) const = 0;
    virtual size_t get_data_size() const = 0;

    static const patch_operation_code_t OPER_MEMCPY = 0;
    static const patch_operation_code_t OPER_MEMMOVE = 1;

private:
    block_id_t block_id;
    patch_counter_t patch_counter;
    patch_operation_code_t operation_code;
};

class memcpy_patch_t : public buf_patch_t {
public:
    memcpy_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const size_t dest_offset, const char *src, const size_t n);
    memcpy_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const size_t data_length);

    virtual ~memcpy_patch_t();

    virtual void apply_to_buf(char* buf_data);

protected:
    virtual void serialize_data(char* destination) const;
    virtual size_t get_data_size() const;

private:
    char* src_buf;
    size_t dest_offset;
    size_t n;
};

class memmove_patch_t : public buf_patch_t {
public:
    memmove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const size_t dest_offset, const size_t src_offset, const size_t n);
    memmove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const size_t data_length);

    virtual void apply_to_buf(char* buf_data);

protected:
    virtual void serialize_data(char* destination) const;
    virtual size_t get_data_size() const;

private:
    size_t src_offset;
    size_t dest_offset;
    size_t n;
};

#endif	/* __BUF_PATCH_HPP__ */

