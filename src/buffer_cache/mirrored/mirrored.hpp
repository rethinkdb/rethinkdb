
#ifndef __MIRRORED_CACHE_HPP__
#define __MIRRORED_CACHE_HPP__

#include "arch/arch.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/access.hpp"
#include "concurrency/rwi_lock.hpp"
#include "concurrency/cond_var.hpp"
#include "buffer_cache/mirrored/callbacks.hpp"
#include "containers/two_level_array.hpp"
#include "serializer/serializer.hpp"
#include "serializer/translator.hpp"
#include "server/cmd_args.hpp"
#include "buffer_cache/stats.hpp"
#include "buffer_cache/buf_patch.hpp"
#include "buffer_cache/mirrored/patch_memory_storage.hpp"
#include "buffer_cache/mirrored/patch_disk_storage.hpp"
#include <boost/crc.hpp>

#include "writeback/writeback.hpp"

#include "page_repl/page_repl_random.hpp"
typedef page_repl_random_t page_repl_t;

#include "free_list.hpp"
typedef array_free_list_t free_list_t;

#include "page_map.hpp"
typedef array_map_t page_map_t;

// This cache doesn't actually do any operations itself. Instead, it
// provides a framework that collects all components of the cache
// (memory allocation, page lookup, page replacement, writeback, etc.)
// into a coherent whole. This allows easily experimenting with
// various components of the cache to improve performance.

#define MC_CONFIGBLOCK_ID (SUPERBLOCK_ID + 1)

class mc_inner_buf_t : public home_thread_mixin_t {
    friend class load_buf_fsm_t;
    friend class mc_cache_t;
    friend class mc_transaction_t;
    friend class mc_buf_t;
    friend class writeback_t;
    friend class writeback_t::local_buf_t;
    friend class writeback_t::concurrent_flush_t;
    friend class page_repl_random_t;
    friend class page_repl_random_t::local_buf_t;
    friend class array_map_t;
    friend class array_map_t::local_buf_t;
    friend class patch_disk_storage_t;

    typedef mc_cache_t cache_t;
    
    cache_t *cache;
    block_id_t block_id;
    repli_timestamp subtree_recency;
    void *data;
    rwi_lock_t lock;
    patch_counter_t next_patch_counter;

    /* The number of mc_buf_ts that exist for this mc_inner_buf_t */
    int refcount;

    /* true if we are being deleted */
    bool do_delete;
    bool write_empty_deleted_block;

    /* true if there is a mc_buf_t that holds a pointer to the data in read-only outdated-OK
    mode. */
    bool cow_will_be_needed;

    // Each of these local buf types holds a redundant pointer to the inner_buf that they are a part of
    writeback_t::local_buf_t writeback_buf;
    page_repl_t::local_buf_t page_repl_buf;
    page_map_t::local_buf_t page_map_buf;

    bool safe_to_unload();

    // Load an existing buf from disk
    mc_inner_buf_t(cache_t *cache, block_id_t block_id, bool should_load);

    // Load an existing buf but use the provided data buffer (for read ahead)
    mc_inner_buf_t(cache_t *cache, block_id_t block_id, void *buf);

    // Create an entirely new buf
    explicit mc_inner_buf_t(cache_t *cache);

    ~mc_inner_buf_t();

    ser_transaction_id_t transaction_id;

private:
    // Helper function for inner_buf construction from an existing block
    void replay_patches();
};

/* This class represents a hold on a mc_inner_buf_t. */
class mc_buf_t :
    public lock_available_callback_t
{
    typedef mc_cache_t cache_t;
    typedef mc_block_available_callback_t block_available_callback_t;

    friend class mc_cache_t;
    friend class mc_transaction_t;
    friend class patch_disk_storage_t;

private:
    mc_buf_t(mc_inner_buf_t *, access_t mode);
    void on_lock_available();
    bool ready;
    block_available_callback_t *callback;

    ticks_t start_time;

    access_t mode;
    bool non_locking_access;
    mc_inner_buf_t *inner_buf;
    void *data;   /* Same as inner_buf->data until a COW happens */

    ~mc_buf_t();

#ifndef FAST_PERFMON
    long int patches_affected_data_size_at_start;
#endif

public:
    void release();

    void apply_patch(buf_patch_t *patch); // This might delete the supplied patch, do not use patch after its application
    patch_counter_t get_next_patch_counter();

    const void *get_data_read() const {
        rassert(ready);
        return data;
    }
    // Use this only for writes which affect a large part of the block, as it bypasses the diff system
    void *get_data_major_write();
    // Convenience function to set some address in the buffer acquired through get_data_read. (similar to memcpy)
    void set_data(const void* dest, const void* src, const size_t n);
    // Convenience function to move data within the buffer acquired through get_data_read. (similar to memmove)
    void move_data(const void* dest, const void* src, const size_t n);

    // Makes sure the block itself gets flushed, instead of just the patch log
    void ensure_flush();

    block_id_t get_block_id() const {
        return inner_buf->block_id;
    }

    void mark_deleted(bool write_null = true) {
        rassert(mode == rwi_write);
        rassert(!inner_buf->safe_to_unload());
        inner_buf->do_delete = true;
        inner_buf->write_empty_deleted_block = write_null;
        ensure_flush(); // Disable patch log system for the buffer
    }

    void touch_recency() {
        // TODO: use some slice-specific timestamp that gets updated
        // every epoll call.
        inner_buf->subtree_recency = current_time();
        inner_buf->writeback_buf.set_recency_dirty();
    }

    bool is_dirty() {
        return inner_buf->writeback_buf.dirty;
    }
};

/* Transaction class. */
class mc_transaction_t :
    public intrusive_list_node_t<mc_transaction_t>,
    public writeback_t::sync_callback_t,
    public home_thread_mixin_t
{
    typedef mc_cache_t cache_t;
    typedef mc_buf_t buf_t;
    typedef mc_inner_buf_t inner_buf_t;
    typedef mc_transaction_begin_callback_t transaction_begin_callback_t;
    typedef mc_transaction_commit_callback_t transaction_commit_callback_t;
    typedef mc_block_available_callback_t block_available_callback_t;

    friend class mc_cache_t;
    friend class writeback_t;
    friend struct acquire_lock_callback_t;
    
public:
    cache_t *get_cache() const { return cache; }
    access_t get_access() const { return access; }

    bool commit(transaction_commit_callback_t *callback);

    buf_t *acquire(block_id_t block_id, access_t mode,
                   block_available_callback_t *callback, bool should_load = true);
    buf_t *allocate();
    repli_timestamp get_subtree_recency(block_id_t block_id);

    cache_t *cache;

private:
    explicit mc_transaction_t(cache_t *cache, access_t access, repli_timestamp recency_timestamp);
    ~mc_transaction_t();

    ticks_t start_time;

    void green_light();   // Called by the writeback when it's OK for us to start
    virtual void on_sync();

    access_t access;
    repli_timestamp recency_timestamp;
    transaction_begin_callback_t *begin_callback;
    transaction_commit_callback_t *commit_callback;
    enum { state_open, state_in_commit_call, state_committing, state_committed } state;
};

struct mc_cache_t :
    public home_thread_mixin_t,
    public translator_serializer_t::read_ahead_callback_t
{
    friend class load_buf_fsm_t;
    friend class mc_buf_t;
    friend class mc_inner_buf_t;
    friend class mc_transaction_t;
    friend class writeback_t;
    friend class writeback_t::local_buf_t;
    friend class writeback_t::concurrent_flush_t;
    friend class page_repl_random_t;
    friend class page_repl_random_t::local_buf_t;
    friend class array_map_t;
    friend class array_map_t::local_buf_t;
    friend class patch_disk_storage_t;

public:
    typedef mc_inner_buf_t inner_buf_t;
    typedef mc_buf_t buf_t;
    typedef mc_transaction_t transaction_t;
    typedef mc_block_available_callback_t block_available_callback_t;
    typedef mc_transaction_begin_callback_t transaction_begin_callback_t;
    typedef mc_transaction_commit_callback_t transaction_commit_callback_t;

private:

    mirrored_cache_config_t *dynamic_config;
    
    // TODO: how do we design communication between cache policies?
    // Should they all have access to the cache, or should they only
    // be given access to each other as necessary? The first is more
    // flexible as anyone can access anyone else, but encourages too
    // many dependencies. The second is more strict, but might not be
    // extensible when some policy implementation requires access to
    // components it wasn't originally given.

    translator_serializer_t *serializer;

    page_map_t page_map;
    page_repl_t page_repl;
    writeback_t writeback;
    free_list_t free_list;

public:
    static void create(translator_serializer_t *serializer, mirrored_cache_static_config_t *config);
    mc_cache_t(translator_serializer_t *serializer, mirrored_cache_config_t *dynamic_config);
    ~mc_cache_t();

public:
    block_size_t get_block_size();

    // Transaction API
    transaction_t *begin_transaction(access_t access, transaction_begin_callback_t *callback, repli_timestamp recency_timestamp);

private:
    void on_transaction_commit(transaction_t *txn);

    bool shutting_down;

    // Used to keep track of how many transactions there are so that we can wait for transactions to
    // complete before shutting down.
    int num_live_transactions;
    cond_t *to_pulse_when_last_transaction_commits;

private:
    patch_memory_storage_t patch_memory_storage;

    // Pointer, not member, because we need to call its destructor explicitly in our destructor
    boost::scoped_ptr<patch_disk_storage_t> patch_disk_storage;

    unsigned int max_patches_size_ratio;

public:
    void offer_read_ahead_buf(block_id_t block_id, void *buf);
private:
    bool offer_read_ahead_buf_home_thread(block_id_t block_id, void *buf);
};

#endif // __MIRRORED_CACHE_HPP__

