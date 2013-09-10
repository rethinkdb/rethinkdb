// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/wire_func.hpp"

#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/protocol.hpp"

namespace ql {

wire_func_t::wire_func_t() { }

wire_func_t::wire_func_t(counted_t<func_t> f) {
    r_sanity_check(f.has());
    cached_func = f;
}

wire_func_t::wire_func_t(protob_t<const Term> body, std::vector<sym_t> arg_names,
                         protob_t<const Backtrace> backtrace) {
    compile_env_t env(var_visibility_t().with_func_arg_name_list(arg_names));
    cached_func = make_counted<reql_func_t>(backtrace, var_scope_t(), arg_names, compile_term(&env, body));
}

wire_func_t::wire_func_t(const wire_func_t &copyee)
    : cached_func(copyee.cached_func) { }

wire_func_t &wire_func_t::operator=(const wire_func_t &assignee) {
    cached_func = assignee.cached_func;
    return *this;
}

wire_func_t::~wire_func_t() { }


counted_t<func_t> wire_func_t::compile_wire_func() const {
    return cached_func;
}

protob_t<const Backtrace> wire_func_t::get_bt() const {
    return cached_func->backtrace();
}

enum class wire_func_type_t { REQL, JS };

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(wire_func_type_t, int8_t,
                                      wire_func_type_t::REQL, wire_func_type_t::JS);

class wire_func_serialization_visitor_t : public func_visitor_t {
public:
    wire_func_serialization_visitor_t(write_message_t *_msg) : msg(_msg) { }

    void on_reql_func(const reql_func_t *reql_func) {
        *msg << wire_func_type_t::REQL;
        const var_scope_t &scope = reql_func->captured_scope;
        *msg << scope;
        const std::vector<sym_t> &arg_names = reql_func->arg_names;
        *msg << arg_names;
        const protob_t<const Term> &body = reql_func->body->get_src();
        *msg << *body;
        const protob_t<const Backtrace> &backtrace = reql_func->backtrace();
        *msg << *backtrace;
    }

    void on_js_func(const js_func_t *js_func) {
        *msg << wire_func_type_t::JS;
        const std::string &js_source = js_func->js_source;
        *msg << js_source;
        const uint64_t &js_timeout_ms = js_func->js_timeout_ms;
        *msg << js_timeout_ms;
        const protob_t<const Backtrace> &backtrace = js_func->backtrace();
        *msg << *backtrace;
    }

private:
    write_message_t *msg;
};

void wire_func_t::rdb_serialize(write_message_t &msg) const {
    wire_func_serialization_visitor_t v(&msg);
    cached_func->visit(&v);
}

archive_result_t wire_func_t::rdb_deserialize(read_stream_t *s) {
    wire_func_type_t type;
    archive_result_t res = deserialize(s, &type);
    if (res) { return res; }
    switch (type) {
    case wire_func_type_t::REQL: {
        var_scope_t scope;
        res = deserialize(s, &scope);
        if (res) { return res; }

        std::vector<sym_t> arg_names;
        res = deserialize(s, &arg_names);
        if (res) { return res; }

        protob_t<Term> body = make_counted_term();
        res = deserialize(s, &*body);
        if (res) { return res; }

        protob_t<Backtrace> backtrace = make_counted_backtrace();
        res = deserialize(s, &*backtrace);
        if (res) { return res; }

        compile_env_t env(scope.compute_visibility().with_func_arg_name_list(arg_names));
        cached_func = make_counted<reql_func_t>(backtrace, scope, arg_names, compile_term(&env, body));
        return res;
    } break;
    case wire_func_type_t::JS: {
        std::string js_source;
        res = deserialize(s, &js_source);
        if (res) { return res; }

        uint64_t js_timeout_ms;
        res = deserialize(s, &js_timeout_ms);
        if (res) { return res; }

        protob_t<Backtrace> backtrace = make_counted_backtrace();
        res = deserialize(s, &*backtrace);
        if (res) { return res; }

        cached_func = make_counted<js_func_t>(js_source, js_timeout_ms, backtrace);
        return res;
    } break;
    default:
        unreachable();
    }
}

gmr_wire_func_t::gmr_wire_func_t(counted_t<func_t> _group,
                                 counted_t<func_t> _map,
                                 counted_t<func_t> _reduce)
    : group(_group), map(_map), reduce(_reduce) { }

counted_t<func_t> gmr_wire_func_t::compile_group() const {
    return group.compile_wire_func();
}
counted_t<func_t> gmr_wire_func_t::compile_map() const {
    return map.compile_wire_func();
}
counted_t<func_t> gmr_wire_func_t::compile_reduce() const {
    return reduce.compile_wire_func();
}


}  // namespace ql
