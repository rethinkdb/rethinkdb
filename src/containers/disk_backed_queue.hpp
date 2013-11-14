// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONTAINERS_DISK_BACKED_QUEUE_HPP_
#define CONTAINERS_DISK_BACKED_QUEUE_HPP_

#define MAX_REF_SIZE 251

#include <string>
#include <vector>

#define DBQ_USE_ALT_CACHE 1

#if !DBQ_USE_ALT_CACHE
#include "buffer_cache/types.hpp"
#endif
#include "concurrency/fifo_checker.hpp"
#include "concurrency/mutex.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/scoped.hpp"
#include "perfmon/core.hpp"
#include "serializer/types.hpp"

#if DBQ_USE_ALT_CACHE
namespace alt {
class alt_cache_t;
class alt_txn_t;
}
#endif
class io_backender_t;
class perfmon_collection_t;

//TODO there are extra copies all over the place mostly stemming from having a
//vector<char> from the serialization code and strings from the blob code.

struct queue_block_t {
    block_id_t next;
    int data_size, live_data_offset;
    char data[0];
};

class internal_disk_backed_queue_t {
public:
    internal_disk_backed_queue_t(io_backender_t *io_backender, const serializer_filepath_t& filename, perfmon_collection_t *stats_parent);
    ~internal_disk_backed_queue_t();

    // TODO: order_token_t::ignore.  This should take an order token and store it.
    void push(const write_message_t &value);

    // TODO: order_token_t::ignore.  This should output an order token (that was passed in to push).
    void pop(std::vector<char> *buf_out);

    bool empty();

    int64_t size();

private:
#if DBQ_USE_ALT_CACHE
    void add_block_to_head(alt::alt_txn_t *txn);
    void remove_block_from_tail(alt::alt_txn_t *txn);
#else
    void add_block_to_head(transaction_t *txn);
    void remove_block_from_tail(transaction_t *txn);
#endif

    mutex_t mutex;

    // Serves more as sanity-checking for the cache than this type's ordering.
    order_source_t cache_order_source;
    perfmon_collection_t perfmon_collection;
    perfmon_membership_t perfmon_membership;

    int64_t queue_size;

    // The end we push onto.
    block_id_t head_block_id;
    // The end we pop from.
    block_id_t tail_block_id;
    scoped_ptr_t<standard_serializer_t> serializer;
#if DBQ_USE_ALT_CACHE
    scoped_ptr_t<alt::alt_cache_t> cache;
#else
    scoped_ptr_t<cache_t> cache;
#endif

    DISABLE_COPYING(internal_disk_backed_queue_t);
};

template <class T>
class disk_backed_queue_t {
public:
    disk_backed_queue_t(io_backender_t *io_backender, const serializer_filepath_t& filename, perfmon_collection_t *stats_parent)
        : internal_(io_backender, filename, stats_parent) { }

    void push(const T &t) {
        // TODO: There's an unnecessary copying of data here (which would require a
        // serialization_size overloaded function to be implemented in order to eliminate).
        write_message_t wm;
        wm << t;
        internal_.push(wm);
    }

    void pop(T *out) {
        // TODO: There's an unnecessary copying of data here.
        std::vector<char> data_vec;

        internal_.pop(&data_vec);

        vector_read_stream_t read_stream(&data_vec);
        archive_result_t res = deserialize(&read_stream, out);
        guarantee_deserialization(res, "disk backed queue");
    }

    bool empty() {
        return internal_.empty();
    }

    int64_t size() {
        return internal_.size();
    }

private:
    internal_disk_backed_queue_t internal_;
    DISABLE_COPYING(disk_backed_queue_t);
};

#endif /* CONTAINERS_DISK_BACKED_QUEUE_HPP_ */
