// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_STORE_HPP_
#define RDB_PROTOCOL_STORE_HPP_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "btree/node.hpp"
#include "btree/parallel_traversal.hpp"
#include "btree/secondary_operations.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/queue/disk_backed_queue_wrapper.hpp"
#include "concurrency/new_mutex.hpp"
#include "concurrency/new_semaphore.hpp"
#include "concurrency/rwlock.hpp"
#include "containers/map_sentries.hpp"
#include "containers/scoped.hpp"
#include "perfmon/perfmon.hpp"
#include "protocol_api.hpp"
#include "rdb_protocol/changefeed.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/store_metainfo.hpp"
#include "rpc/mailbox/typed.hpp"
#include "store_view.hpp"
#include "utils.hpp"

class store_t;
class btree_slice_t;
class cache_conn_t;
class cache_t;
class internal_disk_backed_queue_t;
class io_backender_t;
class real_superblock_t;
class sindex_superblock_t;
class superblock_t;
class txn_t;
class cache_balancer_t;
struct rdb_modification_report_t;

class sindex_not_ready_exc_t : public std::exception {
public:
    explicit sindex_not_ready_exc_t(std::string sindex_name,
                                    const secondary_index_t &sindex,
                                    const std::string &table_name);
    const char* what() const throw();
    ~sindex_not_ready_exc_t() throw();
protected:
    std::string info;
};

class deletion_context_t {
public:
    virtual ~deletion_context_t() { }

    // Used by btree balancing operations to detach values
    virtual const value_deleter_t *balancing_detacher() const = 0;

    // Applied when deleting or erasing a value from a leaf node
    // (in either secondary index trees or the primary btree)
    virtual const value_deleter_t *in_tree_deleter() const = 0;

    // Applied after value_deleter() has been applied to all indexes that
    // reference a value
    virtual const value_deleter_t *post_deleter() const = 0;
};

enum class update_sindexes_t {
    UPDATE,
    LEAVE_ALONE
};

class store_t final : public store_view_t {
public:
    using home_thread_mixin_t::assert_thread;

    store_t(const region_t &region,
            serializer_t *serializer,
            cache_balancer_t *balancer,
            const std::string &perfmon_name,
            bool create,
            perfmon_collection_t *parent_perfmon_collection,
            rdb_context_t *_ctx,
            io_backender_t *io_backender,
            const base_path_t &base_path,
            namespace_id_t table_id,
            update_sindexes_t update_sindexes);
    ~store_t();

    void note_reshard();

    /* store_view_t interface */

    void new_read_token(read_token_t *token_out);
    void new_write_token(write_token_t *token_out);

    region_map_t<binary_blob_t> get_metainfo(
            order_token_t order_token,
            read_token_t *token,
            const region_t &region,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void set_metainfo(
            const region_map_t<binary_blob_t> &new_metainfo,
            order_token_t order_token,
            write_token_t *token,
            write_durability_t durability,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    cluster_version_t metainfo_version(read_token_t *token,
                                       signal_t *interruptor);

    void migrate_metainfo(
            order_token_t order_token,
            write_token_t *token,
            cluster_version_t from,
            cluster_version_t to,
            const std::function<binary_blob_t(const region_t &,
                                              const binary_blob_t &)> &cb,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void read(
            DEBUG_ONLY(const metainfo_checker_t& metainfo_checker, )
            const read_t &read,
            read_response_t *response,
            read_token_t *token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void write(
            DEBUG_ONLY(const metainfo_checker_t& metainfo_checker, )
            const region_map_t<binary_blob_t>& new_metainfo,
            const write_t &write,
            write_response_t *response,
            write_durability_t durability,
            state_timestamp_t timestamp,
            order_token_t order_token,
            write_token_t *token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    continue_bool_t send_backfill_pre(
            const region_map_t<state_timestamp_t> &start_point,
            backfill_pre_item_consumer_t *pre_item_consumer,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);
    continue_bool_t send_backfill(
            const region_map_t<state_timestamp_t> &start_point,
            backfill_pre_item_producer_t *pre_item_producer,
            backfill_item_consumer_t *item_consumer,
            backfill_item_memory_tracker_t *memory_tracker,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);
    continue_bool_t receive_backfill(
            const region_t &region,
            backfill_item_producer_t *item_producer,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void wait_until_ok_to_receive_backfill(signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);
    bool check_ok_to_receive_backfill() THROWS_NOTHING;

    void reset_data(
            const binary_blob_t &zero_version,
            const region_t &subregion,
            write_durability_t durability,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    /* End of `store_view_t` interface */

    std::map<std::string, std::pair<sindex_config_t, sindex_status_t> > sindex_list(
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);

    /* Warning: If the index already exists, this function will crash. Make sure that
    you don't run multiple instances of this for the same index at the same time. */
    void sindex_create(
            const std::string &name,
            const sindex_config_t &config,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);

    void sindex_rename_multi(
            const std::map<std::string, std::string> &name_changes,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);

    void sindex_drop(
            const std::string &id,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);

    new_mutex_in_line_t get_in_line_for_sindex_queue(buf_lock_t *sindex_block);
    rwlock_in_line_t get_in_line_for_cfeed_stamp(access_t access);

    void register_sindex_queue(
            disk_backed_queue_wrapper_t<rdb_modification_report_t> *disk_backed_queue,
            const key_range_t &construction_range,
            const new_mutex_in_line_t *acq);

    void deregister_sindex_queue(
            disk_backed_queue_wrapper_t<rdb_modification_report_t> *disk_backed_queue,
            const new_mutex_in_line_t *acq);

    void emergency_deregister_sindex_queue(
            disk_backed_queue_wrapper_t<rdb_modification_report_t> *disk_backed_queue);

    // Updates the live sindexes, and pushes modification reports onto the sindex
    // queues of non-live indexes.
    void update_sindexes(
            txn_t *txn,
            buf_lock_t *sindex_block,
            const std::vector<rdb_modification_report_t> &mod_reports,
            bool release_sindex_block);

    void sindex_queue_push(
            const rdb_modification_report_t &mod_report,
            const new_mutex_in_line_t *acq);
    void sindex_queue_push(
            const std::vector<rdb_modification_report_t> &mod_reports,
            const new_mutex_in_line_t *acq);

    // Returns the UUID of the created index, or boost::none if an index by `name`
    // already existed.
    MUST_USE boost::optional<uuid_u> add_sindex_internal(
        const sindex_name_t &name,
        const std::vector<char> &opaque_definition,
        buf_lock_t *sindex_block);

    std::map<sindex_name_t, secondary_index_t> get_sindexes() const;

    bool mark_index_up_to_date(
        uuid_u id,
        buf_lock_t *sindex_block,
        const key_range_t &except_for_remaining_range)
    THROWS_NOTHING;

    MUST_USE bool acquire_sindex_superblock_for_read(
            const sindex_name_t &name,
            const std::string &table_name,
            real_superblock_t *superblock,  // releases this.
            scoped_ptr_t<sindex_superblock_t> *sindex_sb_out,
            std::vector<char> *opaque_definition_out,
            uuid_u *sindex_uuid_out)
        THROWS_ONLY(sindex_not_ready_exc_t);

    MUST_USE bool acquire_sindex_superblock_for_write(
            const sindex_name_t &name,
            const std::string &table_name,
            real_superblock_t *superblock,  // releases this.
            scoped_ptr_t<sindex_superblock_t> *sindex_sb_out,
            uuid_u *sindex_uuid_out)
        THROWS_ONLY(sindex_not_ready_exc_t);

    struct sindex_access_t {
        sindex_access_t(btree_slice_t *_btree,
                        sindex_name_t _name,
                        secondary_index_t _sindex,
                        scoped_ptr_t<sindex_superblock_t> _superblock);
        ~sindex_access_t();

        btree_slice_t *btree;
        sindex_name_t name;
        secondary_index_t sindex;
        scoped_ptr_t<sindex_superblock_t> superblock;
    };

    typedef std::vector<scoped_ptr_t<sindex_access_t> > sindex_access_vector_t;

    void acquire_all_sindex_superblocks_for_write(
            block_id_t sindex_block_id,
            buf_parent_t parent,
            sindex_access_vector_t *sindex_sbs_out)
        THROWS_ONLY(sindex_not_ready_exc_t);

    void acquire_all_sindex_superblocks_for_write(
            buf_lock_t *sindex_block,
            sindex_access_vector_t *sindex_sbs_out)
        THROWS_ONLY(sindex_not_ready_exc_t);

    bool acquire_sindex_superblocks_for_write(
            boost::optional<std::set<uuid_u> > sindexes_to_acquire, //none means acquire all sindexes
            buf_lock_t *sindex_block,
            sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_ready_exc_t);

    btree_slice_t *get_sindex_slice(const uuid_u &id) {
        return secondary_index_slices.at(id).get();
    }

    void protocol_read(const read_t &read,
                       read_response_t *response,
                       real_superblock_t *superblock,
                       signal_t *interruptor);

    void protocol_write(const write_t &write,
                        write_response_t *response,
                        state_timestamp_t timestamp,
                        scoped_ptr_t<real_superblock_t> *superblock,
                        signal_t *interruptor);

    void acquire_superblock_for_read(
            read_token_t *token,
            scoped_ptr_t<txn_t> *txn_out,
            scoped_ptr_t<real_superblock_t> *sb_out,
            signal_t *interruptor,
            bool use_snapshot)
            THROWS_ONLY(interrupted_exc_t);

    void acquire_superblock_for_write(
            int expected_change_count,
            write_durability_t durability,
            write_token_t *token,
            scoped_ptr_t<txn_t> *txn_out,
            scoped_ptr_t<real_superblock_t> *sb_out,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);

    // Used by `delayed_clear_and_drop_sindex` and during index post-construction.
    // Clears out a slice of a secondary index.
    void clear_sindex_data(
            uuid_u sindex_id,
            value_sizer_t *sizer,
            const deletion_context_t *deletion_context,
            const key_range_t &pkey_range_to_clear,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);

private:
    // Helper function to clear out a secondary index that has been
    // marked as deleted and drop it at the end. To be run in a coroutine.
    void delayed_clear_and_drop_sindex(
            secondary_index_t sindex,
            auto_drainer_t::lock_t store_keepalive)
            THROWS_NOTHING;
    // Drops a secondary index. Assumes that the index has previously been cleared
    // through `clear_sindex_data()`.
    void drop_sindex(uuid_u sindex_id) THROWS_NOTHING;

    // Resumes post construction for partially constructed indexes.  Resumes deleting
    // deleted indexes.  Also migrates the secondary index block to the current version.
    void help_construct_bring_sindexes_up_to_date();

    MUST_USE bool mark_secondary_index_deleted(
            buf_lock_t *sindex_block,
            const sindex_name_t &name);

public:
    namespace_id_t const &get_table_id() const;

    // The `double` is the progress of the secondary index construction.
    typedef std::map<uuid_u, std::pair<microtime_t, double const *> >
        sindex_context_map_t;
    sindex_context_map_t *get_sindex_context_map();

    double get_sindex_progress(uuid_u const &id);
    microtime_t get_sindex_start_time(uuid_u const &id);

    fifo_enforcer_source_t main_token_source, sindex_token_source;
    fifo_enforcer_sink_t main_token_sink, sindex_token_sink;

    perfmon_collection_t perfmon_collection;
    // Mind the constructor ordering. We must destruct the cache and btree
    // before we destruct perfmon_collection
    scoped_ptr_t<cache_t> cache;
    scoped_ptr_t<cache_conn_t> general_cache_conn;
    scoped_ptr_t<btree_slice_t> btree;
    io_backender_t *io_backender_;
    base_path_t base_path_;
    perfmon_membership_t perfmon_collection_membership;
    scoped_ptr_t<store_metainfo_manager_t> metainfo;

    std::map<uuid_u, scoped_ptr_t<btree_slice_t> > secondary_index_slices;

    // We construct secondary indexes by starting with a `universe()` construction_range,
    // and then making the range increasingly smaller until it is `empty()`.
    // While we are in that process, we must put any write for a primary key that is in
    // `construction_range` into the associated secondary index queue. Any other write
    // must be applied directly to the secondary index.
    struct ranged_sindex_queue_t {
        // Once the index has been fully constructed, `construction_range` will be empty.
        key_range_t construction_range;
        disk_backed_queue_wrapper_t<rdb_modification_report_t> *queue;
    };
    std::vector<ranged_sindex_queue_t> sindex_queues;
    new_mutex_t sindex_queue_mutex;
    // Used to control access to stamps.  We need this so that `do_stamp` in
    // `store.cc` can synchronize with the `rdb_modification_report_cb_t` in
    // `btree.cc`.
    rwlock_t cfeed_stamp_lock;

private:
    rdb_context_t *ctx;
    // We store regions here even though we only really need the key ranges
    // because it's nice to have a unique identifier across `store_t`s.  In the
    // future we may use these `region_t`s instead of the `uuid_u`s in the
    // changefeed server.
    std::map<region_t, scoped_ptr_t<ql::changefeed::server_t> > changefeed_servers;
    rwlock_t changefeed_servers_lock;

    std::pair<ql::changefeed::server_t *, auto_drainer_t::lock_t> changefeed_server(
            const region_t &region,
            const rwlock_acq_t *acq);
public:
    // Returns a pointer to `changefeed_servers` together with a read acquisition
    // on `changefeed_servers_lock`.
    std::pair<const std::map<region_t, scoped_ptr_t<ql::changefeed::server_t> > *,
              scoped_ptr_t<rwlock_acq_t> > access_changefeed_servers();
    // Return a pointer to a specific changefeed server if it exists. These can
    // block.
    std::pair<ql::changefeed::server_t *, auto_drainer_t::lock_t> changefeed_server(
            const region_t &region);
    std::pair<ql::changefeed::server_t *, auto_drainer_t::lock_t> changefeed_server(
            const store_key_t &key);
    // Like `changefeed_server()`, but creates the server if it doesn't exist.
    std::pair<ql::changefeed::server_t *, auto_drainer_t::lock_t>
            get_or_make_changefeed_server(const region_t &region);

private:
    namespace_id_t table_id;

    sindex_context_map_t sindex_context;

    // Having a lot of writes queued up waiting for the superblock to become available
    // can stall reads for unacceptably long time periods.
    // We use this semaphore to limit the number of writes that can be in line for a
    // superblock acquisition at a time (including the write that's currently holding
    // the superblock, if any).
    new_semaphore_t write_superblock_acq_semaphore;

public:
    // This lock is used to pause backfills while secondary indexes are being
    // post constructed. Secondary index post construction gets in line for a write
    // lock on this and stays there for as long as it's running. It does not
    // actually wait for the write lock, so multiple secondary indexes can be
    // post constructed at the same time.
    // A read lock is acquired before a backfill chunk is being processed.
    rwlock_t backfill_postcon_lock;

    // Mind the constructor ordering. We must destruct drainer before destructing
    // many of the other structures.
    auto_drainer_t drainer;

private:
    DISABLE_COPYING(store_t);
};

#endif  // RDB_PROTOCOL_STORE_HPP_
