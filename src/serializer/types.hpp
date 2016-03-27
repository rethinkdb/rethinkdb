// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef SERIALIZER_TYPES_HPP_
#define SERIALIZER_TYPES_HPP_

#include <inttypes.h>
#include <time.h>

#include <string>
#include <utility>

#include "arch/compiler.hpp"
#include "containers/counted.hpp"
#include "containers/scoped.hpp"
#include "errors.hpp"
#include "valgrind.hpp"

// A relatively "lightweight" header file (we wish), in a sense.
class buf_ptr_t;

class printf_buffer_t;

typedef uint64_t block_id_t;
// The block ID space is divided into two sub-ranges:
//  1. Regular block IDs starting at 0 up to FIRST_AUX_BLOCK_ID-1
//  2. Aux block IDs starting at FIRST_AUX_BLOCK_ID up to NULL_BLOCK_ID-1
#define FIRST_AUX_BLOCK_ID ((uint64_t)1 << 63)
#define NULL_BLOCK_ID (block_id_t(-1))
inline bool is_aux_block_id(const block_id_t id) {
    return id >= FIRST_AUX_BLOCK_ID && id != NULL_BLOCK_ID;
}
// Maps from the aux block ID space into a block ID space that starts at 0.
// This is useful if you want to use an aux block ID to index into an array for
// example.
inline block_id_t make_aux_block_id_relative(const block_id_t id) {
    rassert(is_aux_block_id(id));
    return id - FIRST_AUX_BLOCK_ID;
}

#define PR_BLOCK_ID PRIu64

// The first bytes of any block stored on disk or (as it happens) cached in memory.
ATTR_PACKED(struct ls_buf_data_t {
    block_id_t block_id;
});

// For use via scoped_malloc_t, a buffer that represents a block on disk.  Contains
// convenient access to the serializer header and cache portion of the block.  This
// is better than (e.g.) performing arithmetic on void pointers when passing bufs
// between the cache and serializer.  When used in memory, this structure _might_ be
// aligned to a device block size boundary, to save copying when reading from disk.
// Since block sizes can vary, don't generally assume this to be the case.
ATTR_PACKED(struct ser_buffer_t {
    ls_buf_data_t ser_header;
    char cache_data[];
});


class block_size_t {
public:
    // This is a bit ugly in that things could use the wrong method:
    // things could call value() instead of ser_value() or vice versa.

    // The "block size" (in bytes) used by things above the serializer.
    uint32_t value() const {
        rassert(ser_bs_ != 0);
        return ser_bs_ - sizeof(ls_buf_data_t);
    }

    // The "block size" (in bytes) used by things in the serializer.
    uint32_t ser_value() const {
        rassert(ser_bs_ != 0);
        return ser_bs_;
    }

    static block_size_t make_from_cache(uint32_t cache_block_size) {
        return block_size_t(cache_block_size + sizeof(ls_buf_data_t));
    }

    // Avoid using this function.  We want there to be a small
    // number of uses so that we can be sure it's impossible to pass
    // the wrong value as a block_size_t.
    static block_size_t unsafe_make(uint32_t ser_bs) {
        return block_size_t(ser_bs);
    }

    // Returns an undefined value that you may not use.
    static block_size_t undefined() {
        return valgrind_undefined<block_size_t>(unsafe_make(0));
    }

protected:
    explicit block_size_t(uint32_t ser_bs) : ser_bs_(ser_bs) { }

private:
    uint32_t ser_bs_;
};

// For use in compile-time constants
template<uint32_t ser_size> struct from_ser_block_size_t {
    static const uint32_t cache_size = ser_size - sizeof(ls_buf_data_t);
};
template<uint32_t cache_size> struct from_cache_block_size_t {
    static const uint32_t ser_size = cache_size + sizeof(ls_buf_data_t);
};

inline bool operator==(block_size_t x, block_size_t y) {
    return x.ser_value() == y.ser_value();
}

inline bool operator!=(block_size_t x, block_size_t y) {
    return !(x == y);
}

class max_block_size_t : public block_size_t {
public:
    using block_size_t::value;
    using block_size_t::ser_value;

    static max_block_size_t unsafe_make(uint32_t ser_bs) {
        CT_ASSERT(sizeof(block_size_t) == sizeof(max_block_size_t));
        return max_block_size_t(ser_bs);
    }

private:
    explicit max_block_size_t(uint32_t ser_bs)
        : block_size_t(ser_bs) { }
};

class repli_timestamp_t;

template <class serializer_type> struct serializer_traits_t;

class log_serializer_t;

class ls_block_token_pointee_t {
public:
    int64_t offset() const { return offset_; }
    block_size_t block_size() const { return block_size_; }

private:
    friend class log_serializer_t;
    friend class dbm_read_ahead_fsm_t;  // For read-ahead tokens.

    friend void counted_add_ref(ls_block_token_pointee_t *p);
    friend void counted_release(ls_block_token_pointee_t *p);

    ls_block_token_pointee_t(log_serializer_t *serializer,
                             int64_t initial_offset,
                             block_size_t initial_ser_block_size);

    log_serializer_t *serializer_;
    std::atomic<intptr_t> ref_count_;

    // The block's size.
    block_size_t block_size_;

    // The block's offset on disk.
    int64_t offset_;

    void do_destroy();

    DISABLE_COPYING(ls_block_token_pointee_t);
};

void debug_print(printf_buffer_t *buf,
                 const counted_t<ls_block_token_pointee_t> &token);

void counted_add_ref(ls_block_token_pointee_t *p);
void counted_release(ls_block_token_pointee_t *p);

template <>
struct serializer_traits_t<log_serializer_t> {
    typedef ls_block_token_pointee_t block_token_type;
};

class file_t;

class serializer_file_opener_t {
public:
    virtual ~serializer_file_opener_t() { }

    // A name to describe the file for use in error messages.  For
    // real files, this should be the filepath.
    virtual std::string file_name() const = 0;

    virtual void open_serializer_file_create_temporary(scoped_ptr_t<file_t> *file_out) = 0;
    virtual void move_serializer_file_to_permanent_location() = 0;
    virtual void open_serializer_file_existing(scoped_ptr_t<file_t> *file_out) = 0;
    virtual void unlink_serializer_file() = 0;
};

// TODO: This is a hack remaining from when we had the semantic checking serializer
// and had to mask the implementation of a block token. We should check if we
// can remove this indirection now.
inline
counted_t<ls_block_token_pointee_t>
to_standard_block_token(UNUSED block_id_t block_id,
                        counted_t<ls_block_token_pointee_t> tok) {
    return tok;
}

typedef serializer_traits_t<log_serializer_t>::block_token_type standard_block_token_t;

class serializer_t;

template <>
struct serializer_traits_t<serializer_t> {
    typedef standard_block_token_t block_token_type;
};

// TODO: This is obsolete because the serializer multiplexer isn't used with multiple
// files any more.
typedef int64_t creation_timestamp_t;


class serializer_read_ahead_callback_t {
public:
    virtual ~serializer_read_ahead_callback_t() { }

    // Offers a buf_ptr_t to the callback.  The callee is free to take
    // ownership of the `buf_ptr_t` from `*buf`.  It's also free to decline
    // ownership, by leaving `*buf` untouched.
    virtual void offer_read_ahead_buf(block_id_t block_id,
                                      buf_ptr_t *buf,
                                      const counted_t<standard_block_token_t> &token) = 0;
};

struct buf_write_info_t {
    buf_write_info_t(ser_buffer_t *_buf, block_size_t _block_size,
                     block_id_t _block_id)
        : buf(_buf), block_size(_block_size), block_id(_block_id) { }
    ser_buffer_t *buf;
    block_size_t block_size;
    block_id_t block_id;
};

void debug_print(printf_buffer_t *buf, const buf_write_info_t &info);

#endif  // SERIALIZER_TYPES_HPP_
