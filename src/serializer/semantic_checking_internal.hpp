// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef SERIALIZER_SEMANTIC_CHECKING_INTERNAL_HPP_
#define SERIALIZER_SEMANTIC_CHECKING_INTERNAL_HPP_

#include "serializer/semantic_checking.hpp"

struct scs_persisted_block_info_t {
    block_id_t block_id;
    scs_block_info_t block_info;
};

#endif /* SERIALIZER_SEMANTIC_CHECKING_INTERNAL_HPP_ */
