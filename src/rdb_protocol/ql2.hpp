// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_QL2_HPP_
#define RDB_PROTOCOL_QL2_HPP_

#include "utils.hpp"

#include "rdb_protocol/env.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/stream_cache.hpp"

namespace ql {
// Runs a query!  This is all outside code should ever need to call.  See
// term.cc for definition.
void run(Query2 *q, env_t *env, Response2 *res, stream_cache_t *stream_cache);
} // namespace ql

#endif // RDB_PROTOCOL_QL2_HPP_
