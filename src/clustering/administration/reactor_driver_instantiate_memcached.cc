#include "clustering/administration/reactor_driver.tcc"
#include "memcached/protocol.hpp"
#include "serializer/serializer.hpp"
#include "serializer/translator.hpp"

template class reactor_driver_t<memcached_protocol_t>;
