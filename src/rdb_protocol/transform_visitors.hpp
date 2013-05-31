// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_
#define RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_

#include <list>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "http/json.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/protocol.hpp"

namespace ql {

void terminal_exception(const datum_exc_t &exc,
                        const rdb_protocol_details::terminal_variant_t &t,
                        rdb_protocol_t::rget_read_response_t::result_t *out);

void transform_exception(const datum_exc_t &exc,
                         const rdb_protocol_details::transform_variant_t &t,
                         rdb_protocol_t::rget_read_response_t::result_t *out);

} // namespace ql

namespace query_language {

void transform_apply(ql::env_t *ql_env,
                     const backtrace_t &backtrace,
                     boost::shared_ptr<scoped_cJSON_t> json,
                     rdb_protocol_details::transform_variant_t *t,
                     std::list<boost::shared_ptr<scoped_cJSON_t> > *out);

// Sets the result type based on a terminal.
void terminal_initialize(ql::env_t *ql_env,
                         const backtrace_t &backtrace,
                         rdb_protocol_details::terminal_variant_t *t,
                         rdb_protocol_t::rget_read_response_t::result_t *out);

void terminal_apply(ql::env_t *ql_env,
                    const backtrace_t &backtrace,
                    boost::shared_ptr<scoped_cJSON_t> _json,
                    rdb_protocol_details::terminal_variant_t *t,
                    rdb_protocol_t::rget_read_response_t::result_t *out);

}  // namespace query_language

#endif  // RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_
