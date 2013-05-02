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

namespace ql {

typedef rdb_protocol_t::rget_read_response_t rget_read_response_t;

class terminal_exc_visitor_t : public boost::static_visitor<void> {
public:
    terminal_exc_visitor_t(const datum_exc_t &_exc,
                           rget_read_response_t::result_t *_res_out)
        : exc(_exc), res_out(_res_out) { }

    void operator()(const gmr_wire_func_t &func) const {
        *res_out = exc_t(exc.what(), func.get_bt().get(), 1);
    }

    NORETURN void operator()(const count_wire_func_t &) const {
        r_sanity_check(false);  // Server should never crash here.
        unreachable();
    }

    void operator()(const reduce_wire_func_t &func) const {
        *res_out = exc_t(exc.what(), func.get_bt().get(), 1);
    }

private:
    const datum_exc_t exc;
    rget_read_response_t::result_t *res_out;

    DISABLE_COPYING(terminal_exc_visitor_t);
};

class transform_exc_visitor_t : public boost::static_visitor<void> {
public:
    transform_exc_visitor_t(const datum_exc_t &_exc,
                            rget_read_response_t::result_t *_res_out)
        : exc(_exc), res_out(_res_out) { }

    void operator()(const map_wire_func_t &func) const {
        *res_out = exc_t(exc.what(), func.get_bt().get(), 1);
    }

    void operator()(const filter_wire_func_t &func) const {
        *res_out = exc_t(exc.what(), func.get_bt().get(), 1);
    }

    void operator()(const concatmap_wire_func_t &func) const {
        *res_out = exc_t(exc.what(), func.get_bt().get(), 1);
    }

private:
    const datum_exc_t exc;
    rget_read_response_t::result_t *res_out;

    DISABLE_COPYING(transform_exc_visitor_t);
};

} // namespace ql

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

    void operator()(ql::gmr_wire_func_t &f) const {
        counted_t<ql::func_t> group = f.compile_group(ql_env);
        counted_t<ql::func_t> map = f.compile_map(ql_env);
        counted_t<ql::func_t> reduce = f.compile_reduce(ql_env);
        guarantee(group.has() && map.has() && reduce.has());
        *out = ql::wire_datum_map_t();
    }

    void operator()(const ql::count_wire_func_t &) const {
        *out = ql::wire_datum_t(make_counted<ql::datum_t>(0.0));
    }

    void operator()(ql::reduce_wire_func_t &f) const {
        counted_t<ql::func_t> reduce = f.compile(ql_env);
        guarantee(reduce.has());
        *out = rget_read_response_t::empty_t();
    }

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
