// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/wire_func.hpp"

#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/protocol.hpp"

namespace ql {

wire_func_t::wire_func_t() { }

class wire_func_construction_visitor_t : public func_visitor_t {
public:
    explicit wire_func_construction_visitor_t(wire_func_t *_that) : that(_that) { }

    void on_reql_func(const reql_func_t *good_func) {
        that->func = wire_reql_func_t();
        wire_reql_func_t *p = boost::get<wire_reql_func_t>(&that->func);
        p->captured_scope = good_func->captured_scope;
        p->arg_names = good_func->arg_names;
        p->body = good_func->body->get_src();
        p->backtrace = good_func->backtrace();
    }
    void on_js_func(const js_func_t *js_func) {
        that->func = wire_js_func_t();
        wire_js_func_t *p = boost::get<wire_js_func_t>(&that->func);
        p->js_source = js_func->js_source;
        p->js_timeout_ms = js_func->js_timeout_ms;
        p->backtrace = js_func->backtrace();
    }

private:
    wire_func_t *that;
};

wire_func_t::wire_func_t(counted_t<const func_t> f) {
    r_sanity_check(f.has());
    wire_func_construction_visitor_t v(this);
    f->visit(&v);
}

wire_func_t::wire_func_t(protob_t<const Term> body, std::vector<sym_t> arg_names) {
    func = wire_reql_func_t();
    wire_reql_func_t *p = boost::get<wire_reql_func_t>(&func);
    p->arg_names = std::move(arg_names);
    p->body = std::move(body);
    p->backtrace = make_counted_backtrace();
}


// RSI: Go through all code everywhere and look for non-temporary "env;" variables pointing to some env.

struct wire_func_compile_visitor_t : public boost::static_visitor<counted_t<const func_t> > {
    wire_func_compile_visitor_t(env_t *_env) : env(_env) { }
    env_t *env;

    counted_t<const func_t> operator()(const wire_reql_func_t &func) const {
        r_sanity_check(func.body.has() && func.backtrace.has());
        visibility_env_t visibility_env(env, func.captured_scope.compute_visibility().with_func_arg_name_list(func.arg_names));
        return make_counted<reql_func_t>(func.backtrace, func.captured_scope, func.arg_names,
                                         compile_term(&visibility_env, func.body));
    }

    counted_t<const func_t> operator()(const wire_js_func_t &func) const {
        r_sanity_check(func.backtrace.has());
        return make_counted<js_func_t>(func.js_source, func.js_timeout_ms, func.backtrace);
    }

    DISABLE_COPYING(wire_func_compile_visitor_t);
};

counted_t<const func_t> wire_func_t::compile(env_t *env) const {
    return boost::apply_visitor(wire_func_compile_visitor_t(env), func);
}

struct wire_func_get_backtrace_visitor_t : public boost::static_visitor<protob_t<const Backtrace> > {
    template <class T>
    protob_t<const Backtrace> operator()(const T &func) const {
        return func.backtrace;
    }
};

// RSI: Rename this function.  And who uses it?
protob_t<const Backtrace> wire_func_t::get_bt() const {
    return boost::apply_visitor(wire_func_get_backtrace_visitor_t(), func);
}

std::string wire_func_t::debug_str() const {
    // RSI: Give this a reasonable implementation or remove it.
    // Previously implemented as
    //     return source->DebugString();
    return "wire_func_t";
}

write_message_t &operator<<(write_message_t &msg, const wire_reql_func_t &func) {  // NOLINT(runtime/references)
    guarantee(func.body.has() && func.backtrace.has());
    msg << func.captured_scope;
    msg << func.arg_names;
    msg << *func.body;
    msg << *func.backtrace;
    return msg;
}

archive_result_t deserialize(read_stream_t *s, wire_reql_func_t *func) {
    var_scope_t captured_scope;
    archive_result_t res = deserialize(s, &captured_scope);
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

    func->captured_scope = std::move(captured_scope);
    func->arg_names = std::move(arg_names);
    func->body = std::move(body);
    func->backtrace = std::move(backtrace);
    return res;
}

write_message_t &operator<<(write_message_t &msg, const wire_js_func_t &func) {  // NOLINT(runtime/references)
    guarantee(func.backtrace.has());
    msg << func.js_source;
    msg << func.js_timeout_ms;
    msg << *func.backtrace;
    return msg;
}

archive_result_t deserialize(read_stream_t *s, wire_js_func_t *func) {
    wire_js_func_t tmp;

    archive_result_t res = deserialize(s, &tmp.js_source);
    if (res) { return res; }

    res = deserialize(s, &tmp.js_timeout_ms);
    if (res) { return res; }

    protob_t<Backtrace> backtrace = make_counted_backtrace();
    res = deserialize(s, &*backtrace);
    if (res) { return res; }
    tmp.backtrace = backtrace;

    *func = std::move(tmp);
    return res;
}

RDB_IMPL_ME_SERIALIZABLE_1(wire_func_t, func);

// RSI: Remove this old wire_func_t.
#if 0
wire_func_t::wire_func_t() : source(make_counted_term()) { }
wire_func_t::wire_func_t(env_t *env, counted_t<const func_t> func)
    : source(make_counted_term_copy(*func->get_source())), uuid(generate_uuid()) {
    r_sanity_check(env != NULL);
    env->func_cache.precache_func(this, func);
    func->dump_scope(&scope);
}
wire_func_t::wire_func_t(const Term &_source, const std::map<sym_t, Datum> &_scope)
    : source(make_counted_term_copy(_source)), scope(_scope), uuid(generate_uuid()) { }

counted_t<const func_t> wire_func_t::compile(env_t *env) {
    r_sanity_check(!uuid.is_unset() && !uuid.is_nil());
    // RSI: Is this right?
    visibility_env_t visibility_env;
    visibility_env.env = env;
    return env->func_cache.get_or_compile_func(&visibility_env, this);
}

protob_t<const Backtrace> wire_func_t::get_bt() const {
    return get_backtrace(source);
}

std::string wire_func_t::debug_str() const {
    return source->DebugString();
}



void wire_func_t::rdb_serialize(write_message_t &msg) const {  // NOLINT(runtime/references)
    guarantee(source.has());
    msg << *source;
    msg << scope;
    msg << uuid;
}

archive_result_t wire_func_t::rdb_deserialize(read_stream_t *stream) {
    guarantee(source.has());
    source = make_counted_term();
    archive_result_t res = deserialize(stream, source.get());
    if (res != ARCHIVE_SUCCESS) { return res; }
    res = deserialize(stream, &scope);
    if (res != ARCHIVE_SUCCESS) { return res; }
    return deserialize(stream, &uuid);
}
#endif  // 0

gmr_wire_func_t::gmr_wire_func_t(counted_t<const func_t> _group,
                                 counted_t<const func_t> _map,
                                 counted_t<const func_t> _reduce)
    : group(_group), map(_map), reduce(_reduce) { }

counted_t<const func_t> gmr_wire_func_t::compile_group(env_t *env) {
    return group.compile(env);
}
counted_t<const func_t> gmr_wire_func_t::compile_map(env_t *env) {
    return map.compile(env);
}
counted_t<const func_t> gmr_wire_func_t::compile_reduce(env_t *env) {
    return reduce.compile(env);
}


}  // namespace ql
