#include "clustering/administration/reactor_driver.tcc"
#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "mock/dummy_protocol.hpp"

template class reactor_driver_t<mock::dummy_protocol_t>;
