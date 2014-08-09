// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/wire_func.hpp"

#include "containers/archive/boost_types.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/archive/archive.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/term_walker.hpp"
#include "stl_utils.hpp"

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
    r_sanity_check(func.has());
    return func;
}

protob_t<const Backtrace> wire_func_t::get_bt() const {
    r_sanity_check(func.has());
    return func->backtrace();
}

enum class wire_func_type_t { REQL, JS };

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(wire_func_type_t, int8_t,
                                      wire_func_type_t::REQL, wire_func_type_t::JS);

template <cluster_version_t W>
class wire_func_serialization_visitor_t : public func_visitor_t {
public:
    explicit wire_func_serialization_visitor_t(write_message_t *_wm) : wm(_wm) { }

    void on_reql_func(const reql_func_t *reql_func) {
        serialize<W>(wm, wire_func_type_t::REQL);
        const var_scope_t &scope = reql_func->captured_scope;
        serialize<W>(wm, scope);
        const std::vector<sym_t> &arg_names = reql_func->arg_names;
        serialize<W>(wm, arg_names);
        const protob_t<const Term> &body = reql_func->body->get_src();
        serialize_protobuf(wm, *body);
        const protob_t<const Backtrace> &backtrace = reql_func->backtrace();
        serialize_protobuf(wm, *backtrace);
    }

    void on_js_func(const js_func_t *js_func) {
        serialize<W>(wm, wire_func_type_t::JS);
        const std::string &js_source = js_func->js_source;
        serialize<W>(wm, js_source);
        const uint64_t &js_timeout_ms = js_func->js_timeout_ms;
        serialize<W>(wm, js_timeout_ms);
        const protob_t<const Backtrace> &backtrace = js_func->backtrace();
        serialize_protobuf(wm, *backtrace);
    }

private:
    write_message_t *wm;
};

template <cluster_version_t W>
void wire_func_t::rdb_serialize(write_message_t *wm) const {
    r_sanity_check(func.has());
    wire_func_serialization_visitor_t<W> v(wm);
    func->visit(&v);
}

template <cluster_version_t W>
archive_result_t wire_func_t::rdb_deserialize(read_stream_t *s) {
    archive_result_t res;

    wire_func_type_t type;
    res = deserialize<W>(s, &type);
    if (bad(res)) { return res; }
    switch (type) {
    case wire_func_type_t::REQL: {
        var_scope_t scope;
        res = deserialize<W>(s, &scope);
        if (bad(res)) { return res; }

        std::vector<sym_t> arg_names;
        res = deserialize<W>(s, &arg_names);
        if (bad(res)) { return res; }

        protob_t<Term> body = make_counted_term();
        res = deserialize_protobuf(s, &*body);
        if (bad(res)) { return res; }

        protob_t<Backtrace> backtrace = make_counted_backtrace();
        res = deserialize_protobuf(s, &*backtrace);
        if (bad(res)) { return res; }

        compile_env_t env(
            scope.compute_visibility().with_func_arg_name_list(arg_names));
        func = make_counted<reql_func_t>(
            backtrace, scope, arg_names, compile_term(&env, body));
        return res;
    } break;
    case wire_func_type_t::JS: {
        std::string js_source;
        res = deserialize<W>(s, &js_source);
        if (bad(res)) { return res; }

        uint64_t js_timeout_ms;
        res = deserialize<W>(s, &js_timeout_ms);
        if (bad(res)) { return res; }

        protob_t<Backtrace> backtrace = make_counted_backtrace();
        res = deserialize_protobuf(s, &*backtrace);
        if (bad(res)) { return res; }

        func = make_counted<js_func_t>(js_source, js_timeout_ms, backtrace);
        return res;
    } break;
    default:
        unreachable();
    }
}

INSTANTIATE_SERIALIZABLE_SELF_SINCE_v1_13(wire_func_t);


template <cluster_version_t W>
void maybe_wire_func_t::rdb_serialize(write_message_t *wm) const {
    bool has_value = wrapped.has();
    serialize<W>(wm, has_value);
    if (has_value) {
        serialize<W>(wm, wrapped);
    }
}

template <cluster_version_t W>
archive_result_t maybe_wire_func_t::rdb_deserialize(read_stream_t *s) {
    bool has_value;
    archive_result_t res = deserialize<W>(s, &has_value);
    if (bad(res)) { return res; }
    if (has_value) {
        return deserialize<W>(s, &wrapped);
    } else {
        wrapped = wire_func_t();
        return archive_result_t::SUCCESS;
    }
}

INSTANTIATE_SERIALIZABLE_SELF_SINCE_v1_13(maybe_wire_func_t);

counted_t<func_t> maybe_wire_func_t::compile_wire_func_or_null() const {
    if (wrapped.has()) {
        return wrapped.compile_wire_func();
    } else {
        return counted_t<func_t>();
    }
}

group_wire_func_t::group_wire_func_t(std::vector<counted_t<func_t> > &&_funcs,
                                     bool _append_index, bool _multi)
    : append_index(_append_index), multi(_multi) {
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

bool group_wire_func_t::should_append_index() const {
    return append_index;
}

bool group_wire_func_t::is_multi() const {
    return multi;
}

protob_t<const Backtrace> group_wire_func_t::get_bt() const {
    return bt.get_bt();
}

RDB_IMPL_ME_SERIALIZABLE_4_SINCE_v1_13(group_wire_func_t, funcs, append_index, multi, bt);

RDB_IMPL_SERIALIZABLE_0_SINCE_v1_13(count_wire_func_t);

RDB_IMPL_SERIALIZABLE_0(zip_wire_func_t);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(zip_wire_func_t);

RDB_IMPL_SERIALIZABLE_2_SINCE_v1_13(filter_wire_func_t, filter_func, default_filter_val);

RDB_MAKE_SERIALIZABLE_1(distinct_wire_func_t, use_index);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(distinct_wire_func_t);

template <cluster_version_t W>
void bt_wire_func_t::rdb_serialize(write_message_t *wm) const {
    serialize_protobuf(wm, *bt);
}

template <cluster_version_t W>
archive_result_t bt_wire_func_t::rdb_deserialize(read_stream_t *s) {
    protob_t<Backtrace> backtrace = make_counted_backtrace();
    archive_result_t res = deserialize_protobuf(s, backtrace.get());
    if (bad(res)) { return res; }
    bt = std::move(backtrace);
    return archive_result_t::SUCCESS;
}

INSTANTIATE_SERIALIZABLE_SELF_SINCE_v1_13(bt_wire_func_t);

}  // namespace ql
