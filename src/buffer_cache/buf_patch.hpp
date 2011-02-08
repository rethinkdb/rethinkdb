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

    // This is used in buf_t
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

private:
    block_id_t block_id;
    patch_counter_t patch_counter;
    ser_transaction_id_t applies_to_transaction_id;
    patch_operation_code_t operation_code;
};


/* Binary patches */

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


/* Btree leaf node logical patches */

#include "store.hpp"

class leaf_shift_pairs_patch_t : public buf_patch_t {
public:
    leaf_shift_pairs_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const uint16_t offset, const uint16_t shift);
    leaf_shift_pairs_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const uint16_t data_length);

    virtual void apply_to_buf(char* buf_data);

    virtual size_t get_affected_data_size() const {
        return 16; // TODO!
    }

protected:
    virtual void serialize_data(char* destination) const;
    virtual uint16_t get_data_size() const;

private:
    uint16_t offset;
    uint16_t shift;
};

class leaf_insert_pair_patch_t : public buf_patch_t {
public:
    leaf_insert_pair_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const uint8_t value_size, const uint8_t value_metadata_flags, const byte *value_contents, const uint8_t key_size, const char *key_contents);
    leaf_insert_pair_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const uint16_t data_length);

    virtual ~leaf_insert_pair_patch_t();

    virtual void apply_to_buf(char* buf_data);

    virtual size_t get_affected_data_size() const;

protected:
    virtual void serialize_data(char* destination) const;
    virtual uint16_t get_data_size() const;

private:
    byte *value_buf;
    byte *key_buf;
};

class leaf_insert_patch_t : public buf_patch_t {
public:
    leaf_insert_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const block_size_t block_size, const uint8_t value_size, const uint8_t value_metadata_flags, const byte *value_contents, const uint8_t key_size, const char *key_contents, const repli_timestamp insertion_time);
    leaf_insert_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const uint16_t data_length);

    virtual ~leaf_insert_patch_t();

    virtual void apply_to_buf(char* buf_data);

    virtual size_t get_affected_data_size() const;

protected:
    virtual void serialize_data(char* destination) const;
    virtual uint16_t get_data_size() const;

private:
    block_size_t block_size;
    byte *value_buf;
    byte *key_buf;
    repli_timestamp insertion_time;
};

class leaf_remove_patch_t : public buf_patch_t {
public:
    leaf_remove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const block_size_t block_size, const uint8_t key_size, const char *key_contents);
    leaf_remove_patch_t(const block_id_t block_id, const patch_counter_t patch_counter, const char* data, const uint16_t data_length);

    virtual ~leaf_remove_patch_t();

    virtual void apply_to_buf(char* buf_data);

    virtual size_t get_affected_data_size() const;

protected:
    virtual void serialize_data(char* destination) const;
    virtual uint16_t get_data_size() const;

private:
    block_size_t block_size;
    byte *key_buf;
};


#endif	/* __BUF_PATCH_HPP__ */

