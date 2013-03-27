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
    DEBUG_ONLY_CODE(env->do_eval_callback()); // This is a hook for unit tests to change things mid-query
    env->throw_if_interruptor_pulsed();
    try {
        // We loop until we get a value.
        for (;;) {
            const datum_t *ret = NULL;
            const batch_info_t res = next_impl(&ret);
            if (ret != NULL || res == END_OF_STREAM) {
                return ret;
            }
        }

    } catch (const datum_exc_t &e) {
        rfail("%s", e.what());
        unreachable();
    }
}

std::vector<const datum_t *> datum_stream_t::next_batch() {
    env->throw_if_interruptor_pulsed();
    try {
        std::vector<const datum_t *> batch;
        for (;;) {
            const datum_t *datum = NULL;
            const batch_info_t res = next_impl(&datum);
            if (datum != NULL) {
                batch.push_back(datum);
            }
            if (res == LAST_OF_BATCH || res == END_OF_STREAM || batch.size() == MAX_BATCH_SIZE) {
                return batch;
            }
        }
    } catch (const datum_exc_t &e) {
        rfail("%s", e.what());
        unreachable();
    }
}

batch_info_t datum_stream_t::next_with_batch_info(const datum_t **datum_out) {
    env->throw_if_interruptor_pulsed();
    try {
        return next_impl(datum_out);
    } catch (const datum_exc_t &e) {
        rfail("%s", e.what());
        unreachable();
    }
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
void lazy_datum_stream_t::run_terminal(const rdb_protocol_details::terminal_variant_t &t) {
    terminal = t;
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

batch_info_t lazy_datum_stream_t::next_impl(const datum_t **datum_out) {
    boost::shared_ptr<scoped_cJSON_t> json;
    const batch_info_t res = json_stream->next_with_batch_info(&json);

    *datum_out = json ? env->add_ptr(new datum_t(json, env)) : NULL;
    return res;
}

// ARRAY_DATUM_STREAM_T
array_datum_stream_t::array_datum_stream_t(env_t *env, const datum_t *_arr,
                                           const pb_rcheckable_t *backtrace_source)
    : eager_datum_stream_t(env, backtrace_source), index(0), arr(_arr) { }

batch_info_t array_datum_stream_t::next_impl(const datum_t **datum_out) {
    const datum_t *datum = arr->el(index, NOTHROW);
    if (datum == NULL) {
        *datum_out = datum;
        return END_OF_STREAM;
    } else {
        *datum_out = datum;
        ++index;
        return index == arr->size() ? LAST_OF_BATCH : MID_BATCH;
    }
}

// MAP_DATUM_STREAM_T
batch_info_t map_datum_stream_t::next_impl(const datum_t **datum_out) {
    const datum_t *arg = NULL;
    const batch_info_t res = source->next_with_batch_info(&arg);
    if (arg == NULL) {
        *datum_out = NULL;
    } else {
        *datum_out = f->call(arg)->as_datum();
    }
    return res;

}

// FILTER_DATUM_STREAM_T
batch_info_t filter_datum_stream_t::next_impl(const datum_t **datum_out) {
    for (;;) {
        env_checkpoint_t outer_checkpoint(env, &env_t::discard_checkpoint);

        const datum_t *arg = NULL;
        const batch_info_t res = source->next_with_batch_info(&arg);

        env_checkpoint_t inner_checkpoint(env, &env_t::discard_checkpoint);

        if (arg != NULL) {
            if (f->filter_call(arg)) {
                outer_checkpoint.reset(&env_t::merge_checkpoint);
                *datum_out = arg;
                return res;
            }
        }

        if (res != MID_BATCH) {
            *datum_out = NULL;
            return res;
        }
    }
}

// CONCATMAP_DATUM_STREAM_T
batch_info_t concatmap_datum_stream_t::next_impl(const datum_t **datum_out) {
    for (;;) {
        if (subsource == NULL) {
            const datum_t *arg = source->next();
            if (arg == NULL) {
                *datum_out = NULL;
                return END_OF_STREAM;
            }
            subsource = f->call(arg)->as_seq();
        }

        const datum_t *retval = NULL;
        const batch_info_t res = subsource->next_with_batch_info(&retval);
        if (res != END_OF_STREAM) {
            *datum_out = retval;
            return res;
        }

        subsource = NULL;
    }
}

// SLICE_DATUM_STREAM_T
slice_datum_stream_t::slice_datum_stream_t(env_t *_env, size_t _left, size_t _right,
                                           datum_stream_t *_source)
    : eager_datum_stream_t(_env, _source), env(_env), index(0),
      left(_left), right(_right), source(_source) { }

batch_info_t slice_datum_stream_t::next_impl(const datum_t **datum_out) {
    if (left > right || index > right) {
        *datum_out = NULL;
        return END_OF_STREAM;
    }

    while (index < left) {
        env_checkpoint_t ect(env, &env_t::discard_checkpoint);
        const datum_t *discard = NULL;
        const batch_info_t res = source->next_with_batch_info(&discard);
        if (res == END_OF_STREAM) {
            *datum_out = NULL;
            return END_OF_STREAM;
        }
        if (discard != NULL) {
            ++index;
        }
    }

    const datum_t *datum = NULL;
    const batch_info_t res = source->next_with_batch_info(&datum);
    if (datum != NULL) {
        ++index;
    }
    *datum_out = datum;
    return res;
}

// ZIP_DATUM_STREAM_T
zip_datum_stream_t::zip_datum_stream_t(env_t *_env, datum_stream_t *_source)
    : eager_datum_stream_t(_env, _source), env(_env), source(_source) { }

batch_info_t zip_datum_stream_t::next_impl(const datum_t **datum_out) {
    const datum_t *datum = NULL;
    const batch_info_t res = source->next_with_batch_info(&datum);

    if (datum != NULL) {
        const datum_t *left = datum->el("left", NOTHROW);
        const datum_t *right = datum->el("right", NOTHROW);
        rcheck(left != NULL, "ZIP can only be called on the result of a join.");
        datum = right != NULL ? env->add_ptr(left->merge(right)) : left;
    }

    *datum_out = datum;
    return res;
}


// UNION_DATUM_STREAM_T
batch_info_t union_datum_stream_t::next_impl(const datum_t **datum_out) {
    for (; streams_index < streams.size(); ++streams_index) {
        const datum_t *datum = NULL;
        const batch_info_t res = streams[streams_index]->next_with_batch_info(&datum);
        if (res != END_OF_STREAM) {
            *datum_out = datum;
            return res;
        }
    }

    *datum_out = NULL;
    return END_OF_STREAM;
}


} // namespace ql
