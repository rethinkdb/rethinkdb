// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_QL2_HPP_
#define RDB_PROTOCOL_QL2_HPP_

class Query;
class Response;
template <class> class scoped_ptr_t;

namespace ql {
class stream_cache2_t;
class env_t;

// Runs a query!  This is all outside code should ever need to call.  See
// term.cc for definition.
void run(protob_t<Query> q, scoped_ptr_t<env_t> &&env_ptr,
         Response *res, stream_cache2_t *stream_cache2,
         bool *response_needed_out);
}  // namespace ql

#endif // RDB_PROTOCOL_QL2_HPP_
