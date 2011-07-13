#include "replication/delete_queue.hpp"

#include "btree/node.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "buffer_cache/co_functions.hpp"
#include "containers/scoped_malloc.hpp"
#include "data_provider.hpp"
#include "logger.hpp"
#include "store.hpp"

namespace replication {

namespace delete_queue {

// The offset of the primal offset.
const int PRIMAL_OFFSET_OFFSET = sizeof(block_magic_t);
const int TIMESTAMPS_AND_OFFSETS_OFFSET = PRIMAL_OFFSET_OFFSET + sizeof(off64_t);
const int TIMESTAMPS_AND_OFFSETS_SIZE = blob::maxreflen_from_blockid_count(3);

off64_t *primal_offset(void *root_buffer) {
    return reinterpret_cast<off64_t *>(reinterpret_cast<char *>(root_buffer) + PRIMAL_OFFSET_OFFSET);
}

char *timestamps_and_offsets_blob_ref(void *root_buffer) {
    return reinterpret_cast<char *>(root_buffer) + TIMESTAMPS_AND_OFFSETS_OFFSET;
}

char *keys_blob_ref(void *root_buffer) {
    return reinterpret_cast<char *>(root_buffer) + (TIMESTAMPS_AND_OFFSETS_OFFSET + TIMESTAMPS_AND_OFFSETS_SIZE);
}

int keys_blob_ref_size(block_size_t block_size) {
    return block_size.value() - (TIMESTAMPS_AND_OFFSETS_OFFSET + TIMESTAMPS_AND_OFFSETS_SIZE);
}

}  // namespace delete_queue

void add_key_to_delete_queue(int64_t delete_queue_limit, transaction_t *txn, block_id_t queue_root_id, repli_timestamp_t timestamp, const store_key_t *key) {
    // Beware: Right now, some aspects of correctness depend on the
    // fact that we hold the queue_root lock for the entire operation.
    buf_lock_t queue_root(txn, queue_root_id, rwi_write);

    // TODO this could be a non-major write?
    void *queue_root_buf = queue_root->get_data_major_write();

    off64_t *primal_offset = delete_queue::primal_offset(queue_root_buf);
    blob_t t_o_blob(delete_queue::timestamps_and_offsets_blob_ref(queue_root_buf), delete_queue::TIMESTAMPS_AND_OFFSETS_SIZE);
    blob_t keys_blob(delete_queue::keys_blob_ref(queue_root_buf), delete_queue::keys_blob_ref_size(txn->get_cache()->get_block_size()));

#ifndef NDEBUG
    {
        bool modcmp = t_o_blob.valuesize() % sizeof(delete_queue::t_and_o) == 0;
        rassert(modcmp, "valuesize = %ld", keys_blob.valuesize());
    }
#endif

    // Figure out what we need to do.
    bool will_want_to_dequeue = (keys_blob.valuesize() + 1 + int64_t(key->size) > delete_queue_limit);
    bool will_actually_dequeue = false;

    delete_queue::t_and_o second_tao = { repli_timestamp_t::invalid, -1 };

    // Possibly update the (timestamp, offset) queue.  (This happens at most once per second.)
    {
        if (t_o_blob.valuesize() == 0) {
            delete_queue::t_and_o tao;
            tao.timestamp = timestamp;
            tao.offset = *primal_offset + keys_blob.valuesize();
            buffer_group_t copyee;
            copyee.add_buffer(sizeof(tao), &tao);

            t_o_blob.append_region(txn, sizeof(tao));
            blob_acq_t acqs;
            buffer_group_t bg;
            t_o_blob.expose_region(txn, rwi_write, t_o_blob.valuesize() - sizeof(tao), sizeof(tao), &bg, &acqs);
            buffer_group_copy_data(&bg, const_view(&copyee));

            rassert(keys_blob.valuesize() == 0);
        } else {

            delete_queue::t_and_o last_tao;

            {
                buffer_group_t bg_last_tao;
                bg_last_tao.add_buffer(sizeof(last_tao), &last_tao);
                {
                    buffer_group_t tmp;
                    blob_acq_t acqs;
                    t_o_blob.expose_region(txn, rwi_read, t_o_blob.valuesize() - sizeof(last_tao), sizeof(last_tao), &tmp, &acqs);
                    buffer_group_copy_data(&bg_last_tao, const_view(&tmp));
                }

                if (last_tao.timestamp.time > timestamp.time) {
                    logWRN("The delete queue is receiving updates out of order (t1 = %d, t2 = %d), or the system clock has been set back!  Bringing up a replica may be excessively inefficient.\n", last_tao.timestamp.time, timestamp.time);

                    // Timestamps must be monotonically increasing, so sorry.
                    timestamp = last_tao.timestamp;
                }

                if (last_tao.timestamp.time != timestamp.time) {
                    delete_queue::t_and_o tao;
                    tao.timestamp = timestamp;
                    tao.offset = *primal_offset + keys_blob.valuesize();
                    buffer_group_t copyee;
                    copyee.add_buffer(sizeof(tao), &tao);

                    t_o_blob.append_region(txn, sizeof(tao));
                    blob_acq_t acqs;
                    buffer_group_t bg;
                    t_o_blob.expose_region(txn, rwi_write, t_o_blob.valuesize() - sizeof(tao), sizeof(tao), &bg, &acqs);
                    buffer_group_copy_data(&bg, const_view(&copyee));
                }
            }

            if (will_want_to_dequeue && t_o_blob.valuesize() >= int64_t(2 * sizeof(second_tao))) {
                {
                    blob_acq_t acqs;
                    buffer_group_t bg;

                    t_o_blob.expose_region(txn, rwi_read, 0, 2 * sizeof(second_tao), &bg, &acqs);
                    buffer_group_t target;
                    target.add_buffer(sizeof(second_tao), &second_tao);
                    buffer_group_copy_data(&target, const_view(&bg));
                }

                // We want to encourage at least one timestamp to
                // actually be less than the minimum valid timestamp.
                if (second_tao.offset < *primal_offset) {
                    will_actually_dequeue = true;

                    t_o_blob.unprepend_region(txn, sizeof(second_tao));
                }
            }
        }
    }

    // Update the keys list.

    {
        int64_t amount_to_unprepend = will_actually_dequeue ? second_tao.offset - *primal_offset : 0;
        rassert(amount_to_unprepend <= keys_blob.valuesize() + 1 + key->size);

        const_buffer_group_t copyee;
        copyee.add_buffer(1 + key->size, key);

        keys_blob.append_region(txn, 1 + key->size);
        {
            blob_acq_t acqs;
            buffer_group_t bufs;
            keys_blob.expose_region(txn, rwi_write, keys_blob.valuesize() - (1 + key->size), 1 + key->size, &bufs, &acqs);
            buffer_group_copy_data(&bufs, &copyee);
        }

        keys_blob.unprepend_region(txn, amount_to_unprepend);

        *primal_offset += amount_to_unprepend;
    }
}

bool dump_keys_from_delete_queue(transaction_t *txn, block_id_t queue_root_id, repli_timestamp_t begin_timestamp, deletion_key_stream_receiver_t *recipient) {
    // Beware: Right now, some aspects of correctness depend on the
    // fact that we hold the queue_root lock for the entire operation.
    buf_lock_t queue_root(txn, queue_root_id, rwi_read);

    void *queue_root_buf = const_cast<void *>(queue_root->get_data_read());

    off64_t *primal_offset = delete_queue::primal_offset(queue_root_buf);
    blob_t t_o_blob(delete_queue::timestamps_and_offsets_blob_ref(queue_root_buf), delete_queue::TIMESTAMPS_AND_OFFSETS_SIZE);
    blob_t keys_blob(delete_queue::keys_blob_ref(queue_root_buf), delete_queue::keys_blob_ref_size(txn->get_cache()->get_block_size()));

    if (t_o_blob.valuesize() != 0 && keys_blob.valuesize() != 0) {

#ifndef NDEBUG
        {
            bool modcmp = t_o_blob.valuesize() % sizeof(delete_queue::t_and_o) == 0;
            rassert(modcmp);
        }
#endif

        // TODO: DON'T hold the queue_root lock for the entire operation.  Sheesh.

        int64_t begin_offset = 0;
        int64_t end_offset = keys_blob.valuesize();

        {
            delete_queue::t_and_o tao;
            int64_t i = 0, ie = t_o_blob.valuesize();
            bool begin_found = false;

            while (i < ie) {
                blob_acq_t acqs;
                buffer_group_t bg;
                t_o_blob.expose_region(txn, rwi_read, i, sizeof(tao), &bg, &acqs);
                buffer_group_t tmp;
                tmp.add_buffer(sizeof(tao), &tao);
                buffer_group_copy_data(&tmp, const_view(&bg));

                if (begin_timestamp.time <= tao.timestamp.time) {
                    begin_offset = tao.offset - *primal_offset;
                    begin_found = true;
                    break;
                }

                i += sizeof(tao);
            }

            if (begin_found && tao.offset < *primal_offset) {
                begin_found = false;
            } else if (!begin_found && ie > 0 && begin_timestamp.time > tao.timestamp.time) {
                begin_offset = end_offset;
                begin_found = true;
            }

            if (!recipient->should_send_deletion_keys(begin_found)) {
                return false;
            }

            // So we have a begin_offset and an end_offset.
        }

        rassert(begin_offset <= end_offset);

        if (begin_offset < end_offset) {
            // TODO: Don't copy needlessly... sheesh.  This is a fake
            // implementation, make something that actually streams
            // later.

            int64_t n = end_offset - begin_offset;
            scoped_malloc<char> buf(n);

            {
                blob_acq_t bacq;
                buffer_group_t bg;
                keys_blob.expose_region(txn, rwi_read_outdated_ok, begin_offset, end_offset - begin_offset, &bg, &bacq);

                buffer_group_t bufgroup;
                bufgroup.add_buffer(n, buf.get());

                buffer_group_copy_data(&bufgroup, const_view(&bg));
            }

            char *p = buf.get();
            char *e = p + n;
            while (p < e) {
                btree_key_t *k = reinterpret_cast<btree_key_t *>(p);
                rassert(k->size + 1 <= e - p);

                recipient->deletion_key(k);
                p += k->size + 1;
            }
        }
    } else {
        // We can only be here if the delete queue never had any keys
        // inserted into it -- in which it's correct to say that we
        // can send all the keys in the delete queue from a given time
        // stamp.  So we pass true.
        if (!recipient->should_send_deletion_keys(true)) {
            return false;
        }
    }

    recipient->done_deletion_keys();
    return true;
}

void initialize_empty_delete_queue(transaction_t *txn, delete_queue_block_t *dqb, block_size_t block_size) {
    memset(dqb, 0, block_size.value());
    dqb->magic = delete_queue_block_t::expected_magic;

    blob_t t_o_blob(delete_queue::timestamps_and_offsets_blob_ref(dqb), delete_queue::TIMESTAMPS_AND_OFFSETS_SIZE);

    delete_queue::t_and_o zerotime = { { 0 }, 0 };
    buffer_group_t bg_zerotime;
    bg_zerotime.add_buffer(sizeof(zerotime), &zerotime);

    blob_acq_t acqs;
    buffer_group_t bg;
    t_o_blob.append_region(txn, sizeof(zerotime));
    t_o_blob.expose_region(txn, rwi_write, 0, sizeof(zerotime), &bg, &acqs);
    buffer_group_copy_data(&bg, const_view(&bg_zerotime));
}



const block_magic_t delete_queue_block_t::expected_magic = { { 'D', 'e', 'l', 'Q' } };



}  // namespace replication
