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
#include "concurrency/new_mutex.hpp"
#include "concurrency/new_semaphore.hpp"
#include "concurrency/rwlock.hpp"
#include "containers/map_sentries.hpp"
#include "containers/scoped.hpp"
#include "perfmon/perfmon.hpp"
#include "protocol_api.hpp"
#include "rdb_protocol/changefeed.hpp"
#include "rdb_protocol/protocol.hpp"
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

class outdated_index_report_t {
public:
    outdated_index_report_t() { }
    virtual ~outdated_index_report_t() { }

    // Called during store_t instantiation
    virtual void set_outdated_indexes(std::set<std::string> &&indexes) = 0;

    // Called when indexes change during store_t lifetime
    virtual void index_dropped(const std::string &index_name) = 0;
    virtual void index_renamed(const std::string &old_name,
                               const std::string &new_name) = 0;
};

class store_t final : public store_view_t {
public:
    using home_thread_mixin_t::assert_thread;

    store_t(serializer_t *serializer,
            cache_balancer_t *balancer,
            const std::string &perfmon_name,
            bool create,
            perfmon_collection_t *parent_perfmon_collection,
            rdb_context_t *_ctx,
            io_backender_t *io_backender,
            const base_path_t &base_path,
            scoped_ptr_t<outdated_index_report_t> &&_index_report,
            namespace_id_t table_id);
    ~store_t();

    void note_reshard();

    /* store_view_t interface */
    void new_read_token(read_token_t *token_out);
    void new_write_token(write_token_t *token_out);

    void do_get_metainfo(
            order_token_t order_token,
            read_token_t *token,
            signal_t *interruptor,
            region_map_t<binary_blob_t> *out)
        THROWS_ONLY(interrupted_exc_t);

    void set_metainfo(
            const region_map_t<binary_blob_t> &new_metainfo,
            order_token_t order_token,
            write_token_t *token,
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

    bool send_backfill(
            const region_map_t<state_timestamp_t> &start_point,
            send_backfill_callback_t *send_backfill_cb,
            traversal_progress_combiner_t *progress,
            read_token_t *token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void receive_backfill(
            const backfill_chunk_t &chunk,
            write_token_t *token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    // Preserves ordering
    void throttle_backfill_chunk(signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void reset_data(
            const binary_blob_t &zero_version,
            const region_t &subregion,
            write_durability_t durability,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    new_mutex_in_line_t get_in_line_for_sindex_queue(buf_lock_t *sindex_block);

    void register_sindex_queue(
            internal_disk_backed_queue_t *disk_backed_queue,
            const new_mutex_in_line_t *acq);

    void deregister_sindex_queue(
            internal_disk_backed_queue_t *disk_backed_queue,
            const new_mutex_in_line_t *acq);

    void emergency_deregister_sindex_queue(
            internal_disk_backed_queue_t *disk_backed_queue);

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

    MUST_USE bool add_sindex(
        const sindex_name_t &name,
        const std::vector<char> &opaque_definition,
        buf_lock_t *sindex_block);

    std::map<sindex_name_t, secondary_index_t> get_sindexes() const;

    void set_sindexes(
        const std::map<sindex_name_t, secondary_index_t> &sindexes,
        buf_lock_t *sindex_block,
        std::set<sindex_name_t> *created_sindexes_out)
    THROWS_ONLY(interrupted_exc_t);

    bool mark_index_up_to_date(
        const sindex_name_t &name,
        buf_lock_t *sindex_block)
    THROWS_NOTHING;

    bool mark_index_up_to_date(
        uuid_u id,
        buf_lock_t *sindex_block)
    THROWS_NOTHING;

    bool drop_sindex(
        const sindex_name_t &name,
        buf_lock_t *sindex_block)
    THROWS_ONLY(interrupted_exc_t);

    void rename_sindex(
        const sindex_name_t &old_name,
        const sindex_name_t &new_name,
        buf_lock_t *sindex_block)
    THROWS_ONLY(interrupted_exc_t);

    void update_outdated_sindex_list(buf_lock_t *sindex_block);

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

    void acquire_post_constructed_sindex_superblocks_for_write(
            buf_lock_t *sindex_block,
            sindex_access_vector_t *sindex_sbs_out)
    THROWS_NOTHING;

    bool acquire_sindex_superblocks_for_write(
            boost::optional<std::set<sindex_name_t> > sindexes_to_acquire, //none means acquire all sindexes
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

    void protocol_send_backfill(const region_map_t<state_timestamp_t> &start_point,
                                chunk_fun_callback_t *chunk_fun_cb,
                                real_superblock_t *superblock,
                                buf_lock_t *sindex_block,
                                traversal_progress_combiner_t *progress,
                                signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void protocol_receive_backfill(scoped_ptr_t<real_superblock_t> &&superblock,
                                   signal_t *interruptor,
                                   const backfill_chunk_t &chunk);

    void get_metainfo_internal(real_superblock_t *superblock,
                               region_map_t<binary_blob_t> *out)
        const THROWS_NOTHING;

    void acquire_superblock_for_read(
            read_token_t *token,
            scoped_ptr_t<txn_t> *txn_out,
            scoped_ptr_t<real_superblock_t> *sb_out,
            signal_t *interruptor,
            bool use_snapshot)
            THROWS_ONLY(interrupted_exc_t);

    void acquire_superblock_for_backfill(
            read_token_t *token,
            scoped_ptr_t<txn_t> *txn_out,
            scoped_ptr_t<real_superblock_t> *sb_out,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);

    void acquire_superblock_for_write(
            int expected_change_count,
            write_durability_t durability,
            write_token_t *token,
            scoped_ptr_t<txn_t> *txn_out,
            scoped_ptr_t<real_superblock_t> *sb_out,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);

private:
    // Drops all sindexes if this store is not responsible for any data
    void maybe_drop_all_sindexes(const binary_blob_t &zero_metainfo,
                                 const write_durability_t durability,
                                 signal_t *interruptor);

    // Helper function to clear out a secondary index that has been
    // marked as deleted. To be run in a coroutine.
    void delayed_clear_sindex(
            secondary_index_t sindex,
            auto_drainer_t::lock_t store_keepalive)
            THROWS_NOTHING;
    // Internally called by `delayed_clear_sindex()`
    void clear_sindex(
            secondary_index_t sindex,
            value_sizer_t *sizer,
            const deletion_context_t *deletion_context,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);

    void help_construct_bring_sindexes_up_to_date();

    MUST_USE bool mark_secondary_index_deleted(
            buf_lock_t *sindex_block,
            const sindex_name_t &name);

public:
    void check_and_update_metainfo(
        DEBUG_ONLY(const metainfo_checker_t &metainfo_checker, )
        const region_map_t<binary_blob_t> &new_metainfo,
        real_superblock_t *superblock) const
        THROWS_NOTHING;

    region_map_t<binary_blob_t> check_metainfo(
        DEBUG_ONLY(const metainfo_checker_t &metainfo_checker, )
        real_superblock_t *superblock) const
        THROWS_NOTHING;

    void update_metainfo(const region_map_t<binary_blob_t> &old_metainfo,
                         const region_map_t<binary_blob_t> &new_metainfo,
                         real_superblock_t *superblock) const THROWS_NOTHING;

    namespace_id_t const &get_table_id() const;

    typedef std::map<
        uuid_u, std::pair<microtime_t, parallel_traversal_progress_t const *>
    > sindex_context_map_t;
    sindex_context_map_t *get_sindex_context_map();

    progress_completion_fraction_t get_sindex_progress(uuid_u const &id);
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

    std::map<uuid_u, scoped_ptr_t<btree_slice_t> > secondary_index_slices;

    std::vector<internal_disk_backed_queue_t *> sindex_queues;
    new_mutex_t sindex_queue_mutex;

    rdb_context_t *ctx;
    scoped_ptr_t<ql::changefeed::server_t> changefeed_server;

    // This report is used by the outdated index issue tracker, and should be updated
    // any time the set of outdated indexes for this table changes
    scoped_ptr_t<outdated_index_report_t> index_report;

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
