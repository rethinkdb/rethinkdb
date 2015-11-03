// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONTAINERS_DISK_BACKED_QUEUE_HPP_
#define CONTAINERS_DISK_BACKED_QUEUE_HPP_

#include <string>
#include <vector>

#include "concurrency/fifo_checker.hpp"
#include "concurrency/mutex.hpp"
#include "containers/buffer_group.hpp"
#include "containers/archive/buffer_group_stream.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/scoped.hpp"
#include "perfmon/core.hpp"
#include "serializer/types.hpp"

class cache_balancer_t;
class cache_conn_t;
class cache_t;
class txn_t;
class io_backender_t;
class perfmon_collection_t;

struct queue_block_t {
    block_id_t next;
    int32_t data_size, live_data_offset;
    char data[0];
} __attribute__((__packed__));

class value_acquisition_object_t;

class buffer_group_viewer_t {
public:
    virtual void view_buffer_group(const const_buffer_group_t *group) = 0;

protected:
    buffer_group_viewer_t() { }
    virtual ~buffer_group_viewer_t() { }

    DISABLE_COPYING(buffer_group_viewer_t);
};

class internal_disk_backed_queue_t {
public:
    internal_disk_backed_queue_t(io_backender_t *io_backender, const serializer_filepath_t& filename, perfmon_collection_t *stats_parent);
    ~internal_disk_backed_queue_t();

    void push(const write_message_t &value);
    void push(const scoped_array_t<write_message_t> &values);

    void pop(buffer_group_viewer_t *viewer);

    bool empty();

    int64_t size();

private:
    void add_block_to_head(txn_t *txn);
    void remove_block_from_tail(txn_t *txn);
    void push_single(txn_t *txn, const write_message_t &value);

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

    scoped_ptr_t<serializer_file_opener_t> file_opener;
    scoped_ptr_t<log_serializer_t> serializer;
    scoped_ptr_t<cache_balancer_t> balancer;
    scoped_ptr_t<cache_t> cache;
    scoped_ptr_t<cache_conn_t> cache_conn;

    DISABLE_COPYING(internal_disk_backed_queue_t);
};

template <class T>
class deserializing_viewer_t : public buffer_group_viewer_t {
public:
    explicit deserializing_viewer_t(T *value_out) : value_out_(value_out) { }
    virtual ~deserializing_viewer_t() { }

    virtual void view_buffer_group(const const_buffer_group_t *group) {
        // TODO: We assume here that the data was serialized by _other_ code using
        // LATEST -- some in disk_backed_queue_t::push, but also in btree_store.cc,
        // which uses internal_disk_backed_queue_t directly.  (There's no good reason
        // for this today: it needed to be generic when that code was templatized on
        // protocol_t.)
        deserialize_from_group<cluster_version_t::LATEST_OVERALL>(group, value_out_);
    }

private:
    T *value_out_;

    DISABLE_COPYING(deserializing_viewer_t);
};

// Copies the buffer group into a write_message_t
class copying_viewer_t : public buffer_group_viewer_t {
public:
    explicit copying_viewer_t(write_message_t *wm_out) : wm_out_(wm_out) { }
    ~copying_viewer_t() { }

    void view_buffer_group(const const_buffer_group_t *group) {
        buffer_group_read_stream_t stream(group);
        char buf[1024];
        while (!stream.entire_stream_consumed()) {
            int64_t c = stream.read(&buf, 1024);
            wm_out_->append(&buf, c);
        }
    }

private:
    write_message_t *wm_out_;

    DISABLE_COPYING(copying_viewer_t);
};

template <class T>
class disk_backed_queue_t {
public:
    disk_backed_queue_t(io_backender_t *io_backender, const serializer_filepath_t& filename, perfmon_collection_t *stats_parent)
        : internal_(io_backender, filename, stats_parent) { }

    void push(const T &t) {
        // TODO: There's an unnecessary copying of data here (which would require a
        // serialization_size overloaded function to be implemented in order to eliminate).
        // TODO: We have such a serialization_size function.
        write_message_t wm;
        // Despite that we are serializing this *to disk* disk backed
        // queues are not intended to persist across restarts, so this
        // is safe.
        serialize<cluster_version_t::LATEST_OVERALL>(&wm, t);
        internal_.push(wm);
    }

    void pop(T *out) {
        deserializing_viewer_t<T> viewer(out);
        internal_.pop(&viewer);
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
