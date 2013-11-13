// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "containers/disk_backed_queue.hpp"

#include "arch/io/disk.hpp"
#if DBQ_USE_ALT_CACHE
#include "buffer_cache/alt/alt.hpp"
#include "buffer_cache/alt/alt_blob.hpp"
#else
#include "buffer_cache/blob.hpp"
#include "buffer_cache/buffer_cache.hpp"
#endif
#include "serializer/config.hpp"

#if DBQ_USE_ALT_CACHE
using alt::alt_access_t;
using alt::alt_cache_t;
using alt::alt_create_t;
using alt::alt_txn_t;
using alt::alt_buf_lock_t;
using alt::alt_buf_write_t;
#endif

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

#if DBQ_USE_ALT_CACHE
    cache.init(new alt_cache_t(serializer.get()));
    // Emulate cache_t::create behavior by zeroing the block with id SUPERBLOCK_ID.
    // RSI: Is this actually necessary?
    alt_txn_t txn(cache.get());
    alt_buf_lock_t block(&txn, SUPERBLOCK_ID, alt_create_t::create);
    alt_buf_write_t write(&block);
    const block_size_t block_size = cache->max_block_size();
    void *buf = write.get_data_write(block_size.value());
    memset(buf, 0, block_size.value());
#else
    /* Create the cache. */
    cache_t::create(serializer.get());

    mirrored_cache_config_t cache_dynamic_config;
    cache_dynamic_config.max_size = MEGABYTE;
    cache_dynamic_config.max_dirty_size = MEGABYTE / 2;
    cache.init(new cache_t(serializer.get(), cache_dynamic_config, &perfmon_collection));
#endif
}

internal_disk_backed_queue_t::~internal_disk_backed_queue_t() { }

void internal_disk_backed_queue_t::push(const write_message_t &wm) {
    mutex_t::acq_t mutex_acq(&mutex);

#if DBQ_USE_ALT_CACHE
    alt_txn_t txn(cache.get());
#else
    // First, we need a transaction.
    transaction_t txn(cache.get(),
                      rwi_write,
                      2,
                      repli_timestamp_t::distant_past,
                      cache_order_source.check_in("push"),
                      WRITE_DURABILITY_SOFT /* No need for durability with unlinked dbq file. */);
#endif

    if (head_block_id == NULL_BLOCK_ID) {
        add_block_to_head(&txn);
    }

#if DBQ_USE_ALT_CACHE
    auto _head = make_scoped<alt_buf_lock_t>(&txn, head_block_id,
                                             alt_access_t::write);
    auto write = make_scoped<alt_buf_write_t>(_head.get());
    queue_block_t *head = static_cast<queue_block_t *>(write->get_data_write());
#else
    scoped_ptr_t<buf_lock_t> _head(new buf_lock_t(&txn, head_block_id, rwi_write));
    queue_block_t *head = static_cast<queue_block_t *>(_head->get_data_write());
#endif

    vector_stream_t stream;
    int res = send_write_message(&stream, &wm);
    guarantee(res == 0);

    char buffer[MAX_REF_SIZE];
    bzero(buffer, MAX_REF_SIZE);

#if DBQ_USE_ALT_CACHE
    // RSI: Wrong parent?  Can we just make the blob later?
    alt::blob_t blob(cache->max_block_size(), buffer, MAX_REF_SIZE);
    blob.append_region(_head.get(), stream.vector().size());
#else
    blob_t blob(cache->get_block_size(), buffer, MAX_REF_SIZE);
    blob.append_region(&txn, stream.vector().size());
#endif
    std::string sered_data(stream.vector().begin(), stream.vector().end());

#if DBQ_USE_ALT_CACHE
    blob.write_from_string(sered_data, _head.get(), 0);
#else
    blob.write_from_string(sered_data, &txn, 0);
#endif

#if DBQ_USE_ALT_CACHE
    if (static_cast<size_t>((head->data + head->data_size) - reinterpret_cast<char *>(head)) + blob.refsize(cache->max_block_size()) > cache->max_block_size().value()) {
        // The data won't fit in our current head block, so it's time to make a new one.
        head = NULL;
        write.reset();
        _head.reset();
        add_block_to_head(&txn);
        _head.init(new alt_buf_lock_t(&txn, head_block_id, alt_access_t::write));
        write.init(new alt_buf_write_t(_head.get()));
        head = static_cast<queue_block_t *>(write->get_data_write());
    }
#else
    if (static_cast<size_t>((head->data + head->data_size) - reinterpret_cast<char *>(head)) + blob.refsize(cache->get_block_size()) > cache->get_block_size().value()) {
        // The data won't fit in our current head block, so it's time to make a new one.
        head = NULL;
        _head.reset();
        add_block_to_head(&txn);
        _head.init(new buf_lock_t(&txn, head_block_id, rwi_write));
        head = static_cast<queue_block_t *>(_head->get_data_write());
    }
#endif

#if DBQ_USE_ALT_CACHE
    memcpy(head->data + head->data_size, buffer,
           blob.refsize(cache->max_block_size()));
    head->data_size += blob.refsize(cache->max_block_size());
#else
    memcpy(head->data + head->data_size, buffer, blob.refsize(cache->get_block_size()));
    head->data_size += blob.refsize(cache->get_block_size());
#endif

    queue_size++;
}

void internal_disk_backed_queue_t::pop(std::vector<char> *buf_out) {
    guarantee(size() != 0);
    mutex_t::acq_t mutex_acq(&mutex);

    char buffer[MAX_REF_SIZE];
#if DBQ_USE_ALT_CACHE
    alt_txn_t txn(cache.get());
#else
    transaction_t txn(cache.get(),
                      rwi_write,
                      2,
                      repli_timestamp_t::distant_past,
                      cache_order_source.check_in("pop"),
                      WRITE_DURABILITY_SOFT /* No durability for unlinked dbq file. */);
#endif

#if DBQ_USE_ALT_CACHE
    auto _tail = make_scoped<alt_buf_lock_t>(&txn, tail_block_id,
                                             alt_access_t::write);
    auto write = make_scoped<alt_buf_write_t>(_tail.get());
    queue_block_t *tail = static_cast<queue_block_t *>(write->get_data_write());
#else
    scoped_ptr_t<buf_lock_t> _tail(new buf_lock_t(&txn, tail_block_id, rwi_write));
    queue_block_t *tail = static_cast<queue_block_t *>(_tail->get_data_write());
#endif
    rassert(tail->data_size != tail->live_data_offset);

    /* Grab the data from the blob and delete it. */

#if DBQ_USE_ALT_CACHE
    memcpy(buffer, tail->data + tail->live_data_offset, alt::blob::ref_size(cache->max_block_size(), tail->data + tail->live_data_offset, MAX_REF_SIZE));
#else
    memcpy(buffer, tail->data + tail->live_data_offset, blob::ref_size(cache->get_block_size(), tail->data + tail->live_data_offset, MAX_REF_SIZE));
#endif
    std::vector<char> data_vec;

#if DBQ_USE_ALT_CACHE
    alt::blob_t blob(cache->max_block_size(), buffer, MAX_REF_SIZE);
#else
    blob_t blob(cache->get_block_size(), buffer, MAX_REF_SIZE);
#endif
    {
#if DBQ_USE_ALT_CACHE
        alt::blob_acq_t acq_group;
#else
        blob_acq_t acq_group;
#endif
        buffer_group_t blob_group;
#if DBQ_USE_ALT_CACHE
        blob.expose_all(_tail.get(), alt_access_t::read, &blob_group, &acq_group);
#else
        blob.expose_all(&txn, rwi_read, &blob_group, &acq_group);
#endif

        data_vec.resize(blob_group.get_size());
        buffer_group_t data_vec_group;
        data_vec_group.add_buffer(data_vec.size(), data_vec.data());
        buffer_group_copy_data(&data_vec_group, const_view(&blob_group));
    }

    /* Record how far along in the blob we are. */
#if DBQ_USE_ALT_CACHE
    tail->live_data_offset += blob.refsize(cache->max_block_size());
#else
    tail->live_data_offset += blob.refsize(cache->get_block_size());
#endif

#if DBQ_USE_ALT_CACHE
    blob.clear(_tail.get());
#else
    blob.clear(&txn);
#endif

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

#if DBQ_USE_ALT_CACHE
void internal_disk_backed_queue_t::add_block_to_head(alt_txn_t *txn) {
    alt_buf_lock_t _new_head(txn, alt_access_t::write);
    alt_buf_write_t write(&_new_head);
    queue_block_t *new_head = static_cast<queue_block_t *>(write.get_data_write());
#else
void internal_disk_backed_queue_t::add_block_to_head(transaction_t *txn) {
    buf_lock_t _new_head(txn);
    queue_block_t *new_head = static_cast<queue_block_t *>(_new_head.get_data_write());
#endif
    if (head_block_id == NULL_BLOCK_ID) {
        rassert(tail_block_id == NULL_BLOCK_ID);
#if DBQ_USE_ALT_CACHE
        head_block_id = tail_block_id = _new_head.block_id();
#else
        head_block_id = tail_block_id = _new_head.get_block_id();
#endif
    } else {
#if DBQ_USE_ALT_CACHE
        alt_buf_lock_t _old_head(txn, head_block_id, alt_access_t::write);
        alt_buf_write_t old_write(&_old_head);
        queue_block_t *old_head
            = static_cast<queue_block_t *>(old_write.get_data_write());
#else
        buf_lock_t _old_head(txn, head_block_id, rwi_write);
        queue_block_t *old_head = static_cast<queue_block_t *>(_old_head.get_data_write());
#endif
        rassert(old_head->next == NULL_BLOCK_ID);
        old_head->next = _new_head.get_block_id();
        head_block_id = _new_head.get_block_id();
    }

    new_head->next = NULL_BLOCK_ID;
    new_head->data_size = 0;
    new_head->live_data_offset = 0;
}

#if DBQ_USE_ALT_CACHE
void internal_disk_backed_queue_t::remove_block_from_tail(alt_txn_t *txn) {
#else
void internal_disk_backed_queue_t::remove_block_from_tail(transaction_t *txn) {
#endif
    rassert(tail_block_id != NULL_BLOCK_ID);
#if DBQ_USE_ALT_CACHE
    alt_buf_lock_t _old_tail(txn, tail_block_id, alt_access_t::write);
#else
    buf_lock_t _old_tail(txn, tail_block_id, rwi_write);
#endif

    {
#if DBQ_USE_ALT_CACHE
        alt_buf_write_t old_write(&_old_tail);
        queue_block_t *old_tail = static_cast<queue_block_t *>(old_write.get_data_write());
#else
        queue_block_t *old_tail = static_cast<queue_block_t *>(_old_tail.get_data_write());
#endif

        if (old_tail->next == NULL_BLOCK_ID) {
            rassert(head_block_id == _old_tail.get_block_id());
            tail_block_id = head_block_id = NULL_BLOCK_ID;
        } else {
            tail_block_id = old_tail->next;
        }
    }

    _old_tail.mark_deleted();
}

