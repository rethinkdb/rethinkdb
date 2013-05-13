#include "rdb_protocol/datum_stream.hpp"

#include <algorithm>
#include <functional>
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
    // This is a hook for unit tests to change things mid-query.
    env->do_unittest_eval_callback();
    std::vector<counted_t<const datum_t> > datums = next_batch(1);
    rassert(datums.size() <= 1);
    return datums.empty() ? counted_t<const datum_t>() : datums[0];
}

std::vector<counted_t<const datum_t> > datum_stream_t::next_batch(size_t max_size) {
    env->throw_if_interruptor_pulsed();

    r_sanity_check(max_size > 0);
    try {
        // next_batch_impl is allowed to assume max_size is positive.
        return next_batch_impl(max_size);
    } catch (const datum_exc_t &e) {
        rfail("%s", e.what());
        unreachable();
    }
}

counted_t<const datum_t> eager_datum_stream_t::count() {
    int64_t i = 0;
    for (;;) {
        size_t n = next_batch(datum_stream_t::RECOMMENDED_MAX_SIZE).size();
        if (n == 0) {
            break;
        }
        i += n;
    }
    return make_counted<datum_t>(static_cast<double>(i));
}

counted_t<const datum_t> eager_datum_stream_t::reduce(counted_t<val_t> base_val,
                                                      counted_t<func_t> f) {
    counted_t<const datum_t> base = base_val.has() ? base_val->as_datum() : next();
    rcheck(base.has(), "Cannot reduce over an empty stream with no base.");

    while (counted_t<const datum_t> rhs = next()) {
        base = f->call(base, rhs)->as_datum();
    }
    return base;
}

counted_t<const datum_t> eager_datum_stream_t::gmr(counted_t<func_t> group,
                                                   counted_t<func_t> map,
                                                   counted_t<const datum_t> base,
                                                   counted_t<func_t> reduce) {
    wire_datum_map_t wd_map;
    while (counted_t<const datum_t> el = next()) {
        counted_t<const datum_t> el_group = group->call(el)->as_datum();
        counted_t<const datum_t> el_map = map->call(el)->as_datum();
        if (!wd_map.has(el_group)) {
            wd_map.set(el_group,
                       base.has() ? reduce->call(base, el_map)->as_datum() : el_map);
        } else {
            wd_map.set(el_group, reduce->call(wd_map.get(el_group), el_map)->as_datum());
        }
    }
    return wd_map.to_arr();
}

counted_t<datum_stream_t> eager_datum_stream_t::filter(counted_t<func_t> f) {
    return make_counted<filter_datum_stream_t>(env, f, this->counted_from_this());
}
counted_t<datum_stream_t> eager_datum_stream_t::map(counted_t<func_t> f) {
    return make_counted<map_datum_stream_t>(env, f, this->counted_from_this());
}
counted_t<datum_stream_t> eager_datum_stream_t::concatmap(counted_t<func_t> f) {
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
    const protob_t<const Backtrace> &bt_src)
    : datum_stream_t(env, bt_src),
      json_stream(new query_language::batched_rget_stream_t(
                      *ns_access, env->interruptor, counted_t<datum_t>(), counted_t<datum_t>(),
                      env->get_all_optargs(), use_outdated))
{ }

lazy_datum_stream_t::lazy_datum_stream_t(
    env_t *env, bool use_outdated, namespace_repo_t<rdb_protocol_t>::access_t *ns_access,
    counted_t<const datum_t> left_bound, counted_t<const datum_t> right_bound,
    const protob_t<const Backtrace> &bt_src)
    : datum_stream_t(env, bt_src),
      json_stream(new query_language::batched_rget_stream_t(
                      *ns_access, env->interruptor, left_bound, right_bound,
                      env->get_all_optargs(), use_outdated))
{ }

lazy_datum_stream_t::lazy_datum_stream_t(
    env_t *env, bool use_outdated, namespace_repo_t<rdb_protocol_t>::access_t *ns_access,
    counted_t<const datum_t> left_bound, counted_t<const datum_t> right_bound,
    const std::string &sindex_id,
    const protob_t<const Backtrace> &bt_src)
    : datum_stream_t(env, bt_src),
      json_stream(new query_language::batched_rget_stream_t(
                      *ns_access, env->interruptor, sindex_id,
                      env->get_all_optargs(), use_outdated, left_bound, right_bound))
{ }

lazy_datum_stream_t::lazy_datum_stream_t(const lazy_datum_stream_t *src)
    : datum_stream_t(src->env, src->backtrace()), json_stream(src->json_stream) { }

counted_t<datum_stream_t> lazy_datum_stream_t::map(counted_t<func_t> f) {
    scoped_ptr_t<lazy_datum_stream_t> out(new lazy_datum_stream_t(this));
    out->json_stream = json_stream->add_transformation(
        rdb_protocol_details::transform_variant_t(map_wire_func_t(env, f)),
        env, query_language::scopes_t(), query_language::backtrace_t());
    return counted_t<datum_stream_t>(out.release());
}

counted_t<datum_stream_t> lazy_datum_stream_t::concatmap(counted_t<func_t> f) {
    scoped_ptr_t<lazy_datum_stream_t> out(new lazy_datum_stream_t(this));
    out->json_stream = json_stream->add_transformation(
        rdb_protocol_details::transform_variant_t(concatmap_wire_func_t(env, f)),
        env, query_language::scopes_t(), query_language::backtrace_t());
    return counted_t<datum_stream_t>(out.release());
}
counted_t<datum_stream_t> lazy_datum_stream_t::filter(counted_t<func_t> f) {
    scoped_ptr_t<lazy_datum_stream_t> out(new lazy_datum_stream_t(this));
    out->json_stream = json_stream->add_transformation(
        rdb_protocol_details::transform_variant_t(filter_wire_func_t(env, f)),
        env, query_language::scopes_t(), query_language::backtrace_t());
    return counted_t<datum_stream_t>(out.release());
}

// This applies a terminal to the JSON stream, evaluates it, and pulls out the
// shard data.
rdb_protocol_t::rget_read_response_t::result_t lazy_datum_stream_t::run_terminal(const rdb_protocol_details::terminal_variant_t &t) {
    return json_stream->apply_terminal(t,
                                       env,
                                       query_language::scopes_t(),
                                       query_language::backtrace_t());
}

counted_t<const datum_t> lazy_datum_stream_t::count() {
    rdb_protocol_t::rget_read_response_t::result_t res = run_terminal(count_wire_func_t());
    wire_datum_t *wire_datum = boost::get<wire_datum_t>(&res);
    r_sanity_check(wire_datum != NULL);
    return wire_datum->compile(env);
}

counted_t<const datum_t> lazy_datum_stream_t::reduce(counted_t<val_t> base_val,
                                                     counted_t<func_t> f) {
    rdb_protocol_t::rget_read_response_t::result_t res
        = run_terminal(reduce_wire_func_t(env, f));

    if (wire_datum_t *wire_datum = boost::get<wire_datum_t>(&res)) {
        counted_t<const datum_t> datum = wire_datum->compile(env);
        if (base_val.has()) {
            return f->call(base_val->as_datum(), datum)->as_datum();
        } else {
            return datum;
        }
    } else {
        r_sanity_check(boost::get<rdb_protocol_t::rget_read_response_t::empty_t>(&res));
        if (base_val.has()) {
            return base_val->as_datum();
        } else {
            rfail("Cannot reduce over an empty stream with no base.");
        }
    }
}

counted_t<const datum_t> lazy_datum_stream_t::gmr(counted_t<func_t> g,
                                                  counted_t<func_t> m,
                                                  counted_t<const datum_t> base,
                                                  counted_t<func_t> r) {
    rdb_protocol_t::rget_read_response_t::result_t res =
        json_stream->apply_terminal(
            rdb_protocol_details::terminal_variant_t(gmr_wire_func_t(env, g, m, r)),
            env, query_language::scopes_t(), query_language::backtrace_t());
    wire_datum_map_t *dm = boost::get<wire_datum_map_t>(&res);
    r_sanity_check(dm);
    dm->compile(env);
    counted_t<const datum_t> dm_arr = dm->to_arr();
    if (!base.has()) {
        return dm_arr;
    } else {
        wire_datum_map_t map;

        for (size_t f = 0; f < dm_arr->size(); ++f) {
            counted_t<const datum_t> key = dm_arr->get(f)->get("group");
            counted_t<const datum_t> val = dm_arr->get(f)->get("reduction");
            r_sanity_check(!map.has(key));
            map.set(key, r->call(base, val)->as_datum());
        }
        return map.to_arr();
    }
}

std::vector<counted_t<const datum_t> >
lazy_datum_stream_t::next_batch_impl(const size_t max_size) {
    std::vector<boost::shared_ptr<scoped_cJSON_t> > jsons
        = json_stream->next_batch(max_size);

    std::vector<counted_t<const datum_t> > ret;
    ret.reserve(jsons.size());
    for (auto it = jsons.begin(); it != jsons.end(); ++it) {
        ret.push_back(make_counted<datum_t>(*it, env));
    }

    return ret;
}

// ARRAY_DATUM_STREAM_T
array_datum_stream_t::array_datum_stream_t(env_t *env, counted_t<const datum_t> _arr,
                                           const protob_t<const Backtrace> &bt_source)
    : eager_datum_stream_t(env, bt_source), index(0), arr(_arr) { }

std::vector<counted_t<const datum_t> >
array_datum_stream_t::next_batch_impl(const size_t max_size) {
    const size_t count = std::min(arr->size() - index, max_size);

    std::vector<counted_t<const datum_t> > ret;
    ret.reserve(count);

    const size_t end = index + count;
    for (; index < end; ++index) {
        ret.push_back(arr->get(index));
    }

    return ret;
}

// MAP_DATUM_STREAM_T
std::vector<counted_t<const datum_t> >
map_datum_stream_t::next_batch_impl(size_t max_size) {
    std::vector<counted_t<const datum_t> > datums = source->next_batch(max_size);
    for (auto it = datums.begin(); it != datums.end(); ++it) {
        *it = f->call(*it)->as_datum();
    }
    return datums;
}

bool should_be_removed(func_t *func, const counted_t<const datum_t> &arg) {
    return !func->filter_call(arg);
}

// FILTER_DATUM_STREAM_T
std::vector<counted_t<const datum_t> >
filter_datum_stream_t::next_batch_impl(size_t max_size) {
    for (;;) {
        std::vector<counted_t<const datum_t> > datums = source->next_batch(max_size);
        if (datums.empty()) {
            return datums;
        }

        datums.erase(std::remove_if(datums.begin(), datums.end(),
                                    boost::bind(should_be_removed, f.get(), _1)),
                     datums.end());

        if (!datums.empty()) {
            return datums;
        }
    }
}

// CONCATMAP_DATUM_STREAM_T
std::vector<counted_t<const datum_t> >
concatmap_datum_stream_t::next_batch_impl(size_t max_size) {
    for (;;) {
        if (!subsource.has()) {
            std::vector<counted_t<const datum_t> > arg = source->next_batch(1);
            if (arg.empty()) {
                return std::vector<counted_t<const datum_t> >();
            }
            subsource = f->call(arg[0])->as_seq();
        }

        std::vector<counted_t<const datum_t> > datums = subsource->next_batch(max_size);
        if (!datums.empty()) {
            return datums;
        }

        subsource.reset();
    }
}

// SLICE_DATUM_STREAM_T
slice_datum_stream_t::slice_datum_stream_t(env_t *env, size_t _left, size_t _right,
                                           counted_t<datum_stream_t> _src)
    : wrapper_datum_stream_t(env, _src), index(0),
      left(_left), right(_right) { }

std::vector<counted_t<const datum_t> >
slice_datum_stream_t::next_batch_impl(const size_t max_size) {
    if (left > right || index > right) {
        return std::vector<counted_t<const datum_t> >();
    }

    while (index < left) {
        // max_size is an indicator of how comfortable we are loading a ton of
        // datums into memory at once, so we don't want a discard batch to be
        // bigger than max_size.
        std::vector<counted_t<const datum_t> > discards
            = source->next_batch(std::min(max_size, left - index));
        if (discards.empty()) {
            return discards;
        }
        index += discards.size();
    }

    // Fucking Christ.  Why the fuck are slices still inclusive?
    std::vector<counted_t<const datum_t> > datums
        = source->next_batch(std::min(right + 1 - index, max_size));

    index += datums.size();

    return datums;
}

// ZIP_DATUM_STREAM_T
zip_datum_stream_t::zip_datum_stream_t(env_t *env, counted_t<datum_stream_t> _src)
    : wrapper_datum_stream_t(env, _src) { }

void zip_datum_stream_t::transform(counted_t<const datum_t> &datum) {  // NOLINT(runtime/references)
    counted_t<const datum_t> left = datum->get("left", NOTHROW);
    counted_t<const datum_t> right = datum->get("right", NOTHROW);
    rcheck(left.has(), "ZIP can only be called on the result of a join.");
    datum = right.has() ? left->merge(right) : left;
}

std::vector<counted_t<const datum_t> >
zip_datum_stream_t::next_batch_impl(size_t max_size) {
    std::vector<counted_t<const datum_t> > datums = source->next_batch(max_size);
    std::for_each(datums.begin(), datums.end(),
                  boost::bind(&zip_datum_stream_t::transform, this, _1));
    return datums;
}


// UNION_DATUM_STREAM_T
std::vector<counted_t<const datum_t> >
union_datum_stream_t::next_batch_impl(size_t max_size) {
    while (streams_index < streams.size()) {
        std::vector<counted_t<const datum_t> > datums
            = streams[streams_index]->next_batch(max_size);
        if (!datums.empty()) {
            return datums;
        }

        streams[streams_index].reset();
        ++streams_index;
    }
    return std::vector<counted_t<const datum_t> >();
}

} // namespace ql
