// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_WAIT_FOR_READINESS_HPP_
#define RDB_PROTOCOL_WAIT_FOR_READINESS_HPP_

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "concurrency/interruptor.hpp"

class namespace_interface_t;
class signal_t;

/* `test_for_rdb_table_readiness()` returns `true` if the given namespace is ready for
reading, and `false` otherwise. */
MUST_USE bool test_for_rdb_table_readiness(namespace_interface_t *namespace_if,
                                           signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

#endif /* RDB_PROTOCOL_WAIT_FOR_READINESS_HPP_ */
