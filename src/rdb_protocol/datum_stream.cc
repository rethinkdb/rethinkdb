// Copyright 2010-2015 RethinkDB, all rights reserved.
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

bool changespec_t::include_initial_vals() {
    if (auto *range = boost::get<changefeed::keyspec_t::range_t>(&keyspec.spec)) {
        if (range->range.is_universe()) return false;
    }
    return true;
}

// RANGE/READGEN STUFF
rget_response_reader_t::rget_response_reader_t(
    const counted_t<real_table_t> &_table,
    scoped_ptr_t<readgen_t> &&_readgen)
    : table(_table),
      started(false), shards_exhausted(false),
      readgen(std::move(_readgen)),
      last_read_start(store_key_t::min()),
      active_range(readgen->original_keyrange()),
      items_index(0) { }

void rget_response_reader_t::add_transformation(transform_variant_t &&tv) {
    r_sanity_check(!started);
    transforms.push_back(std::move(tv));
}

bool rget_response_reader_t::add_stamp(changefeed_stamp_t _stamp) {
    stamp = std::move(_stamp);
    return true;
}

boost::optional<active_state_t> rget_response_reader_t::get_active_state() const {
    if (!stamp || !active_range || shard_stamps.size() == 0) return boost::none;
    return active_state_t{
        key_range_t(key_range_t::closed, last_read_start,
                    key_range_t::open, active_range->left),
        shard_stamps,
        skey_version,
        DEBUG_ONLY(readgen->sindex_name())};
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
                    res.size() <= env->limits().array_size_limit(), base_exc_t::RESOURCE,
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

struct last_read_start_visitor_t : public boost::static_visitor<store_key_t> {
    store_key_t operator()(const intersecting_geo_read_t &geo) const {
        return geo.sindex.region ? geo.sindex.region->inner.left : store_key_t::min();
    }
    store_key_t operator()(const rget_read_t &rget) const {
        if (rget.sindex) {
            return rget.sindex->region
                ? rget.sindex->region->inner.left
                : store_key_t::min();
        } else {
            return rget.region.inner.left;
        }
    }
    template<class T>
    store_key_t operator()(const T &) const {
        r_sanity_fail();
    }
};

rget_read_response_t rget_response_reader_t::do_read(env_t *env, const read_t &read) {
    read_response_t res;
    table->read_with_profile(env, read, &res);
    auto rget_res = boost::get<rget_read_response_t>(&res.response);
    r_sanity_check(rget_res != NULL);
    if (auto e = boost::get<exc_t>(&rget_res->result)) {
        throw *e;
    }
    last_read_start = boost::apply_visitor(last_read_start_visitor_t(), read.read);
    return std::move(*rget_res);
}

rget_reader_t::rget_reader_t(
    const counted_t<real_table_t> &_table,
    scoped_ptr_t<readgen_t> &&_readgen)
    : rget_response_reader_t(_table, std::move(_readgen)) { }

void rget_reader_t::accumulate_all(env_t *env, eager_acc_t *acc) {
    r_sanity_check(!started);
    started = true;
    batchspec_t batchspec = batchspec_t::all();
    read_t read = readgen->next_read(active_range, stamp, transforms, batchspec);
    rget_read_response_t resp = do_read(env, std::move(read));

    auto *rr = boost::get<rget_read_t>(&read.read);
    auto final_key = !reversed(rr->sorting) ? store_key_t::max() : store_key_t::min();
    r_sanity_check(resp.last_key == final_key);
    r_sanity_check(!resp.truncated);
    shards_exhausted = true;

    acc->add_res(env, &resp.result);
}

std::vector<rget_item_t>
rget_reader_t::do_range_read(env_t *env, const read_t &read) {
    auto *rr = boost::get<rget_read_t>(&read.read);
    r_sanity_check(rr);
    rget_read_response_t res = do_read(env, read);

    key_range_t rng;
    if (rr->sindex) {
        if (skey_version) {
            r_sanity_check(res.skey_version == *skey_version);
        } else {
            skey_version = res.skey_version;
        }
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

    r_sanity_check(static_cast<bool>(stamp) == static_cast<bool>(rr->stamp));
    if (stamp) {
        r_sanity_check(res.stamp_response);
        rcheck_datum(res.stamp_response->stamps, base_exc_t::OP_FAILED,
                     "Changefeed aborted.  (Did you just reshard?)");
        for (const auto &pair : *res.stamp_response->stamps) {
            // It's OK to blow away old values.
            shard_stamps[pair.first] = pair.second;
        }
    }

    // We need to do some adjustments to the last considered key so that we
    // update the range correctly in the case where we're reading a subportion
    // of the total range.
    store_key_t *key = &res.last_key;
    if (*key == store_key_t::max() && !reversed(rr->sorting)) {
        if (!rng.right.unbounded) {
            *key = rng.right.key();
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
    return groups_to_batch(gs->get_underlying_map());
}

bool rget_reader_t::load_items(env_t *env, const batchspec_t &batchspec) {
    started = true;
    if (items_index >= items.size() && !shards_exhausted) { // read some more
        items_index = 0;
        // `active_range` is guaranteed to be full after the `do_range_read`,
        // because `do_range_read` is responsible for updating the active range.
        items = do_range_read(
            env, readgen->next_read(active_range, stamp, transforms, batchspec));
        // Everything below this point can handle `items` being empty (this is
        // good hygiene anyway).
        r_sanity_check(active_range);
        while (boost::optional<read_t> read = readgen->sindex_sort_read(
                   *active_range, items, stamp, transforms, batchspec)) {
            std::vector<rget_item_t> new_items = do_range_read(env, *read);
            if (new_items.size() == 0) {
                break;
            }

            rcheck_datum(
                (items.size() + new_items.size()) <= env->limits().array_size_limit(),
                base_exc_t::RESOURCE,
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
    scoped_ptr_t<readgen_t> &&_readgen)
    : rget_response_reader_t(_table, std::move(_readgen)) { }

void intersecting_reader_t::accumulate_all(env_t *env, eager_acc_t *acc) {
    r_sanity_check(!started);
    started = true;
    batchspec_t batchspec = batchspec_t::all();
    read_t read = readgen->next_read(active_range, stamp, transforms, batchspec);
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
            env, readgen->next_read(active_range, stamp, transforms, batchspec));
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
                    rcheck_toplevel(
                        processed_pkeys.size() < env->limits().array_size_limit(),
                        ql::base_exc_t::RESOURCE,
                        "Array size limit exceeded during geospatial index traversal.");
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
            *key = rng.right.key();
            bool b = key->decrement();
            r_sanity_check(b);
        }
    }

    shards_exhausted = readgen->update_range(&*active_range, res.last_key);
    grouped_t<stream_t> *gs = boost::get<grouped_t<stream_t> >(&res.result);

    // groups_to_batch asserts that underlying_map has 0 or 1 elements, so it is
    // correct to declare that the order doesn't matter.
    return groups_to_batch(gs->get_underlying_map());
}

readgen_t::readgen_t(
    global_optargs_t _global_optargs,
    std::string _table_name,
    profile_bool_t _profile,
    read_mode_t _read_mode,
    sorting_t _sorting)
    : global_optargs(std::move(_global_optargs)),
      table_name(std::move(_table_name)),
      profile(_profile),
      read_mode(_read_mode),
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
                && (active_range->right.key() < active_range->left))) {
            return true;
        }
    } else {
        r_sanity_check(!active_range->right.unbounded);
        if (!active_range->right.key().decrement()
            || active_range->right.key() < active_range->left) {
            return true;
        }
    }
    return active_range->is_empty();
}

rget_readgen_t::rget_readgen_t(
    global_optargs_t _global_optargs,
    std::string _table_name,
    const datum_range_t &_original_datum_range,
    profile_bool_t _profile,
    read_mode_t _read_mode,
    sorting_t _sorting)
    : readgen_t(std::move(_global_optargs),
                std::move(_table_name),
                _profile, _read_mode, _sorting),
      original_datum_range(_original_datum_range) { }

read_t rget_readgen_t::next_read(
    const boost::optional<key_range_t> &active_range,
    boost::optional<changefeed_stamp_t> stamp,
    std::vector<transform_variant_t> transforms,
    const batchspec_t &batchspec) const {
    return read_t(next_read_impl(
                      active_range,
                      std::move(stamp),
                      std::move(transforms),
                      batchspec),
                  profile,
                  read_mode);
}

// TODO: this is how we did it before, but it sucks.
read_t rget_readgen_t::terminal_read(
    const std::vector<transform_variant_t> &transforms,
    const terminal_variant_t &_terminal,
    const batchspec_t &batchspec) const {
    rget_read_t read = next_read_impl(
        original_keyrange(),
        boost::optional<changefeed_stamp_t>(), // No need to stamp terminals.
        transforms,
        batchspec);
    read.terminal = _terminal;
    return read_t(read, profile, read_mode);
}

primary_readgen_t::primary_readgen_t(
    global_optargs_t global_optargs,
    std::string table_name,
    datum_range_t range,
    profile_bool_t _profile,
    read_mode_t _read_mode,
    sorting_t sorting)
    : rget_readgen_t(std::move(global_optargs),
                     std::move(table_name),
                     range, _profile, _read_mode, sorting) { }

scoped_ptr_t<readgen_t> primary_readgen_t::make(
    env_t *env,
    std::string table_name,
    read_mode_t read_mode,
    datum_range_t range,
    sorting_t sorting) {
    return scoped_ptr_t<readgen_t>(
        new primary_readgen_t(
            env->get_all_optargs(),
            std::move(table_name),
            range,
            env->profile(),
            read_mode,
            sorting));
}

rget_read_t primary_readgen_t::next_read_impl(
    const boost::optional<key_range_t> &active_range,
    boost::optional<changefeed_stamp_t> stamp,
    std::vector<transform_variant_t> transforms,
    const batchspec_t &batchspec) const {
    r_sanity_check(active_range);
    return rget_read_t(
        std::move(stamp),
        region_t(*active_range),
        global_optargs,
        table_name,
        batchspec,
        std::move(transforms),
        boost::optional<terminal_variant_t>(),
        boost::optional<sindex_rangespec_t>(),
        sorting);
}

// We never need to do an sindex sort when indexing by a primary key.
boost::optional<read_t> primary_readgen_t::sindex_sort_read(
    UNUSED const key_range_t &active_range,
    UNUSED const std::vector<rget_item_t> &items,
    UNUSED boost::optional<changefeed_stamp_t> stamp,
    UNUSED std::vector<transform_variant_t> transforms,
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
    global_optargs_t global_optargs,
    std::string table_name,
    const std::string &_sindex,
    datum_range_t range,
    profile_bool_t _profile,
    read_mode_t _read_mode,
    sorting_t sorting)
    : rget_readgen_t(std::move(global_optargs),
                     std::move(table_name),
                     range, _profile, _read_mode, sorting),
      sindex(_sindex),
      sent_first_read(false) { }

scoped_ptr_t<readgen_t> sindex_readgen_t::make(
    env_t *env,
    std::string table_name,
    read_mode_t read_mode,
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
            read_mode,
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
                ? l.sindex_key > r.sindex_key
                : l.sindex_key < r.sindex_key;
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
    boost::optional<changefeed_stamp_t> stamp,
    std::vector<transform_variant_t> transforms,
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
        std::move(stamp),
        region_t::universe(),
        global_optargs,
        table_name,
        batchspec,
        std::move(transforms),
        boost::optional<terminal_variant_t>(),
        sindex_rangespec_t(sindex, std::move(region), original_datum_range),
        sorting);
}

boost::optional<read_t> sindex_readgen_t::sindex_sort_read(
    const key_range_t &active_range,
    const std::vector<rget_item_t> &items,
    boost::optional<changefeed_stamp_t> stamp,
    std::vector<transform_variant_t> transforms,
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
            if (rng.right.unbounded || rng.left < rng.right.key()) {
                return read_t(
                    rget_read_t(
                        std::move(stamp),
                        region_t::universe(),
                        global_optargs,
                        table_name,
                        batchspec.with_new_batch_type(batch_type_t::SINDEX_CONSTANT),
                        std::move(transforms),
                        boost::optional<terminal_variant_t>(),
                        sindex_rangespec_t(
                            sindex,
                            region_t(key_range_t(rng)),
                            original_datum_range),
                        sorting),
                    profile,
                    read_mode);
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
    global_optargs_t global_optargs,
    std::string table_name,
    const std::string &_sindex,
    const datum_t &_query_geometry,
    profile_bool_t _profile,
    read_mode_t _read_mode)
    : readgen_t(std::move(global_optargs),
                std::move(table_name),
                _profile, _read_mode, sorting_t::UNORDERED),
      sindex(_sindex),
      query_geometry(_query_geometry) { }

scoped_ptr_t<readgen_t> intersecting_readgen_t::make(
    env_t *env,
    std::string table_name,
    read_mode_t read_mode,
    const std::string &sindex,
    const datum_t &query_geometry) {
    return scoped_ptr_t<readgen_t>(
        new intersecting_readgen_t(
            env->get_all_optargs(),
            std::move(table_name),
            sindex,
            query_geometry,
            env->profile(),
            read_mode));
}

read_t intersecting_readgen_t::next_read(
    const boost::optional<key_range_t> &active_range,
    boost::optional<changefeed_stamp_t> stamp,
    std::vector<transform_variant_t> transforms,
    const batchspec_t &batchspec) const {
    return read_t(next_read_impl(
                      active_range,
                      std::move(stamp),
                      std::move(transforms),
                      batchspec),
                  profile,
                  read_mode);
}

read_t intersecting_readgen_t::terminal_read(
    const std::vector<transform_variant_t> &transforms,
    const terminal_variant_t &_terminal,
    const batchspec_t &batchspec) const {
    intersecting_geo_read_t read =
        next_read_impl(
            original_keyrange(),
            boost::optional<changefeed_stamp_t>(), // No need to stamp terminals.
            transforms,
            batchspec);
    read.terminal = _terminal;
    return read_t(read, profile, read_mode);
}

intersecting_geo_read_t intersecting_readgen_t::next_read_impl(
    const boost::optional<key_range_t> &active_range,
    boost::optional<changefeed_stamp_t> stamp,
    std::vector<transform_variant_t> transforms,
    const batchspec_t &batchspec) const {
    r_sanity_check(active_range);
    return intersecting_geo_read_t(
        std::move(stamp),
        region_t::universe(),
        global_optargs,
        table_name,
        batchspec,
        std::move(transforms),
        boost::optional<terminal_variant_t>(),
        sindex_rangespec_t(sindex, region_t(*active_range), datum_range_t::universe()),
        query_geometry);
}

boost::optional<read_t> intersecting_readgen_t::sindex_sort_read(
    UNUSED const key_range_t &active_range,
    UNUSED const std::vector<rget_item_t> &items,
    UNUSED boost::optional<changefeed_stamp_t> stamp,
    UNUSED std::vector<transform_variant_t> transform,
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

bool datum_stream_t::add_stamp(changefeed_stamp_t) {
    // By default most datum streams can't stamp their responses.
    return false;
}

boost::optional<active_state_t> datum_stream_t::get_active_state() const {
    return boost::none;
}

scoped_ptr_t<val_t> datum_stream_t::run_terminal(
    env_t *env, const terminal_variant_t &tv) {
    scoped_ptr_t<eager_acc_t> acc(make_eager_terminal(tv));
    accumulate(env, acc.get(), tv);
    return acc->finish_eager(backtrace(), is_grouped(), env->limits());
}

scoped_ptr_t<val_t> datum_stream_t::to_array(env_t *env) {
    scoped_ptr_t<eager_acc_t> acc = make_to_array();
    accumulate_all(env, acc.get());
    return acc->finish_eager(backtrace(), is_grouped(), env->limits());
}

// DATUM_STREAM_T
counted_t<datum_stream_t> datum_stream_t::slice(size_t l, size_t r) {
    return make_counted<slice_datum_stream_t>(l, r, this->counted_from_this());
}
counted_t<datum_stream_t> datum_stream_t::offsets_of(counted_t<const func_t> f) {
    return make_counted<offsets_of_datum_stream_t>(f, counted_from_this());
}
counted_t<datum_stream_t> datum_stream_t::ordered_distinct() {
    return make_counted<ordered_distinct_datum_stream_t>(counted_from_this());
}

datum_stream_t::datum_stream_t(backtrace_id_t bt)
    : bt_rcheckable_t(bt), batch_cache_index(0), grouped(false) {
}

void datum_stream_t::add_grouping(transform_variant_t &&tv,
                                  backtrace_id_t bt) {
    check_not_grouped("Cannot call `group` on the output of `group` "
                      "(did you mean to `ungroup`?).");
    grouped = true;
    add_transformation(std::move(tv), bt);
}
void datum_stream_t::check_not_grouped(const char *msg) {
    rcheck(!is_grouped(), base_exc_t::LOGIC, msg);
}

std::vector<datum_t>
datum_stream_t::next_batch(env_t *env, const batchspec_t &batchspec) {
    env->do_eval_callback();
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

datum_t datum_stream_t::next(env_t *env, const batchspec_t &batchspec) {

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
    transform_variant_t &&tv, backtrace_id_t bt) {
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

std::vector<datum_t>
eager_datum_stream_t::next_batch_impl(env_t *env, const batchspec_t &bs) {
    groups_t data;
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
lazy_datum_stream_t::lazy_datum_stream_t(scoped_ptr_t<reader_t> &&_reader,
                                         backtrace_id_t bt)
    : datum_stream_t(bt),
      current_batch_offset(0),
      reader(std::move(_reader)) { }

void lazy_datum_stream_t::add_transformation(transform_variant_t &&tv,
                                             backtrace_id_t bt) {
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
                                           backtrace_id_t bt)
    : eager_datum_stream_t(bt), index(0), arr(_arr) { }

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

// OFFSETS_OF_DATUM_STREAM_T
offsets_of_datum_stream_t::offsets_of_datum_stream_t(counted_t<const func_t> _f,
                                                     counted_t<datum_stream_t> _source)
    : wrapper_datum_stream_t(_source), f(_f), index(0) {
    guarantee(f.has() && source.has());
}

std::vector<datum_t>
offsets_of_datum_stream_t::next_raw_batch(env_t *env, const batchspec_t &bs) {
    std::vector<datum_t> ret;
    profile::sampler_t sampler("Finding offsets_of eagerly.", env->trace);
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

std::vector<changespec_t> slice_datum_stream_t::get_changespecs() {
    if (left == 0) {
        auto subspecs = source->get_changespecs();
        rcheck(subspecs.size() == 1, base_exc_t::LOGIC,
               "Cannot call `changes` on a slice of a union.");
        auto subspec = subspecs[0];
        auto *rspec = boost::get<changefeed::keyspec_t::range_t>(&subspec.keyspec.spec);
        if (rspec != NULL) {
            std::copy(transforms.begin(), transforms.end(),
                      std::back_inserter(rspec->transforms));
            rcheck(right <= static_cast<uint64_t>(std::numeric_limits<size_t>::max()),
                   base_exc_t::LOGIC,
                   strprintf("Cannot call `changes` on a slice "
                             "with size > %zu (got %" PRIu64 ").",
                             std::numeric_limits<size_t>::max(),
                             right));
            return std::vector<changespec_t>{changespec_t(
                    changefeed::keyspec_t(
                        changefeed::keyspec_t::limit_t{
                            std::move(*rspec),
                            static_cast<size_t>(right)},
                        std::move(subspec.keyspec.table),
                        std::move(subspec.keyspec.table_name)),
                    counted_from_this())};
        }
    }
    return wrapper_datum_stream_t::get_changespecs();
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

    while (index < right) {
        if (batcher.should_send_batch()) break;
        if (source->cfeed_type() != feed_type_t::not_feed && ret.size() > 0) break;
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
class coro_stream_t {
public:
    coro_stream_t(counted_t<datum_stream_t> _stream, union_datum_stream_t *_parent)
        : stream(_stream),
          running(false),
          is_first_batch(true),
          parent(_parent) {
        if (!stream->is_exhausted()) parent->active += 1;
    }
    void maybe_launch_read() {
        if (!stream->is_exhausted() && !running) {
            running = true;
            auto_drainer_t::lock_t lock(&parent->drainer);
            if (stream->cfeed_type() == feed_type_t::not_feed) {
                // We only launch a limited number of non-feed reads per union
                // at a time, controlled by a coro pool.
                parent->read_queue.push([this, lock]{this->cb(lock);});
            } else {
                // For feeds, we have to spawn a coroutine since we cannot afford
                // to wait for a specific subset of substreams to yield a result
                // before spawning a read from the remaining ones.
                coro_t::spawn_sometime([this, lock]{this->cb(lock);});
            }
        }
    }
    const counted_t<datum_stream_t> stream;
private:
    void cb(auto_drainer_t::lock_t lock) THROWS_NOTHING {
        // See `next_batch` call below.
        DEBUG_ONLY(scoped_ptr_t<assert_no_coro_waiting_t> no_coro_waiting(
                       make_scoped<assert_no_coro_waiting_t>(__FILE__, __LINE__)));
        lock.assert_is_holding(&parent->drainer);
        parent->home_thread_mixin_t::assert_thread();
        try {
            r_sanity_check(parent->coro_batchspec.has());
            batchspec_t bs = *parent->coro_batchspec;
            if (bs.get_batch_type() == batch_type_t::NORMAL_FIRST && !is_first_batch) {
                bs = bs.with_new_batch_type(batch_type_t::NORMAL);
            }
            is_first_batch = false;

            // We want to make sure that this call to `next_batch` is the only
            // thing in this whole function that blocks.
            DEBUG_ONLY(no_coro_waiting.reset());
            std::vector<datum_t> batch = stream->next_batch(parent->coro_env.get(), bs);
            DEBUG_ONLY(no_coro_waiting
                       = make_scoped<assert_no_coro_waiting_t>(__FILE__, __LINE__));

            if (batch.size() == 0 && stream->cfeed_type() == feed_type_t::not_feed) {
                r_sanity_check(stream->is_exhausted());
                // We're done, fall through and deactivate ourselves.
            } else {
                r_sanity_check(
                    batch.size() != 0
                    || (parent->coro_env->return_empty_normal_batches
                        == return_empty_normal_batches_t::YES)
                    || (bs.get_batch_type() == batch_type_t::NORMAL_FIRST));
                parent->queue.push(std::move(batch));
                parent->data_available->pulse_if_not_already_pulsed();
            }
        } catch (const interrupted_exc_t &) {
            // Just fall through and end; the drainer signal being pulsed
            // will interrupt `next_batch_impl` as well.
        } catch (...) {
            parent->abort_exc.pulse_if_not_already_pulsed(std::current_exception());
        }
        if (stream->is_exhausted()) {
            parent->active -= 1;
            if (parent->active == 0 && parent->data_available.has()) {
                parent->data_available->pulse_if_not_already_pulsed();
            }
        }
        running = false;
    }
    bool running, is_first_batch;
    union_datum_stream_t *parent;
};

// The maximum number of reads that a union_datum_stream spawns on its substreams
// at a time. This limit does not apply to changefeed streams.
const size_t MAX_CONCURRENT_UNION_READS = 32;

union_datum_stream_t::union_datum_stream_t(
    env_t *env,
    std::vector<counted_t<datum_stream_t> > &&streams,
    backtrace_id_t bt,
    size_t expected_states)
    : datum_stream_t(bt),
      union_type(feed_type_t::not_feed),
      is_infinite_union(false),
      sent_init(false),
      ready_needed(expected_states),
      read_coro_pool(MAX_CONCURRENT_UNION_READS, &read_queue, &read_coro_callback),
      active(0),
      coros_exhausted(false) {

    for (const auto &stream : streams) {
        union_type = union_of(union_type, stream->cfeed_type());
        is_infinite_union |= stream->is_infinite();
    }

    if (env->trace != nullptr) {
        trace = make_scoped<profile::trace_t>();
        disabler = make_scoped<profile::disabler_t>(trace.get());
    }
    coro_env = make_scoped<env_t>(
        env->get_rdb_ctx(),
        env->return_empty_normal_batches,
        drainer.get_drain_signal(),
        env->get_all_optargs(),
        trace.has() ? trace.get() : nullptr);

    coro_streams.reserve(streams.size());
    for (auto &&stream : streams) {
        coro_streams.push_back(make_scoped<coro_stream_t>(std::move(stream), this));
    }
}

std::vector<datum_t>
union_datum_stream_t::next_batch_impl(env_t *env, const batchspec_t &batchspec) {
    // This needs to be on the same thread as the coroutines spawned in the
    // constructor.
    home_thread_mixin_t::assert_thread();
    auto_drainer_t::lock_t lock(&drainer);
    wait_any_t interruptor(env->interruptor, lock.get_drain_signal(),
                           abort_exc.get_ready_signal());
    for (;;) {
        try {
            // The client is already doing prefetching, so we don't want to do
            // *double* prefetching by prefetching on the server as well.
            while (queue.size() == 0) {
                std::exception_ptr exc;
                if (abort_exc.try_get_value(&exc)) std::rethrow_exception(exc);
                if (active == 0) {
                    coros_exhausted = true;
                    return std::vector<datum_t>();
                }
                // We don't have the batchspec during construction.
                if (!coro_batchspec.has()) {
                    coro_batchspec = make_scoped<batchspec_t>(batchspec);
                }

                data_available = make_scoped<cond_t>();
                for (auto &&s : coro_streams) s->maybe_launch_read();
                r_sanity_check(active != 0 || data_available->is_pulsed());
                wait_interruptible(data_available.get(), &interruptor);
            }
        } catch (...) {
            // Prefer throwing coroutine exceptions because we might have been
            // interrupted by `abort_exc`, and in all other cases it doesn't
            // matter which we throw.
            std::exception_ptr exc;
            if (abort_exc.try_get_value(&exc)) std::rethrow_exception(exc);
            throw;
        }
        r_sanity_check(queue.size() != 0);
        std::vector<datum_t> data;
        if (ready_needed > 0) {
            data.reserve(queue.front().size());
            for (auto &&d : std::move(queue.front())) {
                datum_t state = d.get_field("state", NOTHROW);
                if (state.has()) {
                    if (state.as_str() == "initializing") {
                        if (sent_init) continue;
                        sent_init = true;
                    } else if (state.as_str() == "ready") {
                        ready_needed -= 1;
                        if (ready_needed != 0) continue;
                    } else {
                        rfail(base_exc_t::INTERNAL,
                              "Unrecognized state string `%s`.",
                              state.as_str().to_std().c_str());
                    }
                }
                data.push_back(std::move(d));
            }
        } else {
            data = std::move(queue.front());
        }
        queue.pop();
        if (data.size() == 0) {
            // We should only ever get empty batches if one of our streams is a
            // changefeed.
            r_sanity_check(union_type != feed_type_t::not_feed);
            // If we aren't supposed to send empty normal batches, and this
            // isn't a `NORMAL_FIRST` batch, we don't send it.
            if ((env->return_empty_normal_batches == return_empty_normal_batches_t::NO)
                && (batchspec.get_batch_type() != batch_type_t::NORMAL_FIRST)) {
                continue;
            }
        }
        if (active == 0 && queue.size() == 0) coros_exhausted = true;
        return data;
    }
}

void union_datum_stream_t::add_transformation(transform_variant_t &&tv,
                                              backtrace_id_t bt) {
    for (auto &&coro_stream : coro_streams) {
        coro_stream->stream->add_transformation(transform_variant_t(tv), bt);
    }
    update_bt(bt);
}

void union_datum_stream_t::accumulate(
    env_t *env, eager_acc_t *acc, const terminal_variant_t &tv) {
    for (auto &&coro_stream : coro_streams) {
        coro_stream->stream->accumulate(env, acc, tv);
    }
}

void union_datum_stream_t::accumulate_all(env_t *env, eager_acc_t *acc) {
    for (auto &&coro_stream : coro_streams) {
        coro_stream->stream->accumulate_all(env, acc);
    }
}

bool union_datum_stream_t::is_array() const {
    for (auto &&coro_stream : coro_streams) {
        if (!coro_stream->stream->is_array()) {
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
    return batch_cache_exhausted() && coros_exhausted;
}
feed_type_t union_datum_stream_t::cfeed_type() const {
    return union_type;
}
bool union_datum_stream_t::is_infinite() const {
    return is_infinite_union;
}

std::vector<changespec_t> union_datum_stream_t::get_changespecs() {
    std::vector<changespec_t> specs;
    for (auto &&coro_stream : coro_streams) {
        auto subspecs = coro_stream->stream->get_changespecs();
        std::move(subspecs.begin(), subspecs.end(), std::back_inserter(specs));
    }
    return specs;
}

// RANGE_DATUM_STREAM_T
range_datum_stream_t::range_datum_stream_t(bool _is_infinite_range,
                                           int64_t _start,
                                           int64_t _stop,
                                           backtrace_id_t bt)
    : eager_datum_stream_t(bt),
      is_infinite_range(_is_infinite_range),
      start(_start),
      stop(_stop) { }

std::vector<datum_t>
range_datum_stream_t::next_raw_batch(env_t *, const batchspec_t &batchspec) {
    rcheck(!is_infinite_range
           || batchspec.get_batch_type() == batch_type_t::NORMAL
           || batchspec.get_batch_type() == batch_type_t::NORMAL_FIRST,
           base_exc_t::LOGIC,
           "Cannot use an infinite stream with an aggregation function "
           "(`reduce`, `count`, etc.) or coerce it to an array.");

    std::vector<datum_t> batch;
    // 500 is picked out of a hat for latency, primarily in the Data Explorer. If you
    // think strongly it should be something else you're probably right.
    batcher_t batcher = batchspec.get_batch_type() == batch_type_t::TERMINAL ?
                            batchspec.to_batcher() :
                            batchspec.with_at_most(500).to_batcher();

    while (!is_exhausted()) {
        double next = safe_to_double(start++);
        // `safe_to_double` returns NaN on error, which signals that `start` is larger
        // than 2^53 indicating we've reached the end of our infinite stream. This must
        // be checked before creating a `datum_t` as that does a similar check on
        // construction.
        rcheck(risfinite(next), base_exc_t::LOGIC,
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
map_datum_stream_t::map_datum_stream_t(
        std::vector<counted_t<datum_stream_t> > &&_streams,
        counted_t<const func_t> &&_func,
        backtrace_id_t bt)
    : eager_datum_stream_t(bt), streams(std::move(_streams)), func(std::move(_func)),
      union_type(feed_type_t::not_feed), is_array_map(true), is_infinite_map(true) {
    args.reserve(streams.size());
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
           base_exc_t::LOGIC,
           "Cannot use an infinite stream with an aggregation function "
           "(`reduce`, `count`, etc.) or coerce it to an array.");

    std::vector<datum_t> batch;
    batcher_t batcher = batchspec.to_batcher();

    // We need a separate batchspec for the streams to prevent calling `stream->next`
    // with a `batch_type_t::TERMINAL` on an infinite stream.
    batchspec_t batchspec_inner = batchspec_t::default_for(batch_type_t::NORMAL);
    while (!is_exhausted()) {
        while (args.size() < streams.size()) {
            datum_t d = streams[args.size()]->next(env, batchspec_inner);
            if (union_type == feed_type_t::not_feed) {
                r_sanity_check(d.has());
            } else {
                if (!d.has()) {
                    // Return with `args` partway full and continue next time
                    // the client asks for a batch.
                    return batch;
                }
            }
            args.push_back(std::move(d));
        }
        datum_t datum = func->call(env, args)->as_datum();
        r_sanity_check(datum.has());
        args.clear();
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
        backtrace_id_t bt,
        std::vector<datum_t> &&_rows,
        boost::optional<ql::changefeed::keyspec_t> &&_changespec) :
    eager_datum_stream_t(bt),
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
    transform_variant_t &&tv, backtrace_id_t bt) {
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

std::vector<changespec_t> vector_datum_stream_t::get_changespecs() {
    if (changespec) {
        return std::vector<changespec_t>{
            changespec_t(*changespec, counted_from_this())};
    } else {
        rfail(base_exc_t::LOGIC, "%s", "Cannot call `changes` on this stream.");
    }
}

} // namespace ql
