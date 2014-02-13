// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/wire_func.hpp"

#include "containers/archive/archive.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/term_walker.hpp"

namespace ql {

wire_func_t::wire_func_t() { }

wire_func_t::wire_func_t(const counted_t<func_t> &f) : func(f) {
    r_sanity_check(func.has());
}

wire_func_t::wire_func_t(protob_t<const Term> body, std::vector<sym_t> arg_names,
                         protob_t<const Backtrace> backtrace) {
    compile_env_t env(var_visibility_t().with_func_arg_name_list(arg_names));
    func = make_counted<reql_func_t>(backtrace, var_scope_t(), arg_names, compile_term(&env, body));
}

wire_func_t::wire_func_t(const wire_func_t &copyee)
    : func(copyee.func) { }

wire_func_t &wire_func_t::operator=(const wire_func_t &assignee) {
    func = assignee.func;
    return *this;
}

wire_func_t::~wire_func_t() { }


counted_t<func_t> wire_func_t::compile_wire_func() const {
    return func;
}

protob_t<const Backtrace> wire_func_t::get_bt() const {
    return func->backtrace();
}

enum class wire_func_type_t { REQL, JS };

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(wire_func_type_t, int8_t,
                                      wire_func_type_t::REQL, wire_func_type_t::JS);

class wire_func_serialization_visitor_t : public func_visitor_t {
public:
    explicit wire_func_serialization_visitor_t(write_message_t *_msg) : msg(_msg) { }

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


void wire_func_t::rdb_serialize(write_message_t &msg) const { // NOLINT
    wire_func_serialization_visitor_t v(&msg);
    func->visit(&v);
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

        compile_env_t env(
            scope.compute_visibility().with_func_arg_name_list(arg_names));
        func = make_counted<reql_func_t>(
            backtrace, scope, arg_names, compile_term(&env, body));
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

        func = make_counted<js_func_t>(js_source, js_timeout_ms, backtrace);
        return res;
    } break;
    default:
        unreachable();
    }
}

group_wire_func_t::group_wire_func_t(std::vector<counted_t<func_t> > &&_funcs) {
    funcs.reserve(_funcs.size());
    for (size_t i = 0; i < _funcs.size(); ++i) {
        funcs.push_back(wire_func_t(std::move(_funcs[i])));
    }
}
std::vector<counted_t<func_t> > group_wire_func_t::compile_funcs() const {
    std::vector<counted_t<func_t> > ret;
    ret.reserve(funcs.size());
    for (size_t i = 0; i < funcs.size(); ++i) {
        ret.push_back(funcs[i].compile_wire_func());
    }
    return std::move(ret);
}


map_wire_func_t map_wire_func_t::make_safely(
    pb::dummy_var_t dummy_var,
    const std::function<protob_t<Term>(sym_t argname)> &body_generator,
    protob_t<const Backtrace> backtrace) {
    const sym_t varname = dummy_var_to_sym(dummy_var);
    protob_t<Term> body = body_generator(varname);
    propagate_backtrace(body.get(), backtrace.get());
    return map_wire_func_t(body, make_vector(varname), backtrace);
}

RDB_IMPL_SERIALIZABLE_2(filter_wire_func_t, filter_func, default_filter_val);

void bt_wire_func_t::rdb_serialize(write_message_t &msg) const { // NOLINT
    msg << *bt;
}

archive_result_t bt_wire_func_t::rdb_deserialize(read_stream_t *s) {
    // It's OK to cheat on const-ness during deserialization.
    return deserialize(s, const_cast<Backtrace *>(&*bt));
}

}  // namespace ql
