#ifndef BUFFER_CACHE_SEMANTIC_CHECKING_HPP_
#define BUFFER_CACHE_SEMANTIC_CHECKING_HPP_

#include <algorithm>

#include "utils.hpp"
#include <boost/crc.hpp>

#include "buffer_cache/buf_patch.hpp"
#include "buffer_cache/mirrored/config.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/coro_fifo.hpp"
#include "concurrency/fifo_checker.hpp"
#include "concurrency/rwi_lock.hpp"
#include "containers/scoped.hpp"
#include "containers/two_level_array.hpp"
#include "perfmon/types.hpp"
#include "repli_timestamp.hpp"

// TODO: Have the semantic checking cache make sure that the
// repli_timestamp_ts are correct.

/* The semantic-checking cache (scc_cache_t) is a wrapper around another cache that will
make sure that the inner cache obeys the proper semantics. */

template<class inner_cache_t> class scc_buf_lock_t;
template<class inner_cache_t> class scc_transaction_t;
template<class inner_cache_t> class scc_cache_t;

typedef uint32_t crc_t;

class serializer_t;

/* Buf */

template<class inner_cache_t>
class scc_buf_lock_t {
public:
    scc_buf_lock_t(
        scc_transaction_t<inner_cache_t> *txn, block_id_t block_id, access_t mode,
        buffer_cache_order_mode_t order_mode = buffer_cache_order_mode_check,
        lock_in_line_callback_t *call_when_in_line = 0);
    explicit scc_buf_lock_t(scc_transaction_t<inner_cache_t> *txn);
    scc_buf_lock_t();
    ~scc_buf_lock_t();

    void swap(scc_buf_lock_t<inner_cache_t> &swapee);

    void release();
    void release_if_acquired();

    block_id_t get_block_id() const;
    const void *get_data_read() const;
    // Use this only for writes which affect a large part of the block, as it bypasses the diff system
    void *get_data_major_write();
    // Convenience function to set some address in the buffer acquired through get_data_read. (similar to memcpy)
    void set_data(void* dest, const void* src, const size_t n);
    // Convenience function to move data within the buffer acquired through get_data_read. (similar to memmove)
    void move_data(void* dest, const void* src, const size_t n);
    void apply_patch(buf_patch_t *patch); // This might delete the supplied patch, do not use patch after its application
    patch_counter_t get_next_patch_counter();
    void mark_deleted();
    void touch_recency(repli_timestamp_t timestamp);

    bool is_acquired() const;
    void ensure_flush();
    bool is_deleted() const;
    repli_timestamp_t get_recency() const;

private:
    bool snapshotted;
    bool has_been_changed;
    scoped_ptr_t<typename inner_cache_t::buf_lock_type> internal_buf_lock;
    scc_cache_t<inner_cache_t> *cache;
private:
    crc_t compute_crc() {
        boost::crc_optimal<32, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true> crc_computer;
        crc_computer.process_bytes(internal_buf_lock->get_data_read(), cache->get_block_size().value());
        return crc_computer.checksum();
    }
public:
    eviction_priority_t get_eviction_priority() {
        return internal_buf_lock->get_eviction_priority();
    }

    void set_eviction_priority(eviction_priority_t val) {
        internal_buf_lock->set_eviction_priority(val);
    }
};

/* Transaction */

template<class inner_cache_t>
class scc_transaction_t :
    public home_thread_mixin_t
{
public:
    scc_transaction_t(scc_cache_t<inner_cache_t> *cache, access_t access, int expected_change_count, repli_timestamp_t recency_timestamp);
    ~scc_transaction_t();

    // TODO: Implement semantic checking for snapshots!
    void snapshot() {
        snapshotted = true;
        inner_transaction.snapshot();
    }

    void set_account(typename inner_cache_t::cache_account_type *cache_account);

    void get_subtree_recencies(block_id_t *block_ids, size_t num_block_ids, repli_timestamp_t *recencies_out, get_subtree_recencies_callback_t *cb);

    scc_cache_t<inner_cache_t> *get_cache() const { return cache; }
    scc_cache_t<inner_cache_t> *cache;

    order_token_t order_token;

    void set_token(order_token_t token) { order_token = token; }

private:
    bool snapshotted; // Disables CRC checks

    friend class scc_buf_lock_t<inner_cache_t>;
    friend class scc_cache_t<inner_cache_t>;
    access_t access;
    typename inner_cache_t::transaction_type inner_transaction;
};

/* Cache */

template<class inner_cache_t>
class scc_cache_t : public home_thread_mixin_t, public serializer_read_ahead_callback_t {
public:
    typedef scc_buf_lock_t<inner_cache_t> buf_lock_type;
    typedef scc_transaction_t<inner_cache_t> transaction_type;
    typedef typename inner_cache_t::cache_account_type cache_account_type;

    static void create(
        serializer_t *serializer,
        mirrored_cache_static_config_t *static_config);
    scc_cache_t(serializer_t *serializer,
                mirrored_cache_config_t *dynamic_config,
                perfmon_collection_t *parent);

    block_size_t get_block_size();
    void create_cache_account(int priority, scoped_ptr_t<typename inner_cache_t::cache_account_type> *out);

    bool offer_read_ahead_buf(block_id_t block_id, void *buf, const intrusive_ptr_t<standard_block_token_t>& token, repli_timestamp_t recency_timestamp);
    bool contains_block(block_id_t block_id);

    coro_fifo_t& co_begin_coro_fifo() { return inner_cache.co_begin_coro_fifo(); }

private:
    inner_cache_t inner_cache;

private:
    friend class scc_transaction_t<inner_cache_t>;
    friend class scc_buf_lock_t<inner_cache_t>;

    /* CRC checking stuff */
    two_level_array_t<crc_t, MAX_BLOCK_ID> crc_map;
    /* order checking stuff */
    two_level_array_t<plain_sink_t, MAX_BLOCK_ID> sink_map;
};

#include "buffer_cache/semantic_checking.tcc"

#endif /* BUFFER_CACHE_SEMANTIC_CHECKING_HPP_ */

