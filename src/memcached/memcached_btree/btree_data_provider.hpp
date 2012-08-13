#ifndef MEMCACHED_BTREE_BTREE_DATA_PROVIDER_HPP_
#define MEMCACHED_BTREE_BTREE_DATA_PROVIDER_HPP_

#include "buffer_cache/types.hpp"
#include "containers/intrusive_ptr.hpp"

struct memcached_value_t;
struct data_buffer_t;

intrusive_ptr_t<data_buffer_t> value_to_data_buffer(const memcached_value_t *value, transaction_t *transaction);

#endif // MEMCACHED_BTREE_BTREE_DATA_PROVIDER_HPP_
