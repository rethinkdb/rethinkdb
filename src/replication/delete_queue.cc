#include "replication/delete_queue.hpp"

#include "buffer_cache/buf_lock.hpp"
#include "buffer_cache/co_functions.hpp"
#include "logger.hpp"
#include "scoped_malloc.hpp"
#include "store.hpp"

namespace replication {

namespace delete_queue {

const int TIMESTAMPS_AND_OFFSETS_OFFSET = sizeof(off64_t);
const int TIMESTAMPS_AND_OFFSETS_SIZE = sizeof(large_buf_ref) + 3 * sizeof(block_id_t);

off64_t primal_offset(void *root_buffer) {
    return *reinterpret_cast<off64_t *>(root_buffer);
}

large_buf_ref *timestamps_and_offsets_largebuf(void *root_buffer) {
    char *p = reinterpret_cast<char *>(root_buffer);
    return reinterpret_cast<large_buf_ref *>(p + TIMESTAMPS_AND_OFFSETS_OFFSET);
}

large_buf_ref *keys_largebuf(void *root_buffer) {
    char *p = reinterpret_cast<char *>(root_buffer);
    return reinterpret_cast<large_buf_ref *>(p + (TIMESTAMPS_AND_OFFSETS_OFFSET + TIMESTAMPS_AND_OFFSETS_SIZE));
}

int keys_largebuf_ref_size(block_size_t block_size) {
    return block_size.value() - (TIMESTAMPS_AND_OFFSETS_OFFSET + TIMESTAMPS_AND_OFFSETS_SIZE);
}

// TODO: Add test doublechecking that sizeof(t_and_o) == 12.
struct t_and_o {
    repli_timestamp timestamp;
    off64_t offset;
} __attribute__((__packed__));

}  // namespace delete_queue

void add_key_to_delete_queue(transaction_t *txn, block_id_t queue_root_id, repli_timestamp timestamp, const store_key_t *key) {
    // Beware: Right now, some aspects of correctness depend on the
    // fact that we hold the queue_root lock for the entire operation.
    buf_lock_t queue_root(txn, queue_root_id, rwi_write);

    // TODO this could be a non-major write?
    void *queue_root_buf = queue_root->get_data_major_write();

    off64_t primal_offset = delete_queue::primal_offset(queue_root_buf);
    large_buf_ref *t_o_ref = delete_queue::timestamps_and_offsets_largebuf(queue_root_buf);
    large_buf_ref *keys_ref = delete_queue::keys_largebuf(queue_root_buf);

    rassert(t_o_ref->size % sizeof(delete_queue::t_and_o) == 0);

    // 1. Possibly update the (timestamp, offset) queue.  (This happens at most once per second.)
    {
        // TODO: Why must we allocate large_buf_t's with new?
        boost::scoped_ptr<large_buf_t> t_o_largebuf(new large_buf_t(txn, t_o_ref, lbref_limit_t(delete_queue::TIMESTAMPS_AND_OFFSETS_SIZE), rwi_write));

        // TODO: Allow upgrade of large buf intent.
        co_acquire_large_buf(t_o_largebuf.get());

        if (t_o_ref->size == 0) {
            delete_queue::t_and_o tao;
            tao.timestamp = timestamp;
            tao.offset = primal_offset + keys_ref->size;
            int refsize_adjustment_dontcare;
            t_o_largebuf->append(sizeof(tao), &refsize_adjustment_dontcare);
            t_o_largebuf->fill_at(0, &tao, sizeof(tao));
        } else {
            delete_queue::t_and_o last_tao;
            t_o_largebuf->read_at(t_o_ref->size - sizeof(last_tao), &last_tao, sizeof(last_tao));

            if (last_tao.timestamp.time > timestamp.time) {
                logWRN("The delete queue is receiving updates out of order (t1 = %d, t2 = %d), or the system clock has been set back!  Bringing up a replica may be excessively inefficient.\n", last_tao.timestamp.time, timestamp.time);

                // Timestamps must be monotonically increasing, so sorry.
                timestamp = last_tao.timestamp;
            }

            if (last_tao.timestamp.time != timestamp.time) {
                delete_queue::t_and_o tao;
                tao.timestamp = timestamp;
                tao.offset = primal_offset + keys_ref->size;
                int refsize_adjustment_dontcare;
                t_o_largebuf->append(sizeof(tao), &refsize_adjustment_dontcare);
                t_o_largebuf->fill_at(t_o_ref->size - sizeof(tao), &tao, sizeof(tao));
            }
        }

        // TODO: Remove old items from the front of t_o_largebuf.
    }

    // 2. Update the keys list.

    {
        boost::scoped_ptr<large_buf_t> keys_largebuf(new large_buf_t(txn, keys_ref, lbref_limit_t(delete_queue::keys_largebuf_ref_size(txn->cache->get_block_size())), rwi_write));

        // TODO: acquire rhs, or lhs+rhs, something appropriate.
        co_acquire_large_buf(keys_largebuf.get());

        int refsize_adjustment_dontcare;
        keys_largebuf->append(1 + key->size, &refsize_adjustment_dontcare);
        keys_largebuf->fill_at(keys_ref->size - (1 + key->size), key, 1 + key->size);
    }
}

void dump_keys_from_delete_queue(transaction_t *txn, block_id_t queue_root_id, repli_timestamp begin_timestamp, repli_timestamp end_timestamp, deletion_key_stream_receiver_t *recipient) {
    // Beware: Right now, some aspects of correctness depend on the
    // fact that we hold the queue_root lock for the entire operation.
    buf_lock_t queue_root(txn, queue_root_id, rwi_read);

    void *queue_root_buf = const_cast<void *>(queue_root->get_data_read());

    off64_t primal_offset = delete_queue::primal_offset(queue_root_buf);
    large_buf_ref *t_o_ref = delete_queue::timestamps_and_offsets_largebuf(queue_root_buf);
    large_buf_ref *keys_ref = delete_queue::keys_largebuf(queue_root_buf);

    rassert(t_o_ref->size % sizeof(delete_queue::t_and_o) == 0);

    // TODO: DON'T hold the queue_root lock for the entire operation.  Sheesh.

    int64_t begin_offset = 0, end_offset = 0;

    {
        boost::scoped_ptr<large_buf_t> t_o_largebuf(new large_buf_t(txn, t_o_ref, lbref_limit_t(delete_queue::TIMESTAMPS_AND_OFFSETS_SIZE), rwi_read));
        co_acquire_large_buf(t_o_largebuf.get());

        delete_queue::t_and_o tao;
        int64_t i = 0, ie = t_o_ref->size;
        bool begin_found = false, end_found = false;
        while (i < ie) {
            t_o_largebuf->read_at(i, &tao, sizeof(tao));
            if (!begin_found && begin_timestamp.time <= tao.timestamp.time) {
                begin_offset = tao.offset - primal_offset;
                begin_found = true;
            }
            if (end_timestamp.time <= tao.timestamp.time) {
                rassert(begin_found);
                end_offset = tao.offset - primal_offset;
                end_found = true;
                break;
            }
            i += sizeof(tao);
        }

        if (!begin_found) {
            return;
            // Nothing to do!
        }

        if (!end_found) {
            end_offset = t_o_ref->size;
        }

        // So we have a begin_offset and an end_offset;
    }

    rassert(begin_offset <= end_offset);

    if (begin_offset < end_offset) {
        boost::scoped_ptr<large_buf_t> keys_largebuf(new large_buf_t(txn, keys_ref, lbref_limit_t(delete_queue::keys_largebuf_ref_size(txn->cache->get_block_size())), rwi_read));

        // TODO: acquire subinterval.
        co_acquire_large_buf(keys_largebuf.get());

        int64_t n = end_offset - begin_offset;

        // TODO: don't copy needlessly... sheesh.  This is a fake
        // implementation, make something that actually streams later.
        scoped_malloc<byte> buf(n);

        keys_largebuf->fill_at(begin_offset, buf.get(), n);

        // To force deletion_key_stream_receiver_t to be designed to
        // accept multiple calls, we send two calls.
        int64_t half_n = n / 2;
        recipient->receive_keys(buf.get(), half_n);
        recipient->receive_keys(buf.get() + half_n, n - half_n);
    }
}



}  // namespace replication
