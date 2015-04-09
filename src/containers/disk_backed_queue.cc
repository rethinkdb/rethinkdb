// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "containers/disk_backed_queue.hpp"

#include "arch/io/disk.hpp"
#include "buffer_cache/alt.hpp"
#include "buffer_cache/blob.hpp"
#include "buffer_cache/cache_balancer.hpp"
#include "buffer_cache/serialize_onto_blob.hpp"
#include "serializer/config.hpp"


#define DBQ_MAX_REF_SIZE 251

internal_disk_backed_queue_t::internal_disk_backed_queue_t(io_backender_t *io_backender,
                                                           const serializer_filepath_t &filename,
                                                           perfmon_collection_t *stats_parent)
    : perfmon_membership(stats_parent, &perfmon_collection,
                         filename.permanent_path().c_str()),
      queue_size(0),
      head_block_id(NULL_BLOCK_ID),
      tail_block_id(NULL_BLOCK_ID),
      file_opener(new filepath_file_opener_t(filename, io_backender)) {
    standard_serializer_t::create(file_opener.get(),
                                  standard_serializer_t::static_config_t());

    serializer.init(new standard_serializer_t(standard_serializer_t::dynamic_config_t(),
                                              file_opener.get(),
                                              &perfmon_collection));

    balancer.init(new dummy_cache_balancer_t(2 * MEGABYTE));
    cache.init(new cache_t(serializer.get(), balancer.get(), &perfmon_collection));
    cache_conn.init(new cache_conn_t(cache.get()));
    // Emulate cache_t::create behavior by zeroing the block with id SUPERBLOCK_ID.
    txn_t txn(cache_conn.get(), write_durability_t::HARD, 1);
    buf_lock_t block(&txn, SUPERBLOCK_ID, alt_create_t::create);
    buf_write_t write(&block);
    const block_size_t block_size = cache->max_block_size();
    void *buf = write.get_data_write(block_size.value());
    memset(buf, 0, block_size.value());
}

internal_disk_backed_queue_t::~internal_disk_backed_queue_t() {
    /* First destroy the serializer, then remove the temporary file.
    This avoids issues with certain file systems (specifically VirtualBox
    shared folders), see https://github.com/rethinkdb/rethinkdb/issues/3791. */
    cache_conn.reset();
    cache.reset();
    balancer.reset();
    serializer.reset();

    file_opener->unlink_serializer_file();
}

void internal_disk_backed_queue_t::push(const write_message_t &wm) {
    mutex_t::acq_t mutex_acq(&mutex);

    // There's no need for hard durability with an unlinked dbq file.
    txn_t txn(cache_conn.get(), write_durability_t::SOFT, 2);

    push_single(&txn, wm);
}

void internal_disk_backed_queue_t::push(const scoped_array_t<write_message_t> &wms) {
    mutex_t::acq_t mutex_acq(&mutex);

    // There's no need for hard durability with an unlinked dbq file.
    txn_t txn(cache_conn.get(), write_durability_t::SOFT, 2);

    for (size_t i = 0; i < wms.size(); ++i) {
        push_single(&txn, wms[i]);
    }
}

void internal_disk_backed_queue_t::push_single(txn_t *txn, const write_message_t &wm) {
    if (head_block_id == NULL_BLOCK_ID) {
        add_block_to_head(txn);
    }

    auto _head = make_scoped<buf_lock_t>(buf_parent_t(txn), head_block_id,
                                         access_t::write);
    auto write = make_scoped<buf_write_t>(_head.get());
    queue_block_t *head = static_cast<queue_block_t *>(write->get_data_write());

    char buffer[DBQ_MAX_REF_SIZE];
    memset(buffer, 0, DBQ_MAX_REF_SIZE);

    blob_t blob(cache->max_block_size(), buffer, DBQ_MAX_REF_SIZE);

    write_onto_blob(buf_parent_t(_head.get()), &blob, wm);

    if (static_cast<size_t>((head->data + head->data_size) - reinterpret_cast<char *>(head)) + blob.refsize(cache->max_block_size()) > cache->max_block_size().value()) {
        // The data won't fit in our current head block, so it's time to make a new one.
        head = NULL;
        write.reset();
        _head.reset();
        add_block_to_head(txn);
        _head.init(new buf_lock_t(buf_parent_t(txn), head_block_id,
                                  access_t::write));
        write.init(new buf_write_t(_head.get()));
        head = static_cast<queue_block_t *>(write->get_data_write());
    }

    memcpy(head->data + head->data_size, buffer,
           blob.refsize(cache->max_block_size()));
    head->data_size += blob.refsize(cache->max_block_size());

    queue_size++;
}

void internal_disk_backed_queue_t::pop(buffer_group_viewer_t *viewer) {
    guarantee(size() != 0);
    mutex_t::acq_t mutex_acq(&mutex);

    char buffer[DBQ_MAX_REF_SIZE];
    // No need for hard durability with an unlinked dbq file.
    txn_t txn(cache_conn.get(), write_durability_t::SOFT, 2);

    buf_lock_t _tail(buf_parent_t(&txn), tail_block_id, access_t::write);

    /* Grab the data from the blob and delete it. */
    {
        buf_read_t read(&_tail);
        const queue_block_t *tail
            = static_cast<const queue_block_t *>(read.get_data_read());
        rassert(tail->data_size != tail->live_data_offset);
        memcpy(buffer, tail->data + tail->live_data_offset,
               blob::ref_size(cache->max_block_size(),
                              tail->data + tail->live_data_offset,
                              DBQ_MAX_REF_SIZE));
    }

    /* Grab the data from the blob and delete it. */

    std::vector<char> data_vec;

    blob_t blob(cache->max_block_size(), buffer, DBQ_MAX_REF_SIZE);
    {
        blob_acq_t acq_group;
        buffer_group_t blob_group;
        blob.expose_all(buf_parent_t(&_tail), access_t::read,
                        &blob_group, &acq_group);

        viewer->view_buffer_group(const_view(&blob_group));
    }

    int32_t data_size;
    int32_t live_data_offset;
    {
        buf_write_t write(&_tail);
        queue_block_t *tail = static_cast<queue_block_t *>(write.get_data_write());
        /* Record how far along in the blob we are. */
        tail->live_data_offset += blob.refsize(cache->max_block_size());
        data_size = tail->data_size;
        live_data_offset = tail->live_data_offset;
    }

    blob.clear(buf_parent_t(&_tail));

    queue_size--;

    _tail.reset_buf_lock();

    /* If that was the last blob in this block move on to the next one. */
    if (live_data_offset == data_size) {
        remove_block_from_tail(&txn);
    }
}

bool internal_disk_backed_queue_t::empty() {
    return queue_size == 0;
}

int64_t internal_disk_backed_queue_t::size() {
    return queue_size;
}

void internal_disk_backed_queue_t::add_block_to_head(txn_t *txn) {
    buf_lock_t _new_head(buf_parent_t(txn), alt_create_t::create);
    buf_write_t write(&_new_head);
    queue_block_t *new_head = static_cast<queue_block_t *>(write.get_data_write());
    if (head_block_id == NULL_BLOCK_ID) {
        rassert(tail_block_id == NULL_BLOCK_ID);
        head_block_id = tail_block_id = _new_head.block_id();
    } else {
        buf_lock_t _old_head(buf_parent_t(txn), head_block_id,
                             access_t::write);
        buf_write_t old_write(&_old_head);
        queue_block_t *old_head
            = static_cast<queue_block_t *>(old_write.get_data_write());
        rassert(old_head->next == NULL_BLOCK_ID);
        old_head->next = _new_head.block_id();
        head_block_id = _new_head.block_id();
    }

    new_head->next = NULL_BLOCK_ID;
    new_head->data_size = 0;
    new_head->live_data_offset = 0;
}

void internal_disk_backed_queue_t::remove_block_from_tail(txn_t *txn) {
    rassert(tail_block_id != NULL_BLOCK_ID);
    buf_lock_t _old_tail(buf_parent_t(txn), tail_block_id,
                         access_t::write);

    {
        buf_write_t old_write(&_old_tail);
        queue_block_t *old_tail = static_cast<queue_block_t *>(old_write.get_data_write());

        if (old_tail->next == NULL_BLOCK_ID) {
            rassert(head_block_id == _old_tail.block_id());
            tail_block_id = head_block_id = NULL_BLOCK_ID;
        } else {
            tail_block_id = old_tail->next;
        }
    }

    _old_tail.mark_deleted();
}

