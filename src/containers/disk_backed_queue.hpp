#ifndef CONTAINERS_DISK_BACKED_QUEUE_HPP_
#define CONTAINERS_DISK_BACKED_QUEUE_HPP_

#define MAX_REF_SIZE 251

#include <string>
#include <vector>

#include "buffer_cache/types.hpp"
#include "concurrency/mutex.hpp"
#include "containers/archive/vector_stream.hpp"
#include "containers/scoped.hpp"
#include "serializer/types.hpp"

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
    internal_disk_backed_queue_t(io_backender_t *io_backender, const std::string& filename, perfmon_collection_t *stats_parent);
    ~internal_disk_backed_queue_t();

    void push(const write_message_t& value);

    void pop(std::vector<char> *buf_out);

    bool empty();

    int64_t size();

private:
    void add_block_to_head(transaction_t *txn);

    void remove_block_from_tail(transaction_t *txn);

    mutex_t mutex;
    int64_t queue_size;
    block_id_t head_block_id, tail_block_id;
    scoped_ptr_t<standard_serializer_t> serializer;
    scoped_ptr_t<cache_t> cache;

    DISABLE_COPYING(internal_disk_backed_queue_t);
};

template <class T>
class disk_backed_queue_t {
public:
    disk_backed_queue_t(io_backender_t *io_backender, const std::string& filename, perfmon_collection_t *stats_parent)
        : internal_(io_backender, filename, stats_parent) { }

    void push(const T &t) {
        write_message_t wm;
        wm << t;
        internal_.push(wm);
    }

    T pop() {
        std::vector<char> data_vec;

        internal_.pop(&data_vec);

        vector_read_stream_t read_stream(&data_vec);
        T t;
        int res = deserialize(&read_stream, &t);
        guarantee_err(res == 0, "corruption in disk-backed queue");
        return t;
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
