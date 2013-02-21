// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONTAINERS_DISK_BACKED_QUEUE_HPP_
#define CONTAINERS_DISK_BACKED_QUEUE_HPP_

#define MAX_REF_SIZE 251

#include <string>
#include <vector>

#include "buffer_cache/types.hpp"
#include "concurrency/fifo_checker.hpp"
#include "concurrency/mutex.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/scoped.hpp"
#include "serializer/types.hpp"

class io_backender_t;
class perfmon_collection_t;

//TODO there are extra copies all over the place mostly stemming from having a
//vector<char> from the serialization code and strings from the blob code.

struct queue_superblock_t {
    block_id_t head, tail;
    int64_t queue_size;
};

struct queue_block_t {
    block_id_t next;
    int data_size, live_data_offset;
    char data[0];
};


class internal_disk_backed_queue_t {
public:
    /* Initializes a new disk backed queue. */
    internal_disk_backed_queue_t(cache_t *cache);
    internal_disk_backed_queue_t(cache_t *cache, transaction_t *txn);
    internal_disk_backed_queue_t(cache_t *cache, block_id_t superblock_id);
    ~internal_disk_backed_queue_t();

    void push(const write_message_t& value);

    void pop(std::vector<char> *buf_out);

    bool empty();

    int64_t size();

private:
    queue_superblock_t *get_superblock(transaction_t *txn, scoped_ptr_t<buf_lock_t> *superblock_out);

    void add_block_to_head(transaction_t *txn, queue_superblock_t *superblock);

    void remove_block_from_tail(transaction_t *txn, queue_superblock_t *superblock);

    mutex_t mutex;
    block_id_t superblock_id;
    cache_t *cache;

    // Serves more as sanity-checking for the cache than this type's ordering.
    order_source_t cache_order_source;

    DISABLE_COPYING(internal_disk_backed_queue_t);
};

/* This creates a disk_backed_queue that constructs it's own cache and
 * serializer. */
class cache_serializer_t {
public:
    cache_serializer_t(io_backender_t *io_backender,
            const std::string &filename, perfmon_collection_t *stats_parent);
    ~cache_serializer_t();

    scoped_ptr_t<standard_serializer_t> serializer;
    scoped_ptr_t<cache_t> cache;

private:
    DISABLE_COPYING(cache_serializer_t);
};

template <class T>
class disk_backed_queue_t {
public:
    disk_backed_queue_t(io_backender_t *io_backender, const std::string &filename, perfmon_collection_t *stats_parent)
        : cache_serializer_(new cache_serializer_t(io_backender, filename, stats_parent)),
            internal_(cache_serializer_->cache.get())
    { }

    disk_backed_queue_t(cache_t *cache, transaction_t *txn)
        : internal_(cache, txn)
    { }

    disk_backed_queue_t(cache_t *cache, block_id_t superblock_id)
        : internal_(cache, superblock_id)
    { }

    void push(const T &t) {
        write_message_t wm;
        wm << t;
        internal_.push(wm);
    }

    void pop(T *out) {
        std::vector<char> data_vec;

        internal_.pop(&data_vec);

        vector_read_stream_t read_stream(&data_vec);
        int res = deserialize(&read_stream, out);
        guarantee_err(res == 0, "corruption in disk-backed queue");
    }

    bool empty() {
        return internal_.empty();
    }

    int64_t size() {
        return internal_.size();
    }

private:
    scoped_ptr_t<cache_serializer_t> cache_serializer_;
    internal_disk_backed_queue_t internal_;
    DISABLE_COPYING(disk_backed_queue_t);
};

#endif /* CONTAINERS_DISK_BACKED_QUEUE_HPP_ */
