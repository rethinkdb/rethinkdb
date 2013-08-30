// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/wire_func.hpp"

#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"

namespace ql {

wire_func_t::wire_func_t() : source(make_counted_term()) { }
wire_func_t::wire_func_t(env_t *env, counted_t<func_t> func)
    : source(make_counted_term_copy(*func->source)), uuid(generate_uuid()) {
    // RSI: vvv this can be sometimes NULL, sometimes not?  This is bad.  When can it
    // be NULL?
    if (env) {
        env->func_cache.precache_func(this, func);
    }
    func->dump_scope(&scope);
}
wire_func_t::wire_func_t(const Term &_source, const std::map<sym_t, Datum> &_scope)
    : source(make_counted_term_copy(_source)), scope(_scope), uuid(generate_uuid()) { }

counted_t<func_t> wire_func_t::compile(env_t *env) {
    r_sanity_check(!uuid.is_unset() && !uuid.is_nil());
    return env->func_cache.get_or_compile_func(env, this);
}

protob_t<const Backtrace> wire_func_t::get_bt() const {
    return source.make_child(&source->GetExtension(ql2::extension::backtrace));
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

gmr_wire_func_t::gmr_wire_func_t(env_t *env, counted_t<func_t> _group,
                                 counted_t<func_t> _map,
                                 counted_t<func_t> _reduce)
    : group(env, _group), map(env, _map), reduce(env, _reduce) { }

counted_t<func_t> gmr_wire_func_t::compile_group(env_t *env) {
    return group.compile(env);
}
counted_t<func_t> gmr_wire_func_t::compile_map(env_t *env) {
    return map.compile(env);
}
counted_t<func_t> gmr_wire_func_t::compile_reduce(env_t *env) {
    return reduce.compile(env);
}


}  // namespace ql
