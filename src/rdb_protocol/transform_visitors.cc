// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/transform_visitors.hpp"


namespace query_language {

transform_visitor_t::transform_visitor_t(boost::shared_ptr<scoped_cJSON_t> _json, json_list_t *_out, query_language::runtime_environment_t *_env, ql::env_t *_ql_env, const scopes_t &_scopes, const backtrace_t &_backtrace)
    : json(_json), out(_out), env(_env), ql_env(_ql_env),
      scopes(_scopes), backtrace(_backtrace)
{ }

// All of this logic is analogous to the eager logic in datum_stream.cc.  This
// code duplication needs to go away, but I'm not 100% sure how to do it (there
// are sometimes minor differences between the lazy and eager evaluations) and
// it definitely isn't making it into 1.4.
void transform_visitor_t::operator()(ql::map_wire_func_t &func/*NOLINT*/) const {
    ql::env_checkpoint_t(ql_env, &ql::env_t::discard_checkpoint);
    const ql::datum_t *arg = ql_env->add_ptr(new ql::datum_t(json, ql_env));
    out->push_back(func.compile(ql_env)->call(arg)->as_datum()->as_json());
}

void transform_visitor_t::operator()(ql::concatmap_wire_func_t &func/*NOLINT*/) const {
    ql::env_checkpoint_t(ql_env, &ql::env_t::discard_checkpoint);
    const ql::datum_t *arg = ql_env->add_ptr(new ql::datum_t(json, ql_env));
    ql::datum_stream_t *ds = func.compile(ql_env)->call(arg)->as_seq();
    while (const ql::datum_t *d = ds->next()) out->push_back(d->as_json());
}

void transform_visitor_t::operator()(ql::filter_wire_func_t &func/*NOLINT*/) const {
    ql::env_checkpoint_t(ql_env, &ql::env_t::discard_checkpoint);
    ql::func_t *f = func.compile(ql_env);
    const ql::datum_t *arg = ql_env->add_ptr(new ql::datum_t(json, ql_env));
    if (f->filter_call(ql_env, arg)) out->push_back(arg->as_json());
}

terminal_initializer_visitor_t::terminal_initializer_visitor_t(
    rget_read_response_t::result_t *_out,
    query_language::runtime_environment_t *_env,
    ql::env_t *_ql_env,
    const scopes_t &_scopes,
    const backtrace_t &_backtrace)
    : out(_out), env(_env), ql_env(_ql_env), scopes(_scopes), backtrace(_backtrace)
{ }

void terminal_initializer_visitor_t::operator()(
    const rdb_protocol_details::Length &) const {
    rget_read_response_t::length_t l;
    l.length = 0;
    *out = l;
}

terminal_visitor_t::terminal_visitor_t(boost::shared_ptr<scoped_cJSON_t> _json,
                   query_language::runtime_environment_t *_env,
                   ql::env_t *_ql_env,
                   const scopes_t &_scopes,
                   const backtrace_t &_backtrace,
                   rget_read_response_t::result_t *_out)
    : json(_json), env(_env), ql_env(_ql_env),
      scopes(_scopes), backtrace(_backtrace), out(_out)
{ }

void terminal_visitor_t::operator()(const rdb_protocol_details::Length &) const {
    rget_read_response_t::length_t *res_length = boost::get<rget_read_response_t::length_t>(out);
    guarantee(res_length);
    ++res_length->length;
}

void terminal_initializer_visitor_t::operator()(
    UNUSED const ql::gmr_wire_func_t &f) const {
    *out = ql::wire_datum_map_t();
}
// All of this logic is analogous to the eager logic in datum_stream.cc.  This
// code duplication needs to go away, but I'm not 100% sure how to do it (there
// are sometimes minor differences between the lazy and eager evaluations) and
// it definitely isn't making it into 1.4.
void terminal_visitor_t::operator()(ql::gmr_wire_func_t &func/*NOLINT*/) const {
    ql::wire_datum_map_t *obj = boost::get<ql::wire_datum_map_t>(out);
    guarantee(obj);

    const ql::datum_t *el = ql_env->add_ptr(new ql::datum_t(json, ql_env));
    const ql::datum_t *el_group = func.compile_group(ql_env)->call(el)->as_datum();
    const ql::datum_t *elm = ql_env->add_ptr(new ql::datum_t(json, ql_env));
    const ql::datum_t *el_map = func.compile_map(ql_env)->call(elm)->as_datum();

    if (!obj->has(el_group)) {
        obj->set(el_group, el_map);
    } else {
        const ql::datum_t *lhs = obj->get(el_group);
        obj->set(el_group, func.compile_reduce(ql_env)->call(lhs, el_map)->as_datum());
    }
}

void terminal_initializer_visitor_t::operator()(
    UNUSED const ql::count_wire_func_t &f) const {
    *out = ql::wire_datum_t(ql_env->add_ptr(new ql::datum_t(0.0)));
}

void terminal_visitor_t::operator()(UNUSED const ql::count_wire_func_t &func) const {
    // TODO: just pass an int around
    ql::wire_datum_t *d = boost::get<ql::wire_datum_t>(out);
    d->reset(ql_env->add_ptr(new ql::datum_t(d->get()->as_int() + 1.0)));
}

void terminal_initializer_visitor_t::operator()(
    UNUSED const ql::reduce_wire_func_t &f) const {
    *out = rget_read_response_t::empty_t();
}

void terminal_visitor_t::operator()(ql::reduce_wire_func_t &func/*NOLINT*/) const {
    ql::wire_datum_t *d = boost::get<ql::wire_datum_t>(out);
    const ql::datum_t *rhs = ql_env->add_ptr(new ql::datum_t(json, ql_env));
    if (d) {
        d->reset(func.compile(ql_env)->call(d->get(), rhs)->as_datum());
    } else {
        guarantee(boost::get<rget_read_response_t::empty_t>(out));
        *out = ql::wire_datum_t(rhs);
    }
}

} //namespace query_language
