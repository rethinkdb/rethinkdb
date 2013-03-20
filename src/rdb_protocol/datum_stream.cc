#include "rdb_protocol/datum_stream.hpp"

#include <map>

#include "clustering/administration/metadata.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/val.hpp"

namespace ql {

// DATUM_STREAM_T
counted_t<datum_stream_t> datum_stream_t::slice(size_t l, size_t r) {
    return make_counted<slice_datum_stream_t>(env, l, r, this->counted_from_this());
}
counted_t<datum_stream_t> datum_stream_t::zip() {
    return make_counted<zip_datum_stream_t>(env, this->counted_from_this());
}

counted_t<const datum_t> datum_stream_t::next() {
    env->throw_if_interruptor_pulsed();
    try {
        return next_impl();
    } catch (const datum_exc_t &e) {
        rfail("%s", e.what());
        unreachable();
    }
}

counted_t<const datum_t> eager_datum_stream_t::count() {
    int64_t i = 0;
    for (;;) {
        env_checkpoint_t ect(env, &env_t::discard_checkpoint);
        if (!next()) break;
        ++i;
    }
    return make_counted<const datum_t>(static_cast<double>(i));
}

counted_t<const datum_t> eager_datum_stream_t::reduce(val_t *base_val, func_t *f) {
    counted_t<const datum_t> base;
    base = base_val ? base_val->as_datum() : next();
    rcheck(base, "Cannot reduce over an empty stream with no base.");

    while (counted_t<const datum_t> rhs = next()) {
        base = f->call(base, rhs)->as_datum();
    }
    return base;
}

counted_t<const datum_t> eager_datum_stream_t::gmr(
    func_t *g, func_t *m, counted_t<const datum_t> d, func_t *r) {
    wire_datum_map_t map;
    while (counted_t<const datum_t> el = next()) {
        counted_t<const datum_t> el_group = g->call(el)->as_datum();
        counted_t<const datum_t> el_map = m->call(el)->as_datum();
        if (!map.has(el_group)) {
            map.set(el_group, d ? r->call(d, el_map)->as_datum() : el_map);
        } else {
            map.set(el_group, r->call(map.get(el_group), el_map)->as_datum());
        }
    }
    return map.to_arr();
}

counted_t<datum_stream_t> eager_datum_stream_t::filter(func_t *f) {
    return make_counted<filter_datum_stream_t>(env, f, this->counted_from_this());
}
counted_t<datum_stream_t> eager_datum_stream_t::map(func_t *f) {
    return make_counted<map_datum_stream_t>(env, f, this->counted_from_this());
}
counted_t<datum_stream_t> eager_datum_stream_t::concatmap(func_t *f) {
    return make_counted<concatmap_datum_stream_t>(env, f, this->counted_from_this());
}

counted_t<const datum_t> eager_datum_stream_t::as_array() {
    scoped_ptr_t<datum_t> arr(new datum_t(datum_t::R_ARRAY));
    while (counted_t<const datum_t> d = next()) {
        arr->add(d);
    }
    return counted_t<const datum_t>(arr.release());
}

// LAZY_DATUM_STREAM_T
lazy_datum_stream_t::lazy_datum_stream_t(
    env_t *env, bool use_outdated, namespace_repo_t<rdb_protocol_t>::access_t *ns_access,
    const pb_rcheckable_t *bt_src)
    : datum_stream_t(env, bt_src),
      json_stream(new query_language::batched_rget_stream_t(
                      *ns_access, env->interruptor, key_range_t::universe(),
                      env->get_all_optargs(), use_outdated))
{ }
lazy_datum_stream_t::lazy_datum_stream_t(const lazy_datum_stream_t *src)
    : datum_stream_t(src->env, src), json_stream(src->json_stream), trans(src->trans), terminal(src->terminal), _s(src->_s), _b(src->_b), shard_data(src->shard_data) { }

counted_t<datum_stream_t> lazy_datum_stream_t::map(func_t *f) {
    scoped_ptr_t<lazy_datum_stream_t> out(new lazy_datum_stream_t(this));
    out->trans = rdb_protocol_details::transform_variant_t(map_wire_func_t(env, f));
    out->json_stream = json_stream->add_transformation(out->trans, env, _s, _b);
    return counted_t<datum_stream_t>(out.release());
}
counted_t<datum_stream_t> lazy_datum_stream_t::concatmap(func_t *f) {
    scoped_ptr_t<lazy_datum_stream_t> out(new lazy_datum_stream_t(this));
    out->trans
        = rdb_protocol_details::transform_variant_t(concatmap_wire_func_t(env, f));
    out->json_stream = json_stream->add_transformation(out->trans, env, _s, _b);
    return counted_t<datum_stream_t>(out.release());
}
counted_t<datum_stream_t> lazy_datum_stream_t::filter(func_t *f) {
    scoped_ptr_t<lazy_datum_stream_t> out(new lazy_datum_stream_t(this));
    out->trans = rdb_protocol_details::transform_variant_t(filter_wire_func_t(env, f));
    out->json_stream = json_stream->add_transformation(out->trans, env, _s, _b);
    return counted_t<datum_stream_t>(out.release());
}

// This applies a terminal to the JSON stream, evaluates it, and pulls out the
// shard data.
template<class T>
void lazy_datum_stream_t::run_terminal(T t) {
    terminal = rdb_protocol_details::terminal_variant_t(t);
    rdb_protocol_t::rget_read_response_t::result_t res =
        json_stream->apply_terminal(terminal, env, _s, _b);
    std::vector<wire_datum_t> *data = boost::get<std::vector<wire_datum_t> >(&res);
    r_sanity_check(data);
    for (size_t i = 0; i < data->size(); ++i) {
        shard_data.push_back((*data)[i].compile(env));
    }
}

counted_t<const datum_t> lazy_datum_stream_t::count() {
    scoped_ptr_t<datum_t> d(new datum_t(0.0));
    env_checkpoint_t ect(env, &env_t::discard_checkpoint);
    run_terminal(count_wire_func_t());
    for (size_t i = 0; i < shard_data.size(); ++i) {
        *d = datum_t(d->as_num() + shard_data[i]->as_int());
    }
    return counted_t<const datum_t>(d.release());
}

counted_t<const datum_t> lazy_datum_stream_t::reduce(val_t *base_val, func_t *f) {
    run_terminal(reduce_wire_func_t(env, f));
    counted_t<const datum_t> out;
    if (base_val) {
        out = base_val->as_datum();
    } else {
        rcheck(shard_data.size() > 0,
               "Cannot reduce over an empty stream with no base.");
        out = shard_data[0];
    }
    for (size_t i = !base_val; i < shard_data.size(); ++i) {
        out = f->call(out, shard_data[i])->as_datum();
    }
    return out;
}

counted_t<const datum_t> lazy_datum_stream_t::gmr(
    func_t *g, func_t *m, counted_t<const datum_t> d, func_t *r) {
    terminal = rdb_protocol_details::terminal_variant_t(gmr_wire_func_t(env, g, m, r));
    rdb_protocol_t::rget_read_response_t::result_t res =
        json_stream->apply_terminal(terminal, env, _s, _b);
    typedef std::vector<wire_datum_map_t> wire_datum_maps_t;
    wire_datum_maps_t *dms = boost::get<wire_datum_maps_t>(&res);
    r_sanity_check(dms);
    wire_datum_map_t map;

    env_gc_checkpoint_t egct(env);
    for (size_t i = 0; i < dms->size(); ++i) {
        wire_datum_map_t *rhs = &((*dms)[i]);
        rhs->compile(env);
        counted_t<const datum_t> rhs_arr = rhs->to_arr();
        for (size_t f = 0; f < rhs_arr->size(); ++f) {
            counted_t<const datum_t> key = rhs_arr->el(f)->el("group");
            counted_t<const datum_t> val = rhs_arr->el(f)->el("reduction");
            if (!map.has(key)) {
                map.set(key, d ? r->call(d, val)->as_datum() : val);
            } else {
                map.set(key, r->call(map.get(key), val)->as_datum());
            }
        }
    }
    return egct.finalize(map.to_arr());
}

counted_t<const datum_t> lazy_datum_stream_t::next_impl() {
    boost::shared_ptr<scoped_cJSON_t> json = json_stream->next();
    if (!json.get()) return counted_t<const datum_t>();
    return make_counted<datum_t>(json, env);
}

// ARRAY_DATUM_STREAM_T
array_datum_stream_t::array_datum_stream_t(env_t *env, counted_t<const datum_t> _arr,
                                           const pb_rcheckable_t *bt_src)
    : eager_datum_stream_t(env, bt_src), index(0), arr(_arr) { }

counted_t<const datum_t> array_datum_stream_t::next_impl() {
    return arr->el(index++, NOTHROW);
}

// MAP_DATUM_STREAM_T
counted_t<const datum_t> map_datum_stream_t::next_impl() {
    counted_t<const datum_t> arg = src->next();
    return !arg ? counted_t<const datum_t>() : f->call(arg)->as_datum();
}

// CONCATMAP_DATUM_STREAM_T
counted_t<const datum_t> concatmap_datum_stream_t::next_impl() {
    for (;;) {
        if (!subsrc) {
            counted_t<const datum_t> arg = src->next();
            if (!arg) return counted_t<const datum_t>();
            subsrc = f->call(arg)->as_seq();
        }
        if (counted_t<const datum_t> retval = subsrc->next()) return retval;
        subsrc.reset();
    }
}

counted_t<const datum_t> filter_datum_stream_t::next_impl() {
    counted_t<const datum_t> arg;
    for (;;) {
        env_checkpoint_t outer_checkpoint(env, &env_t::discard_checkpoint);
        if (!(arg = src->next())) return counted_t<const datum_t>();
        env_checkpoint_t inner_checkpoint(env, &env_t::discard_checkpoint);
        if (f->filter_call(env, arg)) {
                outer_checkpoint.reset(&env_t::merge_checkpoint);
                break;
        }
    }
    return arg;
}

// SLICE_DATUM_STREAM_T
slice_datum_stream_t::slice_datum_stream_t(
    env_t *_env, size_t _l, size_t _r, counted_t<datum_stream_t> _src)
    : eager_datum_stream_t(_env, _src.get()), env(_env), ind(0), l(_l), r(_r), src(_src) { }
counted_t<const datum_t> slice_datum_stream_t::next_impl() {
    if (l > r || ind > r) return counted_t<const datum_t>();
    while (ind++ < l) {
        env_checkpoint_t ect(env, &env_t::discard_checkpoint);
        src->next();
    }
    return src->next();
}

// ZIP_DATUM_STREAM_T
zip_datum_stream_t::zip_datum_stream_t(env_t *_env, counted_t<datum_stream_t> _src)
    : eager_datum_stream_t(_env, _src.get()), env(_env), src(_src) { }
counted_t<const datum_t> zip_datum_stream_t::next_impl() {
    counted_t<const datum_t> d = src->next();
    if (!d) return counted_t<const datum_t>();
    counted_t<const datum_t> l = d->el("left", NOTHROW);
    counted_t<const datum_t> r = d->el("right", NOTHROW);
    rcheck(l, "ZIP can only be called on the result of a join.");
    return r ? l->merge(r) : l;
}

// UNION_DATUM_STREAM_T
counted_t<const datum_t> union_datum_stream_t::next_impl() {
    for (; streams_index < streams.size(); ++streams_index) {
        if (counted_t<const datum_t> d = streams[streams_index]->next()) return d;
    }
    return counted_t<const datum_t>();
};

} // namespace ql
