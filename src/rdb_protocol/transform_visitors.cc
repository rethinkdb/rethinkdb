// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/transform_visitors.hpp"

#include "rdb_protocol/func.hpp"
#include "rdb_protocol/lazy_json.hpp"

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

    void operator()(const filter_transform_t &transf) const {
        *res_out = exc_t(exc, transf.filter_func.get_bt().get(), 1);
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
    boost::apply_visitor(transform_exc_visitor_t(exc, out), t);
}


}  // namespace ql


namespace query_language {

/* A visitor for applying a transformation to a bit of json. */
class transform_visitor_t : public boost::static_visitor<void> {
public:
    transform_visitor_t(counted_t<const ql::datum_t> _arg,
                        std::vector<counted_t<const ql::datum_t> > *_out,
                        ql::env_t *_ql_env);

    void operator()(const ql::map_wire_func_t &func) const;
    void operator()(const filter_transform_t &func) const;
    void operator()(const ql::concatmap_wire_func_t &func) const;

private:
    counted_t<const ql::datum_t> arg;
    std::vector<counted_t<const ql::datum_t> > *out;
    ql::env_t *ql_env;
};

transform_visitor_t::transform_visitor_t(
    counted_t<const ql::datum_t> _arg,
    std::vector<counted_t<const ql::datum_t> > *_out,
    ql::env_t *_ql_env)
    : arg(_arg), out(_out), ql_env(_ql_env) { }

// All of this logic is analogous to the eager logic in datum_stream.cc.  This
// code duplication needs to go away, but I'm not 100% sure how to do it (there
// are sometimes minor differences between the lazy and eager evaluations) and
// it definitely isn't making it into 1.4.
void transform_visitor_t::operator()(const ql::map_wire_func_t &func) const {
    out->push_back(func.compile_wire_func()->call(ql_env, arg)->as_datum());
}

void transform_visitor_t::operator()(const ql::concatmap_wire_func_t &func) const {
    counted_t<ql::datum_stream_t> ds = func.compile_wire_func()->call(ql_env, arg)->as_seq(ql_env);
    while (counted_t<const ql::datum_t> d = ds->next(ql_env)) {
        out->push_back(d);
    }
}

void transform_visitor_t::operator()(const filter_transform_t &transf) const {
    counted_t<ql::func_t> f = transf.filter_func.compile_wire_func();
    counted_t<ql::func_t> default_filter_val = transf.default_filter_val ?
        transf.default_filter_val->compile_wire_func() :
        counted_t<ql::func_t>();
    if (f->filter_call(ql_env, arg, default_filter_val)) {
        out->push_back(arg);
    }
}

void transform_apply(ql::env_t *ql_env,
                     counted_t<const ql::datum_t> json,
                     const rdb_protocol_details::transform_variant_t *t,
                     std::vector<counted_t<const ql::datum_t> > *out) {
    boost::apply_visitor(transform_visitor_t(json, out, ql_env), *t);
}

/* A visitor for applying a terminal to a bit of json. */
class terminal_visitor_t : public boost::static_visitor<void> {
public:
    terminal_visitor_t(lazy_json_t _json,
                       ql::env_t *_ql_env,
                       rget_read_response_t::result_t *_out);

    void operator()(const ql::count_wire_func_t &) const;
    void operator()(const ql::gmr_wire_func_t &) const;
    void operator()(const ql::reduce_wire_func_t &) const;
private:
    lazy_json_t json;
    ql::env_t *ql_env;
    rget_read_response_t::result_t *out;
};

terminal_visitor_t::terminal_visitor_t(lazy_json_t _json,
                                       ql::env_t *_ql_env,
                                       rget_read_response_t::result_t *_out)
    : json(_json), ql_env(_ql_env), out(_out) { }

// All of this logic is analogous to the eager logic in datum_stream.cc.  This
// code duplication needs to go away, but I'm not 100% sure how to do it (there
// are sometimes minor differences between the lazy and eager evaluations) and
// it definitely isn't making it into 1.4.
void terminal_visitor_t::operator()(const ql::gmr_wire_func_t &func) const {
    ql::wire_datum_map_t *obj = boost::get<ql::wire_datum_map_t>(out);
    guarantee(obj);

    counted_t<const ql::datum_t> el = json.get();
    counted_t<const ql::datum_t> el_group
        = func.compile_group()->call(ql_env, el)->as_datum();
    counted_t<const ql::datum_t> elm = json.get();
    counted_t<const ql::datum_t> el_map
        = func.compile_map()->call(ql_env, elm)->as_datum();

    if (!obj->has(el_group)) {
        obj->set(el_group, el_map);
    } else {
        counted_t<const ql::datum_t> lhs = obj->get(el_group);
        obj->set(el_group, func.compile_reduce()->call(ql_env, lhs, el_map)->as_datum());
    }
}

void terminal_visitor_t::operator()(UNUSED const ql::count_wire_func_t &func) const {
    counted_t<const ql::datum_t> d = boost::get<counted_t<const ql::datum_t> >(*out);
    *out = make_counted<const ql::datum_t>(d->as_int() + 1.0);
}

void terminal_visitor_t::operator()(const ql::reduce_wire_func_t &func) const {
    counted_t<const ql::datum_t> *d = boost::get<counted_t<const ql::datum_t> >(out);
    counted_t<const ql::datum_t> rhs = json.get();
    if (d != NULL) {
        *out = func.compile_wire_func()->call(ql_env, *d, rhs)->as_datum();
    } else {
        guarantee(boost::get<rget_read_response_t::empty_t>(out));
        *out = rhs;
    }
}

void terminal_apply(ql::env_t *ql_env,
                    lazy_json_t json,
                    const rdb_protocol_details::terminal_variant_t *t,
                    rget_read_response_t::result_t *out) {
    boost::apply_visitor(terminal_visitor_t(json, ql_env, out), *t);
}


/* A visitor for setting the result type based on a terminal. */
class terminal_initializer_visitor_t : public boost::static_visitor<void> {
public:
    terminal_initializer_visitor_t(rget_read_response_t::result_t *_out)
        : out(_out) { }

    void operator()(const ql::gmr_wire_func_t &f) const {
        counted_t<ql::func_t> group = f.compile_group();
        counted_t<ql::func_t> map = f.compile_map();
        counted_t<ql::func_t> reduce = f.compile_reduce();
        guarantee(group.has() && map.has() && reduce.has());
        *out = ql::wire_datum_map_t();
    }

    void operator()(const ql::count_wire_func_t &) const {
        *out = make_counted<const ql::datum_t>(0.0);
    }

    void operator()(const ql::reduce_wire_func_t &f) const {
        counted_t<ql::func_t> reduce = f.compile_wire_func();
        guarantee(reduce.has());
        *out = rget_read_response_t::empty_t();
    }

private:
    rget_read_response_t::result_t *out;
};

void terminal_initialize(const rdb_protocol_details::terminal_variant_t *t,
                         rget_read_response_t::result_t *out) {
    boost::apply_visitor(terminal_initializer_visitor_t(out), *t);
}



}  // namespace query_language
