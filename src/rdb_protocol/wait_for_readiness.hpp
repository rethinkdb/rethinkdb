// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_WAIT_FOR_READINESS_HPP_
#define RDB_PROTOCOL_WAIT_FOR_READINESS_HPP_

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "concurrency/interruptor.hpp"

class base_namespace_repo_t;
class signal_t;
class uuid_u;

void wait_for_rdb_table_readiness(base_namespace_repo_t *ns_repo,
                                  uuid_u namespace_id,
                                  signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

#endif /* RDB_PROTOCOL_WAIT_FOR_READINESS_HPP_ */
