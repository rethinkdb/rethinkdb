// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_
#define RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_

#include <list>

#include "http/json.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/lazy_json.hpp"
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
                     std::list<lazy_json_t> &&jsons,
                     rdb_protocol_details::transform_variant_t *t,
                     std::list<counted_t<const ql::datum_t> > *out);

// Sets the result type based on a terminal.
void terminal_initialize(ql::env_t *ql_env,
                         const backtrace_t &backtrace,
                         rdb_protocol_details::terminal_variant_t *t,
                         rdb_protocol_t::rget_read_response_t::result_t *out);

void terminal_apply(ql::env_t *ql_env,
                    const backtrace_t &backtrace,
                    lazy_json_t json,
                    rdb_protocol_details::terminal_variant_t *t,
                    rdb_protocol_t::rget_read_response_t::result_t *out);

}  // namespace query_language

#endif  // RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_
