#ifndef __SERIALIZER_TYPES_HPP__
#define __SERIALIZER_TYPES_HPP__

#include <stdint.h>
#include <time.h>

// Sigh.
#include "errors.hpp"
#include <boost/shared_ptr.hpp>

// A relatively "lightweight" header file (we wish), in a sense.

typedef uint32_t block_id_t;
#define NULL_BLOCK_ID (block_id_t(-1))

/* Each time we write a block to disk, that block receives a new unique block sequence id */
typedef uint64_t block_sequence_id_t;
#define NULL_BLOCK_SEQUENCE_ID  (block_sequence_id_t(0))
#define FIRST_BLOCK_SEQUENCE_ID (block_sequence_id_t(1))

/* TODO: block_size_t depends on the serializer implementation details, so it doesn't
belong in this file. */

//  block_size_t is serialized as part of some patches.  Changing this changes the disk format!
class block_size_t {
public:
    // This is a bit ugly in that things could use the wrong method:
    // things could call value() instead of ser_value() or vice versa.

    // The "block size" used by things above the serializer.
    // TODO: As a hack, the implementation of this is currently in log_serializer.cc
    //  as ut depends on ls_buf_data_t.
    //  In the long-term, we will want to refactor block_size_t.
    uint64_t value() const;

    // The "block size" used by things in the serializer.
    uint64_t ser_value() const { return ser_bs_; }

    // Avoid using this function.  We want there to be a small
    // number of uses so that we can be sure it's impossible to pass
    // the wrong value as a block_size_t.
    static block_size_t unsafe_make(uint64_t ser_bs) {
        return block_size_t(ser_bs);
    }
private:
    explicit block_size_t(uint64_t ser_bs) : ser_bs_(ser_bs) { }
    uint64_t ser_bs_;
};

class serializer_block_token_t {
public:
    virtual ~serializer_block_token_t() { }
};

class repli_timestamp_t;

class serializer_read_ahead_callback_t {
public:
    virtual ~serializer_read_ahead_callback_t() { }
    /* If the callee returns true, it is responsible to free buf by calling free(buf) in the corresponding serializer. */
    virtual bool offer_read_ahead_buf(block_id_t block_id, void *buf, repli_timestamp_t recency_timestamp) = 0;
};

template <class serializer_type> struct serializer_traits_t;

class log_serializer_t;

class ls_block_token_t : public serializer_block_token_t {
    friend class log_serializer_t;

    ls_block_token_t(log_serializer_t *serializer, off64_t initial_offset);
    log_serializer_t *serializer;

public:
    virtual ~ls_block_token_t();
};


template <>
struct serializer_traits_t<log_serializer_t> {
    typedef ls_block_token_t block_token_type;
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

    scs_block_info_t(uint32_t crc) : state(state_have_crc), crc(crc) {}

    // TODO (sam): We write semantic serializer check information to disk?

    // For compatibility with two_level_array_t. We initialize crc to 0 to avoid having
    // uninitialized memory lying around, which annoys valgrind when we try to write
    // persisted_block_info_ts to disk.
    scs_block_info_t() : state(state_unknown), crc(0) {}
    operator bool() { return state != state_unknown; }
};

template <class inner_serializer_t>
struct scs_block_token_t : public serializer_block_token_t {
    scs_block_token_t(semantic_checking_serializer_t<inner_serializer_t> *ser, block_id_t block_id, const scs_block_info_t &info,
                      boost::shared_ptr<serializer_block_token_t> tok)
        : serializer(ser), block_id(block_id), info(info), inner_token(tok) {
        rassert(inner_token, "scs_block_token wrapping null token");
    }
    semantic_checking_serializer_t<inner_serializer_t> *serializer;
    block_id_t block_id;    // NULL_BLOCK_ID if not associated with a block id
    scs_block_info_t info;      // invariant: info.state != scs_block_info_t::state_deleted
    boost::shared_ptr<serializer_block_token_t> inner_token;
};


template <>
template <class inner_serializer_type>
struct serializer_traits_t<semantic_checking_serializer_t<inner_serializer_type> > {
    typedef scs_block_token_t<inner_serializer_type> block_token_type;
};



#else

typedef log_serializer_t standard_serializer_t;

#endif

// TODO: time_t is disgusting.
typedef time_t creation_timestamp_t;

#endif  // __SERIALIZER_TYPES_HPP__
