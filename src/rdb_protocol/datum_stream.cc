// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/datum_stream.hpp"

#include <map>

#include "boost_utils.hpp"
#include "rdb_protocol/batching.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/val.hpp"
#include "utils.hpp"

#include "debug.hpp"

namespace ql {

// RANGE/READGEN STUFF
rget_response_reader_t::rget_response_reader_t(
    const counted_t<real_table_t> &_table,
    bool _use_outdated,
    scoped_ptr_t<readgen_t> &&_readgen)
    : table(_table),
      use_outdated(_use_outdated),
      started(false), shards_exhausted(false),
      readgen(std::move(_readgen)),
      active_range(readgen->original_keyrange()),
      items_index(0) { }

void rget_response_reader_t::add_transformation(transform_variant_t &&tv) {
    r_sanity_check(!started);
    transforms.push_back(std::move(tv));
}

void rget_response_reader_t::accumulate(env_t *env, eager_acc_t *acc,
                                        const terminal_variant_t &tv) {
    r_sanity_check(!started);
    started = shards_exhausted = true;
    batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env);
    read_t read = readgen->terminal_read(transforms, tv, batchspec);
    result_t res = do_read(env, std::move(read)).result;
    acc->add_res(env, &res);
}

std::vector<datum_t> rget_response_reader_t::next_batch(env_t *env,
                                                        const batchspec_t &batchspec) {
    started = true;
    if (!load_items(env, batchspec)) {
        return std::vector<datum_t>();
    }
    r_sanity_check(items_index < items.size());

    std::vector<datum_t> res;
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
        ql::datum_t sindex = std::move(items[items_index].sindex_key);
        store_key_t key = std::move(items[items_index].key);
        res.push_back(std::move(items[items_index].data));
        items_index += 1;

        bool maybe_more_with_sindex = true;
        while (maybe_more_with_sindex) {
            for (; items_index < items.size(); ++items_index) {
                if (sindex.has()) {
                    r_sanity_check(items[items_index].sindex_key.has());
                    if (items[items_index].sindex_key != sindex) {
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
                    res.size() <= env->limits().array_size_limit(), base_exc_t::GENERIC,
                    strprintf("Too many rows (> %zu) with the same value "
                              "for index `%s`:\n%s",
                              env->limits().array_size_limit(),
                              opt_or(readgen->sindex_name(), "").c_str(),
                              // This is safe because you can't have duplicate
                              // primary keys, so they will never exceed the
                              // array limit.
                              sindex.trunc_print().c_str()));
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

bool rget_response_reader_t::is_finished() const {
    return shards_exhausted && items_index >= items.size();
}

rget_read_response_t rget_response_reader_t::do_read(env_t *env, const read_t &read) {
    read_response_t res;
    table->read_with_profile(env, read, &res, use_outdated);
    auto rget_res = boost::get<rget_read_response_t>(&res.response);
    r_sanity_check(rget_res != NULL);
    if (auto e = boost::get<exc_t>(&rget_res->result)) {
        throw *e;
    }
    return std::move(*rget_res);
}

rget_reader_t::rget_reader_t(
    const counted_t<real_table_t> &_table,
    bool _use_outdated,
    scoped_ptr_t<readgen_t> &&_readgen)
    : rget_response_reader_t(_table, _use_outdated, std::move(_readgen)) { }

void rget_reader_t::accumulate_all(env_t *env, eager_acc_t *acc) {
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

std::vector<rget_item_t> rget_reader_t::do_range_read(
        env_t *env, const read_t &read) {
    rget_read_response_t res = do_read(env, read);

    auto rr = boost::get<rget_read_t>(&read.read);
    r_sanity_check(rr);

    key_range_t rng;
    if (rr->sindex) {
        if (!active_range) {
            r_sanity_check(!rr->sindex->region);
            active_range = rng = readgen->sindex_keyrange(res.skey_version);
        } else {
            r_sanity_check(rr->sindex->region);
            rng = (*rr->sindex->region).inner;
        }
    } else {
        rng = rr->region.inner;
    }

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

    r_sanity_check(active_range);
    shards_exhausted = readgen->update_range(&*active_range, res.last_key);
    grouped_t<stream_t> *gs = boost::get<grouped_t<stream_t> >(&res.result);

    // groups_to_batch asserts that underlying_map has 0 or 1 elements, so it is
    // correct to declare that the order doesn't matter.
    return groups_to_batch(gs->get_underlying_map(grouped::order_doesnt_matter_t()));
}

bool rget_reader_t::load_items(env_t *env, const batchspec_t &batchspec) {
    started = true;
    if (items_index >= items.size() && !shards_exhausted) { // read some more
        items_index = 0;
        // `active_range` is guaranteed to be full after the `do_range_read`,
        // because `do_range_read` is responsible for updating the active range.
        items = do_range_read(
                env, readgen->next_read(active_range, transforms, batchspec));
        // Everything below this point can handle `items` being empty (this is
        // good hygiene anyway).
        r_sanity_check(active_range);
        while (boost::optional<read_t> read = readgen->sindex_sort_read(
                   *active_range, items, transforms, batchspec)) {
            std::vector<rget_item_t> new_items = do_range_read(env, *read);
            if (new_items.size() == 0) {
                break;
            }

            rcheck_datum(
                (items.size() + new_items.size()) <= env->limits().array_size_limit(),
                base_exc_t::GENERIC,
                strprintf("Too many rows (> %zu) with the same "
                          "truncated key for index `%s`.  "
                          "Example value:\n%s\n"
                          "Truncated key:\n%s",
                          env->limits().array_size_limit(),
                          opt_or(readgen->sindex_name(), "").c_str(),
                          items[items.size() - 1].sindex_key.trunc_print().c_str(),
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

intersecting_reader_t::intersecting_reader_t(
    const counted_t<real_table_t> &_table,
    bool _use_outdated,
    scoped_ptr_t<readgen_t> &&_readgen)
    : rget_response_reader_t(_table, _use_outdated, std::move(_readgen)) { }

void intersecting_reader_t::accumulate_all(env_t *env, eager_acc_t *acc) {
    r_sanity_check(!started);
    started = true;
    batchspec_t batchspec = batchspec_t::all();
    read_t read = readgen->next_read(active_range, transforms, batchspec);
    rget_read_response_t resp = do_read(env, std::move(read));

    auto final_key = store_key_t::max();
    r_sanity_check(resp.last_key == final_key);
    r_sanity_check(!resp.truncated);
    shards_exhausted = true;

    acc->add_res(env, &resp.result);
}

bool intersecting_reader_t::load_items(env_t *env, const batchspec_t &batchspec) {
    started = true;
    while (items_index >= items.size() && !shards_exhausted) { // read some more
        std::vector<rget_item_t> unfiltered_items = do_intersecting_read(
                env, readgen->next_read(active_range, transforms, batchspec));
        if (unfiltered_items.empty()) {
            shards_exhausted = true;
        } else {
            items_index = 0;
            items.clear();
            items.reserve(unfiltered_items.size());
            for (size_t i = 0; i < unfiltered_items.size(); ++i) {
                r_sanity_check(unfiltered_items[i].key.size() > 0);
                store_key_t pkey(ql::datum_t::extract_primary(unfiltered_items[i].key));
                if (processed_pkeys.count(pkey) == 0) {
                    if (processed_pkeys.size() >= env->limits().array_size_limit()) {
                        throw ql::exc_t(ql::base_exc_t::GENERIC,
                            "Array size limit exceeded during geospatial index "
                            "traversal.", NULL);
                    }
                    processed_pkeys.insert(pkey);
                    items.push_back(std::move(unfiltered_items[i]));
                }
            }
        }
    }
    return items_index < items.size();
}

std::vector<rget_item_t> intersecting_reader_t::do_intersecting_read(
        env_t *env, const read_t &read) {
    rget_read_response_t res = do_read(env, read);

    auto gr = boost::get<intersecting_geo_read_t>(&read.read);
    r_sanity_check(gr);

    r_sanity_check(active_range);
    r_sanity_check(gr->sindex.region);
    const key_range_t &rng = (*gr->sindex.region).inner;

    // We need to do some adjustments to the last considered key so that we
    // update the range correctly in the case where we're reading a subportion
    // of the total range.
    store_key_t *key = &res.last_key;
    if (*key == store_key_t::max()) {
        if (!rng.right.unbounded) {
            *key = rng.right.key;
            bool b = key->decrement();
            r_sanity_check(b);
        }
    }

    shards_exhausted = readgen->update_range(&*active_range, res.last_key);
    grouped_t<stream_t> *gs = boost::get<grouped_t<stream_t> >(&res.result);

    // groups_to_batch asserts that underlying_map has 0 or 1 elements, so it is
    // correct to declare that the order doesn't matter.
    return groups_to_batch(gs->get_underlying_map(grouped::order_doesnt_matter_t()));
}

readgen_t::readgen_t(
    const std::map<std::string, wire_func_t> &_global_optargs,
    std::string _table_name,
    profile_bool_t _profile,
    sorting_t _sorting)
    : global_optargs(_global_optargs),
      table_name(std::move(_table_name)),
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

rget_readgen_t::rget_readgen_t(
    const std::map<std::string, wire_func_t> &_global_optargs,
    std::string _table_name,
    const datum_range_t &_original_datum_range,
    profile_bool_t _profile,
    sorting_t _sorting)
    : readgen_t(_global_optargs, std::move(_table_name), _profile, _sorting),
      original_datum_range(_original_datum_range) { }

read_t rget_readgen_t::next_read(
    const boost::optional<key_range_t> &active_range,
    const std::vector<transform_variant_t> &transforms,
    const batchspec_t &batchspec) const {
    return read_t(next_read_impl(active_range, transforms, batchspec), profile);
}

// TODO: this is how we did it before, but it sucks.
read_t rget_readgen_t::terminal_read(
    const std::vector<transform_variant_t> &transforms,
    const terminal_variant_t &_terminal,
    const batchspec_t &batchspec) const {
    rget_read_t read = next_read_impl(
        original_keyrange(), transforms, batchspec);
    read.terminal = _terminal;
    return read_t(read, profile);
}

primary_readgen_t::primary_readgen_t(
    const std::map<std::string, wire_func_t> &global_optargs,
    std::string table_name,
    datum_range_t range,
    profile_bool_t profile,
    sorting_t sorting)
    : rget_readgen_t(global_optargs, std::move(table_name), range, profile, sorting) { }

scoped_ptr_t<readgen_t> primary_readgen_t::make(
    env_t *env,
    std::string table_name,
    datum_range_t range,
    sorting_t sorting) {
    return scoped_ptr_t<readgen_t>(
        new primary_readgen_t(
            env->get_all_optargs(),
            std::move(table_name),
            range,
            env->profile(),
            sorting));
}

rget_read_t primary_readgen_t::next_read_impl(
    const boost::optional<key_range_t> &active_range,
    const std::vector<transform_variant_t> &transforms,
    const batchspec_t &batchspec) const {
    r_sanity_check(active_range);
    return rget_read_t(
        region_t(*active_range),
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

boost::optional<key_range_t> primary_readgen_t::original_keyrange() const {
    return original_datum_range.to_primary_keyrange();
}

key_range_t primary_readgen_t::sindex_keyrange(skey_version_t) const {
    crash("Cannot call `sindex_keyrange` on a primary readgen (internal server error).");
}

boost::optional<std::string> primary_readgen_t::sindex_name() const {
    return boost::optional<std::string>();
}

sindex_readgen_t::sindex_readgen_t(
    const std::map<std::string, wire_func_t> &global_optargs,
    std::string table_name,
    const std::string &_sindex,
    datum_range_t range,
    profile_bool_t profile,
    sorting_t sorting)
    : rget_readgen_t(global_optargs, std::move(table_name), range, profile, sorting),
      sindex(_sindex),
      sent_first_read(false) { }

scoped_ptr_t<readgen_t> sindex_readgen_t::make(
    env_t *env,
    std::string table_name,
    const std::string &sindex,
    datum_range_t range,
    sorting_t sorting) {
    return scoped_ptr_t<readgen_t>(
        new sindex_readgen_t(
            env->get_all_optargs(),
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
        if (l.sindex_key == r.sindex_key) {
            return reversed(sorting)
                ? datum_t::extract_primary(l.key) > datum_t::extract_primary(r.key)
                : datum_t::extract_primary(l.key) < datum_t::extract_primary(r.key);
        } else {
            return reversed(sorting)
                ? l.sindex_key.compare_gt(reql_version_t::LATEST, r.sindex_key)
                : l.sindex_key.compare_lt(reql_version_t::LATEST, r.sindex_key);
        }
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
    const boost::optional<key_range_t> &active_range,
    const std::vector<transform_variant_t> &transforms,
    const batchspec_t &batchspec) const {
    boost::optional<region_t> region;
    if (active_range) {
        region = region_t(*active_range);
    } else {
        // We should send at most one read before we're able to calculate the
        // active range.
        r_sanity_check(!sent_first_read);
        // We cheat here because we're just using this for an assert.  In terms
        // of actual behavior, the function is const, and this assert will go
        // away once we drop support for pre-1.16 sindex key skey_version.
        const_cast<sindex_readgen_t *>(this)->sent_first_read = true;
    }
    return rget_read_t(
        region_t::universe(),
        global_optargs,
        table_name,
        batchspec,
        transforms,
        boost::optional<terminal_variant_t>(),
        sindex_rangespec_t(sindex, std::move(region), original_datum_range),
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
            // We need to truncate the skey down to the smallest *guaranteed*
            // length of secondary index keys in the btree.
            // This is important because the prefix of a truncated sindex key
            // that's actually getting stored can vary for different documents
            // even with the same key, if their primary key is of different lengths.
            // If we didn't do that, the search range which we construct in the
            // next step might miss some relevant keys that have been truncated
            // differently. (the lack of this was the cause of
            // https://github.com/rethinkdb/rethinkdb/issues/3444)
            std::string skey =
                datum_t::extract_truncated_secondary(key_to_unescaped_str(key));
            key_range_t rng = active_range;
            if (!reversed(sorting)) {
                // We construct a right bound that's larger than the maximum
                // possible row with this truncated sindex.
                rng.right = key_range_t::right_bound_t(
                    store_key_t(skey + std::string(MAX_KEY_SIZE - skey.size(), 0xFF)));
            } else {
                // We construct a left bound that's smaller than the minimum
                // possible row with this truncated sindex.
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

boost::optional<key_range_t> sindex_readgen_t::original_keyrange() const {
    return boost::none;
}

key_range_t sindex_readgen_t::sindex_keyrange(skey_version_t skey_version) const {
    return original_datum_range.to_sindex_keyrange(skey_version);
}

boost::optional<std::string> sindex_readgen_t::sindex_name() const {
    return sindex;
}

intersecting_readgen_t::intersecting_readgen_t(
    const std::map<std::string, wire_func_t> &global_optargs,
    std::string table_name,
    const std::string &_sindex,
    const datum_t &_query_geometry,
    profile_bool_t profile)
    : readgen_t(global_optargs, std::move(table_name), profile, sorting_t::UNORDERED),
      sindex(_sindex),
      query_geometry(_query_geometry) { }

scoped_ptr_t<readgen_t> intersecting_readgen_t::make(
    env_t *env,
    std::string table_name,
    const std::string &sindex,
    const datum_t &query_geometry) {
    return scoped_ptr_t<readgen_t>(
        new intersecting_readgen_t(
            env->get_all_optargs(),
            std::move(table_name),
            sindex,
            query_geometry,
            env->profile()));
}

read_t intersecting_readgen_t::next_read(
    const boost::optional<key_range_t> &active_range,
    const std::vector<transform_variant_t> &transforms,
    const batchspec_t &batchspec) const {
    return read_t(next_read_impl(active_range, transforms, batchspec), profile);
}

read_t intersecting_readgen_t::terminal_read(
    const std::vector<transform_variant_t> &transforms,
    const terminal_variant_t &_terminal,
    const batchspec_t &batchspec) const {
    intersecting_geo_read_t read =
        next_read_impl(original_keyrange(), transforms, batchspec);
    read.terminal = _terminal;
    return read_t(read, profile);
}

intersecting_geo_read_t intersecting_readgen_t::next_read_impl(
    const boost::optional<key_range_t> &active_range,
    const std::vector<transform_variant_t> &transforms,
    const batchspec_t &batchspec) const {
    r_sanity_check(active_range);
    return intersecting_geo_read_t(
        region_t::universe(),
        global_optargs,
        table_name,
        batchspec,
        transforms,
        boost::optional<terminal_variant_t>(),
        sindex_rangespec_t(sindex, region_t(*active_range), datum_range_t::universe()),
        query_geometry);
}

boost::optional<read_t> intersecting_readgen_t::sindex_sort_read(
    UNUSED const key_range_t &active_range,
    UNUSED const std::vector<rget_item_t> &items,
    UNUSED const std::vector<transform_variant_t> &transforms,
    UNUSED const batchspec_t &batchspec) const {
    // Intersection queries don't support sorting
    return boost::optional<read_t>();
}

void intersecting_readgen_t::sindex_sort(UNUSED std::vector<rget_item_t> *vec) const {
    // No sorting required for intersection queries, since they don't
    // support any specific ordering.
}

boost::optional<key_range_t> intersecting_readgen_t::original_keyrange() const {
    // This is always universe for intersection reads.
    // The real query is in the query geometry.

    // We can use whatever skey_version we want here because `universe`
    // becomes the same key range anyway.
    return datum_range_t::universe().to_sindex_keyrange(skey_version_t::post_1_16);
}

key_range_t intersecting_readgen_t::sindex_keyrange(
    skey_version_t skey_version) const {
    // This is always universe for intersection reads.
    // The real query is in the query geometry.
    return datum_range_t::universe().to_sindex_keyrange(skey_version);
}

boost::optional<std::string> intersecting_readgen_t::sindex_name() const {
    return sindex;
}

scoped_ptr_t<val_t> datum_stream_t::run_terminal(
    env_t *env, const terminal_variant_t &tv) {
    scoped_ptr_t<eager_acc_t> acc(make_eager_terminal(tv));
    accumulate(env, acc.get(), tv);
    return acc->finish_eager(backtrace(), is_grouped(), env->limits());
}

scoped_ptr_t<val_t> datum_stream_t::to_array(env_t *env) {
    scoped_ptr_t<eager_acc_t> acc = make_to_array(env->reql_version());
    accumulate_all(env, acc.get());
    return acc->finish_eager(backtrace(), is_grouped(), env->limits());
}

// DATUM_STREAM_T
counted_t<datum_stream_t> datum_stream_t::slice(size_t l, size_t r) {
    return make_counted<slice_datum_stream_t>(l, r, this->counted_from_this());
}
counted_t<datum_stream_t> datum_stream_t::indexes_of(counted_t<const func_t> f) {
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

std::vector<datum_t>
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

datum_t datum_stream_t::next(
    env_t *env, const batchspec_t &batchspec) {

    profile::starter_t("Reading element from datum stream.", env->trace);
    if (batch_cache_index >= batch_cache.size()) {
        r_sanity_check(batch_cache_index == 0);
        batch_cache = next_batch(env, batchspec);
        if (batch_cache_index >= batch_cache.size()) {
            return datum_t();
        }
    }
    r_sanity_check(batch_cache_index < batch_cache.size());
    datum_t d = std::move(batch_cache[batch_cache_index++]);
    if (batch_cache_index >= batch_cache.size()) {
        // Free the vector as soon as we're done with it.  This also keeps the
        // assert in `next_batch` happy.
        batch_cache_index = 0;
        std::vector<datum_t> tmp;
        tmp.swap(batch_cache);
    }
    return d;
}

bool datum_stream_t::batch_cache_exhausted() const {
    return batch_cache_index >= batch_cache.size();
}

void eager_datum_stream_t::add_transformation(
    transform_variant_t &&tv, const protob_t<const Backtrace> &bt) {
    ops.push_back(make_op(tv));
    transforms.push_back(std::move(tv));
    update_bt(bt);
}

eager_datum_stream_t::done_t eager_datum_stream_t::next_grouped_batch(
    env_t *env, const batchspec_t &bs, groups_t *out) {
    r_sanity_check(out->size() == 0);
    while (out->size() == 0) {
        std::vector<datum_t> v = next_raw_batch(env, bs);
        if (v.size() == 0) {
            return done_t::YES;
        }
        (*out)[datum_t()] = std::move(v);
        for (auto it = ops.begin(); it != ops.end(); ++it) {
            (**it)(env, out, datum_t());
        }
    }
    return done_t::NO;
}

void eager_datum_stream_t::accumulate(
    env_t *env, eager_acc_t *acc, const terminal_variant_t &) {
    batchspec_t bs = batchspec_t::user(batch_type_t::TERMINAL, env);
    // I'm guessing reql_version doesn't matter here, but why think about it?  We use
    // th env's reql_version.
    groups_t data(optional_datum_less_t(env->reql_version()));
    while (next_grouped_batch(env, bs, &data) == done_t::NO) {
        (*acc)(env, &data);
    }
}

void eager_datum_stream_t::accumulate_all(env_t *env, eager_acc_t *acc) {
    groups_t data(optional_datum_less_t(env->reql_version()));
    done_t done = next_grouped_batch(env, batchspec_t::all(), &data);
    (*acc)(env, &data);
    if (done == done_t::NO) {
        done_t must_be_yes = next_grouped_batch(env, batchspec_t::all(), &data);
        r_sanity_check(data.size() == 0);
        r_sanity_check(must_be_yes == done_t::YES);
    }
}

std::vector<datum_t>
eager_datum_stream_t::next_batch_impl(env_t *env, const batchspec_t &bs) {
    groups_t data(optional_datum_less_t(env->reql_version()));
    next_grouped_batch(env, bs, &data);
    return groups_to_batch(&data);
}

datum_t eager_datum_stream_t::as_array(env_t *env) {
    if (is_grouped() || !is_array()) {
        return datum_t();
    }

    datum_array_builder_t arr(env->limits());
    batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env);
    {
        profile::sampler_t sampler("Evaluating stream eagerly.", env->trace);
        datum_t d;
        while (d = next(env, batchspec), d.has()) {
            arr.add(d);
            sampler.new_sample();
        }
    }
    return std::move(arr).to_datum();
}

// LAZY_DATUM_STREAM_T
lazy_datum_stream_t::lazy_datum_stream_t(
    scoped_ptr_t<reader_t> &&_reader,
    const protob_t<const Backtrace> &bt_src)
    : datum_stream_t(bt_src),
      current_batch_offset(0),
      reader(std::move(_reader)) { }

void lazy_datum_stream_t::add_transformation(transform_variant_t &&tv,
                                             const protob_t<const Backtrace> &bt) {
    reader->add_transformation(std::move(tv));
    update_bt(bt);
}

void lazy_datum_stream_t::accumulate(
    env_t *env, eager_acc_t *acc, const terminal_variant_t &tv) {
    reader->accumulate(env, acc, tv);
}

void lazy_datum_stream_t::accumulate_all(env_t *env, eager_acc_t *acc) {
    reader->accumulate_all(env, acc);
}

std::vector<datum_t>
lazy_datum_stream_t::next_batch_impl(env_t *env, const batchspec_t &batchspec) {
    // Should never mix `next` with `next_batch`.
    r_sanity_check(current_batch_offset == 0 && current_batch.size() == 0);
    return reader->next_batch(env, batchspec);
}

bool lazy_datum_stream_t::is_exhausted() const {
    return reader->is_finished() && batch_cache_exhausted();
}
feed_type_t lazy_datum_stream_t::cfeed_type() const {
    return feed_type_t::not_feed;
}
bool lazy_datum_stream_t::is_infinite() const {
    return false;
}

array_datum_stream_t::array_datum_stream_t(datum_t _arr,
                                           const protob_t<const Backtrace> &bt_source)
    : eager_datum_stream_t(bt_source), index(0), arr(_arr) { }

datum_t array_datum_stream_t::next(env_t *env, const batchspec_t &bs) {
    return ops_to_do() ? datum_stream_t::next(env, bs) : next_arr_el();
}
datum_t array_datum_stream_t::next_arr_el() {
    return index < arr.arr_size() ? arr.get(index++) : datum_t();
}

bool array_datum_stream_t::is_exhausted() const {
    return index >= arr.arr_size();
}
feed_type_t array_datum_stream_t::cfeed_type() const {
    return feed_type_t::not_feed;
}
bool array_datum_stream_t::is_infinite() const {
    return false;
}

std::vector<datum_t>
array_datum_stream_t::next_raw_batch(env_t *env, const batchspec_t &batchspec) {
    std::vector<datum_t> v;
    batcher_t batcher = batchspec.to_batcher();

    profile::sampler_t sampler("Fetching array elements.", env->trace);
    datum_t d;
    while (d = next_arr_el(), d.has()) {
        batcher.note_el(d);
        v.push_back(std::move(d));
        if (batcher.should_send_batch()) {
            break;
        }
        sampler.new_sample();
    }
    return v;
}

bool array_datum_stream_t::is_array() const {
    return !is_grouped();
}

// INDEXED_SORT_DATUM_STREAM_T
indexed_sort_datum_stream_t::indexed_sort_datum_stream_t(
    counted_t<datum_stream_t> stream,
    std::function<bool(env_t *,  // NOLINT(readability/casting)
                       profile::sampler_t *,
                       const datum_t &,
                       const datum_t &)> _lt_cmp)
    : wrapper_datum_stream_t(stream), lt_cmp(_lt_cmp), index(0) { }

std::vector<datum_t>
indexed_sort_datum_stream_t::next_raw_batch(env_t *env, const batchspec_t &batchspec) {
    std::vector<datum_t> ret;
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

std::vector<datum_t>
ordered_distinct_datum_stream_t::next_raw_batch(env_t *env, const batchspec_t &bs) {
    std::vector<datum_t> ret;
    profile::sampler_t sampler("Ordered distinct.", env->trace);
    while (ret.size() == 0) {
        std::vector<datum_t> v = source->next_batch(env, bs);
        if (v.size() == 0) break;
        for (auto &&el : v) {
            if (!last_val.has() || last_val != el) {
                last_val = el;
                ret.push_back(std::move(el));
            }
            sampler.new_sample();
        }
    }
    return ret;
}

// INDEXES_OF_DATUM_STREAM_T
indexes_of_datum_stream_t::indexes_of_datum_stream_t(counted_t<const func_t> _f,
                                                     counted_t<datum_stream_t> _source)
    : wrapper_datum_stream_t(_source), f(_f), index(0) {
    guarantee(f.has() && source.has());
}

std::vector<datum_t>
indexes_of_datum_stream_t::next_raw_batch(env_t *env, const batchspec_t &bs) {
    std::vector<datum_t> ret;
    profile::sampler_t sampler("Finding indexes_of eagerly.", env->trace);
    while (ret.size() == 0) {
        std::vector<datum_t> v = source->next_batch(env, bs);
        if (v.size() == 0) {
            break;
        }
        for (auto it = v.begin(); it != v.end(); ++it, ++index) {
            if (f->filter_call(env, *it, counted_t<const func_t>())) {
                ret.push_back(datum_t(static_cast<double>(index)));
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

changefeed::keyspec_t slice_datum_stream_t::get_change_spec() {
    if (left == 0) {
        changefeed::keyspec_t subspec = source->get_change_spec();
        auto *rspec = boost::get<changefeed::keyspec_t::range_t>(&subspec.spec);
        if (rspec != NULL) {
            std::copy(transforms.begin(), transforms.end(),
                      std::back_inserter(rspec->transforms));
            rcheck(right <= static_cast<uint64_t>(std::numeric_limits<size_t>::max()),
                   base_exc_t::GENERIC,
                   strprintf("Cannot call `changes` on a slice "
                             "with size > %zu (got %" PRIu64 ").",
                             std::numeric_limits<size_t>::max(),
                             right));
            return changefeed::keyspec_t(
                changefeed::keyspec_t::limit_t{
                    std::move(*rspec),
                    static_cast<size_t>(right)},
                std::move(subspec.table),
                std::move(subspec.table_name));
        }
    }
    return wrapper_datum_stream_t::get_change_spec();
}

std::vector<datum_t>
slice_datum_stream_t::next_raw_batch(env_t *env, const batchspec_t &batchspec) {
    if (left >= right || index >= right) {
        return std::vector<datum_t>();
    }

    batcher_t batcher = batchspec.to_batcher();
    profile::sampler_t sampler("Slicing eagerly.", env->trace);

    std::vector<datum_t> ret;

    while (index < left) {
        sampler.new_sample();
        std::vector<datum_t> v =
            source->next_batch(env, batchspec.with_at_most(right - index));
        if (v.size() == 0) {
            return ret;
        }
        index += v.size();
        if (index > right) {
            v.resize(v.size() - (index - right));
            index = right;
        }

        if (index > left) {
            auto start = v.end() - (index - left);
            for (auto it = start; it != v.end(); ++it) {
                batcher.note_el(*it);
            }
            ret.reserve(index - left);
            std::move(start, v.end(), std::back_inserter(ret));
        }
    }

    while (index < right && !batcher.should_send_batch()) {
        sampler.new_sample();
        std::vector<datum_t> v =
            source->next_batch(env, batchspec.with_at_most(right - index));
        if (v.size() == 0) {
            return ret;
        }
        index += v.size();
        if (index > right) {
            v.resize(v.size() - (index - right));
            index = right;
        }

        for (const auto &d : v) {
            batcher.note_el(d);
        }
        ret.reserve(ret.size() + v.size());
        std::move(v.begin(), v.end(), std::back_inserter(ret));
    }

    r_sanity_check(index >= left);
    r_sanity_check(index <= right);
    return ret;
}

bool slice_datum_stream_t::is_exhausted() const {
    return (left >= right || index >= right || source->is_exhausted())
        && batch_cache_exhausted();
}
feed_type_t slice_datum_stream_t::cfeed_type() const {
    return source->cfeed_type();
}
bool slice_datum_stream_t::is_infinite() const {
    return source->is_infinite() && right == std::numeric_limits<size_t>::max();
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

bool union_datum_stream_t::is_array() const {
    for (auto it = streams.begin(); it != streams.end(); ++it) {
        if (!(*it)->is_array()) {
            return false;
        }
    }
    return true;
}

datum_t union_datum_stream_t::as_array(env_t *env) {
    if (!is_array()) {
        return datum_t();
    }
    datum_array_builder_t arr(env->limits());
    batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env);
    {
        profile::sampler_t sampler("Evaluating stream eagerly.", env->trace);
        datum_t d;
        while (d = next(env, batchspec), d.has()) {
            arr.add(d);
            sampler.new_sample();
        }
    }
    return std::move(arr).to_datum();
}

bool union_datum_stream_t::is_exhausted() const {
    for (auto it = streams.begin(); it != streams.end(); ++it) {
        if (!(*it)->is_exhausted()) {
            return false;
        }
    }
    return batch_cache_exhausted();
}
feed_type_t union_datum_stream_t::cfeed_type() const {
    return union_type;
}
bool union_datum_stream_t::is_infinite() const {
    return is_infinite_union;
}

std::vector<datum_t>
union_datum_stream_t::next_batch_impl(env_t *env, const batchspec_t &batchspec) {
    for (; streams_index < streams.size(); ++streams_index) {
        std::vector<datum_t> batch
            = streams[streams_index]->next_batch(env, batchspec);
        if (batch.size() != 0 ||
            streams[streams_index]->cfeed_type() != feed_type_t::not_feed) {
            return batch;
        }
    }
    return std::vector<datum_t>();
}

// RANGE_DATUM_STREAM_T
range_datum_stream_t::range_datum_stream_t(bool _is_infinite_range,
                                           int64_t _start,
                                           int64_t _stop,
                                           const protob_t<const Backtrace> &bt_source)
    : eager_datum_stream_t(bt_source),
      is_infinite_range(_is_infinite_range),
      start(_start),
      stop(_stop) { }

std::vector<datum_t>
range_datum_stream_t::next_raw_batch(env_t *, const batchspec_t &batchspec) {
    rcheck(!is_infinite_range
           || batchspec.get_batch_type() == batch_type_t::NORMAL
           || batchspec.get_batch_type() == batch_type_t::NORMAL_FIRST,
           base_exc_t::GENERIC,
           "Cannot use an infinite stream with an aggregation function (`reduce`, `count`, etc.) or coerce it to an array.");

    std::vector<datum_t> batch;
    // 500 is picked out of a hat for latency, primarily in the Data Explorer. If you
    // think strongly it should be something else you're probably right.
    batcher_t batcher = batchspec.with_at_most(500).to_batcher();

    while (!is_exhausted()) {
        double next = safe_to_double(start++);
        // `safe_to_double` returns NaN on error, which signals that `start` is larger
        // than 2^53 indicating we've reached the end of our infinite stream. This must
        // be checked before creating a `datum_t` as that does a similar check on
        // construction.
        rcheck(risfinite(next), base_exc_t::GENERIC,
               "`range` out of safe double bounds.");

        batch.emplace_back(next);
        batcher.note_el(batch.back());
        if (batcher.should_send_batch()) {
            break;
        }
    }

    return batch;
}

bool range_datum_stream_t::is_exhausted() const {
    return !is_infinite_range && start >= stop && batch_cache_exhausted();
}

// MAP_DATUM_STREAM_T
map_datum_stream_t::map_datum_stream_t(std::vector<counted_t<datum_stream_t> > &&_streams,
                                       counted_t<const func_t> &&_func,
                                       const protob_t<const Backtrace> &bt_src)
    : eager_datum_stream_t(bt_src), streams(std::move(_streams)), func(std::move(_func)),
      union_type(feed_type_t::not_feed), is_array_map(true), is_infinite_map(true) {
    for (const auto &stream : streams) {
        is_array_map &= stream->is_array();
        union_type = union_of(union_type, stream->cfeed_type());
        is_infinite_map &= stream->is_infinite();
    }
}

std::vector<datum_t>
map_datum_stream_t::next_raw_batch(env_t *env, const batchspec_t &batchspec) {
    rcheck(!is_infinite_map
           || batchspec.get_batch_type() == batch_type_t::NORMAL
           || batchspec.get_batch_type() == batch_type_t::NORMAL_FIRST,
           base_exc_t::GENERIC,
           "Cannot use an infinite stream with an aggregation function (`reduce`, `count`, etc.) or coerce it to an array.");

    std::vector<datum_t> batch;
    batcher_t batcher = batchspec.to_batcher();

    std::vector<datum_t> args;
    args.reserve(streams.size());
    // We need a separate batchspec for the streams to prevent calling `stream->next`
    // with a `batch_type_t::TERMINAL` on an infinite stream.
    batchspec_t batchspec_inner = batchspec_t::default_for(batch_type_t::NORMAL);
    while (!is_exhausted()) {
        args.clear();   // This prevents allocating a new vector every iteration.
        for (const auto &stream : streams) {
            args.push_back(stream->next(env, batchspec_inner));
        }

        datum_t datum = func->call(env, args)->as_datum();
        batcher.note_el(datum);
        batch.push_back(std::move(datum));
        if (batcher.should_send_batch()) {
            break;
        }
    }

    return batch;
}

bool map_datum_stream_t::is_exhausted() const {
    for (const auto &stream : streams) {
        if (stream->is_exhausted()) {
            return batch_cache_exhausted();
        }
    }
    return false;
}

vector_datum_stream_t::vector_datum_stream_t(
        const protob_t<const Backtrace> &bt_source,
        std::vector<datum_t> &&_rows,
        boost::optional<ql::changefeed::keyspec_t> &&_changespec) :
    eager_datum_stream_t(bt_source),
    rows(std::move(_rows)),
    index(0),
    changespec(std::move(_changespec)) { }

datum_t vector_datum_stream_t::next(
        env_t *env, const batchspec_t &bs) {
    if (ops_to_do()) {
        return datum_stream_t::next(env, bs);
    }
    return next_impl(env);
}

datum_t vector_datum_stream_t::next_impl(env_t *) {
    if (index < rows.size()) {
        return std::move(rows[index++]);
    } else {
        return datum_t();
    }
}

std::vector<datum_t> vector_datum_stream_t::next_raw_batch(
        env_t *env, const batchspec_t &bs) {
    std::vector<datum_t> v;
    batcher_t batcher = bs.to_batcher();
    datum_t d;
    while (d = next_impl(env), d.has()) {
        batcher.note_el(d);
        v.push_back(std::move(d));
        if (batcher.should_send_batch()) {
            break;
        }
    }
    return v;
}

void vector_datum_stream_t::add_transformation(
    transform_variant_t &&tv, const protob_t<const Backtrace> &bt) {
    if (changespec) {
        if (auto *rng = boost::get<changefeed::keyspec_t::range_t>(&changespec->spec)) {
            rng->transforms.push_back(tv);
        }
    }
    eager_datum_stream_t::add_transformation(std::move(tv), bt);
}

bool vector_datum_stream_t::is_exhausted() const {
    return index == rows.size();
}

feed_type_t vector_datum_stream_t::cfeed_type() const {
    return feed_type_t::not_feed;
}

bool vector_datum_stream_t::is_array() const {
    return false;
}

bool vector_datum_stream_t::is_infinite() const {
    return false;
}

changefeed::keyspec_t vector_datum_stream_t::get_change_spec() {
    if (changespec) {
        return *changespec;
    } else {
        rfail(base_exc_t::GENERIC, "%s", "Cannot call `changes` on this stream.");
    }
}

} // namespace ql
