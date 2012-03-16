#ifndef BTREE_BTREE_DATA_PROVIDER_HPP_
#define BTREE_BTREE_DATA_PROVIDER_HPP_

#include "errors.hpp"
#include <boost/intrusive_ptr.hpp>

#include "buffer_cache/types.hpp"

struct memcached_value_t;
struct data_buffer_t;

boost::intrusive_ptr<data_buffer_t> value_to_data_buffer(const memcached_value_t *value, transaction_t *transaction);




#endif // BTREE_BTREE_DATA_PROVIDER_HPP_
