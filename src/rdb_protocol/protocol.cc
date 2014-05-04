// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/protocol.hpp"

#include <algorithm>
#include <functional>

#include "containers/disk_backed_queue.hpp"
#include "containers/cow_ptr.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/store.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "rpc/semilattice/view.hpp"
#include "rpc/semilattice/watchable.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "clustering/administration/metadata.hpp"

store_key_t key_max(sorting_t sorting) {
    return !reversed(sorting) ? store_key_t::max() : store_key_t::min();
}

//TODO figure out how to do 0 copy serialization with this.

#define RDB_MAKE_PROTOB_SERIALIZABLE_HELPER(pb_t, isinline)             \
    isinline void serialize(write_message_t *wm, const pb_t &p) {      \
        CT_ASSERT(sizeof(int) == sizeof(int32_t));                      \
        int size = p.ByteSize();                                        \
        scoped_array_t<char> data(size);                                \
        p.SerializeToArray(data.data(), size);                          \
        int32_t size32 = size;                                          \
        serialize(wm, size32);                                        \
        wm->append(data.data(), data.size());                          \
    }                                                                   \
                                                                        \
    isinline MUST_USE archive_result_t deserialize(read_stream_t *s, pb_t *p) { \
        CT_ASSERT(sizeof(int) == sizeof(int32_t));                      \
        int32_t size;                                                   \
        archive_result_t res = deserialize(s, &size);                   \
        if (bad(res)) { return res; }                                   \
        if (size < 0) { return archive_result_t::RANGE_ERROR; }         \
        scoped_array_t<char> data(size);                                \
        int64_t read_res = force_read(s, data.data(), data.size());     \
        if (read_res != size) { return archive_result_t::SOCK_ERROR; }  \
        p->ParseFromArray(data.data(), data.size());                    \
        return archive_result_t::SUCCESS;                               \
    }

#define RDB_IMPL_PROTOB_SERIALIZABLE(pb_t) RDB_MAKE_PROTOB_SERIALIZABLE_HELPER(pb_t, )

RDB_IMPL_PROTOB_SERIALIZABLE(Term);
RDB_IMPL_PROTOB_SERIALIZABLE(Datum);
RDB_IMPL_PROTOB_SERIALIZABLE(Backtrace);

datum_range_t::datum_range_t()
    : left_bound_type(key_range_t::none), right_bound_type(key_range_t::none) { }
datum_range_t::datum_range_t(
    counted_t<const ql::datum_t> _left_bound, key_range_t::bound_t _left_bound_type,
    counted_t<const ql::datum_t> _right_bound, key_range_t::bound_t _right_bound_type)
    : left_bound(_left_bound), right_bound(_right_bound),
      left_bound_type(_left_bound_type), right_bound_type(_right_bound_type) { }
datum_range_t::datum_range_t(counted_t<const ql::datum_t> val)
    : left_bound(val), right_bound(val),
      left_bound_type(key_range_t::closed), right_bound_type(key_range_t::closed) { }

datum_range_t datum_range_t::universe()  {
    return datum_range_t(counted_t<const ql::datum_t>(), key_range_t::open,
                         counted_t<const ql::datum_t>(), key_range_t::open);
}
bool datum_range_t::is_universe() const {
    return !left_bound.has() && !right_bound.has()
        && left_bound_type == key_range_t::open && right_bound_type == key_range_t::open;
}

bool datum_range_t::contains(counted_t<const ql::datum_t> val) const {
    return (!left_bound.has()
            || *left_bound < *val
            || (*left_bound == *val && left_bound_type == key_range_t::closed))
        && (!right_bound.has()
            || *right_bound > *val
            || (*right_bound == *val && right_bound_type == key_range_t::closed));
}

key_range_t datum_range_t::to_primary_keyrange() const {
    return key_range_t(
        left_bound_type,
        left_bound.has()
            ? store_key_t(left_bound->print_primary())
            : store_key_t::min(),
        right_bound_type,
        right_bound.has()
            ? store_key_t(right_bound->print_primary())
            : store_key_t::max());
}

key_range_t datum_range_t::to_sindex_keyrange() const {
    return rdb_protocol::sindex_key_range(
        left_bound.has()
            ? store_key_t(left_bound->truncated_secondary())
            : store_key_t::min(),
        right_bound.has()
            ? store_key_t(right_bound->truncated_secondary())
            : store_key_t::max());
}

RDB_IMPL_SERIALIZABLE_3(backfill_atom_t, 0, key, value, recency);

namespace rdb_protocol {

void post_construct_and_drain_queue(
        auto_drainer_t::lock_t lock,
        const std::set<uuid_u> &sindexes_to_bring_up_to_date,
        store_t *store,
        internal_disk_backed_queue_t *mod_queue_ptr)
    THROWS_NOTHING;

/* Creates a queue of operations for the sindex, runs a post construction for
 * the data already in the btree and finally drains the queue. */
void bring_sindexes_up_to_date(
        const std::set<std::string> &sindexes_to_bring_up_to_date,
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

    scoped_ptr_t<internal_disk_backed_queue_t> mod_queue(
            new internal_disk_backed_queue_t(
                store->io_backender_,
                serializer_filepath_t(
                    store->base_path_,
                    "post_construction_" + uuid_to_str(post_construct_id)),
                &store->perfmon_collection));

    {
        scoped_ptr_t<new_mutex_in_line_t> acq =
            store->get_in_line_for_sindex_queue(sindex_block);
        store->register_sindex_queue(mod_queue.get(), acq.get());
    }

    std::map<std::string, secondary_index_t> sindexes;
    get_secondary_indexes(sindex_block, &sindexes);
    std::set<uuid_u> sindexes_to_bring_up_to_date_uuid;

    for (auto it = sindexes_to_bring_up_to_date.begin();
         it != sindexes_to_bring_up_to_date.end(); ++it) {
        auto sindexes_it = sindexes.find(*it);
        guarantee(sindexes_it != sindexes.end());
        sindexes_to_bring_up_to_date_uuid.insert(sindexes_it->second.id);
    }

    coro_t::spawn_sometime(std::bind(
                &post_construct_and_drain_queue,
                store_drainer_acq,
                sindexes_to_bring_up_to_date_uuid,
                store,
                mod_queue.release()));
}

/* Helper for `post_construct_and_drain_queue()`. */
class apply_sindex_change_visitor_t : public boost::static_visitor<> {
public:
    apply_sindex_change_visitor_t(const store_t::sindex_access_vector_t *sindexes,
            txn_t *txn,
            signal_t *interruptor)
        : sindexes_(sindexes), txn_(txn), interruptor_(interruptor) { }
    void operator()(const rdb_modification_report_t &mod_report) const {
        rdb_post_construction_deletion_context_t deletion_context;
        rdb_update_sindexes(*sindexes_, &mod_report, txn_, &deletion_context);
    }

    void operator()(const rdb_erase_major_range_report_t &erase_range_report) const {
        noop_value_deleter_t no_deleter;
        rdb_erase_major_range_sindexes(*sindexes_, &erase_range_report,
                                       interruptor_, &no_deleter);
    }

private:
    const store_t::sindex_access_vector_t *sindexes_;
    txn_t *txn_;
    signal_t *interruptor_;
};

/* This function is really part of the logic of bring_sindexes_up_to_date
 * however it needs to be in a seperate function so that it can be spawned in a
 * coro.
 */
void post_construct_and_drain_queue(
        auto_drainer_t::lock_t lock,
        const std::set<uuid_u> &sindexes_to_bring_up_to_date,
        store_t *store,
        internal_disk_backed_queue_t *mod_queue_ptr)
    THROWS_NOTHING
{
    scoped_ptr_t<internal_disk_backed_queue_t> mod_queue(mod_queue_ptr);

    try {
        post_construct_secondary_indexes(store, sindexes_to_bring_up_to_date, lock.get_drain_signal());

        /* Drain the queue. */

        while (!lock.get_drain_signal()->is_pulsed()) {
            // Yield while we are not holding any locks yet.
            coro_t::yield();

            write_token_pair_t token_pair;
            store->new_write_token_pair(&token_pair);

            scoped_ptr_t<txn_t> queue_txn;
            scoped_ptr_t<real_superblock_t> queue_superblock;

            // We use HARD durability because we want post construction
            // to be throttled if we insert data faster than it can
            // be written to disk. Otherwise we might exhaust the cache's
            // dirty page limit and bring down the whole table.
            // Other than that, the hard durability guarantee is not actually
            // needed here.
            store->acquire_superblock_for_write(
                repli_timestamp_t::distant_past,
                2,
                write_durability_t::HARD,
                &token_pair,
                &queue_txn,
                &queue_superblock,
                lock.get_drain_signal());

            block_id_t sindex_block_id = queue_superblock->get_sindex_block_id();

            buf_lock_t queue_sindex_block
                = store->acquire_sindex_block_for_write(queue_superblock->expose_buf(),
                                                        sindex_block_id);

            queue_superblock->release();

            store_t::sindex_access_vector_t sindexes;
            store->acquire_sindex_superblocks_for_write(
                    sindexes_to_bring_up_to_date,
                    &queue_sindex_block,
                    &sindexes);

            if (sindexes.empty()) {
                break;
            }

            scoped_ptr_t<new_mutex_in_line_t> acq =
                store->get_in_line_for_sindex_queue(&queue_sindex_block);
            // TODO (daniel): Is there a way to release the queue_sindex_block
            // earlier than we do now, ideally before we wait for the acq signal?
            acq->acq_signal()->wait_lazily_unordered();

            const int MAX_CHUNK_SIZE = 10;
            int current_chunk_size = 0;
            while (current_chunk_size < MAX_CHUNK_SIZE && mod_queue->size() > 0) {
                rdb_sindex_change_t sindex_change;
                deserializing_viewer_t<rdb_sindex_change_t> viewer(&sindex_change);
                mod_queue->pop(&viewer);
                boost::apply_visitor(apply_sindex_change_visitor_t(
                                        &sindexes,
                                        queue_txn.get(),
                                        lock.get_drain_signal()),
                                     sindex_change);
                ++current_chunk_size;
            }

            if (mod_queue->size() == 0) {
                for (auto it = sindexes_to_bring_up_to_date.begin();
                     it != sindexes_to_bring_up_to_date.end(); ++it) {
                    store->mark_index_up_to_date(*it, &queue_sindex_block);
                }
                store->deregister_sindex_queue(mod_queue.get(), acq.get());
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
        write_token_pair_t token_pair;
        store->new_write_token_pair(&token_pair);

        scoped_ptr_t<txn_t> queue_txn;
        scoped_ptr_t<real_superblock_t> queue_superblock;

        store->acquire_superblock_for_write(
            repli_timestamp_t::distant_past,
            2,
            write_durability_t::HARD,
            &token_pair,
            &queue_txn,
            &queue_superblock,
            lock.get_drain_signal());

        block_id_t sindex_block_id = queue_superblock->get_sindex_block_id();

        buf_lock_t queue_sindex_block
            = store->acquire_sindex_block_for_write(queue_superblock->expose_buf(),
                                                    sindex_block_id);

        queue_superblock->release();

        scoped_ptr_t<new_mutex_in_line_t> acq =
                store->get_in_line_for_sindex_queue(&queue_sindex_block);
        store->deregister_sindex_queue(mod_queue.get(), acq.get());
    }
}


bool range_key_tester_t::key_should_be_erased(const btree_key_t *key) {
    uint64_t h = hash_region_hasher(key->contents, key->size);
    return delete_range->beg <= h && h < delete_range->end
        && delete_range->inner.contains_key(key->contents, key->size);
}

typedef boost::variant<rdb_modification_report_t,
                       rdb_erase_major_range_report_t>
        sindex_change_t;

void add_status(const single_sindex_status_t &new_status,
    single_sindex_status_t *status_out) {
    status_out->blocks_processed += new_status.blocks_processed;
    status_out->blocks_total += new_status.blocks_total;
    status_out->ready &= new_status.ready;
}

}  // namespace rdb_protocol

rdb_context_t::rdb_context_t()
    : extproc_pool(NULL), ns_repo(NULL),
    cross_thread_namespace_watchables(get_num_threads()),
    cross_thread_database_watchables(get_num_threads()),
    directory_read_manager(NULL),
    signals(get_num_threads()),
    ql_stats_membership(&get_global_perfmon_collection(), &ql_stats_collection, "query_language"),
    ql_ops_running_membership(&ql_stats_collection, &ql_ops_running, "ops_running")
{ }

rdb_context_t::rdb_context_t(
    extproc_pool_t *_extproc_pool,
    namespace_repo_t *_ns_repo,
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
        _cluster_metadata,
    boost::shared_ptr<semilattice_readwrite_view_t<auth_semilattice_metadata_t> >
        _auth_metadata,
    directory_read_manager_t<cluster_directory_metadata_t>
        *_directory_read_manager,
    machine_id_t _machine_id,
    perfmon_collection_t *global_stats)
    : extproc_pool(_extproc_pool), ns_repo(_ns_repo),
      cross_thread_namespace_watchables(get_num_threads()),
      cross_thread_database_watchables(get_num_threads()),
      cluster_metadata(_cluster_metadata),
      auth_metadata(_auth_metadata),
      directory_read_manager(_directory_read_manager),
      signals(get_num_threads()),
      machine_id(_machine_id),
      ql_stats_membership(global_stats, &ql_stats_collection, "query_language"),
      ql_ops_running_membership(&ql_stats_collection, &ql_ops_running, "ops_running")
{
    for (int thread = 0; thread < get_num_threads(); ++thread) {
        cross_thread_namespace_watchables[thread].init(new cross_thread_watchable_variable_t<cow_ptr_t<namespaces_semilattice_metadata_t> >(
                                                    clone_ptr_t<semilattice_watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t> > >
                                                        (new semilattice_watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t> >(
                                                            metadata_field(&cluster_semilattice_metadata_t::rdb_namespaces, _cluster_metadata))), threadnum_t(thread)));

        cross_thread_database_watchables[thread].init(new cross_thread_watchable_variable_t<databases_semilattice_metadata_t>(
                                                    clone_ptr_t<semilattice_watchable_t<databases_semilattice_metadata_t> >
                                                        (new semilattice_watchable_t<databases_semilattice_metadata_t>(
                                                            metadata_field(&cluster_semilattice_metadata_t::databases, _cluster_metadata))), threadnum_t(thread)));

        signals[thread].init(new cross_thread_signal_t(&interruptor, threadnum_t(thread)));
    }
}

rdb_context_t::~rdb_context_t() { }

namespace rdb_protocol {
// Construct a region containing only the specified key
region_t monokey_region(const store_key_t &k) {
    uint64_t h = hash_region_hasher(k.contents(), k.size());
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

region_t cpu_sharding_subspace(int subregion_number,
                               int num_cpu_shards) {
    guarantee(subregion_number >= 0);
    guarantee(subregion_number < num_cpu_shards);

    // We have to be careful with the math here, to avoid overflow.
    uint64_t width = HASH_REGION_HASH_SIZE / num_cpu_shards;

    uint64_t beg = width * subregion_number;
    uint64_t end = subregion_number + 1 == num_cpu_shards
        ? HASH_REGION_HASH_SIZE : beg + width;

    return region_t(beg, end, key_range_t::universe());
}

}  // namespace rdb_protocol

// Returns the key identifying the monokey region used for sindex_list_t
// operations.
store_key_t sindex_list_region_key() {
    return store_key_t();
}

/* read_t::get_region implementation */
struct rdb_r_get_region_visitor : public boost::static_visitor<region_t> {
    region_t operator()(const point_read_t &pr) const {
        return rdb_protocol::monokey_region(pr.key);
    }

    region_t operator()(const rget_read_t &rg) const {
        return rg.region;
    }

    region_t operator()(const distribution_read_t &dg) const {
        return dg.region;
    }

    region_t operator()(UNUSED const sindex_list_t &sl) const {
        return rdb_protocol::monokey_region(sindex_list_region_key());
    }

    region_t operator()(const sindex_status_t &ss) const {
        return ss.region;
    }
};

region_t read_t::get_region() const THROWS_NOTHING {
    return boost::apply_visitor(rdb_r_get_region_visitor(), read);
}

struct rdb_r_shard_visitor_t : public boost::static_visitor<bool> {
    explicit rdb_r_shard_visitor_t(const hash_region_t<key_range_t> *_region,
                                   profile_bool_t _profile, read_t *_read_out)
        : region(_region), profile(_profile), read_out(_read_out) { }

    // The key was somehow already extracted from the arg.
    template <class T>
    bool keyed_read(const T &arg, const store_key_t &key) const {
        if (region_contains_key(*region, key)) {
            *read_out = read_t(arg, profile);
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
            *read_out = read_t(tmp, profile);
            return true;
        } else {
            return false;
        }
    }

    bool operator()(const rget_read_t &rg) const {
        bool do_read = rangey_read(rg);
        if (do_read) {
            auto rg_out = boost::get<rget_read_t>(&read_out->read);
            rg_out->batchspec = rg_out->batchspec.scale_down(CPU_SHARDING_FACTOR);
        }
        return do_read;
    }

    bool operator()(const distribution_read_t &dg) const {
        return rangey_read(dg);
    }

    bool operator()(const sindex_list_t &sl) const {
        return keyed_read(sl, sindex_list_region_key());
    }

    bool operator()(const sindex_status_t &ss) const {
        return rangey_read(ss);
    }

    const hash_region_t<key_range_t> *region;
    profile_bool_t profile;
    read_t *read_out;
};

bool read_t::shard(const hash_region_t<key_range_t> &region,
                   read_t *read_out) const THROWS_NOTHING {
    return boost::apply_visitor(rdb_r_shard_visitor_t(&region, profile, read_out), read);
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
    rdb_r_unshard_visitor_t(read_response_t *_responses,
                            size_t _count,
                            read_response_t *_response_out,
                            rdb_context_t *ctx,
                            signal_t *interruptor)
        : responses(_responses), count(_count), response_out(_response_out),
          env(ctx, interruptor) { }

    void operator()(const point_read_t &);

    void operator()(const rget_read_t &rg);
    void operator()(const distribution_read_t &rg);
    void operator()(const sindex_list_t &rg);
    void operator()(const sindex_status_t &rg);

private:
    read_response_t *responses; // Cannibalized for efficiency.
    size_t count;
    read_response_t *response_out;
    ql::env_t env;
};

void rdb_r_unshard_visitor_t::operator()(const point_read_t &) {
    guarantee(count == 1);
    guarantee(NULL != boost::get<point_read_response_t>(&responses[0].response));
    *response_out = responses[0];
}

void rdb_r_unshard_visitor_t::operator()(const rget_read_t &rg) {
    // Initialize response.
    response_out->response = rget_read_response_t();
    auto out = boost::get<rget_read_response_t>(&response_out->response);
    out->truncated = false;
    out->key_range = read_t(rg, profile_bool_t::DONT_PROFILE).get_region().inner;

    // Fill in `truncated` and `last_key`, get responses, abort if there's an error.
    std::vector<ql::result_t *> results(count);
    store_key_t *best = NULL;
    key_le_t key_le(rg.sorting);
    for (size_t i = 0; i < count; ++i) {
        auto resp = boost::get<rget_read_response_t>(&responses[i].response);
        guarantee(resp);
        if (resp->truncated) {
            out->truncated = true;
            if (best == NULL || key_le(resp->last_key, *best)) {
                best = &resp->last_key;
            }
        }
        if (boost::get<ql::exc_t>(&resp->result) != NULL) {
            out->result = std::move(resp->result);
            return;
        }
        results[i] = &resp->result;
    }
    out->last_key = (best != NULL) ? std::move(*best) : key_max(rg.sorting);

    // Unshard and finish up.
    scoped_ptr_t<ql::accumulator_t> acc(rg.terminal
        ? ql::make_terminal(&env, *rg.terminal)
        : ql::make_append(rg.sorting, NULL));
    acc->unshard(out->last_key, results);
    acc->finish(&out->result);
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

void rdb_r_unshard_visitor_t::operator()(UNUSED const sindex_list_t &sl) {
    guarantee(count == 1);
    guarantee(boost::get<sindex_list_response_t>(&responses[0].response));
    *response_out = responses[0];
}

void rdb_r_unshard_visitor_t::operator()(UNUSED const sindex_status_t &ss) {
    *response_out = read_response_t(sindex_status_response_t());
    auto ss_response = boost::get<sindex_status_response_t>(&response_out->response);
    for (size_t i = 0; i < count; ++i) {
        auto resp = boost::get<sindex_status_response_t>(&responses[i].response);
        guarantee(resp != NULL);
        for (auto it = resp->statuses.begin(); it != resp->statuses.end(); ++it) {
            add_status(it->second, &ss_response->statuses[it->first]);
        }
    }
}

void read_t::unshard(read_response_t *responses, size_t count,
                     read_response_t *response_out, rdb_context_t *ctx,
                     signal_t *interruptor) const
    THROWS_ONLY(interrupted_exc_t) {
    rdb_r_unshard_visitor_t v(responses, count, response_out, ctx, interruptor);
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

        const uint64_t hash_value = hash_region_hasher(key.contents(), key.size());
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
            keys.emplace_back((*it)->get(bi.pkey)->print_primary());
        }
        return region_from_keys(keys);
    }

    region_t operator()(const point_write_t &pw) const {
        return rdb_protocol::monokey_region(pw.key);
    }

    region_t operator()(const point_delete_t &pd) const {
        return rdb_protocol::monokey_region(pd.key);
    }

    region_t operator()(const sindex_create_t &s) const {
        return s.region;
    }

    region_t operator()(const sindex_drop_t &d) const {
        return d.region;
    }

    region_t operator()(const sync_t &s) const {
        return s.region;
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
    rdb_w_shard_visitor_t(const region_t *_region,
                          durability_requirement_t _durability_requirement,
                          profile_bool_t _profile, write_t *_write_out)
        : region(_region),
          durability_requirement(_durability_requirement),
          profile(_profile), write_out(_write_out) {}

    template <class T>
    bool keyed_write(const T &arg) const {
        if (region_contains_key(*region, arg.key)) {
            *write_out = write_t(arg, durability_requirement, profile);
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
            *write_out = write_t(
                batched_replace_t(
                    std::move(shard_keys),
                    br.pkey,
                    br.f.compile_wire_func(),
                    br.optargs,
                    br.return_vals),
                durability_requirement,
                profile);
            return true;
        } else {
            return false;
        }
    }

    bool operator()(const batched_insert_t &bi) const {
        std::vector<counted_t<const ql::datum_t> > shard_inserts;
        for (auto it = bi.inserts.begin(); it != bi.inserts.end(); ++it) {
            store_key_t key((*it)->get(bi.pkey)->print_primary());
            if (region_contains_key(*region, key)) {
                shard_inserts.push_back(*it);
            }
        }
        if (!shard_inserts.empty()) {
            *write_out = write_t(
                batched_insert_t(
                    std::move(shard_inserts), bi.pkey, bi.upsert, bi.return_vals),
                durability_requirement,
                profile);
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
            *write_out = write_t(tmp, durability_requirement, profile);
            return true;
        } else {
            return false;
        }
    }

    bool operator()(const sindex_create_t &c) const {
        return rangey_write(c);
    }

    bool operator()(const sindex_drop_t &d) const {
        return rangey_write(d);
    }

    bool operator()(const sync_t &s) const {
        return rangey_write(s);
    }

    const region_t *region;
    durability_requirement_t durability_requirement;
    profile_bool_t profile;
    write_t *write_out;
};

bool write_t::shard(const region_t &region,
                    write_t *write_out) const THROWS_NOTHING {
    const rdb_w_shard_visitor_t v(&region, durability_requirement, profile, write_out);
    return boost::apply_visitor(v, write);
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
        counted_t<const ql::datum_t> stats(new ql::datum_t(ql::datum_t::R_OBJECT));
        for (size_t i = 0; i < count; ++i) {
            const counted_t<const ql::datum_t> *stats_i =
                boost::get<counted_t<const ql::datum_t> >(&responses[i].response);
            guarantee(stats_i != NULL);
            stats = stats->merge(*stats_i, ql::stats_merge);
        }
        *response_out = write_response_t(stats);
    }
    void operator()(const batched_replace_t &) const {
        merge_stats();
    }

    void operator()(const batched_insert_t &) const {
        merge_stats();
    }

    void operator()(const point_write_t &) const { monokey_response(); }
    void operator()(const point_delete_t &) const { monokey_response(); }

    void operator()(const sindex_create_t &) const {
        *response_out = responses[0];
    }

    void operator()(const sindex_drop_t &) const {
        *response_out = responses[0];
    }

    void operator()(const sync_t &) const {
        *response_out = responses[0];
    }

    rdb_w_unshard_visitor_t(const write_response_t *_responses, size_t _count,
                            write_response_t *_response_out)
        : responses(_responses), count(_count), response_out(_response_out) { }

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
};

void write_t::unshard(write_response_t *responses, size_t count,
                      write_response_t *response_out, rdb_context_t *, signal_t *)
    const THROWS_NOTHING {
    const rdb_w_unshard_visitor_t visitor(responses, count, response_out);
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


RDB_IMPL_ME_SERIALIZABLE_3(rdb_protocol::single_sindex_status_t, 0,
                           blocks_total, blocks_processed, ready);

RDB_IMPL_ME_SERIALIZABLE_1(point_read_response_t, 0, data);
RDB_IMPL_ME_SERIALIZABLE_4(rget_read_response_t, 0,
                           result, key_range, truncated, last_key);
RDB_IMPL_ME_SERIALIZABLE_2(distribution_read_response_t, 0,
                           region, key_counts);
RDB_IMPL_ME_SERIALIZABLE_1(sindex_list_response_t, 0, sindexes);
RDB_IMPL_ME_SERIALIZABLE_3(read_response_t, 0,
                           response, event_log, n_shards);
RDB_IMPL_ME_SERIALIZABLE_1(sindex_status_response_t, 0, statuses);

RDB_IMPL_ME_SERIALIZABLE_1(point_read_t, 0, key);
RDB_IMPL_ME_SERIALIZABLE_3(sindex_rangespec_t, 0,
                           id, region, original_range);

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(key_range_t::bound_t, int8_t,
                                      key_range_t::open, key_range_t::none);
RDB_IMPL_ME_SERIALIZABLE_4(datum_range_t, 0,
                           empty_ok(left_bound), empty_ok(right_bound),
                           left_bound_type, right_bound_type);
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        sorting_t, int8_t,
        sorting_t::UNORDERED, sorting_t::DESCENDING);
RDB_IMPL_ME_SERIALIZABLE_8(rget_read_t, 0, region, optargs, table_name, batchspec,
                           transforms, terminal, sindex, sorting);

RDB_IMPL_ME_SERIALIZABLE_3(distribution_read_t, 0,
                           max_depth, result_limit, region);
RDB_IMPL_ME_SERIALIZABLE_0(sindex_list_t, 0);
RDB_IMPL_ME_SERIALIZABLE_2(sindex_status_t, 0, sindexes, region);
RDB_IMPL_ME_SERIALIZABLE_2(read_t, 0, read, profile);
RDB_IMPL_ME_SERIALIZABLE_1(point_write_response_t, 0, result);

RDB_IMPL_ME_SERIALIZABLE_1(point_delete_response_t, 0, result);
RDB_IMPL_ME_SERIALIZABLE_1(sindex_create_response_t, 0, success);
RDB_IMPL_ME_SERIALIZABLE_1(sindex_drop_response_t, 0, success);
RDB_IMPL_ME_SERIALIZABLE_0(sync_response_t, 0);

RDB_IMPL_ME_SERIALIZABLE_3(write_response_t, 0, response, event_log, n_shards);

RDB_IMPL_ME_SERIALIZABLE_5(batched_replace_t, 0,
                           keys, pkey, f, optargs, return_vals);
RDB_IMPL_ME_SERIALIZABLE_4(batched_insert_t, 0,
                           inserts, pkey, upsert, return_vals);

RDB_IMPL_ME_SERIALIZABLE_3(point_write_t, 0, key, data, overwrite);
RDB_IMPL_ME_SERIALIZABLE_1(point_delete_t, 0, key);

RDB_IMPL_ME_SERIALIZABLE_4(sindex_create_t, 0, id, mapping, region, multi);
RDB_IMPL_ME_SERIALIZABLE_2(sindex_drop_t, 0, id, region);
RDB_IMPL_ME_SERIALIZABLE_1(sync_t, 0, region);

RDB_IMPL_ME_SERIALIZABLE_3(write_t, 0,
                           write, durability_requirement, profile);
RDB_IMPL_ME_SERIALIZABLE_1(backfill_chunk_t::delete_key_t, 0, key);

RDB_IMPL_ME_SERIALIZABLE_1(backfill_chunk_t::delete_range_t, 0, range);

RDB_IMPL_ME_SERIALIZABLE_1(backfill_chunk_t::key_value_pairs_t, 0,
                           backfill_atoms);

RDB_IMPL_ME_SERIALIZABLE_1(backfill_chunk_t::sindexes_t, 0, sindexes);

RDB_IMPL_ME_SERIALIZABLE_1(backfill_chunk_t, 0, val);
