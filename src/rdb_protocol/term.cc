// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "rdb_protocol/ql2.hpp"

namespace ql {

void run(Query2 *q, env_t *env, Response2 *res, stream_cache_t *stream_cache) {
    runtime_check(false, strprintf("UNIMPLIMENTED: %p %p %p %p",
                                   q, env, res, stream_cache));
}

} //namespace ql
