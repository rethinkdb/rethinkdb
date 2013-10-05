// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef BTREE_BTREE_STORE_HPP_
#define BTREE_BTREE_STORE_HPP_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "btree/erase_range.hpp"
#include "btree/secondary_operations.hpp"
#include "buffer_cache/mirrored/config.hpp"  // TODO: Move to buffer_cache/config.hpp or something.
#include "buffer_cache/types.hpp"
#include "concurrency/auto_drainer.hpp"
#include "containers/disk_backed_queue.hpp"
#include "perfmon/perfmon.hpp"
#include "protocol_api.hpp"

struct rdb_protocol_t;
template <class T> class btree_store_t;

class btree_slice_t;
class io_backender_t;
class superblock_t;
class real_superblock_t;

class sindex_not_post_constructed_exc_t : public std::exception {
public:
    explicit sindex_not_post_constructed_exc_t(std::string sindex_name);
    const char* what() const throw();
    ~sindex_not_post_constructed_exc_t() throw();
private:
    std::string info;
};

template <class protocol_t>
class btree_store_t : public store_view_t<protocol_t> {
public:
    using home_thread_mixin_t::assert_thread;

    btree_store_t(serializer_t *serializer,
                  const std::string &perfmon_name,
                  int64_t cache_target,
                  bool create,
                  perfmon_collection_t *parent_perfmon_collection,
                  typename protocol_t::context_t *,
                  io_backender_t *io_backender,
                  const base_path_t &base_path);
    virtual ~btree_store_t();

    /* store_view_t interface */
    void new_read_token(object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token_out);
    void new_write_token(object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token_out);

    /* These functions get tokens for both the main B-Tree and the secondary
     * structures. */
    void new_read_token_pair(read_token_pair_t *token_pair_out);
    void new_write_token_pair(write_token_pair_t *token_pair_out);

    typedef region_map_t<protocol_t, binary_blob_t> metainfo_t;

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
            DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
            const typename protocol_t::read_t &read,
            typename protocol_t::read_response_t *response,
            order_token_t order_token,
            read_token_pair_t *token_pair,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void write(
            DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
            const metainfo_t& new_metainfo,
            const typename protocol_t::write_t &write,
            typename protocol_t::write_response_t *response,
            write_durability_t durability,
            transition_timestamp_t timestamp,
            order_token_t order_token,
            write_token_pair_t *token_pair,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    bool send_backfill(
            const region_map_t<protocol_t, state_timestamp_t> &start_point,
            send_backfill_callback_t<protocol_t> *send_backfill_cb,
            typename protocol_t::backfill_progress_t *progress,
            read_token_pair_t *token_pair,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void receive_backfill(
            const typename protocol_t::backfill_chunk_t &chunk,
            write_token_pair_t *token_pair,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void reset_data(
            const typename protocol_t::region_t &subregion,
            const metainfo_t &new_metainfo,
            write_token_pair_t *token_pair,
            write_durability_t durability,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void lock_sindex_queue(buf_lock_t *sindex_block, mutex_t::acq_t *acq);

    void register_sindex_queue(
            internal_disk_backed_queue_t *disk_backed_queue,
            const mutex_t::acq_t *acq);

    void deregister_sindex_queue(
            internal_disk_backed_queue_t *disk_backed_queue,
            const mutex_t::acq_t *acq);

    void emergency_deregister_sindex_queue(
            internal_disk_backed_queue_t *disk_backed_queue);

    void sindex_queue_push(
            const write_message_t& value,
            const mutex_t::acq_t *acq);

    void acquire_sindex_block_for_read(
            read_token_pair_t *token_pair,
            transaction_t *txn,
            scoped_ptr_t<buf_lock_t> *sindex_block_out,
            block_id_t sindex_block_id,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void acquire_sindex_block_for_write(
            write_token_pair_t *token_pair,
            transaction_t *txn,
            scoped_ptr_t<buf_lock_t> *sindex_block_out,
            block_id_t sindex_block_id,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    MUST_USE bool add_sindex(
        write_token_pair_t *token_pair,
        const std::string &id,
        const secondary_index_t::opaque_definition_t &definition,
        transaction_t *txn,
        superblock_t *super_block,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

    MUST_USE bool add_sindex(
        write_token_pair_t *token_pair,
        const std::string &id,
        const secondary_index_t::opaque_definition_t &definition,
        transaction_t *txn,
        superblock_t *super_block,
        scoped_ptr_t<buf_lock_t> *sindex_block_out,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

    void set_sindexes(
        write_token_pair_t *token_pair,
        const std::map<std::string, secondary_index_t> &sindexes,
        transaction_t *txn,
        superblock_t *superblock,
        value_sizer_t<void> *sizer,
        value_deleter_t *deleter,
        scoped_ptr_t<buf_lock_t> *sindex_block_out,
        std::set<std::string> *created_sindexes_out,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

    bool mark_index_up_to_date(
        const std::string &id,
        transaction_t *txn,
        buf_lock_t *sindex_block)
    THROWS_NOTHING;

    bool mark_index_up_to_date(
        uuid_u id,
        transaction_t *txn,
        buf_lock_t *sindex_block)
    THROWS_NOTHING;

    bool drop_sindex(
        write_token_pair_t *token_pair,
        const std::string &id,
        transaction_t *txn,
        superblock_t *super_block,
        value_sizer_t<void> *sizer,
        value_deleter_t *deleter,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

    void drop_all_sindexes(
        write_token_pair_t *token_pair,
        transaction_t *txn,
        superblock_t *super_block,
        value_sizer_t<void> *sizer,
        value_deleter_t *deleter,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

    void get_sindexes(
        read_token_pair_t *token_pair,
        transaction_t *txn,
        superblock_t *super_block,
        std::map<std::string, secondary_index_t> *sindexes_out,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

    void get_sindexes(
        buf_lock_t *sindex_block,
        transaction_t *txn,
        std::map<std::string, secondary_index_t> *sindexes_out)
    THROWS_NOTHING;

    MUST_USE bool acquire_sindex_superblock_for_read(
            const std::string &id,
            block_id_t sindex_block_id,
            read_token_pair_t *token_pair,
            transaction_t *txn_out,
            scoped_ptr_t<real_superblock_t> *sindex_sb_out,
            std::vector<char> *opaque_definition_out, // Optional, may be NULL
            signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, sindex_not_post_constructed_exc_t);

    MUST_USE bool acquire_sindex_superblock_for_write(
            const std::string &id,
            block_id_t sindex_block_id,
            write_token_pair_t *token_pair,
            transaction_t *txn,
            scoped_ptr_t<real_superblock_t> *sindex_sb_out,
            signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, sindex_not_post_constructed_exc_t);

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
            write_token_pair_t *token_pair,
            transaction_t *txn,
            sindex_access_vector_t *sindex_sbs_out,
            signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, sindex_not_post_constructed_exc_t);

    void acquire_all_sindex_superblocks_for_write(
            buf_lock_t *sindex_block,
            transaction_t *txn,
            sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_post_constructed_exc_t);

    void aquire_post_constructed_sindex_superblocks_for_write(
            block_id_t sindex_block_id,
            write_token_pair_t *token_pair,
            transaction_t *txn,
            sindex_access_vector_t *sindex_sbs_out,
            signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

    void aquire_post_constructed_sindex_superblocks_for_write(
            buf_lock_t *sindex_block,
            transaction_t *txn,
            sindex_access_vector_t *sindex_sbs_out)
    THROWS_NOTHING;

    bool acquire_sindex_superblocks_for_write(
            boost::optional<std::set<std::string> > sindexes_to_acquire, //none means acquire all sindexes
            buf_lock_t *sindex_block,
            transaction_t *txn,
            sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_post_constructed_exc_t);

    bool acquire_sindex_superblocks_for_write(
            boost::optional<std::set<uuid_u> > sindexes_to_acquire, //none means acquire all sindexes
            buf_lock_t *sindex_block,
            transaction_t *txn,
            sindex_access_vector_t *sindex_sbs_out)
    THROWS_ONLY(sindex_not_post_constructed_exc_t);

    btree_slice_t *get_sindex_slice(std::string id) {
        return &(secondary_index_slices.at(id));
    }

    virtual void protocol_read(const typename protocol_t::read_t &read,
                               typename protocol_t::read_response_t *response,
                               btree_slice_t *btree,
                               transaction_t *txn,
                               superblock_t *superblock,
                               read_token_pair_t *token_pair,
                               signal_t *interruptor) = 0;

    virtual void protocol_write(const typename protocol_t::write_t &write,
                                typename protocol_t::write_response_t *response,
                                transition_timestamp_t timestamp,
                                btree_slice_t *btree,
                                transaction_t *txn,
                                scoped_ptr_t<superblock_t> *superblock,
                                write_token_pair_t *token_pair,
                                signal_t *interruptor) = 0;

    virtual void protocol_send_backfill(const region_map_t<protocol_t, state_timestamp_t> &start_point,
                                        chunk_fun_callback_t<protocol_t> *chunk_fun_cb,
                                        superblock_t *superblock,
                                        buf_lock_t *sindex_block,
                                        btree_slice_t *btree,
                                        transaction_t *txn,
                                        typename protocol_t::backfill_progress_t *progress,
                                        signal_t *interruptor)
                                        THROWS_ONLY(interrupted_exc_t) = 0;

    virtual void protocol_receive_backfill(btree_slice_t *btree,
                                           transaction_t *txn,
                                           superblock_t *superblock,
                                           write_token_pair_t *token_pair,
                                           signal_t *interruptor,
                                           const typename protocol_t::backfill_chunk_t &chunk) = 0;

    virtual void protocol_reset_data(const typename protocol_t::region_t& subregion,
                                     btree_slice_t *btree,
                                     transaction_t *txn,
                                     superblock_t *superblock,
                                     write_token_pair_t *token_pair,
                                     signal_t *interruptor) = 0;

    void get_metainfo_internal(transaction_t* txn, buf_lock_t* sb_buf, region_map_t<protocol_t, binary_blob_t> *out) const THROWS_NOTHING;

    void acquire_superblock_for_read(
            access_t access,
            object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token,
            scoped_ptr_t<transaction_t> *txn_out,
            scoped_ptr_t<real_superblock_t> *sb_out,
            signal_t *interruptor,
            bool use_snapshot)
            THROWS_ONLY(interrupted_exc_t);

    void acquire_superblock_for_backfill(
            object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token,
            scoped_ptr_t<transaction_t> *txn_out,
            scoped_ptr_t<real_superblock_t> *sb_out,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);

    void acquire_superblock_for_write(
            access_t access,
            repli_timestamp_t timestamp,
            int expected_change_count,
            write_durability_t durability,
            write_token_pair_t *token_pair,
            scoped_ptr_t<transaction_t> *txn_out,
            scoped_ptr_t<real_superblock_t> *sb_out,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);
private:
    void acquire_superblock_for_write(
            access_t access,
            repli_timestamp_t timestamp,
            int expected_change_count,
            write_durability_t durability,
            object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token,
            scoped_ptr_t<transaction_t> *txn_out,
            scoped_ptr_t<real_superblock_t> *sb_out,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);

public:
    void check_and_update_metainfo(
        DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
        const metainfo_t &new_metainfo,
        transaction_t *txn,
        real_superblock_t *superblock) const
        THROWS_NOTHING;

    metainfo_t check_metainfo(
        DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
        transaction_t *txn,
        real_superblock_t *superblock) const
        THROWS_NOTHING;

    void update_metainfo(const metainfo_t &old_metainfo, const metainfo_t &new_metainfo, transaction_t *txn, real_superblock_t *superbloc) const THROWS_NOTHING;

    mirrored_cache_config_t cache_dynamic_config;
    order_source_t order_source;

    fifo_enforcer_source_t main_token_source, sindex_token_source;
    fifo_enforcer_sink_t main_token_sink, sindex_token_sink;

    perfmon_collection_t perfmon_collection;
    scoped_ptr_t<cache_t> cache;
    scoped_ptr_t<btree_slice_t> btree;
    io_backender_t *io_backender_;
    base_path_t base_path_;
    perfmon_membership_t perfmon_collection_membership;

    boost::ptr_map<const std::string, btree_slice_t> secondary_index_slices;

    std::vector<internal_disk_backed_queue_t *> sindex_queues;
    mutex_t sindex_queue_mutex;

    auto_drainer_t drainer;

private:
    DISABLE_COPYING(btree_store_t);
};

#endif  // BTREE_BTREE_STORE_HPP_
