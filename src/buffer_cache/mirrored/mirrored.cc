// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "buffer_cache/mirrored/mirrored.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/arch.hpp"
#include "do_on_thread.hpp"
#include "serializer/serializer.hpp"

/**
 * Buffer implementation.
 */

// Types of snapshots

// In order for snapshots to get deleted properly, they must obey the invariant that, after
// get_data() returns or get_data_if_available() returns non-NULL, get_data_if_available() will
// return the same (non-NULL) value.

// TODO(rntz): it should be possible for us to cause snapshots which were not cow-referenced to be
// flushed to disk during writeback, sans block id, to allow them to be unloaded if necessary.
class mc_inner_buf_t::buf_snapshot_t : private evictable_t, public intrusive_list_node_t<mc_inner_buf_t::buf_snapshot_t> {
public:
    buf_snapshot_t(mc_inner_buf_t *buf,
                   size_t _snapshot_refcount, size_t _active_refcount,
                   bool leave_clone)
        : evictable_t(buf->cache, /* TODO: we can load the data later and we never get added to the page map */ buf->data.has() ? true : false),
          parent(buf), snapshotted_version(buf->version_id),
          token(buf->data_token),
          subtree_recency(buf->subtree_recency),
          snapshot_refcount(_snapshot_refcount), active_refcount(_active_refcount) {
        cache->assert_thread();

        if (leave_clone) {
            if (buf->data.has()) {
                data.swap(buf->data);
                buf->data.init_clone(buf->cache->serializer, data);
            }
        } else {
            data.swap(buf->data);
        }

        rassert(data.has() || token, "creating buf snapshot without data or block token");
        rassert(snapshot_refcount + active_refcount, "creating buf snapshot with 0 refcount");
    }

private:
    ~buf_snapshot_t() {
        cache->assert_thread();
        rassert(!snapshot_refcount && !active_refcount);
        parent->snapshots.remove(this);
        if (data.has()) {
            data.free(cache->serializer);
        }

        if (in_page_repl()) {
            remove_from_page_repl();
        }
    }

public:
    void *acquire_data(file_account_t *io_account) {
        cache->assert_thread();
        ++active_refcount;

        mutex_t::acq_t m(&data_mutex);
        // The buffer might have already been loaded.
        if (data.has()) {
            return data.get();
        }
        rassert(token, "buffer snapshot lacks both token and data");


        // Use a temporary to avoid putting our data member in an allocated-but-uninitialized state.
        serializer_data_ptr_t tmp;
        {
            on_thread_t th(cache->serializer->home_thread());
            tmp.init_malloc(cache->serializer);
            cache->serializer->block_read(token, tmp.get(), io_account);
        }
        rassert(!data.has(), "data changed while holding mutex");
        data.swap(tmp);

        return data.get();
    }

    void release_data() {
        cache->assert_thread();
        rassert(active_refcount, "releasing snapshot data with no active references");
        --active_refcount;
        if (0 == active_refcount + snapshot_refcount) {
            delete this;
        }
    }

    void release() {
        cache->assert_thread();
        rassert(snapshot_refcount, "releasing snapshot with no references");
        --snapshot_refcount;
        if (0 == snapshot_refcount + active_refcount) {
            delete this;
        }
    }

    // We are safe to unload if we are saved to disk and have no mc_buf_lock_ts actively referencing us.
    bool safe_to_unload() {
        cache->assert_thread();
        return token && !active_refcount;
    }

    void unload() {
        cache->assert_thread();
        // Acquiring the mutex isn't necessary here because active_refcount == 0.
        rassert(safe_to_unload());
        rassert(data.has());
        data.free(cache->serializer);
    }

private:
    friend class mc_inner_buf_t;

    // I guess we know what this means.
    mc_inner_buf_t *parent;

    // Some kind of snapshotted version.  :/
    version_id_t snapshotted_version;

    mutable mutex_t data_mutex;

    // The buffer of the snapshot we hold.
    serializer_data_ptr_t data;

    // Our block token to the serializer.
    intrusive_ptr_t<standard_block_token_t> token;

    // The recency of the snapshot we hold.
    repli_timestamp_t subtree_recency;

    // snapshot_refcount is the number of snapshots that could potentially use this buf_snapshot_t.
    size_t snapshot_refcount;

    // active_refcount is the number of mc_buf_lock_ts currently using this buf_snapshot_t. As long as
    // >0, we cannot be unloaded.
    size_t active_refcount;

    DISABLE_COPYING(buf_snapshot_t);
};

// This loads a block from the serializer and stores it into buf.
void mc_inner_buf_t::load_inner_buf(bool should_lock, file_account_t *io_account) {
    if (should_lock) {
        DEBUG_VAR bool locked = lock.lock(rwi_write, NULL);
        rassert(locked);
    } else {
        // We should have at least *some* kind of lock on the buffer, shouldn't we?
        rassert(lock.locked());
    }

    // Read the block...
    {
        on_thread_t thread(cache->serializer->home_thread());
        subtree_recency = cache->serializer->get_recency(block_id);
        // TODO: Merge this initialization with the read itself eventually
        data_token = cache->serializer->index_read(block_id);
        guarantee(data_token);
        cache->serializer->block_read(data_token, data.get(), io_account);
    }

    // Read the block sequence id
    block_sequence_id = cache->serializer->get_block_sequence_id(block_id, data.get());

    replay_patches();

    if (should_lock) {
        lock.unlock();
    }
}

// This form of the buf constructor is used when the block exists on disk and needs to be loaded
mc_inner_buf_t::mc_inner_buf_t(mc_cache_t *_cache, block_id_t _block_id, file_account_t *_io_account)
    : evictable_t(_cache),
      writeback_t::local_buf_t(),
      block_id(_block_id),
      subtree_recency(repli_timestamp_t::invalid),  // Gets initialized by load_inner_buf
      data(_cache->serializer->malloc()),
      version_id(_cache->get_min_snapshot_version(_cache->get_current_version_id())),
      lock(),
      next_patch_counter(1),
      refcount(0),
      do_delete(false),
      cow_refcount(0),
      snap_refcount(0),
      block_sequence_id(NULL_BLOCK_SEQUENCE_ID) {

    rassert(version_id != faux_version_id);

    array_map_t::constructing_inner_buf(this);

    // Some things expect us to return immediately (as of 5/12/2011), so we do the loading in a
    // separate coro. We have to make sure that load_inner_buf() acquires the lock first
    // however, so we use spawn_now_dangerously().
    coro_t::spawn_now_dangerously(boost::bind(&mc_inner_buf_t::load_inner_buf, this, true, _io_account));

    // TODO: only increment pm_n_blocks_in_memory when we actually load the block into memory.
    ++_cache->stats->pm_n_blocks_in_memory;
    refcount++; // Make the refcount nonzero so this block won't be considered safe to unload.

    _cache->page_repl.make_space();
    _cache->maybe_unregister_read_ahead_callback();

    refcount--;
}

// This form of the buf constructor is used when the block exists on disks but has been loaded into buf already
mc_inner_buf_t::mc_inner_buf_t(mc_cache_t *_cache, block_id_t _block_id, void *_buf, const intrusive_ptr_t<standard_block_token_t>& token, repli_timestamp_t _recency_timestamp)
    : evictable_t(_cache),
      writeback_t::local_buf_t(),
      block_id(_block_id),
      subtree_recency(_recency_timestamp),
      data(_buf),
      version_id(_cache->get_min_snapshot_version(_cache->get_current_version_id())),
      data_token(token),
      lock(),
      refcount(0),
      do_delete(false),
      cow_refcount(0),
      snap_refcount(0),
      block_sequence_id(NULL_BLOCK_SEQUENCE_ID) {

    rassert(version_id != faux_version_id);

    array_map_t::constructing_inner_buf(this);

    ++_cache->stats->pm_n_blocks_in_memory;
    refcount++; // Make the refcount nonzero so this block won't be considered safe to unload.
    _cache->page_repl.make_space();
    _cache->maybe_unregister_read_ahead_callback();
    refcount--;

    // Read the block sequence id
    block_sequence_id = _cache->serializer->get_block_sequence_id(_block_id, data.get());

    replay_patches();
}

mc_inner_buf_t *mc_inner_buf_t::allocate(mc_cache_t *cache, version_id_t snapshot_version, repli_timestamp_t recency_timestamp) {
    cache->assert_thread();

    if (snapshot_version == faux_version_id)
        snapshot_version = cache->get_current_version_id();

    block_id_t block_id = cache->free_list.gen_block_id();
    mc_inner_buf_t *inner_buf = cache->find_buf(block_id);
    if (!inner_buf) {
        return new mc_inner_buf_t(cache, block_id, snapshot_version, recency_timestamp);
    } else {
        // Block with block_id was logically deleted, but its inner_buf survived.
        // This can happen when there are active snapshots of that block.
        // We cannot just re-create a new mc_inner_buf_t for the block id in that case,
        // as that would cause a conflict in the page map.
        // Instead, we keep the snapshots around and reset the remaining state of
        // the buffer to make it behave just like a freshly constructed one.
        
        rassert(inner_buf->do_delete);
        rassert(!inner_buf->data.has());

        inner_buf->initialize_to_new(snapshot_version, recency_timestamp);
        inner_buf->writeback_buf().reset();

        return inner_buf;
    }
}

// This routine is used in the constructor for constructing new bufs, and also called directly from the
// allocate() method. The latter uses it to reset an existing mc_inner_buf_t to its initial state, without
// requiring the creation of a new mc_inner_buf_t object.
// See the comment in allocate() for why this is necessary.
void mc_inner_buf_t::initialize_to_new(version_id_t _snapshot_version, repli_timestamp_t _recency_timestamp) {
    rassert(!data.has());
    
    subtree_recency = _recency_timestamp;
    data.init_malloc(cache->serializer);
#if !defined(NDEBUG) || defined(VALGRIND)
        // The memory allocator already filled this with 0xBD, but it's nice to be able to distinguish
        // between problems with uninitialized memory and problems with uninitialized blocks
        memset(data.get(), 0xCD, cache->serializer->get_block_size().value());
#endif
    version_id = _snapshot_version;
    do_delete = false;
    next_patch_counter = 1;
    cow_refcount = 0;
    snap_refcount = 0;
    block_sequence_id = NULL_BLOCK_SEQUENCE_ID;
    data_token.reset();
}

// This form of the buf constructor is used when a completely new block is being created.
// Used by mc_inner_buf_t::allocate() and by the patch log.
// If you update this constructor, please don't forget to update mc_inner_buf_t::allocate
// accordingly.
mc_inner_buf_t::mc_inner_buf_t(mc_cache_t *_cache, block_id_t _block_id, version_id_t _snapshot_version, repli_timestamp_t _recency_timestamp)
    : evictable_t(_cache),
      writeback_t::local_buf_t(),
      block_id(_block_id),
      lock(),
      refcount(0) {

    rassert(_snapshot_version != faux_version_id);
    _cache->assert_thread();

    initialize_to_new(_snapshot_version, _recency_timestamp);

    array_map_t::constructing_inner_buf(this);

    ++_cache->stats->pm_n_blocks_in_memory;
    ++refcount; // Make the refcount nonzero so this block won't be considered safe to unload.

    _cache->page_repl.make_space();
    _cache->maybe_unregister_read_ahead_callback();

    --refcount;
}

void mc_inner_buf_t::unload() {
    delete this;
}

mc_inner_buf_t::~mc_inner_buf_t() {
    cache->assert_thread();

#ifndef NDEBUG
#ifndef VALGRIND
    // We're about to free the data, let's set it to a recognizable
    // value to make sure we don't depend on accessing things that may
    // be flushed out of the cache.
    if (data.has()) {
        memset(data.get(), 0xDD, cache->serializer->get_block_size().value());
    }
#endif
#endif

    array_map_t::destroying_inner_buf(this);

    rassert(safe_to_unload());
    if (data.has()) {
        data.free(cache->serializer);
    }

    if (in_page_repl()) {
        remove_from_page_repl();
    }

    --cache->stats->pm_n_blocks_in_memory;
}

void mc_inner_buf_t::replay_patches() {
    // Remove obsolete patches from diff storage
    if (cache->patch_memory_storage.has_patches_for_block(block_id)) {
        // TODO: Perhaps there is a problem if the question of whether
        // we can call filter_applied_patches depends on whether the
        // block id is already in the patch_memory_storage.
        cache->patch_memory_storage.filter_applied_patches(block_id, block_sequence_id);
    }
    // All patches that currently exist must have been materialized out of core...
    writeback_buf().set_last_patch_materialized(cache->patch_memory_storage.last_patch_materialized_or_zero(block_id));

    // Apply outstanding patches
    cache->patch_memory_storage.apply_patches(block_id, reinterpret_cast<char *>(data.get()), cache->get_block_size());

    // Set next_patch_counter such that the next patches get values consistent with the existing patches
    next_patch_counter = cache->patch_memory_storage.last_patch_materialized_or_zero(block_id) + 1;
}

bool mc_inner_buf_t::snapshot_if_needed(version_id_t new_version, bool leave_clone) {
    cache->assert_thread();
    // we can get snapshotted_version == version_id due to copy-on-write doing the snapshotting
    rassert(snapshots.empty() || snapshots.head()->snapshotted_version <= version_id);

    // all snapshot txns such that
    //   inner_version <= snapshot_txn->version_id < new_version
    // can see the current version of inner_buf->data, so we need to make some snapshots for them
    size_t num_snapshots_affected = cache->calculate_snapshots_affected(version_id, new_version);
    if (num_snapshots_affected + cow_refcount > 0) {
        if (!data.has()) {
            // Ok, we are in trouble. We don't have data (probably because we were constructed
            // with should_load == false), but now a snapshot of that non-existing data is needed.
            // That in turn means that we have to acquire the data now, before we can proceed...
            data.init_malloc(cache->serializer);

            // Our callee (hopefully!!!) already has a lock at this point, so there's no need
            // to acquire another one inside of load_inner_buf (and of course it would dead-lock).
            load_inner_buf(false, cache->reads_io_account.get());

            // Now that we have loaded the data, it could have happended that the snapshot is
            // actually not needed anymore (in which case we have loaded the block unnecessarily,
            // but what could we possibly do about that?). So, we update the count of affected
            // snapshots.
            num_snapshots_affected = cache->calculate_snapshots_affected(version_id, new_version);
        }
    }

    // This "sanity check" is commented out because it is now wrong.
    // It is possible for multiple buf_lock_t's from the same
    // transaction to be held at the same time (for example, when
    // backfilling).  That invalidates this assertion, without meaning
    // the rest of the code is broken.

    // commented out: rassert(snap_refcount <= num_snapshots_affected); // sanity check

    // check whether snapshot refcount is > 0
    if (0 == num_snapshots_affected + cow_refcount)
        return false;           // no snapshot necessary

    // Initially actively referencing the snapshotted buf are mc_buf_lock_t's in rwi_read_outdated_ok
    // mode, corresponding to cow_refcount; and mc_buf_lock_t's in snapshotted rwi_read mode, indicated
    // by snap_refcount.

    buf_snapshot_t *snap = new buf_snapshot_t(this, num_snapshots_affected, cow_refcount + snap_refcount, leave_clone);
    cow_refcount = 0;
    snap_refcount = 0;

    // Register the snapshot
    snapshots.push_front(snap);
    cache->register_buf_snapshot(this, snap, version_id, new_version);

    return true;
}

void *mc_inner_buf_t::acquire_snapshot_data(version_id_t version_to_access, file_account_t *io_account, repli_timestamp_t *subtree_recency_out) {
    rassert(version_to_access != mc_inner_buf_t::faux_version_id);
    for (buf_snapshot_t *snap = snapshots.head(); snap; snap = snapshots.next(snap)) {
        if (snap->snapshotted_version <= version_to_access) {
            *subtree_recency_out = snap->subtree_recency;
            return snap->acquire_data(io_account);
        }
    }
    unreachable("No acceptable snapshotted version found for version %lu", version_to_access);
}

// TODO (sam): Look at who's passing this void pointer.
void mc_inner_buf_t::release_snapshot_data(void *_data) {
    // This should never be called in non-coroutine context, since it could release the last
    // reference to a block_token_t, whose destructor switches to the serializer thread.
    rassert(coro_t::self());
    cache->assert_thread();
    rassert(_data, "tried to release NULL snapshot data");
    for (buf_snapshot_t *snap = snapshots.head(); snap; snap = snapshots.next(snap)) {
        // TODO (sam): Obviously this comparison is disgusting.
        if (snap->data.equals(_data)) {
            snap->release_data();
            return;
        }
    }
    unreachable("Tried to release block snapshot that doesn't exist");
}

void mc_inner_buf_t::release_snapshot(buf_snapshot_t *snapshot) {
    cache->assert_thread();
    snapshot->release();
}

bool mc_inner_buf_t::safe_to_unload() {
    return !lock.locked() && writeback_buf().safe_to_unload() && refcount == 0 && cow_refcount == 0 && snapshots.empty();
}

// TODO (sam): Look at who's passing this void pointer.
void mc_inner_buf_t::update_data_token(const void *the_data, const intrusive_ptr_t<standard_block_token_t>& token) {
    cache->assert_thread();
    // TODO (sam): Obviously this comparison is disgusting.
    if (data.equals(the_data)) {
        rassert(!data_token, "data token already up-to-date");
        data_token = token;
        return;
    }
    for (buf_snapshot_t *snap = snapshots.head(); snap; snap = snapshots.next(snap)) {
        // TODO (sam): Obviously this comparison is disgusting.
        if (!snap->data.equals(the_data)) {
            continue;
        }
        rassert(!snap->token, "snapshot data token already up-to-date");
        snap->token = token;
        return;
    }
    unreachable("data does not correspond to current buffer or any existing snapshot of it");
}

/* Default constructor does not obtain a lock, but a lock may be allocated or swapped from another mc_buf_lock_t object */
mc_buf_lock_t::mc_buf_lock_t() :
    acquired(false),
    snapshotted(false),
    non_locking_access(false),
    inner_buf(NULL),
    data(NULL),
    subtree_recency(repli_timestamp_t::invalid),
    parent_transaction(NULL)
{ }


/* Constructor to obtain a buffer lock within a transaction */
mc_buf_lock_t::mc_buf_lock_t(mc_transaction_t *transaction,
                             block_id_t block_id, access_t _mode,
                             UNUSED buffer_cache_order_mode_t order_mode,
                             lock_in_line_callback_t *call_when_in_line) THROWS_NOTHING :
    acquired(false),
    snapshotted(transaction->snapshotted),
    non_locking_access(snapshotted),
    mode(_mode),
    inner_buf(transaction->cache->find_buf(block_id)),
    data(NULL),
    subtree_recency(repli_timestamp_t::invalid),
    parent_transaction(transaction)
{
    transaction->assert_thread();
    rassert(block_id != NULL_BLOCK_ID);

    // Note that it is critical that between here and creating our buf_lock_t wrapper that we do nothing
    // blocking (unless it acquires a lock on inner_buf or otherwise prevents it from being
    // unloaded), or else inner_buf could be selected for deletion from the cache, then recreated,
    // and we'd have two inner_bufs corresponding to the same block id floating around.

    if (!inner_buf) {
        /* The buf isn't in the cache and must be loaded from disk */
        // We are either not snapshotted or our snapshot is consistent with the latest version;
        // otherwise, the inner buf would be around to keep track of the snapshotted version. Thus,
        // it is not wasteful to load the latest version if should_load is true.
        inner_buf = new mc_inner_buf_t(transaction->cache, block_id, transaction->get_io_account());
    } else {
        // TODO: the logic for when to load an inner_buf's versions (most recent or snapshotted) is
        // scattered around everywhere (eg: here). consolidate it, perhaps in mc_buf_lock_t.
        rassert(!inner_buf->do_delete || snapshotted);

        // ensures we're using the top version
        if (!inner_buf->data.has() && !inner_buf->do_delete &&
            // if we're accessing a snapshot rather than the top version, no need to load it here
            !(snapshotted && transaction->snapshot_version != mc_inner_buf_t::faux_version_id && transaction->snapshot_version < inner_buf->version_id))
        {
            // The inner_buf doesn't have any data currently. We need the data though,
            // so load it!
            inner_buf->data.init_malloc(transaction->cache->serializer);

            // Please keep in mind that this is blocking...
            inner_buf->load_inner_buf(true, transaction->get_io_account());
        }
    }

    initialize(transaction->snapshot_version, transaction->get_io_account(), call_when_in_line);

    if (is_write_mode(mode)) {
        touch_recency(transaction->recency_timestamp);
    }

    transaction->maybe_finalize_version();
    transaction->assert_thread();
}

void mc_buf_lock_t::initialize(mc_inner_buf_t::version_id_t version_to_access,
                               file_account_t *io_account,
                               lock_in_line_callback_t *call_when_in_line)
{
    inner_buf->cache->assert_thread();
    inner_buf->refcount++;
    patches_serialized_size_at_start = -1;

    if (snapshotted) {
        rassert(is_read_mode(mode), "Only read access is allowed to block snapshots");
        // Our snapshotting-specific code can't handle weird read
        // modes, unfortunately.

        // TODO: Make snapshotting code work properly with weird read
        // modes.  (Or maybe don't do that, just keep what we have
        // here.)
        if (mode == rwi_read_outdated_ok || mode == rwi_read_sync) {
            mode = rwi_read;
        }
    }

    // If the top version is less or equal to snapshot_version, then we need to acquire
    // a read lock first (otherwise we may get the data of the unfinished write on top).
    if (snapshotted && version_to_access != mc_inner_buf_t::faux_version_id && version_to_access < inner_buf->version_id) {
        // acquire the snapshotted block; no need to lock
        data = inner_buf->acquire_snapshot_data(version_to_access, io_account, &subtree_recency);
        guarantee(data != NULL);

        // we never needed to get in line, so just call the function straight-up to ensure it gets called.
        if (call_when_in_line) call_when_in_line->on_in_line();
    } else {
        ticks_t lock_start_time;
        // the top version is the right one for us; acquire a lock of the appropriate type first
        inner_buf->cache->stats->pm_bufs_acquiring.begin(&lock_start_time);
        inner_buf->lock.co_lock(mode == rwi_read_outdated_ok ? rwi_read : mode, call_when_in_line);
        inner_buf->cache->stats->pm_bufs_acquiring.end(&lock_start_time);

        // It's possible that, now that we've acquired the lock, that
        // the inner buf's version id has changed, and that we should
        // get a snapshotted version.

        if (snapshotted && version_to_access != mc_inner_buf_t::faux_version_id && version_to_access < inner_buf->version_id) {
            inner_buf->lock.unlock();

            // acquire the snapshotted block; no need to lock
            data = inner_buf->acquire_snapshot_data(version_to_access, io_account, &subtree_recency);
            guarantee(data != NULL);

        } else {
            acquire_block(version_to_access);
        }
    }

    inner_buf->cache->stats->pm_bufs_held.begin(&start_time);
    if (parent_transaction) {
        ++parent_transaction->num_buf_locks_acquired;
    }
    acquired = true;
}

/* Constructor for mc_buf_lock_t used by patch_disk_storage_t, takes some shortcuts and has some special behavior */
mc_buf_lock_t * mc_buf_lock_t::acquire_non_locking_lock(mc_cache_t *cache, const block_id_t block_id)
{
    mc_buf_lock_t *lock = new mc_buf_lock_t();
    lock->mode = rwi_read;
    lock->inner_buf = cache->page_map.find(block_id);
    cache->assert_thread();

    if (!lock->inner_buf) {
        /* The buf isn't in the cache and must be loaded from disk */
        lock->inner_buf = new mc_inner_buf_t(cache, block_id, cache->reads_io_account.get());
    }

    // We still have to acquire the lock once to wait for the buf to get ready
    lock->initialize(mc_inner_buf_t::faux_version_id, DEFAULT_DISK_ACCOUNT, 0);

    // Release the lock we've got
    lock->inner_buf->lock.unlock();
    lock->non_locking_access = true;
    lock->mode = rwi_write;

    cache->assert_thread();
    return lock;
}

mc_buf_lock_t::mc_buf_lock_t(mc_transaction_t *transaction) THROWS_NOTHING :
    acquired(false),
    snapshotted(transaction->snapshotted),
    non_locking_access(snapshotted),
    mode(transaction->access),
    inner_buf(NULL),
    data(NULL),
    subtree_recency(repli_timestamp_t::invalid),
    parent_transaction(transaction)
{
    transaction->assert_thread();
    rassert(mode == rwi_write);
    rassert(!snapshotted);

    /* Make a completely new block, complete with a shiny new block_id. */
    inner_buf = mc_inner_buf_t::allocate(transaction->cache, transaction->snapshot_version, transaction->recency_timestamp);

    // Assign a snapshot version, to ensure we can no longer become a snapshotting transaction.
    if (transaction->snapshot_version == mc_inner_buf_t::faux_version_id)
        transaction->snapshot_version = inner_buf->version_id;

    transaction->assert_thread();

    // This must pass since no one else holds references to this block.
    initialize(transaction->snapshot_version, transaction->get_io_account(), 0);

    transaction->assert_thread();
}

mc_buf_lock_t::~mc_buf_lock_t() {
    assert_thread();
    release_if_acquired();
}

void mc_buf_lock_t::release_if_acquired() {
    assert_thread();
    if (acquired) {
        release();
    }
}

void mc_buf_lock_t::acquire_block(mc_inner_buf_t::version_id_t version_to_access) {
    inner_buf->cache->assert_thread();
    rassert(!inner_buf->do_delete);

    subtree_recency = inner_buf->subtree_recency;

    switch (mode) {
        case rwi_read_sync:
        case rwi_read: {
            if (snapshotted) {
                rassert(version_to_access == mc_inner_buf_t::faux_version_id || inner_buf->version_id <= version_to_access);
                ++inner_buf->snap_refcount;
            }
            // TODO (sam): Obviously something's f'd up about this.
            data = inner_buf->data.get();
            rassert(data != NULL);
            break;
        }
        case rwi_read_outdated_ok: {
            ++inner_buf->cow_refcount;
            // TODO (sam): Obviously something's f'd up about this.
            data = inner_buf->data.get();
            rassert(data != NULL);
            // unlock the buffer now that we have established a COW reference to it so that
            // writers can overwrite it. we know we have locked the buffer because !snapshotted,
            // therefore mc_buf_lock_t() locked it before calling us.
            inner_buf->lock.unlock();
            break;
        }
        case rwi_write: {
            if (version_to_access == mc_inner_buf_t::faux_version_id)
                version_to_access = inner_buf->cache->get_current_version_id();
            rassert(inner_buf->version_id <= version_to_access);

            inner_buf->snapshot_if_needed(version_to_access, true);

            inner_buf->version_id = version_to_access;
            // TODO (sam): Obviously something's f'd up about this.
            data = inner_buf->data.has() ? inner_buf->data.get() : 0;
            rassert(data != NULL);

            if (!inner_buf->writeback_buf().needs_flush() &&
                  patches_serialized_size_at_start == -1 &&
                  global_full_perfmon) {
                patches_serialized_size_at_start =
                    inner_buf->cache->patch_memory_storage.get_patches_serialized_size(inner_buf->block_id);
            }

            break;
        }
        case rwi_intent:
            not_implemented("Locking with intent not supported yet.");
        case rwi_upgrade:
        default:
            unreachable();
    }

    if (snapshotted) {
        inner_buf->lock.unlock();
    }

    inner_buf->cache->assert_thread();
}

void mc_buf_lock_t::swap(mc_buf_lock_t& swapee) {
    assert_thread();
    swapee.assert_thread();
    std::swap(acquired, swapee.acquired);
    std::swap(snapshotted, swapee.snapshotted);
    std::swap(non_locking_access, swapee.non_locking_access);
    std::swap(start_time, swapee.start_time);
    std::swap(mode, swapee.mode);
    std::swap(patches_serialized_size_at_start, swapee.patches_serialized_size_at_start);
    std::swap(inner_buf, swapee.inner_buf);
    std::swap(data, swapee.data);
    std::swap(subtree_recency, swapee.subtree_recency);
#ifndef NDEBUG
    std::swap(real_home_thread, swapee.real_home_thread);
#endif
    std::swap(parent_transaction, swapee.parent_transaction);
}

bool mc_buf_lock_t::is_acquired() const {
    return acquired;
}

bool mc_buf_lock_t::is_deleted() const {
    return data == NULL;
}

const void * mc_buf_lock_t::get_data_read() const {
    rassert(data);
    return data;
}

block_id_t mc_buf_lock_t::get_block_id() const {
    return inner_buf->block_id;
}

void mc_buf_lock_t::touch_recency(repli_timestamp_t timestamp) {
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

repli_timestamp_t mc_buf_lock_t::get_recency() const {
    // TODO: Make it possible to get the recency _without_
    // acquiring the buf.  The recency should be locked with a
    // lock that sits "above" buffer acquisition.  Or the recency
    // simply should be set _before_ trying to acquire the buf.
    return subtree_recency;
}

eviction_priority_t mc_buf_lock_t::get_eviction_priority() const {
    return inner_buf->eviction_priority;
}

void mc_buf_lock_t::set_eviction_priority(eviction_priority_t val) {
    inner_buf->eviction_priority = val;
}

void mc_buf_lock_t::apply_patch(buf_patch_t *_patch) {
    assert_thread();
    rassert(!inner_buf->safe_to_unload()); // If this assertion fails, it probably means that you're trying to access a buf you don't own.
    rassert(!inner_buf->do_delete);
    rassert(mode == rwi_write);
    // TODO (sam): Obviously something's f'd up about this.
    rassert(inner_buf->data.equals(data));
    rassert(data, "Probably tried to write to a buffer acquired with !should_load.");
    rassert(_patch->get_block_id() == inner_buf->block_id);

    scoped_ptr_t<buf_patch_t> patch(_patch);

    patch->apply_to_buf(reinterpret_cast<char *>(data), inner_buf->cache->get_block_size());
    inner_buf->writeback_buf().set_dirty();
    // Invalidate the token
    inner_buf->data_token.reset();

    // We cannot accept patches for blocks without a valid block sequence id (namely newly allocated blocks, they have to be written to disk at least once)
    if (inner_buf->block_sequence_id == NULL_BLOCK_SEQUENCE_ID) {
        ensure_flush();
    }

    if (!inner_buf->writeback_buf().needs_flush()) {
        // Check if we want to disable patching for this block and flush it directly instead
        const int32_t max_patches_size = inner_buf->cache->serializer->get_block_size().value() / inner_buf->cache->get_max_patches_size_ratio();
        if (patch->get_serialized_size() + inner_buf->cache->patch_memory_storage.get_patches_serialized_size(inner_buf->block_id) > max_patches_size) {
            ensure_flush();
        } else {
            // Store the patch if the buffer does not have to be flushed anyway
            if (patch->get_patch_counter() == 1) {
                // Clean up any left-over patches
                inner_buf->cache->patch_memory_storage.drop_patches(inner_buf->block_id);
            }

            // Takes ownership of patch.
            inner_buf->cache->patch_memory_storage.store_patch(patch.release());
        }
    }
}

void *mc_buf_lock_t::get_data_major_write() {
    ASSERT_NO_CORO_WAITING;

    rassert(!inner_buf->safe_to_unload()); // If this assertion fails, it probably means that you're trying to access a buf you don't own.
    rassert(!inner_buf->do_delete);
    rassert(mode == rwi_write);
    // TODO (sam): f'd up
    rassert(inner_buf->data.equals(data));
    rassert(data, "Probably tried to write to a buffer acquired with !should_load.");

    inner_buf->assert_thread();

    // Invalidate the token
    inner_buf->data_token.reset();
    ensure_flush();

    return data;
}

void mc_buf_lock_t::ensure_flush() {
    ASSERT_NO_CORO_WAITING;
    assert_thread();

    // TODO (sam): f'd up
    rassert(inner_buf->data.equals(data));
    if (!inner_buf->writeback_buf().needs_flush()) {
        // We bypass the patching system, make sure this buffer gets flushed.
        inner_buf->writeback_buf().set_needs_flush(true);
        // ... we can also get rid of existing patches at this point.
        inner_buf->cache->patch_memory_storage.drop_patches(inner_buf->block_id);
        // Make sure that the buf is marked as dirty
        inner_buf->writeback_buf().set_dirty();
        inner_buf->data_token.reset();
    }
}

void mc_buf_lock_t::mark_deleted() {
    assert_thread();
    rassert(mode == rwi_write);
    rassert(!inner_buf->safe_to_unload());
    // TODO (sam): f'd up
    rassert(inner_buf->data.equals(data));

    bool we_snapshotted = inner_buf->snapshot_if_needed(inner_buf->version_id, false);
    if (!we_snapshotted && data) {
        // We already asserted above that data ==
        // inner_buf->data.get(), and snapshot_if_needed would not
        // change that if it returned false.
        inner_buf->data.free(inner_buf->cache->serializer);
    } else {
        rassert(!inner_buf->data.has(), "We snapshotted with deletion in mind but inner_buf still has a live data.");
    }

    rassert(!inner_buf->data.has());
    data = NULL;

    inner_buf->do_delete = true;
    ensure_flush(); // Disable patch log system for the buffer
}

patch_counter_t mc_buf_lock_t::get_next_patch_counter() {
    rassert(!inner_buf->do_delete);
    rassert(mode == rwi_write);
    // TODO (sam): f'd up
    rassert(inner_buf->data.equals(data));
    return inner_buf->next_patch_counter++;
}

bool ptr_in_byte_range(const void *p, const void *range_start, size_t size_in_bytes) {
    const uint8_t *p8 = reinterpret_cast<const uint8_t *>(p);
    const uint8_t *range8 = reinterpret_cast<const uint8_t *>(range_start);
    return range8 <= p8 && p8 < range8 + size_in_bytes;
}

bool range_inside_of_byte_range(const void *p, size_t n_bytes, const void *range_start, size_t size_in_bytes) {
    const uint8_t *p8 = reinterpret_cast<const uint8_t *>(p);
    return ptr_in_byte_range(p, range_start, size_in_bytes) &&
        (n_bytes == 0 || ptr_in_byte_range(p8 + n_bytes - 1, range_start, size_in_bytes));
}

// Personally I'd be happier if these functions took offsets.  That's
// a sort of long-term TODO, though.
//
// ^ Who are you?  A ghost in the github...
void mc_buf_lock_t::set_data(void *dest, const void *src, size_t n) {
    assert_thread();
    // TODO (sam): f'd up.
    rassert(inner_buf->data.equals(data));
    if (n == 0) {
        return;
    }
    rassert(range_inside_of_byte_range(dest, n, data, inner_buf->cache->get_block_size().value()));

    if (inner_buf->writeback_buf().needs_flush()) {
        // Save the allocation / construction of a patch object
        get_data_major_write();
        memcpy(dest, src, n);
    } else {
        size_t offset = reinterpret_cast<uint8_t *>(dest) - reinterpret_cast<uint8_t *>(data);
        // block_sequence ID will be set later...
        apply_patch(new memcpy_patch_t(inner_buf->block_id, get_next_patch_counter(), offset, reinterpret_cast<const char *>(src), n));
    }
}

void mc_buf_lock_t::move_data(void *dest, const void *src, const size_t n) {
    assert_thread();
    // TODO (sam): f'd up.
    rassert(inner_buf->data.equals(data));
    if (n == 0) {
        return;
    }

    rassert(range_inside_of_byte_range(src, n, data, inner_buf->cache->get_block_size().value()));
    rassert(range_inside_of_byte_range(dest, n, data, inner_buf->cache->get_block_size().value()));

    if (inner_buf->writeback_buf().needs_flush()) {
        // Save the allocation / construction of a patch object
        get_data_major_write();
        memmove(dest, src, n);
    } else {
        size_t dest_offset = reinterpret_cast<uint8_t *>(dest) - reinterpret_cast<uint8_t *>(data);
        size_t src_offset = reinterpret_cast<const uint8_t *>(src) - reinterpret_cast<uint8_t *>(data);
        // tblock_sequence ID will be set later...
        apply_patch(new memmove_patch_t(inner_buf->block_id, get_next_patch_counter(), dest_offset, src_offset, n));
    }
}


void mc_buf_lock_t::release() {
    assert_thread();
    guarantee(acquired);
    inner_buf->cache->assert_thread();
    inner_buf->cache->stats->pm_bufs_held.end(&start_time);

    if (mode == rwi_write && !inner_buf->writeback_buf().needs_flush() && patches_serialized_size_at_start >= 0) {
        if (inner_buf->cache->patch_memory_storage.get_patches_serialized_size(inner_buf->block_id) > patches_serialized_size_at_start) {
            inner_buf->cache->stats->pm_patches_size_per_write.record(inner_buf->cache->patch_memory_storage.get_patches_serialized_size(inner_buf->block_id) - patches_serialized_size_at_start);
        }
    }

    rassert(inner_buf->refcount > 0);
    --inner_buf->refcount;

    switch (mode) {
        case rwi_read_sync:
        case rwi_read:
        case rwi_write: {
            if (!non_locking_access) {
                inner_buf->lock.unlock();
            }
            if (snapshotted) {
                // TODO (sam): f'd up.
                if (inner_buf->data.equals(data)) {
                    --inner_buf->snap_refcount;
                } else {
                    inner_buf->release_snapshot_data(data);
                }
            } else {
                // TODO (sam): f'd up.
                rassert(inner_buf->data.equals(data));
            }
            break;
        }
        case rwi_read_outdated_ok: {
            // TODO (sam): f'd up.
            if (inner_buf->data.equals(data)) {
                rassert(inner_buf->cow_refcount > 0);
                --inner_buf->cow_refcount;
            } else {
                inner_buf->release_snapshot_data(data);
            }
            break;
        }
        case rwi_intent:
        case rwi_upgrade:
        default:
            unreachable("Unexpected mode.");
    }

    // If the buf is marked deleted, then we can delete it from memory already
    // and just keep track of the deleted block_id (and whether to write an
    // empty block).
    if (inner_buf->do_delete) {
        if (mode == rwi_write) {
            // TODO(sam): Shouldn't these already be set somehow?  What about the data token?
            inner_buf->writeback_buf().mark_block_id_deleted();
            inner_buf->writeback_buf().set_dirty(false);
            inner_buf->writeback_buf().set_recency_dirty(false); // TODO: Do we need to handle recency in master in some other way?
        }
        if (inner_buf->safe_to_unload()) {
            delete inner_buf;
            inner_buf = NULL;
        }
    }

#ifndef AGGRESSIVE_BUF_UNLOADING
#define AGGRESSIVE_BUF_UNLOADING 0
#endif

#if AGGRESSIVE_BUF_UNLOADING == 1
    // If this code is enabled, then it will cause bufs to be unloaded very aggressively.
    // This is useful for catching bugs in which something expects a buf to remain valid even though
    // it is eligible to be unloaded.

    if (inner_buf && inner_buf->safe_to_unload()) {
        delete inner_buf;
    }
#endif
    acquired = false;
    if (parent_transaction) {
        --parent_transaction->num_buf_locks_acquired;
    }
}


/**
 * Transaction implementation.
 */

mc_transaction_t::mc_transaction_t(mc_cache_t *_cache, access_t _access, int _expected_change_count, repli_timestamp_t _recency_timestamp, UNUSED order_token_t order_token /* used only by the scc transaction */)
    : cache(_cache),
      expected_change_count(_expected_change_count),
      access(_access),
      recency_timestamp(_recency_timestamp),
      snapshot_version(mc_inner_buf_t::faux_version_id),
      snapshotted(false),
      cache_account(NULL),
      num_buf_locks_acquired(0),
      is_writeback_transaction(false) {
    block_pm_duration start_timer(&cache->stats->pm_transactions_starting);

    coro_fifo_acq_t write_throttle_acq;

    if (is_write_mode(access)) {
        write_throttle_acq.enter(&cache->co_begin_coro_fifo());
    }

    rassert(access == rwi_read || access == rwi_read_sync || access == rwi_write);
    cache->assert_thread();
    rassert(!cache->shutting_down);
    rassert(access == rwi_write || expected_change_count == 0);
    cache->num_live_non_writeback_transactions++;
    cache->writeback.begin_transaction(this);

    cache->stats->pm_transactions_active.begin(&start_time);
}

/* This version is only for read transactions from the writeback!  And some unit tests use it. */
mc_transaction_t::mc_transaction_t(mc_cache_t *_cache, access_t _access, UNUSED int fook, DEBUG_VAR bool dont_assert_about_shutting_down) :
    cache(_cache),
    expected_change_count(0),
    access(_access),
    recency_timestamp(repli_timestamp_t::distant_past),
    snapshot_version(mc_inner_buf_t::faux_version_id),
    snapshotted(false),
    cache_account(NULL),
    num_buf_locks_acquired(0),
    is_writeback_transaction(false) {
    block_pm_duration start_timer(&cache->stats->pm_transactions_starting);
    rassert(access == rwi_read || access == rwi_read_sync);

    // No write throttle acq.

    cache->assert_thread();
    rassert(dont_assert_about_shutting_down || !cache->shutting_down);
    cache->num_live_non_writeback_transactions++;
    cache->writeback.begin_transaction(this);
    cache->stats->pm_transactions_active.begin(&start_time);
}

mc_transaction_t::mc_transaction_t(mc_cache_t *_cache, access_t _access, UNUSED i_am_writeback_t i_am_writeback) :
    cache(_cache),
    expected_change_count(0),
    access(_access),
    recency_timestamp(repli_timestamp_t::distant_past),
    snapshot_version(mc_inner_buf_t::faux_version_id),
    snapshotted(false),
    cache_account(NULL),
    num_buf_locks_acquired(0),
    is_writeback_transaction(true) {
    block_pm_duration start_timer(&cache->stats->pm_transactions_starting);
    rassert(access == rwi_read || access == rwi_read_sync);

    cache->assert_thread();
    cache->num_live_writeback_transactions++;
    cache->writeback.begin_transaction(this);
    cache->stats->pm_transactions_active.begin(&start_time);
}


void mc_transaction_t::register_buf_snapshot(mc_inner_buf_t *inner_buf, mc_inner_buf_t::buf_snapshot_t *snap) {
    assert_thread();
    ++cache->stats->pm_registered_snapshot_blocks;
    owned_buf_snapshots.push_back(std::make_pair(inner_buf, snap));
}

mc_transaction_t::~mc_transaction_t() {

    assert_thread();

    cache->stats->pm_transactions_active.end(&start_time);

    rassert(num_buf_locks_acquired == 0, "num_buf_locks_acquired = %ld", num_buf_locks_acquired);
    guarantee(num_buf_locks_acquired == 0);

    block_pm_duration commit_timer(&cache->stats->pm_transactions_committing);

    if (snapshotted && snapshot_version != mc_inner_buf_t::faux_version_id) {
        cache->unregister_snapshot(this);
        for (std::vector<std::pair<mc_inner_buf_t*, mc_inner_buf_t::buf_snapshot_t*> >::iterator it = owned_buf_snapshots.begin();
             it != owned_buf_snapshots.end();
             ++it) {
            (*it).first->release_snapshot((*it).second);
        }
    }

    if (access == rwi_write && cache->writeback.wait_for_flush) {
        /* We have to call `sync_patiently()` before `on_transaction_commit()` so that if
        `on_transaction_commit()` starts a sync, we will get included in it */
        struct : public writeback_t::sync_callback_t, public cond_t {
            void on_sync() { pulse(); }
        } sync_callback;

        if (cache->writeback.sync_patiently(&sync_callback)) {
            sync_callback.pulse();
        }

        cache->on_transaction_commit(this);
        sync_callback.wait();

    } else {
        cache->on_transaction_commit(this);

    }

    cache->stats->pm_snapshots_per_transaction.record(owned_buf_snapshots.size());
    cache->stats->pm_registered_snapshot_blocks -= owned_buf_snapshots.size();
}

void mc_transaction_t::maybe_finalize_version() {
    cache->assert_thread();

    const bool have_to_snapshot = snapshot_version == mc_inner_buf_t::faux_version_id && snapshotted;
    if (have_to_snapshot) {
        // register_snapshot sets transaction snapshot_version
        cache->register_snapshot(this);
    }
    if (snapshot_version == mc_inner_buf_t::faux_version_id) {
        // For non-snapshotted transactions, we still assign a version number on the first acquire,
        // to allow us to check for snapshotting after having acquired blocks
        snapshot_version = cache->next_snapshot_version;
    }
}

void mc_transaction_t::snapshot() {
    rassert(is_read_mode(get_access()), "Can only make a snapshot in non-writing transaction");
    rassert(snapshot_version == mc_inner_buf_t::faux_version_id, "Tried to take a snapshot after having acquired a first block");

    snapshotted = true;
}

void mc_transaction_t::set_account(mc_cache_account_t *_cache_account) {
    rassert(cache_account == NULL, "trying to set the transaction's cache_account twice");

    cache_account = _cache_account;
}

file_account_t *mc_transaction_t::get_io_account() const {
    return (cache_account == NULL ? cache->reads_io_account.get() : cache_account->io_account_);
}

void get_subtree_recencies_helper(int slice_home_thread, serializer_t *serializer, block_id_t *block_ids, size_t num_block_ids, repli_timestamp_t *recencies_out, get_subtree_recencies_callback_t *cb) {
    serializer->assert_thread();

    for (size_t i = 0; i < num_block_ids; ++i) {
        if (block_ids[i] != NULL_BLOCK_ID && recencies_out[i] == repli_timestamp_t::invalid) {
            recencies_out[i] = serializer->get_recency(block_ids[i]);
        }
    }

    do_on_thread(slice_home_thread, boost::bind(&get_subtree_recencies_callback_t::got_subtree_recencies, cb));
}

void mc_transaction_t::get_subtree_recencies(block_id_t *block_ids, size_t num_block_ids, repli_timestamp_t *recencies_out, get_subtree_recencies_callback_t *cb) {
    assert_thread();
    bool need_second_loop = false;
    for (size_t i = 0; i < num_block_ids; ++i) {
        if (block_ids[i] != NULL_BLOCK_ID) {
            mc_inner_buf_t *inner_buf = cache->find_buf(block_ids[i]);
            if (inner_buf) {
                recencies_out[i] = inner_buf->subtree_recency;
            } else {
                need_second_loop = true;
                recencies_out[i] = repli_timestamp_t::invalid;
            }
        }
    }

    if (need_second_loop) {
        do_on_thread(cache->serializer->home_thread(), boost::bind(&get_subtree_recencies_helper, get_thread_id(), cache->serializer, block_ids, num_block_ids, recencies_out, cb));
    } else {
        cb->got_subtree_recencies();
    }
}

mc_cache_account_t::mc_cache_account_t(int thread, file_account_t *io_account)
    : thread_(thread), io_account_(io_account) { }

mc_cache_account_t::~mc_cache_account_t() {
    on_thread_t thread_switcher(thread_);
    delete io_account_;
}

/**
 * Cache implementation.
 */

void mc_cache_t::create(serializer_t *serializer, mirrored_cache_static_config_t *config) {
    /* Initialize config block and differential log */

    patch_disk_storage_t::create(serializer, MC_CONFIGBLOCK_ID, config);

    /* Write an empty superblock */

    on_thread_t switcher(serializer->home_thread());

    void *superblock = serializer->malloc();
    bzero(superblock, serializer->get_block_size().value());

    index_write_op_t op(SUPERBLOCK_ID);
    op.token = serializer->block_write(superblock, SUPERBLOCK_ID, DEFAULT_DISK_ACCOUNT);
    op.recency = repli_timestamp_t::invalid;
    serializer_index_write(serializer, op, DEFAULT_DISK_ACCOUNT);

    serializer->free(superblock);
}

mc_cache_t::mc_cache_t(serializer_t *_serializer,
                       mirrored_cache_config_t *_dynamic_config,
                       perfmon_collection_t *perfmon_parent) :
    dynamic_config(*_dynamic_config),
    serializer(_serializer),
    stats(new mc_cache_stats_t(perfmon_parent)),
    page_repl(
        // Launch page replacement if the user-specified maximum number of blocks is reached
        dynamic_config.max_size / _serializer->get_block_size().ser_value(),
        this),
    writeback(
        this,
        dynamic_config.wait_for_flush,
        dynamic_config.flush_timer_ms,
        dynamic_config.flush_dirty_size / _serializer->get_block_size().ser_value(),
        dynamic_config.max_dirty_size / _serializer->get_block_size().ser_value(),
        dynamic_config.flush_waiting_threshold,
        dynamic_config.max_concurrent_flushes),
    /* Build list of free blocks (the free_list constructor blocks) */
    free_list(_serializer, stats.get()),
    shutting_down(false),
    num_live_writeback_transactions(0),
    num_live_non_writeback_transactions(0),
    to_pulse_when_last_transaction_commits(NULL),
    max_patches_size_ratio((dynamic_config.wait_for_flush || dynamic_config.flush_timer_ms == 0) ? MAX_PATCHES_SIZE_RATIO_DURABILITY : MAX_PATCHES_SIZE_RATIO_MIN),
    read_ahead_registered(false),
    next_snapshot_version(mc_inner_buf_t::faux_version_id+1) {

    {
        on_thread_t thread_switcher(serializer->home_thread());
        reads_io_account.init(serializer->make_io_account(dynamic_config.io_priority_reads));
        writes_io_account.init(serializer->make_io_account(dynamic_config.io_priority_writes));
    }

#ifndef NDEBUG
    writebacks_allowed = false;
#endif

    /* Load differential log from disk */
    patch_disk_storage.init(new patch_disk_storage_t(this, MC_CONFIGBLOCK_ID));
    patch_disk_storage->load_patches(&patch_memory_storage);

    /* Please note: writebacks must *not* happen prior to this point! */
    /* Writebacks ( / syncs / flushes) can cause blocks to be rewritten and require an intact patch_memory_storage! */
#ifndef NDEBUG
    writebacks_allowed = true;
#endif

    // Register us for read ahead to warm up faster
    serializer->register_read_ahead_cb(this);
    read_ahead_registered = true;

    /* We may have made a lot of blocks dirty by initializing the patch log. We need to start
    a sync explicitly because it bypassed transaction_t. */
    writeback.sync(NULL);

    /* Init the stat system with the block size */
    stats->pm_block_size.block_size = get_block_size().ser_value();
}

mc_cache_t::~mc_cache_t() {
    assert_thread();

    shutting_down = true;
    serializer->unregister_read_ahead_cb(this);

    rassert(num_live_non_writeback_transactions == 0,
            "num_live_non_writeback_transactions is %d\n",
            num_live_non_writeback_transactions);

    /* Wait for all transactions to commit before shutting down */
    if (num_live_non_writeback_transactions + num_live_writeback_transactions > 0) {
        cond_t cond;
        to_pulse_when_last_transaction_commits = &cond;
        cond.wait();
        // writeback is going to start another transaction, we don't want to get notified again (which would fail)
        to_pulse_when_last_transaction_commits = NULL;
    }

    if (writeback.has_active_flushes()) {
        cond_t cond;
        writeback.to_pulse_when_last_active_flush_finishes = &cond;
        cond.wait();
        writeback.to_pulse_when_last_active_flush_finishes = NULL;
    }

    rassert(!writeback.has_active_flushes());
    rassert(num_live_writeback_transactions + num_live_non_writeback_transactions == 0,
            "num_live_writeback_transactions = %d, num_live_non_writeback_transactions = %d",
            num_live_writeback_transactions, num_live_non_writeback_transactions);

    /* Perform a final sync */
    struct : public writeback_t::sync_callback_t, public cond_t {
        void on_sync() { pulse(); }
    } sync_cb;
    if (!writeback.sync(&sync_cb)) sync_cb.wait();

    /* Must destroy patch_disk_storage before we delete bufs because it uses the buf mechanism
    to hold the differential log. */
    patch_disk_storage.reset();

    /* Delete all the buffers */
    while (evictable_t *buf = page_repl.get_first_buf()) {
        // TODO(rntz) check that buf is actually a mc_inner_buf_t
        delete buf;
    }

    {
        /* IO accounts must be destroyed on the thread they were created on */
        on_thread_t thread_switcher(serializer->home_thread());
        reads_io_account.reset();
        writes_io_account.reset();
    }
}

block_size_t mc_cache_t::get_block_size() {
    return serializer->get_block_size();
}

void mc_cache_t::register_snapshot(mc_transaction_t *txn) {
    ++stats->pm_registered_snapshots;
    rassert(txn->snapshot_version == mc_inner_buf_t::faux_version_id, "Snapshot has been already created for this transaction");

    txn->snapshot_version = next_snapshot_version++;
    active_snapshots[txn->snapshot_version] = txn;
}

void mc_cache_t::unregister_snapshot(mc_transaction_t *txn) {
    std::map<mc_inner_buf_t::version_id_t, mc_transaction_t *>::iterator it = active_snapshots.find(txn->snapshot_version);
    if (it != active_snapshots.end() && (*it).second == txn) {
        active_snapshots.erase(it);
    } else {
        unreachable("Tried to unregister a snapshot which doesn't exist");
    }
    --stats->pm_registered_snapshots;
}

size_t mc_cache_t::calculate_snapshots_affected(mc_inner_buf_t::version_id_t snapshotted_version, mc_inner_buf_t::version_id_t new_version) {
    rassert(snapshotted_version <= new_version);    // on equals we'll get 0 snapshots affected
    size_t num_snapshots_affected = 0;
    for (std::map<mc_inner_buf_t::version_id_t, mc_transaction_t *>::iterator it = active_snapshots.lower_bound(snapshotted_version),
             itend = active_snapshots.lower_bound(new_version);
         it != itend;
         it++) {
        num_snapshots_affected++;
    }
    return num_snapshots_affected;
}

size_t mc_cache_t::register_buf_snapshot(mc_inner_buf_t *inner_buf, mc_inner_buf_t::buf_snapshot_t *snap, mc_inner_buf_t::version_id_t snapshotted_version, mc_inner_buf_t::version_id_t new_version) {
    rassert(snapshotted_version <= new_version);    // on equals we'll get 0 snapshots affected
    size_t num_snapshots_affected = 0;
    for (std::map<mc_inner_buf_t::version_id_t, mc_transaction_t *>::iterator it = active_snapshots.lower_bound(snapshotted_version),
             itend = active_snapshots.lower_bound(new_version);
         it != itend;
         it++) {
        (*it).second->register_buf_snapshot(inner_buf, snap);
        num_snapshots_affected++;
    }
    return num_snapshots_affected;
}

mc_inner_buf_t *mc_cache_t::find_buf(block_id_t block_id) {
    mc_inner_buf_t *buf = page_map.find(block_id);
    if (buf) {
        ++stats->pm_cache_hits;
    } else {
        ++stats->pm_cache_misses;
    }
    return buf;
}

bool mc_cache_t::contains_block(block_id_t block_id) {
    return find_buf(block_id) != NULL;
}


void mc_cache_t::create_cache_account(int priority, scoped_ptr_t<mc_cache_account_t> *out) {
    // We assume that a priority of 100 means that the transaction should have the same priority as
    // all the non-accounted transactions together. Not sure if this makes sense.

    // Be aware of rounding errors... (what can be do against those? probably just setting the default io_priority_reads high enough)
    int io_priority = std::max(1, dynamic_config.io_priority_reads * priority / 100);

    // TODO: This is a heuristic. While it might not be evil, it's not really optimal either.
    int outstanding_requests_limit = std::max(1, 16 * priority / 100);

    file_account_t *io_account;
    {
        on_thread_t thread_switcher(serializer->home_thread());
        io_account = serializer->make_io_account(io_priority, outstanding_requests_limit);
    }

    out->init(new mc_cache_account_t(serializer->home_thread(), io_account));
}

void mc_cache_t::on_transaction_commit(mc_transaction_t *txn) {
    assert_thread();

    writeback.on_transaction_commit(txn);

    if (txn->is_writeback_transaction) {
        num_live_writeback_transactions--;
    } else {
        num_live_non_writeback_transactions--;
    }

    if (to_pulse_when_last_transaction_commits && num_live_writeback_transactions + num_live_non_writeback_transactions == 0) {
        // We started a shutdown earlier, but we had to wait for the transactions to all finish.
        // Now that all transactions are done, continue shutting down.
        to_pulse_when_last_transaction_commits->pulse();
    }
}

bool mc_cache_t::offer_read_ahead_buf(block_id_t block_id, void *buf, const intrusive_ptr_t<standard_block_token_t>& token, repli_timestamp_t recency_timestamp) {
    // Note that the offered block might get deleted between the point where the serializer offers it and the message gets delivered!
    do_on_thread(home_thread(), boost::bind(&mc_cache_t::offer_read_ahead_buf_home_thread, this, block_id, buf, token, recency_timestamp));
    return true;
}

void mc_cache_t::offer_read_ahead_buf_home_thread(block_id_t block_id, void *buf, const intrusive_ptr_t<standard_block_token_t>& token, repli_timestamp_t recency_timestamp) {
    assert_thread();

    // Check that the offered block is allowed to be accepted at the current time
    // (e.g. that we don't have a more recent version already nor that it got deleted in the meantime)
    if (can_read_ahead_block_be_accepted(block_id)) {
        new mc_inner_buf_t(this, block_id, buf, token, recency_timestamp);
    } else {
        serializer->free(buf);
    }
}

bool mc_cache_t::can_read_ahead_block_be_accepted(block_id_t block_id) {
    assert_thread();

    if (shutting_down) {
        return false;
    }

    const bool we_already_have_the_block = find_buf(block_id);
    const bool writeback_has_no_objections = writeback.can_read_ahead_block_be_accepted(block_id);

    return !we_already_have_the_block && writeback_has_no_objections;
}

void mc_cache_t::maybe_unregister_read_ahead_callback() {
    // Unregister when 90 % of the cache are filled up.
    if (read_ahead_registered && page_repl.is_full(dynamic_config.max_size / serializer->get_block_size().ser_value() / 10 + 1)) {
        read_ahead_registered = false;
        // unregister_read_ahead_cb requires a coro context, but we might not be in any
        coro_t::spawn_now_dangerously(boost::bind(&serializer_t::unregister_read_ahead_cb, serializer, this));
    }
}

void mc_cache_t::adjust_max_patches_size_ratio_toward_minimum() {
    rassert(MAX_PATCHES_SIZE_RATIO_MAX <= MAX_PATCHES_SIZE_RATIO_MIN);  // just to make things clear.
    max_patches_size_ratio = static_cast<unsigned int>(0.9 * max_patches_size_ratio + 0.1 * MAX_PATCHES_SIZE_RATIO_MIN);
    rassert(max_patches_size_ratio <= MAX_PATCHES_SIZE_RATIO_MIN);
    rassert(max_patches_size_ratio >= MAX_PATCHES_SIZE_RATIO_MAX);
}

void mc_cache_t::adjust_max_patches_size_ratio_toward_maximum() {
    rassert(MAX_PATCHES_SIZE_RATIO_MAX <= MAX_PATCHES_SIZE_RATIO_MIN);  // just to make things clear.
    // We should be paranoid that if max_patches_size_ratio ==
    // MAX_PATCHES_SIZE_RATIO_MAX, (i.e. that 2 == 2) then 0.9f * 2 +
    // 0.1f * 2 will be less than 2 (and then round down to 1).
    max_patches_size_ratio = static_cast<unsigned int>(0.9 * max_patches_size_ratio + 0.1 * MAX_PATCHES_SIZE_RATIO_MAX);
    if (max_patches_size_ratio < MAX_PATCHES_SIZE_RATIO_MAX) {
        max_patches_size_ratio = MAX_PATCHES_SIZE_RATIO_MAX;
    }
    rassert(max_patches_size_ratio <= MAX_PATCHES_SIZE_RATIO_MIN);
    rassert(max_patches_size_ratio >= MAX_PATCHES_SIZE_RATIO_MAX);
}

