#ifndef BTREE_BTREE_STORE_HPP_
#define BTREE_BTREE_STORE_HPP_

#include "protocol_api.hpp"
#include "btree/slice.hpp"
#include "btree/operations.hpp"
#include "perfmon/perfmon.hpp"

class io_backender_t;

template <class protocol_t>
class btree_store_t : public store_view_t<protocol_t> {
private:
    boost::scoped_ptr<standard_serializer_t> serializer;
    mirrored_cache_config_t cache_dynamic_config;
    boost::scoped_ptr<cache_t> cache;
    boost::scoped_ptr<btree_slice_t> btree;
    order_source_t order_source;

    fifo_enforcer_source_t token_source;
    fifo_enforcer_sink_t token_sink;

public:
    btree_store_t(io_backender_t *io_backender,
                  const std::string& filename,
                  bool create,
                  perfmon_collection_t *parent_perfmon_collection);
    ~btree_store_t();

    /* store_view_t interface */
    void new_read_token(scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *token_out);
    void new_write_token(scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *token_out);

    typedef region_map_t<protocol_t, binary_blob_t> metainfo_t;

    metainfo_t get_metainfo(
            order_token_t order_token,
            scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void set_metainfo(
            const metainfo_t &new_metainfo,
            order_token_t order_token,
            scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    typename protocol_t::read_response_t read(
            DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
            const typename protocol_t::read_t &read,
            order_token_t order_token,
            scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    typename protocol_t::write_response_t write(
            DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
            const metainfo_t& new_metainfo,
            const typename protocol_t::write_t &write,
            transition_timestamp_t timestamp,
            order_token_t order_token,
            scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    bool send_backfill(
            const region_map_t<protocol_t, state_timestamp_t> &start_point,
            const boost::function<bool(const metainfo_t&)> &should_backfill,
            const boost::function<void(typename protocol_t::backfill_chunk_t)> &chunk_fun,
            typename protocol_t::backfill_progress_t *progress,
            scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void receive_backfill(
            const typename protocol_t::backfill_chunk_t &chunk,
            scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void reset_data(
            const typename protocol_t::region_t &subregion,
            const metainfo_t &new_metainfo,
            scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *token,
            signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

protected:
    // Functions to be implemented by derived (protocol-specific) store_t classes
    virtual typename protocol_t::read_response_t protocol_read(const typename protocol_t::read_t &read,
                                                               btree_slice_t *btree,
                                                               transaction_t *txn,
                                                               superblock_t *superblock) = 0;

    virtual typename protocol_t::write_response_t protocol_write(const typename protocol_t::write_t &write,
                                                                 transition_timestamp_t timestamp,
                                                                 btree_slice_t *btree,
                                                                 transaction_t *txn,
                                                                 superblock_t *superblock) = 0;

    virtual void protocol_send_backfill(const region_map_t<protocol_t, state_timestamp_t> &start_point,
                                        const boost::function<void(typename protocol_t::backfill_chunk_t)> &chunk_fun,
                                        superblock_t *superblock,
                                        btree_slice_t *btree,
                                        transaction_t *txn,
                                        typename protocol_t::backfill_progress_t *progress) = 0;

    virtual void protocol_receive_backfill(btree_slice_t *btree,
                                           transaction_t *txn,
                                           superblock_t *superblock,
                                           signal_t *interruptor,
                                           const typename protocol_t::backfill_chunk_t &chunk) = 0;

    virtual void protocol_reset_data(const typename protocol_t::region_t& subregion,
                                     btree_slice_t *btree,
                                     transaction_t *txn,
                                     superblock_t *superblock) = 0;

    class refcount_superblock_t : public superblock_t {
    public:
        refcount_superblock_t(superblock_t *sb, int rc) :
            sub_superblock(sb), refcount(rc) { }

        void release() {
            refcount--;
            rassert(refcount >= 0);
            if (refcount == 0) {
                sub_superblock->release();
                sub_superblock = NULL;
            }
        }

        block_id_t get_root_block_id() const {
            return sub_superblock->get_root_block_id();
        }

        void set_root_block_id(const block_id_t new_root_block) {
            sub_superblock->set_root_block_id(new_root_block);
        }

        block_id_t get_stat_block_id() const {
            return sub_superblock->get_stat_block_id();
        }

        void set_stat_block_id(block_id_t new_stat_block) {
            sub_superblock->set_stat_block_id(new_stat_block);
        }

        void set_eviction_priority(eviction_priority_t eviction_priority) {
            sub_superblock->set_eviction_priority(eviction_priority);
        }

        eviction_priority_t get_eviction_priority() {
            return sub_superblock->get_eviction_priority();
        }

    private:
        superblock_t *sub_superblock;
        int refcount;
    };

private:
    region_map_t<protocol_t, binary_blob_t> get_metainfo_internal(transaction_t* txn, buf_lock_t* sb_buf) const THROWS_NOTHING;

    void acquire_superblock_for_read(
            access_t access,
            scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *token,
            boost::scoped_ptr<transaction_t> &txn_out,
            boost::scoped_ptr<real_superblock_t> &sb_out,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);

    void acquire_superblock_for_backfill(
            scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *token,
            boost::scoped_ptr<transaction_t> &txn_out,
            boost::scoped_ptr<real_superblock_t> &sb_out,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);

    void acquire_superblock_for_write(
            access_t access,
            int expected_change_count,
            scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *token,
            boost::scoped_ptr<transaction_t> &txn_out,
            boost::scoped_ptr<real_superblock_t> &sb_out,
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

    perfmon_collection_t perfmon_collection;
    perfmon_membership_t perfmon_collection_membership;
};

#include "btree/btree_store.tcc"

#endif  // BTREE_BTREE_STORE_HPP_
