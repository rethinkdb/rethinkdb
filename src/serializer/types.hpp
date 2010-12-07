#ifndef __SERIALIZER_TYPES_HPP__
#define __SERIALIZER_TYPES_HPP__

#include <stdint.h>
#include "utils.hpp"

// A relatively "lightweight" header file (we wish), in a sense.

typedef uint64_t ser_transaction_id_t;
#define NULL_SER_TRANSACTION_ID (ser_transaction_id_t(0))
#define FIRST_SER_TRANSACTION_ID (ser_transaction_id_t(1))

struct ser_block_id_t {
    typedef uint32_t number_t;

    // Distrust things that access value directly.
    number_t value;

    inline bool operator==(ser_block_id_t other) const { return value == other.value; }
    inline bool operator!=(ser_block_id_t other) const { return value != other.value; }
    inline bool operator<(ser_block_id_t other) const { return value < other.value; }

    static inline ser_block_id_t make(number_t num) {
        assert(num != number_t(-1));
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

struct config_block_id_t {
    ser_block_id_t ser_id;

    ser_block_id_t subsequent_ser_id() const { return ser_block_id_t::make(ser_id.value + 1); }
    static inline config_block_id_t make(ser_block_id_t::number_t num) {
        assert(num == 0);  // only one possible config_block_id_t value.

        config_block_id_t ret;
        ret.ser_id = ser_block_id_t::make(num);
        return ret;
    }
};




//  Data to be serialized to disk with each block.  Changing this changes the disk format!
struct buf_data_t {
    ser_block_id_t block_id;
    ser_transaction_id_t transaction_id;
} __attribute__((__packed__));



#endif  // __SERIALIZER_TYPES_HPP__
