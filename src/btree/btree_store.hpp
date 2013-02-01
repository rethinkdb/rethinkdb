// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef BTREE_BTREE_STORE_HPP_
#define BTREE_BTREE_STORE_HPP_

#include <string>

#include "errors.hpp"
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "btree/erase_range.hpp"
#include "btree/operations.hpp"
#include "btree/secondary_operations.hpp"
#include "buffer_cache/mirrored/config.hpp"  // TODO: Move to buffer_cache/config.hpp or something.
#include "buffer_cache/types.hpp"
#include "perfmon/perfmon.hpp"
#include "protocol_api.hpp"

namespace unittest {
// A forward decleration of this is needed so that it can be friended and the
// unit test can access the private API of btree_store_t.
void run_sindex_btree_store_api_test();
} //namespace unittest

class btree_slice_t;
class io_backender_t;
class superblock_t;
class real_superblock_t;

template <class protocol_t>
class btree_store_t : public store_view_t<protocol_t> {
public:
    using home_thread_mixin_t::assert_thread;

    btree_store_t(serializer_t *serializer,
                  const std::string &perfmon_name,
                  int64_t cache_target,
                  bool create,
                  perfmon_collection_t *parent_perfmon_collection,
                  typename protocol_t::context_t *);
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
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

private:

void acquire_sindex_block_for_read(
        read_token_pair_t *token_pair,
        transaction_t *txn,
        scoped_ptr_t<buf_lock_t> *sindex_block_out,
        block_id_t sindex_block_id,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

/* Why are there two versions of acquire_superblock_for_write? The prior first
 * is to be used if you want to make a change to the sindexes right after
 * acquiring the superblock. This happens if you're creating a new sindex for
 * example. That later is for when you want to make a change having already
 * descended the tree. They are only superficial different mostly for
 * convenience. If you were to extract the sindex_block_id from the
 * superblock_t in the first method and pass it to the second you would get the
 * same results, there mostly for convenience. The same thing doesn't exist for
 * acquire_sindex_block_for_read because I can't actually think of a case where
 * people should be travering the primary B-Tree and then traversing the
 * secondaries so I can't think of a use case for it. */
void acquire_sindex_block_for_write(
        write_token_pair_t *token_pair,
        transaction_t *txn,
        scoped_ptr_t<buf_lock_t> *sindex_block_out,
        const superblock_t *super_block,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

void acquire_sindex_block_for_write(
        write_token_pair_t *token_pair,
        transaction_t *txn,
        scoped_ptr_t<buf_lock_t> *sindex_block_out,
        block_id_t sindex_block_id,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

public: // <--- so this is some bullshit right here
    friend void unittest::run_sindex_btree_store_api_test();

    void add_sindex(
        write_token_pair_t *token_pair,
        uuid_u id,
        const secondary_index_t::opaque_definition_t &definition,
        transaction_t *txn,
        superblock_t *super_block,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

    void set_sindexes(
        write_token_pair_t *token_pair,
        const std::map<uuid_u, secondary_index_t> &sindexes,
        transaction_t *txn,
        superblock_t *super_block,
        value_sizer_t<void> *sizer,
        value_deleter_t *deleter,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

    void mark_index_up_to_date(
        write_token_pair_t *token_pair,
        uuid_u id,
        transaction_t *txn,
        superblock_t *super_block,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

    void drop_sindex(
        write_token_pair_t *token_pair,
        uuid_u id,
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
        std::map<uuid_u, secondary_index_t> *sindexes_out,
        read_token_pair_t *token_pair,
        transaction_t *txn,
        superblock_t *super_block,
        signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

    void acquire_sindex_superblock_for_read(
            uuid_u id,
            block_id_t sindex_block_id,
            read_token_pair_t *token_pair,
            transaction_t *txn_out,
            scoped_ptr_t<real_superblock_t> *sindex_sb_out,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);

    void acquire_sindex_superblock_for_write(
            uuid_u id,
            block_id_t sindex_block_id,
            write_token_pair_t *token_pair,
            transaction_t *txn,
            scoped_ptr_t<real_superblock_t> *sindex_sb_out,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);

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
            THROWS_ONLY(interrupted_exc_t);

    btree_slice_t *get_sindex_slice(uuid_u id) {
        return &(secondary_index_slices.at(id));
    }

protected:
    // Functions to be implemented by derived (protocol-specific) store_t classes
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
                                superblock_t *superblock,
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
                                     write_token_pair_t *token_pair) = 0;

private:
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
            object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token,
            scoped_ptr_t<transaction_t> *txn_out,
            scoped_ptr_t<real_superblock_t> *sb_out,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);

    void check_and_update_metainfo(
        DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
        const metainfo_t &new_metainfo,
        transaction_t *txn,
        real_superblock_t *superbloc) const
        THROWS_NOTHING;

    metainfo_t check_metainfo(
        DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
        transaction_t *txn,
        real_superblock_t *superbloc) const
        THROWS_NOTHING;

    void update_metainfo(const metainfo_t &old_metainfo, const metainfo_t &new_metainfo, transaction_t *txn, real_superblock_t *superbloc) const THROWS_NOTHING;

    mirrored_cache_config_t cache_dynamic_config;
    order_source_t order_source;

    fifo_enforcer_source_t main_token_source, sindex_token_source;
    fifo_enforcer_sink_t main_token_sink, sindex_token_sink;

    perfmon_collection_t perfmon_collection;
    scoped_ptr_t<cache_t> cache;
    scoped_ptr_t<btree_slice_t> btree;
    perfmon_membership_t perfmon_collection_membership;

    boost::ptr_map<uuid_u, btree_slice_t> secondary_index_slices;

    DISABLE_COPYING(btree_store_t);
};

#endif  // BTREE_BTREE_STORE_HPP_
