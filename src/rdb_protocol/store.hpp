#ifndef RDB_PROTOCOL_STORE_HPP_
#define RDB_PROTOCOL_STORE_HPP_

#include "rdb_protocol/btree_store.hpp"

class store_t : public btree_store_t {
public:
    store_t(serializer_t *serializer,
            cache_balancer_t *balancer,
            const std::string &perfmon_name,
            bool create,
            perfmon_collection_t *parent_perfmon_collection,
            rdb_context_t *ctx,
            io_backender_t *io,
            const base_path_t &base_path);
    ~store_t();

private:
    friend struct read_visitor_t;
    void protocol_read(const read_t &read,
                       read_response_t *response,
                       btree_slice_t *btree,
                       superblock_t *superblock,
                       signal_t *interruptor);

    friend struct write_visitor_t;
    void protocol_write(const write_t &write,
                        write_response_t *response,
                        transition_timestamp_t timestamp,
                        btree_slice_t *btree,
                        scoped_ptr_t<superblock_t> *superblock,
                        signal_t *interruptor);

    void protocol_send_backfill(const region_map_t<state_timestamp_t> &start_point,
                                chunk_fun_callback_t *chunk_fun_cb,
                                superblock_t *superblock,
                                buf_lock_t *sindex_block,
                                btree_slice_t *btree,
                                rdb_protocol_t::backfill_progress_t *progress,
                                signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

    void protocol_receive_backfill(btree_slice_t *btree,
                                   scoped_ptr_t<superblock_t> &&superblock,
                                   signal_t *interruptor,
                                   const backfill_chunk_t &chunk);

    void protocol_reset_data(const region_t& subregion,
                             btree_slice_t *btree,
                             superblock_t *superblock,
                             signal_t *interruptor);
    rdb_context_t *ctx;
};



#endif  // RDB_PROTOCOL_STORE_HPP_
