// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/protocol.hpp"

#include <algorithm>
#include <functional>

#include "arch/io/disk.hpp"
#include "btree/erase_range.hpp"
#include "btree/parallel_traversal.hpp"
#include "btree/slice.hpp"
#include "btree/superblock.hpp"
#include "clustering/administration/metadata.hpp"
#include "clustering/reactor/reactor.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/promise.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/disk_backed_queue.hpp"
#include "containers/scoped.hpp"
#include "protob/protob.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/shards.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/term_walker.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "rpc/semilattice/watchable.hpp"
#include "serializer/config.hpp"
#include "stl_utils.hpp"

typedef rdb_protocol_details::backfill_atom_t rdb_backfill_atom_t;
typedef rdb_protocol_details::range_key_tester_t range_key_tester_t;

typedef rdb_protocol_t::context_t context_t;

typedef rdb_protocol_t::store_t store_t;
typedef rdb_protocol_t::region_t region_t;

typedef rdb_protocol_t::read_t read_t;
typedef rdb_protocol_t::read_response_t read_response_t;

typedef rdb_protocol_t::point_read_t point_read_t;
typedef rdb_protocol_t::point_read_response_t point_read_response_t;

typedef rdb_protocol_t::rget_read_t rget_read_t;
typedef rdb_protocol_t::rget_read_response_t rget_read_response_t;

typedef rdb_protocol_t::distribution_read_t distribution_read_t;
typedef rdb_protocol_t::distribution_read_response_t distribution_read_response_t;

typedef rdb_protocol_t::sindex_list_t sindex_list_t;
typedef rdb_protocol_t::sindex_list_response_t sindex_list_response_t;

typedef rdb_protocol_t::sindex_status_t sindex_status_t;
typedef rdb_protocol_t::sindex_status_response_t sindex_status_response_t;

typedef rdb_protocol_t::write_t write_t;
typedef rdb_protocol_t::write_response_t write_response_t;

typedef rdb_protocol_t::batched_replace_t batched_replace_t;
typedef rdb_protocol_t::batched_insert_t batched_insert_t;

typedef rdb_protocol_t::point_write_t point_write_t;
typedef rdb_protocol_t::point_write_response_t point_write_response_t;

typedef rdb_protocol_t::point_delete_t point_delete_t;
typedef rdb_protocol_t::point_delete_response_t point_delete_response_t;

typedef rdb_protocol_t::sindex_create_t sindex_create_t;
typedef rdb_protocol_t::sindex_create_response_t sindex_create_response_t;

typedef rdb_protocol_t::sindex_drop_t sindex_drop_t;
typedef rdb_protocol_t::sindex_drop_response_t sindex_drop_response_t;

typedef rdb_protocol_t::sync_t sync_t;
typedef rdb_protocol_t::sync_response_t sync_response_t;

typedef rdb_protocol_t::backfill_chunk_t backfill_chunk_t;

typedef rdb_protocol_t::backfill_progress_t backfill_progress_t;

typedef btree_store_t<rdb_protocol_t>::sindex_access_vector_t sindex_access_vector_t;

const std::string rdb_protocol_t::protocol_name("rdb");

store_key_t key_max(sorting_t sorting) {
    return !reversed(sorting) ? store_key_t::max() : store_key_t::min();
}

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
    return rdb_protocol_t::sindex_key_range(
        left_bound.has()
            ? store_key_t(left_bound->truncated_secondary())
            : store_key_t::min(),
        right_bound.has()
            ? store_key_t(right_bound->truncated_secondary())
            : store_key_t::max());
}

namespace rdb_protocol_details {

RDB_IMPL_SERIALIZABLE_3(backfill_atom_t, key, value, recency);

void post_construct_and_drain_queue(
        auto_drainer_t::lock_t lock,
        const std::set<uuid_u> &sindexes_to_bring_up_to_date,
        btree_store_t<rdb_protocol_t> *store,
        internal_disk_backed_queue_t *mod_queue_ptr)
    THROWS_NOTHING;

/* Creates a queue of operations for the sindex, runs a post construction for
 * the data already in the btree and finally drains the queue. */
void bring_sindexes_up_to_date(
        const std::set<std::string> &sindexes_to_bring_up_to_date,
        btree_store_t<rdb_protocol_t> *store,
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
        guarantee(std_contains(sindexes, *it));
        sindexes_to_bring_up_to_date_uuid.insert(sindexes[*it].id);
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
    apply_sindex_change_visitor_t(const sindex_access_vector_t *sindexes,
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
    const sindex_access_vector_t *sindexes_;
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
        btree_store_t<rdb_protocol_t> *store,
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

            sindex_access_vector_t sindexes;
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

}  // namespace rdb_protocol_details

rdb_protocol_t::context_t::context_t()
    : extproc_pool(NULL), ns_repo(NULL),
    cross_thread_namespace_watchables(get_num_threads()),
    cross_thread_database_watchables(get_num_threads()),
    directory_read_manager(NULL),
    signals(get_num_threads()),
    ql_stats_membership(&get_global_perfmon_collection(), &ql_stats_collection, "query_language"),
    ql_ops_running_membership(&ql_stats_collection, &ql_ops_running, "ops_running")
{ }

rdb_protocol_t::context_t::context_t(
    extproc_pool_t *_extproc_pool,
    namespace_repo_t<rdb_protocol_t> *_ns_repo,
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
        cross_thread_namespace_watchables[thread].init(new cross_thread_watchable_variable_t<cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > >(
                                                    clone_ptr_t<semilattice_watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > > >
                                                        (new semilattice_watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > >(
                                                            metadata_field(&cluster_semilattice_metadata_t::rdb_namespaces, _cluster_metadata))), threadnum_t(thread)));

        cross_thread_database_watchables[thread].init(new cross_thread_watchable_variable_t<databases_semilattice_metadata_t>(
                                                    clone_ptr_t<semilattice_watchable_t<databases_semilattice_metadata_t> >
                                                        (new semilattice_watchable_t<databases_semilattice_metadata_t>(
                                                            metadata_field(&cluster_semilattice_metadata_t::databases, _cluster_metadata))), threadnum_t(thread)));

        signals[thread].init(new cross_thread_signal_t(&interruptor, threadnum_t(thread)));
    }
}

rdb_protocol_t::context_t::~context_t() { }

// Construct a region containing only the specified key
region_t rdb_protocol_t::monokey_region(const store_key_t &k) {
    uint64_t h = hash_region_hasher(k.contents(), k.size());
    return region_t(h, h + 1, key_range_t(key_range_t::closed, k, key_range_t::closed, k));
}

key_range_t rdb_protocol_t::sindex_key_range(const store_key_t &start,
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

// Returns the key identifying the monokey region used for sindex_list_t
// operations.
store_key_t sindex_list_region_key() {
    return store_key_t();
}

/* read_t::get_region implementation */
struct rdb_r_get_region_visitor : public boost::static_visitor<region_t> {
    region_t operator()(const point_read_t &pr) const {
        return rdb_protocol_t::monokey_region(pr.key);
    }

    region_t operator()(const rget_read_t &rg) const {
        return rg.region;
    }

    region_t operator()(const distribution_read_t &dg) const {
        return dg.region;
    }

    region_t operator()(UNUSED const sindex_list_t &sl) const {
        return rdb_protocol_t::monokey_region(sindex_list_region_key());
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
                            rdb_protocol_t::context_t *ctx,
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
        auto resp = boost::get<sindex_status_response_t>(&responses[0].response);
        guarantee(resp != NULL);
        for (auto it = resp->statuses.begin(); it != resp->statuses.end(); ++it) {
            add_status(it->second, &ss_response->statuses[it->first]);
        }
    }
}

void read_t::unshard(read_response_t *responses, size_t count,
                     read_response_t *response_out, context_t *ctx,
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
        return rdb_protocol_t::monokey_region(pw.key);
    }

    region_t operator()(const point_delete_t &pd) const {
        return rdb_protocol_t::monokey_region(pd.key);
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
                      write_response_t *response_out, context_t *, signal_t *)
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

store_t::store_t(serializer_t *serializer,
                 cache_balancer_t *balancer,
                 const std::string &perfmon_name,
                 bool create,
                 perfmon_collection_t *parent_perfmon_collection,
                 context_t *_ctx,
                 io_backender_t *io,
                 const base_path_t &base_path) :
    btree_store_t<rdb_protocol_t>(serializer, balancer, perfmon_name,
            create, parent_perfmon_collection, _ctx, io, base_path),
    ctx(_ctx)
{
    // Make sure to continue bringing sindexes up-to-date if it was interrupted earlier

    // This uses a dummy interruptor because this is the only thing using the store at
    //  the moment (since we are still in the constructor), so things should complete
    //  rather quickly.
    cond_t dummy_interruptor;
    read_token_pair_t token_pair;
    new_read_token_pair(&token_pair);

    scoped_ptr_t<txn_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_read(&token_pair.main_read_token, &txn,
                                &superblock, &dummy_interruptor, false);

    buf_lock_t sindex_block
        = acquire_sindex_block_for_read(superblock->expose_buf(),
                                        superblock->get_sindex_block_id());

    superblock.reset();

    std::map<std::string, secondary_index_t> sindexes;
    get_secondary_indexes(&sindex_block, &sindexes);

    std::set<std::string> sindexes_to_update;
    for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
        if (!it->second.post_construction_complete) {
            sindexes_to_update.insert(it->first);
        }
    }

    if (!sindexes_to_update.empty()) {
        rdb_protocol_details::bring_sindexes_up_to_date(sindexes_to_update, this,
                                                        &sindex_block);
    }
}

store_t::~store_t() {
    assert_thread();
}

// TODO: get rid of this extra response_t copy on the stack
struct rdb_read_visitor_t : public boost::static_visitor<void> {
    void operator()(const point_read_t &get) {
        response->response = point_read_response_t();
        point_read_response_t *res =
            boost::get<point_read_response_t>(&response->response);
        rdb_get(get.key, btree, superblock, res, ql_env.trace.get_or_null());
    }

    void operator()(const rget_read_t &rget) {
        if (rget.transforms.size() != 0 || rget.terminal) {
            rassert(rget.optargs.size() != 0);
        }
        ql_env.global_optargs.init_optargs(rget.optargs);
        response->response = rget_read_response_t();
        rget_read_response_t *res =
            boost::get<rget_read_response_t>(&response->response);

        if (!rget.sindex) {
            // Normal rget
            rdb_rget_slice(btree, rget.region.inner, superblock,
                           &ql_env, rget.batchspec, rget.transforms, rget.terminal,
                           rget.sorting, res);
        } else {
            scoped_ptr_t<real_superblock_t> sindex_sb;
            std::vector<char> sindex_mapping_data;

            try {
                bool found = store->acquire_sindex_superblock_for_read(
                    rget.sindex->id, superblock, &sindex_sb, &sindex_mapping_data);
                if (!found) {
                    res->result = ql::exc_t(
                        ql::base_exc_t::GENERIC,
                        strprintf("Index `%s` was not found.", rget.sindex->id.c_str()),
                        NULL);
                    return;
                }
            } catch (const sindex_not_post_constructed_exc_t &) {
                res->result = ql::exc_t(
                    ql::base_exc_t::GENERIC,
                    strprintf("Index `%s` was accessed before its construction "
                              "was finished.", rget.sindex->id.c_str()),
                    NULL);
                return;
            }

            // This chunk of code puts together a filter so we can exclude any items
            //  that don't fall in the specified range.  Because the secondary index
            //  keys may have been truncated, we can't go by keys alone.  Therefore,
            //  we construct a filter function that ensures all returned items lie
            //  between sindex_start_value and sindex_end_value.
            ql::map_wire_func_t sindex_mapping;
            sindex_multi_bool_t multi_bool = sindex_multi_bool_t::MULTI;
            inplace_vector_read_stream_t read_stream(&sindex_mapping_data);
            archive_result_t success = deserialize(&read_stream, &sindex_mapping);
            guarantee_deserialization(success, "sindex description");
            success = deserialize(&read_stream, &multi_bool);
            guarantee_deserialization(success, "sindex description");

            rdb_rget_secondary_slice(
                store->get_sindex_slice(rget.sindex->id),
                rget.sindex->original_range, rget.sindex->region,
                sindex_sb.get(), &ql_env, rget.batchspec, rget.transforms,
                rget.terminal, rget.region.inner, rget.sorting,
                sindex_mapping, multi_bool, res);
        }
    }

    void operator()(const distribution_read_t &dg) {
        response->response = distribution_read_response_t();
        distribution_read_response_t *res = boost::get<distribution_read_response_t>(&response->response);
        rdb_distribution_get(dg.max_depth, dg.region.inner.left,
                             superblock, res);
        for (std::map<store_key_t, int64_t>::iterator it = res->key_counts.begin(); it != res->key_counts.end(); ) {
            if (!dg.region.inner.contains_key(store_key_t(it->first))) {
                std::map<store_key_t, int64_t>::iterator tmp = it;
                ++it;
                res->key_counts.erase(tmp);
            } else {
                ++it;
            }
        }

        // If the result is larger than the requested limit, scale it down
        if (dg.result_limit > 0 && res->key_counts.size() > dg.result_limit) {
            scale_down_distribution(dg.result_limit, &res->key_counts);
        }

        res->region = dg.region;
    }

    void operator()(UNUSED const sindex_list_t &sinner) {
        response->response = sindex_list_response_t();
        sindex_list_response_t *res = &boost::get<sindex_list_response_t>(response->response);

        buf_lock_t sindex_block
            = store->acquire_sindex_block_for_read(superblock->expose_buf(),
                                                   superblock->get_sindex_block_id());
        superblock->release();

        std::map<std::string, secondary_index_t> sindexes;
        get_secondary_indexes(&sindex_block, &sindexes);
        sindex_block.reset_buf_lock();

        res->sindexes.reserve(sindexes.size());
        for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
            res->sindexes.push_back(it->first);
        }
    }

    void operator()(const sindex_status_t &sindex_status) {
        response->response = sindex_status_response_t();
        auto res = &boost::get<sindex_status_response_t>(response->response);

        buf_lock_t sindex_block
            = store->acquire_sindex_block_for_read(superblock->expose_buf(),
                                                   superblock->get_sindex_block_id());
        superblock->release();

        std::map<std::string, secondary_index_t> sindexes;
        get_secondary_indexes(&sindex_block, &sindexes);
        sindex_block.reset_buf_lock();

        for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
            if (std_contains(sindex_status.sindexes, it->first) ||
                sindex_status.sindexes.empty()) {
                progress_completion_fraction_t frac =
                    store->get_progress(it->second.id);
                rdb_protocol_details::single_sindex_status_t *s =
                    &res->statuses[it->first];
                s->ready = it->second.post_construction_complete;
                if (!s->ready) {
                    if (frac.estimate_of_total_nodes == -1) {
                        s->blocks_processed = 0;
                        s->blocks_total = 0;
                    } else {
                        s->blocks_processed = frac.estimate_of_released_nodes;
                        s->blocks_total = frac.estimate_of_total_nodes;
                    }
                }
            }
        }
    }

    rdb_read_visitor_t(btree_slice_t *_btree,
                       btree_store_t<rdb_protocol_t> *_store,
                       superblock_t *_superblock,
                       rdb_protocol_t::context_t *ctx,
                       read_response_t *_response,
                       profile_bool_t profile,
                       signal_t *_interruptor) :
        response(_response),
        btree(_btree),
        store(_store),
        superblock(_superblock),
        interruptor(_interruptor, ctx->signals[get_thread_id().threadnum].get()),
        ql_env(ctx->extproc_pool,
               ctx->ns_repo,
               ctx->cross_thread_namespace_watchables[get_thread_id().threadnum].get()
                   ->get_watchable(),
               ctx->cross_thread_database_watchables[get_thread_id().threadnum].get()
                   ->get_watchable(),
               ctx->cluster_metadata,
               NULL,
               &interruptor,
               ctx->machine_id,
               profile)
    { }

    ql::env_t *get_env() {
        return &ql_env;
    }

    profile::event_log_t extract_event_log() {
        if (ql_env.trace.has()) {
            return std::move(*ql_env.trace).extract_event_log();
        } else {
            return profile::event_log_t();
        }
    }

private:
    read_response_t *response;
    btree_slice_t *btree;
    btree_store_t<rdb_protocol_t> *store;
    superblock_t *superblock;
    wait_any_t interruptor;
    ql::env_t ql_env;

    DISABLE_COPYING(rdb_read_visitor_t);
};

void store_t::protocol_read(const read_t &read,
                            read_response_t *response,
                            btree_slice_t *btree,
                            superblock_t *superblock,
                            signal_t *interruptor) {
    rdb_read_visitor_t v(
        btree, this,
        superblock,
        ctx, response, read.profile, interruptor);
    {
        profile::starter_t start_write("Perform read on shard.", v.get_env()->trace);
        boost::apply_visitor(v, read.read);
    }

    response->n_shards = 1;
    response->event_log = v.extract_event_log();
    //This is a tad hacky, this just adds a stop event to signal the end of the parallal task.
    response->event_log.push_back(profile::stop_t());
}

class func_replacer_t : public btree_batched_replacer_t {
public:
    func_replacer_t(ql::env_t *_env, const ql::wire_func_t &wf, bool _return_vals)
        : env(_env), f(wf.compile_wire_func()), return_vals(_return_vals) { }
    counted_t<const ql::datum_t> replace(
        const counted_t<const ql::datum_t> &d, size_t) const {
        return f->call(env, d, ql::LITERAL_OK)->as_datum();
    }
    bool should_return_vals() const { return return_vals; }
private:
    ql::env_t *const env;
    const counted_t<ql::func_t> f;
    const bool return_vals;
};

class datum_replacer_t : public btree_batched_replacer_t {
public:
    datum_replacer_t(const std::vector<counted_t<const ql::datum_t> > *_datums,
                     bool _upsert, const std::string &_pkey, bool _return_vals)
        : datums(_datums), upsert(_upsert), pkey(_pkey), return_vals(_return_vals) { }
    counted_t<const ql::datum_t> replace(
        const counted_t<const ql::datum_t> &d, size_t index) const {
        guarantee(index < datums->size());
        counted_t<const ql::datum_t> newd = (*datums)[index];
        if (d->get_type() == ql::datum_t::R_NULL || upsert) {
            return newd;
        } else {
            rfail_target(d, ql::base_exc_t::GENERIC,
                         "Duplicate primary key `%s`:\n%s\n%s",
                         pkey.c_str(), d->print().c_str(), newd->print().c_str());
        }
        unreachable();
    }
    bool should_return_vals() const { return return_vals; }
private:
    const std::vector<counted_t<const ql::datum_t> > *const datums;
    const bool upsert;
    const std::string pkey;
    const bool return_vals;
};

// TODO: get rid of this extra response_t copy on the stack
struct rdb_write_visitor_t : public boost::static_visitor<void> {
    void operator()(const batched_replace_t &br) {
        ql_env.global_optargs.init_optargs(br.optargs);
        rdb_modification_report_cb_t sindex_cb(
            store, &sindex_block,
            auto_drainer_t::lock_t(&store->drainer));
        func_replacer_t replacer(&ql_env, br.f, br.return_vals);
        response->response =
            rdb_batched_replace(
                btree_info_t(btree, timestamp,
                             &br.pkey),
                superblock, br.keys, &replacer, &sindex_cb,
                ql_env.trace.get_or_null());
    }

    void operator()(const batched_insert_t &bi) {
        rdb_modification_report_cb_t sindex_cb(
            store,
            &sindex_block,
            auto_drainer_t::lock_t(&store->drainer));
        datum_replacer_t replacer(&bi.inserts, bi.upsert, bi.pkey, bi.return_vals);
        std::vector<store_key_t> keys;
        keys.reserve(bi.inserts.size());
        for (auto it = bi.inserts.begin(); it != bi.inserts.end(); ++it) {
            keys.emplace_back((*it)->get(bi.pkey)->print_primary());
        }
        response->response =
            rdb_batched_replace(
                btree_info_t(btree, timestamp,
                             &bi.pkey),
                superblock, keys, &replacer, &sindex_cb,
                ql_env.trace.get_or_null());
    }

    void operator()(const point_write_t &w) {
        response->response = point_write_response_t();
        point_write_response_t *res =
            boost::get<point_write_response_t>(&response->response);

        rdb_live_deletion_context_t deletion_context;
        rdb_modification_report_t mod_report(w.key);
        rdb_set(w.key, w.data, w.overwrite, btree, timestamp, superblock->get(),
                &deletion_context, res, &mod_report.info, ql_env.trace.get_or_null());

        update_sindexes(&mod_report);
    }

    void operator()(const point_delete_t &d) {
        response->response = point_delete_response_t();
        point_delete_response_t *res =
            boost::get<point_delete_response_t>(&response->response);

        rdb_live_deletion_context_t deletion_context;
        rdb_modification_report_t mod_report(d.key);
        rdb_delete(d.key, btree, timestamp, superblock->get(), &deletion_context,
                res, &mod_report.info, ql_env.trace.get_or_null());

        update_sindexes(&mod_report);
    }

    void operator()(const sindex_create_t &c) {
        sindex_create_response_t res;

        write_message_t wm;
        wm << c.mapping;
        wm << c.multi;

        vector_stream_t stream;
        stream.reserve(wm.size());
        int write_res = send_write_message(&stream, &wm);
        guarantee(write_res == 0);

        res.success = store->add_sindex(
            c.id,
            stream.vector(),
            &sindex_block);

        if (res.success) {
            std::set<std::string> sindexes;
            sindexes.insert(c.id);
            rdb_protocol_details::bring_sindexes_up_to_date(
                sindexes, store, &sindex_block);
        }

        response->response = res;
    }

    void operator()(const sindex_drop_t &d) {
        sindex_drop_response_t res;
        value_sizer_t<rdb_value_t> sizer(btree->cache()->get_block_size());
        rdb_live_deletion_context_t live_deletion_context;
        rdb_post_construction_deletion_context_t post_construction_deletion_context;

        res.success = store->drop_sindex(d.id,
                                         &sindex_block,
                                         &sizer,
                                         &live_deletion_context,
                                         &post_construction_deletion_context,
                                         &interruptor);

        response->response = res;
    }

    void operator()(const sync_t &) {
        response->response = sync_response_t();

        // We know this sync_t operation will force all preceding write transactions
        // (on our cache_conn_t) to flush before or at the same time, because the
        // cache guarantees that.  (Right now it will force _all_ preceding write
        // transactions to flush, on any conn, because they all touch the metainfo in
        // the superblock.)
    }


    rdb_write_visitor_t(btree_slice_t *_btree,
                        btree_store_t<rdb_protocol_t> *_store,
                        txn_t *_txn,
                        scoped_ptr_t<superblock_t> *_superblock,
                        repli_timestamp_t _timestamp,
                        rdb_protocol_t::context_t *ctx,
                        write_response_t *_response,
                        signal_t *_interruptor) :
        btree(_btree),
        store(_store),
        txn(_txn),
        response(_response),
        superblock(_superblock),
        timestamp(_timestamp),
        interruptor(_interruptor, ctx->signals[get_thread_id().threadnum].get()),
        ql_env(ctx->extproc_pool,
               ctx->ns_repo,
               ctx->cross_thread_namespace_watchables[get_thread_id().threadnum].get()->get_watchable(),
               ctx->cross_thread_database_watchables[get_thread_id().threadnum].get()->get_watchable(),
               ctx->cluster_metadata,
               NULL,
               &interruptor,
               ctx->machine_id,
               ql::protob_t<Query>()) {
        sindex_block =
            store->acquire_sindex_block_for_write((*superblock)->expose_buf(),
                                                  (*superblock)->get_sindex_block_id());
    }

    ql::env_t *get_env() {
        return &ql_env;
    }

    profile::event_log_t extract_event_log() {
        if (ql_env.trace.has()) {
            return std::move(*ql_env.trace).extract_event_log();
        } else {
            return profile::event_log_t();
        }
    }

private:
    void update_sindexes(const rdb_modification_report_t *mod_report) {
        scoped_ptr_t<new_mutex_in_line_t> acq =
            store->get_in_line_for_sindex_queue(&sindex_block);

        write_message_t wm;
        wm << rdb_sindex_change_t(*mod_report);
        store->sindex_queue_push(wm, acq.get());

        sindex_access_vector_t sindexes;
        store->acquire_post_constructed_sindex_superblocks_for_write(&sindex_block,
                                                                     &sindexes);
        rdb_live_deletion_context_t deletion_context;
        rdb_update_sindexes(sindexes, mod_report, txn, &deletion_context);
    }

    btree_slice_t *btree;
    btree_store_t<rdb_protocol_t> *store;
    txn_t *txn;
    write_response_t *response;
    scoped_ptr_t<superblock_t> *superblock;
    repli_timestamp_t timestamp;
    wait_any_t interruptor;
    ql::env_t ql_env;
    buf_lock_t sindex_block;

    DISABLE_COPYING(rdb_write_visitor_t);
};

void store_t::protocol_write(const write_t &write,
                             write_response_t *response,
                             transition_timestamp_t timestamp,
                             btree_slice_t *btree,
                             scoped_ptr_t<superblock_t> *superblock,
                             signal_t *interruptor) {
    rdb_write_visitor_t v(btree, this,
                          (*superblock)->expose_buf().txn(),
                          superblock,
                          timestamp.to_repli_timestamp(), ctx,
                          response, interruptor);
    {
        profile::starter_t start_write("Perform write on shard.", v.get_env()->trace);
        boost::apply_visitor(v, write.write);
    }

    response->n_shards = 1;
    response->event_log = v.extract_event_log();
    //This is a tad hacky, this just adds a stop event to signal the end of the parallal task.
    response->event_log.push_back(profile::stop_t());
}

struct rdb_backfill_chunk_get_btree_repli_timestamp_visitor_t : public boost::static_visitor<repli_timestamp_t> {
    repli_timestamp_t operator()(const backfill_chunk_t::delete_key_t &del) {
        return del.recency;
    }

    repli_timestamp_t operator()(const backfill_chunk_t::delete_range_t &) {
        return repli_timestamp_t::invalid;
    }

    repli_timestamp_t operator()(const backfill_chunk_t::key_value_pairs_t &kv) {
        repli_timestamp_t most_recent = repli_timestamp_t::invalid;
        rassert(!kv.backfill_atoms.empty());
        for (size_t i = 0; i < kv.backfill_atoms.size(); ++i) {
            if (most_recent == repli_timestamp_t::invalid
                || most_recent < kv.backfill_atoms[i].recency) {

                most_recent = kv.backfill_atoms[i].recency;
            }
        }
        return most_recent;
    }

    repli_timestamp_t operator()(const backfill_chunk_t::sindexes_t &) {
        return repli_timestamp_t::invalid;
    }
};

repli_timestamp_t backfill_chunk_t::get_btree_repli_timestamp() const THROWS_NOTHING {
    rdb_backfill_chunk_get_btree_repli_timestamp_visitor_t v;
    return boost::apply_visitor(v, val);
}

struct rdb_backfill_callback_impl_t : public rdb_backfill_callback_t {
public:
    typedef backfill_chunk_t chunk_t;

    explicit rdb_backfill_callback_impl_t(chunk_fun_callback_t<rdb_protocol_t> *_chunk_fun_cb)
        : chunk_fun_cb(_chunk_fun_cb) { }
    ~rdb_backfill_callback_impl_t() { }

    void on_delete_range(const key_range_t &range,
                         signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        chunk_fun_cb->send_chunk(chunk_t::delete_range(region_t(range)), interruptor);
    }

    void on_deletion(const btree_key_t *key, repli_timestamp_t recency,
                     signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        chunk_fun_cb->send_chunk(chunk_t::delete_key(to_store_key(key), recency),
                                 interruptor);
    }

    void on_keyvalues(std::vector<rdb_backfill_atom_t> &&atoms,
                      signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        chunk_fun_cb->send_chunk(chunk_t::set_keys(std::move(atoms)), interruptor);
    }

    void on_sindexes(const std::map<std::string, secondary_index_t> &sindexes,
                     signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        chunk_fun_cb->send_chunk(chunk_t::sindexes(sindexes), interruptor);
    }

protected:
    store_key_t to_store_key(const btree_key_t *key) {
        return store_key_t(key->size, key->contents);
    }

private:
    chunk_fun_callback_t<rdb_protocol_t> *chunk_fun_cb;

    DISABLE_COPYING(rdb_backfill_callback_impl_t);
};

void call_rdb_backfill(int i, btree_slice_t *btree,
                       const std::vector<std::pair<region_t, state_timestamp_t> > &regions,
                       rdb_backfill_callback_t *callback,
                       superblock_t *superblock,
                       buf_lock_t *sindex_block,
                       backfill_progress_t *progress,
                       signal_t *interruptor) THROWS_NOTHING {
    parallel_traversal_progress_t *p = new parallel_traversal_progress_t;
    scoped_ptr_t<traversal_progress_t> p_owned(p);
    progress->add_constituent(&p_owned);
    repli_timestamp_t timestamp = regions[i].second.to_repli_timestamp();
    try {
        rdb_backfill(btree, regions[i].first.inner, timestamp, callback,
                     superblock, sindex_block, p, interruptor);
    } catch (const interrupted_exc_t &) {
        /* do nothing; `protocol_send_backfill()` will notice that interruptor
        has been pulsed */
    }
}

void store_t::protocol_send_backfill(const region_map_t<rdb_protocol_t, state_timestamp_t> &start_point,
                                     chunk_fun_callback_t<rdb_protocol_t> *chunk_fun_cb,
                                     superblock_t *superblock,
                                     buf_lock_t *sindex_block,
                                     btree_slice_t *btree,
                                     backfill_progress_t *progress,
                                     signal_t *interruptor)
                                     THROWS_ONLY(interrupted_exc_t) {
    with_priority_t p(CORO_PRIORITY_BACKFILL_SENDER);
    rdb_backfill_callback_impl_t callback(chunk_fun_cb);
    std::vector<std::pair<region_t, state_timestamp_t> > regions(start_point.begin(), start_point.end());
    refcount_superblock_t refcount_wrapper(superblock, regions.size());
    pmap(regions.size(), std::bind(&call_rdb_backfill, ph::_1,
                                   btree, regions, &callback,
                                   &refcount_wrapper, sindex_block, progress,
                                   interruptor));

    /* If interruptor was pulsed, `call_rdb_backfill()` exited silently, so we
    have to check directly. */
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
}

void backfill_chunk_single_rdb_set(const rdb_backfill_atom_t &bf_atom,
                                   btree_slice_t *btree, superblock_t *superblock,
                                   UNUSED auto_drainer_t::lock_t drainer_acq,
                                   rdb_modification_report_t *mod_report_out,
                                   promise_t<superblock_t *> *superblock_promise_out) {
    mod_report_out->primary_key = bf_atom.key;
    point_write_response_t response;
    rdb_live_deletion_context_t deletion_context;
    rdb_set(bf_atom.key, bf_atom.value, true,
            btree, bf_atom.recency, superblock, &deletion_context, &response,
            &mod_report_out->info, static_cast<profile::trace_t *>(NULL),
            superblock_promise_out);
}

struct rdb_receive_backfill_visitor_t : public boost::static_visitor<void> {
    rdb_receive_backfill_visitor_t(btree_store_t<rdb_protocol_t> *_store,
                                   btree_slice_t *_btree,
                                   txn_t *_txn,
                                   scoped_ptr_t<superblock_t> &&_superblock,
                                   signal_t *_interruptor) :
        store(_store), btree(_btree), txn(_txn), superblock(std::move(_superblock)),
        interruptor(_interruptor) {
        sindex_block =
            store->acquire_sindex_block_for_write(superblock->expose_buf(),
                                                  superblock->get_sindex_block_id());
    }

    void operator()(const backfill_chunk_t::delete_key_t &delete_key) {
        point_delete_response_t response;
        std::vector<rdb_modification_report_t> mod_reports(1);
        mod_reports[0].primary_key = delete_key.key;
        rdb_live_deletion_context_t deletion_context;
        rdb_delete(delete_key.key, btree, delete_key.recency,
                   superblock.get(), &deletion_context, &response,
                   &mod_reports[0].info, static_cast<profile::trace_t *>(NULL));
        superblock.reset();
        update_sindexes(mod_reports);
    }

    void operator()(const backfill_chunk_t::delete_range_t &delete_range) {
        range_key_tester_t tester(&delete_range.range);
        rdb_live_deletion_context_t deletion_context;
        std::vector<rdb_modification_report_t> mod_reports;
        rdb_erase_small_range(&tester, delete_range.range.inner,
                              superblock.get(), &deletion_context, interruptor,
                              &mod_reports);
        superblock.reset();
        if (!mod_reports.empty()) {
            update_sindexes(mod_reports);
        }
    }

    void operator()(const backfill_chunk_t::key_value_pairs_t &kv) {
        std::vector<rdb_modification_report_t> mod_reports(kv.backfill_atoms.size());
        {
            auto_drainer_t drainer;
            for (size_t i = 0; i < kv.backfill_atoms.size(); ++i) {
                promise_t<superblock_t *> superblock_promise;
                // `spawn_now_dangerously` so that we don't have to wait for the
                // superblock if it's immediately available.
                coro_t::spawn_now_dangerously(std::bind(&backfill_chunk_single_rdb_set,
                                                        kv.backfill_atoms[i], btree,
                                                        superblock.release(),
                                                        auto_drainer_t::lock_t(&drainer),
                                                        &mod_reports[i],
                                                        &superblock_promise));
                superblock.init(superblock_promise.wait());
            }
            superblock.reset();
        }
        update_sindexes(mod_reports);
    }

    void operator()(const backfill_chunk_t::sindexes_t &s) {
        // Release the superblock. We don't need it for this.
        superblock.reset();

        value_sizer_t<rdb_value_t> sizer(txn->cache()->get_block_size());
        rdb_live_deletion_context_t live_deletion_context;
        rdb_post_construction_deletion_context_t post_construction_deletion_context;
        std::set<std::string> created_sindexes;

        // backfill_chunk_t::sindexes_t contains hard-coded UUIDs for the
        // secondary indexes. This can cause problems if indexes are deleted
        // and recreated very quickly. The very reason for why we have UUIDs
        // in the sindexes is to avoid two post-constructions to interfere with
        // each other in such cases.
        // More information:
        // https://github.com/rethinkdb/rethinkdb/issues/657
        // https://github.com/rethinkdb/rethinkdb/issues/2087
        //
        // Assign new random UUIDs to the secondary indexes to avoid collisions
        // during post construction:
        std::map<std::string, secondary_index_t> sindexes = s.sindexes;
        for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
            it->second.id = generate_uuid();
        }

        store->set_sindexes(sindexes, &sindex_block, &sizer,
                            &live_deletion_context,
                            &post_construction_deletion_context,
                            &created_sindexes, interruptor);

        if (!created_sindexes.empty()) {
            rdb_protocol_details::bring_sindexes_up_to_date(created_sindexes, store,
                                                            &sindex_block);
        }
    }

private:
    void update_sindexes(const std::vector<rdb_modification_report_t> &mod_reports) {
        scoped_ptr_t<new_mutex_in_line_t> acq =
            store->get_in_line_for_sindex_queue(&sindex_block);
        scoped_array_t<write_message_t> queue_wms(mod_reports.size());
        {
            sindex_access_vector_t sindexes;
            store->acquire_post_constructed_sindex_superblocks_for_write(
                    &sindex_block, &sindexes);
            sindex_block.reset_buf_lock();

            rdb_live_deletion_context_t deletion_context;
            for (size_t i = 0; i < mod_reports.size(); ++i) {
                queue_wms[i] << rdb_sindex_change_t(mod_reports[i]);
                rdb_update_sindexes(sindexes, &mod_reports[i], txn, &deletion_context);
            }
        }

        // Write mod reports onto the sindex queue. We are in line for the
        // sindex_queue mutex and can already release all other locks.
        store->sindex_queue_push(queue_wms, acq.get());
    }

    btree_store_t<rdb_protocol_t> *store;
    btree_slice_t *btree;
    txn_t *txn;
    scoped_ptr_t<superblock_t> superblock;
    signal_t *interruptor;
    buf_lock_t sindex_block;

    DISABLE_COPYING(rdb_receive_backfill_visitor_t);
};

void store_t::protocol_receive_backfill(btree_slice_t *btree,
                                        scoped_ptr_t<superblock_t> &&_superblock,
                                        signal_t *interruptor,
                                        const backfill_chunk_t &chunk) {
    scoped_ptr_t<superblock_t> superblock(std::move(_superblock));
    with_priority_t p(CORO_PRIORITY_BACKFILL_RECEIVER);
    rdb_receive_backfill_visitor_t v(this, btree,
                                     superblock->expose_buf().txn(),
                                     std::move(superblock),
                                     interruptor);
    boost::apply_visitor(v, chunk.val);
}

void store_t::protocol_reset_data(const region_t& subregion,
                                  btree_slice_t *btree,
                                  superblock_t *superblock,
                                  signal_t *interruptor) {
    with_priority_t p(CORO_PRIORITY_RESET_DATA);
    value_sizer_t<rdb_value_t> sizer(btree->cache()->get_block_size());

    always_true_key_tester_t key_tester;
    buf_lock_t sindex_block
        = acquire_sindex_block_for_write(superblock->expose_buf(),
                                         superblock->get_sindex_block_id());
    rdb_erase_major_range(&key_tester, subregion.inner,
                          &sindex_block,
                          superblock, this,
                          interruptor);
}

region_t rdb_protocol_t::cpu_sharding_subspace(int subregion_number,
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

RDB_IMPL_ME_SERIALIZABLE_3(rdb_protocol_details::single_sindex_status_t,
                           blocks_total, blocks_processed, ready);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::point_read_response_t, data);
RDB_IMPL_ME_SERIALIZABLE_4(rdb_protocol_t::rget_read_response_t,
                           result, key_range, truncated, last_key);
RDB_IMPL_ME_SERIALIZABLE_2(rdb_protocol_t::distribution_read_response_t,
                           region, key_counts);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::sindex_list_response_t, sindexes);
RDB_IMPL_ME_SERIALIZABLE_3(rdb_protocol_t::read_response_t,
                           response, event_log, n_shards);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::sindex_status_response_t, statuses);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::point_read_t, key);
RDB_IMPL_ME_SERIALIZABLE_3(rdb_protocol_t::sindex_rangespec_t,
                           id, region, original_range);

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(key_range_t::bound_t, int8_t,
                                      key_range_t::open, key_range_t::none);
RDB_IMPL_ME_SERIALIZABLE_4(datum_range_t,
                           empty_ok(left_bound), empty_ok(right_bound),
                           left_bound_type, right_bound_type);
RDB_IMPL_ME_SERIALIZABLE_7(rdb_protocol_t::rget_read_t,
                           region, optargs, batchspec,
                           transforms, terminal, sindex, sorting);

RDB_IMPL_ME_SERIALIZABLE_3(rdb_protocol_t::distribution_read_t,
                           max_depth, result_limit, region);
RDB_IMPL_ME_SERIALIZABLE_0(rdb_protocol_t::sindex_list_t);
RDB_IMPL_ME_SERIALIZABLE_2(rdb_protocol_t::sindex_status_t, sindexes, region);
RDB_IMPL_ME_SERIALIZABLE_2(rdb_protocol_t::read_t, read, profile);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::point_write_response_t, result);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::point_delete_response_t, result);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::sindex_create_response_t, success);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::sindex_drop_response_t, success);
RDB_IMPL_ME_SERIALIZABLE_0(rdb_protocol_t::sync_response_t);

RDB_IMPL_ME_SERIALIZABLE_3(rdb_protocol_t::write_response_t, response, event_log, n_shards);

RDB_IMPL_ME_SERIALIZABLE_5(rdb_protocol_t::batched_replace_t,
                           keys, pkey, f, optargs, return_vals);
RDB_IMPL_ME_SERIALIZABLE_4(rdb_protocol_t::batched_insert_t,
                           inserts, pkey, upsert, return_vals);

RDB_IMPL_ME_SERIALIZABLE_3(rdb_protocol_t::point_write_t, key, data, overwrite);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::point_delete_t, key);

RDB_IMPL_ME_SERIALIZABLE_4(rdb_protocol_t::sindex_create_t, id, mapping, region, multi);
RDB_IMPL_ME_SERIALIZABLE_2(rdb_protocol_t::sindex_drop_t, id, region);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::sync_t, region);

RDB_IMPL_ME_SERIALIZABLE_3(rdb_protocol_t::write_t,
                           write, durability_requirement, profile);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::backfill_chunk_t::delete_key_t, key);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::backfill_chunk_t::delete_range_t, range);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::backfill_chunk_t::key_value_pairs_t,
                           backfill_atoms);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::backfill_chunk_t::sindexes_t, sindexes);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::backfill_chunk_t, val);
