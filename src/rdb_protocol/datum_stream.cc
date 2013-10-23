// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/datum_stream.hpp"

#include <map>

#include "clustering/administration/metadata.hpp"
#include "rdb_protocol/constants.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/val.hpp"

namespace ql {

const char *const empty_stream_msg =
    "Cannot reduce over an empty stream with no base.";

// RANGE/READGEN STUFF
reader_t::reader_t(
    const namespace_repo_t<rdb_protocol_t>::access_t &_ns_access,
    bool _use_outdated,
    scoped_ptr_t<readgen_t> &&_readgen)
    : ns_access(_ns_access),
      use_outdated(_use_outdated),
      started(false), finished(false),
      readgen(std::move(_readgen)),
      active_range(readgen->original_keyrange()) { }

void reader_t::add_transformation(transform_variant_t &&tv) {
    r_sanity_check(!started);
    transform.push_back(std::move(tv));
}

rget_read_response_t::result_t
reader_t::run_terminal(env_t *env, terminal_variant_t &&tv) {
    r_sanity_check(!started);
    started = finished = true;
    rget_read_response_t res
        = do_read(env, readgen->terminal_read(transform, std::move(tv)));
    return std::move(res.result);
}

rget_read_response_t reader_t::do_read(env_t *env, const read_t &read) {
    read_response_t res;
    try {
        if (use_outdated) {
            ns_access.get_namespace_if()->read_outdated(read, &res, env->interruptor);
        } else {
            ns_access.get_namespace_if()->read(
                read, &res, order_token_t::ignore, env->interruptor);
        }
    } catch (const cannot_perform_query_exc_t &e) {
        rfail_datum(ql::base_exc_t::GENERIC, "cannot perform read: %s", e.what());
    }
    auto rget_res = boost::get<rget_read_response_t>(&res.response);
    r_sanity_check(rget_res != NULL);
    /* Re-throw an exception if we got one. */
    if (auto e = boost::get<ql::exc_t>(&rget_res->result)) {
        throw *e;
    } else if (auto e2 = boost::get<ql::datum_exc_t>(&rget_res->result)) {
        throw *e2;
    }
    return std::move(*rget_res);
}

std::vector<rget_item_t> reader_t::do_range_read(env_t *env, const read_t &read) {
    rget_read_response_t res = do_read(env, read);
    finished = readgen->update_range(&active_range, res.last_considered_key);
    auto v = boost::get<std::vector<rget_item_t> >(&res.result);
    r_sanity_check(v);
    return std::move(*v);
}

bool reader_t::load_items(env_t *env) {
    started = true;
    if (items_index >= items.size() && !finished) { // read some more
        items_index = 0;
        items = do_range_read(env, readgen->next_read(active_range, transform));
        // RSI: Enforce sort limit.
        // Everything below this point can handle `items` being empty (this is
        // good hygiene anyway).
        while (boost::optional<read_t> read
               = readgen->sindex_sort_read(items, transform)) {
            std::vector<rget_item_t> new_items = do_range_read(env, *read);
            if (new_items.size() == 0) {
                break;
            }
            items.reserve(items.size() + new_items.size());
            std::move(new_items.begin(), new_items.end(), std::back_inserter(items));
        }
        readgen->sindex_sort(&items);
    }
    if (items_index >= items.size()) {
        finished = true;
    }
    return items_index < items.size();
}

std::vector<counted_t<const datum_t> >
reader_t::next_batch(env_t *env, batch_type_t batch_type) {
    started = true;
    if (!load_items(env)) {
        return std::vector<counted_t<const datum_t> >();
    }
    r_sanity_check(items_index < items.size());

    std::vector<counted_t<const datum_t> > toret;
    switch (batch_type) {
    case NORMAL: {
        toret.reserve(items.size() - items_index);
        for (; items_index < items.size(); ++items_index) {
            toret.push_back(std::move(items[items_index].data));
        }
    } break;
    case SINDEX_CONSTANT: {
        boost::optional<counted_t<const ql::datum_t> > sindex
            = std::move(items[items_index].sindex_key);
        toret.push_back(std::move(items[items_index].data));
        items_index += 1;

        for (; items_index < items.size(); ++items_index) {
            if (sindex) {
                r_sanity_check(items[items_index].sindex_key);
                if (*items[items_index].sindex_key != *sindex) {
                    break; // batch is done
                }
            } else {
                r_sanity_check(!items[items_index].sindex_key);
            }
            toret.push_back(std::move(items[items_index].data));
        }
    } break;
    default: unreachable();
    }

    // RSI: prefetch instead?
    if (items_index >= items.size()) { // free memory immediately
        items_index = 0;
        std::vector<rget_item_t> tmp;
        tmp.swap(items);
    }

    return toret;
}

readgen_t::readgen_t(
    const std::map<std::string, wire_func_t> &_global_optargs,
    const datum_range_t &_original_datum_range,
    sorting_t _sorting)
    : global_optargs(_global_optargs),
      original_datum_range(_original_datum_range),
      sorting(_sorting) { }

bool readgen_t::update_range(key_range_t *active_range,
                             const store_key_t &last_considered_key) {
    if (sorting != DESCENDING) {
        active_range->left = last_considered_key;
    } else {
        active_range->right = key_range_t::right_bound_t(last_considered_key);
    }

    // TODO: mixing these non-const operations INTO THE CONDITIONAL is bad, and
    // confused me for a while when I tried moving some stuff around.
    if (sorting != DESCENDING) {
        if (!active_range->left.increment()
            || (!active_range->right.unbounded
                && (active_range->right.key < active_range->left))) {
            return true;
        }
    } else {
        r_sanity_check(!active_range->right.unbounded);
        if (!active_range->right.key.decrement()
            || active_range->right.key < active_range->left) {
            return true;
        }
    }
    return false;
}

// RSI: remove if one line
read_t readgen_t::next_read(
    const key_range_t &active_range, const transform_t &transform) {
    return read_t(next_read_impl(active_range, transform));
}

// TODO: this is how we did it before, but it sucks.
read_t readgen_t::terminal_read(
    const transform_t &transform, terminal_t &&_terminal) {
    rget_read_t read = next_read_impl(original_keyrange(), transform);
    read.terminal = std::move(_terminal);
    return read_t(read);
}

primary_readgen_t::primary_readgen_t(
    const std::map<std::string, wire_func_t> &global_optargs,
    datum_range_t range,
    sorting_t sorting)
    : readgen_t(global_optargs, range, sorting) { }

rget_read_t primary_readgen_t::next_read_impl(
    const key_range_t &active_range, const transform_t &transform) {
    return rget_read_t(region_t(active_range), transform, global_optargs, sorting);
}

// We never need to do an sindex sort when indexing by a primary key.
boost::optional<read_t> primary_readgen_t::sindex_sort_read(
    UNUSED const std::vector<rget_item_t> &items, UNUSED const transform_t &transform) {
    return boost::optional<read_t>();
}
void primary_readgen_t::sindex_sort(UNUSED std::vector<rget_item_t> *vec) {
    return;
}

key_range_t primary_readgen_t::original_keyrange() {
    return original_datum_range.to_primary_keyrange();
}

sindex_readgen_t::sindex_readgen_t(
    const std::map<std::string, wire_func_t> &global_optargs,
    const std::string &_sindex,
    datum_range_t range,
    sorting_t sorting)
    : readgen_t(global_optargs, range, sorting), sindex(_sindex) { }

void sindex_readgen_t::sindex_sort(std::vector<rget_item_t> *vec) {
    if (vec->size() == 0) {
        return;
    }
    class sorter_t {
    public:
        sorter_t(sorting_t _sorting) : sorting(_sorting) { }
        bool operator()(const rget_item_t &l, const rget_item_t &r) {
            r_sanity_check(l.sindex_key && r.sindex_key);
            return sorting == ASCENDING
                ? (*l.sindex_key < *r.sindex_key)
                : (*l.sindex_key > *r.sindex_key);
        }
    private:
        sorting_t sorting;
    };
    if (sorting != UNORDERED) {
        std::sort(vec->begin(), vec->end(), sorter_t(sorting));
    }
}

rget_read_t sindex_readgen_t::next_read_impl(
    const key_range_t &active_range, const transform_t &transform) {
    return rget_read_t(region_t(active_range), sindex, original_datum_range,
                       transform, global_optargs, sorting);
}

// RSI: test this shit
boost::optional<read_t> sindex_readgen_t::sindex_sort_read(
    const std::vector<rget_item_t> &items, const transform_t &transform) {
    if (sorting != UNORDERED && items.size() > 0) {
        const store_key_t &key = items[items.size() - 1].key;
        if (datum_t::key_is_truncated(key)) {
            std::string skey = datum_t::extract_secondary(key_to_unescaped_str(key));
            // Generate the right bound by filling to the end with 0xFF.  This
            // lets us produce a range that's "everything after the last row we
            // read with the same truncated sindex value".
            store_key_t rbound(skey + std::string(MAX_KEY_SIZE - skey.size(), 0xFF));
            key_range_t rng(key_range_t::open, key, key_range_t::closed, rbound);
            return boost::optional<read_t>(read_t(rget_read_t(
                region_t(key_range_t(rng)), sindex, original_datum_range,
                transform, global_optargs, sorting)));
        }
    }
    return boost::optional<read_t>();
}

key_range_t sindex_readgen_t::original_keyrange() {
    return original_datum_range.to_sindex_keyrange();
}

// DATUM_STREAM_T
counted_t<datum_stream_t> datum_stream_t::slice(size_t l, size_t r) {
    return make_counted<slice_datum_stream_t>(l, r, this->counted_from_this());
}
counted_t<datum_stream_t> datum_stream_t::zip() {
    return make_counted<zip_datum_stream_t>(this->counted_from_this());
}
counted_t<datum_stream_t> datum_stream_t::indexes_of(counted_t<func_t> f) {
    return make_counted<indexes_of_datum_stream_t>(f, counted_from_this());
}

std::vector<counted_t<const datum_t> >
datum_stream_t::next_batch(env_t *env, batch_type_t batch_type) {
    DEBUG_ONLY_CODE(env->do_eval_callback());
    env->throw_if_interruptor_pulsed();
    // Cannot mix `next` and `next_batch`.
    r_sanity_check(batch_cache_index == 0 && batch_cache.size() == 0);
    try {
        return next_batch_impl(env, batch_type);
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
        unreachable();
    }
}

counted_t<const datum_t> datum_stream_t::next(env_t *env) {
    if (batch_cache_index >= batch_cache.size()) {
        batch_cache_index = 0;
        batch_cache = next_batch(env, NORMAL);
        if (batch_cache_index >= batch_cache.size()) {
            return counted_t<const datum_t>();
        }
    }
    r_sanity_check(batch_cache_index < batch_cache.size());
    return std::move(batch_cache[batch_cache_index++]);
}

counted_t<const datum_t> eager_datum_stream_t::count(env_t *env) {
    size_t acc = 0;
    while (counted_t<const datum_t> d = next(env)) {
        acc += 1;
    }
    return make_counted<datum_t>(static_cast<double>(acc));
}

counted_t<const datum_t> eager_datum_stream_t::reduce(env_t *env,
                                                      counted_t<val_t> base_val,
                                                      counted_t<func_t> f) {
    counted_t<const datum_t> base = base_val.has() ? base_val->as_datum() : next(env);
    rcheck(base.has(), base_exc_t::NON_EXISTENCE, empty_stream_msg);

    while (counted_t<const datum_t> rhs = next(env)) {
        base = f->call(env, base, rhs)->as_datum();
    }
    return base;
}

counted_t<const datum_t> eager_datum_stream_t::gmr(env_t *env,
                                                   counted_t<func_t> group,
                                                   counted_t<func_t> map,
                                                   counted_t<const datum_t> base,
                                                   counted_t<func_t> reduce) {
    wire_datum_map_t wd_map;
    while (counted_t<const datum_t> el = next(env)) {
        counted_t<const datum_t> el_group = group->call(env, el)->as_datum();
        counted_t<const datum_t> el_map = map->call(env, el)->as_datum();
        if (!wd_map.has(el_group)) {
            wd_map.set(el_group,
                       base.has() ? reduce->call(env, base, el_map)->as_datum() : el_map);
        } else {
            wd_map.set(el_group, reduce->call(env, wd_map.get(el_group), el_map)->as_datum());
        }
    }
    return wd_map.to_arr();
}

counted_t<datum_stream_t> eager_datum_stream_t::filter(
    counted_t<func_t> f,
    counted_t<func_t> default_filter_val) {
    return make_counted<filter_datum_stream_t>(
        f, default_filter_val, this->counted_from_this());
}
counted_t<datum_stream_t> eager_datum_stream_t::map(counted_t<func_t> f) {
    return make_counted<map_datum_stream_t>(f, this->counted_from_this());
}
counted_t<datum_stream_t> eager_datum_stream_t::concatmap(counted_t<func_t> f) {
    return make_counted<concatmap_datum_stream_t>(f, this->counted_from_this());
}

counted_t<const datum_t> eager_datum_stream_t::as_array(env_t *env) {
    datum_ptr_t arr(datum_t::R_ARRAY);
    while (counted_t<const datum_t> d = next(env)) {
        arr.add(d);
    }
    return arr.to_counted();
}

// LAZY_DATUM_STREAM_T
lazy_datum_stream_t::lazy_datum_stream_t(
    namespace_repo_t<rdb_protocol_t>::access_t *ns_access,
    bool use_outdated,
    scoped_ptr_t<readgen_t> &&readgen,
    const protob_t<const Backtrace> &bt_src)
    : datum_stream_t(bt_src),
      current_batch_offset(0),
      reader(*ns_access, use_outdated, std::move(readgen)) { }

counted_t<datum_stream_t> lazy_datum_stream_t::map(counted_t<func_t> f) {
    reader.add_transformation(map_wire_func_t(f));
    return counted_from_this();
}

counted_t<datum_stream_t> lazy_datum_stream_t::concatmap(counted_t<func_t> f) {
    reader.add_transformation(concatmap_wire_func_t(f));
    return counted_from_this();
}

counted_t<datum_stream_t> lazy_datum_stream_t::filter(
    counted_t<func_t> f, counted_t<func_t> default_filter_val) {
    reader.add_transformation(
        filter_transform_t(
            wire_func_t(f),
            default_filter_val.has()
                ? boost::make_optional(wire_func_t(default_filter_val))
                : boost::none));
    return counted_from_this();
}

counted_t<const datum_t> lazy_datum_stream_t::count(env_t *env) {
    rget_read_response_t::result_t res = reader.run_terminal(env, count_wire_func_t());
    return boost::get<counted_t<const datum_t> >(res);
}

counted_t<const datum_t> lazy_datum_stream_t::reduce(
    env_t *env, counted_t<val_t> base_val, counted_t<func_t> f) {
    rget_read_response_t::result_t res
        = reader.run_terminal(env, reduce_wire_func_t(f));

    if (counted_t<const datum_t> *d = boost::get<counted_t<const datum_t> >(&res)) {
        counted_t<const datum_t> datum = *d;
        if (base_val.has()) {
            return f->call(env, base_val->as_datum(), datum)->as_datum();
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

counted_t<const datum_t> lazy_datum_stream_t::gmr(
    env_t *env, counted_t<func_t> g, counted_t<func_t> m,
    counted_t<const datum_t> base, counted_t<func_t> r) {
    rget_read_response_t::result_t res =
        reader.run_terminal(env, gmr_wire_func_t(g, m, r));
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
            map.set(key, r->call(env, base, val)->as_datum());
        }
        return map.to_arr();
    }
}

std::vector<counted_t<const datum_t> >
lazy_datum_stream_t::next_batch_impl(env_t *env, batch_type_t batch_type) {
    // Should never mix `next` with `next_batch`.
    r_sanity_check(current_batch_offset == 0 && current_batch.size() == 0);
    return reader.next_batch(env, batch_type);
}

counted_t<const datum_t> lazy_datum_stream_t::next_impl(env_t *env) {
    if (current_batch_offset >= current_batch.size()) {
        current_batch_offset = 0;
        current_batch = reader.next_batch(env, NORMAL);
        if (current_batch_offset >= current_batch.size()) {
            return counted_t<const datum_t>();
        }
    }
    return std::move(current_batch[current_batch_offset++]);
}

// ARRAY_DATUM_STREAM_T
array_datum_stream_t::array_datum_stream_t(counted_t<const datum_t> _arr,
                                           const protob_t<const Backtrace> &bt_source)
    : eager_datum_stream_t(bt_source), index(0), arr(_arr) { }

counted_t<const datum_t> array_datum_stream_t::next(UNUSED env_t *env) {
    return arr->get(index++, NOTHROW);
}

std::vector<counted_t<const datum_t> >
array_datum_stream_t::next_batch_impl(env_t *env, UNUSED batch_type_t batch_type) {
    std::vector<counted_t<const datum_t> > v;
    size_t total_size = 0;
    while (counted_t<const datum_t> d = next(env)) {
        total_size += serialized_size(d);
        v.push_back(std::move(d));
        if (should_send_batch(v.size(), total_size, 0)) {
            break;
        }
    }
    return v;
}

bool array_datum_stream_t::is_array() {
    return true;
}

// MAP_DATUM_STREAM_T
map_datum_stream_t::map_datum_stream_t(counted_t<func_t> _f,
                                       counted_t<datum_stream_t> _source)
    : wrapper_datum_stream_t(_source), f(_f) {
    guarantee(f.has() && source.has());
}
std::vector<counted_t<const datum_t> >
map_datum_stream_t::next_batch_impl(env_t *env, batch_type_t batch_type) {
    std::vector<counted_t<const datum_t> > v = source->next_batch(env, batch_type);
    for (auto it = v.begin(); it != v.end(); ++it) {
        *it = f->call(env, *it)->as_datum();
    }
    return v;
}

// INDEXES_OF_DATUM_STREAM_T
indexes_of_datum_stream_t::indexes_of_datum_stream_t(counted_t<func_t> _f,
                                                     counted_t<datum_stream_t> _source)
    : wrapper_datum_stream_t(_source), f(_f), index(0) {
    guarantee(f.has() && source.has());
}

std::vector<counted_t<const datum_t> >
indexes_of_datum_stream_t::next_batch_impl(env_t *env, batch_type_t batch_type) {
    std::vector<counted_t<const datum_t> > ret;
    while (ret.size() == 0) {
        std::vector<counted_t<const datum_t> > v = source->next_batch(env, batch_type);
        if (v.size() == 0) {
            break;
        }
        for (auto it = v.begin(); it != v.end(); ++it, ++index) {
            if (f->filter_call(env, *it, counted_t<func_t>())) {
                ret.push_back(make_counted<datum_t>(static_cast<double>(index)));
            }
        }
    }
    return ret;
}

// FILTER_DATUM_STREAM_T
filter_datum_stream_t::filter_datum_stream_t(counted_t<func_t> _f,
                                             counted_t<func_t> _default_filter_val,
                                             counted_t<datum_stream_t> _source)
    : wrapper_datum_stream_t(_source), f(_f),
      default_filter_val(_default_filter_val) {
    guarantee(f.has() && source.has());
}

std::vector<counted_t<const datum_t> >
filter_datum_stream_t::next_batch_impl(env_t *env, batch_type_t batch_type) {
    std::vector<counted_t<const datum_t> > ret;
    while (ret.size() == 0) {
        std::vector<counted_t<const datum_t> > v = source->next_batch(env, batch_type);
        if (v.size() == 0) {
            break;
        }
        for (auto it = v.begin(); it != v.end(); ++it) {
            if (f->filter_call(env, *it, default_filter_val)) {
                ret.push_back(std::move(*it));
            }
        }
    }
    return ret;
}

// CONCATMAP_DATUM_STREAM_T
concatmap_datum_stream_t::concatmap_datum_stream_t(counted_t<func_t> _f,
                                                   counted_t<datum_stream_t> _source)
    : wrapper_datum_stream_t(_source), f(_f) {
    guarantee(f.has() && source.has());
}

std::vector<counted_t<const datum_t> >
concatmap_datum_stream_t::next_batch_impl(env_t *env, batch_type_t batch_type) {
    for (;;) {
        if (!subsource.has()) {
            // We can use `next` here because the `batch_type` comes into play below.
            counted_t<const datum_t> arg = source->next(env);
            if (!arg.has()) {
                return std::vector<counted_t<const datum_t> >();
            }
            subsource = f->call(env, arg)->as_seq(env);
        }
        std::vector<counted_t<const datum_t> > v
            = subsource->next_batch(env, batch_type);
        if (v.size() != 0) {
            return v;
        } else {
            subsource.reset();
        }
    }
}

// SLICE_DATUM_STREAM_T
slice_datum_stream_t::slice_datum_stream_t(size_t _left, size_t _right,
                                           counted_t<datum_stream_t> _src)
    : wrapper_datum_stream_t(_src), index(0),
      left(_left), right(_right) { }

std::vector<counted_t<const datum_t> >
slice_datum_stream_t::next_batch_impl(env_t *env, batch_type_t batch_type) {
    if (left >= right) {
        return std::vector<counted_t<const datum_t> >();
    }

    while (index < left) {
        std::vector<counted_t<const datum_t> > v = source->next_batch(env, batch_type);
        index += v.size();
        if (index > left) {
            std::vector<counted_t<const datum_t> > ret;
            ret.reserve(index - left);
            std::move(v.end() - (index - left), v.end(), std::back_inserter(ret));
            return ret;
        }
    }

    while (index < right) {
        std::vector<counted_t<const datum_t> > v = source->next_batch(env, batch_type);
        index += v.size();
        if (index > right) {
            v.resize(v.size() - (index - right));
        }
        return v;
    }

    return std::vector<counted_t<const datum_t> >();
}

// ZIP_DATUM_STREAM_T
zip_datum_stream_t::zip_datum_stream_t(counted_t<datum_stream_t> _src)
    : wrapper_datum_stream_t(_src) { }

std::vector<counted_t<const datum_t> >
zip_datum_stream_t::next_batch_impl(env_t *env, batch_type_t batch_type) {
    std::vector<counted_t<const datum_t> > v = source->next_batch(env, batch_type);
    for (auto it = v.begin(); it != v.end(); ++it) {
        auto left = (*it)->get("left", NOTHROW);
        auto right = (*it)->get("right", NOTHROW);
        rcheck(left.has(), base_exc_t::GENERIC,
               "ZIP can only be called on the result of a join.");
        *it = right.has() ? left->merge(right) : left;
    }
    return v;
}

// UNION_DATUM_STREAM_T
counted_t<datum_stream_t> union_datum_stream_t::filter(
    counted_t<func_t> f,
    counted_t<func_t> default_filter_val) {
    for (auto it = streams.begin(); it != streams.end(); ++it) {
        *it = (*it)->filter(f, default_filter_val);
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
counted_t<const datum_t> union_datum_stream_t::count(env_t *env) {
    counted_t<const datum_t> acc(new datum_t(0.0));
    for (auto it = streams.begin(); it != streams.end(); ++it) {
        acc = make_counted<const datum_t>(acc->as_num() + (*it)->count(env)->as_num());
    }
    return acc;
}
counted_t<const datum_t> union_datum_stream_t::reduce(env_t *env,
                                                      counted_t<val_t> base_val,
                                                      counted_t<func_t> f) {
    counted_t<const datum_t> base =
        base_val.has() ? base_val->as_datum() : counted_t<const datum_t>();
    std::vector<counted_t<const datum_t> > vals;

    for (auto it = streams.begin(); it != streams.end(); ++it) {
        try {
            counted_t<const datum_t> d = (*it)->reduce(env, base_val, f);
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
            d = f->call(env, d, *it)->as_datum();
        }
        return d;
    }
}

counted_t<const datum_t> union_datum_stream_t::gmr(
        env_t *env,
        counted_t<func_t> g, counted_t<func_t> m,
        counted_t<const datum_t> base, counted_t<func_t> r) {
    wire_datum_map_t dm;
    for (auto it = streams.begin(); it != streams.end(); ++it) {
        counted_t<const datum_t> d = (*it)->gmr(env, g, m, base, r);
        for (size_t i = 0; i < d->size(); ++i) {
            counted_t<const datum_t> el = d->get(i);
            counted_t<const datum_t> el_group = el->get("group");
            counted_t<const datum_t> el_reduction = el->get("reduction");
            if (!dm.has(el_group)) {
                dm.set(el_group, el_reduction);
            } else {
                dm.set(el_group,
                       r->call(env, dm.get(el_group), el_reduction)->as_datum());
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
counted_t<const datum_t> union_datum_stream_t::as_array(env_t *env) {
    if (!is_array()) {
        return counted_t<const datum_t>();
    }
    datum_ptr_t arr(datum_t::R_ARRAY);
    while (counted_t<const datum_t> d = next(env)) {
        arr.add(d);
    }
    return arr.to_counted();
}

std::vector<counted_t<const datum_t> >
union_datum_stream_t::next_batch_impl(env_t *env, batch_type_t batch_type) {
    for (; streams_index < streams.size(); ++streams_index) {
        std::vector<counted_t<const datum_t> > batch
            = streams[streams_index]->next_batch(env, batch_type);
        if (batch.size() != 0) {
            return batch;
        }
    }
    return std::vector<counted_t<const datum_t> >();
}

} // namespace ql
