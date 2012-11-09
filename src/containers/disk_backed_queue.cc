// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "containers/disk_backed_queue.hpp"

#include "arch/io/disk.hpp"
#include "buffer_cache/blob.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "serializer/config.hpp"

internal_disk_backed_queue_t::internal_disk_backed_queue_t(io_backender_t *io_backender,
                                                           const std::string& filename,
                                                           perfmon_collection_t *stats_parent)
    : queue_size(0), head_block_id(NULL_BLOCK_ID), tail_block_id(NULL_BLOCK_ID) {
    /* We're going to register for writes, however those writes won't be able
     * to find their way in to the btree until we're done backfilling. Thus we
     * need to set up a serializer and cache for them to go in to. */
    //perfmon_collection_t backfill_stats_collection("queue-" + filename, NULL, true, true);

    filepath_file_opener_t file_opener(filename, io_backender);
    standard_serializer_t::create(&file_opener,
                                  standard_serializer_t::static_config_t());

    serializer.init(new standard_serializer_t(standard_serializer_t::dynamic_config_t(),
                                              &file_opener,
                                              stats_parent));

    /* Remove the file we just created from the filesystem, so that it will
       get deleted as soon as the serializer is destroyed or if the process
       crashes */
    int res = unlink(filename.c_str());
    guarantee_err(res == 0, "unlink() failed");

    /* Create the cache. */
    mirrored_cache_static_config_t cache_static_config;
    cache_t::create(serializer.get(), &cache_static_config);

    mirrored_cache_config_t cache_dynamic_config;
    cache_dynamic_config.max_size = MEGABYTE;
    cache_dynamic_config.max_dirty_size = MEGABYTE / 2;
    cache.init(new cache_t(serializer.get(), &cache_dynamic_config, stats_parent));
}

internal_disk_backed_queue_t::~internal_disk_backed_queue_t() { }

void internal_disk_backed_queue_t::push(const write_message_t& wm) {
    mutex_t::acq_t mutex_acq(&mutex);

    //first we need a transaction
    transaction_t txn(cache.get(), rwi_write, 2, repli_timestamp_t::distant_past, cache_order_source.check_in("push"));

    if (head_block_id == NULL_BLOCK_ID) {
        add_block_to_head(&txn);
    }

    scoped_ptr_t<buf_lock_t> _head(new buf_lock_t(&txn, head_block_id, rwi_write));
    queue_block_t* head = reinterpret_cast<queue_block_t *>(_head->get_data_major_write());

    vector_stream_t stream;
    int res = send_write_message(&stream, &wm);
    guarantee(res == 0);

    char buffer[MAX_REF_SIZE];
    bzero(buffer, MAX_REF_SIZE);

    blob_t blob(buffer, MAX_REF_SIZE);
    blob.append_region(&txn, stream.vector().size());
    std::string sered_data(stream.vector().begin(), stream.vector().end());
    blob.write_from_string(sered_data, &txn, 0);

    if (((head->data + head->data_size) - reinterpret_cast<char *>(head)) + blob.refsize(cache->get_block_size()) > cache->get_block_size().value()) {
        //The data won't fit in our current head block, so it's time to make a new one
        head = NULL;
        _head.reset();
        add_block_to_head(&txn);
        _head.init(new buf_lock_t(&txn, head_block_id, rwi_write));
        head = reinterpret_cast<queue_block_t *>(_head->get_data_major_write());
    }

    memcpy(head->data + head->data_size, buffer, blob.refsize(cache->get_block_size()));
    head->data_size += blob.refsize(cache->get_block_size());

    queue_size++;
}

void internal_disk_backed_queue_t::pop(std::vector<char> *buf_out) {
    mutex_t::acq_t mutex_acq(&mutex);

    char buffer[MAX_REF_SIZE];
    transaction_t txn(cache.get(), rwi_write, 2, repli_timestamp_t::distant_past, cache_order_source.check_in("pop"));

    scoped_ptr_t<buf_lock_t> _tail(new buf_lock_t(&txn, tail_block_id, rwi_write));
    queue_block_t *tail = reinterpret_cast<queue_block_t *>(_tail->get_data_major_write());
    rassert(tail->data_size != tail->live_data_offset);

    /* Grab the data from the blob and delete it. */

    memcpy(buffer, tail->data + tail->live_data_offset, blob::ref_size(cache->get_block_size(), tail->data + tail->live_data_offset, MAX_REF_SIZE));
    std::vector<char> data_vec;

    blob_t blob(buffer, MAX_REF_SIZE);
    {
        blob_acq_t acq_group;
        buffer_group_t blob_group;
        blob.expose_all(&txn, rwi_read, &blob_group, &acq_group);

        data_vec.resize(blob_group.get_size());
        buffer_group_t data_vec_group;
        data_vec_group.add_buffer(data_vec.size(), data_vec.data());
        buffer_group_copy_data(&data_vec_group, const_view(&blob_group));
    }

    /* Record how far along in the blob we are. */
    tail->live_data_offset += blob.refsize(cache->get_block_size());

    blob.clear(&txn);

    queue_size--;

    /* If that was the last blob in this block move on to the next one. */
    if (tail->live_data_offset == tail->data_size) {
        _tail.reset();
        remove_block_from_tail(&txn);
    }

    /* Deserialize the value and return it. */

    buf_out->swap(data_vec);
}

bool internal_disk_backed_queue_t::empty() {
    return queue_size == 0;
}

int64_t internal_disk_backed_queue_t::size() {
    return queue_size;
}

void internal_disk_backed_queue_t::add_block_to_head(transaction_t *txn) {
    buf_lock_t _new_head(txn);
    queue_block_t *new_head = reinterpret_cast<queue_block_t *>(_new_head.get_data_major_write());
    if (head_block_id == NULL_BLOCK_ID) {
        rassert(tail_block_id == NULL_BLOCK_ID);
        head_block_id = tail_block_id = _new_head.get_block_id();
    } else {
        buf_lock_t _old_head(txn, head_block_id, rwi_write);
        queue_block_t *old_head = reinterpret_cast<queue_block_t *>(_old_head.get_data_major_write());
        rassert(old_head->next == NULL_BLOCK_ID);
        old_head->next = _new_head.get_block_id();
        head_block_id = _new_head.get_block_id();
    }

    new_head->next = NULL_BLOCK_ID;
    new_head->data_size = 0;
    new_head->live_data_offset = 0;
}

void internal_disk_backed_queue_t::remove_block_from_tail(transaction_t *txn) {
    rassert(tail_block_id != NULL_BLOCK_ID);
    buf_lock_t _old_tail(txn, tail_block_id, rwi_write);
    queue_block_t *old_tail = reinterpret_cast<queue_block_t *>(_old_tail.get_data_major_write());

    if (old_tail->next == NULL_BLOCK_ID) {
        rassert(head_block_id == _old_tail.get_block_id());
        tail_block_id = head_block_id = NULL_BLOCK_ID;
    } else {
        tail_block_id = old_tail->next;
    }

    _old_tail.mark_deleted();
}

