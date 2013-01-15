#include "clustering/administration/metadata.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/val.hpp"

namespace ql {

// DATUM_STREAM_T
datum_stream_t::datum_stream_t(env_t *_env) : env(_env) { guarantee(env); }
datum_stream_t *datum_stream_t::map(func_t *f) {
    return env->add_ptr(new map_datum_stream_t(env, f, this));
}
datum_stream_t *datum_stream_t::concatmap(func_t *f) {
    return env->add_ptr(new concatmap_datum_stream_t(env, f, this));
}
datum_stream_t *datum_stream_t::filter(func_t *f) {
    return env->add_ptr(new filter_datum_stream_t(env, f, this));
}

const datum_t *datum_stream_t::count() {
    int i = 0;
    for (;;) {
        env_checkpointer_t ect(env, &env_t::discard_checkpoint);
        if (!next()) break;
        ++i;
    }
    return env->add_ptr(new datum_t(i));
}

const datum_t *datum_stream_t::reduce(val_t *base_val, func_t *f) {
    const datum_t *base = base_val ? base_val->as_datum() : next();
    rcheck(base, "Cannot reduce over an empty stream with no base.");
    try {
        const datum_t *rhs = base;
        while (rhs) {
            env_checkpointer_t ect(env, &env_t::merge_checkpoint);
            for (int i = 0; i < reduce_gc_rounds; ++i) {
                if (!(rhs = next())) break;
                guarantee(base != rhs);
                base = f->call(base, rhs)->as_datum();
            }
            ect.gc(base);
        }
        return base;
    } catch (exc_t &e) {
        e.backtrace.frames.push_front(reduce_bt_frame);
        throw;
    }
}
datum_stream_t *datum_stream_t::slice(size_t l, size_t r) {
    return env->add_ptr(new slice_datum_stream_t(env, l, r, this));
}

const datum_t *datum_stream_t::as_arr() {
    datum_t *arr = env->add_ptr(new datum_t(datum_t::R_ARRAY));
    while (const datum_t *d = next()) arr->add(d);
    return arr;
}

// LAZY_DATUM_STREAM_T
lazy_datum_stream_t::lazy_datum_stream_t(env_t *env, bool use_outdated,
                               namespace_repo_t<rdb_protocol_t>::access_t *ns_access)
    : datum_stream_t(env),
      json_stream(new query_language::batched_rget_stream_t(
                      *ns_access, env->interruptor, key_range_t::universe(),
                      100, use_outdated))
{ }
lazy_datum_stream_t::lazy_datum_stream_t(lazy_datum_stream_t *src)
    :datum_stream_t(src->env) {
    *this = *src;
}

#define SIMPLE_LAZY_TRANSFORMATION(name)                                                \
datum_stream_t *lazy_datum_stream_t::name(func_t *f) {                                  \
    lazy_datum_stream_t *out = env->add_ptr(new lazy_datum_stream_t(this));             \
    out->trans = rdb_protocol_details::transform_variant_t(name##_wire_func_t(env, f)); \
    out->json_stream = json_stream->add_transformation(out->trans, 0, env, _s, _b);     \
    return out;                                                                         \
}

SIMPLE_LAZY_TRANSFORMATION(map);
SIMPLE_LAZY_TRANSFORMATION(concatmap);
SIMPLE_LAZY_TRANSFORMATION(filter);
#undef SIMPLE_LAZY_TRANSFORMATION

template<class T>
void lazy_datum_stream_t::run_terminal(T t) {
    terminal = rdb_protocol_details::terminal_variant_t(t);
    rdb_protocol_t::rget_read_response_t::result_t res =
        json_stream->apply_terminal(terminal, 0, env, _s, _b);
    std::vector<boost::shared_ptr<scoped_cJSON_t> > *vec =
        boost::get<rdb_protocol_t::rget_read_response_t::vec_t>(&res);
    r_sanity_check(vec);
    for (size_t i = 0; i < vec->size(); ++i) {
        shard_data.push_back(env->add_ptr(new datum_t((*vec)[i], env)));
    }
}

const datum_t *lazy_datum_stream_t::count() {
    datum_t *d = env->add_ptr(new datum_t(0));
    env_checkpointer_t ect(env, &env_t::discard_checkpoint);
    run_terminal(count_wire_func_t());
    for (size_t i = 0; i < shard_data.size(); ++i) {
        *d = datum_t(d->as_int() + shard_data[i]->as_int());
    }
    return d;
}

const datum_t *lazy_datum_stream_t::reduce(val_t *base_val, func_t *f) {
    try {
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
    } catch (exc_t &e) {
        e.backtrace.frames.push_front(reduce_bt_frame);
        throw;
    }
}

const datum_t *lazy_datum_stream_t::next() {
    boost::shared_ptr<scoped_cJSON_t> json = json_stream->next();
    if (!json.get()) return 0;
    return env->add_ptr(new datum_t(json, env));
}

// ARRAY_DATUM_STREAM_T
array_datum_stream_t::array_datum_stream_t(env_t *env, const datum_t *_arr) :
    datum_stream_t(env), index(0), arr(_arr) { }

const datum_t *array_datum_stream_t::next() {
    return arr->el(index++, false);
    //                      ^^^^^ return 0 instead of throwing on error
}

// MAP_DATUM_STREAM_T
const datum_t *map_datum_stream_t::next() {
    try {
        const datum_t *arg = src->next();
        if (!arg) return 0;
        return f->call(arg)->as_datum();
    } catch (exc_t &e) {
        e.backtrace.frames.push_front(map_bt_frame);
        throw;
    }
}

// CONCATMAP_DATUM_STREAM_T
const datum_t *concatmap_datum_stream_t::next() {
    try {
        while (!arr || index >= arr->size()) {
            const datum_t *arg = src->next();
            if (!arg) return 0;
            arr = f->call(arg)->as_datum();
            index = 0;
        }
        return arr->el(index++);
    } catch (exc_t &e) {
        e.backtrace.frames.push_front(concatmap_bt_frame);
        throw;
    }
}

// FILTER_DATUM_STREAM_T
const datum_t *filter_datum_stream_t::next() {
    try {
        const datum_t *arg = 0;
        for (;;) {
            env_checkpointer_t outer_checkpoint(env, &env_t::discard_checkpoint);
            if (!(arg = src->next())) return 0;
            env_checkpointer_t inner_checkpoint(env, &env_t::discard_checkpoint);
            if (f->call(arg)->as_datum()->as_bool()) {
                outer_checkpoint.reset(&env_t::merge_checkpoint);
                break;
            }
        }
        return arg;
    } catch (exc_t &e) {
        e.backtrace.frames.push_front(filter_bt_frame);
        throw;
    }
}

// SLICE_DATUM_STREAM_T
slice_datum_stream_t::slice_datum_stream_t(
    env_t *_env, size_t _l, size_t _r, datum_stream_t *_src)
    : datum_stream_t(_env), env(_env), ind(0), l(_l), r(_r), src(_src) { }
const datum_t *slice_datum_stream_t::next() {
    if (ind > r) return 0;
    while (ind++ < l) {
        env_checkpointer_t ect(env, &env_t::discard_checkpoint);
        src->next();
    }
    return src->next();
}

// UNION_DATUM_STREAM_T
const datum_t *union_datum_stream_t::next() {
    for (; streams_index < streams.size(); ++streams_index) {
        if (const datum_t *d = streams[streams_index]->next()) return d;
    }
    return 0;
};

} // namespace ql
