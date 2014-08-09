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

template<class T>
T groups_to_batch(std::map<counted_t<const datum_t>, T, counted_datum_less_t> *g) {
    if (g->size() == 0) {
        return T();
    } else {
        r_sanity_check(g->size() == 1 && !g->begin()->first.has());
        return std::move(g->begin()->second);
    }
}


// RANGE/READGEN STUFF
reader_t::reader_t(
    const real_table_t &_table,
    bool _use_outdated,
    scoped_ptr_t<readgen_t> &&_readgen)
    : table(_table),
      use_outdated(_use_outdated),
      started(false), shards_exhausted(false),
      readgen(std::move(_readgen)),
      active_range(readgen->original_keyrange()),
      items_index(0) { }

void reader_t::add_transformation(transform_variant_t &&tv) {
    r_sanity_check(!started);
    transforms.push_back(std::move(tv));
}

void reader_t::accumulate(env_t *env, eager_acc_t *acc, const terminal_variant_t &tv) {
    r_sanity_check(!started);
    started = shards_exhausted = true;
    batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env);
    read_t read = readgen->terminal_read(transforms, tv, batchspec);
    result_t res = do_read(env, std::move(read)).result;
    acc->add_res(env, &res);
}

void reader_t::accumulate_all(env_t *env, eager_acc_t *acc) {
    r_sanity_check(!started);
    started = true;
    batchspec_t batchspec = batchspec_t::all();
    read_t read = readgen->next_read(active_range, transforms, batchspec);
    rget_read_response_t resp = do_read(env, std::move(read));

    auto rr = boost::get<rget_read_t>(&read.read);
    auto final_key = !reversed(rr->sorting) ? store_key_t::max() : store_key_t::min();
    r_sanity_check(resp.last_key == final_key);
    r_sanity_check(!resp.truncated);
    shards_exhausted = true;

    acc->add_res(env, &resp.result);
}

rget_read_response_t reader_t::do_read(env_t *env, const read_t &read) {
    read_response_t res;
    table.read_with_profile(env, read, &res, use_outdated);
    auto rget_res = boost::get<rget_read_response_t>(&res.response);
    r_sanity_check(rget_res != NULL);
    if (auto e = boost::get<exc_t>(&rget_res->result)) {
        throw *e;
    }
    return std::move(*rget_res);
}

std::vector<rget_item_t> reader_t::do_range_read(
        env_t *env, const read_t &read) {
    rget_read_response_t res = do_read(env, read);

    // It's called `do_range_read`.  If we have more than one type of range
    // read (which we might; rget_read_t should arguably be two types), this
    // will have to be a visitor.
    auto rr = boost::get<rget_read_t>(&read.read);
    r_sanity_check(rr);
    const key_range_t &rng = rr->sindex? rr->sindex->region.inner : rr->region.inner;

    // We need to do some adjustments to the last considered key so that we
    // update the range correctly in the case where we're reading a subportion
    // of the total range.
    store_key_t *key = &res.last_key;
    if (*key == store_key_t::max() && !reversed(rr->sorting)) {
        if (!rng.right.unbounded) {
            *key = rng.right.key;
            bool b = key->decrement();
            r_sanity_check(b);
        }
    } else if (*key == store_key_t::min() && reversed(rr->sorting)) {
        *key = rng.left;
    }

    shards_exhausted = readgen->update_range(&active_range, res.last_key);
    grouped_t<stream_t> *gs = boost::get<grouped_t<stream_t> >(&res.result);

    // groups_to_batch asserts that underlying_map has 0 or 1 elements, so it is
    // correct to declare that the order doesn't matter.
    return groups_to_batch(gs->get_underlying_map(grouped::order_doesnt_matter_t()));
}

bool reader_t::load_items(env_t *env, const batchspec_t &batchspec) {
    started = true;
    if (items_index >= items.size() && !shards_exhausted) { // read some more
        items_index = 0;
        items = do_range_read(
                env, readgen->next_read(active_range, transforms, batchspec));
        // Everything below this point can handle `items` being empty (this is
        // good hygiene anyway).
        while (boost::optional<read_t> read = readgen->sindex_sort_read(
                   active_range, items, transforms, batchspec)) {
            std::vector<rget_item_t> new_items = do_range_read(env, *read);
            if (new_items.size() == 0) {
                break;
            }

            rcheck_datum(
                (items.size() + new_items.size()) <= env->limits.array_size_limit(),
                base_exc_t::GENERIC,
                strprintf("Too many rows (> %zu) with the same "
                          "truncated key for index `%s`.  "
                          "Example value:\n%s\n"
                          "Truncated key:\n%s",
                          env->limits.array_size_limit(),
                          readgen->sindex_name().c_str(),
                          items[items.size() - 1].sindex_key->trunc_print().c_str(),
                          key_to_debug_str(items[items.size() - 1].key).c_str()));

            items.reserve(items.size() + new_items.size());
            std::move(new_items.begin(), new_items.end(), std::back_inserter(items));
        }
        readgen->sindex_sort(&items);
    }
    if (items_index >= items.size()) {
        shards_exhausted = true;
    }
    return items_index < items.size();
}

std::vector<counted_t<const datum_t> >
reader_t::next_batch(env_t *env, const batchspec_t &batchspec) {
    started = true;
    if (!load_items(env, batchspec)) {
        return std::vector<counted_t<const datum_t> >();
    }
    r_sanity_check(items_index < items.size());

    std::vector<counted_t<const datum_t> > res;
    switch (batchspec.get_batch_type()) {
    case batch_type_t::NORMAL: // fallthru
    case batch_type_t::NORMAL_FIRST: // fallthru
    case batch_type_t::TERMINAL: {
        res.reserve(items.size() - items_index);
        for (; items_index < items.size(); ++items_index) {
            res.push_back(std::move(items[items_index].data));
        }
    } break;
    case batch_type_t::SINDEX_CONSTANT: {
        counted_t<const ql::datum_t> sindex = std::move(items[items_index].sindex_key);
        store_key_t key = std::move(items[items_index].key);
        res.push_back(std::move(items[items_index].data));
        items_index += 1;

        bool maybe_more_with_sindex = true;
        while (maybe_more_with_sindex) {
            for (; items_index < items.size(); ++items_index) {
                if (sindex.has()) {
                    r_sanity_check(items[items_index].sindex_key.has());
                    if (*items[items_index].sindex_key != *sindex) {
                        break; // batch is done
                    }
                } else {
                    r_sanity_check(!items[items_index].sindex_key.has());
                    if (items[items_index].key != key) {
                        break;
                    }
                }
                res.push_back(std::move(items[items_index].data));

                rcheck_datum(
                    res.size() <= env->limits.array_size_limit(), base_exc_t::GENERIC,
                    strprintf("Too many rows (> %zu) with the same value "
                              "for index `%s`:\n%s",
                              env->limits.array_size_limit(),
                              readgen->sindex_name().c_str(),
                              // This is safe because you can't have duplicate
                              // primary keys, so they will never exceed the
                              // array limit.
                              sindex->trunc_print().c_str()));
            }
            if (items_index >= items.size()) {
                // If we consumed the whole batch without finding a new sindex,
                // we might have more rows with the same sindex in the next
                // batch, which we promptly load.
                maybe_more_with_sindex = load_items(env, batchspec);
            } else {
                maybe_more_with_sindex = false;
            }
        }
    } break;
    default: unreachable();
    }

    if (items_index >= items.size()) { // free memory immediately
        items_index = 0;
        std::vector<rget_item_t> tmp;
        tmp.swap(items);
    }

    shards_exhausted = (res.size() == 0) ? true : shards_exhausted;
    return res;
}

bool reader_t::is_finished() const {
    return shards_exhausted && items_index >= items.size();
}

readgen_t::readgen_t(
    const std::map<std::string, wire_func_t> &_global_optargs,
    std::string _table_name,
    const datum_range_t &_original_datum_range,
    profile_bool_t _profile,
    sorting_t _sorting)
    : global_optargs(_global_optargs),
      table_name(std::move(_table_name)),
      original_datum_range(_original_datum_range),
      profile(_profile),
      sorting(_sorting) { }

bool readgen_t::update_range(key_range_t *active_range,
                             const store_key_t &last_key) const {
    if (!reversed(sorting)) {
        active_range->left = last_key;
    } else {
        active_range->right = key_range_t::right_bound_t(last_key);
    }

    // TODO: mixing these non-const operations INTO THE CONDITIONAL is bad, and
    // confused me (mlucy) for a while when I tried moving some stuff around.
    if (!reversed(sorting)) {
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
    return active_range->is_empty();
}

read_t readgen_t::next_read(
    const key_range_t &active_range,
    const std::vector<transform_variant_t> &transforms,
    const batchspec_t &batchspec) const {
    return read_t(next_read_impl(active_range, transforms, batchspec), profile);
}

// TODO: this is how we did it before, but it sucks.
read_t readgen_t::terminal_read(
    const std::vector<transform_variant_t> &transforms,
    const terminal_variant_t &_terminal,
    const batchspec_t &batchspec) const {
    rget_read_t read = next_read_impl(original_keyrange(), transforms, batchspec);
    read.terminal = _terminal;
    return read_t(read, profile);
}

primary_readgen_t::primary_readgen_t(
    const std::map<std::string, wire_func_t> &global_optargs,
    std::string table_name,
    datum_range_t range,
    profile_bool_t profile,
    sorting_t sorting)
    : readgen_t(global_optargs, std::move(table_name), range, profile, sorting) { }

scoped_ptr_t<readgen_t> primary_readgen_t::make(
    env_t *env,
    std::string table_name,
    datum_range_t range,
    sorting_t sorting) {
    return scoped_ptr_t<readgen_t>(
        new primary_readgen_t(
            env->global_optargs.get_all_optargs(),
            std::move(table_name),
            range,
            env->profile(),
            sorting));
}

rget_read_t primary_readgen_t::next_read_impl(
    const key_range_t &active_range,
    const std::vector<transform_variant_t> &transforms,
    const batchspec_t &batchspec) const {
    return rget_read_t(
        region_t(active_range),
        global_optargs,
        table_name,
        batchspec,
        transforms,
        boost::optional<terminal_variant_t>(),
        boost::optional<sindex_rangespec_t>(),
        sorting);
}

// We never need to do an sindex sort when indexing by a primary key.
boost::optional<read_t> primary_readgen_t::sindex_sort_read(
    UNUSED const key_range_t &active_range,
    UNUSED const std::vector<rget_item_t> &items,
    UNUSED const std::vector<transform_variant_t> &transforms,
    UNUSED const batchspec_t &batchspec) const {
    return boost::optional<read_t>();
}
void primary_readgen_t::sindex_sort(UNUSED std::vector<rget_item_t> *vec) const {
    return;
}

key_range_t primary_readgen_t::original_keyrange() const {
    return original_datum_range.to_primary_keyrange();
}

std::string primary_readgen_t::sindex_name() const {
    return "";
}

sindex_readgen_t::sindex_readgen_t(
    const std::map<std::string, wire_func_t> &global_optargs,
    std::string table_name,
    const std::string &_sindex,
    datum_range_t range,
    profile_bool_t profile,
    sorting_t sorting)
    : readgen_t(global_optargs, std::move(table_name), range, profile, sorting),
      sindex(_sindex) { }

scoped_ptr_t<readgen_t> sindex_readgen_t::make(
    env_t *env,
    std::string table_name,
    const std::string &sindex,
    datum_range_t range,
    sorting_t sorting) {
    return scoped_ptr_t<readgen_t>(
        new sindex_readgen_t(
            env->global_optargs.get_all_optargs(),
            std::move(table_name),
            sindex,
            range,
            env->profile(),
            sorting));
}

class sindex_compare_t {
public:
    explicit sindex_compare_t(sorting_t _sorting)
        : sorting(_sorting) { }
    bool operator()(const rget_item_t &l, const rget_item_t &r) {
        r_sanity_check(l.sindex_key.has() && r.sindex_key.has());

        // We don't have to worry about v1.13.x because there's no way this is
        // running inside of a secondary index function.  Also, in case you're
        // wondering, it's okay for this ordering to be different from the buggy
        // secondary index key ordering that existed in v1.13.  It was different in
        // v1.13 itself.  For that, we use the last_key value in the
        // rget_read_response_t.
        return reversed(sorting)
            ? l.sindex_key->compare_gt(reql_version_t::LATEST, *r.sindex_key)
            : l.sindex_key->compare_lt(reql_version_t::LATEST, *r.sindex_key);
    }
private:
    sorting_t sorting;
};

void sindex_readgen_t::sindex_sort(std::vector<rget_item_t> *vec) const {
    if (vec->size() == 0) {
        return;
    }
    if (sorting != sorting_t::UNORDERED) {
        std::stable_sort(vec->begin(), vec->end(), sindex_compare_t(sorting));
    }
}

rget_read_t sindex_readgen_t::next_read_impl(
    const key_range_t &active_range,
    const std::vector<transform_variant_t> &transforms,
    const batchspec_t &batchspec) const {
    return rget_read_t(
        region_t::universe(),
        global_optargs,
        table_name,
        batchspec,
        transforms,
        boost::optional<terminal_variant_t>(),
        sindex_rangespec_t(sindex, region_t(active_range), original_datum_range),
        sorting);
}

boost::optional<read_t> sindex_readgen_t::sindex_sort_read(
    const key_range_t &active_range,
    const std::vector<rget_item_t> &items,
    const std::vector<transform_variant_t> &transforms,
    const batchspec_t &batchspec) const {

    if (sorting != sorting_t::UNORDERED && items.size() > 0) {
        const store_key_t &key = items[items.size() - 1].key;
        if (datum_t::key_is_truncated(key)) {
            std::string skey = datum_t::extract_secondary(key_to_unescaped_str(key));
            key_range_t rng = active_range;
            if (!reversed(sorting)) {
                // We construct a right bound that's larger than the maximum
                // possible row with this truncated sindex but smaller than the
                // minimum possible row with a larger sindex.
                rng.right = key_range_t::right_bound_t(
                    store_key_t(skey + std::string(MAX_KEY_SIZE - skey.size(), 0xFF)));
            } else {
                // We construct a left bound that's smaller than the minimum
                // possible row with this truncated sindex but larger than the
                // maximum possible row with a smaller sindex.
                rng.left = store_key_t(skey);
            }
            if (rng.right.unbounded || rng.left < rng.right.key) {
                return read_t(
                    rget_read_t(
                        region_t::universe(),
                        global_optargs,
                        table_name,
                        batchspec.with_new_batch_type(batch_type_t::SINDEX_CONSTANT),
                        transforms,
                        boost::optional<terminal_variant_t>(),
                        sindex_rangespec_t(
                            sindex,
                            region_t(key_range_t(rng)),
                            original_datum_range),
                        sorting),
                    profile);
            }
        }
    }
    return boost::optional<read_t>();
}

key_range_t sindex_readgen_t::original_keyrange() const {
    return original_datum_range.to_sindex_keyrange();
}

std::string sindex_readgen_t::sindex_name() const {
    return sindex;
}

counted_t<val_t> datum_stream_t::run_terminal(
    env_t *env, const terminal_variant_t &tv) {
    scoped_ptr_t<eager_acc_t> acc(make_eager_terminal(tv));
    accumulate(env, acc.get(), tv);
    return acc->finish_eager(backtrace(), is_grouped(), env->limits);
}

counted_t<val_t> datum_stream_t::to_array(env_t *env) {
    scoped_ptr_t<eager_acc_t> acc = make_to_array(env->reql_version);
    accumulate_all(env, acc.get());
    return acc->finish_eager(backtrace(), is_grouped(), env->limits);
}

// DATUM_STREAM_T
counted_t<datum_stream_t> datum_stream_t::slice(size_t l, size_t r) {
    return make_counted<slice_datum_stream_t>(l, r, this->counted_from_this());
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
    // I'm guessing reql_version doesn't matter here, but why think about it?  We use
    // th env's reql_version.
    groups_t data(counted_datum_less_t(env->reql_version));
    while (next_grouped_batch(env, bs, &data) == done_t::NO) {
        (*acc)(env, &data);
    }
}

void eager_datum_stream_t::accumulate_all(env_t *env, eager_acc_t *acc) {
    groups_t data(counted_datum_less_t(env->reql_version));
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
    groups_t data(counted_datum_less_t(env->reql_version));
    next_grouped_batch(env, bs, &data);
    return groups_to_batch(&data);
}

counted_t<const datum_t> eager_datum_stream_t::as_array(env_t *env) {
    if (is_grouped() || !is_array()) {
        return counted_t<const datum_t>();
    }

    datum_array_builder_t arr(env->limits);
    batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env);
    {
        profile::sampler_t sampler("Evaluating stream eagerly.", env->trace);
        while (counted_t<const datum_t> d = next(env, batchspec)) {
            arr.add(d);
            sampler.new_sample();
        }
    }
    return std::move(arr).to_counted();
}

// LAZY_DATUM_STREAM_T
lazy_datum_stream_t::lazy_datum_stream_t(
    const real_table_t &_table,
    bool use_outdated,
    scoped_ptr_t<readgen_t> &&readgen,
    const protob_t<const Backtrace> &bt_src)
    : datum_stream_t(bt_src),
      current_batch_offset(0),
      reader(_table, use_outdated, std::move(readgen)) { }

void lazy_datum_stream_t::add_transformation(transform_variant_t &&tv,
                                             const protob_t<const Backtrace> &bt) {
    reader.add_transformation(std::move(tv));
    update_bt(bt);
}

void lazy_datum_stream_t::accumulate(
    env_t *env, eager_acc_t *acc, const terminal_variant_t &tv) {
    reader.accumulate(env, acc, tv);
}

void lazy_datum_stream_t::accumulate_all(env_t *env, eager_acc_t *acc) {
    reader.accumulate_all(env, acc);
}

std::vector<counted_t<const datum_t> >
lazy_datum_stream_t::next_batch_impl(env_t *env, const batchspec_t &batchspec) {
    // Should never mix `next` with `next_batch`.
    r_sanity_check(current_batch_offset == 0 && current_batch.size() == 0);
    return reader.next_batch(env, batchspec);
}

bool lazy_datum_stream_t::is_exhausted() const {
    return reader.is_finished() && batch_cache_exhausted();
}
bool lazy_datum_stream_t::is_cfeed() const {
    return false;
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

// UNION_DATUM_STREAM_T
void union_datum_stream_t::add_transformation(transform_variant_t &&tv,
                                              const protob_t<const Backtrace> &bt) {
    for (auto it = streams.begin(); it != streams.end(); ++it) {
        (*it)->add_transformation(transform_variant_t(tv), bt);
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
    datum_array_builder_t arr(env->limits);
    batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env);
    {
        profile::sampler_t sampler("Evaluating stream eagerly.", env->trace);
        while (counted_t<const datum_t> d = next(env, batchspec)) {
            arr.add(d);
            sampler.new_sample();
        }
    }
    return std::move(arr).to_counted();
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
