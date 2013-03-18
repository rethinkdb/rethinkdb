#include "rdb_protocol/datum_stream.hpp"

#include <map>

#include "clustering/administration/metadata.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/val.hpp"

namespace ql {

// DATUM_STREAM_T
datum_stream_t *datum_stream_t::slice(size_t l, size_t r) {
    return env->add_ptr(new slice_datum_stream_t(env, l, r, this));
}
datum_stream_t *datum_stream_t::zip() {
    return env->add_ptr(new zip_datum_stream_t(env, this));
}

const datum_t *datum_stream_t::next() {
    env->throw_if_interruptor_pulsed();
    try {
        return next_impl();
    } catch (const datum_exc_t &e) {
        rfail("%s", e.what());
        unreachable();
    }
}

std::vector<const datum_t *> datum_stream_t::next_batch() {
    env->throw_if_interruptor_pulsed();
    try {
        return next_batch_impl();
    } catch (const datum_exc_t &e) {
        rfail("%s", e.what());
        unreachable();
    }
}

std::vector<const datum_t *> datum_stream_t::next_batch_impl() {
    std::vector<const datum_t *> ret;
    const datum_t *value = next_impl();
    if (value != NULL) {
        ret.push_back(value);
    }
    return ret;
}


const datum_t *eager_datum_stream_t::count() {
    int64_t i = 0;
    for (;;) {
        env_checkpoint_t ect(env, &env_t::discard_checkpoint);
        if (!next()) break;
        ++i;
    }
    return env->add_ptr(new datum_t(static_cast<double>(i)));
}

const datum_t *eager_datum_stream_t::reduce(val_t *base_val, func_t *f) {
    const datum_t *base;
    base = base_val ? base_val->as_datum() : next();
    rcheck(base, "Cannot reduce over an empty stream with no base.");

    env_gc_checkpoint_t egct(env);
    while (const datum_t *rhs = next()){
        base = egct.maybe_gc(f->call(base, rhs)->as_datum());
    }
    return egct.finalize(base);
}

const datum_t *eager_datum_stream_t::gmr(
    func_t *g, func_t *m, const datum_t *d, func_t *r) {
    int i = 0;
    env_gc_checkpoint_t egct(env);
    wire_datum_map_t map;
    while (const datum_t *el = next()) {
        const datum_t *el_group = g->call(el)->as_datum();
        const datum_t *el_map = m->call(el)->as_datum();
        if (!map.has(el_group)) {
            map.set(el_group, d ? r->call(d, el_map)->as_datum() : el_map);
        } else {
            map.set(el_group, r->call(map.get(el_group), el_map)->as_datum());
            // TODO: this is a hack because GCing a `wire_datum_map_t` is
            // expensive.  Need a better way to do this.
            int rounds = 10000 DEBUG_ONLY(/ 5000);
            if (!(++i%rounds)) egct.maybe_gc(map.to_arr(env));
        }
    }
    return egct.finalize(map.to_arr(env));
}

datum_stream_t *eager_datum_stream_t::filter(func_t *f) {
    return env->add_ptr(new filter_datum_stream_t(env, f, this));
}
datum_stream_t *eager_datum_stream_t::map(func_t *f) {
    return env->add_ptr(new map_datum_stream_t(env, f, this));
}
datum_stream_t *eager_datum_stream_t::concatmap(func_t *f) {
    return env->add_ptr(new concatmap_datum_stream_t(env, f, this));
}

const datum_t *eager_datum_stream_t::as_array() {
    datum_t *arr = env->add_ptr(new datum_t(datum_t::R_ARRAY));
    while (const datum_t *d = next()) arr->add(d);
    return arr;
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
    : datum_stream_t(src->env, src) {
    // RSI: WHAT THE FUCK
    *this = *src;
}

datum_stream_t *lazy_datum_stream_t::map(func_t *f) {
    lazy_datum_stream_t *out = env->add_ptr(new lazy_datum_stream_t(this));
    out->trans = rdb_protocol_details::transform_variant_t(map_wire_func_t(env, f));
    out->json_stream = json_stream->add_transformation(out->trans, env, _s, _b);
    return out;
}
datum_stream_t *lazy_datum_stream_t::concatmap(func_t *f) {
    lazy_datum_stream_t *out = env->add_ptr(new lazy_datum_stream_t(this));
    out->trans
        = rdb_protocol_details::transform_variant_t(concatmap_wire_func_t(env, f));
    out->json_stream = json_stream->add_transformation(out->trans, env, _s, _b);
    return out;
}
datum_stream_t *lazy_datum_stream_t::filter(func_t *f) {
    lazy_datum_stream_t *out = env->add_ptr(new lazy_datum_stream_t(this));
    out->trans = rdb_protocol_details::transform_variant_t(filter_wire_func_t(env, f));
    out->json_stream = json_stream->add_transformation(out->trans, env, _s, _b);
    return out;
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

const datum_t *lazy_datum_stream_t::count() {
    datum_t *d = env->add_ptr(new datum_t(0.0));
    env_checkpoint_t ect(env, &env_t::discard_checkpoint);
    run_terminal(count_wire_func_t());
    for (size_t i = 0; i < shard_data.size(); ++i) {
        *d = datum_t(d->as_num() + shard_data[i]->as_int());
    }
    return d;
}

const datum_t *lazy_datum_stream_t::reduce(val_t *base_val, func_t *f) {
    run_terminal(reduce_wire_func_t(env, f));
    const datum_t *out;
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

const datum_t *lazy_datum_stream_t::gmr(
    func_t *g, func_t *m, const datum_t *d, func_t *r) {
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
        const datum_t *rhs_arr = rhs->to_arr(env);
        for (size_t f = 0; f < rhs_arr->size(); ++f) {
            const datum_t *key = rhs_arr->el(f)->el("group");
            const datum_t *val = rhs_arr->el(f)->el("reduction");
            if (!map.has(key)) {
                map.set(key, d ? r->call(d, val)->as_datum() : val);
            } else {
                map.set(key, r->call(map.get(key), val)->as_datum());
            }
        }
    }
    return egct.finalize(map.to_arr(env));
}

const datum_t *lazy_datum_stream_t::next_impl() {
    boost::shared_ptr<scoped_cJSON_t> json = json_stream->next();
    if (!json.get()) return 0;
    return env->add_ptr(new datum_t(json, env));
}

// ARRAY_DATUM_STREAM_T
array_datum_stream_t::array_datum_stream_t(env_t *env, const datum_t *_arr,
                                           const pb_rcheckable_t *backtrace_source)
    : eager_datum_stream_t(env, backtrace_source), index(0), arr(_arr) { }

const datum_t *array_datum_stream_t::next_impl() {
    const size_t old_index = index;
    const datum_t *ret = arr->el(old_index, NOTHROW);
    if (ret) { ++index; }
    return ret;
}

std::vector<const datum_t *> array_datum_stream_t::next_batch_impl() {
    std::vector<const datum_t *> ret;
    for (;;) {
        if (ret.size() == 100) {
            break;
        }

        const datum_t *value = next_impl();
        if (value == NULL) {
            break;
        }

        ret.push_back(value);
    }

    return ret;
}

// MAP_DATUM_STREAM_T
const datum_t *map_datum_stream_t::next_impl() {
    const datum_t *arg = source->next();  // RSI: Should this call ->next or ->next_impl()?
    return arg == NULL ? NULL : f->call(arg)->as_datum();
}

std::vector<const datum_t *> map_datum_stream_t::next_batch_impl() {
    const std::vector<const datum_t *> datums = source->next_batch();
    std::vector<const datum_t *> ret;
    ret.reserve(datums.size());
    for (auto datum = datums.begin(); datum != datums.end(); ++datum) {
        ret.push_back(f->call(*datum)->as_datum());
    }
    return ret;
}

// FILTER_DATUM_STREAM_T
const datum_t *filter_datum_stream_t::next_impl() {
    for (;;) {
        env_checkpoint_t outer_checkpoint(env, &env_t::discard_checkpoint);

        const datum_t *arg = source->next();
        if (arg == NULL) {
            return NULL;
        }
        env_checkpoint_t inner_checkpoint(env, &env_t::discard_checkpoint);
        if (f->filter_call(env, arg)) {
            outer_checkpoint.reset(&env_t::merge_checkpoint);
            return arg;
        }
    }
}

std::vector<const datum_t *> filter_datum_stream_t::next_batch_impl() {
    for (;;) {
        env_checkpoint_t outer_checkpoint(env, &env_t::discard_checkpoint);

        // RSI: How do we free the memory of the values that get filtered away
        // here?  If any values survive the filtering, all of these are under
        // the same checkpoint.
        std::vector<const datum_t *> unfiltered = source->next_batch();

        if (unfiltered.empty()) {
            return unfiltered;
        }

        env_checkpoint_t inner_checkpoint(env, &env_t::discard_checkpoint);

        std::vector<const datum_t *> filtered;
        for (auto it = unfiltered.begin(); it != unfiltered.end(); ++it) {
            if (f->filter_call(env, *it)) {
                filtered.push_back(*it);
            }
        }

        if (!filtered.empty()) {
            outer_checkpoint.reset(&env_t::merge_checkpoint);
            return filtered;
        }
    }
}

// CONCATMAP_DATUM_STREAM_T
const datum_t *concatmap_datum_stream_t::next_impl() {
    for (;;) {
        if (subsource == NULL) {
            const datum_t *arg = source->next();
            if (arg == NULL) {
                return NULL;
            }
            subsource = f->call(arg)->as_seq();
        }
        if (const datum_t *retval = subsource->next()) {
            return retval;
        }
        subsource = NULL;
    }
}

std::vector<const datum_t *> concatmap_datum_stream_t::next_batch_impl() {
    for (;;) {
        if (subsource == NULL) {
            const datum_t *arg = source->next();
            if (arg == NULL) {
                return std::vector<const datum_t *>();
            }
            subsource = f->call(arg)->as_seq();
        }

        std::vector<const datum_t *> batch = subsource->next_batch();
        if (!batch.empty()) {
            return batch;
        }
        subsource = NULL;
    }
}




// SLICE_DATUM_STREAM_T
slice_datum_stream_t::slice_datum_stream_t(env_t *_env, size_t _left, size_t _right,
                                           datum_stream_t *_source)
    : eager_datum_stream_t(_env, _source), env(_env), index(0),
      left(_left), right(_right), source(_source) { }

const datum_t *slice_datum_stream_t::next_impl() {
    if (left > right || index > right) {
        return NULL;
    }

    while (index++ < left) {
        env_checkpoint_t ect(env, &env_t::discard_checkpoint);
        source->next();
    }
    return source->next();
}

std::vector<const datum_t *> slice_datum_stream_t::next_batch_impl() {
    if (left > right || index > right) {
        return std::vector<const datum_t *>();
    }

    // RSI: We get more elements than necessary.  Wouldn't it be nice if we
    // could ask for a size-limited batch?
    std::vector<const datum_t *> batch = source->next_batch();

    // We have to add 1 because we use closed intervals for some insane reason.
    if (batch.size() > right + 1 - index) {
        // RSI: If we truncate the batch, we leak the datums on the right side of the batch.
        batch.resize(right + 1 - index);
    }

    index += batch.size();
    return batch;
}

// ZIP_DATUM_STREAM_T
zip_datum_stream_t::zip_datum_stream_t(env_t *_env, datum_stream_t *_src)
    : eager_datum_stream_t(_env, _src), env(_env), src(_src) { }
const datum_t *zip_datum_stream_t::next_impl() {
    const datum_t *d = src->next();
    if (!d) return 0;
    const datum_t *l = d->el("left", NOTHROW);
    const datum_t *r = d->el("right", NOTHROW);
    rcheck(l, "ZIP can only be called on the result of a join.");
    return r ? env->add_ptr(l->merge(r)) : l;
}

// UNION_DATUM_STREAM_T
const datum_t *union_datum_stream_t::next_impl() {
    for (; streams_index < streams.size(); ++streams_index) {
        if (const datum_t *d = streams[streams_index]->next()) return d;
    }
    return 0;
};

} // namespace ql
