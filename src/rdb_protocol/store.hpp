// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_STORE_HPP_
#define RDB_PROTOCOL_STORE_HPP_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "btree/erase_range.hpp"
#include "btree/operations.hpp"
#include "btree/parallel_traversal.hpp"
#include "btree/secondary_operations.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/new_mutex.hpp"
#include "containers/map_sentries.hpp"
#include "containers/scoped.hpp"
#include "perfmon/perfmon.hpp"
#include "protocol_api.hpp"
#include "rdb_protocol/protocol.hpp"
#include "store_view.hpp"
#include "utils.hpp"

class store_t;
class btree_slice_t;
class cache_conn_t;
class cache_t;
class internal_disk_backed_queue_t;
class io_backender_t;
class real_superblock_t;
class superblock_t;
class txn_t;
class cache_balancer_t;

class sindex_not_post_constructed_exc_t : public std::exception {
public:
    explicit sindex_not_post_constructed_exc_t(
        const std::string &sindex_name, const std::string &table_name);
    const char* what() const throw();
    ~sindex_not_post_constructed_exc_t() throw();
private:
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

class store_t : public store_view_t {
public:
    using home_thread_mixin_t::assert_thread;

    store_t(serializer_t *serializer,
            cache_balancer_t *balancer,
            const std::string &perfmon_name,
            bool create,
            perfmon_collection_t *parent_perfmon_collection,
            rdb_context_t *,
            io_backender_t *io_backender,
            const base_path_t &base_path);
    ~store_t();

    /* store_view_t interface */
    void new_read_token(object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token_out);
    void new_write_token(object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token_out);

    void do_get_metainfo(
            order_token_t order_token,
            object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token,
            signal_t *interruptor,
            metainfo_t *out)
        THROWS_ONLY(interrupted_exc_t);

    void set_metainfo(
            const metainfo_t &new_metainfo,
            order_token_t order_token,
            object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void read(
            DEBUG_ONLY(const metainfo_checker_t& metainfo_checker, )
            const read_t &read,
            read_response_t *response,
            order_token_t order_token,
            read_token_pair_t *token_pair,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void write(
            DEBUG_ONLY(const metainfo_checker_t& metainfo_checker, )
            const metainfo_t& new_metainfo,
            const write_t &write,
            write_response_t *response,
            write_durability_t durability,
            transition_timestamp_t timestamp,
            order_token_t order_token,
            write_token_pair_t *token_pair,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    bool send_backfill(
            const region_map_t<state_timestamp_t> &start_point,
            send_backfill_callback_t *send_backfill_cb,
            traversal_progress_combiner_t *progress,
            read_token_pair_t *token_pair,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void receive_backfill(
            const backfill_chunk_t &chunk,
            write_token_pair_t *token_pair,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void reset_data(
            const region_t &subregion,
            const metainfo_t &new_metainfo,
            write_token_pair_t *token_pair,
            write_durability_t durability,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    scoped_ptr_t<new_mutex_in_line_t> get_in_line_for_sindex_queue(
            buf_lock_t *sindex_block);

    void register_sindex_queue(
            internal_disk_backed_queue_t *disk_backed_queue,
            const new_mutex_in_line_t *acq);

    void deregister_sindex_queue(
            internal_disk_backed_queue_t *disk_backed_queue,
            const new_mutex_in_line_t *acq);

    void emergency_deregister_sindex_queue(
            internal_disk_backed_queue_t *disk_backed_queue);

    void sindex_queue_push(
            const write_message_t &value,
            const new_mutex_in_line_t *acq);
    void sindex_queue_push(
            const scoped_array_t<write_message_t> &values,
            const new_mutex_in_line_t *acq);

    void add_progress_tracker(
        map_insertion_sentry_t<uuid_u, const parallel_traversal_progress_t *> *sentry,
        uuid_u id, const parallel_traversal_progress_t *p);

    progress_completion_fraction_t get_progress(uuid_u id);

    MUST_USE buf_lock_t acquire_sindex_block_for_read(
            buf_parent_t parent,
            block_id_t sindex_block_id);

    MUST_USE buf_lock_t acquire_sindex_block_for_write(
            buf_parent_t parent,
            block_id_t sindex_block_id);

    MUST_USE bool add_sindex(
        const std::string &id,
        const secondary_index_t::opaque_definition_t &definition,
        buf_lock_t *sindex_block);

    void set_sindexes(
        const std::map<std::string, secondary_index_t> &sindexes,
        buf_lock_t *sindex_block,
        value_sizer_t *sizer,
        const deletion_context_t *live_deletion_context,
        const deletion_context_t *post_construction_deletion_context,
        std::set<std::string> *created_sindexes_out,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

    bool mark_index_up_to_date(
        const std::string &id,
        buf_lock_t *sindex_block)
    THROWS_NOTHING;

    bool mark_index_up_to_date(
        uuid_u id,
        buf_lock_t *sindex_block)
    THROWS_NOTHING;

    bool drop_sindex(
        const std::string &id,
        buf_lock_t *sindex_block,
        value_sizer_t *sizer,
        const deletion_context_t *live_deletion_context,
        const deletion_context_t *post_construction_deletion_context,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

    MUST_USE bool acquire_sindex_superblock_for_read(
            const std::string &id,
            const std::string &table_name,
            superblock_t *superblock,  // releases this.
            scoped_ptr_t<real_superblock_t> *sindex_sb_out,
            std::vector<char> *opaque_definition_out) // Optional, may be NULL
        THROWS_ONLY(sindex_not_post_constructed_exc_t);

    MUST_USE bool acquire_sindex_superblock_for_write(
            const std::string &id,
            const std::string &table_name,
            superblock_t *superblock,  // releases this.
            scoped_ptr_t<real_superblock_t> *sindex_sb_out)
        THROWS_ONLY(sindex_not_post_constructed_exc_t);

    struct sindex_access_t {
        sindex_access_t(btree_slice_t *_btree, secondary_index_t _sindex,
                real_superblock_t *_super_block)
            : btree(_btree), sindex(_sindex),
              super_block(_super_block)
        { }

        btree_slice_t *btree;
        secondary_index_t sindex;
        scoped_ptr_t<real_superblock_t> super_block;
    };

    typedef boost::ptr_vector<sindex_access_t> sindex_access_vector_t;

    void acquire_all_sindex_superblocks_for_write(
            block_id_t sindex_block_id,
            buf_parent_t parent,
            sindex_access_vector_t *sindex_sbs_out)
        THROWS_ONLY(sindex_not_post_constructed_exc_t);

    void acquire_all_sindex_superblocks_for_write(
            buf_lock_t *sindex_block,
            sindex_access_vector_t *sindex_sbs_out)
        THROWS_ONLY(sindex_not_post_constructed_exc_t);

    void acquire_post_constructed_sindex_superblocks_for_write(
            buf_lock_t *sindex_block,
            sindex_access_vector_t *sindex_sbs_out)
    THROWS_NOTHING;

    bool acquire_sindex_superblocks_for_write(
            boost::optional<std::set<std::string> > sindexes_to_acquire, //none means acquire all sindexes
            buf_lock_t *sindex_block,
            sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_post_constructed_exc_t);

    bool acquire_sindex_superblocks_for_write(
            boost::optional<std::set<uuid_u> > sindexes_to_acquire, //none means acquire all sindexes
            buf_lock_t *sindex_block,
            sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_post_constructed_exc_t);

    btree_slice_t *get_sindex_slice(std::string id) {
        return &(secondary_index_slices.at(id));
    }

    void protocol_read(const read_t &read,
                       read_response_t *response,
                       superblock_t *superblock,
                       signal_t *interruptor);

    void protocol_write(const write_t &write,
                        write_response_t *response,
                        transition_timestamp_t timestamp,
                        scoped_ptr_t<superblock_t> *superblock,
                        signal_t *interruptor);

    void protocol_send_backfill(const region_map_t<state_timestamp_t> &start_point,
                                chunk_fun_callback_t *chunk_fun_cb,
                                superblock_t *superblock,
                                buf_lock_t *sindex_block,
                                traversal_progress_combiner_t *progress,
                                signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void protocol_receive_backfill(scoped_ptr_t<superblock_t> &&superblock,
                                   signal_t *interruptor,
                                   const backfill_chunk_t &chunk);

    void protocol_reset_data(const region_t &subregion,
                             superblock_t *superblock,
                             signal_t *interruptor);

    void get_metainfo_internal(buf_lock_t *sb_buf,
                               region_map_t<binary_blob_t> *out)
        const THROWS_NOTHING;

    void acquire_superblock_for_read(
            object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token,
            scoped_ptr_t<txn_t> *txn_out,
            scoped_ptr_t<real_superblock_t> *sb_out,
            signal_t *interruptor,
            bool use_snapshot)
            THROWS_ONLY(interrupted_exc_t);

    void acquire_superblock_for_backfill(
            object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token,
            scoped_ptr_t<txn_t> *txn_out,
            scoped_ptr_t<real_superblock_t> *sb_out,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);

    void acquire_superblock_for_write(
            repli_timestamp_t timestamp,
            int expected_change_count,
            write_durability_t durability,
            write_token_pair_t *token_pair,
            scoped_ptr_t<txn_t> *txn_out,
            scoped_ptr_t<real_superblock_t> *sb_out,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);

private:
    void help_construct_bring_sindexes_up_to_date();

    void acquire_superblock_for_write(
            repli_timestamp_t timestamp,
            int expected_change_count,
            write_durability_t durability,
            object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token,
            scoped_ptr_t<txn_t> *txn_out,
            scoped_ptr_t<real_superblock_t> *sb_out,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);

public:
    void check_and_update_metainfo(
        DEBUG_ONLY(const metainfo_checker_t &metainfo_checker, )
        const metainfo_t &new_metainfo,
        real_superblock_t *superblock) const
        THROWS_NOTHING;

    metainfo_t check_metainfo(
        DEBUG_ONLY(const metainfo_checker_t &metainfo_checker, )
        real_superblock_t *superblock) const
        THROWS_NOTHING;

    void update_metainfo(const metainfo_t &old_metainfo,
                         const metainfo_t &new_metainfo,
                         real_superblock_t *superblock) const THROWS_NOTHING;

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

    boost::ptr_map<const std::string, btree_slice_t> secondary_index_slices;

    std::vector<internal_disk_backed_queue_t *> sindex_queues;
    new_mutex_t sindex_queue_mutex;
    std::map<uuid_u, const parallel_traversal_progress_t *> progress_trackers;

    // Mind the constructor ordering. We must destruct drainer before destructing
    // many of the other structures.
    auto_drainer_t drainer;

    rdb_context_t *ctx;

private:
    DISABLE_COPYING(store_t);
};

#endif  // RDB_PROTOCOL_STORE_HPP_
