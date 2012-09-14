#ifndef RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_
#define RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_

#include <list>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>

#include "http/json.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/query_language.pb.h"

namespace query_language {

typedef rdb_protocol_t::rget_read_response_t rget_read_response_t;

class runtime_environment_t;

typedef std::list<boost::shared_ptr<scoped_cJSON_t> > json_list_t;

/* A visitor for applying a transformation to a bit of json. */
class transform_visitor_t : public boost::static_visitor<void> {
public:
    transform_visitor_t(boost::shared_ptr<scoped_cJSON_t> _json, json_list_t *_out, query_language::runtime_environment_t *_env, const scopes_t &_scopes, const backtrace_t &_backtrace);

    void operator()(const Builtin_Filter &filter) const;

    void operator()(const Mapping &mapping) const;

    void operator()(const Builtin_ConcatMap &concatmap) const;

    void operator()(Builtin_Range range) const;

private:
    boost::shared_ptr<scoped_cJSON_t> json;
    json_list_t *out;
    query_language::runtime_environment_t *env;
    scopes_t scopes;
    backtrace_t backtrace;
};

/* A visitor for setting the result type based on a terminal. */
class terminal_initializer_visitor_t : public boost::static_visitor<void> {
public:
    terminal_initializer_visitor_t(rget_read_response_t::result_t *_out,
                                   query_language::runtime_environment_t *_env,
                                   const scopes_t &_scopes,
                                   const backtrace_t &_backtrace);

    void operator()(const Builtin_GroupedMapReduce &) const;

    void operator()(const Reduction &) const;

    void operator()(const rdb_protocol_details::Length &) const;

    void operator()(const WriteQuery_ForEach &) const;
private:
    rget_read_response_t::result_t *out;
    query_language::runtime_environment_t *env;
    scopes_t scopes;
    backtrace_t backtrace;
};

/* A visitor for applying a terminal to a bit of json. */
class terminal_visitor_t : public boost::static_visitor<void> {
public:
    terminal_visitor_t(boost::shared_ptr<scoped_cJSON_t> _json,
                       query_language::runtime_environment_t *_env,
                       const scopes_t &_scopes,
                       const backtrace_t &_backtrace,
                       rget_read_response_t::result_t *_out);

    void operator()(const Builtin_GroupedMapReduce &gmr) const;

    void operator()(const Reduction &r) const;

    void operator()(const rdb_protocol_details::Length &) const;

    void operator()(const WriteQuery_ForEach &w) const;

private:
    boost::shared_ptr<scoped_cJSON_t> json;
    query_language::runtime_environment_t *env;
    scopes_t scopes;
    backtrace_t backtrace;
    rget_read_response_t::result_t *out;
};

}  // namespace query_language

#endif  // RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_
