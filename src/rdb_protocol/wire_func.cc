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

wire_func_t::wire_func_t(const counted_t<const func_t> &f) : func(f) {
    r_sanity_check(func.has());
}

wire_func_t::wire_func_t(protob_t<const Term> body,
                         std::vector<sym_t> arg_names,
                         backtrace_id_t bt) {
    compile_env_t env(var_visibility_t().with_func_arg_name_list(arg_names));
    func = make_counted<reql_func_t>(bt, var_scope_t(), arg_names,
                                     compile_term(&env, body));
}

wire_func_t::wire_func_t(const wire_func_t &copyee)
    : func(copyee.func) { }

wire_func_t &wire_func_t::operator=(const wire_func_t &assignee) {
    func = assignee.func;
    return *this;
}

wire_func_t::~wire_func_t() { }

counted_t<const func_t> wire_func_t::compile_wire_func() const {
    r_sanity_check(func.has());
    return func;
}

backtrace_id_t wire_func_t::get_bt() const {
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
        backtrace_id_t backtrace = reql_func->backtrace();
        serialize<W>(wm, backtrace);
    }

    void on_js_func(const js_func_t *js_func) {
        serialize<W>(wm, wire_func_type_t::JS);
        const std::string &js_source = js_func->js_source;
        serialize<W>(wm, js_source);
        const uint64_t &js_timeout_ms = js_func->js_timeout_ms;
        serialize<W>(wm, js_timeout_ms);
        backtrace_id_t backtrace = js_func->backtrace();
        serialize<W>(wm, backtrace);
    }

private:
    write_message_t *wm;
};

template <cluster_version_t W>
void serialize(write_message_t *wm, const wire_func_t &wf) {
    r_sanity_check(wf.func.has());
    wire_func_serialization_visitor_t<W> v(wm);
    wf.func->visit(&v);
}

INSTANTIATE_SERIALIZE_FOR_CLUSTER_AND_DISK(wire_func_t);

template <cluster_version_t W>
archive_result_t deserialize(read_stream_t *s, wire_func_t *wf) {
    archive_result_t res;

    // When deserializing a protobuf backtrace, we won't actually be able to use it
    // for anything since all queries will be using backtrace ids.  We will just
    // provide an empty backtrace id in this case.
    Backtrace dummy_bt;
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

        res = deserialize_protobuf(s, &dummy_bt);
        if (bad(res)) { return res; }

        compile_env_t env(
            scope.compute_visibility().with_func_arg_name_list(arg_names));
        wf->func = make_counted<reql_func_t>(
            backtrace_id_t::empty(), scope, arg_names, compile_term(&env, body));
        return res;
    }
    case wire_func_type_t::JS: {
        std::string js_source;
        res = deserialize<W>(s, &js_source);
        if (bad(res)) { return res; }

        uint64_t js_timeout_ms;
        res = deserialize<W>(s, &js_timeout_ms);
        if (bad(res)) { return res; }

        res = deserialize_protobuf(s, &dummy_bt);
        if (bad(res)) { return res; }

        wf->func = make_counted<js_func_t>(js_source, js_timeout_ms,
                                           backtrace_id_t::empty());
        return res;
    }
    default:
        unreachable();
    }
}

template archive_result_t deserialize<cluster_version_t::v1_13>(
        read_stream_t *, wire_func_t *);
template archive_result_t deserialize<cluster_version_t::v1_13_2>(
        read_stream_t *, wire_func_t *);
template archive_result_t deserialize<cluster_version_t::v1_14>(
        read_stream_t *, wire_func_t *);
template archive_result_t deserialize<cluster_version_t::v1_15>(
        read_stream_t *, wire_func_t *);
template archive_result_t deserialize<cluster_version_t::v1_16>(
        read_stream_t *, wire_func_t *);
template archive_result_t deserialize<cluster_version_t::v2_0>(
        read_stream_t *, wire_func_t *);

template <>
archive_result_t deserialize<cluster_version_t::v2_1_is_latest>(read_stream_t *s,
                                                                wire_func_t *wf) {
    const cluster_version_t W = cluster_version_t::v2_1_is_latest;
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

        backtrace_id_t bt;
        res = deserialize<W>(s, &bt);
        if (bad(res)) { return res; }

        compile_env_t env(
            scope.compute_visibility().with_func_arg_name_list(arg_names));
        wf->func = make_counted<reql_func_t>(
            bt, scope, arg_names, compile_term(&env, body));
        return res;
    }
    case wire_func_type_t::JS: {
        std::string js_source;
        res = deserialize<W>(s, &js_source);
        if (bad(res)) { return res; }

        uint64_t js_timeout_ms;
        res = deserialize<W>(s, &js_timeout_ms);
        if (bad(res)) { return res; }

        backtrace_id_t bt;
        res = deserialize<W>(s, &bt);
        if (bad(res)) { return res; }

        wf->func = make_counted<js_func_t>(js_source, js_timeout_ms, bt);
        return res;
    }
    default:
        unreachable();
    }
}

template <cluster_version_t W>
void serialize(write_message_t *wm, const maybe_wire_func_t &mwf) {
    bool has_value = mwf.has();
    serialize<W>(wm, has_value);
    if (has_value) {
        serialize<W>(wm, mwf.wrapped);
    }
}

template <cluster_version_t W>
archive_result_t deserialize(read_stream_t *s, maybe_wire_func_t *mwf) {
    bool has_value;
    archive_result_t res = deserialize<W>(s, &has_value);
    if (bad(res)) { return res; }
    if (has_value) {
        return deserialize<W>(s, &mwf->wrapped);
    } else {
        mwf->wrapped = wire_func_t();
        return archive_result_t::SUCCESS;
    }
}

INSTANTIATE_SERIALIZABLE_SINCE_v1_13(maybe_wire_func_t);

counted_t<const func_t> maybe_wire_func_t::compile_wire_func_or_null() const {
    if (wrapped.has()) {
        return wrapped.compile_wire_func();
    } else {
        return counted_t<const func_t>();
    }
}

group_wire_func_t::group_wire_func_t(std::vector<counted_t<const func_t> > &&_funcs,
                                     bool _append_index, bool _multi)
    : append_index(_append_index), multi(_multi) {
    funcs.reserve(_funcs.size());
    for (size_t i = 0; i < _funcs.size(); ++i) {
        funcs.push_back(wire_func_t(std::move(_funcs[i])));
    }
}

std::vector<counted_t<const func_t> > group_wire_func_t::compile_funcs() const {
    std::vector<counted_t<const func_t> > ret;
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

backtrace_id_t group_wire_func_t::get_bt() const {
    return bt;
}

RDB_IMPL_SERIALIZABLE_4_SINCE_v1_13(group_wire_func_t, funcs, append_index, multi, bt);

RDB_IMPL_SERIALIZABLE_0_SINCE_v1_13(count_wire_func_t);

RDB_IMPL_SERIALIZABLE_0_FOR_CLUSTER(zip_wire_func_t);

RDB_IMPL_SERIALIZABLE_2_SINCE_v1_13(filter_wire_func_t, filter_func, default_filter_val);

RDB_MAKE_SERIALIZABLE_1_FOR_CLUSTER(distinct_wire_func_t, use_index);

}  // namespace ql
