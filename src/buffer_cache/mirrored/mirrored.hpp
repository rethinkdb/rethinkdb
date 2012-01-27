#ifndef __MIRRORED_CACHE_HPP__
#define __MIRRORED_CACHE_HPP__

#include <map>

#include "errors.hpp"
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "arch/types.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/access.hpp"
#include "concurrency/coro_fifo.hpp"
#include "concurrency/fifo_checker.hpp"
#include "concurrency/rwi_lock.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/mutex.hpp"
#include "containers/intrusive_list.hpp"
#include "containers/two_level_array.hpp"
#include "buffer_cache/mirrored/config.hpp"
#include "buffer_cache/stats.hpp"
#include "buffer_cache/buf_patch.hpp"
#include "buffer_cache/mirrored/patch_memory_storage.hpp"
#include "buffer_cache/mirrored/patch_disk_storage.hpp"

#include "buffer_cache/mirrored/writeback.hpp"

#include "buffer_cache/mirrored/page_repl_random.hpp"
typedef page_repl_random_t page_repl_t;

#include "buffer_cache/mirrored/free_list.hpp"

#include "buffer_cache/mirrored/page_map.hpp"


class mc_cache_account_t;

// evictable_t must go before array_map_t::local_buf_t, which
// references evictable_t's cache field.
class mc_inner_buf_t : public evictable_t,
		       private writeback_t::local_buf_t, /* This local_buf_t has state used by the writeback. */
		       public home_thread_mixin_t {
    friend class mc_cache_t;
    friend class mc_transaction_t;
    friend class mc_buf_t;
    friend class writeback_t;
    friend class writeback_t::local_buf_t;
    friend class page_repl_random_t;
    friend class array_map_t;
    friend class patch_disk_storage_t;

    typedef mc_cache_t cache_t;

    typedef uint64_t version_id_t;



    class buf_snapshot_t;



    static const version_id_t faux_version_id = 0;  // this version id must be smaller than any valid version id

    // writeback_t::local_buf_t used to be a field, not a privately
    // inherited superclass, so this is the proper way to access its
    // fields.
    writeback_t::local_buf_t& writeback_buf() { return *this; }

    // Functions of the evictable_t interface.
    bool safe_to_unload();
    void unload();

    // Load an existing buf from disk
    mc_inner_buf_t(cache_t *cache, block_id_t block_id, bool should_load, file_account_t *io_account);

    // Load an existing buf but use the provided data buffer (for read ahead)
    mc_inner_buf_t(cache_t *cache, block_id_t block_id, void *buf, const boost::intrusive_ptr<standard_block_token_t>& token, repli_timestamp_t recency_timestamp);

    // Create an entirely new buf
    static mc_inner_buf_t *allocate(cache_t *cache, version_id_t snapshot_version, repli_timestamp_t recency_timestamp);
    mc_inner_buf_t(cache_t *cache, block_id_t block_id, version_id_t snapshot_version, repli_timestamp_t recency_timestamp);
    ~mc_inner_buf_t();

    // Loads data from the serializer.
    void load_inner_buf(bool should_lock, file_account_t *io_account);

    // Informs us that a certain data buffer (whether the current one or one used by a
    // buf_snapshot_t) has been written back to disk; used by writeback
    void update_data_token(const void *data, const boost::intrusive_ptr<standard_block_token_t>& token);

    // If required, make a snapshot of the data before being overwritten with new_version
    bool snapshot_if_needed(version_id_t new_version, bool leave_clone);
    // releases a buffer snapshot used by a transaction snapshot
    void release_snapshot(buf_snapshot_t *snapshot);
    // acquires the snapshot data buffer, loading from disk if necessary; must be matched by a call
    // to release_snapshot_data to keep track of when data buffer is in use
    void *acquire_snapshot_data(version_id_t version_to_access, file_account_t *io_account, repli_timestamp_t *recency_out);
    void release_snapshot_data(void *data);

private:
    // Helper function for inner_buf construction from an existing block
    void replay_patches();

    // Our block's block id.
    block_id_t block_id;

    // The subtree recency value associated with our block.
    repli_timestamp_t subtree_recency;

    // The data for the block.
    serializer_data_ptr_t data;
    // The snapshot version id of the block.
    version_id_t version_id;
    /* As long as data has not been changed since the last serializer write, data_token contains a token to the on-serializer block */
    boost::intrusive_ptr<standard_block_token_t> data_token;

    // A lock for loading the block.
    rwi_lock_t lock;
    // A patch counter that belongs to this block.
    patch_counter_t next_patch_counter;

    // The number of mc_buf_ts that exist for this mc_inner_buf_t.
    unsigned int refcount;

    // true if this block is to be deleted.
    bool do_delete;

    // number of references from mc_buf_t buffers, which hold a
    // pointer to the data in read_outdated_ok mode.
    size_t cow_refcount;

    // number of references from mc_buf_t buffers which point to the current version of `data` as a
    // snapshot. this is ugly, but necessary to correctly initialize buf_snapshot_t refcounts.
    size_t snap_refcount;

    // This is used to figure out what patches still need to be
    // applied.
    block_sequence_id_t block_sequence_id;

    // snapshot types' implementations are internal and deferred to mirrored.cc
    intrusive_list_t<buf_snapshot_t> snapshots;

    DISABLE_COPYING(mc_inner_buf_t);
};



/* This class represents a hold on a mc_inner_buf_t (and potentially a specific version of one). */
class mc_buf_t {
    typedef mc_cache_t cache_t;

    friend class mc_cache_t;
    friend class mc_transaction_t;
    friend class patch_disk_storage_t;
    friend class writeback_t;
    friend class writeback_t::buf_writer_t;

private:
    // io_account is only used if a snapshot needs to be read from disk. it may safely be
    // DEFAULT_DISK_ACCOUNT if snapshotted is false. TODO: this is ugly. find a cleaner interface.
    mc_buf_t(mc_inner_buf_t *inner, access_t mode, mc_inner_buf_t::version_id_t version_id, bool snapshotted,
             boost::function<void()> call_when_in_line, file_account_t *io_account = DEFAULT_DISK_ACCOUNT);
    void acquire_block(mc_inner_buf_t::version_id_t version_to_access);

    ~mc_buf_t();


public:
    void release();
    bool is_deleted() { return data == NULL; }

    void apply_patch(buf_patch_t *patch); // This might delete the supplied patch, do not use patch after its application
    patch_counter_t get_next_patch_counter();

    const void *get_data_read() const {
        rassert(data);
        return data;
    }
    // Use this only for writes which affect a large part of the block, as it bypasses the diff system
    void *get_data_major_write();
    // Convenience function to set some address in the buffer acquired through get_data_read. (similar to memcpy)
    void set_data(void *dest, const void *src, size_t n);
    // Convenience function to move data within the buffer acquired through get_data_read. (similar to memmove)
    void move_data(void* dest, const void* src, size_t n);

    // Makes sure the block itself gets flushed, instead of just the patch log
    void ensure_flush();

    block_id_t get_block_id() const {
        return inner_buf->block_id;
    }

    void mark_deleted();

    void touch_recency(repli_timestamp_t timestamp) {
        rassert(mode == rwi_write);

        // Some operations acquire in write mode but should not
        // actually affect subtree recency.  For example, delete
        // operations, and delete_expired operations -- the subtree
        // recency is an upper bound of the maximum timestamp of all
        // the subtree's keys, and this cannot get affected by a
        // delete operation.  (It is a bit ghetto that we use
        // repli_timestamp_t invalid as a magic constant that indicates
        // this fact, instead of using something robust.  We are so
        // prone to using that value as a placeholder.)
        if (timestamp != repli_timestamp_t::invalid) {
            // TODO: Add rassert(inner_buf->subtree_recency <= timestamp)

            // TODO: use some slice-specific timestamp that gets updated
            // every epoll call.
            inner_buf->subtree_recency = timestamp;
            subtree_recency = timestamp;
            inner_buf->writeback_buf().set_recency_dirty();
        }
    }

    repli_timestamp_t get_recency() {
        // TODO: Make it possible to get the recency _without_
        // acquiring the buf.  The recency should be locked with a
        // lock that sits "above" buffer acquisition.  Or the recency
        // simply should be set _before_ trying to acquire the buf.
        return subtree_recency;
    }

public:
    int get_eviction_priority() {
        return inner_buf->eviction_priority;
    }

    void set_eviction_priority(int val) {
        inner_buf->eviction_priority = val;
    }

private:
    // Used for the pm_bufs_held perfmon.
    ticks_t start_time;

    // Presumably, the mode with which this mc_buf_t holds the inner buf.
    access_t mode;

    // True if this is an mc_buf_t for a snapshotted view of the buf.
    const bool snapshotted;

    // non_locking_access is a hack for the sake of patch_disk_storage.cc. It would be nice if we
    // could eliminate it.
    bool non_locking_access;

    // Used for perfmon, measuring how much the patches' serialized
    // size changed.  TODO: Maybe this could be a uint16_t.
    int32_t patches_serialized_size_at_start;

    // Our pointer to an inner_buf -- we have a bunch of mc_buf_t's
    // all pointing at an inner buf.
    mc_inner_buf_t *inner_buf;

    // Usually the same as inner_buf->data. If a COW happens or this
    // mc_buf_t is part of a snapshotted transaction, it reference a
    // different buffer however.
    void *data;

    // Similarly, usually the same as inner_buf->subtree_recency.  If
    // a COW happens or this mc_buf_t is part of a snapshotted
    // transaction, it may have a different value.
    repli_timestamp_t subtree_recency;

    DISABLE_COPYING(mc_buf_t);
};


struct i_am_writeback_t { };

class sequence_group_t;

/* Transaction class. */
class mc_transaction_t :
    public home_thread_mixin_t
{
    typedef mc_cache_t cache_t;
    typedef mc_buf_t buf_t;
    typedef mc_inner_buf_t inner_buf_t;
    typedef mc_cache_account_t cache_account_t;

    friend class mc_cache_t;
    friend class writeback_t;

public:
    mc_transaction_t(cache_t *cache, sequence_group_t *seq_group, access_t access, int expected_change_count, repli_timestamp_t recency_timestamp);
    mc_transaction_t(cache_t *cache, sequence_group_t *seq_group, access_t access, int fook, bool dont_assert_about_shutting_down = false);   // Not for use with write transactions
    mc_transaction_t(cache_t *cache, access_t access, i_am_writeback_t i_am_writeback);
    ~mc_transaction_t();

    cache_t *get_cache() const { return cache; }
    access_t get_access() const { return access; }

    /* `acquire()` acquires the block indicated by `block_id`. `mode` specifies whether
    we want read or write access. `acquire()` blocks until the block has been acquired.
    If `call_when_in_line` is non-zero, it will be a function to call once we have
    gotten in line for the block but before `acquire()` actually returns. If `should_load`
    is false, we will not bother loading the block from disk if it isn't in memory; this
    is useful if we intend to delete or rewrite the block without accessing its previous
    contents. */
    buf_t *acquire(block_id_t block_id, access_t mode, boost::function<void()> call_when_in_line = 0, bool should_load = true);

    buf_t *allocate();

    void get_subtree_recencies(block_id_t *block_ids, size_t num_block_ids, repli_timestamp_t *recencies_out, get_subtree_recencies_callback_t *cb);

    // This just sets the snapshotted flag, we finalize the snapshot as soon as the first block has been acquired (see finalize_version() )
    void snapshot();

    void set_account(const boost::shared_ptr<cache_account_t>& cache_account);

    // Order tokens are only actually stored by semantic checking and mock caches.
    void set_token(UNUSED order_token_t token) { }

private:
    void register_buf_snapshot(mc_inner_buf_t *inner_buf, mc_inner_buf_t::buf_snapshot_t *snap);

    // If not done before, sets snapshot_version, if in snapshotted mode also registers the snapshot
    void maybe_finalize_version();

    file_account_t *get_io_account() const;

    // Note: Make sure that no automatic destructors do anything
    // interesting, they could get run on the WRONG THREAD!
    cache_t *cache;

    ticks_t start_time;
    const int expected_change_count;
    access_t access;
    repli_timestamp_t recency_timestamp;
    mc_inner_buf_t::version_id_t snapshot_version;
    bool snapshotted;

    // This is manually reset() in the destructor.
    boost::shared_ptr<cache_account_t> cache_account_;

    typedef std::vector<std::pair<mc_inner_buf_t*, mc_inner_buf_t::buf_snapshot_t*> > owned_snapshots_list_t;
    owned_snapshots_list_t owned_buf_snapshots;

    DISABLE_COPYING(mc_transaction_t);
};



class mc_cache_account_t {
public:
    ~mc_cache_account_t();
private:
    friend class mc_cache_t;
    friend class mc_transaction_t;
    mc_cache_account_t(int thread, file_account_t *io_account);
    int thread_;
    file_account_t *io_account_;
    DISABLE_COPYING(mc_cache_account_t);
};



class mc_cache_t : public home_thread_mixin_t, public serializer_read_ahead_callback_t {
    friend class mc_buf_t;
    friend class mc_inner_buf_t;
    friend class mc_transaction_t;
    friend class writeback_t;
    friend class writeback_t::local_buf_t;
    friend class page_repl_random_t;
    friend class evictable_t;
    friend class array_map_t;
    friend class patch_disk_storage_t;

public:
    typedef mc_inner_buf_t inner_buf_t;
    typedef mc_buf_t buf_t;
    typedef mc_transaction_t transaction_t;
    typedef mc_cache_account_t cache_account_t;

    // For sequence groups to access the right fifo.  This is a bit of a HACK.
    int get_slice_num() const { return slice_num; }

    static void create(serializer_t *serializer, mirrored_cache_static_config_t *config);
    mc_cache_t(serializer_t *serializer, mirrored_cache_config_t *dynamic_config, int this_slice_num);
    ~mc_cache_t();

    block_size_t get_block_size();

    // TODO: Come up with a consistent priority scheme, i.e. define a "default" priority etc.
    // TODO: As soon as we can support it, we might consider supporting a mem_cap paremeter.
    boost::shared_ptr<cache_account_t> create_account(int priority);

    bool contains_block(block_id_t block_id);

    mc_inner_buf_t::version_id_t get_current_version_id() { return next_snapshot_version; }

    // must be O(1)
    mc_inner_buf_t::version_id_t get_min_snapshot_version(mc_inner_buf_t::version_id_t default_version) const {
        return no_active_snapshots() ? default_version : (*active_snapshots.begin()).first;
    }
    // must be O(1)
    mc_inner_buf_t::version_id_t get_max_snapshot_version(mc_inner_buf_t::version_id_t default_version) const {
        return no_active_snapshots() ? default_version : (*active_snapshots.rbegin()).first;
    }

    void register_snapshot(mc_transaction_t *txn);
    void unregister_snapshot(mc_transaction_t *txn);

private:
    bool no_active_snapshots() const { return active_snapshots.empty(); }
    bool no_active_snapshots(mc_inner_buf_t::version_id_t from_version, mc_inner_buf_t::version_id_t to_version) const {
        return active_snapshots.lower_bound(from_version) == active_snapshots.upper_bound(to_version);
    }

    size_t register_buf_snapshot(mc_inner_buf_t *inner_buf, mc_inner_buf_t::buf_snapshot_t *snap, mc_inner_buf_t::version_id_t snapshotted_version, mc_inner_buf_t::version_id_t new_version);
    size_t calculate_snapshots_affected(mc_inner_buf_t::version_id_t snapshotted_version, mc_inner_buf_t::version_id_t new_version);

    inner_buf_t *find_buf(block_id_t block_id);
    void on_transaction_commit(transaction_t *txn);



public:
    bool offer_read_ahead_buf(block_id_t block_id, void *buf, const boost::intrusive_ptr<standard_block_token_t>& token, repli_timestamp_t recency_timestamp);

private:
    void offer_read_ahead_buf_home_thread(block_id_t block_id, void *buf, const boost::intrusive_ptr<standard_block_token_t>& token, repli_timestamp_t recency_timestamp);
    bool can_read_ahead_block_be_accepted(block_id_t block_id);
    void maybe_unregister_read_ahead_callback();

public:
    coro_fifo_t& co_begin_coro_fifo() { return co_begin_coro_fifo_; }

private:
    // Which slice this cache is for.  Used in get_slice_num or co_begin_transaction.
    const int slice_num;

    mirrored_cache_config_t dynamic_config; // Local copy of our initial configuration

    // TODO: how do we design communication between cache policies?
    // Should they all have access to the cache, or should they only
    // be given access to each other as necessary? The first is more
    // flexible as anyone can access anyone else, but encourages too
    // many dependencies. The second is more strict, but might not be
    // extensible when some policy implementation requires access to
    // components it wasn't originally given.

    serializer_t *serializer;

    // We use a separate IO account for reads and writes, so reads can pass ahead
    // of active writebacks. Otherwise writebacks could badly block out readers,
    // thereby blocking user queries.
    boost::scoped_ptr<file_account_t> reads_io_account;
    boost::scoped_ptr<file_account_t> writes_io_account;

    array_map_t page_map;
    page_repl_t page_repl;
    writeback_t writeback;
    array_free_list_t free_list;

    // More fields
    bool shutting_down;
#ifndef NDEBUG
    bool writebacks_allowed;
#endif

    // Used to keep track of how many transactions there are so that we can wait for transactions to
    // complete before shutting down.
    int num_live_transactions;
    cond_t *to_pulse_when_last_transaction_commits;

    patch_memory_storage_t patch_memory_storage;

    // Pointer, not member, because we need to call its destructor explicitly in our destructor
    boost::scoped_ptr<patch_disk_storage_t> patch_disk_storage;

    // The ratio of block size to patch size (for some block id, at
    // some point in time) at which we think it's worth it to flush
    // the whole block and drop the patch history.
    unsigned int max_patches_size_ratio;

    unsigned int get_max_patches_size_ratio() const { return max_patches_size_ratio; }
    // Functions that adjust this ratio up and down, between
    // MAX_PATCHES_SIZE_RATIO_MIN and MAX_PATCHES_SIZE_RATIO_MAX, for
    // use based on whether we are bottlenecking on I/O.  Note that
    // increasing the field's value lowers the proportion of block
    // size at which we would drop the patches.
    void adjust_max_patches_size_ratio_toward_minimum();
    void adjust_max_patches_size_ratio_toward_maximum();

    bool read_ahead_registered;

    typedef std::map<mc_inner_buf_t::version_id_t, mc_transaction_t*> snapshots_map_t;
    snapshots_map_t active_snapshots;
    mc_inner_buf_t::version_id_t next_snapshot_version;

    coro_fifo_t co_begin_coro_fifo_;

    DISABLE_COPYING(mc_cache_t);
};

#endif // __MIRRORED_CACHE_HPP__
