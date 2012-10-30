// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/reactor_driver.tcc"
#include "rdb_protocol/protocol.hpp"
#include "serializer/serializer.hpp"
#include "serializer/translator.hpp"

template class reactor_driver_t<rdb_protocol_t>;
