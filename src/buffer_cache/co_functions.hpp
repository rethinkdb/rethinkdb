#ifndef __BUFFER_CACHE_CO_FUNCTIONS_HPP__
#define __BUFFER_CACHE_CO_FUNCTIONS_HPP__

#include <stddef.h>

#include "buffer_cache/types.hpp"
#include "concurrency/access.hpp"

class cond_t;
class repli_timestamp_t;

// TODO: `co_acquire_block()` is redundant; get rid of it.
buf_t *co_acquire_block(transaction_t *transaction, block_id_t block_id, access_t mode, cond_t *acquisition_cond = 0);

void co_get_subtree_recencies(transaction_t *transaction, block_id_t *block_ids, size_t num_block_ids, repli_timestamp_t *recencies_out);


#endif  // __BUFFER_CACHE_CO_FUNCTIONS_HPP__
