// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_
#define RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_

#include <list>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>

#include "http/json.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/protocol.hpp"

namespace query_language {

typedef rdb_protocol_t::rget_read_response_t rget_read_response_t;

typedef std::list<boost::shared_ptr<scoped_cJSON_t> > json_list_t;

/* A visitor for applying a transformation to a bit of json. */
class transform_visitor_t : public boost::static_visitor<void> {
public:
    transform_visitor_t(boost::shared_ptr<scoped_cJSON_t> _json,
                        json_list_t *_out,
                        ql::env_t *_ql_env,
                        const scopes_t &_scopes,
                        const backtrace_t &_backtrace);

    // This is a non-const reference because it caches the compiled function
    void operator()(ql::map_wire_func_t &func/*NOLINT*/) const;
    void operator()(ql::filter_wire_func_t &func/*NOLINT*/) const;
    void operator()(ql::concatmap_wire_func_t &func/*NOLINT*/) const;

private:
    boost::shared_ptr<scoped_cJSON_t> json;
    json_list_t *out;
    ql::env_t *ql_env;
    scopes_t scopes;
    backtrace_t backtrace;
};

/* A visitor for setting the result type based on a terminal. */
class terminal_initializer_visitor_t : public boost::static_visitor<void> {
public:
    terminal_initializer_visitor_t(rget_read_response_t::result_t *_out,
                                   ql::env_t *_ql_env,
                                   const scopes_t &_scopes,
                                   const backtrace_t &_backtrace);

    void operator()(const ql::gmr_wire_func_t &) const;
    void operator()(const ql::count_wire_func_t &) const;
    void operator()(const ql::reduce_wire_func_t &) const;
private:
    rget_read_response_t::result_t *out;
    ql::env_t *ql_env;
    scopes_t scopes;
    backtrace_t backtrace;
};

/* A visitor for applying a terminal to a bit of json. */
class terminal_visitor_t : public boost::static_visitor<void> {
public:
    terminal_visitor_t(boost::shared_ptr<scoped_cJSON_t> _json,
                       ql::env_t *_ql_env,
                       const scopes_t &_scopes,
                       const backtrace_t &_backtrace,
                       rget_read_response_t::result_t *_out);

    void operator()(const ql::count_wire_func_t &) const;
    // This is a non-const reference because it caches the compiled function
    void operator()(ql::gmr_wire_func_t &) const;
    void operator()(ql::reduce_wire_func_t &) const;
private:
    boost::shared_ptr<scoped_cJSON_t> json;
    ql::env_t *ql_env;
    scopes_t scopes;
    backtrace_t backtrace;
    rget_read_response_t::result_t *out;
};

}  // namespace query_language

#endif  // RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_
