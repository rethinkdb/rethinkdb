#include "buffer_cache/mirrored/mirrored.hpp"
#include "buffer_cache/stats.hpp"

/**
 * Buffer implementation.
 */

/* This mini-FSM loads a buf from disk. */

struct load_buf_fsm_t :
    public thread_message_t,
    serializer_t::read_callback_t
{
    bool have_loaded;
    mc_inner_buf_t *inner_buf;
    explicit load_buf_fsm_t(mc_inner_buf_t *buf) : inner_buf(buf) {
        bool locked __attribute__((unused)) = inner_buf->lock.lock(rwi_write, NULL);
        rassert(locked);
        have_loaded = false;
        if (continue_on_thread(inner_buf->cache->serializer->home_thread, this)) on_thread_switch();
    }
    void on_thread_switch() {
        if (!have_loaded) {
            inner_buf->subtree_recency = inner_buf->cache->serializer->get_recency(inner_buf->block_id);
            if (inner_buf->cache->serializer->do_read(inner_buf->block_id, inner_buf->data, this))
                on_serializer_read();
        } else {
            // Read the transaction id
            buf_data_t *ser_data = (buf_data_t*)inner_buf->data;
            ser_data--;
            inner_buf->transaction_id = ser_data->transaction_id;

            const std::vector<buf_patch_t*>* patches = inner_buf->cache->diff_core_storage.get_patches(inner_buf->block_id);
            // Remove obsolete patches from diff storage
            if (patches) {
                inner_buf->cache->diff_core_storage.filter_applied_patches(inner_buf->block_id, inner_buf->transaction_id);
                patches = inner_buf->cache->diff_core_storage.get_patches(inner_buf->block_id);
            }
            // All patches that currently exist must have been materialized out of core...
            if (patches) {
                inner_buf->writeback_buf.last_patch_materialized = patches->back()->get_patch_counter();
            } else {
                inner_buf->writeback_buf.last_patch_materialized = 0; // Nothing of relevance is materialized (only obsolete patches if any).
            }

            /*if (patches) {
                fprintf(stderr, "Replaying %d patches on block %d\n", (int)patches->size(), inner_buf->block_id);
            }*/

            // Apply outstanding patches
            inner_buf->cache->diff_core_storage.apply_patches(inner_buf->block_id, (char*)inner_buf->data);
            
            // Set next_patch_counter such that the next patches get values consistent with the existing patches
            if (patches) {
                inner_buf->next_patch_counter = patches->back()->get_patch_counter() + 1;
            } else {
                inner_buf->next_patch_counter = 1;
            }

            inner_buf->lock.unlock();
            delete this;
        }
    }
    void on_serializer_read() {
        have_loaded = true;
        if (continue_on_thread(inner_buf->cache->home_thread, this)) on_thread_switch();
    }
};

// This form of the buf constructor is used when the block exists on disk and needs to be loaded

mc_inner_buf_t::mc_inner_buf_t(cache_t *cache, block_id_t block_id)
    : cache(cache),
      block_id(block_id),
      data(cache->serializer->malloc()),
      next_patch_counter(1),
      refcount(0),
      do_delete(false),
      cow_will_be_needed(false),
      writeback_buf(this),
      page_repl_buf(this),
      page_map_buf(this),
      transaction_id(NULL_SER_TRANSACTION_ID) {
    new load_buf_fsm_t(this);

    pm_n_blocks_in_memory++;
    refcount++; // Make the refcount nonzero so this block won't be considered safe to unload.
    cache->page_repl.make_space(1);
    refcount--;
}

// This form of the buf constructor is used when a completely new block is being created
mc_inner_buf_t::mc_inner_buf_t(cache_t *cache)
    : cache(cache),
      block_id(cache->free_list.gen_block_id()),
      subtree_recency(current_time()),
      data(cache->serializer->malloc()),
      next_patch_counter(1),
      refcount(0),
      do_delete(false),
      cow_will_be_needed(false),
      writeback_buf(this),
      page_repl_buf(this),
      page_map_buf(this),
      transaction_id(NULL_SER_TRANSACTION_ID) {
    cache->assert_thread();

#if !defined(NDEBUG) || defined(VALGRIND)
    // The memory allocator already filled this with 0xBD, but it's nice to be able to distinguish
    // between problems with uninitialized memory and problems with uninitialized blocks
    memset(data, 0xCD, cache->serializer->get_block_size().value());
#endif

    // Initialize the transaction id...
    buf_data_t *ser_data = (buf_data_t*)data;
    ser_data--;
    ser_data->transaction_id = NULL_SER_TRANSACTION_ID;
    
    pm_n_blocks_in_memory++;
    refcount++; // Make the refcount nonzero so this block won't be considered safe to unload.
    cache->page_repl.make_space(1);
    refcount--;
}

mc_inner_buf_t::~mc_inner_buf_t() {
    cache->assert_thread();
    
#ifndef NDEBUG
    // We're about to free the data, let's set it to a recognizable
    // value to make sure we don't depend on accessing things that may
    // be flushed out of the cache.
    memset(data, 0xDD, cache->serializer->get_block_size().value());
#endif
    
    rassert(safe_to_unload());
    cache->serializer->free(data);
    
    pm_n_blocks_in_memory--;
}

bool mc_inner_buf_t::safe_to_unload() {
    return !lock.locked() && writeback_buf.safe_to_unload() && refcount == 0 && !cow_will_be_needed;
}

perfmon_duration_sampler_t
    pm_bufs_acquiring("bufs_acquiring", secs_to_ticks(1)),
    pm_bufs_held("bufs_held", secs_to_ticks(1));

mc_buf_t::mc_buf_t(mc_inner_buf_t *inner, access_t mode)
    : ready(false), callback(NULL), mode(mode), non_locking_access(false), inner_buf(inner)
{
#ifndef FAST_PERFMON
    patches_affected_data_size_at_start = -1;
#endif

    pm_bufs_acquiring.begin(&start_time);
    inner_buf->refcount++;
    if (inner_buf->lock.lock(mode == rwi_read_outdated_ok ? rwi_read : mode, this)) {
        on_lock_available();
    }
}

void mc_buf_t::on_lock_available() {
    pm_bufs_acquiring.end(&start_time);
    
    inner_buf->cache->assert_thread();
    rassert(!inner_buf->do_delete);
    
    switch (mode) {
        case rwi_read: {
            data = inner_buf->data;
            break;
        }
        case rwi_read_outdated_ok: {
            if (inner_buf->cow_will_be_needed) {
                data = inner_buf->cache->serializer->malloc();
                memcpy(data, inner_buf->data, inner_buf->cache->get_block_size().value());
            } else {
                data = inner_buf->data;
                inner_buf->cow_will_be_needed = true;
                inner_buf->lock.unlock();
            }
            break;
        }
        case rwi_write: {
            if (inner_buf->cow_will_be_needed) {
                data = inner_buf->cache->serializer->malloc();
                memcpy(data, inner_buf->data, inner_buf->cache->get_block_size().value());
                inner_buf->data = data;
                inner_buf->cow_will_be_needed = false;
            }
            data = inner_buf->data;
#ifndef FAST_PERFMON
            if (!inner_buf->writeback_buf.needs_flush && patches_affected_data_size_at_start == -1) {
                patches_affected_data_size_at_start = inner_buf->cache->diff_core_storage.get_affected_data_size(inner_buf->block_id);
            }
#endif
            break;
        }
        case rwi_intent:
            not_implemented("Locking with intent not supported yet.");
        case rwi_upgrade:
        default:
            unreachable();
    }
    
    pm_bufs_held.begin(&start_time);
    
    ready = true;
    if (callback) callback->on_block_available(this);
}

void mc_buf_t::apply_patch(buf_patch_t& patch) {
    rassert(ready);
    rassert(!inner_buf->safe_to_unload()); // If this assertion fails, it probably means that you're trying to access a buf you don't own.
    rassert(!inner_buf->do_delete);
    rassert(mode == rwi_write);
    rassert(data == inner_buf->data);
    rassert(patch.get_block_id() == inner_buf->block_id);

    patch.apply_to_buf((char*)data);
    inner_buf->writeback_buf.set_dirty();

    // We cannot accept patches for blocks without a valid transaction id (newly allocated blocks)
    if (inner_buf->transaction_id == NULL_SER_TRANSACTION_ID)
        ensure_flush();

    // Check if we want to disable patching for this block and flush it directly instead
    if (!inner_buf->writeback_buf.needs_flush) {
        const size_t MAX_PATCHES_SIZE = inner_buf->cache->serializer->get_block_size().value() / MAX_PATCHES_SIZE_RATIO;
        if (patch.get_affected_data_size() + inner_buf->cache->diff_core_storage.get_affected_data_size(inner_buf->block_id) > MAX_PATCHES_SIZE) {
            ensure_flush();
        }
    }

    // Store the patch if the buffer does not have to be flushed anyway
    if (!inner_buf->writeback_buf.needs_flush) {
        if (patch.get_patch_counter() == 1) {
            // Clean up any left-over patches
            inner_buf->cache->diff_core_storage.drop_patches(inner_buf->block_id);
        }

        inner_buf->cache->diff_core_storage.store_patch(patch);
    }
    else
        delete &patch;
}

void *mc_buf_t::get_data_major_write() {
    rassert(ready);
    rassert(!inner_buf->safe_to_unload()); // If this assertion fails, it probably means that you're trying to access a buf you don't own.
    rassert(!inner_buf->do_delete);
    rassert(mode == rwi_write);
    rassert(data == inner_buf->data);

    inner_buf->assert_thread();

    ensure_flush();

    return data;
}

void mc_buf_t::ensure_flush() {
    rassert(data == inner_buf->data);
    if (!inner_buf->writeback_buf.needs_flush) {
        // We bypass the patching system, make sure this buffer gets flushed.
        inner_buf->writeback_buf.needs_flush = true;
        // ... we can also get rid of existing patches at this point.
        inner_buf->cache->diff_core_storage.drop_patches(inner_buf->block_id);
        // Make sure that the buf is marked as dirty
        inner_buf->writeback_buf.set_dirty();
    }
}

patch_counter_t mc_buf_t::get_next_patch_counter() {
    rassert(ready);
    rassert(!inner_buf->do_delete);
    rassert(mode == rwi_write);
    rassert(data == inner_buf->data);
    return inner_buf->next_patch_counter++;
}

void mc_buf_t::set_data(const void* dest, const void* src, const size_t n) {
    rassert(data == inner_buf->data);
    if (n == 0)
        return;
    rassert(dest >= data && (size_t)dest < (size_t)data + inner_buf->cache->get_block_size().value());
    rassert((size_t)dest + n <= (size_t)data + inner_buf->cache->get_block_size().value());

    if (inner_buf->writeback_buf.needs_flush) {
        // Save the allocation / construction of a patch object
        get_data_major_write();
        memcpy(const_cast<void*>(dest), src, n);
    } else {
        size_t offset = (const char*)dest - (const char*)data;
        // transaction ID will be set later...
        apply_patch(*(new memcpy_patch_t(inner_buf->block_id, get_next_patch_counter(), NULL_SER_TRANSACTION_ID, offset, (const char*)src, n)));
    }
}

void mc_buf_t::move_data(const void* dest, const void* src, const size_t n) {
    rassert(data == inner_buf->data);
    if (n == 0)
        return;
    rassert(dest >= data && (size_t)dest < (size_t)data + inner_buf->cache->get_block_size().value());
    rassert((size_t)dest + n <= (size_t)data + inner_buf->cache->get_block_size().value());
    rassert(src >= data && (size_t)src < (size_t)data + inner_buf->cache->get_block_size().value());
    rassert((size_t)src + n <= (size_t)data + inner_buf->cache->get_block_size().value());

    if (inner_buf->writeback_buf.needs_flush) {
        // Save the allocation / construction of a patch object
        get_data_major_write();
        memmove(const_cast<void*>(dest), src, n);
    } else {
        size_t dest_offset = (const char*)dest - (const char*)data;
        size_t src_offset = (const char*)src - (const char*)data;
        // transaction ID will be set later...
        apply_patch(*(new memmove_patch_t(inner_buf->block_id, get_next_patch_counter(), NULL_SER_TRANSACTION_ID, dest_offset, src_offset, n)));
    }
}

#ifndef FAST_PERFMON
perfmon_sampler_t pm_patches_size_per_write("patches_size_per_write_buf", secs_to_ticks(1), false);
#endif

void mc_buf_t::release() {
    pm_bufs_held.end(&start_time);

#ifndef FAST_PERFMON
    if (mode == rwi_write && !inner_buf->writeback_buf.needs_flush && patches_affected_data_size_at_start >= 0) {
        if (inner_buf->cache->diff_core_storage.get_affected_data_size(inner_buf->block_id) > (size_t)patches_affected_data_size_at_start)
            pm_patches_size_per_write.record(inner_buf->cache->diff_core_storage.get_affected_data_size(inner_buf->block_id) - patches_affected_data_size_at_start);
    }
#endif
    
    inner_buf->cache->assert_thread();
    
    inner_buf->refcount--;

    if (!non_locking_access) {
        switch (mode) {
            case rwi_read:
            case rwi_write: {
                inner_buf->lock.unlock();
                break;
            }
            case rwi_read_outdated_ok: {
                if (data == inner_buf->data) {
                    inner_buf->cow_will_be_needed = false;
                } else {
                    inner_buf->cache->serializer->free(data);
                }
                break;
            }
            case rwi_intent:
            case rwi_upgrade:
            default:
                unreachable("Unexpected mode.");
        }
    }
    
    // If this code is not commented out, then it will cause bufs to be unloaded very aggressively.
    // This is useful for catching bugs in which something expects a buf to remain valid even though
    // it is eligible to be unloaded.
    
    /*
    if (inner_buf->safe_to_unload()) {
        delete inner_buf;
    }
    */
    
    delete this;
}

mc_buf_t::~mc_buf_t() {
}

/**
 * Transaction implementation.
 */

perfmon_duration_sampler_t
    pm_transactions_starting("transactions_starting", secs_to_ticks(1)),
    pm_transactions_active("transactions_active", secs_to_ticks(1)),
    pm_transactions_committing("transactions_committing", secs_to_ticks(1));

mc_transaction_t::mc_transaction_t(cache_t *cache, access_t access)
    : cache(cache),
      access(access),
      begin_callback(NULL),
      commit_callback(NULL),
      state(state_open) {
    pm_transactions_starting.begin(&start_time);
    rassert(access == rwi_read || access == rwi_write);
}

mc_transaction_t::~mc_transaction_t() {
    rassert(state == state_committed);
    pm_transactions_committing.end(&start_time);
}

void mc_transaction_t::green_light() {
    pm_transactions_starting.end(&start_time);
    pm_transactions_active.begin(&start_time);
    begin_callback->on_txn_begin(this);
}

bool mc_transaction_t::commit(transaction_commit_callback_t *callback) {
    rassert(state == state_open);
    pm_transactions_active.end(&start_time);
    pm_transactions_committing.begin(&start_time);

    assert_thread();
    
    /* We have to call sync_patiently() before on_transaction_commit() so that if
    on_transaction_commit() starts a sync, we will get included in it */
    if (access == rwi_write && cache->writeback.wait_for_flush) {
        if (cache->writeback.sync_patiently(this)) {
            state = state_committed;
        } else {
            state = state_in_commit_call;
        }
    } else {
        state = state_committed;
    }
    
    cache->on_transaction_commit(this);
    
    if (state == state_in_commit_call) {
        state = state_committing;
        commit_callback = callback;
        return false;
    } else {
        delete this;
        return true;
    }
}

void mc_transaction_t::on_sync() {
    /* cache->on_transaction_commit() could cause on_sync() to be called even after sync_patiently()
    failed. To detect when this happens, we use the state state_in_commit_call. If we get an
    on_sync() while in state_in_commit_call, we know that we are still inside of commit(), so we
    don't delete ourselves yet and just set state to state_committed instead, thereby signalling
    commit() to delete us. I think there must be a better way to do this, but I can't think of it
    right now. */
    
    if (state == state_in_commit_call) {
        state = state_committed;
    } else if (state == state_committing) {
        state = state_committed;
        // TODO(NNW): We should push notifications through event queue.
        commit_callback->on_txn_commit(this);
        delete this;
    } else {
        unreachable("Unexpected state.");
    }
}

mc_buf_t *mc_transaction_t::allocate() {
    /* Make a completely new block, complete with a shiny new block_id. */
    rassert(access == rwi_write);
    assert_thread();
    
    // This form of the inner_buf_t constructor generates a new block with a new block ID.
    inner_buf_t *inner_buf = new inner_buf_t(cache);
    
    // This must pass since no one else holds references to this block.
    buf_t *buf = new buf_t(inner_buf, rwi_write);
    rassert(buf->ready);

    return buf;
}

mc_buf_t *mc_transaction_t::acquire(block_id_t block_id, access_t mode,
                                    block_available_callback_t *callback) {
    rassert(mode == rwi_read || mode == rwi_read_outdated_ok || access != rwi_read);
    assert_thread();
       
    inner_buf_t *inner_buf = cache->page_map.find(block_id);
    if (!inner_buf) {
        /* The buf isn't in the cache and must be loaded from disk */
        inner_buf = new inner_buf_t(cache, block_id);
    }

    buf_t *buf = new buf_t(inner_buf, mode);

    // We set the recency _before_ we get the buf.  This is correct,
    // because we are "underneath" any replicators that come later
    // (trees grow downward from the root).
    if (!(mode == rwi_read || mode == rwi_read_outdated_ok)) {
        buf->touch_recency();
    }

    if (buf->ready) {
        return buf;
    } else {
        buf->callback = callback;
        return NULL;
    }
}

repli_timestamp mc_transaction_t::get_subtree_recency(block_id_t block_id) {
    crash("Operation not implemented: mc_transaction_t::get_subtree_recency");
    /*
    inner_buf_t *inner_buf = cache->page_map.find(block_id);
    if (inner_buf) {
        // The buf is in the cache and we must use its recency.
        return inner_buf->subtree_recency;
    } else {
        // The buf is not in the cache, so ask the serializer.
        // This is dangerous and will make things crash.

        return cache->serializer->get_recency(block_id);

        // This is dangerous because we're not on the same core, being
        // on the same core would be a hassle for a feature that never
        // gets used so far.
    }
*/
}

/**
 * Cache implementation.
 */

mc_cache_t::mc_cache_t(
            translator_serializer_t *serializer,
            mirrored_cache_config_t *dynamic_config,
            mirrored_cache_static_config_t *static_config) :

    static_config(static_config),
    serializer(serializer),
    page_repl(
        // Launch page replacement if the user-specified maximum number of blocks is reached
        dynamic_config->max_size / serializer->get_block_size().ser_value(),
        this),
    writeback(
        this,
        dynamic_config->wait_for_flush,
        dynamic_config->flush_timer_ms,
        dynamic_config->flush_dirty_size / serializer->get_block_size().ser_value(),
        dynamic_config->max_dirty_size / serializer->get_block_size().ser_value(),
        dynamic_config->flush_waiting_threshold,
        dynamic_config->max_concurrent_flushes),
    free_list(serializer),
    shutdown_transaction_backdoor(false),
    state(state_unstarted),
    num_live_transactions(0),
    diff_oocore_storage(*this)
    {
}

mc_cache_t::~mc_cache_t() {
    rassert(state == state_unstarted || state == state_shut_down);

    while (inner_buf_t *buf = page_repl.get_first_buf()) {
       delete buf;
    }
    rassert(num_live_transactions == 0);
}

bool mc_cache_t::start(ready_callback_t *cb) {
    rassert(state == state_unstarted);

    state = state_starting_up_create_free_list;
    ready_callback = NULL;
    if (next_starting_up_step()) {
        return true;
    } else {
        ready_callback = cb;
        return false;
    }
}

bool mc_cache_t::next_starting_up_step() {
    if (state == state_starting_up_create_free_list) {
        if (free_list.start(this)) {
            state = state_starting_up_init_fixed_blocks;
        } else {
            state = state_starting_up_waiting_for_free_list;
            return false;
        }
    }
    
    if (state == state_starting_up_init_fixed_blocks) {
        /* Create an initial superblock */
        if (free_list.num_blocks_in_use == 0) {
            inner_buf_t *b = new mc_inner_buf_t(this);
            rassert(b->block_id == SUPERBLOCK_ID);
            bzero(b->data, get_block_size().value());
        }

        /* Initialize the diff storage (needs coro context) */
        coro_t::spawn(&mc_cache_t::init_diff_storage, this);

        return false;
    }

    if (state == state_starting_up_finish) {
        state = state_ready;
        
        if (ready_callback) ready_callback->on_cache_ready();
        ready_callback = NULL;
        
        return true;
    }
    
    unreachable("Invalid state.");
}

void mc_cache_t::init_diff_storage() {
    rassert(state == state_starting_up_init_fixed_blocks);
    diff_oocore_storage.init(SUPERBLOCK_ID + 1, static_config->n_diff_log_blocks);
    diff_oocore_storage.load_patches(diff_core_storage);

    state = state_starting_up_finish;
    call_later_on_this_thread(this);
}

void mc_cache_t::on_thread_switch() {
    rassert(state == state_starting_up_finish);
    next_starting_up_step();
}

void mc_cache_t::on_free_list_ready() {
    rassert(state == state_starting_up_waiting_for_free_list);
    state = state_starting_up_init_fixed_blocks;
    next_starting_up_step();
}

block_size_t mc_cache_t::get_block_size() {
    return serializer->get_block_size();
}

mc_transaction_t *mc_cache_t::begin_transaction(access_t access,
        transaction_begin_callback_t *callback) {
    
    assert_thread();
    // shutdown_transaction_backdoor allows the writeback to request a transaction for the shutdown
    // sync.
    rassert(state == state_ready ||
        (shutdown_transaction_backdoor &&
            (state == state_shutting_down_start_flush ||
                state == state_shutting_down_waiting_for_flush)));
    
    transaction_t *txn = new transaction_t(this, access);
    num_live_transactions++;
    if (writeback.begin_transaction(txn, callback)) {
        pm_transactions_starting.end(&txn->start_time);
        pm_transactions_active.begin(&txn->start_time);
        return txn;
    } else {
        return NULL;
    }
}

void mc_cache_t::on_transaction_commit(transaction_t *txn) {
    rassert(state == state_ready ||
        state == state_shutting_down_waiting_for_transactions ||
        state == state_shutting_down_start_flush ||
        state == state_shutting_down_waiting_for_flush);
    
    writeback.on_transaction_commit(txn);
    
    num_live_transactions--;
    if (state == state_shutting_down_waiting_for_transactions && num_live_transactions == 0) {
        // We started a shutdown earlier, but we had to wait for the transactions to all finish.
        // Now that all transactions are done, continue shutting down.
        state = state_shutting_down_start_flush;
        next_shutting_down_step();
    }
}

bool mc_cache_t::shutdown(shutdown_callback_t *cb) {
    rassert(state == state_ready);

    if (num_live_transactions == 0) {
        state = state_shutting_down_start_flush;
        shutdown_callback = NULL;
        if (next_shutting_down_step()) {
            return true;
        } else {
            shutdown_callback = cb;
            return false;
        }
    } else {
        // The shutdown will be resumed by on_transaction_commit() when the last transaction
        // completes.
        state = state_shutting_down_waiting_for_transactions;
        shutdown_callback = cb;
        return false;
    }
}

bool mc_cache_t::next_shutting_down_step() {
    if (state == state_shutting_down_start_flush) {
        if (writeback.sync(this)) {
            state = state_shutting_down_finish;
        } else {
            state = state_shutting_down_waiting_for_flush;
            return false;
        }
    }
    
    if (state == state_shutting_down_finish) {
        diff_oocore_storage.shutdown();

        /* Use do_later() rather than calling it immediately because it might call
        our destructor, and it might not be safe to call our destructor right here. */
        if (shutdown_callback) {
            do_later(boost::bind(&shutdown_callback_t::on_cache_shutdown, shutdown_callback));
        }
        shutdown_callback = NULL;
        state = state_shut_down;
        
        return true;
    }
    
    unreachable("Invalid state.");
}

void mc_cache_t::on_sync() {
    rassert(state == state_shutting_down_waiting_for_flush);
    state = state_shutting_down_finish;
    next_shutting_down_step();
}

