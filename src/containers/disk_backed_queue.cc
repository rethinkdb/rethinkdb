// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "containers/disk_backed_queue.hpp"

#include "arch/io/disk.hpp"
#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/alt_blob.hpp"
#include "buffer_cache/alt/alt_serialize_onto_blob.hpp"
#include "serializer/config.hpp"

internal_disk_backed_queue_t::internal_disk_backed_queue_t(io_backender_t *io_backender,
                                                           const serializer_filepath_t &filename,
                                                           perfmon_collection_t *stats_parent)
    : perfmon_membership(stats_parent, &perfmon_collection,
                         filename.permanent_path().c_str()),
      queue_size(0),
      head_block_id(NULL_BLOCK_ID),
      tail_block_id(NULL_BLOCK_ID) {
    filepath_file_opener_t file_opener(filename, io_backender);
    standard_serializer_t::create(&file_opener,
                                  standard_serializer_t::static_config_t());

    serializer.init(new standard_serializer_t(standard_serializer_t::dynamic_config_t(),
                                              &file_opener,
                                              &perfmon_collection));

    /* Remove the file we just created from the filesystem, so that it will
       get deleted as soon as the serializer is destroyed or if the process
       crashes. */
    file_opener.unlink_serializer_file();

    alt_cache_config_t cache_dynamic_config;
    cache_dynamic_config.page_config.memory_limit = MEGABYTE;
    cache.init(new alt_cache_t(serializer.get(), cache_dynamic_config,
                               &perfmon_collection));
    // Emulate cache_t::create behavior by zeroing the block with id SUPERBLOCK_ID.
    alt_txn_t txn(cache.get(), write_durability_t::HARD,
                  repli_timestamp_t::distant_past, 1);
    alt_buf_lock_t block(&txn, SUPERBLOCK_ID, alt_create_t::create);
    alt_buf_write_t write(&block);
    const block_size_t block_size = cache->max_block_size();
    void *buf = write.get_data_write(block_size.value());
    memset(buf, 0, block_size.value());
}

internal_disk_backed_queue_t::~internal_disk_backed_queue_t() { }

void internal_disk_backed_queue_t::push(const write_message_t &wm) {
    mutex_t::acq_t mutex_acq(&mutex);

    // There's no need for hard durability with an unlinked dbq file.
    alt_txn_t txn(cache.get(),
                  write_durability_t::SOFT,
                  repli_timestamp_t::distant_past,
                  2);

    if (head_block_id == NULL_BLOCK_ID) {
        add_block_to_head(&txn);
    }

    auto _head = make_scoped<alt_buf_lock_t>(alt_buf_parent_t(&txn), head_block_id,
                                             alt_access_t::write);
    auto write = make_scoped<alt_buf_write_t>(_head.get());
    queue_block_t *head = static_cast<queue_block_t *>(write->get_data_write());

    char buffer[MAX_REF_SIZE];
    memset(buffer, 0, MAX_REF_SIZE);

    blob_t blob(cache->max_block_size(), buffer, MAX_REF_SIZE);

    write_onto_blob(alt_buf_parent_t(_head.get()), &blob, wm);

    if (static_cast<size_t>((head->data + head->data_size) - reinterpret_cast<char *>(head)) + blob.refsize(cache->max_block_size()) > cache->max_block_size().value()) {
        // The data won't fit in our current head block, so it's time to make a new one.
        head = NULL;
        write.reset();
        _head.reset();
        add_block_to_head(&txn);
        _head.init(new alt_buf_lock_t(alt_buf_parent_t(&txn), head_block_id,
                                      alt_access_t::write));
        write.init(new alt_buf_write_t(_head.get()));
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

    char buffer[MAX_REF_SIZE];
    // No need for hard durability with an unlinked dbq file.
    alt_txn_t txn(cache.get(),
                  write_durability_t::SOFT,
                  repli_timestamp_t::distant_past,
                  2);

    auto _tail = make_scoped<alt_buf_lock_t>(alt_buf_parent_t(&txn), tail_block_id,
                                             alt_access_t::write);
    auto write = make_scoped<alt_buf_write_t>(_tail.get());
    queue_block_t *tail = static_cast<queue_block_t *>(write->get_data_write());
    rassert(tail->data_size != tail->live_data_offset);

    /* Grab the data from the blob and delete it. */

    memcpy(buffer, tail->data + tail->live_data_offset, blob::ref_size(cache->max_block_size(), tail->data + tail->live_data_offset, MAX_REF_SIZE));
    std::vector<char> data_vec;

    blob_t blob(cache->max_block_size(), buffer, MAX_REF_SIZE);
    {
        blob_acq_t acq_group;
        buffer_group_t blob_group;
        blob.expose_all(alt_buf_parent_t(_tail.get()), alt_access_t::read,
                        &blob_group, &acq_group);

        viewer->view_buffer_group(const_view(&blob_group));
    }

    /* Record how far along in the blob we are. */
    tail->live_data_offset += blob.refsize(cache->max_block_size());

    blob.clear(alt_buf_parent_t(_tail.get()));

    queue_size--;

    /* If that was the last blob in this block move on to the next one. */
    if (tail->live_data_offset == tail->data_size) {
        // RSI: This manual write/_tail resetting is a bit ghetto -- can we clean it up a bit?
        write.reset();
        _tail.reset();
        remove_block_from_tail(&txn);
    }
}

bool internal_disk_backed_queue_t::empty() {
    return queue_size == 0;
}

int64_t internal_disk_backed_queue_t::size() {
    return queue_size;
}

void internal_disk_backed_queue_t::add_block_to_head(alt_txn_t *txn) {
    alt_buf_lock_t _new_head(alt_buf_parent_t(txn), alt_create_t::create);
    alt_buf_write_t write(&_new_head);
    queue_block_t *new_head = static_cast<queue_block_t *>(write.get_data_write());
    if (head_block_id == NULL_BLOCK_ID) {
        rassert(tail_block_id == NULL_BLOCK_ID);
        head_block_id = tail_block_id = _new_head.block_id();
    } else {
        alt_buf_lock_t _old_head(alt_buf_parent_t(txn), head_block_id,
                                 alt_access_t::write);
        alt_buf_write_t old_write(&_old_head);
        queue_block_t *old_head
            = static_cast<queue_block_t *>(old_write.get_data_write());
        rassert(old_head->next == NULL_BLOCK_ID);
        old_head->next = _new_head.get_block_id();
        head_block_id = _new_head.get_block_id();
    }

    new_head->next = NULL_BLOCK_ID;
    new_head->data_size = 0;
    new_head->live_data_offset = 0;
}

void internal_disk_backed_queue_t::remove_block_from_tail(alt_txn_t *txn) {
    rassert(tail_block_id != NULL_BLOCK_ID);
    alt_buf_lock_t _old_tail(alt_buf_parent_t(txn), tail_block_id,
                             alt_access_t::write);

    {
        alt_buf_write_t old_write(&_old_tail);
        queue_block_t *old_tail = static_cast<queue_block_t *>(old_write.get_data_write());

        if (old_tail->next == NULL_BLOCK_ID) {
            rassert(head_block_id == _old_tail.get_block_id());
            tail_block_id = head_block_id = NULL_BLOCK_ID;
        } else {
            tail_block_id = old_tail->next;
        }
    }

    _old_tail.mark_deleted();
}

