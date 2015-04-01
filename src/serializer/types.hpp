// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef SERIALIZER_TYPES_HPP_
#define SERIALIZER_TYPES_HPP_

#include <inttypes.h>
#include <time.h>

#include <string>
#include <utility>

#include "containers/counted.hpp"
#include "containers/scoped.hpp"
#include "errors.hpp"
#include "valgrind.hpp"

// A relatively "lightweight" header file (we wish), in a sense.
class buf_ptr_t;

class block_token_t;

class printf_buffer_t;

typedef uint64_t block_id_t;
#define NULL_BLOCK_ID (block_id_t(-1))

#define PR_BLOCK_ID PRIu64

// The first bytes of any block stored on disk or (as it happens) cached in memory.
struct ls_buf_data_t {
    block_id_t block_id;
} __attribute__((__packed__));

// For use via scoped_malloc_t, a buffer that represents a block on disk.  Contains
// convenient access to the serializer header and cache portion of the block.  This
// is better than (e.g.) performing arithmetic on void pointers when passing bufs
// between the cache and serializer.  When used in memory, this structure _might_ be
// aligned to a device block size boundary, to save copying when reading from disk.
// Since block sizes can vary, don't generally assume this to be the case.
struct ser_buffer_t {
    ls_buf_data_t ser_header;
    char cache_data[];
} __attribute__((__packed__));


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

class log_serializer_t;
class file_t;
class semantic_checking_file_t;

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
#ifdef SEMANTIC_SERIALIZER_CHECK
    virtual void open_semantic_checking_file(scoped_ptr_t<semantic_checking_file_t> *file_out) = 0;
#endif
};

#ifdef SEMANTIC_SERIALIZER_CHECK

template <class T>
class semantic_checking_serializer_t;

typedef semantic_checking_serializer_t<log_serializer_t> standard_serializer_t;

struct scs_block_info_t {
    enum state_t {
        state_unknown,
        state_deleted,
        state_have_crc
    } state;
    uint32_t crc;

    explicit scs_block_info_t(uint32_t _crc) : state(state_have_crc), crc(_crc) {}

    // For compatibility with two_level_array_t. We initialize crc to 0 to avoid
    // having uninitialized memory lying around, which annoys valgrind when we try to
    // write persisted_block_info_ts to disk.
    scs_block_info_t() : state(state_unknown), crc(0) {}
    operator bool() { return state != state_unknown; }
};

template <class inner_serializer_t>
struct scs_block_token_t {
    scs_block_token_t(block_id_t _block_id, const scs_block_info_t &_info,
                      counted_t<typename serializer_traits_t<inner_serializer_t>::block_token_type> tok)
        : block_id(_block_id), info(_info), inner_token(std::move(tok)), ref_count_(0) {
        rassert(inner_token.has(), "scs_block_token wrapping null token");
    }

    block_size_t block_size() const {
        return inner_token->block_size();
    }

    block_id_t block_id;    // NULL_BLOCK_ID if not associated with a block id
    scs_block_info_t info;      // invariant: info.state != scs_block_info_t::state_deleted
    counted_t<typename serializer_traits_t<inner_serializer_t>::block_token_type> inner_token;

    template <class T>
    friend void counted_add_ref(scs_block_token_t<T> *p);
    template <class T>
    friend void counted_release(scs_block_token_t<T> *p);
private:
    intptr_t ref_count_;
};

template <class inner_serializer_t>
void counted_add_ref(scs_block_token_t<inner_serializer_t> *p) {
    DEBUG_VAR const intptr_t res = __sync_add_and_fetch(&p->ref_count_, 1);
    rassert(res > 0);
}

template <class inner_serializer_t>
void counted_release(scs_block_token_t<inner_serializer_t> *p) {
    const intptr_t res = __sync_sub_and_fetch(&p->ref_count_, 1);
    rassert(res >= 0);
    if (res == 0) {
        delete p;
    }
}


template <class inner_serializer_type>
struct serializer_traits_t<semantic_checking_serializer_t<inner_serializer_type> > {
    typedef scs_block_token_t<inner_serializer_type> block_token_type;
};

// God this is such a hack (Part 1 of 2)
inline counted_t< scs_block_token_t<log_serializer_t> >
to_standard_block_token(block_id_t block_id,
                        counted_t<ls_block_token_pointee_t> tok) {
    return make_counted<scs_block_token_t<log_serializer_t> >(block_id,
                                                              scs_block_info_t(),
                                                              std::move(tok));
}

template <class inner_serializer_t>
void debug_print(printf_buffer_t *buf,
                 const counted_t<scs_block_token_t<inner_serializer_t> > &token) {
    debug_print(buf, token->inner_token);
}




#else  // SEMANTIC_SERIALIZER_CHECK

typedef log_serializer_t standard_serializer_t;
class ls_block_token_pointee_t;

// God this is such a hack (Part 2 of 2)
// TODO: We can probably remove this
// Defined in log_serializer.cc
counted_t<block_token_t>
to_standard_block_token(block_id_t block_id,
                        counted_t<ls_block_token_pointee_t> tok);

#endif

class serializer_t;

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
                                      const counted_t<block_token_t> &token) = 0;
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
