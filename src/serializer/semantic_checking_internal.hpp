#ifndef _SERIALIZER_SEMANTIC_CHECKING_INTERNAL_HPP_
#define _SERIALIZER_SEMANTIC_CHECKING_INTERNAL_HPP_

#include "serializer/semantic_checking.hpp"

struct scs_persisted_block_info_t {
    block_id_t block_id;
    scs_block_info_t block_info;
};

#endif
