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
    transform_visitor_t(counted_t<const ql::datum_t> _arg,
                        std::list<counted_t<const ql::datum_t> > *_out,
                        ql::env_t *_ql_env,
                        const backtrace_t &_backtrace);

    // This is a non-const reference because it caches the compiled function
    void operator()(ql::map_wire_func_t &func) const;  // NOLINT(runtime/references)
    void operator()(ql::filter_wire_func_t &func) const;  // NOLINT(runtime/references)
    void operator()(ql::concatmap_wire_func_t &func) const;  // NOLINT(runtime/references)

private:
    counted_t<const ql::datum_t> arg;
    std::list<counted_t<const ql::datum_t> > *out;
    ql::env_t *ql_env;
    backtrace_t backtrace;
};

transform_visitor_t::transform_visitor_t(counted_t<const ql::datum_t> _arg,
                                         std::list<counted_t<const ql::datum_t> > *_out,
                                         ql::env_t *_ql_env,
                                         const backtrace_t &_backtrace)
    : arg(_arg), out(_out), ql_env(_ql_env),
      backtrace(_backtrace) { }

// All of this logic is analogous to the eager logic in datum_stream.cc.  This
// code duplication needs to go away, but I'm not 100% sure how to do it (there
// are sometimes minor differences between the lazy and eager evaluations) and
// it definitely isn't making it into 1.4.
void transform_visitor_t::operator()(ql::map_wire_func_t &func) const {  // NOLINT(runtime/references)
    out->push_back(func.compile(ql_env)->call(arg)->as_datum());
}

void transform_visitor_t::operator()(ql::concatmap_wire_func_t &func) const {  // NOLINT(runtime/references)
    counted_t<ql::datum_stream_t> ds = func.compile(ql_env)->call(arg)->as_seq();
    while (counted_t<const ql::datum_t> d = ds->next()) {
        out->push_back(d);
    }
}

void transform_visitor_t::operator()(ql::filter_wire_func_t &func) const {  // NOLINT(runtime/references)
    counted_t<ql::func_t> f = func.compile(ql_env);
    if (f->filter_call(arg)) {
        out->push_back(arg);
    }
}


void transform_apply(ql::env_t *ql_env,
                     const backtrace_t &backtrace,
                     std::list<lazy_json_t> &&jsons,
                     rdb_protocol_details::transform_variant_t *t,
                     std::list<counted_t<const ql::datum_t> > *out) {
    std::list<lazy_json_t> locals = std::move(jsons);

    for (auto it = locals.begin(); it != locals.end(); ++it) {
        boost::apply_visitor(transform_visitor_t(it->get(), out, ql_env, backtrace),
                             *t);
    }
}

/* A visitor for applying a terminal to a bit of json. */
class terminal_visitor_t : public boost::static_visitor<void> {
public:
    terminal_visitor_t(std::list<lazy_json_t> &&_jsons,
                       ql::env_t *_ql_env,
                       const backtrace_t &_backtrace,
                       rget_read_response_t::result_t *_out);

    void operator()(const ql::count_wire_func_t &) const;
    // This is a non-const reference because it caches the compiled function
    void operator()(ql::gmr_wire_func_t &) const;
    void operator()(ql::reduce_wire_func_t &) const;
private:
    std::list<lazy_json_t> jsons;
    ql::env_t *ql_env;
    backtrace_t backtrace;
    rget_read_response_t::result_t *out;
};

terminal_visitor_t::terminal_visitor_t(std::list<lazy_json_t> &&_jsons,
                                       ql::env_t *_ql_env,
                                       const backtrace_t &_backtrace,
                                       rget_read_response_t::result_t *_out)
    : jsons(std::move(_jsons)), ql_env(_ql_env), backtrace(_backtrace), out(_out) { }

// All of this logic is analogous to the eager logic in datum_stream.cc.  This
// code duplication needs to go away, but I'm not 100% sure how to do it (there
// are sometimes minor differences between the lazy and eager evaluations) and
// it definitely isn't making it into 1.4.
void terminal_visitor_t::operator()(ql::gmr_wire_func_t &func) const {  // NOLINT(runtime/references)
    for (auto it = jsons.begin(); it != jsons.end(); ++it) {
        counted_t<const ql::datum_t> json = it->get();

        ql::wire_datum_map_t *obj = boost::get<ql::wire_datum_map_t>(out);
        guarantee(obj);

        // RSI: Check that el and elm were the same values before (now we pass the
        // variable `json` in their place).
        counted_t<const ql::datum_t> el_group
            = func.compile_group(ql_env)->call(json)->as_datum();
        counted_t<const ql::datum_t> el_map
            = func.compile_map(ql_env)->call(json)->as_datum();

        if (!obj->has(el_group)) {
            obj->set(el_group, el_map);
        } else {
            counted_t<const ql::datum_t> lhs = obj->get(el_group);
            obj->set(el_group, func.compile_reduce(ql_env)->call(lhs, el_map)->as_datum());
        }
    }
}

void terminal_visitor_t::operator()(UNUSED const ql::count_wire_func_t &func) const {
    counted_t<const ql::datum_t> d = boost::get<counted_t<const ql::datum_t> >(*out);
    *out = make_counted<const ql::datum_t>(d->as_int() + static_cast<double>(jsons.size()));
}

void terminal_visitor_t::operator()(ql::reduce_wire_func_t &func) const {  // NOLINT(runtime/references)
    for (auto it = jsons.begin(); it != jsons.end(); ++it) {
        counted_t<const ql::datum_t> json = it->get();

        counted_t<const ql::datum_t> *d = boost::get<counted_t<const ql::datum_t> >(out);
        if (d != NULL) {
            *out = func.compile(ql_env)->call(*d, json)->as_datum();
        } else {
            guarantee(boost::get<rget_read_response_t::empty_t>(out));
            *out = json;
        }
    }
}

void terminal_apply(ql::env_t *ql_env,
                    const backtrace_t &backtrace,
                    std::list<lazy_json_t> &&jsons,
                    rdb_protocol_details::terminal_variant_t *t,
                    rget_read_response_t::result_t *out) {
    boost::apply_visitor(terminal_visitor_t(std::move(jsons), ql_env, backtrace, out),
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
        *out = make_counted<const ql::datum_t>(0.0);
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
    boost::apply_visitor(
        terminal_initializer_visitor_t(out, ql_env, backtrace),
        *t);
}



}  // namespace query_language
