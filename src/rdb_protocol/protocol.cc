// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/protocol.hpp"

#include <algorithm>
#include <functional>

#include "stl_utils.hpp"
#include "boost_utils.hpp"

#include "btree/operations.hpp"
#include "btree/reql_specific.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/disk_backed_queue.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/changefeed.hpp"
#include "rdb_protocol/distribution_progress.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/store.hpp"

#include "debug.hpp"

store_key_t key_max(sorting_t sorting) {
    return !reversed(sorting) ? store_key_t::max() : store_key_t::min();
}

namespace rdb_protocol {

void post_construct_and_drain_queue(
        auto_drainer_t::lock_t lock,
        uuid_u sindex_id_to_bring_up_to_date,
        key_range_t *construction_range_inout,
        int64_t max_pairs_to_construct,
        store_t *store,
        scoped_ptr_t<disk_backed_queue_wrapper_t<rdb_modification_report_t> >
            &&mod_queue)
    THROWS_NOTHING;

/* Creates a queue of operations for the sindex, runs a post construction for
 * the data already in the btree and finally drains the queue. */
void resume_construct_sindex(
        const uuid_u &sindex_to_construct,
        const key_range_t &construct_range,
        store_t *store,
        auto_drainer_t::lock_t store_keepalive) THROWS_NOTHING {
    with_priority_t p(CORO_PRIORITY_SINDEX_CONSTRUCTION);

    // Block out backfills to improve performance.
    // A very useful side-effect of this is that if a server is added as a new replica
    // to a table, its indexes get constructed before the backfill can complete.
    // This makes sure that the indexes are actually ready by the time the new replica
    // becomes active.
    //
    // Note that we don't actually wait for the lock to be acquired.
    // All we want is to pause backfills by having our write lock acquisition
    // in line.
    // Waiting for the write lock would restrict us to having only one post
    // construction active at any time (which would make constructing multiple indexes
    // at the same time less efficient).
    rwlock_in_line_t backfill_lock_acq(&store->backfill_postcon_lock, access_t::write);

    // Used by the `jobs` table and `indexStatus` to track the progress of the
    // construction.
    distribution_progress_estimator_t progress_estimator(
        store, store_keepalive.get_drain_signal());
    double current_progress =
        progress_estimator.estimate_progress(construct_range.left);
    map_insertion_sentry_t<
        store_t::sindex_context_map_t::key_type,
        store_t::sindex_context_map_t::mapped_type> sindex_context_sentry(
            store->get_sindex_context_map(),
            sindex_to_construct,
            std::make_pair(current_microtime(), &current_progress));

    /* We start by clearing out any residual data in the index left behind by a previous
    post construction process (if the server got terminated in the middle). */
    try {
        /* It's safe to use a noop deletion context because this part of the index has
        never been live. */
        rdb_noop_deletion_context_t noop_deletion_context;
        rdb_value_sizer_t sizer(store->cache->max_block_size());
        store->clear_sindex_data(
            sindex_to_construct,
            &sizer,
            &noop_deletion_context,
            construct_range,
            store_keepalive.get_drain_signal());
    } catch (const interrupted_exc_t &) {
        return;
    }

    uuid_u post_construct_id = generate_uuid();

    /* Secondary indexes are constructed in multiple passes, moving through the primary
    key range from the smallest key to the largest one. In each pass, we handle a
    certain number of primary keys and put the corresponding entries into the secondary
    index. While this happens, we use a queue to keep track of any writes to the range
    we're constructing. We then drain the queue and atomically delete it, before we
    start the next pass. */
    const int64_t PAIRS_TO_CONSTRUCT_PER_PASS = 512;
    key_range_t remaining_range = construct_range;
    while (!remaining_range.is_empty()) {
        scoped_ptr_t<disk_backed_queue_wrapper_t<rdb_modification_report_t> > mod_queue;
        {
            /* Start a transaction and acquire the sindex_block */
            write_token_t token;
            store->new_write_token(&token);
            scoped_ptr_t<txn_t> txn;
            scoped_ptr_t<real_superblock_t> superblock;
            try {
                store->acquire_superblock_for_write(1,
                                                    write_durability_t::SOFT,
                                                    &token,
                                                    &txn,
                                                    &superblock,
                                                    store_keepalive.get_drain_signal());
            } catch (const interrupted_exc_t &) {
                return;
            }
            buf_lock_t sindex_block(superblock->expose_buf(),
                                    superblock->get_sindex_block_id(),
                                    access_t::write);
            superblock.reset();

            /* We register our modification queue here.
             * We must register it before calling post_construct_and_drain_queue to
             * make sure that every changes which we don't learn about in
             * the concurrent traversal that's started there, we do learn about from the
             * mod queue. Changes that happen between the mod queue registration and
             * the parallel traversal will be accounted for twice. That is ok though,
             * since every modification can be applied repeatedly without causing any
             * damage (if that should ever not true for any of the modifications, that
             * modification must be fixed or this code would have to be changed to
             * account for that). */
            const int64_t MAX_MOD_QUEUE_MEMORY_BYTES = 8 * MEGABYTE;
            mod_queue.init(
                    new disk_backed_queue_wrapper_t<rdb_modification_report_t>(
                        store->io_backender_,
                        serializer_filepath_t(
                            store->base_path_,
                            "post_construction_" + uuid_to_str(post_construct_id)),
                        &store->perfmon_collection,
                        MAX_MOD_QUEUE_MEMORY_BYTES));

            secondary_index_t sindex;
            bool found_index =
                get_secondary_index(&sindex_block, sindex_to_construct, &sindex);
            if (!found_index || sindex.being_deleted) {
                // The index was deleted. Abort construction.
                return;
            }

            new_mutex_in_line_t acq =
                store->get_in_line_for_sindex_queue(&sindex_block);
            store->register_sindex_queue(mod_queue.get(), remaining_range, &acq);
        }

        // This updates `remaining_range`.
        post_construct_and_drain_queue(
            store_keepalive,
            sindex_to_construct,
            &remaining_range,
            PAIRS_TO_CONSTRUCT_PER_PASS,
            store,
            std::move(mod_queue));

        // Update the progress value
        current_progress = progress_estimator.estimate_progress(remaining_range.left);
    }
}

/* This function is used by resume_construct_sindex. It traverses the primary btree
and creates entries in the given secondary index. It then applies outstanding changes
from the mod_queue and deregisters it. */
void post_construct_and_drain_queue(
        auto_drainer_t::lock_t lock,
        uuid_u sindex_id_to_bring_up_to_date,
        key_range_t *construction_range_inout,
        int64_t max_pairs_to_construct,
        store_t *store,
        scoped_ptr_t<disk_backed_queue_wrapper_t<rdb_modification_report_t> >
            &&mod_queue)
    THROWS_NOTHING {

    // `post_construct_secondary_index_range` can post-construct multiple secondary
    // indexes at the same time. We don't currently make use of that functionality
    // though. (it doesn't make sense to remove it, since it doesn't add anything to
    // its complexity).
    std::set<uuid_u> sindexes_to_bring_up_to_date;
    sindexes_to_bring_up_to_date.insert(sindex_id_to_bring_up_to_date);

    try {
        const size_t MOD_QUEUE_SIZE_LIMIT = 16;
        // This constructs a part of the index and updates `construction_range_inout`
        // to the range that's still remaining.
        post_construct_secondary_index_range(
            store,
            sindexes_to_bring_up_to_date,
            construction_range_inout,
            // Abort if the mod_queue gets larger than the `MOD_QUEUE_SIZE_LIMIT`, or
            // we've constructed `max_pairs_to_construct` pairs.
            [&](int64_t pairs_constructed) {
                return pairs_constructed >= max_pairs_to_construct
                    || mod_queue->size() > MOD_QUEUE_SIZE_LIMIT;
            },
            lock.get_drain_signal());

        // Drain the queue.
        {
            write_token_t token;
            store->new_write_token(&token);

            scoped_ptr_t<txn_t> queue_txn;
            scoped_ptr_t<real_superblock_t> queue_superblock;

            // We use HARD durability because we want post construction
            // to be throttled if we insert data faster than it can
            // be written to disk. Otherwise we might exhaust the cache's
            // dirty page limit and bring down the whole table.
            // Other than that, the hard durability guarantee is not actually
            // needed here.
            store->acquire_superblock_for_write(
                2 + mod_queue->size(),
                write_durability_t::HARD,
                &token,
                &queue_txn,
                &queue_superblock,
                lock.get_drain_signal());

            block_id_t sindex_block_id = queue_superblock->get_sindex_block_id();

            buf_lock_t queue_sindex_block(queue_superblock->expose_buf(),
                                          sindex_block_id,
                                          access_t::write);

            queue_superblock->release();

            store_t::sindex_access_vector_t sindexes;
            store->acquire_sindex_superblocks_for_write(
                    sindexes_to_bring_up_to_date,
                    &queue_sindex_block,
                    &sindexes);

            // Pretend that the indexes in `sindexes` have been post-constructed up to
            // the new range. This is important to make the call to
            // `rdb_update_sindexes()` below actually update the indexes.
            // We use the same trick in the `post_construct_traversal_helper_t`.
            // TODO: Avoid this hackery (here and in `post_construct_traversal_helper_t`)
            for (auto &&access : sindexes) {
                access->sindex.needs_post_construction_range = *construction_range_inout;
            }

            if (sindexes.empty()) {
                // We still need to deregister the queues. This is done further below
                // after the exception handler.
                // We throw here because that's consistent with how
                // `post_construct_secondary_index_range` signals the fact that all
                // indexes have been deleted.
                throw interrupted_exc_t();
            }

            new_mutex_in_line_t acq =
                store->get_in_line_for_sindex_queue(&queue_sindex_block);
            acq.acq_signal()->wait_lazily_unordered();

            while (mod_queue->size() > 0) {
                if (lock.get_drain_signal()->is_pulsed()) {
                    throw interrupted_exc_t();
                }
                // The `disk_backed_queue_wrapper` can sometimes be non-empty, but not
                // have a value available because it's still loading from disk.
                // In that case we must wait until a value becomes available.
                while (!mod_queue->available->get()) {
                    // TODO: The availability_callback_t interface on passive producers
                    //   is difficult to use and should be simplified.
                    struct on_availability_t : public availability_callback_t {
                        void on_source_availability_changed() {
                            cond.pulse_if_not_already_pulsed();
                        }
                        cond_t cond;
                    } on_availability;
                    mod_queue->available->set_callback(&on_availability);
                    try {
                        wait_interruptible(&on_availability.cond,
                                           lock.get_drain_signal());
                    } catch (const interrupted_exc_t &) {
                        mod_queue->available->unset_callback();
                        throw;
                    }
                    mod_queue->available->unset_callback();
                }
                rdb_modification_report_t mod_report = mod_queue->pop();
                // We only need to apply modifications that fall in the range that
                // has actually been constructed.
                // If it's in the range that is still to be constructed we ignore it.
                if (!construction_range_inout->contains_key(mod_report.primary_key)) {
                    rdb_post_construction_deletion_context_t deletion_context;
                    rdb_update_sindexes(store,
                                        sindexes,
                                        &mod_report,
                                        queue_txn.get(),
                                        &deletion_context,
                                        NULL,
                                        NULL,
                                        NULL);
                }
            }

            // Mark parts of the index up to date (except for what remains in
            // `construction_range_inout`).
            store->mark_index_up_to_date(sindex_id_to_bring_up_to_date,
                                         &queue_sindex_block,
                                         *construction_range_inout);
            store->deregister_sindex_queue(mod_queue.get(), &acq);
            return;
        }
    } catch (const interrupted_exc_t &) {
        // We were interrupted so we just exit. Sindex post construct is in an
        // indeterminate state and will be cleaned up at a later point.
    }

    if (lock.get_drain_signal()->is_pulsed()) {
        /* We were interrupted, this means we can't deregister the sindex queue
         * the standard way because it requires blocks. Use the emergency
         * method instead. */
        store->emergency_deregister_sindex_queue(mod_queue.get());
    } else {
        /* The sindexes we were post constructing were all deleted. Time to
         * deregister the queue. */
        write_token_t token;
        store->new_write_token(&token);

        scoped_ptr_t<txn_t> queue_txn;
        scoped_ptr_t<real_superblock_t> queue_superblock;

        cond_t non_interruptor;
        store->acquire_superblock_for_write(
            2,
            write_durability_t::HARD,
            &token,
            &queue_txn,
            &queue_superblock,
            &non_interruptor);

        block_id_t sindex_block_id = queue_superblock->get_sindex_block_id();

        buf_lock_t queue_sindex_block(queue_superblock->expose_buf(),
                                      sindex_block_id,
                                      access_t::write);

        queue_superblock->release();

        new_mutex_in_line_t acq =
            store->get_in_line_for_sindex_queue(&queue_sindex_block);
        store->deregister_sindex_queue(mod_queue.get(), &acq);
    }
}


bool range_key_tester_t::key_should_be_erased(const btree_key_t *key) {
    uint64_t h = hash_region_hasher(key);
    return delete_range->beg <= h && h < delete_range->end
        && delete_range->inner.contains_key(key->contents, key->size);
}

}  // namespace rdb_protocol

namespace rdb_protocol {
// Construct a region containing only the specified key
region_t monokey_region(const store_key_t &k) {
    uint64_t h = hash_region_hasher(k);
    return region_t(h, h + 1, key_range_t(key_range_t::closed, k, key_range_t::closed, k));
}

key_range_t sindex_key_range(const store_key_t &start,
                             const store_key_t &end,
                             key_range_t::bound_t end_type) {

    const size_t max_trunc_size = ql::datum_t::max_trunc_size();

    // If `end` is not truncated and right bound is open, we don't increment the right
    // bound.
    guarantee(static_cast<size_t>(end.size()) <= max_trunc_size);
    store_key_t end_key;
    const bool end_is_truncated = static_cast<size_t>(end.size()) == max_trunc_size;
    // The key range we generate must be open on the right end because the keys in the
    // btree have extra data appended to the secondary key part.
    if (end_is_truncated) {
        // Since the key is already truncated, we must make it larger without making it
        // longer.
        std::string end_key_str(key_to_unescaped_str(end));
        while (end_key_str.length() > 0 &&
               end_key_str[end_key_str.length() - 1] == static_cast<char>(255)) {
            end_key_str.erase(end_key_str.length() - 1);
        }

        if (end_key_str.length() == 0) {
            end_key = store_key_t::max();
        } else {
            ++end_key_str[end_key_str.length() - 1];
            end_key = store_key_t(end_key_str);
        }
    } else if (end_type == key_range_t::bound_t::closed) {
        // `end` is not truncated, but the range is closed. We know that `end` is
        // currently terminated by a null byte. We can replace that by a '\1' to ensure
        // that any key in the btree with that exact secondary index value will be
        // included in the range.
        end_key = end;
        guarantee(end_key.size() > 0);
        guarantee(end_key.contents()[end_key.size() - 1] == 0);
        end_key.contents()[end_key.size() - 1] = 1;
    } else {
        end_key = end;
    }
    return key_range_t(key_range_t::closed, start, key_range_t::open, std::move(end_key));
}

}  // namespace rdb_protocol

/* read_t::get_region implementation */
struct rdb_r_get_region_visitor : public boost::static_visitor<region_t> {
    region_t operator()(const point_read_t &pr) const {
        return rdb_protocol::monokey_region(pr.key);
    }

    region_t operator()(const rget_read_t &rg) const {
        return rg.region;
    }

    region_t operator()(const intersecting_geo_read_t &gr) const {
        return gr.region;
    }

    region_t operator()(const nearest_geo_read_t &gr) const {
        return gr.region;
    }

    region_t operator()(const distribution_read_t &dg) const {
        return dg.region;
    }

    region_t operator()(const changefeed_subscribe_t &s) const {
        return s.shard_region;
    }

    region_t operator()(const changefeed_limit_subscribe_t &s) const {
        return s.region;
    }

    region_t operator()(const changefeed_stamp_t &t) const {
        return t.region;
    }

    region_t operator()(const changefeed_point_stamp_t &t) const {
        return rdb_protocol::monokey_region(t.key);
    }

    region_t operator()(const dummy_read_t &d) const {
        return d.region;
    }
};

region_t read_t::get_region() const THROWS_NOTHING {
    return boost::apply_visitor(rdb_r_get_region_visitor(), read);
}

struct rdb_r_shard_visitor_t : public boost::static_visitor<bool> {
    explicit rdb_r_shard_visitor_t(const hash_region_t<key_range_t> *_region,
                                   read_t::variant_t *_payload_out)
        : region(*_region), payload_out(_payload_out) {
        // One day we'll get rid of unbounded right bounds, but until then
        // we canonicalize them here because otherwise life sucks.
        if (region.inner.right.unbounded) {
            region.inner.right = key_range_t::right_bound_t(store_key_t::max());
        }
    }

    // The key was somehow already extracted from the arg.
    template <class T>
    bool keyed_read(const T &arg, const store_key_t &key) const {
        if (region_contains_key(region, key)) {
            *payload_out = arg;
            return true;
        } else {
            return false;
        }
    }

    bool operator()(const point_read_t &pr) const {
        return keyed_read(pr, pr.key);
    }

    template <class T>
    bool rangey_read(const T &arg) const {
        const hash_region_t<key_range_t> intersection
            = region_intersection(region, arg.region);
        if (!region_is_empty(intersection)) {
            T tmp = arg;
            tmp.region = intersection;
            *payload_out = std::move(tmp);
            return true;
        } else {
            return false;
        }
    }

    bool operator()(const changefeed_subscribe_t &s) const {
        changefeed_subscribe_t tmp = s;
        tmp.shard_region = region;
        *payload_out = std::move(tmp);
        return true;
    }

    bool operator()(const changefeed_limit_subscribe_t &s) const {
        bool do_read = rangey_read(s);
        if (do_read) {
            auto *out = boost::get<changefeed_limit_subscribe_t>(payload_out);
            out->current_shard = region;
        }
        return do_read;
    }

    bool operator()(const changefeed_stamp_t &t) const {
        return rangey_read(t);
    }

    bool operator()(const changefeed_point_stamp_t &t) const {
        return keyed_read(t, t.key);
    }

    bool operator()(const rget_read_t &rg) const {
        bool do_read;
        if (rg.hints) {
            auto it = rg.hints->find(region);
            if (it != rg.hints->end()) {
                do_read = rangey_read(rg);
                auto *rr = boost::get<rget_read_t>(payload_out);
                guarantee(rr);
                if (do_read) {
                    // TODO: We could avoid an `std::map` copy by making
                    // `rangey_read` smarter.
                    rr->hints = boost::none;
                    if (!rg.sindex) {
                        if (!reversed(rg.sorting)) {
                            guarantee(it->second >= rr->region.inner.left);
                            rr->region.inner.left = it->second;
                        } else {
                            key_range_t::right_bound_t rb(it->second);
                            guarantee(rb <= rr->region.inner.right);
                            rr->region.inner.right = rb;
                        }
                        r_sanity_check(!rr->region.inner.is_empty());
                    } else {
                        guarantee(rr->sindex);
                        guarantee(rr->sindex->region);
                        if (!reversed(rg.sorting)) {
                            rr->sindex->region->inner.left = it->second;
                        } else {
                            rr->sindex->region->inner.right =
                                key_range_t::right_bound_t(it->second);
                        }
                        r_sanity_check(!rr->sindex->region->inner.is_empty());
                    }
                }
            } else {
                do_read = false;
            }
        } else {
            do_read = rangey_read(rg);
        }
        if (do_read) {
            auto *rg_out = boost::get<rget_read_t>(payload_out);
            guarantee(!region.inner.right.unbounded);
            rg_out->current_shard = region;
            rg_out->batchspec = rg_out->batchspec.scale_down(
                rg.hints ? rg.hints->size() : CPU_SHARDING_FACTOR);
            if (static_cast<bool>(rg_out->primary_keys)) {
                for (auto it = rg_out->primary_keys->begin();
                     it != rg_out->primary_keys->end();) {
                    auto cur_it = it++;
                    if (!region_contains_key(rg_out->region, cur_it->first)) {
                        rg_out->primary_keys->erase(cur_it);
                    }
                }
                if (rg_out->primary_keys->empty()) {
                    return false;
                }
            }
            if (rg_out->stamp) {
                rg_out->stamp->region = rg_out->region;
            }
        }
        return do_read;
    }

    bool operator()(const intersecting_geo_read_t &gr) const {
        bool do_read = rangey_read(gr);
        if (do_read) {
            // If we're in an include_initial changefeed, we need to copy the
            // new region onto the stamp.
            auto *out = boost::get<intersecting_geo_read_t>(payload_out);
            if (out->stamp) {
                out->stamp->region = out->region;
            }
        }
        return do_read;
    }

    bool operator()(const nearest_geo_read_t &gr) const {
        return rangey_read(gr);
    }

    bool operator()(const distribution_read_t &dg) const {
        return rangey_read(dg);
    }

    bool operator()(const dummy_read_t &d) const {
        return rangey_read(d);
    }

    region_t region;
    read_t::variant_t *payload_out;
};

bool read_t::shard(const hash_region_t<key_range_t> &region,
                   read_t *read_out) const THROWS_NOTHING {
    read_t::variant_t payload;
    bool result = boost::apply_visitor(rdb_r_shard_visitor_t(&region, &payload), read);
    *read_out = read_t(payload, profile, read_mode);
    return result;
}

/* A visitor to handle this unsharding process for us. */

class distribution_read_response_less_t {
public:
    bool operator()(const distribution_read_response_t& x,
                    const distribution_read_response_t& y) {
        if (x.region.inner == y.region.inner) {
            return x.region < y.region;
        } else {
            return x.region.inner < y.region.inner;
        }
    }
};

// Scale the distribution down by combining ranges to fit it within the limit of
// the query
void scale_down_distribution(size_t result_limit, std::map<store_key_t, int64_t> *key_counts) {
    guarantee(result_limit > 0);
    const size_t combine = (key_counts->size() / result_limit); // Combine this many other ranges into the previous range
    for (std::map<store_key_t, int64_t>::iterator it = key_counts->begin(); it != key_counts->end(); ) {
        std::map<store_key_t, int64_t>::iterator next = it;
        ++next;
        for (size_t i = 0; i < combine && next != key_counts->end(); ++i) {
            it->second += next->second;
            std::map<store_key_t, int64_t>::iterator tmp = next;
            ++next;
            key_counts->erase(tmp);
        }
        it = next;
    }
}

class rdb_r_unshard_visitor_t : public boost::static_visitor<void> {
public:
    rdb_r_unshard_visitor_t(profile_bool_t _profile,
                            read_response_t *_responses,
                            size_t _count,
                            read_response_t *_response_out,
                            rdb_context_t *_ctx,
                            signal_t *_interruptor)
        : profile(_profile), responses(_responses),
          count(_count), response_out(_response_out),
          ctx(_ctx), interruptor(_interruptor) { }

    void operator()(const point_read_t &);

    void operator()(const rget_read_t &rg);
    void operator()(const intersecting_geo_read_t &gr);
    void operator()(const nearest_geo_read_t &gr);
    void operator()(const distribution_read_t &rg);
    void operator()(const changefeed_subscribe_t &);
    void operator()(const changefeed_limit_subscribe_t &);
    void operator()(const changefeed_stamp_t &);
    void operator()(const changefeed_point_stamp_t &);
    void operator()(const dummy_read_t &);

private:
    // Shared by rget_read_t and intersecting_geo_read_t operators
    template<class query_response_t, class query_t>
    void unshard_range_batch(const query_t &q, sorting_t sorting);

    const profile_bool_t profile;
    read_response_t *const responses; // Cannibalized for efficiency.
    const size_t count;
    read_response_t *const response_out;
    rdb_context_t *const ctx;
    signal_t *const interruptor;
};

void rdb_r_unshard_visitor_t::operator()(const changefeed_subscribe_t &) {
    response_out->response = changefeed_subscribe_response_t();
    auto out = boost::get<changefeed_subscribe_response_t>(&response_out->response);
    for (size_t i = 0; i < count; ++i) {
        auto res = boost::get<changefeed_subscribe_response_t>(
            &responses[i].response);
        for (auto it = res->addrs.begin(); it != res->addrs.end(); ++it) {
            out->addrs.insert(std::move(*it));
        }
        for (auto it = res->server_uuids.begin();
             it != res->server_uuids.end(); ++it) {
            out->server_uuids.insert(std::move(*it));
        }
    }
}

void rdb_r_unshard_visitor_t::operator()(const changefeed_limit_subscribe_t &) {
    int64_t shards = 0;
    std::vector<ql::changefeed::server_t::limit_addr_t> limit_addrs;
    for (size_t i = 0; i < count; ++i) {
        auto res = boost::get<changefeed_limit_subscribe_response_t>(
            &responses[i].response);
        if (res == NULL) {
            response_out->response = std::move(responses[i].response);
            return;
        } else {
            shards += res->shards;
            std::move(res->limit_addrs.begin(), res->limit_addrs.end(),
                      std::back_inserter(limit_addrs));
        }
    }
    guarantee(count != 0);
    response_out->response =
        changefeed_limit_subscribe_response_t(shards, std::move(limit_addrs));
}

void unshard_stamps(const std::vector<changefeed_stamp_response_t *> &resps,
                    changefeed_stamp_response_t *out) {
    out->stamp_infos = std::map<uuid_u, shard_stamp_info_t>();
    for (auto &&resp : resps) {
        // In the error case abort early.
        if (!resp->stamp_infos) {
            out->stamp_infos = boost::none;
            return;
        }
        for (auto &&info_pair : *resp->stamp_infos) {
            // Previously conflicts were resolved with `it_out->second =
            // std::max(it->second, it_out->second)`, but I don't think that
            // should ever happen and it isn't correct for
            // `include_initial` changefeeds.
            bool inserted = out->stamp_infos->insert(info_pair).second;
            if (!inserted) {
                out->stamp_infos = boost::none;
                return;
            }
        }
    }
}

void rdb_r_unshard_visitor_t::operator()(const changefeed_stamp_t &) {
    response_out->response = changefeed_stamp_response_t();
    auto *out = boost::get<changefeed_stamp_response_t>(&response_out->response);
    guarantee(out != nullptr);
    std::vector<changefeed_stamp_response_t *> resps;
    resps.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        auto *resp = boost::get<changefeed_stamp_response_t>(&responses[i].response);
        guarantee(resp);
        resps.push_back(resp);
    }
    unshard_stamps(resps, out);
}

void rdb_r_unshard_visitor_t::operator()(const changefeed_point_stamp_t &) {
    guarantee(count == 1);
    guarantee(boost::get<changefeed_point_stamp_response_t>(&responses[0].response));
    *response_out = responses[0];
}

void rdb_r_unshard_visitor_t::operator()(const point_read_t &) {
    guarantee(count == 1);
    guarantee(NULL != boost::get<point_read_response_t>(&responses[0].response));
    *response_out = responses[0];
}

void rdb_r_unshard_visitor_t::operator()(const intersecting_geo_read_t &query) {
    unshard_range_batch<rget_read_response_t>(query, sorting_t::UNORDERED);
}

void rdb_r_unshard_visitor_t::operator()(const nearest_geo_read_t &query) {
    // Merge the different results together while preserving ordering.
    struct iter_range_t {
        iter_range_t(
                const nearest_geo_read_response_t::result_t::const_iterator &_beg,
                const nearest_geo_read_response_t::result_t::const_iterator &_end)
            : it(_beg), end(_end) { }
        nearest_geo_read_response_t::result_t::const_iterator it;
        nearest_geo_read_response_t::result_t::const_iterator end;
    };
    std::vector<iter_range_t> iters;
    iters.reserve(count);
    uint64_t total_size = 0;
    for (size_t i = 0; i < count; ++i) {
        auto res = boost::get<nearest_geo_read_response_t>(&responses[i].response);
        guarantee(res != NULL);
        ql::exc_t *error = boost::get<ql::exc_t>(&res->results_or_error);
        if (error != NULL) {
            response_out->response = nearest_geo_read_response_t(*error);
            return;
        }
        auto results =
            boost::get<nearest_geo_read_response_t::result_t>(&res->results_or_error);
        guarantee(results != NULL);

        if (!results->empty()) {
            iters.push_back(iter_range_t(results->begin(), results->end()));
        }
        total_size += results->size();
    }
    total_size = std::min(total_size, query.max_results);
    nearest_geo_read_response_t::result_t combined_results;
    combined_results.reserve(total_size);
    // Collect data until all iterators have been exhausted or we hit the
    // max_results limit.
    while (combined_results.size() < total_size) {
        rassert(!iters.empty());
        // Find the iter with the nearest result
        size_t nearest_it_idx = iters.size();
        double nearest_it_dist = -1.0;
        for (size_t i = 0; i < iters.size(); ++i) {
            if (nearest_it_idx == iters.size()
                || iters[i].it->first < nearest_it_dist) {
                nearest_it_idx = i;
                nearest_it_dist = iters[i].it->first;
            }
        }
        guarantee(nearest_it_idx < iters.size());
        combined_results.push_back(*iters[nearest_it_idx].it);
        ++iters[nearest_it_idx].it;
        if (iters[nearest_it_idx].it == iters[nearest_it_idx].end) {
            iters.erase(iters.begin() + nearest_it_idx);
        }
    }
    response_out->response = nearest_geo_read_response_t(
        std::move(combined_results));
}

void rdb_r_unshard_visitor_t::operator()(const rget_read_t &rg) {
    if (rg.hints && count != rg.hints->size()) {
        response_out->response = rget_read_response_t(
            ql::exc_t(ql::base_exc_t::OP_FAILED, "Read aborted by unshard operation.",
                      ql::backtrace_id_t::empty()));
    } else {
        unshard_range_batch<rget_read_response_t>(rg, rg.sorting);
    }
}

template<class query_response_t, class query_t>
void rdb_r_unshard_visitor_t::unshard_range_batch(const query_t &q, sorting_t sorting) {
    if (q.transforms.size() != 0 || q.terminal) {
        // This asserts that the optargs have been initialized.  (There is always a
        // 'db' optarg.)  We have the same assertion in rdb_read_visitor_t.
        rassert(q.optargs.has_optarg("db"));
    }
    scoped_ptr_t<profile::trace_t> trace = ql::maybe_make_profile_trace(profile);
    ql::env_t env(
        ctx,
        ql::return_empty_normal_batches_t::NO,
        interruptor,
        q.optargs,
        q.m_user_context,
        trace.get_or_null());

    // Initialize response.
    response_out->response = query_response_t();
    query_response_t *out = boost::get<query_response_t>(&response_out->response);
    out->reql_version = reql_version_t::EARLIEST;

    // Fill in `truncated` and `last_key`, get responses, abort if there's an error.
    std::vector<ql::result_t *> results(count);
    std::vector<changefeed_stamp_response_t *> stamp_resps(count);
    key_le_t key_le(sorting);
    for (size_t i = 0; i < count; ++i) {
        auto resp = boost::get<query_response_t>(&responses[i].response);
        guarantee(resp);
        // Make sure this is always the first thing in the loop.
        if (boost::get<ql::exc_t>(&resp->result) != NULL) {
            out->result = std::move(resp->result);
            return;
        }

        if (i == 0) {
            out->reql_version = resp->reql_version;
        } else {
#ifndef NDEBUG
            guarantee(out->reql_version == resp->reql_version);
#else
            if (out->reql_version != resp->reql_version) {
                out->result = ql::exc_t(
                    ql::base_exc_t::INTERNAL,
                    strprintf("Mismatched reql versions %d and %d.",
                              static_cast<int>(out->reql_version),
                              static_cast<int>(resp->reql_version)),
                    ql::backtrace_id_t::empty());
                return;
            }
#endif // NDEBUG
        }
        results[i] = &resp->result;
        if (q.stamp) {
            guarantee(resp->stamp_response);
            stamp_resps[i] = &*resp->stamp_response;
        }
    }
    if (q.stamp) {
        out->stamp_response = changefeed_stamp_response_t();
        unshard_stamps(stamp_resps, &*out->stamp_response);
    }

    // Unshard and finish up.
    try {
        scoped_ptr_t<ql::accumulator_t> acc(q.terminal
            ? ql::make_terminal(*q.terminal)
            : ql::make_unsharding_append());
        acc->unshard(&env, results);
        // The semantics here are that we aborted before ever iterating since no
        // iteration occurred (since we don't actually have a btree here).
        acc->finish(continue_bool_t::ABORT, &out->result);
    } catch (const ql::exc_t &ex) {
        *out = query_response_t(ex);
    }
}

void rdb_r_unshard_visitor_t::operator()(const distribution_read_t &dg) {
    // TODO: do this without copying so much and/or without dynamic memory
    // Sort results by region
    std::vector<distribution_read_response_t> results(count);
    guarantee(count > 0);

    for (size_t i = 0; i < count; ++i) {
        auto result = boost::get<distribution_read_response_t>(&responses[i].response);
        guarantee(result != NULL, "Bad boost::get\n");
        results[i] = *result; // TODO: move semantics.
    }

    std::sort(results.begin(), results.end(), distribution_read_response_less_t());

    distribution_read_response_t res;
    size_t i = 0;
    while (i < results.size()) {
        // Find the largest hash shard for this key range
        key_range_t range = results[i].region.inner;
        size_t largest_index = i;
        size_t largest_size = 0;
        size_t total_range_keys = 0;

        while (i < results.size() && results[i].region.inner == range) {
            size_t tmp_total_keys = 0;
            for (auto mit = results[i].key_counts.begin();
                 mit != results[i].key_counts.end();
                 ++mit) {
                tmp_total_keys += mit->second;
            }

            if (tmp_total_keys > largest_size) {
                largest_size = tmp_total_keys;
                largest_index = i;
            }

            total_range_keys += tmp_total_keys;
            ++i;
        }

        if (largest_size > 0) {
            // Scale up the selected hash shard
            double scale_factor =
                static_cast<double>(total_range_keys)
                / static_cast<double>(largest_size);

            guarantee(scale_factor >= 1.0);  // Directly provable from code above.

            for (auto mit = results[largest_index].key_counts.begin();
                 mit != results[largest_index].key_counts.end();
                 ++mit) {
                mit->second = static_cast<int64_t>(mit->second * scale_factor);
            }

            // TODO: move semantics.
            res.key_counts.insert(
                results[largest_index].key_counts.begin(),
                results[largest_index].key_counts.end());
        }
    }

    // If the result is larger than the requested limit, scale it down
    if (dg.result_limit > 0 && res.key_counts.size() > dg.result_limit) {
        scale_down_distribution(dg.result_limit, &res.key_counts);
    }

    response_out->response = res;
}

void rdb_r_unshard_visitor_t::operator()(const dummy_read_t &) {
    *response_out = responses[0];
}

void read_t::unshard(read_response_t *responses, size_t count,
                     read_response_t *response_out, rdb_context_t *ctx,
                     signal_t *interruptor) const
    THROWS_ONLY(interrupted_exc_t) {
    rassert(ctx != NULL);
    rdb_r_unshard_visitor_t v(profile, responses, count,
                              response_out, ctx, interruptor);
    boost::apply_visitor(v, read);

    /* We've got some profiling to do. */
    /* This is a tad hacky, some of the methods in rdb_r_unshard_visitor_t set
     * these fields because they just do dumb copies. So we clear them before
     * we set them here. */
    response_out->n_shards = 0;
    response_out->event_log.clear();
    if (profile == profile_bool_t::PROFILE) {
        for (size_t i = 0; i < count; ++i) {
            response_out->event_log.insert(
                response_out->event_log.end(),
                responses[i].event_log.begin(),
                responses[i].event_log.end());
            response_out->n_shards += responses[i].n_shards;
        }
    }
}

struct use_snapshot_visitor_t : public boost::static_visitor<bool> {
    bool operator()(const point_read_t &) const {                 return false; }
    bool operator()(const dummy_read_t &) const {                 return false; }

    // If the `rget_read_t` or `intersecting_geo_read_t` is part of an
    // `include_initial` changefeed, we can't snapshot yet, since we first need
    // to get the timestamps in an atomic fashion (i.e. without any writes
    // happening before we get there). We'll instead create a snapshot later
    // inside the `rdb_read_visitor_t`.
    bool operator()(const rget_read_t &rget) const {
        return !static_cast<bool>(rget.stamp);
    }
    bool operator()(const intersecting_geo_read_t &geo_read) const {
        return !static_cast<bool>(geo_read.stamp);
    }

    bool operator()(const nearest_geo_read_t &) const {           return true;  }
    bool operator()(const changefeed_subscribe_t &) const {       return false; }
    bool operator()(const changefeed_limit_subscribe_t &) const { return false; }
    bool operator()(const changefeed_stamp_t &) const {           return false; }
    bool operator()(const changefeed_point_stamp_t &) const {     return false; }
    bool operator()(const distribution_read_t &) const {          return true;  }
};

// Only use snapshotting if we're doing a range get.
bool read_t::use_snapshot() const THROWS_NOTHING {
    return boost::apply_visitor(use_snapshot_visitor_t(), read);
}

struct route_to_primary_visitor_t : public boost::static_visitor<bool> {
    // `include_initial` changefeed reads must be routed to the primary, since
    // that's where changefeeds are managed.
    bool operator()(const rget_read_t &rget) const {
        return static_cast<bool>(rget.stamp);
    }
    bool operator()(const intersecting_geo_read_t &geo_read) const {
        return static_cast<bool>(geo_read.stamp);
    }

    bool operator()(const point_read_t &) const {                 return false; }
    bool operator()(const dummy_read_t &) const {                 return false; }
    bool operator()(const nearest_geo_read_t &) const {           return false; }
    bool operator()(const changefeed_subscribe_t &) const {       return true;  }
    bool operator()(const changefeed_limit_subscribe_t &) const { return true;  }
    bool operator()(const changefeed_stamp_t &) const {           return true;  }
    bool operator()(const changefeed_point_stamp_t &) const {     return true;  }
    bool operator()(const distribution_read_t &) const {          return false; }
};

// Route changefeed reads to the primary replica. For other reads we don't care.
bool read_t::route_to_primary() const THROWS_NOTHING {
    return boost::apply_visitor(route_to_primary_visitor_t(), read);
}

/* write_t::get_region() implementation */

// TODO: This entire type is suspect, given the performance for
// batched_replaces_t.  Is it used in anything other than assertions?
region_t region_from_keys(const std::vector<store_key_t> &keys) {
    // It shouldn't be empty, but we let the places that would break use a
    // guarantee.
    rassert(!keys.empty());
    if (keys.empty()) {
        return hash_region_t<key_range_t>();
    }

    store_key_t min_key = store_key_t::max();
    store_key_t max_key = store_key_t::min();
    uint64_t min_hash_value = HASH_REGION_HASH_SIZE - 1;
    uint64_t max_hash_value = 0;

    for (auto it = keys.begin(); it != keys.end(); ++it) {
        const store_key_t &key = *it;
        if (key < min_key) {
            min_key = key;
        }
        if (key > max_key) {
            max_key = key;
        }

        const uint64_t hash_value = hash_region_hasher(key);
        if (hash_value < min_hash_value) {
            min_hash_value = hash_value;
        }
        if (hash_value > max_hash_value) {
            max_hash_value = hash_value;
        }
    }

    return hash_region_t<key_range_t>(
        min_hash_value, max_hash_value + 1,
        key_range_t(key_range_t::closed, min_key, key_range_t::closed, max_key));
}

struct rdb_w_get_region_visitor : public boost::static_visitor<region_t> {
    region_t operator()(const batched_replace_t &br) const {
        return region_from_keys(br.keys);
    }
    region_t operator()(const batched_insert_t &bi) const {
        std::vector<store_key_t> keys;
        keys.reserve(bi.inserts.size());
        for (auto it = bi.inserts.begin(); it != bi.inserts.end(); ++it) {
            keys.emplace_back((*it).get_field(datum_string_t(bi.pkey)).print_primary());
        }
        return region_from_keys(keys);
    }

    region_t operator()(const point_write_t &pw) const {
        return rdb_protocol::monokey_region(pw.key);
    }

    region_t operator()(const point_delete_t &pd) const {
        return rdb_protocol::monokey_region(pd.key);
    }

    region_t operator()(const changefeed_subscribe_t &s) const {
        return s.shard_region;
    }

    region_t operator()(const changefeed_limit_subscribe_t &s) const {
        return s.region;
    }

    region_t operator()(const changefeed_stamp_t &t) const {
        return t.region;
    }

    region_t operator()(const sync_t &s) const {
        return s.region;
    }

    region_t operator()(const dummy_write_t &d) const {
        return d.region;
    }
};

#ifndef NDEBUG
// This is slow, and should only be used in debug mode for assertions.
region_t write_t::get_region() const THROWS_NOTHING {
    return boost::apply_visitor(rdb_w_get_region_visitor(), write);
}
#endif // NDEBUG

/* write_t::shard implementation */

struct rdb_w_shard_visitor_t : public boost::static_visitor<bool> {
    rdb_w_shard_visitor_t(const region_t *_region, write_t::variant_t *_payload_out)
        : region(_region), payload_out(_payload_out) {}

    template <class T>
    bool keyed_write(const T &arg) const {
        if (region_contains_key(*region, arg.key)) {
            *payload_out = arg;
            return true;
        } else {
            return false;
        }
    }

    bool operator()(const batched_replace_t &br) const {
        std::vector<store_key_t> shard_keys;
        for (auto it = br.keys.begin(); it != br.keys.end(); ++it) {
            if (region_contains_key(*region, *it)) {
                shard_keys.push_back(*it);
            }
        }
        if (!shard_keys.empty()) {
            *payload_out = batched_replace_t(
                std::move(shard_keys),
                br.pkey,
                br.f.compile_wire_func(),
                br.optargs,
                br.m_user_context,
                br.return_changes);
            return true;
        } else {
            return false;
        }
    }

    bool operator()(const batched_insert_t &bi) const {
        std::vector<ql::datum_t> shard_inserts;
        for (auto it = bi.inserts.begin(); it != bi.inserts.end(); ++it) {
            store_key_t key((*it).get_field(datum_string_t(bi.pkey)).print_primary());
            if (region_contains_key(*region, key)) {
                shard_inserts.push_back(*it);
            }
        }

        if (!shard_inserts.empty()) {
            boost::optional<counted_t<const ql::func_t> > temp_conflict_func;
            if (bi.conflict_func) {
                temp_conflict_func = bi.conflict_func->compile_wire_func();
            }

            *payload_out = batched_insert_t(std::move(shard_inserts),
                                            bi.pkey,
                                            bi.conflict_behavior,
                                            temp_conflict_func,
                                            bi.limits,
                                            bi.m_user_context,
                                            bi.return_changes);
            return true;
        } else {
            return false;
        }
    }

    bool operator()(const point_write_t &pw) const {
        return keyed_write(pw);
    }

    bool operator()(const point_delete_t &pd) const {
        return keyed_write(pd);
    }

    template <class T>
    bool rangey_write(const T &arg) const {
        const hash_region_t<key_range_t> intersection
            = region_intersection(*region, arg.region);
        if (!region_is_empty(intersection)) {
            T tmp = arg;
            tmp.region = intersection;
            *payload_out = tmp;
            return true;
        } else {
            return false;
        }
    }

    bool operator()(const sync_t &s) const {
        return rangey_write(s);
    }

    bool operator()(const dummy_write_t &d) const {
        return rangey_write(d);
    }

    const region_t *region;
    write_t::variant_t *payload_out;
};

bool write_t::shard(const region_t &region,
                    write_t *write_out) const THROWS_NOTHING {
    write_t::variant_t payload;
    const rdb_w_shard_visitor_t v(&region, &payload);
    bool result = boost::apply_visitor(v, write);
    *write_out = write_t(payload, durability_requirement, profile, limits);
    return result;
}

batched_insert_t::batched_insert_t(
        std::vector<ql::datum_t> &&_inserts,
        const std::string &_pkey,
        conflict_behavior_t _conflict_behavior,
        boost::optional<counted_t<const ql::func_t> > _conflict_func,
        const ql::configured_limits_t &_limits,
        auth::user_context_t user_context,
        return_changes_t _return_changes)
        : inserts(std::move(_inserts)), pkey(_pkey),
          conflict_behavior(_conflict_behavior),
          limits(_limits),
          m_user_context(std::move(user_context)),
          return_changes(_return_changes) {
    r_sanity_check(inserts.size() != 0);

    if (_conflict_func) {
        conflict_func = ql::wire_func_t(*_conflict_func);
    }
#ifndef NDEBUG
    // These checks are done above us, but in debug mode we do them
    // again.  (They're slow.)  We do them above us because the code in
    // val.cc knows enough to report the write errors correctly while
    // still doing the other writes.
    for (auto it = inserts.begin(); it != inserts.end(); ++it) {
        ql::datum_t keyval =
            it->get_field(datum_string_t(pkey), ql::NOTHROW);
        r_sanity_check(keyval.has());
        try {
            keyval.print_primary(); // ERROR CHECKING
            continue;
        } catch (const ql::base_exc_t &e) {
        }
        r_sanity_check(false); // throws, so can't do this in exception handler
    }
#endif // NDEBUG
}

template <class T>
bool first_less(const std::pair<int64_t, T> &left, const std::pair<int64_t, T> &right) {
    return left.first < right.first;
}

struct rdb_w_unshard_visitor_t : public boost::static_visitor<void> {
    // The special case here is batched_replaces_response_t, which actually gets
    // sharded into multiple operations instead of getting sent unsplit in a
    // single direction.
    void merge_stats() const {
        ql::datum_t stats = ql::datum_t::empty_object();

        std::set<std::string> conditions;
        for (size_t i = 0; i < count; ++i) {
            const ql::datum_t *stats_i =
                boost::get<ql::datum_t>(&responses[i].response);
            guarantee(stats_i != NULL);
            stats = stats.merge(*stats_i, ql::stats_merge, *limits, &conditions);
        }
        ql::datum_object_builder_t result(stats);
        result.add_warnings(conditions, *limits);
        *response_out = write_response_t(std::move(result).to_datum());
    }
    void operator()(const batched_replace_t &) const {
        merge_stats();
    }

    void operator()(const batched_insert_t &) const {
        merge_stats();
    }

    void operator()(const point_write_t &) const { monokey_response(); }
    void operator()(const point_delete_t &) const { monokey_response(); }

    void operator()(const sync_t &) const {
        *response_out = responses[0];
    }

    void operator()(const dummy_write_t &) const {
        *response_out = responses[0];
    }

    rdb_w_unshard_visitor_t(const write_response_t *_responses, size_t _count,
                            write_response_t *_response_out,
                            const ql::configured_limits_t *_limits)
        : responses(_responses), count(_count), response_out(_response_out),
          limits(_limits) { }

private:
    void monokey_response() const {
        guarantee(count == 1,
                  "Response with count %zu (greater than 1) returned "
                  "for non-batched write.  (type = %d)",
                  count, responses[0].response.which());

        *response_out = write_response_t(responses[0]);
    }

    const write_response_t *const responses;
    const size_t count;
    write_response_t *const response_out;
    const ql::configured_limits_t *limits;
};

void write_t::unshard(write_response_t *responses, size_t count,
                      write_response_t *response_out, rdb_context_t *,
                      signal_t *)
    const THROWS_NOTHING {
    const rdb_w_unshard_visitor_t visitor(responses, count, response_out, &limits);
    boost::apply_visitor(visitor, write);

    /* We've got some profiling to do. */
    /* This is a tad hacky, some of the methods in rdb_w_unshard_visitor_t set
     * these fields because they just do dumb copies. So we clear them before
     * we set them here. */
    response_out->n_shards = 0;
    response_out->event_log.clear();
    if (profile == profile_bool_t::PROFILE) {
        for (size_t i = 0; i < count; ++i) {
            response_out->event_log.insert(
                response_out->event_log.end(),
                responses[i].event_log.begin(),
                responses[i].event_log.end());
            response_out->n_shards += responses[i].n_shards;
        }
    }
}

struct rdb_w_expected_document_changes_visitor_t : public boost::static_visitor<int> {
    rdb_w_expected_document_changes_visitor_t() { }
    int operator()(const batched_replace_t &w) const {
        return w.keys.size();
    }
    int operator()(const batched_insert_t &w) const {
        return w.inserts.size();
    }
    int operator()(const point_write_t &) const { return 1; }
    int operator()(const point_delete_t &) const { return 1; }
    int operator()(const sync_t &) const { return 0; }
    int operator()(const dummy_write_t &) const { return 0; }
};

int write_t::expected_document_changes() const {
    const rdb_w_expected_document_changes_visitor_t visitor;
    return boost::apply_visitor(visitor, write);
}

RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(point_read_response_t, data);
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
    ql::skey_version_t, int8_t,
    ql::skey_version_t::post_1_16, ql::skey_version_t::post_1_16);
RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(
    rget_read_response_t, stamp_response, result, reql_version);
RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(nearest_geo_read_response_t, results_or_error);
RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(distribution_read_response_t, region, key_counts);
RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(
    changefeed_subscribe_response_t, server_uuids, addrs);
RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(
    changefeed_limit_subscribe_response_t, shards, limit_addrs);
RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(
    shard_stamp_info_t, stamp, shard_region, last_read_start);
RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(changefeed_stamp_response_t, stamp_infos);

RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(
    changefeed_point_stamp_response_t::valid_response_t, stamp, initial_val);
RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(
    changefeed_point_stamp_response_t, resp);

RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(read_response_t, response, event_log, n_shards);
RDB_IMPL_SERIALIZABLE_0_FOR_CLUSTER(dummy_read_response_t);

RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(point_read_t, key);
RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(dummy_read_t, region);
RDB_IMPL_SERIALIZABLE_4_FOR_CLUSTER(sindex_rangespec_t,
                                    id,
                                    region,
                                    datumspec,
                                    require_sindex_val);

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        sorting_t, int8_t,
        sorting_t::UNORDERED, sorting_t::DESCENDING);
RDB_IMPL_SERIALIZABLE_13_FOR_CLUSTER(
    rget_read_t,
    stamp,
    region,
    current_shard,
    hints,
    primary_keys,
    optargs,
    m_user_context,
    table_name,
    batchspec,
    transforms,
    terminal,
    sindex,
    sorting);
RDB_IMPL_SERIALIZABLE_10_FOR_CLUSTER(
    intersecting_geo_read_t,
    stamp,
    region,
    optargs,
    m_user_context,
    table_name,
    batchspec,
    transforms,
    terminal,
    sindex,
    query_geometry);
RDB_IMPL_SERIALIZABLE_9_FOR_CLUSTER(
    nearest_geo_read_t,
    optargs,
    m_user_context,
    center,
    max_dist,
    max_results,
    geo_system,
    region,
    table_name,
    sindex_id);

RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(
        distribution_read_t, max_depth, result_limit, region);

RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(changefeed_subscribe_t, addr, shard_region);
RDB_IMPL_SERIALIZABLE_8_FOR_CLUSTER(
    changefeed_limit_subscribe_t,
    addr,
    uuid,
    spec,
    table,
    optargs,
    m_user_context,
    region,
    current_shard);
RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(changefeed_stamp_t, addr, region);
RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(changefeed_point_stamp_t, addr, key);

RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(read_t, read, profile, read_mode);

RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(point_write_response_t, result);
RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(point_delete_response_t, result);
RDB_IMPL_SERIALIZABLE_0_FOR_CLUSTER(sync_response_t);
RDB_IMPL_SERIALIZABLE_0_FOR_CLUSTER(dummy_write_response_t);

RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(write_response_t, response, event_log, n_shards);

RDB_IMPL_SERIALIZABLE_5_FOR_CLUSTER(
        batched_replace_t, keys, pkey, f, optargs, return_changes);
RDB_IMPL_SERIALIZABLE_7_FOR_CLUSTER(
        batched_insert_t,
        inserts,
        pkey,
        conflict_behavior,
        conflict_func,
        limits,
        m_user_context,
        return_changes);

RDB_IMPL_SERIALIZABLE_3_SINCE_v1_13(point_write_t, key, data, overwrite);
RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(point_delete_t, key);
RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(sync_t, region);
RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(dummy_write_t, region);

RDB_IMPL_SERIALIZABLE_4_FOR_CLUSTER(
    write_t, write, durability_requirement, profile, limits);


