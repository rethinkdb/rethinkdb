#ifndef __SERIALIZER_TYPES_HPP__
#define __SERIALIZER_TYPES_HPP__

#include <stdint.h>
#include "utils.hpp"

// A relatively "lightweight" header file (we wish), in a sense.

typedef uint64_t ser_transaction_id_t;
#define NULL_SER_TRANSACTION_ID (ser_transaction_id_t(0))
#define FIRST_SER_TRANSACTION_ID (ser_transaction_id_t(1))

/* Each time we write a block to disk, that block recieves a new unique block sequence id */
typedef __uint128_t ser_block_sequence_id_t;
#define NULL_SER_BLOCK_SEQUENCE_ID (ser_block_sequence_id_t(0))
#define FIRST_SER_BLOCK_SEQUENCE_ID (ser_block_sequence_id_t(1))

struct ser_block_id_t {
    typedef uint32_t number_t;

    // Distrust things that access value directly.
    number_t value;

    inline bool operator==(ser_block_id_t other) const { return value == other.value; }
    inline bool operator!=(ser_block_id_t other) const { return value != other.value; }
    inline bool operator<(ser_block_id_t other) const { return value < other.value; }

    static inline ser_block_id_t make(number_t num) {
        rassert(num != number_t(-1));
        ser_block_id_t ret;
        ret.value = num;
        return ret;
    }

    static inline ser_block_id_t null() {
        ser_block_id_t ret;
        ret.value = uint32_t(-1);
        return ret;
    }
};

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


#endif  // __SERIALIZER_TYPES_HPP__
