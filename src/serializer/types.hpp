#ifndef __SERIALIZER_TYPES_HPP__
#define __SERIALIZER_TYPES_HPP__

#include <stdint.h>
#include "utils.hpp"

// A relatively "lightweight" header file (we wish), in a sense.

typedef uint64_t ser_transaction_id_t;
#define NULL_SER_TRANSACTION_ID (ser_transaction_id_t(0))
#define FIRST_SER_TRANSACTION_ID (ser_transaction_id_t(1))

typedef uint32_t block_id_t;
#define NULL_BLOCK_ID (block_id_t(-1))

/* TODO: buf_data_t and block_size_t depend on the serializer implementation details, so they don't
belong in this file. */

//  Data to be serialized to disk with each block.  Changing this changes the disk format!
struct buf_data_t {
    block_id_t block_id;
    ser_transaction_id_t transaction_id;
} __attribute__((__packed__));


// TODO: Hopefully it's not serialized using more than 16 bits.
//
//  block_size_t is serialized as part of some patches.  Changing this changes the disk format!
class block_size_t {
public:
    // This is a bit ugly in that things could use the wrong method:
    // things could call value() instead of ser_value() or vice versa.

    // The "block size" used by things above the serializer.
    uint64_t value() const { return ser_bs_ - sizeof(buf_data_t); }

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
