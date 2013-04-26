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
    // This is a hook for unit tests to change things mid-query.
    DEBUG_ONLY_CODE(env->do_eval_callback());
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
        std::vector<const datum_t *> batch;
        for (;;) {
            const datum_t *datum = next_impl();
            if (datum != NULL) {
                batch.push_back(datum);
            }
            if (datum == NULL || batch.size() == MAX_BATCH_SIZE) {
                return batch;
            }
        }
    } catch (const datum_exc_t &e) {
        rfail("%s", e.what());
        unreachable();
    }
}

const datum_t *eager_datum_stream_t::count() {
    int64_t i = 0;
    for (;;) {
        env_checkpoint_t ect(env, env_checkpoint_t::DISCARD);
        if (!next()) break;
        ++i;
    }
    return env->add_ptr(new datum_t(static_cast<double>(i)));
}

const datum_t *eager_datum_stream_t::reduce(val_t *base_val, func_t *f) {
    const datum_t *base;
    base = base_val ? base_val->as_datum() : next();
    rcheck(base, "Cannot reduce over an empty stream with no base.");

    env_gc_checkpoint_t gc_checkpoint(env);
    while (const datum_t *rhs = next()){
        base = gc_checkpoint.maybe_gc(f->call(base, rhs)->as_datum());
    }
    return gc_checkpoint.finalize(base);
}

const datum_t *eager_datum_stream_t::gmr(
    func_t *group, func_t *map, const datum_t *base, func_t *reduce) {
    int i = 0;
    env_gc_checkpoint_t gc_checkpoint(env);
    wire_datum_map_t wd_map;
    while (const datum_t *el = next()) {
        const datum_t *el_group = group->call(el)->as_datum();
        const datum_t *el_map = map->call(el)->as_datum();
        if (!wd_map.has(el_group)) {
            wd_map.set(el_group, base ? reduce->call(base, el_map)->as_datum() : el_map);
        } else {
            wd_map.set(el_group, reduce->call(wd_map.get(el_group), el_map)->as_datum());
            // TODO: this is a hack because GCing a `wire_datum_map_t` is
            // expensive.  Need a better way to do this.
            if (++i % WIRE_DATUM_MAP_GC_ROUNDS == 0) {
                gc_checkpoint.maybe_gc(wd_map.to_arr(env));
            }
        }
    }
    return gc_checkpoint.finalize(wd_map.to_arr(env));
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

lazy_datum_stream_t::lazy_datum_stream_t(
    env_t *env, bool use_outdated, namespace_repo_t<rdb_protocol_t>::access_t *ns_access,
    const datum_t *pval, const std::string &sindex_id,
    const pb_rcheckable_t *bt_src)
    : datum_stream_t(env, bt_src),
      json_stream(new query_language::batched_rget_stream_t(
                      *ns_access, env->interruptor, sindex_id,
                      env->get_all_optargs(), use_outdated, pval, pval))
{ }

lazy_datum_stream_t::lazy_datum_stream_t(const lazy_datum_stream_t *src)
    : datum_stream_t(src->env, src) {
    *this = *src;
}

datum_stream_t *lazy_datum_stream_t::map(func_t *f) {
    lazy_datum_stream_t *out = env->add_ptr(new lazy_datum_stream_t(this));
    out->json_stream = json_stream->add_transformation(
        rdb_protocol_details::transform_variant_t(map_wire_func_t(env, f)),
        env, query_language::scopes_t(), query_language::backtrace_t());
    return out;
}
datum_stream_t *lazy_datum_stream_t::concatmap(func_t *f) {
    lazy_datum_stream_t *out = env->add_ptr(new lazy_datum_stream_t(this));
    out->json_stream = json_stream->add_transformation(
        rdb_protocol_details::transform_variant_t(concatmap_wire_func_t(env, f)),
        env, query_language::scopes_t(), query_language::backtrace_t());
    return out;
}
datum_stream_t *lazy_datum_stream_t::filter(func_t *f) {
    lazy_datum_stream_t *out = env->add_ptr(new lazy_datum_stream_t(this));
    out->json_stream = json_stream->add_transformation(
        rdb_protocol_details::transform_variant_t(filter_wire_func_t(env, f)),
        env, query_language::scopes_t(), query_language::backtrace_t());
    return out;
}

// This applies a terminal to the JSON stream, evaluates it, and pulls out the
// shard data.
rdb_protocol_t::rget_read_response_t::result_t lazy_datum_stream_t::run_terminal(const rdb_protocol_details::terminal_variant_t &t) {
    return json_stream->apply_terminal(t,
                                       env,
                                       query_language::scopes_t(),
                                       query_language::backtrace_t());
}

const datum_t *lazy_datum_stream_t::count() {
    rdb_protocol_t::rget_read_response_t::result_t res = run_terminal(count_wire_func_t());
    auto wire_datum = boost::get<wire_datum_t>(&res);
    r_sanity_check(wire_datum);
    return wire_datum->compile(env);
}

const datum_t *lazy_datum_stream_t::reduce(val_t *base_val, func_t *f) {
    rdb_protocol_t::rget_read_response_t::result_t res =
        run_terminal(reduce_wire_func_t(env, f));

    if (auto wire_datum = boost::get<wire_datum_t>(&res)) {
        const datum_t *datum = wire_datum->compile(env);
        if (base_val) {
            return f->call(base_val->as_datum(), datum)->as_datum();
        } else {
            return datum;
        }
    } else {
        r_sanity_check(boost::get<rdb_protocol_t::rget_read_response_t::empty_t>(&res));
        if (base_val) {
            return base_val->as_datum();
        } else {
            rfail("Cannot reduce over an empty stream with no base.");
        }
    }
}

const datum_t *lazy_datum_stream_t::gmr(
    func_t *g, func_t *m, const datum_t *base, func_t *r) {
    rdb_protocol_t::rget_read_response_t::result_t res =
        json_stream->apply_terminal(
            rdb_protocol_details::terminal_variant_t(gmr_wire_func_t(env, g, m, r)),
            env, query_language::scopes_t(), query_language::backtrace_t());
    wire_datum_map_t *dm = boost::get<wire_datum_map_t>(&res);
    r_sanity_check(dm);
    env_gc_checkpoint_t gc_checkpoint(env);
    dm->compile(env);
    const datum_t *dm_arr = dm->to_arr(env);
    if (!base) {
        return gc_checkpoint.finalize(dm_arr);
    } else {
        wire_datum_map_t map;

        for (size_t f = 0; f < dm_arr->size(); ++f) {
            const datum_t *key = dm_arr->get(f)->get("group");
            const datum_t *val = dm_arr->get(f)->get("reduction");
            r_sanity_check(!map.has(key));
            map.set(key, r->call(base, val)->as_datum());
        }
        return gc_checkpoint.finalize(map.to_arr(env));
    }
}

const datum_t *lazy_datum_stream_t::next_impl() {
    boost::shared_ptr<scoped_cJSON_t> json = json_stream->next();
    return json ? env->add_ptr(new datum_t(json, env)) : NULL;
}

// ARRAY_DATUM_STREAM_T
array_datum_stream_t::array_datum_stream_t(env_t *env, const datum_t *_arr,
                                           const pb_rcheckable_t *backtrace_source)
    : eager_datum_stream_t(env, backtrace_source), index(0), arr(_arr) { }

const datum_t *array_datum_stream_t::next_impl() {
    const datum_t *datum = arr->get(index, NOTHROW);
    if (datum == NULL) {
        return NULL;
    } else {
        ++index;
        return datum;
    }
}

// MAP_DATUM_STREAM_T
const datum_t *map_datum_stream_t::next_impl() {
    const datum_t *arg = source->next();
    if (arg == NULL) {
        return NULL;
    } else {
        return f->call(arg)->as_datum();
    }
}

// FILTER_DATUM_STREAM_T
const datum_t *filter_datum_stream_t::next_impl() {
    for (;;) {
        env_checkpoint_t outer_checkpoint(env, env_checkpoint_t::DISCARD);

        const datum_t *arg = source->next();

        if (arg == NULL) {
            return NULL;
        }

        env_checkpoint_t inner_checkpoint(env, env_checkpoint_t::DISCARD);

        if (f->filter_call(arg)) {
            outer_checkpoint.reset(env_checkpoint_t::MERGE);
            return arg;
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

        const datum_t *datum = subsource->next();
        if (datum != NULL) {
            return datum;
        }

        subsource = NULL;
    }
}

// SLICE_DATUM_STREAM_T
slice_datum_stream_t::slice_datum_stream_t(env_t *_env, size_t _left, size_t _right,
                                           datum_stream_t *_src)
    : wrapper_datum_stream_t(_env, _src), env(_env), index(0),
      left(_left), right(_right) { }

const datum_t *slice_datum_stream_t::next_impl() {
    if (left > right || index > right) {
        return NULL;
    }

    while (index < left) {
        env_checkpoint_t ect(env, env_checkpoint_t::DISCARD);
        const datum_t *discard = src_stream()->next();
        if (discard == NULL) {
            return NULL;
        }
        ++index;
    }

    const datum_t *datum = src_stream()->next();
    if (datum != NULL) {
        ++index;
    }
    return datum;
}

// ZIP_DATUM_STREAM_T
zip_datum_stream_t::zip_datum_stream_t(env_t *_env, datum_stream_t *_src)
    : wrapper_datum_stream_t(_env, _src), env(_env) { }

const datum_t *zip_datum_stream_t::next_impl() {
    const datum_t *datum = src_stream()->next();
    if (datum == NULL) {
        return NULL;
    }

    const datum_t *left = datum->get("left", NOTHROW);
    const datum_t *right = datum->get("right", NOTHROW);
    rcheck(left != NULL, "ZIP can only be called on the result of a join.");
    return right != NULL ? env->add_ptr(left->merge(right)) : left;
}


// UNION_DATUM_STREAM_T
const datum_t *union_datum_stream_t::next_impl() {
    for (; streams_index < streams.size(); ++streams_index) {
        const datum_t *datum = streams[streams_index]->next();
        if (datum != NULL) {
            return datum;
        }
    }

    return NULL;
}


} // namespace ql
