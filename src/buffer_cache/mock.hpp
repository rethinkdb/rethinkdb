#ifndef BUFFER_CACHE_MOCK_HPP_
#define BUFFER_CACHE_MOCK_HPP_

#include <algorithm>

#include "buffer_cache/buf_patch.hpp"
#include "buffer_cache/mirrored/config.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/access.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/coro_fifo.hpp"
#include "concurrency/fifo_checker.hpp"
#include "concurrency/rwi_lock.hpp"
#include "containers/scoped.hpp"
#include "containers/segmented_vector.hpp"
#include "repli_timestamp.hpp"
#include "serializer/types.hpp"
#include "utils.hpp"

/* The mock cache, mock_cache_t, is a drop-in replacement for mc_cache_t that keeps all of
its contents in memory and artificially generates delays in responding to requests. It
should not be used in production; its purpose is to help catch bugs in the btree code. */

class mock_buf_lock_t;
class mock_cache_t;
class mock_cache_account_t;
class mock_transaction_t;
class internal_buf_t;
class serializer_t;
class perfmon_collection_t;

/* Buf */

class mock_buf_lock_t : public home_thread_mixin_t {
public:
    mock_buf_lock_t(
            mock_transaction_t *txn, block_id_t block_id, access_t mode,
            buffer_cache_order_mode_t order_mode = buffer_cache_order_mode_check,
            lock_in_line_callback_t *call_when_in_line = 0);
    explicit mock_buf_lock_t(mock_transaction_t *txn);
    mock_buf_lock_t();
    ~mock_buf_lock_t();

    void swap(mock_buf_lock_t &swapee);

    void release();
    void release_if_acquired();

    block_id_t get_block_id() const;
    const void *get_data_read() const;
    // Use this only for writes which affect a large part of the block, as it bypasses the diff system
    void *get_data_major_write();
    // Convenience function to set some address in the buffer acquired through get_data_read. (similar to memcpy)
    void set_data(void *dest, const void *src, const size_t n);
    // Convenience function to move data within the buffer acquired through get_data_read. (similar to memmove)
    void move_data(void *dest, const void *src, const size_t n);
    void apply_patch(buf_patch_t *patch); // This might delete the supplied patch, do not use patch after its application
    patch_counter_t get_next_patch_counter();
    void mark_deleted();
    void touch_recency(repli_timestamp_t timestamp);

    bool is_acquired() const;
    void ensure_flush();
    bool is_deleted() const;
    repli_timestamp_t get_recency() const;

public:
    eviction_priority_t get_eviction_priority() {
        // Mock cache does not implement eviction priorities
        return ZERO_EVICTION_PRIORITY;
    }

    void set_eviction_priority(UNUSED eviction_priority_t val) {
        // Mock cache does not implement eviction priorities
    }


private:
    friend class mock_transaction_t;
    friend class mock_cache_t;
    friend class internal_buf_t;
    internal_buf_t *internal_buf;
    access_t access;
    bool dirty, deleted;
    bool acquired;
};

class mock_transaction_t :
    public home_thread_mixin_t
{
    typedef mock_buf_lock_t buf_lock_t;

public:
    mock_transaction_t(mock_cache_t *cache, access_t access, int expected_change_count, repli_timestamp_t recency_timestamp, order_token_t order_token = order_token_t::ignore);
    ~mock_transaction_t();

    void snapshot() { }

    void set_account(UNUSED mock_cache_account_t *cache_account) { }

    void get_subtree_recencies(block_id_t *block_ids, size_t num_block_ids, repli_timestamp_t *recencies_out, get_subtree_recencies_callback_t *cb);

    mock_cache_t *get_cache() const { return cache; }
    mock_cache_t *cache;

    const order_token_t order_token;

private:
    friend class mock_buf_lock_t;
    friend class mock_cache_t;
    access_t access;
    int n_bufs;
    repli_timestamp_t recency_timestamp;
    auto_drainer_t::lock_t keepalive;
};

/* Cache */

class mock_cache_account_t {
    friend class mock_cache_t;
    mock_cache_account_t() { }
    DISABLE_COPYING(mock_cache_account_t);
};

class mock_cache_t : public home_thread_mixin_t, public serializer_read_ahead_callback_t {
public:
    typedef mock_buf_lock_t buf_lock_type;
    typedef mock_transaction_t transaction_type;
    typedef mock_cache_account_t cache_account_type;

    static void create(serializer_t *serializer, mirrored_cache_static_config_t *static_config);
    mock_cache_t(serializer_t *serializer, mirrored_cache_config_t *dynamic_config, perfmon_collection_t *parent);
    ~mock_cache_t();

    block_size_t get_block_size();

    void create_cache_account(UNUSED int priority, scoped_ptr_t<mock_cache_account_t> *out) {
        out->init(new mock_cache_account_t());
    }

    bool offer_read_ahead_buf(block_id_t block_id, void *buf, const intrusive_ptr_t<standard_block_token_t>& token, repli_timestamp_t recency_timestamp);

    bool contains_block(block_id_t id);

private:
    friend class mock_transaction_t;
    friend class mock_buf_lock_t;
    friend class internal_buf_t;

    serializer_t *serializer;
    scoped_ptr_t<auto_drainer_t> transaction_counter;
    block_size_t block_size;

    // Makes sure that write operations do not get reordered, which
    // throttling is supposed to do in the real buffer cache.
    coro_fifo_t write_operation_random_delay_fifo;

    scoped_ptr_t< segmented_vector_t<internal_buf_t *, MAX_BLOCK_ID> > bufs;

    coro_fifo_t transaction_constructor_coro_fifo_;
};

#endif /* BUFFER_CACHE_MOCK_HPP_ */
