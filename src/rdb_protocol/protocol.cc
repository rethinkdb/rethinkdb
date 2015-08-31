// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/protocol.hpp"

#include <algorithm>
#include <functional>

#include "btree/operations.hpp"
#include "btree/reql_specific.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "containers/archive/boost_types.hpp"
#include "containers/disk_backed_queue.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/changefeed.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/store.hpp"

#include "debug.hpp"

store_key_t key_max(sorting_t sorting) {
    return !reversed(sorting) ? store_key_t::max() : store_key_t::min();
}

#define RDB_IMPL_PROTOB_SERIALIZABLE(pb_t)                              \
    void serialize_protobuf(write_message_t *wm, const pb_t &p) {       \
        CT_ASSERT(sizeof(int) == sizeof(int32_t));                      \
        int size = p.ByteSize();                                        \
        scoped_array_t<char> data(size);                                \
        p.SerializeToArray(data.data(), size);                          \
        int32_t size32 = size;                                          \
        serialize_universal(wm, size32);                                \
        wm->append(data.data(), data.size());                           \
    }                                                                   \
                                                                        \
    MUST_USE archive_result_t deserialize_protobuf(read_stream_t *s, pb_t *p) { \
        CT_ASSERT(sizeof(int) == sizeof(int32_t));                      \
        int32_t size;                                                   \
        archive_result_t res = deserialize_universal(s, &size);         \
        if (bad(res)) { return res; }                                   \
        if (size < 0) { return archive_result_t::RANGE_ERROR; }         \
        scoped_array_t<char> data(size);                                \
        int64_t read_res = force_read(s, data.data(), data.size());     \
        if (read_res != size) { return archive_result_t::SOCK_ERROR; }  \
        p->ParseFromArray(data.data(), data.size());                    \
        return archive_result_t::SUCCESS;                               \
    }

RDB_IMPL_PROTOB_SERIALIZABLE(Term);
RDB_IMPL_PROTOB_SERIALIZABLE(Datum);
RDB_IMPL_PROTOB_SERIALIZABLE(Backtrace);

namespace rdb_protocol {

void post_construct_and_drain_queue(
        auto_drainer_t::lock_t lock,
        std::map<uuid_u, std::string> const &sindexes_to_bring_up_to_date_uuid_name,
        store_t *store,
        internal_disk_backed_queue_t *mod_queue_ptr)
    THROWS_NOTHING;

/* Creates a queue of operations for the sindex, runs a post construction for
 * the data already in the btree and finally drains the queue. */
void bring_sindexes_up_to_date(
        const std::set<sindex_name_t> &sindexes_to_bring_up_to_date,
        store_t *store,
        buf_lock_t *sindex_block)
    THROWS_NOTHING
{
    with_priority_t p(CORO_PRIORITY_SINDEX_CONSTRUCTION);

    /* We register our modification queue here.
     * We must register it before calling post_construct_and_drain_queue to
     * make sure that every changes which we don't learn about in
     * the parallel traversal that's started there, we do learn about from the mod
     * queue. Changes that happen between the mod queue registration and
     * the parallel traversal will be accounted for twice. That is ok though,
     * since every modification can be applied repeatedly without causing any
     * damage (if that should ever not true for any of the modifications, that
     * modification must be fixed or this code would have to be changed to account
     * for that). */
    uuid_u post_construct_id = generate_uuid();

    /* Keep the store alive for as long as mod_queue exists. It uses its io_backender
     * and perfmon_collection, so that is important. */
    auto_drainer_t::lock_t store_drainer_acq(&store->drainer);

    // TODO: This can now be a disk_backed_queue_t<rdb_modification_report_t>.
    scoped_ptr_t<internal_disk_backed_queue_t> mod_queue(
            new internal_disk_backed_queue_t(
                store->io_backender_,
                serializer_filepath_t(
                    store->base_path_,
                    "post_construction_" + uuid_to_str(post_construct_id)),
                &store->perfmon_collection));

    {
        new_mutex_in_line_t acq = store->get_in_line_for_sindex_queue(sindex_block);
        store->register_sindex_queue(mod_queue.get(), &acq);
    }

    std::map<sindex_name_t, secondary_index_t> sindexes;
    get_secondary_indexes(sindex_block, &sindexes);
    std::map<uuid_u, std::string> sindexes_to_bring_up_to_date_uuid_name;

    for (auto it = sindexes_to_bring_up_to_date.begin();
         it != sindexes_to_bring_up_to_date.end(); ++it) {
        guarantee(!it->being_deleted, "Trying to bring an index up to date that's "
                                      "being deleted");
        auto sindexes_it = sindexes.find(*it);
        guarantee(sindexes_it != sindexes.end());
        sindexes_to_bring_up_to_date_uuid_name.insert(
            std::make_pair(sindexes_it->second.id, sindexes_it->first.name));
    }

    coro_t::spawn_sometime(std::bind(
                &post_construct_and_drain_queue,
                store_drainer_acq,
                sindexes_to_bring_up_to_date_uuid_name,
                store,
                mod_queue.release()));
}

/* This function is really part of the logic of bring_sindexes_up_to_date
 * however it needs to be in a seperate function so that it can be spawned in a
 * coro.
 */
void post_construct_and_drain_queue(
        auto_drainer_t::lock_t lock,
        std::map<uuid_u, std::string> const &sindexes_to_bring_up_to_date_uuid_name,
        store_t *store,
        internal_disk_backed_queue_t *mod_queue_ptr)
    THROWS_NOTHING
{
    std::set<uuid_u> sindexes_to_bring_up_to_date;
    parallel_traversal_progress_t progress_tracker;
    std::vector<map_insertion_sentry_t<
        store_t::sindex_context_map_t::key_type,
        store_t::sindex_context_map_t::mapped_type> > sindex_context_sentries;
    for (auto const &sindex : sindexes_to_bring_up_to_date_uuid_name) {
        sindexes_to_bring_up_to_date.insert(sindex.first);
        sindex_context_sentries.emplace_back(
            store->get_sindex_context_map(),
            sindex.first,
            std::make_pair(current_microtime(), &progress_tracker));
    }

    scoped_ptr_t<internal_disk_backed_queue_t> mod_queue(mod_queue_ptr);

    rwlock_in_line_t lock_acq(&store->backfill_postcon_lock, access_t::write);
    // Note that we don't actually wait for the lock to be acquired.
    // All we want is to pause backfills by having our write lock acquisition
    // in line.
    // Waiting for the write lock would restrict us to having only one post
    // construction active at any time (which we might not want, for no specific
    // reason).

    try {
        post_construct_secondary_indexes(
            store,
            sindexes_to_bring_up_to_date,
            lock.get_drain_signal(),
            &progress_tracker);

        /* Drain the queue. */

        while (!lock.get_drain_signal()->is_pulsed()) {
            // Yield while we are not holding any locks yet.
            coro_t::yield();

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
                2,
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

            if (sindexes.empty()) {
                break;
            }

            new_mutex_in_line_t acq =
                store->get_in_line_for_sindex_queue(&queue_sindex_block);
            // TODO (daniel): Is there a way to release the queue_sindex_block
            // earlier than we do now, ideally before we wait for the acq signal?
            acq.acq_signal()->wait_lazily_unordered();

            const int MAX_CHUNK_SIZE = 10;
            int current_chunk_size = 0;
            while (current_chunk_size < MAX_CHUNK_SIZE && mod_queue->size() > 0) {
                rdb_modification_report_t mod_report;
                // This involves a disk backed queue so there are no versioning issues.
                deserializing_viewer_t<rdb_modification_report_t>
                    viewer(&mod_report);
                mod_queue->pop(&viewer);
                rdb_post_construction_deletion_context_t deletion_context;
                rdb_update_sindexes(store,
                                    sindexes,
                                    &mod_report,
                                    queue_txn.get(),
                                    &deletion_context,
                                    NULL,
                                    NULL,
                                    NULL);
                ++current_chunk_size;
            }

            if (mod_queue->size() == 0) {
                for (auto it = sindexes_to_bring_up_to_date.begin();
                     it != sindexes_to_bring_up_to_date.end(); ++it) {
                    store->mark_index_up_to_date(*it, &queue_sindex_block);
                }
                store->deregister_sindex_queue(mod_queue.get(), &acq);
                return;
            }
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

        store->acquire_superblock_for_write(
            2,
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
                             const store_key_t &end) {
    store_key_t end_key;
    std::string end_key_str(key_to_unescaped_str(end));

    // Need to make the next largest store_key_t without making the key longer
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
    return key_range_t(key_range_t::closed, start, key_range_t::open, end_key);
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
        return s.region;
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
        : region(_region), payload_out(_payload_out) { }

    // The key was somehow already extracted from the arg.
    template <class T>
    bool keyed_read(const T &arg, const store_key_t &key) const {
        if (region_contains_key(*region, key)) {
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

    bool operator()(const changefeed_subscribe_t &s) const {
        return rangey_read(s);
    }

    bool operator()(const changefeed_limit_subscribe_t &s) const {
        return rangey_read(s);
    }

    bool operator()(const changefeed_stamp_t &t) const {
        return rangey_read(t);
    }

    bool operator()(const changefeed_point_stamp_t &t) const {
        return keyed_read(t, t.key);
    }

    bool operator()(const rget_read_t &rg) const {
        bool do_read = rangey_read(rg);
        if (do_read) {
            auto rg_out = boost::get<rget_read_t>(payload_out);
            rg_out->batchspec = rg_out->batchspec.scale_down(CPU_SHARDING_FACTOR);
            if (rg_out->stamp) {
                rg_out->stamp->region = rg_out->region;
            }
        }
        return do_read;
    }

    bool operator()(const intersecting_geo_read_t &gr) const {
        return rangey_read(gr);
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

    const hash_region_t<key_range_t> *region;
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
    bool operator()(const distribution_read_response_t& x, const distribution_read_response_t& y) {
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
    out->stamps = std::map<uuid_u, uint64_t>();
    for (auto &&resp : resps) {
        // In the error case abort early.
        if (!resp->stamps) {
            out->stamps = boost::none;
            return;
        }
        for (auto &&stamp : *resp->stamps) {
            // Previously conflicts were resolved with `it_out->second =
            // std::max(it->second, it_out->second)`, but I don't think that
            // should ever happen and it isn't correct for
            // `include_initial_vals` changefeeds.
            auto pair = out->stamps->insert(std::make_pair(stamp.first, stamp.second));
            if (!pair.second) {
                out->stamps = boost::none;
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
    unshard_range_batch<rget_read_response_t>(rg, rg.sorting);
}

template<class query_response_t, class query_t>
void rdb_r_unshard_visitor_t::unshard_range_batch(const query_t &q, sorting_t sorting) {
    if (q.transforms.size() != 0 || q.terminal) {
        // This asserts that the optargs have been initialized.  (There is always a
        // 'db' optarg.)  We have the same assertion in rdb_read_visitor_t.
        rassert(q.optargs.size() != 0);
    }
    scoped_ptr_t<profile::trace_t> trace = ql::maybe_make_profile_trace(profile);
    ql::env_t env(ctx, ql::return_empty_normal_batches_t::NO,
                  interruptor, q.optargs, trace.get_or_null());

    // Initialize response.
    response_out->response = query_response_t();
    query_response_t *out = boost::get<query_response_t>(&response_out->response);
    out->truncated = false;
    out->skey_version = ql::skey_version_t::pre_1_16;

    // Fill in `truncated` and `last_key`, get responses, abort if there's an error.
    std::vector<ql::result_t *> results(count);
    std::vector<changefeed_stamp_response_t *> stamp_resps(count);
    store_key_t *best = NULL;
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
            out->skey_version = resp->skey_version;
        } else {
#ifndef NDEBUG
            guarantee(out->skey_version == resp->skey_version);
#else
            if (out->skey_version != resp->skey_version) {
                out->result = ql::exc_t(
                    ql::base_exc_t::INTERNAL,
                    strprintf("Mismatched skey versions %d and %d.",
                              static_cast<int>(out->skey_version),
                              static_cast<int>(resp->skey_version)),
                    ql::backtrace_id_t::empty());
                return;
            }
#endif // NDEBUG
        }
        if (resp->truncated) {
            out->truncated = true;
            if (best == NULL || key_le.is_le(resp->last_key, *best)) {
                best = &resp->last_key;
            }
        }
        results[i] = &resp->result;
        if (q.stamp) {
            guarantee(resp->stamp_response);
            stamp_resps[i] = &*resp->stamp_response;
        }
    }
    out->last_key = (best != NULL) ? std::move(*best) : key_max(sorting);
    if (q.stamp) {
        out->stamp_response = changefeed_stamp_response_t();
        unshard_stamps(stamp_resps, &*out->stamp_response);
    }

    // Unshard and finish up.
    try {
        scoped_ptr_t<ql::accumulator_t> acc(q.terminal
            ? ql::make_terminal(*q.terminal)
            : ql::make_append(sorting, NULL));
        acc->unshard(&env, out->last_key, results);
        acc->finish(&out->result);
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
    bool operator()(const rget_read_t &) const {                  return true;  }
    bool operator()(const intersecting_geo_read_t &) const {      return true;  }
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
    bool operator()(const rget_read_t &rget) const {
        return static_cast<bool>(rget.stamp);
    }
    bool operator()(const point_read_t &) const {                 return false; }
    bool operator()(const dummy_read_t &) const {                 return false; }
    bool operator()(const intersecting_geo_read_t &) const {      return false; }
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
        return s.region;
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
            *payload_out = batched_replace_t(std::move(shard_keys), br.pkey,
                                             br.f.compile_wire_func(), br.optargs,
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
            *payload_out = batched_insert_t(std::move(shard_inserts), bi.pkey,
                                            bi.conflict_behavior, bi.limits,
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
    ql::skey_version_t::pre_1_16, ql::skey_version_t::post_1_16);
RDB_IMPL_SERIALIZABLE_5_FOR_CLUSTER(
    rget_read_response_t, stamp_response, result, skey_version, truncated, last_key);
RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(nearest_geo_read_response_t, results_or_error);
RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(distribution_read_response_t, region, key_counts);
RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(
    changefeed_subscribe_response_t, server_uuids, addrs);
RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(
    changefeed_limit_subscribe_response_t, shards, limit_addrs);
RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(changefeed_stamp_response_t, stamps);

RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(
    changefeed_point_stamp_response_t::valid_response_t, stamp, initial_val);
RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(
    changefeed_point_stamp_response_t, resp);

RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(read_response_t, response, event_log, n_shards);
RDB_IMPL_SERIALIZABLE_0_FOR_CLUSTER(dummy_read_response_t);

RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(point_read_t, key);
RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(dummy_read_t, region);
RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(sindex_rangespec_t, id, region, original_range);

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        sorting_t, int8_t,
        sorting_t::UNORDERED, sorting_t::DESCENDING);
RDB_IMPL_SERIALIZABLE_9_FOR_CLUSTER(rget_read_t,
                                    stamp, region, optargs, table_name, batchspec,
                                    transforms, terminal, sindex, sorting);
RDB_IMPL_SERIALIZABLE_8_FOR_CLUSTER(
        intersecting_geo_read_t, region, optargs, table_name, batchspec, transforms,
        terminal, sindex, query_geometry);
RDB_IMPL_SERIALIZABLE_8_FOR_CLUSTER(
        nearest_geo_read_t, optargs, center, max_dist, max_results, geo_system,
        region, table_name, sindex_id);

RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(
        distribution_read_t, max_depth, result_limit, region);

RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(changefeed_subscribe_t, addr, region);
RDB_IMPL_SERIALIZABLE_5_FOR_CLUSTER(
    changefeed_limit_subscribe_t, addr, uuid, spec, table, region);
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
RDB_IMPL_SERIALIZABLE_5_FOR_CLUSTER(
        batched_insert_t, inserts, pkey, conflict_behavior, limits, return_changes);

RDB_IMPL_SERIALIZABLE_3_SINCE_v1_13(point_write_t, key, data, overwrite);
RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(point_delete_t, key);
RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(sync_t, region);
RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(dummy_write_t, region);

RDB_IMPL_SERIALIZABLE_4_FOR_CLUSTER(
    write_t, write, durability_requirement, profile, limits);


