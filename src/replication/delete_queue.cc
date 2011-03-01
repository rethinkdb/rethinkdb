#include "replication/delete_queue.hpp"

namespace replication {

namespace delete_queue {

const int TIMESTAMPS_AND_OFFSETS_OFFSET = sizeof(off64_t);
const int TIMESTAMPS_AND_OFFSETS_SIZE = sizeof(large_buf_ref) + 3 * sizeof(block_id_t);

off64_t primal_offset(const void *root_buffer) {
    return *reinterpret_cast<off64_t *>(root_buffer);
}

large_buf_ref *timestamps_and_offsets_largebuf(void *root_buffer) {
    const char *p = reinterpret_cast<char *>(root_buffer);
    return reinterpret_cast<large_buf_ref *>(p + TIMESTAMPS_AND_OFFSETS_OFFSET);
}

large_buf_ref *keys_largebuf(void *root_buffer) {
    const char *p = reinterpret_cast<char *>(root_buffer);
    return reinterpret_cast<large_buf_ref *>(p + (TIMESTAMPS_AND_OFFSETS_OFFSET + TIMESTAMPS_AND_OFFSETS_SIZE));
}

// TODO: Add test doublechecking that sizeof(t_and_o) == 12.
struct t_and_o {
    repli_timestamp timestamp;
    off64_t offset;
} __attribute__((__packed__));

}  // namespace delete_queue



void add_key_to_delete_queue(transaction_t *txn, block_id_t queue_root_id, repli_timestamp timestamp, const store_key_t *key) {
    buf_lock_t queue_root(txn, queue_root_id, rwi_write);
    void *queue_root_buf = const_cast<void *>(queue_root.buf()->get_data_read());

    large_buf_ref *t_o_ref = delete_queue::timestamps_and_offsets_largebuf(queue_root_buf);

    rassert(t_o_ref->size % sizeof(t_and_o) == 0);

    // TODO: Why must we allocate large_buf_t's with new?
    large_buf_lock_t t_o_largebuf(new large_buf_t(txn));

    // TODO: Allow upgrade of large buf intent.
    co_acquire_large_value(t_o_largebuf.lv(), t_o_ref, lbref_limit_t(TIMESTAMPS_AND_OFFSETS_SIZE), rwi_write);

    // TODO: Implement the rest of this function.




}

void dump_keys_from_delete_queue(transaction_t *txn, block_id_t queue_root, repli_timestamp begin_timestamp, repli_timestamp end_timestamp, deletion_key_receiver_t *recipient) {

    // TODO: Implement this.


}



}  // namespace replication
