#include "rdb_protocol/datum_stream.hpp"

#include <map>

#include "clustering/administration/metadata.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/val.hpp"

namespace ql {

const char *const empty_stream_msg =
    "Cannot reduce over an empty stream with no base.";


// DATUM_STREAM_T
counted_t<datum_stream_t> datum_stream_t::slice(size_t l, size_t r) {
    return make_counted<slice_datum_stream_t>(env, l, r, this->counted_from_this());
}
counted_t<datum_stream_t> datum_stream_t::zip() {
    return make_counted<zip_datum_stream_t>(env, this->counted_from_this());
}
counted_t<datum_stream_t> datum_stream_t::indexes_of(counted_t<func_t> f) {
    return make_counted<indexes_of_datum_stream_t>(env, f, counted_from_this());
}

counted_t<const datum_t> datum_stream_t::next() {
    // This is a hook for unit tests to change things mid-query.
    DEBUG_ONLY_CODE(env->do_eval_callback());
    env->throw_if_interruptor_pulsed();
    try {
        return next_impl();
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
        unreachable();
    }
}

std::vector<counted_t<const datum_t> > datum_stream_t::next_batch() {
    env->throw_if_interruptor_pulsed();
    try {
        std::vector<counted_t<const datum_t> > batch;
        for (;;) {
            counted_t<const datum_t> datum = next_impl();
            if (datum.has()) {
                batch.push_back(datum);
            }
            if (!datum.has() || batch.size() == MAX_BATCH_SIZE) {
                return batch;
            }
        }
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
        unreachable();
    }
}

hinted_datum_t datum_stream_t::sorting_hint_next() {
    return std::make_pair(query_language::CONTINUE, next());
}

counted_t<const datum_t> eager_datum_stream_t::count() {
    int64_t i = 0;
    for (;;) {
        counted_t<const datum_t> value = next();
        if (!value.has()) { break; }
        ++i;
    }
    return make_counted<datum_t>(static_cast<double>(i));
}

counted_t<const datum_t> eager_datum_stream_t::reduce(counted_t<val_t> base_val,
                                                      counted_t<func_t> f) {
    counted_t<const datum_t> base = base_val.has() ? base_val->as_datum() : next();
    rcheck(base.has(), base_exc_t::NON_EXISTENCE, empty_stream_msg);

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
    datum_ptr_t arr(datum_t::R_ARRAY);
    while (counted_t<const datum_t> d = next()) {
        arr.add(d);
    }
    return arr.to_counted();
}

// LAZY_DATUM_STREAM_T
lazy_datum_stream_t::lazy_datum_stream_t(
    env_t *env, bool use_outdated, namespace_repo_t<rdb_protocol_t>::access_t *ns_access,
    sorting_t sorting, const protob_t<const Backtrace> &bt_src)
    : datum_stream_t(env, bt_src),
      json_stream(new query_language::batched_rget_stream_t(
                      *ns_access, env->interruptor,
                      counted_t<datum_t>(), false, counted_t<const datum_t>(), false,
                      env->get_all_optargs(), use_outdated, sorting, this))
{ }

lazy_datum_stream_t::lazy_datum_stream_t(
        env_t *env, bool use_outdated,
        namespace_repo_t<rdb_protocol_t>::access_t *ns_access,
        const std::string &sindex_id, sorting_t sorting,
        const protob_t<const Backtrace> &bt_src)
    : datum_stream_t(env, bt_src),
      json_stream(new query_language::batched_rget_stream_t(
                      *ns_access, env->interruptor, sindex_id,
                      counted_t<datum_t>(), false, counted_t<datum_t>(), false,
                      env->get_all_optargs(), use_outdated,
                      sorting, this))
{ }

lazy_datum_stream_t::lazy_datum_stream_t(
    env_t *env, bool use_outdated, namespace_repo_t<rdb_protocol_t>::access_t *ns_access,
    counted_t<const datum_t> left_bound, bool left_bound_open,
    counted_t<const datum_t> right_bound, bool right_bound_open,
    sorting_t sorting, const protob_t<const Backtrace> &bt_src)
    : datum_stream_t(env, bt_src),
      json_stream(new query_language::batched_rget_stream_t(
                      *ns_access, env->interruptor,
                      left_bound, left_bound_open, right_bound, right_bound_open,
                      env->get_all_optargs(), use_outdated, sorting,
                      this))
{ }

lazy_datum_stream_t::lazy_datum_stream_t(
    env_t *env, bool use_outdated, namespace_repo_t<rdb_protocol_t>::access_t *ns_access,
    counted_t<const datum_t> left_bound, bool left_bound_open,
    counted_t<const datum_t> right_bound, bool right_bound_open,
    const std::string &sindex_id, sorting_t sorting,
    const protob_t<const Backtrace> &bt_src)
    : datum_stream_t(env, bt_src),
      json_stream(new query_language::batched_rget_stream_t(
                      *ns_access, env->interruptor, sindex_id,
                      left_bound, left_bound_open, right_bound, right_bound_open,
                      env->get_all_optargs(), use_outdated, sorting, this))
{ }

lazy_datum_stream_t::lazy_datum_stream_t(const lazy_datum_stream_t *src)
    : datum_stream_t(src->env, src->backtrace()), json_stream(src->json_stream) { }

counted_t<datum_stream_t> lazy_datum_stream_t::map(counted_t<func_t> f) {
    scoped_ptr_t<lazy_datum_stream_t> out(new lazy_datum_stream_t(this));
    out->json_stream = json_stream->add_transformation(
        rdb_protocol_details::transform_variant_t(map_wire_func_t(env, f)),
        env, query_language::backtrace_t());
    return counted_t<datum_stream_t>(out.release());
}

counted_t<datum_stream_t> lazy_datum_stream_t::concatmap(counted_t<func_t> f) {
    scoped_ptr_t<lazy_datum_stream_t> out(new lazy_datum_stream_t(this));
    out->json_stream = json_stream->add_transformation(
        rdb_protocol_details::transform_variant_t(concatmap_wire_func_t(env, f)),
        env, query_language::backtrace_t());
    return counted_t<datum_stream_t>(out.release());
}
counted_t<datum_stream_t> lazy_datum_stream_t::filter(counted_t<func_t> f) {
    scoped_ptr_t<lazy_datum_stream_t> out(new lazy_datum_stream_t(this));
    out->json_stream = json_stream->add_transformation(
        rdb_protocol_details::transform_variant_t(filter_wire_func_t(env, f)),
        env, query_language::backtrace_t());
    return counted_t<datum_stream_t>(out.release());
}

// This applies a terminal to the JSON stream, evaluates it, and pulls out the
// shard data.
rdb_protocol_t::rget_read_response_t::result_t lazy_datum_stream_t::run_terminal(
    const rdb_protocol_details::terminal_variant_t &t) {
    return json_stream->apply_terminal(
        t, env, query_language::backtrace_t());
}

counted_t<const datum_t> lazy_datum_stream_t::count() {
    rdb_protocol_t::rget_read_response_t::result_t res =
        run_terminal(count_wire_func_t());
    return boost::get<counted_t<const datum_t> >(res);
}

counted_t<const datum_t> lazy_datum_stream_t::reduce(counted_t<val_t> base_val,
                                                     counted_t<func_t> f) {
    rdb_protocol_t::rget_read_response_t::result_t res
        = run_terminal(reduce_wire_func_t(env, f));

    if (counted_t<const datum_t> *d = boost::get<counted_t<const datum_t> >(&res)) {
        counted_t<const datum_t> datum = *d;
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
            rfail(base_exc_t::NON_EXISTENCE, empty_stream_msg);
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
            env, query_language::backtrace_t());
    wire_datum_map_t *dm = boost::get<wire_datum_map_t>(&res);
    r_sanity_check(dm);
    dm->compile();
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

hinted_datum_t lazy_datum_stream_t::sorting_hint_next() {
    query_language::hinted_json_t hinted_json = json_stream->sorting_hint_next();
    std::shared_ptr<const scoped_cJSON_t> json = hinted_json.second;
    return std::make_pair(hinted_json.first, 
            json ? make_counted<datum_t>(json) : counted_t<datum_t>());
}

counted_t<const datum_t> lazy_datum_stream_t::next_impl() {
    std::shared_ptr<const scoped_cJSON_t> json = json_stream->next();
    return json ? make_counted<const datum_t>(json) : counted_t<const datum_t>();
}

// ARRAY_DATUM_STREAM_T
array_datum_stream_t::array_datum_stream_t(env_t *env, counted_t<const datum_t> _arr,
                                           const protob_t<const Backtrace> &bt_source)
    : eager_datum_stream_t(env, bt_source), index(0), arr(_arr) { }

counted_t<const datum_t> array_datum_stream_t::next_impl() {
    counted_t<const datum_t> datum = arr->get(index, NOTHROW);
    if (!datum.has()) {
        return counted_t<const datum_t>();
    } else {
        ++index;
        return datum;
    }
}

// MAP_DATUM_STREAM_T
map_datum_stream_t::map_datum_stream_t(env_t *env, counted_t<func_t> _f,
                                       counted_t<datum_stream_t> _source)
    : eager_datum_stream_t(env, _source->backtrace()), f(_f), source(_source) {
    guarantee(f.has() && source.has());
}

counted_t<const datum_t> map_datum_stream_t::next_impl() {
    counted_t<const datum_t> arg = source->next();
    if (!arg.has()) {
        return counted_t<const datum_t>();
    } else {
        return f->call(arg)->as_datum();
    }
}

// INDEXES_OF_DATUM_STREAM_T
indexes_of_datum_stream_t::indexes_of_datum_stream_t(env_t *env, counted_t<func_t> _f,
                                                     counted_t<datum_stream_t> _source)
    : eager_datum_stream_t(env, _source->backtrace()), f(_f), source(_source), index(0) {
    guarantee(f.has() && source.has());
}

counted_t<const datum_t> indexes_of_datum_stream_t::next_impl() {
    for (;;) {
        counted_t<const datum_t> arg = source->next();
        if (!arg.has()) {
            return counted_t<const datum_t>();
        } else if (f->filter_call(arg)) {
            return make_counted<datum_t>(static_cast<double>(index++));
        } else {
            index++;
        }
    }
}

// FILTER_DATUM_STREAM_T
filter_datum_stream_t::filter_datum_stream_t(env_t *env, counted_t<func_t> _f,
                                             counted_t<datum_stream_t> _source)
    : eager_datum_stream_t(env, _source->backtrace()), f(_f), source(_source) {
    guarantee(f.has() && source.has());
}

counted_t<const datum_t> filter_datum_stream_t::next_impl() {
    for (;;) {
        counted_t<const datum_t> arg = source->next();

        if (!arg.has()) {
            return counted_t<const datum_t>();
        }

        if (f->filter_call(arg)) {
            return arg;
        }
    }
}

// CONCATMAP_DATUM_STREAM_T
concatmap_datum_stream_t::concatmap_datum_stream_t(env_t *env, counted_t<func_t> _f,
                                                   counted_t<datum_stream_t> _source)
    : eager_datum_stream_t(env, _source->backtrace()), f(_f), source(_source) {
    guarantee(f.has() && source.has());
}

counted_t<const datum_t> concatmap_datum_stream_t::next_impl() {
    for (;;) {
        if (!subsource.has()) {
            counted_t<const datum_t> arg = source->next();
            if (!arg.has()) {
                return counted_t<const datum_t>();
            }
            subsource = f->call(arg)->as_seq();
        }

        counted_t<const datum_t> datum = subsource->next();
        if (datum.has()) {
            return datum;
        }

        subsource.reset();
    }
}

// SLICE_DATUM_STREAM_T
slice_datum_stream_t::slice_datum_stream_t(env_t *env, size_t _left, size_t _right,
                                           counted_t<datum_stream_t> _src)
    : wrapper_datum_stream_t(env, _src), index(0),
      left(_left), right(_right) { }

counted_t<const datum_t> slice_datum_stream_t::next_impl() {
    if (left >= right || index >= right) {
        return counted_t<const datum_t>();
    }

    while (index < left) {
        counted_t<const datum_t> discard = source->next();
        if (!discard.has()) {
            return counted_t<const datum_t>();
        }
        ++index;
    }

    counted_t<const datum_t> datum = source->next();
    if (datum.has()) {
        ++index;
    }
    return datum;
}

// ZIP_DATUM_STREAM_T
zip_datum_stream_t::zip_datum_stream_t(env_t *env, counted_t<datum_stream_t> _src)
    : wrapper_datum_stream_t(env, _src) { }

counted_t<const datum_t> zip_datum_stream_t::next_impl() {
    counted_t<const datum_t> datum = source->next();
    if (!datum.has()) {
        return counted_t<const datum_t>();
    }

    counted_t<const datum_t> left = datum->get("left", NOTHROW);
    counted_t<const datum_t> right = datum->get("right", NOTHROW);
    rcheck(left.has(), base_exc_t::GENERIC,
           "ZIP can only be called on the result of a join.");
    return right.has() ? left->merge(right) : left;
}

// UNION_DATUM_STREAM_T
counted_t<datum_stream_t> union_datum_stream_t::filter(counted_t<func_t> f) {
    for (auto it = streams.begin(); it != streams.end(); ++it) {
        *it = (*it)->filter(f);
    }
    return counted_t<datum_stream_t>(this);
}
counted_t<datum_stream_t> union_datum_stream_t::map(counted_t<func_t> f) {
    for (auto it = streams.begin(); it != streams.end(); ++it) {
        *it = (*it)->map(f);
    }
    return counted_t<datum_stream_t>(this);
}
counted_t<datum_stream_t> union_datum_stream_t::concatmap(counted_t<func_t> f) {
    for (auto it = streams.begin(); it != streams.end(); ++it) {
        *it = (*it)->concatmap(f);
    }
    return counted_t<datum_stream_t>(this);
}
counted_t<const datum_t> union_datum_stream_t::count() {
    counted_t<const datum_t> acc(new datum_t(0.0));
    for (auto it = streams.begin(); it != streams.end(); ++it) {
        acc = make_counted<const datum_t>(acc->as_num() + (*it)->count()->as_num());
    }
    return acc;
}
counted_t<const datum_t> union_datum_stream_t::reduce(
    counted_t<val_t> base_val, counted_t<func_t> f) {
    counted_t<const datum_t> base =
        base_val.has() ? base_val->as_datum() : counted_t<const datum_t>();
    std::vector<counted_t<const datum_t> > vals;

    for (auto it = streams.begin(); it != streams.end(); ++it) {
        try {
            counted_t<const datum_t> d = (*it)->reduce(base_val, f);
            vals.push_back(d);
        } catch (const base_exc_t &e) {
            // TODO: This is a terrible hack that will go away when we get rid
            // of the optional base, because we will have a better way of
            // communicating this error case than throwing an error with a
            // particular message.
            if (std::string(e.what()) != empty_stream_msg) {
                throw;
            }
        }
    }
    if (vals.empty()) {
        rcheck(base.has(), base_exc_t::NON_EXISTENCE, empty_stream_msg);
        return base;
    } else {
        counted_t<const datum_t> d;
        auto start = vals.begin();
        d = base.has() ? base : *(start++);
        for (auto it = start; it != vals.end(); ++it) {
            d = f->call(d, *it)->as_datum();
        }
        return d;
    }
}

counted_t<const datum_t> union_datum_stream_t::gmr(
    counted_t<func_t> g, counted_t<func_t> m,
    counted_t<const datum_t> base, counted_t<func_t> r) {
    wire_datum_map_t dm;
    for (auto it = streams.begin(); it != streams.end(); ++it) {
        counted_t<const datum_t> d = (*it)->gmr(g, m, base, r);
        for (size_t i = 0; i < d->size(); ++i) {
            counted_t<const datum_t> el = d->get(i);
            counted_t<const datum_t> el_group = el->get("group");
            counted_t<const datum_t> el_reduction = el->get("reduction");
            if (!dm.has(el_group)) {
                dm.set(el_group, el_reduction);
            } else {
                dm.set(el_group, r->call(dm.get(el_group), el_reduction)->as_datum());
            }
        }
    }
    return dm.to_arr();
}

bool union_datum_stream_t::is_array() {
    for (auto it = streams.begin(); it != streams.end(); ++it) {
        if (!(*it)->is_array()) {
            return false;
        }
    }
    return true;
}
counted_t<const datum_t> union_datum_stream_t::as_array() {
    if (!is_array()) {
        return counted_t<const datum_t>();
    }
    datum_ptr_t arr(datum_t::R_ARRAY);
    while (counted_t<const datum_t> d = next()) {
        arr.add(d);
    }
    return arr.to_counted();
}

counted_t<const datum_t> union_datum_stream_t::next_impl() {
    for (; streams_index < streams.size(); ++streams_index) {
        counted_t<const datum_t> datum = streams[streams_index]->next();
        if (datum.has()) {
            return datum;
        }
    }
    return counted_t<const datum_t>();
}

} // namespace ql
