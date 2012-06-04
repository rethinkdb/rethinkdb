#ifndef __CONTAINERS_DISK_BACKED_QUEUE_HPP__
#define __CONTAINERS_DISK_BACKED_QUEUE_HPP__

#define MAX_REF_SIZE 251

#include "buffer_cache/blob.hpp"
#include "buffer_cache/mirrored/mirrored.hpp"
#include "buffer_cache/semantic_checking.hpp"
#include "containers/archive/vector_stream.hpp"
#include "serializer/config.hpp"
#include "serializer/types.hpp"

//TODO there are extra copies all over the place mostly stemming from having a
//vector<char> from the serialization code and strings from the blob code.

struct super_block_t {
    block_id_t tail, head;
    int count;
};

struct queue_block_t {
    block_id_t next;
    int data_size, live_data_offset;
    char data[0];
};

class disk_error_exception_t {
    //TODO this disk related exception is a stub you can help jdoliner by expanding it.
};

template <class T>
class disk_backed_write_queue_t {
public:
    disk_backed_write_queue_t(std::string filename) {
        /* We're going to register for writes, however those writes won't be able
         * to find their way in to the btree until we're done backfilling. Thus we
         * need to set up a serializer and cache for them to go in to. */
        //perfmon_collection_t backfill_stats_collection("queue-" + filename, NULL, true, true);

        //TODO this just making a file in the directory we were launched in, it
        //should be in our data directory. Also we should unlink it the moment we
        //create it.
        standard_serializer_t::create(
                standard_serializer_t::dynamic_config_t(),
                standard_serializer_t::private_dynamic_config_t(filename),
                standard_serializer_t::static_config_t()
                );

        serializer.reset(new standard_serializer_t(
            standard_serializer_t::dynamic_config_t(),
            standard_serializer_t::private_dynamic_config_t(filename),
            NULL
            ));

        /* Create the cache. */
        mirrored_cache_static_config_t cache_static_config;
        cache_t::create(serializer.get(), &cache_static_config);

        mirrored_cache_config_t cache_dynamic_config;
        cache_dynamic_config.max_size = MEGABYTE;
        cache_dynamic_config.max_dirty_size = MEGABYTE / 2;
        cache.reset(new cache_t(serializer.get(), &cache_dynamic_config, NULL));

        //grab a transaction
        transaction_t txn(cache.get(), rwi_write, 1, repli_timestamp_t::distant_past);

        buf_lock_t super_block(&txn, SUPERBLOCK_ID, rwi_write);
        super_block_t *sb = reinterpret_cast<super_block_t *>(super_block.get_data_major_write());
        sb->tail = sb->head = NULL_BLOCK_ID;
        sb->count = 0;
    }

    void push(const T &t) {
        //first we need a transaction
        transaction_t txn(cache.get(), rwi_write, 2, repli_timestamp_t::distant_past);
        buf_lock_t _super_block(&txn, SUPERBLOCK_ID, rwi_write);
        super_block_t *super_block = reinterpret_cast<super_block_t *>(_super_block.get_data_major_write());
        if (super_block->head == NULL_BLOCK_ID) {
            add_block_to_head(&txn, super_block);
        }

        rassert(super_block->head != NULL_BLOCK_ID);
        boost::scoped_ptr<buf_lock_t> _head(new buf_lock_t(&txn, super_block->head, rwi_write));
        queue_block_t* head = reinterpret_cast<queue_block_t *>(_head->get_data_major_write());

        write_message_t wm;
        wm << t;
        vector_stream_t stream;
        int res = send_write_message(&stream, &wm);
        rassert(res == 0);

        char buffer[MAX_REF_SIZE];
        bzero(buffer, MAX_REF_SIZE);

        blob_t blob(buffer, MAX_REF_SIZE);
        blob.append_region(&txn, stream.vector().size());
        std::string sered_data(stream.vector().begin(), stream.vector().end());
        blob.write_from_string(sered_data, &txn, 0);

        if (((char *)(head->data + head->data_size) - (char *)(head)) + blob.refsize(cache->get_block_size()) > cache->get_block_size().value()) {
            //The data won't fit in our current head block, so it's time to make a new one
            head = NULL;
            _head.reset();
            add_block_to_head(&txn, super_block);
            _head.reset(new buf_lock_t(&txn, super_block->head, rwi_write));
            head = reinterpret_cast<queue_block_t *>(_head->get_data_major_write());
        }

        memcpy(head->data + head->data_size, buffer, blob.refsize(cache->get_block_size()));
        head->data_size += blob.refsize(cache->get_block_size());
        super_block->count++;
    }
    T pop() {
        char buffer[MAX_REF_SIZE];
        transaction_t txn(cache.get(), rwi_write, 2, repli_timestamp_t::distant_past);
        buf_lock_t _super_block(&txn, SUPERBLOCK_ID, rwi_write);
        super_block_t *super_block = reinterpret_cast<super_block_t *>(_super_block.get_data_major_write());
        rassert(super_block->tail != NULL_BLOCK_ID);

        boost::scoped_ptr<buf_lock_t> _tail(new buf_lock_t(&txn, super_block->tail, rwi_write));
        queue_block_t *tail = reinterpret_cast<queue_block_t *>(_tail->get_data_major_write());
        rassert(tail->data_size != tail->live_data_offset);

        /* Grab the data from the blob and delete it. */

        memcpy(buffer, tail->data + tail->live_data_offset, blob::ref_size(cache->get_block_size(), tail->data + tail->live_data_offset, MAX_REF_SIZE));
        blob_t blob(buffer, MAX_REF_SIZE);
        std::string data;
        blob.read_to_string(data, &txn, 0, blob.valuesize());

        /* Record how far along in the blob we are. */
        tail->live_data_offset += blob.refsize(cache->get_block_size());

        blob.clear(&txn);

        super_block->count--;

        /* If that was the last blob in this block move on to the next one. */
        if (tail->live_data_offset == tail->data_size) {
            _tail.reset();
            remove_block_from_tail(&txn, super_block);
        }

        /* Deserialize the value and return it. */
        std::vector<char> data_vec(data.begin(), data.end());

        vector_read_stream_t read_stream(&data_vec);
        T t;
        int res = deserialize(&read_stream, &t);
        if (res != 0) {
            throw disk_error_exception_t();
        }
        return t;
    }

    bool empty() {
        transaction_t txn(cache.get(), rwi_read, 0, repli_timestamp_t::distant_past);
        buf_lock_t _super_block(&txn, SUPERBLOCK_ID, rwi_read);
        const super_block_t *super_block = reinterpret_cast<const super_block_t *>(_super_block.get_data_read());
        return (super_block->tail == NULL_BLOCK_ID);
    }

    int size() {
        transaction_t txn(cache.get(), rwi_read, 0, repli_timestamp_t::distant_past);
        buf_lock_t _super_block(&txn, SUPERBLOCK_ID, rwi_read);
        const super_block_t *super_block = reinterpret_cast<const super_block_t *>(_super_block.get_data_read());
        return super_block->count;
    }
private:
    void add_block_to_head(transaction_t *txn, super_block_t *super_block) {
        buf_lock_t _new_head(txn);
        queue_block_t *new_head = reinterpret_cast<queue_block_t *>(_new_head.get_data_major_write());
        if (super_block->head == NULL_BLOCK_ID) {
            rassert(super_block->tail == NULL_BLOCK_ID);
            super_block->head = super_block->tail = _new_head.get_block_id();
        } else {
            buf_lock_t _old_head(txn, super_block->head, rwi_write);
            queue_block_t *old_head = reinterpret_cast<queue_block_t *>(_old_head.get_data_major_write());
            rassert(old_head->next == NULL_BLOCK_ID);
            old_head->next = _new_head.get_block_id();
            super_block->head = _new_head.get_block_id();
        }

        new_head->next = NULL_BLOCK_ID;
        new_head->data_size = 0;
        new_head->live_data_offset = 0;
    }

    void remove_block_from_tail(transaction_t *txn, super_block_t *super_block) {
        rassert(super_block->tail != NULL_BLOCK_ID);
        buf_lock_t _old_tail(txn, super_block->tail, rwi_write);
        queue_block_t *old_tail = reinterpret_cast<queue_block_t *>(_old_tail.get_data_major_write());

        if (old_tail->next == NULL_BLOCK_ID) {
            rassert(super_block->head == _old_tail.get_block_id());
            super_block->tail = super_block->head = NULL_BLOCK_ID;
        } else {
            super_block->tail = old_tail->next;
        }

        _old_tail.mark_deleted();
    }
    boost::scoped_ptr<standard_serializer_t> serializer;
    boost::scoped_ptr<cache_t> cache;
};

#endif
