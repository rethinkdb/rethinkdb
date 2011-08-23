#ifndef __BUFFER_CACHE_CO_FUNCTIONS_HPP__
#define __BUFFER_CACHE_CO_FUNCTIONS_HPP__

#include <stddef.h>

#include "buffer_cache/types.hpp"

class repli_timestamp_t;

void co_get_subtree_recencies(transaction_t *transaction, block_id_t *block_ids, size_t num_block_ids, repli_timestamp_t *recencies_out);


#endif  // __BUFFER_CACHE_CO_FUNCTIONS_HPP__
