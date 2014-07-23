// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/datum_stream.hpp"

#include <map>

#include "rdb_protocol/batching.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/val.hpp"

#include "debug.hpp"

namespace ql {

counted_t<val_t> datum_stream_t::run_terminal(
    env_t *env, const terminal_variant_t &tv) {
    scoped_ptr_t<eager_acc_t> acc(make_eager_terminal(tv));
    accumulate(env, acc.get(), tv);
    return acc->finish_eager(backtrace(), is_grouped());
}

counted_t<val_t> datum_stream_t::to_array(env_t *env) {
    scoped_ptr_t<eager_acc_t> acc = make_to_array();
    accumulate_all(env, acc.get());
    return acc->finish_eager(backtrace(), is_grouped());
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
counted_t<datum_stream_t> datum_stream_t::ordered_distinct() {
    return make_counted<ordered_distinct_datum_stream_t>(counted_from_this());
}

datum_stream_t::datum_stream_t(const protob_t<const Backtrace> &bt_src)
    : pb_rcheckable_t(bt_src), batch_cache_index(0), grouped(false) {
}

void datum_stream_t::add_grouping(transform_variant_t &&tv,
                                  const protob_t<const Backtrace> &bt) {
    check_not_grouped("Cannot call `group` on the output of `group` "
                      "(did you mean to `ungroup`?).");
    grouped = true;
    add_transformation(std::move(tv), bt);
}
void datum_stream_t::check_not_grouped(const char *msg) {
    rcheck(!is_grouped(), base_exc_t::GENERIC, msg);
}

std::vector<counted_t<const datum_t> >
datum_stream_t::next_batch(env_t *env, const batchspec_t &batchspec) {
    DEBUG_ONLY_CODE(env->do_eval_callback());
    if (env->interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
    // Cannot mix `next` and `next_batch`.
    r_sanity_check(batch_cache_index == 0 && batch_cache.size() == 0);
    check_not_grouped("Cannot treat the output of `group` as a stream "
                      "(did you mean to `ungroup`?).");
    try {
        return next_batch_impl(env, batchspec);
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
        unreachable();
    }
}

counted_t<const datum_t> datum_stream_t::next(
    env_t *env, const batchspec_t &batchspec) {

    profile::starter_t("Reading element from datum stream.", env->trace);
    if (batch_cache_index >= batch_cache.size()) {
        r_sanity_check(batch_cache_index == 0);
        batch_cache = next_batch(env, batchspec);
        if (batch_cache_index >= batch_cache.size()) {
            return counted_t<const datum_t>();
        }
    }
    r_sanity_check(batch_cache_index < batch_cache.size());
    counted_t<const datum_t> d = std::move(batch_cache[batch_cache_index++]);
    if (batch_cache_index >= batch_cache.size()) {
        // Free the vector as soon as we're done with it.  This also keeps the
        // assert in `next_batch` happy.
        batch_cache_index = 0;
        std::vector<counted_t<const datum_t> > tmp;
        tmp.swap(batch_cache);
    }
    return d;
}

bool datum_stream_t::batch_cache_exhausted() const {
    return batch_cache_index >= batch_cache.size();
}

void eager_datum_stream_t::add_transformation(
    transform_variant_t &&tv, const protob_t<const Backtrace> &bt) {
    ops.push_back(make_op(std::move(tv)));
    update_bt(bt);
}

eager_datum_stream_t::done_t eager_datum_stream_t::next_grouped_batch(
    env_t *env, const batchspec_t &bs, groups_t *out) {
    r_sanity_check(out->size() == 0);
    while (out->size() == 0) {
        std::vector<counted_t<const datum_t> > v = next_raw_batch(env, bs);
        if (v.size() == 0) {
            return done_t::YES;
        }
        (*out)[counted_t<const datum_t>()] = std::move(v);
        for (auto it = ops.begin(); it != ops.end(); ++it) {
            (**it)(env, out, counted_t<const datum_t>());
        }
    }
    return done_t::NO;
}

void eager_datum_stream_t::accumulate(
    env_t *env, eager_acc_t *acc, const terminal_variant_t &) {
    batchspec_t bs = batchspec_t::user(batch_type_t::TERMINAL, env);
    groups_t data;
    while (next_grouped_batch(env, bs, &data) == done_t::NO) {
        (*acc)(env, &data);
    }
}

void eager_datum_stream_t::accumulate_all(env_t *env, eager_acc_t *acc) {
    groups_t data;
    done_t done = next_grouped_batch(env, batchspec_t::all(), &data);
    (*acc)(env, &data);
    if (done == done_t::NO) {
        done_t must_be_yes = next_grouped_batch(env, batchspec_t::all(), &data);
        r_sanity_check(data.size() == 0);
        r_sanity_check(must_be_yes == done_t::YES);
    }
}

std::vector<counted_t<const datum_t> >
eager_datum_stream_t::next_batch_impl(env_t *env, const batchspec_t &bs) {
    groups_t data;
    next_grouped_batch(env, bs, &data);
    return groups_to_batch(&data);
}

counted_t<const datum_t> eager_datum_stream_t::as_array(env_t *env) {
    if (is_grouped() || !is_array()) return counted_t<const datum_t>();
    datum_ptr_t arr(datum_t::R_ARRAY);
    batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env);
    {
        profile::sampler_t sampler("Evaluating stream eagerly.", env->trace);
        while (counted_t<const datum_t> d = next(env, batchspec)) {
            arr.add(d);
            sampler.new_sample();
        }
    }
    return arr.to_counted();
}

array_datum_stream_t::array_datum_stream_t(counted_t<const datum_t> _arr,
                                           const protob_t<const Backtrace> &bt_source)
    : eager_datum_stream_t(bt_source), index(0), arr(_arr) { }

counted_t<const datum_t> array_datum_stream_t::next(env_t *env, const batchspec_t &bs) {
    return ops_to_do() ? datum_stream_t::next(env, bs) : next_arr_el();
}
counted_t<const datum_t> array_datum_stream_t::next_arr_el() {
    return index < arr->size() ? arr->get(index++) : counted_t<const datum_t>();
}

bool array_datum_stream_t::is_exhausted() const {
    return index >= arr->size();
}
bool array_datum_stream_t::is_cfeed() const {
    return false;
}

std::vector<counted_t<const datum_t> >
array_datum_stream_t::next_raw_batch(env_t *env, const batchspec_t &batchspec) {
    std::vector<counted_t<const datum_t> > v;
    batcher_t batcher = batchspec.to_batcher();

    profile::sampler_t sampler("Fetching array elements.", env->trace);
    while (counted_t<const datum_t> d = next_arr_el()) {
        batcher.note_el(d);
        v.push_back(std::move(d));
        if (batcher.should_send_batch()) {
            break;
        }
        sampler.new_sample();
    }
    return v;
}

bool array_datum_stream_t::is_array() {
    return !is_grouped();
}

// INDEXED_SORT_DATUM_STREAM_T
indexed_sort_datum_stream_t::indexed_sort_datum_stream_t(
    counted_t<datum_stream_t> stream,
    std::function<bool(env_t *,  // NOLINT(readability/casting)
                       profile::sampler_t *,
                       const counted_t<const datum_t> &,
                       const counted_t<const datum_t> &)> _lt_cmp)
    : wrapper_datum_stream_t(stream), lt_cmp(_lt_cmp), index(0) { }

std::vector<counted_t<const datum_t> >
indexed_sort_datum_stream_t::next_raw_batch(env_t *env, const batchspec_t &batchspec) {
    std::vector<counted_t<const datum_t> > ret;
    batcher_t batcher = batchspec.to_batcher();

    profile::sampler_t sampler("Sorting by index.", env->trace);
    while (!batcher.should_send_batch()) {
        if (index >= data.size()) {
            if (ret.size() > 0
                && batchspec.get_batch_type() == batch_type_t::SINDEX_CONSTANT) {
                // Never read more than one SINDEX_CONSTANT batch if we need to
                // return an SINDEX_CONSTANT batch.
                return ret;
            }
            index = 0;
            data = source->next_batch(
                env, batchspec.with_new_batch_type(batch_type_t::SINDEX_CONSTANT));
            if (index >= data.size()) {
                return ret;
            }
            std::stable_sort(data.begin(), data.end(),
                             std::bind(lt_cmp, env, &sampler, ph::_1, ph::_2));
        }
        for (; index < data.size() && !batcher.should_send_batch(); ++index) {
            batcher.note_el(data[index]);
            ret.push_back(std::move(data[index]));
        }
    }
    return ret;
}

// ORDERED_DISTINCT_DATUM_STREAM_T
ordered_distinct_datum_stream_t::ordered_distinct_datum_stream_t(
    counted_t<datum_stream_t> _source) : wrapper_datum_stream_t(_source) { }

std::vector<counted_t<const datum_t> >
ordered_distinct_datum_stream_t::next_raw_batch(env_t *env, const batchspec_t &bs) {
    std::vector<counted_t<const datum_t> > ret;
    profile::sampler_t sampler("Ordered distinct.", env->trace);
    while (ret.size() == 0) {
        std::vector<counted_t<const datum_t> > v = source->next_batch(env, bs);
        if (v.size() == 0) break;
        for (auto &&el : v) {
            if (!last_val.has() || *last_val != *el) {
                last_val = el;
                ret.push_back(std::move(el));
            }
            sampler.new_sample();
        }
    }
    return ret;
}

// INDEXES_OF_DATUM_STREAM_T
indexes_of_datum_stream_t::indexes_of_datum_stream_t(counted_t<func_t> _f,
                                                     counted_t<datum_stream_t> _source)
    : wrapper_datum_stream_t(_source), f(_f), index(0) {
    guarantee(f.has() && source.has());
}

std::vector<counted_t<const datum_t> >
indexes_of_datum_stream_t::next_raw_batch(env_t *env, const batchspec_t &bs) {
    std::vector<counted_t<const datum_t> > ret;
    profile::sampler_t sampler("Finding indexes_of eagerly.", env->trace);
    while (ret.size() == 0) {
        std::vector<counted_t<const datum_t> > v = source->next_batch(env, bs);
        if (v.size() == 0) {
            break;
        }
        for (auto it = v.begin(); it != v.end(); ++it, ++index) {
            if (f->filter_call(env, *it, counted_t<func_t>())) {
                ret.push_back(make_counted<datum_t>(static_cast<double>(index)));
            }
            sampler.new_sample();
        }
    }
    return ret;
}

// SLICE_DATUM_STREAM_T
slice_datum_stream_t::slice_datum_stream_t(
    uint64_t _left, uint64_t _right, counted_t<datum_stream_t> _src)
    : wrapper_datum_stream_t(_src), index(0), left(_left), right(_right) { }

std::vector<counted_t<const datum_t> >
slice_datum_stream_t::next_raw_batch(env_t *env, const batchspec_t &_batchspec) {
    if (left >= right || index >= right) {
        return std::vector<counted_t<const datum_t> >();
    }

    const batchspec_t batchspec = _batchspec.with_at_most(right - index);

    profile::sampler_t sampler("Slicing eagerly.", env->trace);
    while (index < left) {
        sampler.new_sample();
        std::vector<counted_t<const datum_t> > v = source->next_batch(env, batchspec);
        if (v.size() == 0) {
            return v;
        }
        index += v.size();
        if (index > right) {
            v.resize(v.size() - (index - right));
            index = right;
        }
        if (index > left) {
            std::vector<counted_t<const datum_t> > ret;
            ret.reserve(index - left);
            std::move(v.end() - (index - left), v.end(), std::back_inserter(ret));
            return ret;
        }
    }

    while (index < right) {
        sampler.new_sample();
        std::vector<counted_t<const datum_t> > v = source->next_batch(env, batchspec);
        if (v.size() == 0) {
            break;
        }
        index += v.size();
        if (index > right) {
            v.resize(v.size() - (index - right));
        }
        return v;
    }

    return std::vector<counted_t<const datum_t> >();
}

bool slice_datum_stream_t::is_exhausted() const {
    return (left >= right || index >= right || source->is_exhausted())
        && batch_cache_exhausted();
}
bool slice_datum_stream_t::is_cfeed() const {
    return source->is_cfeed();
}

// ZIP_DATUM_STREAM_T
zip_datum_stream_t::zip_datum_stream_t(counted_t<datum_stream_t> _src)
    : wrapper_datum_stream_t(_src) { }

std::vector<counted_t<const datum_t> >
zip_datum_stream_t::next_raw_batch(env_t *env, const batchspec_t &batchspec) {
    std::vector<counted_t<const datum_t> > v = source->next_batch(env, batchspec);

    profile::sampler_t sampler("Zipping eagerly.", env->trace);
    for (auto it = v.begin(); it != v.end(); ++it) {
        auto left = (*it)->get("left", NOTHROW);
        auto right = (*it)->get("right", NOTHROW);
        rcheck(left.has(), base_exc_t::GENERIC,
               "ZIP can only be called on the result of a join.");
        *it = right.has() ? left->merge(right) : left;
        sampler.new_sample();
    }
    return v;
}

// UNION_DATUM_STREAM_T
void union_datum_stream_t::add_transformation(transform_variant_t &&tv,
                                              const protob_t<const Backtrace> &bt) {
    for (auto it = streams.begin(); it != streams.end(); ++it) {
        (*it)->add_transformation(transform_variant_t(std::move(tv)), bt);
    }
    update_bt(bt);
}

void union_datum_stream_t::accumulate(
    env_t *env, eager_acc_t *acc, const terminal_variant_t &tv) {
    for (auto it = streams.begin(); it != streams.end(); ++it) {
        (*it)->accumulate(env, acc, tv);
    }
}

void union_datum_stream_t::accumulate_all(env_t *env, eager_acc_t *acc) {
    for (auto it = streams.begin(); it != streams.end(); ++it) {
        (*it)->accumulate_all(env, acc);
    }
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
    batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env);
    {
        profile::sampler_t sampler("Evaluating stream eagerly.", env->trace);
        while (counted_t<const datum_t> d = next(env, batchspec)) {
            arr.add(d);
            sampler.new_sample();
        }
    }
    return arr.to_counted();
}

bool union_datum_stream_t::is_exhausted() const {
    for (auto it = streams.begin(); it != streams.end(); ++it) {
        if (!(*it)->is_exhausted()) {
            return false;
        }
    }
    return batch_cache_exhausted();
}
bool union_datum_stream_t::is_cfeed() const {
    return is_cfeed_union;
}

std::vector<counted_t<const datum_t> >
union_datum_stream_t::next_batch_impl(env_t *env, const batchspec_t &batchspec) {
    for (; streams_index < streams.size(); ++streams_index) {
        std::vector<counted_t<const datum_t> > batch
            = streams[streams_index]->next_batch(env, batchspec);
        if (batch.size() != 0 || streams[streams_index]->is_cfeed()) {
            return batch;
        }
    }
    return std::vector<counted_t<const datum_t> >();
}

} // namespace ql
