// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/protocol.hpp"

#include <algorithm>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/io/disk.hpp"
#include "btree/erase_range.hpp"
#include "btree/parallel_traversal.hpp"
#include "btree/slice.hpp"
#include "btree/superblock.hpp"
#include "clustering/administration/metadata.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "concurrency/pmap.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/archive/archive.hpp"
#include "containers/archive/vector_stream.hpp"
#include "protob/protob.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/transform_visitors.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/term_walker.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "rpc/semilattice/watchable.hpp"
#include "serializer/config.hpp"

// Ignore clang errors for ql macros
#pragma GCC diagnostic ignored "-Wshadow"

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

typedef rdb_protocol_t::write_t write_t;
typedef rdb_protocol_t::write_response_t write_response_t;

typedef rdb_protocol_t::point_replace_t point_replace_t;
typedef rdb_protocol_t::point_replace_response_t point_replace_response_t;

typedef rdb_protocol_t::batched_replaces_t batched_replaces_t;

typedef rdb_protocol_t::point_write_t point_write_t;
typedef rdb_protocol_t::point_write_response_t point_write_response_t;

typedef rdb_protocol_t::point_delete_t point_delete_t;
typedef rdb_protocol_t::point_delete_response_t point_delete_response_t;

typedef rdb_protocol_t::sindex_create_t sindex_create_t;
typedef rdb_protocol_t::sindex_create_response_t sindex_create_response_t;

typedef rdb_protocol_t::sindex_drop_t sindex_drop_t;
typedef rdb_protocol_t::sindex_drop_response_t sindex_drop_response_t;

typedef rdb_protocol_t::backfill_chunk_t backfill_chunk_t;

typedef rdb_protocol_t::backfill_progress_t backfill_progress_t;

typedef rdb_protocol_t::rget_read_response_t::stream_t stream_t;

typedef btree_store_t<rdb_protocol_t>::sindex_access_vector_t sindex_access_vector_t;

const std::string rdb_protocol_t::protocol_name("rdb");

RDB_IMPL_PROTOB_SERIALIZABLE(Term);
RDB_IMPL_PROTOB_SERIALIZABLE(Datum);
RDB_IMPL_PROTOB_SERIALIZABLE(Backtrace);


RDB_IMPL_SERIALIZABLE_2(filter_transform_t, filter_func, default_filter_val);

namespace rdb_protocol_details {

RDB_IMPL_SERIALIZABLE_3(backfill_atom_t, key, value, recency);

void post_construct_and_drain_queue(
        const std::set<uuid_u> &sindexes_to_bring_up_to_date,
        btree_store_t<rdb_protocol_t> *store,
        boost::shared_ptr<internal_disk_backed_queue_t> mod_queue,
        auto_drainer_t::lock_t lock)
    THROWS_NOTHING;

/* Creates a queue of operations for the sindex, runs a post construction for
 * the data already in the btree and finally drains the queue. */
void bring_sindexes_up_to_date(
        const std::set<std::string> &sindexes_to_bring_up_to_date,
        btree_store_t<rdb_protocol_t> *store,
        buf_lock_t *sindex_block,
        transaction_t *txn)
    THROWS_NOTHING
{
    /* We register our modification queue here. An important point about
     * correctness here: we've held the superblock this whole time and will
     * continue to do so until the call to post_construct_secondary_indexes
     * begins a parallel traversal which releases the superblock. This
     * serves to make sure that every changes which we don't learn about in
     * the parallel traversal we do learn about from the mod queue. */
    uuid_u post_construct_id = generate_uuid();

    boost::shared_ptr<internal_disk_backed_queue_t> mod_queue(
            new internal_disk_backed_queue_t(
                store->io_backender_,
                serializer_filepath_t(store->base_path_, "post_construction_" + uuid_to_str(post_construct_id)),
                &store->perfmon_collection));

    {
        mutex_t::acq_t acq;
        store->lock_sindex_queue(sindex_block, &acq);
        store->register_sindex_queue(mod_queue.get(), &acq);
    }

    std::map<std::string, secondary_index_t> sindexes;
    store->get_sindexes(sindex_block, txn, &sindexes);
    std::set<uuid_u> sindexes_to_bring_up_to_date_uuid;

    for (auto it = sindexes_to_bring_up_to_date.begin();
         it != sindexes_to_bring_up_to_date.end(); ++it) {
        guarantee(std_contains(sindexes, *it));
        sindexes_to_bring_up_to_date_uuid.insert(sindexes[*it].id);
    }

    coro_t::spawn_sometime(boost::bind(
                &post_construct_and_drain_queue,
                sindexes_to_bring_up_to_date_uuid,
                store,
                mod_queue,
                auto_drainer_t::lock_t(&store->drainer)));
}

class apply_sindex_change_visitor_t : public boost::static_visitor<> {
public:
    apply_sindex_change_visitor_t(const sindex_access_vector_t *sindexes,
            transaction_t *txn,
            signal_t *interruptor)
        : sindexes_(sindexes), txn_(txn), interruptor_(interruptor) { }
    void operator()(const rdb_modification_report_t &mod_report) const {
        rdb_update_sindexes(*sindexes_, &mod_report, txn_);
    }

    void operator()(const rdb_erase_range_report_t &erase_range_report) const {
        rdb_erase_range_sindexes(*sindexes_, &erase_range_report, txn_, interruptor_);
    }

private:
    const sindex_access_vector_t *sindexes_;
    transaction_t *txn_;
    signal_t *interruptor_;
};

/* This function is really part of the logic of bring_sindexes_up_to_date
 * however it needs to be in a seperate function so that it can be spawned in a
 * coro. */
void post_construct_and_drain_queue(
        const std::set<uuid_u> &sindexes_to_bring_up_to_date,
        btree_store_t<rdb_protocol_t> *store,
        boost::shared_ptr<internal_disk_backed_queue_t> mod_queue,
        auto_drainer_t::lock_t lock)
    THROWS_NOTHING
{
    try {
        post_construct_secondary_indexes(store, sindexes_to_bring_up_to_date, lock.get_drain_signal());

        /* Drain the queue. */

        int previous_size = mod_queue->size();
        while (!lock.get_drain_signal()->is_pulsed()) {
            write_token_pair_t token_pair;
            store->new_write_token_pair(&token_pair);

            scoped_ptr_t<transaction_t> queue_txn;
            scoped_ptr_t<real_superblock_t> queue_superblock;

            // If we get interrupted, post-construction will happen later, no need to
            //  guarantee that we touch the sindex tree now
            object_buffer_t<fifo_enforcer_sink_t::exit_write_t>::destruction_sentinel_t
                destroyer(&token_pair.sindex_write_token);

            // We don't need hard durability here, because a secondary index just gets rebuilt
            // if the server dies while it's partially constructed.
            store->acquire_superblock_for_write(
                rwi_write,
                repli_timestamp_t::distant_past,
                2,
                WRITE_DURABILITY_SOFT,
                &token_pair,
                &queue_txn,
                &queue_superblock,
                lock.get_drain_signal());

            scoped_ptr_t<buf_lock_t> queue_sindex_block;
            store->acquire_sindex_block_for_write(
                &token_pair,
                queue_txn.get(),
                &queue_sindex_block,
                queue_superblock->get_sindex_block_id(),
                lock.get_drain_signal());

            sindex_access_vector_t sindexes;
            store->acquire_sindex_superblocks_for_write(
                    sindexes_to_bring_up_to_date,
                    queue_sindex_block.get(),
                    queue_txn.get(),
                    &sindexes);

            if (sindexes.empty()) {
                break;
            }

            mutex_t::acq_t acq;
            store->lock_sindex_queue(queue_sindex_block.get(), &acq);

            while (mod_queue->size() >= previous_size &&
                   mod_queue->size() > 0) {
                std::vector<char> data_vec;
                mod_queue->pop(&data_vec);
                vector_read_stream_t read_stream(&data_vec);

                rdb_sindex_change_t sindex_change;
                int ser_res = deserialize(&read_stream, &sindex_change);
                guarantee_err(ser_res == 0, "corruption in disk-backed queue");

                boost::apply_visitor(apply_sindex_change_visitor_t(&sindexes, queue_txn.get(), lock.get_drain_signal()),
                                     sindex_change);
            }

            previous_size = mod_queue->size();

            if (mod_queue->size() == 0) {
                for (auto it = sindexes_to_bring_up_to_date.begin();
                     it != sindexes_to_bring_up_to_date.end(); ++it) {
                    store->mark_index_up_to_date(*it, queue_txn.get(), queue_sindex_block.get());
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
        write_token_pair_t token_pair;
        store->new_write_token_pair(&token_pair);

        scoped_ptr_t<transaction_t> queue_txn;
        scoped_ptr_t<real_superblock_t> queue_superblock;

        // If we get interrupted, post-construction will happen later, no need to
        //  guarantee that we touch the sindex tree now
        object_buffer_t<fifo_enforcer_sink_t::exit_write_t>::destruction_sentinel_t
            destroyer(&token_pair.sindex_write_token);

        store->acquire_superblock_for_write(
            rwi_write,
            repli_timestamp_t::distant_past,
            2,
            WRITE_DURABILITY_HARD,
            &token_pair,
            &queue_txn,
            &queue_superblock,
            lock.get_drain_signal());

        scoped_ptr_t<buf_lock_t> queue_sindex_block;
        store->acquire_sindex_block_for_write(
            &token_pair,
            queue_txn.get(),
            &queue_sindex_block,
            queue_superblock->get_sindex_block_id(),
            lock.get_drain_signal());

        mutex_t::acq_t acq;
        store->lock_sindex_queue(queue_sindex_block.get(), &acq);
        store->deregister_sindex_queue(mod_queue.get(), &acq);
    }
}


bool range_key_tester_t::key_should_be_erased(const btree_key_t *key) {
    uint64_t h = hash_region_hasher(key->contents, key->size);
    return delete_range->beg <= h && h < delete_range->end
        && delete_range->inner.contains_key(key->contents, key->size);
}

typedef boost::variant<rdb_modification_report_t,
                       rdb_erase_range_report_t>
        sindex_change_t;

}  // namespace rdb_protocol_details

rdb_protocol_t::context_t::context_t()
    : extproc_pool(NULL), ns_repo(NULL),
    cross_thread_namespace_watchables(get_num_threads()),
    cross_thread_database_watchables(get_num_threads()),
    directory_read_manager(NULL),
    signals(get_num_threads())
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
    machine_id_t _machine_id)
    : extproc_pool(_extproc_pool), ns_repo(_ns_repo),
      cross_thread_namespace_watchables(get_num_threads()),
      cross_thread_database_watchables(get_num_threads()),
      cluster_metadata(_cluster_metadata),
      auth_metadata(_auth_metadata),
      directory_read_manager(_directory_read_manager),
      signals(get_num_threads()),
      machine_id(_machine_id)
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
};

region_t read_t::get_region() const THROWS_NOTHING {
    return boost::apply_visitor(rdb_r_get_region_visitor(), read);
}

struct rdb_r_shard_visitor_t : public boost::static_visitor<bool> {
    explicit rdb_r_shard_visitor_t(const hash_region_t<key_range_t> *_region,
                                   read_t *_read_out)
        : region(_region), read_out(_read_out) {}

    // The key was somehow already extracted from the arg.
    template <class T>
    bool keyed_read(const T &arg, const store_key_t &key) const {
        if (region_contains_key(*region, key)) {
            *read_out = read_t(arg);
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
            *read_out = read_t(tmp);
            return true;
        } else {
            return false;
        }
    }

    bool operator()(const rget_read_t &rg) const {
        return rangey_read(rg);
    }

    bool operator()(const distribution_read_t &dg) const {
        return rangey_read(dg);
    }

    bool operator()(const sindex_list_t &sl) const {
        return keyed_read(sl, sindex_list_region_key());
    }

    const hash_region_t<key_range_t> *region;
    read_t *read_out;
};

bool read_t::shard(const hash_region_t<key_range_t> &region,
                   read_t *read_out) const THROWS_NOTHING {
    return boost::apply_visitor(rdb_r_shard_visitor_t(&region, read_out), read);
}

/* read_t::unshard implementation */
bool read_response_cmp(const read_response_t &l, const read_response_t &r) {
    const rget_read_response_t *lr = boost::get<rget_read_response_t>(&l.response);
    guarantee(lr);
    const rget_read_response_t *rr = boost::get<rget_read_response_t>(&r.response);
    guarantee(rr);
    return lr->key_range < rr->key_range;
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
    rdb_r_unshard_visitor_t(const read_response_t *_responses,
                            size_t _count,
                            read_response_t *_response_out,
                            rdb_protocol_t::context_t *ctx,
                            signal_t *interruptor)
        : responses(_responses), count(_count), response_out(_response_out),
          ql_env(ctx->extproc_pool,
                 ctx->ns_repo,
                 ctx->cross_thread_namespace_watchables[get_thread_id().threadnum].get()
                     ->get_watchable(),
                 ctx->cross_thread_database_watchables[get_thread_id().threadnum].get()
                     ->get_watchable(),
                 ctx->cluster_metadata,
                 NULL,
                 interruptor,
                 ctx->machine_id,
                 std::map<std::string, ql::wire_func_t>())
    { }

    void operator()(const point_read_t &) {
        guarantee(count == 1);
        guarantee(NULL != boost::get<point_read_response_t>(&responses[0].response));
        *response_out = responses[0];
    }

    void operator()(const rget_read_t &rg) {
        response_out->response = rget_read_response_t();
        rget_read_response_t *rg_response
            = boost::get<rget_read_response_t>(&response_out->response);
        rg_response->truncated = false;
        rg_response->key_range = read_t(rg).get_region().inner;
        rg_response->last_considered_key = read_t(rg).get_region().inner.left;

        /* First check to see if any of the responses we're unsharding threw. */
        for (size_t i = 0; i < count; ++i) {
            // TODO: we're ignoring the limit when recombining.
            auto rr = boost::get<rget_read_response_t>(&responses[i].response);
            guarantee(rr != NULL);

            if (auto e = boost::get<ql::exc_t>(&rr->result)) {
                rg_response->result = *e;
                return;
            } else if (auto e = boost::get<ql::datum_exc_t>(&rr->result)) {
                rg_response->result = *e;
                return;
            }
        }

        if (!rg.terminal) {
            unshard_range_get(rg);
        } else {
            unshard_reduce(rg);
        }
    }

    void operator()(const distribution_read_t &dg) {
        // TODO: do this without copying so much and/or without dynamic memory
        // Sort results by region
        std::vector<distribution_read_response_t> results(count);
        guarantee(count > 0);

        for (size_t i = 0; i < count; ++i) {
            const distribution_read_response_t *result = boost::get<distribution_read_response_t>(&responses[i].response);
            guarantee(result != NULL, "Bad boost::get\n");
            results[i] = *result;
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
                for (std::map<store_key_t, int64_t>::const_iterator mit = results[i].key_counts.begin(); mit != results[i].key_counts.end(); ++mit) {
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
                double scale_factor = static_cast<double>(total_range_keys) / static_cast<double>(largest_size);

                guarantee(scale_factor >= 1.0);  // Directly provable from code above.

                for (std::map<store_key_t, int64_t>::iterator mit = results[largest_index].key_counts.begin();
                     mit != results[largest_index].key_counts.end();
                     ++mit) {
                    mit->second = static_cast<int64_t>(mit->second * scale_factor);
                }

                res.key_counts.insert(results[largest_index].key_counts.begin(), results[largest_index].key_counts.end());
            }
        }

        // If the result is larger than the requested limit, scale it down
        if (dg.result_limit > 0 && res.key_counts.size() > dg.result_limit) {
            scale_down_distribution(dg.result_limit, &res.key_counts);
        }

        response_out->response = res;
    }

    void operator()(UNUSED const sindex_list_t &sl) {
        guarantee(count == 1);
        guarantee(boost::get<sindex_list_response_t>(&responses[0].response));
        *response_out = responses[0];
    }

private:
    const read_response_t *responses;
    size_t count;
    read_response_t *response_out;
    ql::env_t ql_env;

    void unshard_range_get(const rget_read_t &rg) {
        rget_read_response_t *rg_response = boost::get<rget_read_response_t>(&response_out->response);
        // A vanilla range get
        // First we need to determine the cutoff key:
        rg_response->last_considered_key = forward(rg.sorting) ? store_key_t::max() : store_key_t::min();
        for (size_t i = 0; i < count; ++i) {
            const rget_read_response_t *rr = boost::get<rget_read_response_t>(&responses[i].response);
            guarantee(rr != NULL);

            if (rr->truncated &&
                    ((rg_response->last_considered_key > rr->last_considered_key && forward(rg.sorting)) ||
                     (rg_response->last_considered_key < rr->last_considered_key && backward(rg.sorting)))) {
                rg_response->last_considered_key = rr->last_considered_key;
            }
        }

        rg_response->result = stream_t();
        stream_t *res_stream = boost::get<stream_t>(&rg_response->result);

        if (rg.sorting == UNORDERED) {
            for (size_t i = 0; i < count; ++i) {
                // TODO: we're ignoring the limit when recombining.
                const rget_read_response_t *rr = boost::get<rget_read_response_t>(&responses[i].response);
                guarantee(rr != NULL);

                const stream_t *stream = boost::get<stream_t>(&(rr->result));

                for (stream_t::const_iterator it = stream->begin(); it != stream->end(); ++it) {
                    if ((it->key <= rg_response->last_considered_key && forward(rg.sorting)) ||
                            (it->key >= rg_response->last_considered_key && backward(rg.sorting))) {
                        res_stream->push_back(*it);
                    }
                }

                rg_response->truncated = rg_response->truncated || rr->truncated;
            }
        } else {
            std::vector<std::pair<stream_t::const_iterator, stream_t::const_iterator> > iterators;

            for (size_t i = 0; i < count; ++i) {
                // TODO: we're ignoring the limit when recombining.
                const rget_read_response_t *rr = boost::get<rget_read_response_t>(&responses[i].response);
                guarantee(rr != NULL);

                const stream_t *stream = boost::get<stream_t>(&(rr->result));
                iterators.push_back(std::make_pair(stream->begin(), stream->end()));
            }

            while (true) {
                store_key_t key_to_beat = (forward(rg.sorting) ? store_key_t::max() : store_key_t::min());
                bool found_value = false;
                stream_t::const_iterator *value = NULL;

                for (auto it = iterators.begin(); it != iterators.end(); ++it) {
                    if (it->first == it->second) {
                        continue;
                    }

                    if ((forward(rg.sorting) &&
                                it->first->key <= key_to_beat &&
                                it->first->key <= rg_response->last_considered_key) ||
                            (backward(rg.sorting) &&
                             it->first->key >= key_to_beat &&
                             it->first->key >= rg_response->last_considered_key)) {
                        key_to_beat = it->first->key;
                        found_value = true;
                        value = &it->first;
                    }
                }
                if (found_value) {
                    res_stream->push_back(**value);
                    ++(*value);
                } else {
                    break;
                }
            }
        }

    }

    void unshard_reduce(const rget_read_t &rg) {
        rget_read_response_t *rg_response = boost::get<rget_read_response_t>(
            &response_out->response);
        try {
            if (const ql::reduce_wire_func_t *reduce_func =
                    boost::get<ql::reduce_wire_func_t>(&*rg.terminal)) {
                ql::reduce_wire_func_t local_reduce_func = *reduce_func;
                rg_response->result = rget_read_response_t::empty_t();
                for (size_t i = 0; i < count; ++i) {
                    const rget_read_response_t *_rr =
                        boost::get<rget_read_response_t>(&responses[i].response);
                    guarantee(_rr);
                    counted_t<const ql::datum_t> *lhs =
                        boost::get<counted_t<const ql::datum_t> >(&rg_response->result);
                    const counted_t<const ql::datum_t> *rhs =
                        boost::get<counted_t<const ql::datum_t> >(&(_rr->result));
                    if (!rhs) {
                        guarantee(boost::get<rget_read_response_t::empty_t>(
                                      &(_rr->result)));
                        continue;
                    } else {
                        if (lhs) {
                            counted_t<const ql::datum_t> reduced_val =
                                local_reduce_func.compile_wire_func()->call(
                                    &ql_env, *lhs, *rhs)->as_datum();
                            rg_response->result = reduced_val;
                        } else {
                            guarantee(boost::get<rget_read_response_t::empty_t>(
                                          &rg_response->result));
                            rg_response->result = _rr->result;
                        }
                    }
                }
            } else if (boost::get<ql::count_wire_func_t>(&*rg.terminal)) {
                rg_response->result = make_counted<const ql::datum_t>(0.0);

                for (size_t i = 0; i < count; ++i) {
                    const rget_read_response_t *_rr =
                        boost::get<rget_read_response_t>(&responses[i].response);
                    guarantee(_rr);
                    counted_t<const ql::datum_t> lhs =
                        boost::get<counted_t<const ql::datum_t> >(rg_response->result);
                    counted_t<const ql::datum_t> rhs =
                        boost::get<counted_t<const ql::datum_t> >(_rr->result);
                    rg_response->result = make_counted<const ql::datum_t>(
                        lhs->as_num() + rhs->as_num());
                }
            } else if (const ql::gmr_wire_func_t *gmr_func =
                    boost::get<ql::gmr_wire_func_t>(&*rg.terminal)) {
                ql::gmr_wire_func_t local_gmr_func = *gmr_func;
                rg_response->result = ql::wire_datum_map_t();
                ql::wire_datum_map_t *map =
                    boost::get<ql::wire_datum_map_t>(&rg_response->result);

                  for (size_t i = 0; i < count; ++i) {
                    const rget_read_response_t *_rr =
                        boost::get<rget_read_response_t>(&responses[i].response);
                    guarantee(_rr);
                    const ql::wire_datum_map_t *rhs =
                        boost::get<ql::wire_datum_map_t>(&(_rr->result));
                    r_sanity_check(rhs);
                    ql::wire_datum_map_t local_rhs = *rhs;
                    local_rhs.compile();

                    counted_t<const ql::datum_t> rhs_arr = local_rhs.to_arr();
                    for (size_t f = 0; f < rhs_arr->size(); ++f) {
                        counted_t<const ql::datum_t> key
                            = rhs_arr->get(f)->get("group");
                        counted_t<const ql::datum_t> val
                            = rhs_arr->get(f)->get("reduction");
                        if (!map->has(key)) {
                            map->set(key, val);
                        } else {
                            counted_t<ql::func_t> r
                                = local_gmr_func.compile_reduce();
                            map->set(key, r->call(&ql_env, map->get(key), val)->as_datum());
                        }
                    }
                }
                boost::get<ql::wire_datum_map_t>(rg_response->result).finalize();
            } else {
                unreachable();
            }
        } catch (const ql::datum_exc_t &e) {
            /* Evaluation threw so we're not going to be accepting any
               more requests. */
            terminal_exception(e, *rg.terminal, &rg_response->result);
        }
    }
};

void read_t::unshard(read_response_t *responses, size_t count,
                     read_response_t *response, context_t *ctx,
                     signal_t *interruptor) const
    THROWS_ONLY(interrupted_exc_t) {
    rdb_r_unshard_visitor_t v(responses, count, response, ctx, interruptor);
    boost::apply_visitor(v, read);
}

/* write_t::get_region() implementation */

// TODO: This entire type is suspect, given the performance for
// batched_replaces_t.  Is it used in anything other than assertions?
struct rdb_w_get_region_visitor : public boost::static_visitor<region_t> {
    region_t operator()(const point_replace_t &pr) const {
        return rdb_protocol_t::monokey_region(pr.key);
    }

    static const store_key_t &point_replace_key(const std::pair<int64_t, point_replace_t> &pair) {
        return pair.second.key;
    }

    region_t operator()(const batched_replaces_t &br) const {
        // It shouldn't be empty, but we let the places that would break use a guarantee.
        rassert(!br.point_replaces.empty());
        if (br.point_replaces.empty()) {
            return hash_region_t<key_range_t>();
        }

        store_key_t minimum_key = store_key_t::max();
        store_key_t maximum_key = store_key_t::min();
        uint64_t minimum_hash_value = HASH_REGION_HASH_SIZE - 1;
        uint64_t maximum_hash_value = 0;

        for (auto pair = br.point_replaces.begin(); pair != br.point_replaces.end(); ++pair) {
            if (pair->second.key < minimum_key) {
                minimum_key = pair->second.key;
            }
            if (pair->second.key > maximum_key) {
                maximum_key = pair->second.key;
            }

            const uint64_t hash_value = hash_region_hasher(pair->second.key.contents(), pair->second.key.size());
            if (hash_value < minimum_hash_value) {
                minimum_hash_value = hash_value;
            }
            if (hash_value > maximum_hash_value) {
                maximum_hash_value = hash_value;
            }
        }

        return hash_region_t<key_range_t>(minimum_hash_value, maximum_hash_value + 1,
                                          key_range_t(key_range_t::closed, minimum_key, key_range_t::closed, maximum_key));
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
};

region_t write_t::get_region() const THROWS_NOTHING {
    return boost::apply_visitor(rdb_w_get_region_visitor(), write);
}

/* write_t::shard implementation */

struct rdb_w_shard_visitor_t : public boost::static_visitor<bool> {
    rdb_w_shard_visitor_t(const region_t *_region,
                          durability_requirement_t _durability_requirement,
                          write_t *_write_out)
        : region(_region),
          durability_requirement(_durability_requirement),
          write_out(_write_out) {}

    template <class T>
    bool keyed_write(const T &arg) const {
        if (region_contains_key(*region, arg.key)) {
            *write_out = write_t(arg, durability_requirement);
            return true;
        } else {
            return false;
        }
    }

    bool operator()(const point_replace_t &pr) const {
        return keyed_write(pr);
    }

    bool operator()(const batched_replaces_t &br) const {
        std::vector<std::pair<int64_t, point_replace_t> > sharded_replaces;

        for (auto it = br.point_replaces.begin(); it != br.point_replaces.end(); ++it) {
            if (region_contains_key(*region, it->second.key)) {
                sharded_replaces.push_back(*it);
            }
        }

        if (!sharded_replaces.empty()) {
            *write_out = write_t(batched_replaces_t(), durability_requirement);
            batched_replaces_t *batched = boost::get<batched_replaces_t>(&write_out->write);
            batched->point_replaces.swap(sharded_replaces);
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
            *write_out = write_t(tmp);
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

    const region_t *region;
    durability_requirement_t durability_requirement;
    write_t *write_out;
};

bool write_t::shard(const region_t &region,
                    write_t *write_out) const THROWS_NOTHING {
    const rdb_w_shard_visitor_t v(&region, durability_requirement, write_out);
    return boost::apply_visitor(v, write);
}

template <class T>
bool first_less(const std::pair<int64_t, T> &left, const std::pair<int64_t, T> &right) {
    return left.first < right.first;
}

struct rdb_w_unshard_visitor_t : public boost::static_visitor<void> {
    void operator()(const point_replace_t &) const { monokey_response(); }

    // The special case here is batched_replaces_response_t, which actually gets sharded into
    // multiple operations instead of getting sent unsplit in a single direction.
    void operator()(const batched_replaces_t &) const {
        std::vector<std::pair<int64_t, point_replace_response_t> > combined;

        for (size_t i = 0; i < count; ++i) {
            const batched_replaces_response_t *batched_response = boost::get<batched_replaces_response_t>(&responses[i].response);
            guarantee(batched_response != NULL, "unsharding nonhomogeneous responses");

            combined.insert(combined.end(),
                            batched_response->point_replace_responses.begin(),
                            batched_response->point_replace_responses.end());
        }

        std::sort(combined.begin(), combined.end(), first_less<point_replace_response_t>);

        *response_out = write_response_t(batched_replaces_response_t(combined));
    }

    void operator()(const point_write_t &) const { monokey_response(); }
    void operator()(const point_delete_t &) const { monokey_response(); }

    void operator()(const sindex_create_t &) const {
        *response_out = responses[0];
    }

    void operator()(const sindex_drop_t &) const {
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

void write_t::unshard(const write_response_t *responses, size_t count,
                      write_response_t *response_out, context_t *, signal_t *)
    const THROWS_NOTHING {
    const rdb_w_unshard_visitor_t visitor(responses, count, response_out);
    boost::apply_visitor(visitor, write);
}

store_t::store_t(serializer_t *serializer,
                 const std::string &perfmon_name,
                 int64_t cache_target,
                 bool create,
                 perfmon_collection_t *parent_perfmon_collection,
                 context_t *_ctx,
                 io_backender_t *io,
                 const base_path_t &base_path) :
    btree_store_t<rdb_protocol_t>(serializer, perfmon_name, cache_target,
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

    scoped_ptr_t<transaction_t> txn;
    scoped_ptr_t<real_superblock_t> superblock;
    acquire_superblock_for_read(rwi_read, &token_pair.main_read_token, &txn,
                                &superblock, &dummy_interruptor, false);

    scoped_ptr_t<buf_lock_t> sindex_block;
    acquire_sindex_block_for_read(&token_pair, txn.get(), &sindex_block,
                                  superblock->get_sindex_block_id(),
                                  &dummy_interruptor);

    std::map<std::string, secondary_index_t> sindexes;
    get_secondary_indexes(txn.get(), sindex_block.get(), &sindexes);

    std::set<std::string> sindexes_to_update;
    for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
        if (!it->second.post_construction_complete) {
            sindexes_to_update.insert(it->first);
        }
    }

    if (!sindexes_to_update.empty()) {
        rdb_protocol_details::bring_sindexes_up_to_date(sindexes_to_update, this,
                                                        sindex_block.get(), txn.get());
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
        rdb_get(get.key, btree, txn, superblock, res);
    }

    void operator()(const rget_read_t &rget) {
        if (rget.transform.size() != 0 || rget.terminal) {
            rassert(rget.optargs.size() != 0);
        }
        ql_env.global_optargs.init_optargs(rget.optargs);
        response->response = rget_read_response_t();
        rget_read_response_t *res =
            boost::get<rget_read_response_t>(&response->response);

        if (!rget.sindex) {
            // Normal rget
            rdb_rget_slice(btree, rget.region.inner, txn, superblock,
                    &ql_env, rget.transform, rget.terminal,
                    rget.sorting, res);
        } else {
            scoped_ptr_t<real_superblock_t> sindex_sb;
            std::vector<char> sindex_mapping_data;

            try {
                bool found = store->acquire_sindex_superblock_for_read(*rget.sindex,
                        superblock->get_sindex_block_id(), token_pair,
                        txn, &sindex_sb, &sindex_mapping_data, &interruptor);

                if (!found) {
                    res->result = ql::datum_exc_t(
                        ql::base_exc_t::GENERIC,
                        strprintf("Index `%s` was not found.",
                                  rget.sindex->c_str()));
                    return;
                }
            } catch (const sindex_not_post_constructed_exc_t &) {
                res->result = ql::datum_exc_t(
                    ql::base_exc_t::GENERIC,
                    strprintf("Index `%s` was accessed before "
                              "its construction was finished.",
                              rget.sindex->c_str()));
                return;
            }

            guarantee(rget.sindex_range, "If an rget has a sindex specified "
                      "it should also have a sindex_range.");
            guarantee(rget.sindex_region, "If an rget has a sindex specified "
                      "it should also have a sindex_region.");

            // This chunk of code puts together a filter so we can exclude any items
            //  that don't fall in the specified range.  Because the secondary index
            //  keys may have been truncated, we can't go by keys alone.  Therefore,
            //  we construct a filter function that ensures all returned items lie
            //  between sindex_start_value and sindex_end_value.
            ql::map_wire_func_t sindex_mapping;
            sindex_multi_bool_t multi_bool = MULTI;
            vector_read_stream_t read_stream(&sindex_mapping_data);
            int success = deserialize(&read_stream, &sindex_mapping);
            guarantee(success == ARCHIVE_SUCCESS, "Corrupted sindex description.");
            success = deserialize(&read_stream, &multi_bool);
            guarantee(success == ARCHIVE_SUCCESS, "Corrupted sindex description.");

            rdb_rget_secondary_slice(
                    store->get_sindex_slice(*rget.sindex),
                    *rget.sindex_range, //guaranteed present above
                    txn, sindex_sb.get(), &ql_env, rget.transform,
                    rget.terminal, rget.region.inner, rget.sorting,
                    sindex_mapping, multi_bool, res);
        }
    }

    void operator()(const distribution_read_t &dg) {
        response->response = distribution_read_response_t();
        distribution_read_response_t *res = boost::get<distribution_read_response_t>(&response->response);
        rdb_distribution_get(btree, dg.max_depth, dg.region.inner.left, txn, superblock, res);
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

        std::map<std::string, secondary_index_t> sindexes;
        store->get_sindexes(token_pair, txn, superblock, &sindexes, &interruptor);

        res->sindexes.reserve(sindexes.size());
        for (auto it = sindexes.begin(); it != sindexes.end(); ++it) {
            res->sindexes.push_back(it->first);
        }
    }

    rdb_read_visitor_t(btree_slice_t *_btree,
                       btree_store_t<rdb_protocol_t> *_store,
                       transaction_t *_txn,
                       superblock_t *_superblock,
                       read_token_pair_t *_token_pair,
                       rdb_protocol_t::context_t *ctx,
                       read_response_t *_response,
                       signal_t *_interruptor) :
        response(_response),
        btree(_btree),
        store(_store),
        txn(_txn),
        superblock(_superblock),
        token_pair(_token_pair),
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
               std::map<std::string, ql::wire_func_t>())
    { }

private:
    read_response_t *response;
    btree_slice_t *btree;
    btree_store_t<rdb_protocol_t> *store;
    transaction_t *txn;
    superblock_t *superblock;
    read_token_pair_t *token_pair;
    wait_any_t interruptor;
    ql::env_t ql_env;
};

void store_t::protocol_read(const read_t &read,
                            read_response_t *response,
                            btree_slice_t *btree,
                            transaction_t *txn,
                            superblock_t *superblock,
                            read_token_pair_t *token_pair,
                            signal_t *interruptor) {
    rdb_read_visitor_t v(
        btree, this, txn, superblock, token_pair, ctx, response, interruptor);
    boost::apply_visitor(v, read.read);
}

// TODO: get rid of this extra response_t copy on the stack
struct rdb_write_visitor_t : public boost::static_visitor<void> {
    void operator()(const point_replace_t &r) {
        try {
            ql_env.global_optargs.init_optargs(r.optargs);
        } catch (const interrupted_exc_t &) {
            // Clear the sindex_write_token because we didn't have a chance to use it
            token_pair->sindex_write_token.reset();
            throw;
        }

        response->response = point_replace_response_t();
        point_replace_response_t *res = boost::get<point_replace_response_t>(&response->response);
        // TODO: modify surrounding code so we can dump this const_cast.
        ql::map_wire_func_t *f = const_cast<ql::map_wire_func_t *>(&r.f);
        rdb_modification_report_t mod_report(r.key);
        rdb_replace(btree, timestamp, txn, superblock->get(),
                    r.primary_key, r.key, f,
                    r.return_vals ? RETURN_VALS : NO_RETURN_VALS,
                    &ql_env, res, &mod_report.info);

        update_sindexes(&mod_report);
    }

    void operator()(const batched_replaces_t &br) {
        response->response = batched_replaces_response_t();
        batched_replaces_response_t *res = boost::get<batched_replaces_response_t>(&response->response);

        rdb_modification_report_cb_t sindex_cb(store, token_pair, txn,
                                               (*superblock)->get_sindex_block_id(),
                                               auto_drainer_t::lock_t(&store->drainer));
        rdb_batched_replace(br.point_replaces, btree, timestamp, txn, superblock,
                            &ql_env, res, &sindex_cb);
    }

    void operator()(const point_write_t &w) {
        response->response = point_write_response_t();
        point_write_response_t *res = boost::get<point_write_response_t>(&response->response);

        rdb_modification_report_t mod_report(w.key);
        rdb_set(w.key, w.data, w.overwrite, btree, timestamp, txn, superblock->get(), res, &mod_report.info);

        update_sindexes(&mod_report);
    }

    void operator()(const point_delete_t &d) {
        response->response = point_delete_response_t();
        point_delete_response_t *res = boost::get<point_delete_response_t>(&response->response);

        rdb_modification_report_t mod_report(d.key);
        rdb_delete(d.key, btree, timestamp, txn, superblock->get(), res, &mod_report.info);

        update_sindexes(&mod_report);
    }

    void operator()(const sindex_create_t &c) {
        sindex_create_response_t res;

        write_message_t wm;
        wm << c.mapping;
        wm << c.multi;

        vector_stream_t stream;
        int write_res = send_write_message(&stream, &wm);
        guarantee(write_res == 0);

        scoped_ptr_t<buf_lock_t> sindex_block;
        res.success = store->add_sindex(
            token_pair,
            c.id,
            stream.vector(),
            txn,
            superblock->get(),
            &sindex_block,
            &interruptor);

        if (res.success) {
            std::set<std::string> sindexes;
            sindexes.insert(c.id);
            rdb_protocol_details::bring_sindexes_up_to_date(
                sindexes, store, sindex_block.get(), txn);
        }

        response->response = res;
    }

    void operator()(const sindex_drop_t &d) {
        sindex_drop_response_t res;
        value_sizer_t<rdb_value_t> sizer(txn->get_cache()->get_block_size());
        rdb_value_deleter_t deleter;

        res.success = store->drop_sindex(token_pair,
                                         d.id,
                                         txn,
                                         superblock->get(),
                                         &sizer,
                                         &deleter,
                                         &interruptor);

        response->response = res;
    }

    rdb_write_visitor_t(btree_slice_t *_btree,
                        btree_store_t<rdb_protocol_t> *_store,
                        transaction_t *_txn,
                        scoped_ptr_t<superblock_t> *_superblock,
                        write_token_pair_t *_token_pair,
                        repli_timestamp_t _timestamp,
                        rdb_protocol_t::context_t *ctx,
                        write_response_t *_response,
                        signal_t *_interruptor) :
        btree(_btree),
        store(_store),
        txn(_txn),
        response(_response),
        superblock(_superblock),
        token_pair(_token_pair),
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
               std::map<std::string, ql::wire_func_t>()),
        sindex_block_id((*superblock)->get_sindex_block_id())
    { }

private:
    void update_sindexes(const rdb_modification_report_t *mod_report) {
        scoped_ptr_t<buf_lock_t> sindex_block;
        // Don't allow interruption here, or we may end up with inconsistent data
        cond_t dummy_interruptor;
        store->acquire_sindex_block_for_write(token_pair, txn, &sindex_block,
                                              sindex_block_id, &dummy_interruptor);

        mutex_t::acq_t acq;
        store->lock_sindex_queue(sindex_block.get(), &acq);

        write_message_t wm;
        wm << rdb_sindex_change_t(*mod_report);
        store->sindex_queue_push(wm, &acq);

        sindex_access_vector_t sindexes;
        store->aquire_post_constructed_sindex_superblocks_for_write(sindex_block.get(), txn, &sindexes);
        rdb_update_sindexes(sindexes, mod_report, txn);
    }

    btree_slice_t *btree;
    btree_store_t<rdb_protocol_t> *store;
    transaction_t *txn;
    write_response_t *response;
    scoped_ptr_t<superblock_t> *superblock;
    write_token_pair_t *token_pair;
    repli_timestamp_t timestamp;
    wait_any_t interruptor;
    ql::env_t ql_env;
    block_id_t sindex_block_id;
};

void store_t::protocol_write(const write_t &write,
                             write_response_t *response,
                             transition_timestamp_t timestamp,
                             btree_slice_t *btree,
                             transaction_t *txn,
                             scoped_ptr_t<superblock_t> *superblock,
                             write_token_pair_t *token_pair,
                             signal_t *interruptor) {
    rdb_write_visitor_t v(btree, this, txn, superblock, token_pair,
            timestamp.to_repli_timestamp(), ctx, response, interruptor);
    boost::apply_visitor(v, write.write);
}

struct rdb_backfill_chunk_get_btree_repli_timestamp_visitor_t : public boost::static_visitor<repli_timestamp_t> {
    repli_timestamp_t operator()(const backfill_chunk_t::delete_key_t &del) {
        return del.recency;
    }

    repli_timestamp_t operator()(const backfill_chunk_t::delete_range_t &) {
        return repli_timestamp_t::invalid;
    }

    repli_timestamp_t operator()(const backfill_chunk_t::key_value_pair_t &kv) {
        return kv.backfill_atom.recency;
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

    void on_delete_range(const key_range_t &range, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        chunk_fun_cb->send_chunk(chunk_t::delete_range(region_t(range)), interruptor);
    }

    void on_deletion(const btree_key_t *key, repli_timestamp_t recency, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        chunk_fun_cb->send_chunk(chunk_t::delete_key(to_store_key(key), recency), interruptor);
    }

    void on_keyvalue(const rdb_backfill_atom_t &atom, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        chunk_fun_cb->send_chunk(chunk_t::set_key(atom), interruptor);
    }

    void on_sindexes(const std::map<std::string, secondary_index_t> &sindexes, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
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

static void call_rdb_backfill(int i, btree_slice_t *btree, const std::vector<std::pair<region_t, state_timestamp_t> > &regions,
        rdb_backfill_callback_t *callback, transaction_t *txn, superblock_t *superblock, buf_lock_t *sindex_block, backfill_progress_t *progress,
        signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    parallel_traversal_progress_t *p = new parallel_traversal_progress_t;
    scoped_ptr_t<traversal_progress_t> p_owned(p);
    progress->add_constituent(&p_owned);
    repli_timestamp_t timestamp = regions[i].second.to_repli_timestamp();
    try {
        rdb_backfill(btree, regions[i].first.inner, timestamp, callback, txn, superblock, sindex_block, p, interruptor);
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
                                     transaction_t *txn,
                                     backfill_progress_t *progress,
                                     signal_t *interruptor)
                                     THROWS_ONLY(interrupted_exc_t) {
    rdb_backfill_callback_impl_t callback(chunk_fun_cb);
    std::vector<std::pair<region_t, state_timestamp_t> > regions(start_point.begin(), start_point.end());
    refcount_superblock_t refcount_wrapper(superblock, regions.size());
    pmap(regions.size(), boost::bind(&call_rdb_backfill, _1,
        btree, regions, &callback, txn, &refcount_wrapper, sindex_block, progress, interruptor));

    /* If interruptor was pulsed, `call_rdb_backfill()` exited silently, so we
    have to check directly. */
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
}

struct rdb_receive_backfill_visitor_t : public boost::static_visitor<void> {
    rdb_receive_backfill_visitor_t(btree_store_t<rdb_protocol_t> *_store,
                                   btree_slice_t *_btree,
                                   transaction_t *_txn,
                                   superblock_t *_superblock,
                                   write_token_pair_t *_token_pair,
                                   signal_t *_interruptor) :
      store(_store), btree(_btree), txn(_txn), superblock(_superblock),
      token_pair(_token_pair), interruptor(_interruptor),
      sindex_block_id(superblock->get_sindex_block_id()) { }

    void operator()(const backfill_chunk_t::delete_key_t& delete_key) const {
        point_delete_response_t response;
        rdb_modification_report_t mod_report(delete_key.key);
        rdb_delete(delete_key.key, btree, delete_key.recency,
                   txn, superblock, &response, &mod_report.info);

        update_sindexes(&mod_report);
    }

    void operator()(const backfill_chunk_t::delete_range_t& delete_range) const {
        range_key_tester_t tester(&delete_range.range);
        rdb_erase_range(btree, &tester, delete_range.range.inner, txn, superblock,
                store, token_pair, interruptor);
    }

    void operator()(const backfill_chunk_t::key_value_pair_t& kv) const {
        const rdb_backfill_atom_t& bf_atom = kv.backfill_atom;
        point_write_response_t response;
        rdb_modification_report_t mod_report(bf_atom.key);
        rdb_set(bf_atom.key, bf_atom.value, true,
                btree, bf_atom.recency,
                txn, superblock, &response,
                &mod_report.info);

        update_sindexes(&mod_report);
    }

    void operator()(const backfill_chunk_t::sindexes_t &s) const {
        value_sizer_t<rdb_value_t> sizer(txn->get_cache()->get_block_size());
        rdb_value_deleter_t deleter;
        scoped_ptr_t<buf_lock_t> sindex_block;
        std::set<std::string> created_sindexes;
        store->set_sindexes(token_pair, s.sindexes, txn, superblock, &sizer, &deleter, &sindex_block, &created_sindexes, interruptor);

        if (!created_sindexes.empty()) {
            sindex_access_vector_t sindexes;
            store->acquire_sindex_superblocks_for_write(
                    created_sindexes,
                    sindex_block.get(),
                    txn,
                    &sindexes);

            rdb_protocol_details::bring_sindexes_up_to_date(created_sindexes, store,
                    sindex_block.get(), txn);
        }
    }

private:
    void update_sindexes(rdb_modification_report_t *mod_report) const {
        scoped_ptr_t<buf_lock_t> sindex_block;
        // Don't allow interruption here, or we may end up with inconsistent data
        cond_t dummy_interruptor;
        store->acquire_sindex_block_for_write(
            token_pair, txn, &sindex_block,
            sindex_block_id, &dummy_interruptor);

        mutex_t::acq_t acq;
        store->lock_sindex_queue(sindex_block.get(), &acq);

        write_message_t wm;
        wm << rdb_sindex_change_t(*mod_report);
        store->sindex_queue_push(wm, &acq);

        sindex_access_vector_t sindexes;
        store->aquire_post_constructed_sindex_superblocks_for_write(
                sindex_block.get(), txn, &sindexes);
        rdb_update_sindexes(sindexes, mod_report, txn);
    }

    btree_store_t<rdb_protocol_t> *store;
    btree_slice_t *btree;
    transaction_t *txn;
    superblock_t *superblock;
    write_token_pair_t *token_pair;
    signal_t *interruptor;  // FIXME: interruptors are not used in btree code, so this one ignored.
    block_id_t sindex_block_id;
};

void store_t::protocol_receive_backfill(btree_slice_t *btree,
                                        transaction_t *txn,
                                        superblock_t *superblock,
                                        write_token_pair_t *token_pair,
                                        signal_t *interruptor,
                                        const backfill_chunk_t &chunk) {
    boost::apply_visitor(rdb_receive_backfill_visitor_t(this, btree, txn, superblock, token_pair, interruptor), chunk.val);
}

void store_t::protocol_reset_data(const region_t& subregion,
                                  btree_slice_t *btree,
                                  transaction_t *txn,
                                  superblock_t *superblock,
                                  write_token_pair_t *token_pair,
                                  signal_t *interruptor) {
    value_sizer_t<rdb_value_t> sizer(txn->get_cache()->get_block_size());
    rdb_value_deleter_t deleter;

    always_true_key_tester_t key_tester;
    rdb_erase_range(btree, &key_tester, subregion.inner, txn, superblock, this, token_pair, interruptor);
}

region_t rdb_protocol_t::cpu_sharding_subspace(int subregion_number,
                                               int num_cpu_shards) {
    guarantee(subregion_number >= 0);
    guarantee(subregion_number < num_cpu_shards);

    // We have to be careful with the math here, to avoid overflow.
    uint64_t width = HASH_REGION_HASH_SIZE / num_cpu_shards;

    uint64_t beg = width * subregion_number;
    uint64_t end = subregion_number + 1 == num_cpu_shards ? HASH_REGION_HASH_SIZE : beg + width;

    return region_t(beg, end, key_range_t::universe());
}

hash_region_t<key_range_t> sindex_range_t::to_region() const {
    return hash_region_t<key_range_t>(rdb_protocol_t::sindex_key_range(
        start != NULL ? start->truncated_secondary() : store_key_t::min(),
        end != NULL ? end->truncated_secondary() : store_key_t::max()));
}

bool sindex_range_t::contains(counted_t<const ql::datum_t> value) const {
    return (!start || (*start < *value || (*start == *value && !start_open))) &&
           (!end   || (*value < *end   || (*value == *end && !end_open)));
}

RDB_IMPL_ME_SERIALIZABLE_3(rdb_protocol_details::rget_item_t, key, sindex_key, data);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::point_read_response_t, data);
RDB_IMPL_ME_SERIALIZABLE_4(rdb_protocol_t::rget_read_response_t,
                           result, key_range, truncated, last_considered_key);
RDB_IMPL_ME_SERIALIZABLE_2(rdb_protocol_t::distribution_read_response_t, region, key_counts);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::sindex_list_response_t, sindexes);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::read_response_t, response);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::point_read_t, key);

RDB_IMPL_ME_SERIALIZABLE_4(sindex_range_t,
                           empty_ok(start), empty_ok(end), start_open, end_open);
RDB_IMPL_ME_SERIALIZABLE_8(rdb_protocol_t::rget_read_t, region, sindex,
                           sindex_region, sindex_range,
                           transform, terminal, optargs, sorting);

RDB_IMPL_ME_SERIALIZABLE_3(rdb_protocol_t::distribution_read_t, max_depth, result_limit, region);
RDB_IMPL_ME_SERIALIZABLE_0(rdb_protocol_t::sindex_list_t);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::read_t, read);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::point_write_response_t, result);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::point_delete_response_t, result);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::sindex_create_response_t, success);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::sindex_drop_response_t, success);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::write_response_t, response);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::batched_replaces_response_t, point_replace_responses);


RDB_IMPL_ME_SERIALIZABLE_5(rdb_protocol_t::point_replace_t, primary_key, key, f, optargs, return_vals);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::batched_replaces_t, point_replaces);
RDB_IMPL_ME_SERIALIZABLE_3(rdb_protocol_t::point_write_t, key, data, overwrite);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::point_delete_t, key);

RDB_IMPL_ME_SERIALIZABLE_4(rdb_protocol_t::sindex_create_t, id, mapping, region, multi);
RDB_IMPL_ME_SERIALIZABLE_2(rdb_protocol_t::sindex_drop_t, id, region);

RDB_IMPL_ME_SERIALIZABLE_2(rdb_protocol_t::write_t, write, durability_requirement);
RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::backfill_chunk_t::delete_key_t, key);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::backfill_chunk_t::delete_range_t, range);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::backfill_chunk_t::key_value_pair_t, backfill_atom);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::backfill_chunk_t::sindexes_t, sindexes);

RDB_IMPL_ME_SERIALIZABLE_1(rdb_protocol_t::backfill_chunk_t, val);
