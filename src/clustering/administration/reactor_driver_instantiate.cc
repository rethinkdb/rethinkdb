#include "clustering/administration/reactor_driver.tcc"
#include "memcached/protocol.hpp"
#include "mock/dummy_protocol.hpp"

template class reactor_driver_t<memcached_protocol_t>;
template class reactor_driver_t<mock::dummy_protocol_t>;