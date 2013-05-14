// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_QL2_HPP_
#define RDB_PROTOCOL_QL2_HPP_

#include "utils.hpp"

#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/stream_cache.hpp"
#include "rdb_protocol/term.hpp"

namespace ql {
// Runs a query!  This is all outside code should ever need to call.  See
// term.cc for definition.
void run(protob_t<Query> q, scoped_ptr_t<env_t> *env_ptr,
         Response *res, stream_cache2_t *stream_cache2,
         bool *response_needed_out);
} // namespace ql

#endif // RDB_PROTOCOL_QL2_HPP_
