// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/transform_visitors.hpp"

typedef rdb_protocol_t::rget_read_response_t rget_read_response_t;

namespace ql {

class terminal_exc_visitor_t : public boost::static_visitor<void> {
public:
    terminal_exc_visitor_t(const datum_exc_t &_exc,
                           rget_read_response_t::result_t *_res_out)
        : exc(_exc), res_out(_res_out) { }

    void operator()(const gmr_wire_func_t &func) const {
        *res_out = exc_t(exc, func.get_bt().get(), 1);
    }

    NORETURN void operator()(const count_wire_func_t &) const {
        r_sanity_check(false);  // Server should never crash here.
        unreachable();
    }

    void operator()(const reduce_wire_func_t &func) const {
        *res_out = exc_t(exc, func.get_bt().get(), 1);
    }

private:
    const datum_exc_t exc;
    rget_read_response_t::result_t *res_out;

    DISABLE_COPYING(terminal_exc_visitor_t);
};

void terminal_exception(const datum_exc_t &exc,
                        const rdb_protocol_details::terminal_variant_t &t,
                        rdb_protocol_t::rget_read_response_t::result_t *out) {
    boost::apply_visitor(terminal_exc_visitor_t(exc, out),
                         t);
}

class transform_exc_visitor_t : public boost::static_visitor<void> {
public:
    transform_exc_visitor_t(const datum_exc_t &_exc,
                            rget_read_response_t::result_t *_res_out)
        : exc(_exc), res_out(_res_out) { }

    void operator()(const map_wire_func_t &func) const {
        *res_out = exc_t(exc, func.get_bt().get(), 1);
    }

    void operator()(const filter_wire_func_t &func) const {
        *res_out = exc_t(exc, func.get_bt().get(), 1);
    }

    void operator()(const concatmap_wire_func_t &func) const {
        *res_out = exc_t(exc, func.get_bt().get(), 1);
    }

private:
    const datum_exc_t exc;
    rget_read_response_t::result_t *res_out;

    DISABLE_COPYING(transform_exc_visitor_t);
};

void transform_exception(const datum_exc_t &exc,
                         const rdb_protocol_details::transform_variant_t &t,
                         rdb_protocol_t::rget_read_response_t::result_t *out) {
    boost::apply_visitor(transform_exc_visitor_t(exc, out),
                         t);
}


}  // namespace ql


namespace query_language {

/* A visitor for applying a transformation to a bit of json. */
class transform_visitor_t : public boost::static_visitor<void> {
public:
    transform_visitor_t(boost::shared_ptr<scoped_cJSON_t> _json,
                        std::list<boost::shared_ptr<scoped_cJSON_t> > *_out,
                        ql::env_t *_ql_env,
                        const backtrace_t &_backtrace);

    // This is a non-const reference because it caches the compiled function
    void operator()(ql::map_wire_func_t &func) const;  // NOLINT(runtime/references)
    void operator()(ql::filter_wire_func_t &func) const;  // NOLINT(runtime/references)
    void operator()(ql::concatmap_wire_func_t &func) const;  // NOLINT(runtime/references)

private:
    boost::shared_ptr<scoped_cJSON_t> json;
    std::list<boost::shared_ptr<scoped_cJSON_t> > *out;
    ql::env_t *ql_env;
    backtrace_t backtrace;
};

transform_visitor_t::transform_visitor_t(boost::shared_ptr<scoped_cJSON_t> _json,
                                         json_list_t *_out,
                                         ql::env_t *_ql_env,
                                         const backtrace_t &_backtrace)
    : json(_json), out(_out), ql_env(_ql_env),
      backtrace(_backtrace) { }

// All of this logic is analogous to the eager logic in datum_stream.cc.  This
// code duplication needs to go away, but I'm not 100% sure how to do it (there
// are sometimes minor differences between the lazy and eager evaluations) and
// it definitely isn't making it into 1.4.
void transform_visitor_t::operator()(ql::map_wire_func_t &func) const {  // NOLINT(runtime/references)
    counted_t<const ql::datum_t> arg(new ql::datum_t(json, ql_env));
    out->push_back(func.compile(ql_env)->call(arg)->as_datum()->as_json());
}

void transform_visitor_t::operator()(ql::concatmap_wire_func_t &func) const {  // NOLINT(runtime/references)
    counted_t<const ql::datum_t> arg(new ql::datum_t(json, ql_env));
    counted_t<ql::datum_stream_t> ds = func.compile(ql_env)->call(arg)->as_seq();
    while (counted_t<const ql::datum_t> d = ds->next()) {
        out->push_back(d->as_json());
    }
}

void transform_visitor_t::operator()(ql::filter_wire_func_t &func) const {  // NOLINT(runtime/references)
    counted_t<ql::func_t> f = func.compile(ql_env);
    counted_t<const ql::datum_t> arg(new ql::datum_t(json, ql_env));
    if (f->filter_call(arg)) {
        out->push_back(arg->as_json());
    }
}


void transform_apply(ql::env_t *ql_env,
                     const backtrace_t &backtrace,
                     boost::shared_ptr<scoped_cJSON_t> json,
                     rdb_protocol_details::transform_variant_t *t,
                     std::list<boost::shared_ptr<scoped_cJSON_t> > *out) {
    boost::apply_visitor(transform_visitor_t(json, out, ql_env, backtrace),
                         *t);
}


/* A visitor for applying a terminal to a bit of json. */
class terminal_visitor_t : public boost::static_visitor<void> {
public:
    terminal_visitor_t(boost::shared_ptr<scoped_cJSON_t> _json,
                       ql::env_t *_ql_env,
                       const backtrace_t &_backtrace,
                       rget_read_response_t::result_t *_out);

    void operator()(const ql::count_wire_func_t &) const;
    // This is a non-const reference because it caches the compiled function
    void operator()(ql::gmr_wire_func_t &) const;
    void operator()(ql::reduce_wire_func_t &) const;
private:
    boost::shared_ptr<scoped_cJSON_t> json;
    ql::env_t *ql_env;
    backtrace_t backtrace;
    rget_read_response_t::result_t *out;
};


terminal_visitor_t::terminal_visitor_t(boost::shared_ptr<scoped_cJSON_t> _json,
                   ql::env_t *_ql_env,
                   const backtrace_t &_backtrace,
                   rget_read_response_t::result_t *_out)
    : json(_json), ql_env(_ql_env), backtrace(_backtrace), out(_out) { }

// All of this logic is analogous to the eager logic in datum_stream.cc.  This
// code duplication needs to go away, but I'm not 100% sure how to do it (there
// are sometimes minor differences between the lazy and eager evaluations) and
// it definitely isn't making it into 1.4.
void terminal_visitor_t::operator()(ql::gmr_wire_func_t &func) const {  // NOLINT(runtime/references)
    ql::wire_datum_map_t *obj = boost::get<ql::wire_datum_map_t>(out);
    guarantee(obj);

    counted_t<const ql::datum_t> el(new ql::datum_t(json, ql_env));
    counted_t<const ql::datum_t> el_group
        = func.compile_group(ql_env)->call(el)->as_datum();
    counted_t<const ql::datum_t> elm(new ql::datum_t(json, ql_env));
    counted_t<const ql::datum_t> el_map
        = func.compile_map(ql_env)->call(elm)->as_datum();

    if (!obj->has(el_group)) {
        obj->set(el_group, el_map);
    } else {
        counted_t<const ql::datum_t> lhs = obj->get(el_group);
        obj->set(el_group, func.compile_reduce(ql_env)->call(lhs, el_map)->as_datum());
    }
}

void terminal_visitor_t::operator()(UNUSED const ql::count_wire_func_t &func) const {
    // TODO: just pass an int around
    ql::wire_datum_t *d = boost::get<ql::wire_datum_t>(out);
    d->reset(make_counted<ql::datum_t>(d->get()->as_int() + 1.0));
}

void terminal_visitor_t::operator()(ql::reduce_wire_func_t &func) const {  // NOLINT(runtime/references)
    ql::wire_datum_t *d = boost::get<ql::wire_datum_t>(out);
    counted_t<const ql::datum_t> rhs(new ql::datum_t(json, ql_env));
    if (d) {
        d->reset(func.compile(ql_env)->call(d->get(), rhs)->as_datum());
    } else {
        guarantee(boost::get<rget_read_response_t::empty_t>(out));
        *out = ql::wire_datum_t(rhs);
    }
}

void terminal_apply(ql::env_t *ql_env,
                    const backtrace_t &backtrace,
                    boost::shared_ptr<scoped_cJSON_t> json,
                    rdb_protocol_details::terminal_variant_t *t,
                    rget_read_response_t::result_t *out) {
    boost::apply_visitor(terminal_visitor_t(json, ql_env, backtrace, out),
                         *t);
}


/* A visitor for setting the result type based on a terminal. */
class terminal_initializer_visitor_t : public boost::static_visitor<void> {
public:
    terminal_initializer_visitor_t(rget_read_response_t::result_t *_out,
                                   ql::env_t *_ql_env,
                                   const backtrace_t &_backtrace)
        : out(_out), ql_env(_ql_env), backtrace(_backtrace) { }


    void operator()(ql::gmr_wire_func_t &f) const {  // NOLINT(runtime/references)
        counted_t<ql::func_t> group = f.compile_group(ql_env);
        counted_t<ql::func_t> map = f.compile_map(ql_env);
        counted_t<ql::func_t> reduce = f.compile_reduce(ql_env);
        guarantee(group.has() && map.has() && reduce.has());
        *out = ql::wire_datum_map_t();
    }

    void operator()(const ql::count_wire_func_t &) const {
        *out = ql::wire_datum_t(make_counted<ql::datum_t>(0.0));
    }

    void operator()(ql::reduce_wire_func_t &f) const {  // NOLINT(runtime/references)
        counted_t<ql::func_t> reduce = f.compile(ql_env);
        guarantee(reduce.has());
        *out = rget_read_response_t::empty_t();
    }

private:
    rget_read_response_t::result_t *out;
    ql::env_t *ql_env;
    backtrace_t backtrace;
};

void terminal_initialize(ql::env_t *ql_env,
                         const backtrace_t &backtrace,
                         rdb_protocol_details::terminal_variant_t *t,
                         rget_read_response_t::result_t *out) {
    boost::apply_visitor(terminal_initializer_visitor_t(out, ql_env,
                                                        backtrace),
                         *t);
}



}  // namespace query_language
