#include "clustering/administration/reactor_driver.tcc"
#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "memcached/protocol.hpp"

template class reactor_driver_t<memcached_protocol_t>;
