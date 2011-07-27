#ifndef _SERIALIZER_SEMANTIC_CHECKING_INTERNAL_HPP_
#define _SERIALIZER_SEMANTIC_CHECKING_INTERNAL_HPP_

#include "serializer/semantic_checking.hpp"

struct scs_block_info_t {
    enum state_t {
        state_unknown,
        state_deleted,
        state_have_crc
    } state;
    uint32_t crc;

    // For compatibility with two_level_array_t
    scs_block_info_t() : state(state_unknown) {}
    scs_block_info_t(uint32_t crc) : state(state_have_crc), crc(crc) {}
    operator bool() { return state != state_unknown; }
};

struct scs_persisted_block_info_t {
    block_id_t block_id;
    scs_block_info_t block_info;
};

#endif
