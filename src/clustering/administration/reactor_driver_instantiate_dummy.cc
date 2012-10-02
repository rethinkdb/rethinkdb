#include "clustering/administration/reactor_driver.tcc"
#include "mock/dummy_protocol.hpp"
#include "serializer/serializer.hpp"
#include "serializer/translator.hpp"

template class reactor_driver_t<mock::dummy_protocol_t>;
