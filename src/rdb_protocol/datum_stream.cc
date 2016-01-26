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

read_mode_t up_to_date_read_mode(read_mode_t in) {
    switch (in) {
    case read_mode_t::MAJORITY: return in;
    case read_mode_t::SINGLE:   return in;
    case read_mode_t::OUTDATED: return read_mode_t::SINGLE;
    case read_mode_t::DEBUG_DIRECT:
        rfail_datum(base_exc_t::LOGIC,
                    "DEBUG_DIRECT is not a legal read mode for this operation "
                    "(an up-to-date read mode is required).");
    default: unreachable();
    }
}

key_range_t safe_universe() {
    return key_range_t(key_range_t::closed, store_key_t::min(),
                       key_range_t::open, store_key_t::max());
}

void debug_print(printf_buffer_t *buf, const range_state_t &rs) {
    const char *s;
    switch (rs) {
    case range_state_t::ACTIVE:    s = "ACTIVE"; break;
    case range_state_t::SATURATED: s = "SATURATED"; break;
    case range_state_t::EXHAUSTED: s = "EXHAUSTED"; break;
    default: unreachable();
    }
    buf->appendf("%s", s);
}

void debug_print(printf_buffer_t *buf, const hash_range_with_cache_t &hrwc) {
    buf->appendf("hash_range_with_cache_t(");
    debug_print(buf, hrwc.key_range);
    buf->appendf(", ");
    debug_print(buf, hrwc.cache);
    buf->appendf(", ");
    debug_print(buf, hrwc.state);
    buf->appendf(")\n");
}

void debug_print(printf_buffer_t *buf, const hash_ranges_t &hr) {
    debug_print(buf, hr.hash_ranges);
}

void debug_print(printf_buffer_t *buf, const active_ranges_t &ar) {
    debug_print(buf, ar.ranges);
}

bool hash_range_with_cache_t::totally_exhausted() const {
    return state == range_state_t::EXHAUSTED && cache.size() == 0;
}

range_state_t hash_ranges_t::state() const {
    bool seen_saturated = false;
    for (auto &&pair : hash_ranges) {
        switch (pair.second.state) {
        case range_state_t::ACTIVE: return range_state_t::ACTIVE;
        case range_state_t::SATURATED:
            seen_saturated = true;
            break;
        case range_state_t::EXHAUSTED: break;
        default: unreachable();
        }
    }
    return seen_saturated ? range_state_t::SATURATED : range_state_t::EXHAUSTED;
}

bool hash_ranges_t::totally_exhausted() const {
    for (auto &&pair : hash_ranges) {
        if (!pair.second.totally_exhausted()) return false;
    }
    return true;
}

bool active_ranges_t::totally_exhausted() const {
    for (auto &&pair : ranges) {
        if (!pair.second.totally_exhausted()) return false;
    }
    return true;
}

key_range_t active_ranges_to_range(const active_ranges_t &ranges) {
    store_key_t start = store_key_t::max();
    store_key_t end = store_key_t::min();
    bool seen_active = false;
    for (auto &&pair : ranges.ranges) {
        switch (pair.second.state()) {
        case range_state_t::ACTIVE:
            for (auto &&hash_pair : pair.second.hash_ranges) {
                start = std::min(start, hash_pair.second.key_range.left);
                end = std::max(end, hash_pair.second.key_range.right.key_or_max());
            }
            seen_active = true;
            break;
        case range_state_t::SATURATED:
            // If we didn't use any of the values we read last time, don't read
            // any more this time.
            break;
        case range_state_t::EXHAUSTED:
            // If there's no more data in a range then don't bother reading from it.
            break;
        default: unreachable();
        }
    }

    // We shouldn't get here unless there's at least one active range.
    r_sanity_check(seen_active);
    return key_range_t(key_range_t::closed, start, key_range_t::open, end);
}

boost::optional<std::map<region_t, store_key_t> > active_ranges_to_hints(
    sorting_t sorting, const boost::optional<active_ranges_t> &ranges) {

    if (!ranges) return boost::none;

    std::map<region_t, store_key_t> hints;
    for (auto &&pair : ranges->ranges) {
        switch (pair.second.state()) {
        case range_state_t::ACTIVE:
            for (auto &&hash_pair : pair.second.hash_ranges) {
                // If any of the hash shards in a range are active, we send
                // reads for all of them, because the assumption is that data is
                // randomly distributed across hash shards, which means the only
                // case where some are active and some are saturated is when
                // we're reading really small batches for some reason
                // (e.g. because of a sparse filter), in which case we still
                // want to read from all the hash shards at once.
                switch (hash_pair.second.state) {
                case range_state_t::ACTIVE: // fallthru
                case range_state_t::SATURATED:
                    hints[region_t(hash_pair.first.beg,
                                   hash_pair.first.end,
                                   pair.first)] = !reversed(sorting)
                        ? hash_pair.second.key_range.left
                        : hash_pair.second.key_range.right.key_or_max();
                    break;
                case range_state_t::EXHAUSTED: break;
                default: unreachable();
                }
            }
            break;
        case range_state_t::SATURATED: break;
        case range_state_t::EXHAUSTED: break;
        default: unreachable();
        }
    }
    r_sanity_check(hints.size() > 0);
    return std::move(hints);
}

enum class is_secondary_t { NO, YES };
active_ranges_t new_active_ranges(
    const stream_t &stream,
    const key_range_t &original_range,
    const boost::optional<std::map<region_t, uuid_u> > &shard_ids,
    is_secondary_t is_secondary) {
    active_ranges_t ret;
    std::set<uuid_u> covered_shards;
    for (auto &&pair : stream.substreams) {
        uuid_u cfeed_shard_id = nil_uuid();
        if (shard_ids) {
            auto shard_id_it = shard_ids->find(pair.first);
            r_sanity_check(shard_id_it != shard_ids->end());
            cfeed_shard_id = shard_id_it->second;
            covered_shards.insert(cfeed_shard_id);
        }

        ret.ranges[pair.first.inner]
           .hash_ranges[hash_range_t{pair.first.beg, pair.first.end}]
            = hash_range_with_cache_t{
                cfeed_shard_id,
                is_secondary == is_secondary_t::YES
                    ? original_range
                    : pair.first.inner.intersection(original_range),
                raw_stream_t(),
                range_state_t::ACTIVE};
    }

    // Add ranges for missing shards (we get this if some shards didn't return any data)
    if (shard_ids) {
        for (const auto &shard_pair : *shard_ids) {
            if (covered_shards.count(shard_pair.second) > 0) {
                continue;
            }

            ret.ranges[shard_pair.first.inner]
                .hash_ranges[hash_range_t{shard_pair.first.beg, shard_pair.first.end}]
                 = hash_range_with_cache_t{
                     shard_pair.second,
                     is_secondary == is_secondary_t::YES
                         ? original_range
                         : shard_pair.first.inner.intersection(original_range),
                     raw_stream_t(),
                     range_state_t::ACTIVE};
        }
    }

    return ret;
}

class pseudoshard_t {
public:
    pseudoshard_t(sorting_t _sorting,
                  hash_range_with_cache_t *_cached,
                  keyed_stream_t *_fresh)
        : sorting(_sorting),
          cached(_cached),
          cached_index(0),
          fresh(_fresh),
          fresh_index(0),
          finished(false) {
        r_sanity_check(_cached != nullptr);
    }
    pseudoshard_t(pseudoshard_t &&) = default;
    pseudoshard_t &operator=(pseudoshard_t &&) = default;

    void finish() {
        r_sanity_check(!finished);
        switch (cached->state) {
        case range_state_t::ACTIVE:
            if (cached->cache.size() != 0) {
                if (cached_index == 0) {
                    cached->state = range_state_t::SATURATED;
                }
            } else if (fresh != nullptr && fresh->stream.size() != 0) {
                if (fresh_index == 0) {
                    cached->state = range_state_t::SATURATED;
                }
            } else {
                // No fresh values and no cached values means we should be exhausted.
                r_sanity_check(false);
            }
            break;
        case range_state_t::SATURATED:
            // A shard should never be saturated unless it has cached values.
            r_sanity_check(cached->cache.size() != 0);
            if (cached_index != 0) {
                cached->state = range_state_t::ACTIVE;
            }
            break;
        case range_state_t::EXHAUSTED: break;
        default: unreachable();
        }
        if (fresh != nullptr || cached_index > 0) {
            raw_stream_t new_cache;
            new_cache.reserve((cached->cache.size() - cached_index)
                              + (fresh ? (fresh->stream.size() - fresh_index) : 0));
            std::move(cached->cache.begin() + cached_index,
                      cached->cache.end(),
                      std::back_inserter(new_cache));
            if (fresh != nullptr) {
                std::move(fresh->stream.begin() + fresh_index,
                          fresh->stream.end(),
                          std::back_inserter(new_cache));
            }
            cached->cache = std::move(new_cache);
        }
        finished = true;
    }

    const store_key_t *best_unpopped_key() const {
        r_sanity_check(!finished);
        if (cached_index < cached->cache.size()) {
            return &cached->cache[cached_index].key;
        } else if (fresh != nullptr && fresh_index < fresh->stream.size()) {
            return &fresh->stream[fresh_index].key;
        } else {
            if (!reversed(sorting)) {
                return cached->key_range.is_empty()
                    ? &store_key_max
                    : &cached->key_range.left;
            } else {
                return cached->key_range.is_empty()
                    ? &store_key_min
                    : &cached->key_range.right_or_max();
            }
        }
    }

    boost::optional<rget_item_t> pop() {
        r_sanity_check(!finished);
        if (cached_index < cached->cache.size()) {
            return std::move(cached->cache[cached_index++]);
        } else if (fresh != nullptr && fresh_index < fresh->stream.size()) {
            return std::move(fresh->stream[fresh_index++]);
        } else {
            return boost::none;
        }
    }
private:
    // We move out of `cached` and `fresh`, so we need to act like we have ownership.
    DISABLE_COPYING(pseudoshard_t);
    sorting_t sorting;
    hash_range_with_cache_t *cached;
    size_t cached_index;
    keyed_stream_t *fresh;
    size_t fresh_index;
    bool finished;
};

changefeed::keyspec_t empty_reader_t::get_changespec() const {
    return changefeed::keyspec_t(
        changefeed::keyspec_t::empty_t(),
        table,
        table_name);
}

raw_stream_t rget_response_reader_t::unshard(
    sorting_t sorting,
    rget_read_response_t &&res) {

    grouped_t<stream_t> *gs = boost::get<grouped_t<stream_t> >(&res.result);
    r_sanity_check(gs != nullptr);
    auto stream = groups_to_batch(gs->get_underlying_map());
    if (!active_ranges) {
        boost::optional<std::map<region_t, uuid_u> > opt_shard_ids;
        if (res.stamp_response) {
            opt_shard_ids = std::map<region_t, uuid_u>();
            for (const auto &pair : *res.stamp_response->stamp_infos) {
                opt_shard_ids->insert(
                    std::make_pair(pair.second.shard_region, pair.first));
            }
        }
        active_ranges = new_active_ranges(
            stream, readgen->original_keyrange(res.reql_version), opt_shard_ids,
            readgen->sindex_name() ? is_secondary_t::YES : is_secondary_t::NO);
        readgen->restrict_active_ranges(sorting, &*active_ranges);
        reql_version = res.reql_version;
    } else {
        r_sanity_check(res.reql_version == reql_version);
    }

    raw_stream_t ret;
    // Create the pseudoshards, which represent the cached, fresh, and
    // hypothetical `rget_item_t`s from the shards.  We also mark shards
    // exhausted in this step.
    std::vector<pseudoshard_t> pseudoshards;
    pseudoshards.reserve(active_ranges->ranges.size() * CPU_SHARDING_FACTOR);
    size_t n_active = 0, n_fresh = 0;
    for (auto &&pair : active_ranges->ranges) {
        bool range_active = pair.second.state() == range_state_t::ACTIVE;
        for (auto &&hash_pair : pair.second.hash_ranges) {
            if (hash_pair.second.totally_exhausted()) continue;
            keyed_stream_t *fresh = nullptr;
            // Active shards need their bounds updated.
            if (range_active) {
                n_active += 1;
                store_key_t *new_bound = nullptr;
                auto it = stream.substreams.find(
                    region_t(hash_pair.first.beg, hash_pair.first.end, pair.first));
                if (it != stream.substreams.end()) {
                    n_fresh += 1;
                    fresh = &it->second;
                    new_bound = &it->second.last_key;
                }
                if (!reversed(sorting)) {
                    if (new_bound != nullptr && *new_bound != store_key_max) {
                        hash_pair.second.key_range.left = *new_bound;
                        bool incremented = hash_pair.second.key_range.left.increment();
                        r_sanity_check(incremented); // not max key
                    } else {
                        hash_pair.second.key_range.left =
                            hash_pair.second.key_range.right_or_max();
                    }
                } else {
                    // The right bound is open so we don't need to decrement.
                    if (new_bound != nullptr && *new_bound != store_key_min) {
                        hash_pair.second.key_range.right =
                            key_range_t::right_bound_t(*new_bound);
                    } else {
                        hash_pair.second.key_range.right =
                            key_range_t::right_bound_t(hash_pair.second.key_range.left);
                    }
                }
                if (new_bound) {
                    // If there's nothing left to read, it's exhausted.
                    if (hash_pair.second.key_range.is_empty()) {
                        hash_pair.second.state = range_state_t::EXHAUSTED;
                    }
                } else {
                    // If we got no data back, the logic above should have set
                    // the range to something empty.
                    r_sanity_check(hash_pair.second.key_range.is_empty());
                    hash_pair.second.state = range_state_t::EXHAUSTED;
                }
            }
            // If there's any data for a hash shard, we need to consider it
            // while unsharding.  Note that the shard may have *already been
            // marked exhausted* in the step above.
            if (fresh != nullptr || hash_pair.second.cache.size() > 0) {
                pseudoshards.emplace_back(sorting, &hash_pair.second, fresh);
            }
        }
    }
    if (pseudoshards.size() == 0) {
        r_sanity_check(shards_exhausted());
        return ret;
    }

    // Do the unsharding.
    if (sorting != sorting_t::UNORDERED) {
        for (;;) {
            pseudoshard_t *best_shard = &pseudoshards[0];
            const store_key_t *best_key = best_shard->best_unpopped_key();
            for (size_t i = 1; i < pseudoshards.size(); ++i) {
                pseudoshard_t *cur_shard = &pseudoshards[i];
                const store_key_t *cur_key = cur_shard->best_unpopped_key();
                if (is_better(*cur_key, *best_key, sorting)) {
                    best_shard = cur_shard;
                    best_key = cur_key;
                }
            }
            if (auto maybe_item = best_shard->pop()) {
                ret.push_back(*maybe_item);
            } else {
                break;
            }
        }
    } else {
        std::vector<size_t> active;
        active.reserve(pseudoshards.size());
        for (size_t i = 0; i < pseudoshards.size(); ++i) {
            active.push_back(i);
        }
        while (active.size() > 0) {
            // We try to interleave data from different shards so that batched
            // updates distribute load over all servers.
            for (size_t i = 0; i < active.size(); ++i) {
                if (auto maybe_item = pseudoshards[active[i]].pop()) {
                    ret.push_back(std::move(*maybe_item));
                } else {
                    std::swap(active[i], active[active.size() - 1]);
                    active.pop_back();
                }
            }
        }
    }
    // We should have aborted earlier if there was no data.  If this assert ever
    // becomes false, make sure that we can't get into a state where all shards
    // are marked saturated.
    r_sanity_check(ret.size() != 0);

    // Make sure `active_ranges` is in a clean state.
    for (auto &&ps : pseudoshards) ps.finish();
    bool seen_active = false;
    bool seen_saturated = false;
    for (auto &&pair : active_ranges->ranges) {
        switch (pair.second.state()) {
        case range_state_t::ACTIVE: seen_active = true; break;
        case range_state_t::SATURATED: seen_saturated = true; break;
        case range_state_t::EXHAUSTED: break;
        default: unreachable();
        }
    }
    if (!seen_active) {
        // We should always have marked a saturated shard as active if the last
        // active shard was exhausted.
        r_sanity_check(!seen_saturated);
        r_sanity_check(shards_exhausted());
    }

    return ret;
}

// RANGE/READGEN STUFF
rget_response_reader_t::rget_response_reader_t(
    const counted_t<real_table_t> &_table,
    scoped_ptr_t<readgen_t> &&_readgen)
    : table(_table),
      started(false),
      readgen(std::move(_readgen)),
      items_index(0) { }

void rget_response_reader_t::add_transformation(transform_variant_t &&tv) {
    r_sanity_check(!started);
    transforms.push_back(std::move(tv));
}

bool rget_response_reader_t::add_stamp(changefeed_stamp_t _stamp) {
    stamp = std::move(_stamp);
    return true;
}

boost::optional<active_state_t> rget_response_reader_t::get_active_state() {
    if (!stamp || !active_ranges || shard_stamp_infos.empty()) return boost::none;
    std::map<uuid_u, std::pair<key_range_t, uint64_t> > shard_last_read_stamps;
    for (const auto &range_pair : active_ranges->ranges) {
        for (const auto &hash_pair : range_pair.second.hash_ranges) {
            const auto stamp_it = shard_stamp_infos.find(hash_pair.second.cfeed_shard_id);
            r_sanity_check(stamp_it != shard_stamp_infos.end());

            key_range_t last_read_range(
                key_range_t::closed, stamp_it->second.last_read_start,
                key_range_t::open, std::max(stamp_it->second.last_read_start,
                                            hash_pair.second.key_range.left));
            shard_last_read_stamps.insert(std::make_pair(
                stamp_it->first,
                std::make_pair(last_read_range, stamp_it->second.stamp)));
        }
    }
    return active_state_t{
        std::move(shard_last_read_stamps),
        reql_version,
        DEBUG_ONLY(readgen->sindex_name())};
}

void rget_response_reader_t::accumulate(env_t *env,
                                        eager_acc_t *acc,
                                        const terminal_variant_t &tv) {
    r_sanity_check(!started);
    started = true;
    batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env);
    read_t read = readgen->terminal_read(transforms, tv, batchspec);
    result_t res = do_read(env, std::move(read)).result;
    mark_shards_exhausted();
    acc->add_res(env, &res, readgen->sorting(batchspec));
}

std::vector<datum_t> rget_response_reader_t::next_batch(
    env_t *env, const batchspec_t &batchspec) {
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

    if (res.size() == 0) {
        r_sanity_check(shards_exhausted());
    }

    return res;
}

bool rget_response_reader_t::is_finished() const {
    return shards_exhausted() && items_index >= items.size();
}

rget_read_response_t rget_response_reader_t::do_read(env_t *env, const read_t &read) {
    read_response_t res;
    table->read_with_profile(env, read, &res);
    auto rget_res = boost::get<rget_read_response_t>(&res.response);
    r_sanity_check(rget_res != NULL);
    if (auto e = boost::get<exc_t>(&rget_res->result)) {
        throw *e;
    }
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
    read_t read = readgen->next_read(
        active_ranges, reql_version, stamp, transforms, batchspec);
    rget_read_response_t resp = do_read(env, std::move(read));

    auto *rr = boost::get<rget_read_t>(&read.read);
    r_sanity_check(rr != nullptr);
    auto final_key = !reversed(rr->sorting) ? store_key_t::max() : store_key_t::min();
    auto *stream = boost::get<grouped_t<stream_t> >(&resp.result);
    r_sanity_check(stream != nullptr);
    for (auto &&pair : *stream) {
        for (auto &&stream_pair : pair.second.substreams) {
            r_sanity_check(stream_pair.second.last_key == final_key);
        }
    }
    mark_shards_exhausted();

    acc->add_res(env, &resp.result, readgen->sorting(batchspec));
}

std::vector<rget_item_t>
rget_reader_t::do_range_read(env_t *env, const read_t &read) {
    auto *rr = boost::get<rget_read_t>(&read.read);
    r_sanity_check(rr);
    rget_read_response_t res = do_read(env, read);

    r_sanity_check(static_cast<bool>(stamp) == static_cast<bool>(rr->stamp));
    if (stamp) {
        r_sanity_check(res.stamp_response);
        rcheck_datum(res.stamp_response->stamp_infos, base_exc_t::RESUMABLE_OP_FAILED,
                     "Unable to retrieve start stamps.  (Did you just reshard?)");
        rcheck_datum(res.stamp_response->stamp_infos->size() != 0,
                     base_exc_t::RESUMABLE_OP_FAILED,
                     "Empty start stamps.  Did you just reshard?");
        for (const auto &pair : *res.stamp_response->stamp_infos) {
            // It's OK to blow away old values.
            shard_stamp_infos[pair.first] = pair.second;
        }
    }

    return unshard(rr->sorting, std::move(res));
}

bool rget_reader_t::load_items(env_t *env, const batchspec_t &batchspec) {
    started = true;
    while (items_index >= items.size() && !shards_exhausted()) {
        items_index = 0;
        // `active_range` is guaranteed to be full after the `do_range_read`,
        // because `do_range_read` is responsible for updating the active range.
        items = do_range_read(
            env,
            readgen->next_read(
                active_ranges, reql_version, stamp, transforms, batchspec));
        r_sanity_check(active_ranges);
        readgen->sindex_sort(&items, batchspec);
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
    read_t read = readgen->next_read(
        active_ranges, reql_version, stamp, transforms, batchspec);
    rget_read_response_t resp = do_read(env, std::move(read));

    auto final_key = store_key_t::max();
    auto *stream = boost::get<grouped_t<stream_t> >(&resp.result);
    r_sanity_check(stream != nullptr);
    for (auto &&pair : *stream) {
        for (auto &&stream_pair : pair.second.substreams) {
            r_sanity_check(stream_pair.second.last_key == final_key);
        }
    }
    mark_shards_exhausted();

    acc->add_res(env, &resp.result, sorting_t::UNORDERED);
}

bool intersecting_reader_t::load_items(env_t *env, const batchspec_t &batchspec) {
    started = true;
    while (items_index >= items.size() && !shards_exhausted()) { // read some more
        std::vector<rget_item_t> unfiltered_items = do_intersecting_read(
            env,
            readgen->next_read(
                active_ranges, reql_version, stamp, transforms, batchspec));
        if (unfiltered_items.empty()) {
            r_sanity_check(shards_exhausted());
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

    auto *gr = boost::get<intersecting_geo_read_t>(&read.read);
    r_sanity_check(gr);
    r_sanity_check(gr->sindex.region);

    return unshard(sorting_t::UNORDERED, std::move(res));
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
      sorting_(_sorting) { }

rget_readgen_t::rget_readgen_t(
    global_optargs_t _global_optargs,
    std::string _table_name,
    const datumspec_t &_datumspec,
    profile_bool_t _profile,
    read_mode_t _read_mode,
    sorting_t _sorting)
    : readgen_t(std::move(_global_optargs),
                std::move(_table_name),
                _profile, _read_mode, _sorting),
      datumspec(_datumspec) { }

read_t rget_readgen_t::next_read(
    const boost::optional<active_ranges_t> &active_ranges,
    const boost::optional<reql_version_t> &reql_version,
    boost::optional<changefeed_stamp_t> stamp,
    std::vector<transform_variant_t> transforms,
    const batchspec_t &batchspec) const {
    return read_t(
        next_read_impl(
            active_ranges,
            reql_version,
            std::move(stamp),
            std::move(transforms),
            batchspec),
        profile,
        stamp ? up_to_date_read_mode(read_mode) : read_mode);
}

sorting_t readgen_t::sorting(const batchspec_t &batchspec) const {
    return batchspec.lazy_sorting(sorting_);
}

// TODO: this is how we did it before, but it sucks.
read_t rget_readgen_t::terminal_read(
    const std::vector<transform_variant_t> &transforms,
    const terminal_variant_t &_terminal,
    const batchspec_t &batchspec) const {
    rget_read_t read = next_read_impl(
        boost::none, // No active ranges, just use the original range.
        boost::none, // No reql version yet.
        boost::optional<changefeed_stamp_t>(), // No need to stamp terminals.
        transforms,
        batchspec);
    read.terminal = _terminal;
    return read_t(read, profile, read_mode);
}

primary_readgen_t::primary_readgen_t(
    global_optargs_t global_optargs,
    std::string table_name,
    const datumspec_t &datumspec,
    profile_bool_t _profile,
    read_mode_t _read_mode,
    sorting_t sorting)
    : rget_readgen_t(
        std::move(global_optargs),
        std::move(table_name),
        datumspec,
        _profile,
        _read_mode,
        sorting) {
    store_keys = datumspec.primary_key_map();
}

// If we're doing a primary key `get_all`, then we want to restrict the active
// ranges when we first calculate them so that we never try to read past the end
// of the set of store keys.  (This prevents extra reads from being issued and
// preserves the invariant that we always issue a read to a shard if it's in the
// active ranges, which is how we detect resharding right now.)
void primary_readgen_t::restrict_active_ranges(
    sorting_t sorting,
    active_ranges_t *ranges_inout) const {
    if (store_keys && ranges_inout->ranges.size() != 0) {
        std::map<key_range_t,
                 std::map<hash_range_t,
                          std::pair<store_key_t, store_key_t> > > limits;
        for (auto &&pair : ranges_inout->ranges) {
            for (auto &&hash_pair : pair.second.hash_ranges) {
                limits[pair.first][hash_pair.first] =
                    std::make_pair(store_key_t::max(), store_key_t::min());
            }
        }
        for (const auto &key_pair : *store_keys) {
            const store_key_t &key = key_pair.first;
            uint64_t hash = hash_region_hasher(key);
            for (auto &&pair : limits) {
                if (pair.first.contains_key(key)) {
                    for (auto &&hash_pair : pair.second) {
                        if (hash_pair.first.contains(hash)) {
                            hash_pair.second.first =
                                std::min(hash_pair.second.first, key);
                            hash_pair.second.second =
                                std::max(hash_pair.second.second, key);
                            break;
                        }
                    }
                    break;
                }
            }
        }
        for (auto &&pair : ranges_inout->ranges) {
            for (auto &&hash_pair : pair.second.hash_ranges) {
                const auto &keypair = limits[pair.first][hash_pair.first];
                store_key_t new_start =
                    std::max(keypair.first, hash_pair.second.key_range.left);
                store_key_t new_end = keypair.second;
                new_end.increment();
                new_end = std::min(new_end, hash_pair.second.key_range.right_or_max());

                if (new_start > new_end) {
                    if (!reversed(sorting)) {
                        hash_pair.second.key_range.left
                            = hash_pair.second.key_range.right_or_max();
                    } else {
                        hash_pair.second.key_range.right.internal_key
                            = hash_pair.second.key_range.left;
                    }
                    guarantee(hash_pair.second.key_range.is_empty());
                } else {
                    hash_pair.second.key_range.left = new_start;
                    hash_pair.second.key_range.right.internal_key = new_end;
                    guarantee(!hash_pair.second.key_range.is_empty()
                              || new_start == new_end);
                }
                if (hash_pair.second.key_range.is_empty()) {
                    hash_pair.second.state = range_state_t::EXHAUSTED;
                }
            }
        }
    }
}

scoped_ptr_t<readgen_t> primary_readgen_t::make(
    env_t *env,
    std::string table_name,
    read_mode_t read_mode,
    const datumspec_t &datumspec,
    sorting_t sorting) {
    return scoped_ptr_t<readgen_t>(
        new primary_readgen_t(
            env->get_all_optargs(),
            std::move(table_name),
            datumspec,
            env->profile(),
            read_mode,
            sorting));
}

rget_read_t primary_readgen_t::next_read_impl(
    const boost::optional<active_ranges_t> &active_ranges,
    const boost::optional<reql_version_t> &,
    boost::optional<changefeed_stamp_t> stamp,
    std::vector<transform_variant_t> transforms,
    const batchspec_t &batchspec) const {
    region_t region = active_ranges
        ? region_t(active_ranges_to_range(*active_ranges))
        : region_t(datumspec.covering_range().to_primary_keyrange());
    r_sanity_check(!region.inner.is_empty());
    return rget_read_t(
        std::move(stamp),
        std::move(region),
        active_ranges_to_hints(sorting(batchspec), active_ranges),
        store_keys,
        global_optargs,
        table_name,
        batchspec,
        std::move(transforms),
        boost::optional<terminal_variant_t>(),
        boost::optional<sindex_rangespec_t>(),
        sorting(batchspec));
}

void primary_readgen_t::sindex_sort(
    std::vector<rget_item_t> *, const batchspec_t &) const {
    return;
}

key_range_t primary_readgen_t::original_keyrange(reql_version_t) const {
    return datumspec.covering_range().to_primary_keyrange();
}

boost::optional<std::string> primary_readgen_t::sindex_name() const {
    return boost::optional<std::string>();
}

changefeed::keyspec_t::range_t primary_readgen_t::get_range_spec(
        std::vector<transform_variant_t> transforms) const {
    return changefeed::keyspec_t::range_t{
        std::move(transforms),
        sindex_name(),
        sorting_,
        datumspec};
}

sindex_readgen_t::sindex_readgen_t(
    global_optargs_t global_optargs,
    std::string table_name,
    const std::string &_sindex,
    const datumspec_t &datumspec,
    profile_bool_t _profile,
    read_mode_t _read_mode,
    sorting_t sorting)
    : rget_readgen_t(
        std::move(global_optargs),
        std::move(table_name),
        datumspec,
        _profile,
        _read_mode,
        sorting),
      sindex(_sindex),
      sent_first_read(false) { }

scoped_ptr_t<readgen_t> sindex_readgen_t::make(
    env_t *env,
    std::string table_name,
    read_mode_t read_mode,
    const std::string &sindex,
    const datumspec_t &datumspec,
    sorting_t sorting) {
    return scoped_ptr_t<readgen_t>(
        new sindex_readgen_t(
            env->get_all_optargs(),
            std::move(table_name),
            sindex,
            datumspec,
            env->profile(),
            read_mode,
            sorting));
}

void sindex_readgen_t::sindex_sort(
    std::vector<rget_item_t> *vec,
    const batchspec_t &batchspec) const {
    if (vec->size() == 0) {
        return;
    }
    if (sorting(batchspec) != sorting_t::UNORDERED) {
        std::stable_sort(vec->begin(), vec->end(),
                         sindex_compare_t(sorting(batchspec)));
    }
}

rget_read_t sindex_readgen_t::next_read_impl(
    const boost::optional<active_ranges_t> &active_ranges,
    const boost::optional<reql_version_t> &reql_version,
    boost::optional<changefeed_stamp_t> stamp,
    std::vector<transform_variant_t> transforms,
    const batchspec_t &batchspec) const {

    boost::optional<region_t> region;
    datumspec_t ds;
    if (active_ranges) {
        region = region_t(active_ranges_to_range(*active_ranges));
        r_sanity_check(reql_version);
        ds = datumspec.trim_secondary(region->inner, *reql_version);
    } else {
        ds = datumspec;
        // We should send at most one read before we're able to calculate the
        // active range.
        r_sanity_check(!sent_first_read);
        // We cheat here because we're just using this for an assert.  In terms
        // of actual behavior, the function is const, and this assert will go
        // away once we drop support for pre-1.16 sindex key skey_version.
        const_cast<sindex_readgen_t *>(this)->sent_first_read = true;
    }
    r_sanity_check(!ds.is_empty());

    return rget_read_t(
        std::move(stamp),
        region_t::universe(),
        active_ranges_to_hints(sorting(batchspec), active_ranges),
        boost::none,
        global_optargs,
        table_name,
        batchspec,
        std::move(transforms),
        boost::optional<terminal_variant_t>(),
        sindex_rangespec_t(sindex, std::move(region), std::move(ds)),
        sorting(batchspec));
}

key_range_t sindex_readgen_t::original_keyrange(reql_version_t rv) const {
    return datumspec.covering_range().to_sindex_keyrange(rv);
}

boost::optional<std::string> sindex_readgen_t::sindex_name() const {
    return sindex;
}

changefeed::keyspec_t::range_t sindex_readgen_t::get_range_spec(
        std::vector<transform_variant_t> transforms) const {
    return changefeed::keyspec_t::range_t{
        std::move(transforms), sindex_name(), sorting_, datumspec};
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
    const boost::optional<active_ranges_t> &active_ranges,
    const boost::optional<reql_version_t> &reql_version,
    boost::optional<changefeed_stamp_t> stamp,
    std::vector<transform_variant_t> transforms,
    const batchspec_t &batchspec) const {
    return read_t(
        next_read_impl(
            active_ranges,
            reql_version,
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
            boost::none,
            boost::none,
            boost::optional<changefeed_stamp_t>(), // No need to stamp terminals.
            transforms,
            batchspec);
    read.terminal = _terminal;
    return read_t(read, profile, read_mode);
}

intersecting_geo_read_t intersecting_readgen_t::next_read_impl(
    const boost::optional<active_ranges_t> &active_ranges,
    const boost::optional<reql_version_t> &,
    boost::optional<changefeed_stamp_t> stamp,
    std::vector<transform_variant_t> transforms,
    const batchspec_t &batchspec) const {
    region_t region = active_ranges
        ? region_t(active_ranges_to_range(*active_ranges))
        : region_t(safe_universe());
    return intersecting_geo_read_t(
        std::move(stamp),
        region_t::universe(),
        global_optargs,
        table_name,
        batchspec,
        std::move(transforms),
        boost::optional<terminal_variant_t>(),
        sindex_rangespec_t(
            sindex,
            std::move(region),
            datumspec_t(datum_range_t::universe())),
        query_geometry);
}

void intersecting_readgen_t::sindex_sort(
    std::vector<rget_item_t> *,
    const batchspec_t &) const {
    // No sorting required for intersection queries, since they don't
    // support any specific ordering.
}

key_range_t intersecting_readgen_t::original_keyrange(reql_version_t rv) const {
    // This is always universe for intersection reads.
    // The real query is in the query geometry.
    return datum_range_t::universe().to_sindex_keyrange(rv);
}

boost::optional<std::string> intersecting_readgen_t::sindex_name() const {
    return sindex;
}

bool datum_stream_t::add_stamp(changefeed_stamp_t) {
    // By default most datum streams can't stamp their responses.
    return false;
}

boost::optional<active_state_t> datum_stream_t::get_active_state() {
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
            (**it)(env, out, []() { return datum_t(); });
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
