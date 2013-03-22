// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef SERIALIZER_TYPES_HPP_
#define SERIALIZER_TYPES_HPP_

#include <stdint.h>
#include <time.h>

#include <algorithm>
#include <string>

#include "containers/intrusive_ptr.hpp"
#include "containers/scoped.hpp"
#include "errors.hpp"

// A relatively "lightweight" header file (we wish), in a sense.

typedef uint32_t block_id_t;
#define NULL_BLOCK_ID (block_id_t(-1))

/* Each time we write a block to disk, that block receives a new unique block sequence id */
typedef uint64_t block_sequence_id_t;
#define NULL_BLOCK_SEQUENCE_ID  (block_sequence_id_t(0))
#define FIRST_BLOCK_SEQUENCE_ID (block_sequence_id_t(1))

// A struct that
struct ls_buf_data_t {
    block_id_t block_id;
    block_sequence_id_t block_sequence_id;
} __attribute__((__packed__));


class block_size_t {
public:
    // This is a bit ugly in that things could use the wrong method:
    // things could call value() instead of ser_value() or vice versa.

    // The "block size" used by things above the serializer.
    uint32_t value() const { return ser_bs_ - sizeof(ls_buf_data_t); }

    // The "block size" used by things in the serializer.
    uint32_t ser_value() const { return ser_bs_; }

    // Avoid using this function.  We want there to be a small
    // number of uses so that we can be sure it's impossible to pass
    // the wrong value as a block_size_t.
    static block_size_t unsafe_make(uint32_t ser_bs) {
        return block_size_t(ser_bs);
    }
private:
    explicit block_size_t(uint32_t ser_bs) : ser_bs_(ser_bs) { }
    uint32_t ser_bs_;
};

class repli_timestamp_t;

template <class serializer_type> struct serializer_traits_t;

class log_serializer_t;

class ls_block_token_pointee_t {
    friend class log_serializer_t;
    friend class dbm_read_ahead_fsm_t;  // For read-ahead tokens.

    friend void adjust_ref(ls_block_token_pointee_t *p, int adjustment);

    ls_block_token_pointee_t(log_serializer_t *serializer, int64_t initial_offset);

    log_serializer_t *serializer_;
    intptr_t ref_count_;

    void do_destroy();

    DISABLE_COPYING(ls_block_token_pointee_t);
};

void intrusive_ptr_add_ref(ls_block_token_pointee_t *p);
void intrusive_ptr_release(ls_block_token_pointee_t *p);

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
#ifdef SEMANTIC_SERIALIZER_CHECK
    virtual void open_semantic_checking_file(int *fd_out) = 0;
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

    // For compatibility with two_level_array_t. We initialize crc to 0 to avoid having
    // uninitialized memory lying around, which annoys valgrind when we try to write
    // persisted_block_info_ts to disk.
    scs_block_info_t() : state(state_unknown), crc(0) {}
    operator bool() { return state != state_unknown; }
};

template <class inner_serializer_t>
struct scs_block_token_t {
    scs_block_token_t(block_id_t _block_id, const scs_block_info_t& _info,
                      const intrusive_ptr_t<typename serializer_traits_t<inner_serializer_t>::block_token_type>& tok)
        : block_id(_block_id), info(_info), inner_token(tok), ref_count_(0) {
        rassert(inner_token, "scs_block_token wrapping null token");
    }

    block_id_t block_id;    // NULL_BLOCK_ID if not associated with a block id
    scs_block_info_t info;      // invariant: info.state != scs_block_info_t::state_deleted
    intrusive_ptr_t<typename serializer_traits_t<inner_serializer_t>::block_token_type> inner_token;

    template <class T>
    friend void intrusive_ptr_add_ref(scs_block_token_t<T> *p);
    template <class T>
    friend void intrusive_ptr_release(scs_block_token_t<T> *p);
private:
    intptr_t ref_count_;
};

template <class inner_serializer_t>
void intrusive_ptr_add_ref(scs_block_token_t<inner_serializer_t> *p) {
    UNUSED const intptr_t res = __sync_add_and_fetch(&p->ref_count_, 1);
    rassert(res > 0);
}

template <class inner_serializer_t>
void intrusive_ptr_release(scs_block_token_t<inner_serializer_t> *p) {
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
inline
intrusive_ptr_t< scs_block_token_t<log_serializer_t> >
to_standard_block_token(block_id_t block_id, const intrusive_ptr_t<ls_block_token_pointee_t>& tok) {
    intrusive_ptr_t< scs_block_token_t<log_serializer_t> > ret(new scs_block_token_t<log_serializer_t>(block_id, scs_block_info_t(), tok));
    return ret;
}

#else

typedef log_serializer_t standard_serializer_t;

// God this is such a hack (Part 2 of 2)
inline
intrusive_ptr_t<ls_block_token_pointee_t>
to_standard_block_token(UNUSED block_id_t block_id, const intrusive_ptr_t<ls_block_token_pointee_t>& tok) {
    return tok;
}

#endif

typedef serializer_traits_t<standard_serializer_t>::block_token_type standard_block_token_t;

class serializer_t;

template <>
struct serializer_traits_t<serializer_t> {
    typedef standard_block_token_t block_token_type;
};

// TODO: time_t's size is system-dependent.
typedef time_t creation_timestamp_t;


class serializer_data_ptr_t {
public:
    serializer_data_ptr_t() : ptr_(0) { }
    // TODO: Get rid of this constructor.
    explicit serializer_data_ptr_t(void *ptr) : ptr_(ptr) { }
    ~serializer_data_ptr_t() {
        rassert(!ptr_);
    }

    void free(serializer_t *ser);
    void init_malloc(serializer_t *ser);
    void init_clone(serializer_t *ser, const serializer_data_ptr_t& other);

    void swap(serializer_data_ptr_t& other) {
        void *tmp = ptr_;
        ptr_ = other.ptr_;
        other.ptr_ = tmp;
    }

    bool has() const {
        return ptr_;
    }

    void *get() const {
        rassert(ptr_);
        return ptr_;
    }

    // TODO: All uses of this function are disgusting.
    bool equals(const void *buf) const {
        return ptr_ == buf;
    }

private:
    void *ptr_;
    DISABLE_COPYING(serializer_data_ptr_t);
};


class serializer_read_ahead_callback_t {
public:
    virtual ~serializer_read_ahead_callback_t() { }
    /* If the callee returns true, it is responsible to free buf by calling free(buf) in the corresponding serializer. */
    virtual bool offer_read_ahead_buf(block_id_t block_id, void *buf, const intrusive_ptr_t<standard_block_token_t>& token, repli_timestamp_t recency_timestamp) = 0;
};

#endif  // SERIALIZER_TYPES_HPP_
