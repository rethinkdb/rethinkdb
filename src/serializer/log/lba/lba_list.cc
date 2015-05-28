// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "serializer/log/lba/lba_list.hpp"

#include "utils.hpp"
#include "serializer/log/lba/disk_format.hpp"
#include "arch/arch.hpp"
#include "perfmon/perfmon.hpp"
#include "serializer/log/stats.hpp"
#include "arch/runtime/coroutines.hpp"

// TODO: Some of the code in this file is bullshit disgusting shit.

lba_list_t::lba_list_t(extent_manager_t *em,
        const lba_list_t::write_metablock_fun_t &_write_metablock_fun)
    : gc_drainer(new auto_drainer_t), write_metablock_fun(_write_metablock_fun),
      extent_manager(em), state(state_unstarted), inline_lba_entries_count(0)
{
    for (int i = 0; i < LBA_SHARD_FACTOR; i++) {
        gc_active[i] = false;
        disk_structures[i] = NULL;
    }
}

void lba_list_t::prepare_initial_metablock(metablock_mixin_t *mb_out) {

    for (int i = 0; i < LBA_SHARD_FACTOR; i++) {
        mb_out->shards[i].lba_superblock_offset = NULL_OFFSET;
        mb_out->shards[i].lba_superblock_entries_count = 0;
        mb_out->shards[i].last_lba_extent_offset = NULL_OFFSET;
        mb_out->shards[i].last_lba_extent_entries_count = 0;
    }
    mb_out->inline_lba_entries_count = 0;
    memset(mb_out->inline_lba_entries,
           0,
           LBA_NUM_INLINE_ENTRIES * sizeof(lba_entry_t));
}

void lba_list_t::prepare_metablock(metablock_mixin_t *mb_out) {
    for (int i = 0; i < LBA_SHARD_FACTOR; i++) {
        disk_structures[i]->prepare_metablock(&mb_out->shards[i]);
    }
    rassert(inline_lba_entries_count <= LBA_NUM_INLINE_ENTRIES);
    mb_out->inline_lba_entries_count = inline_lba_entries_count;
    memcpy(mb_out->inline_lba_entries,
           inline_lba_entries,
           inline_lba_entries_count * sizeof(lba_entry_t));
    // Zero the remaining entries so we don't write uninitialized data to disk.
    memset(&mb_out->inline_lba_entries[inline_lba_entries_count],
           0,
           (LBA_NUM_INLINE_ENTRIES - inline_lba_entries_count) * sizeof(lba_entry_t));
}

class lba_start_fsm_t :
    private lba_disk_structure_t::load_callback_t,
    private lba_disk_structure_t::read_callback_t
{
public:
    int cbs_out;
    lba_list_t *owner;
    lba_list_t::ready_callback_t *callback;

    lba_start_fsm_t(lba_list_t *l, lba_list_t::metablock_mixin_t *last_metablock)
        : owner(l), callback(NULL)
    {
        rassert(owner->state == lba_list_t::state_unstarted);
        owner->state = lba_list_t::state_starting_up;

        // Copy the current set of inline LBA entries from the metablock
        guarantee(last_metablock->inline_lba_entries_count <= LBA_NUM_INLINE_ENTRIES);
        owner->inline_lba_entries_count = last_metablock->inline_lba_entries_count;
        memcpy(owner->inline_lba_entries,
               last_metablock->inline_lba_entries,
               last_metablock->inline_lba_entries_count * sizeof(lba_entry_t));

        cbs_out = LBA_SHARD_FACTOR;
        for (int i = 0; i < LBA_SHARD_FACTOR; i++) {
            owner->disk_structures[i] = new lba_disk_structure_t(
                owner->extent_manager, owner->dbfile,
                &last_metablock->shards[i]);
            owner->disk_structures[i]->set_load_callback(this);
        }
    }

    void on_lba_load() {
        rassert(cbs_out > 0);
        cbs_out--;
        if (cbs_out == 0) {
            cbs_out = LBA_SHARD_FACTOR;
            for (int i = 0; i < LBA_SHARD_FACTOR; i++) {
                owner->disk_structures[i]->read(&owner->in_memory_index, this);
            }
        }
    }

    void on_lba_extents_read() {
        rassert(cbs_out > 0);
        cbs_out--;
        if (cbs_out == 0) {
            // All LBA entries from the LBA extents have been read.
            // Now we can load the (more recent) inlined entries from
            // the metablock into the index:
            for (int32_t i = 0; i < owner->inline_lba_entries_count; ++i) {
                lba_entry_t *e = &owner->inline_lba_entries[i];
                owner->in_memory_index.set_block_info(
                        e->block_id,
                        e->recency,
                        e->offset,
                        e->ser_block_size);
            }

            owner->state = lba_list_t::state_ready;
            if (callback) callback->on_lba_ready();
            delete this;
        }
    }
};

bool lba_list_t::start_existing(rdb_file_t *file, metablock_mixin_t *last_metablock,
        ready_callback_t *cb) {
    rassert(state == state_unstarted);

    dbfile = file;
    gc_io_account.init(new file_account_t(dbfile, LBA_GC_IO_PRIORITY));

    lba_start_fsm_t *starter = new lba_start_fsm_t(this, last_metablock);
    if (state == state_ready) {
        return true;
    } else {
        starter->callback = cb;
        return false;
    }
}

block_id_t lba_list_t::end_block_id() {
    rassert(state == state_ready || state == state_gc_shutting_down);

    return in_memory_index.end_block_id();
}

index_block_info_t lba_list_t::get_block_info(block_id_t block) {
    rassert(state == state_ready || state == state_gc_shutting_down);
    return in_memory_index.get_block_info(block);
}

flagged_off64_t lba_list_t::get_block_offset(block_id_t block) {
    return get_block_info(block).offset;
}

uint32_t lba_list_t::get_ser_block_size(block_id_t block) {
    return get_block_info(block).ser_block_size;
}

block_size_t lba_list_t::get_block_size(block_id_t block) {
    return block_size_t::unsafe_make(get_block_info(block).ser_block_size);
}

repli_timestamp_t lba_list_t::get_block_recency(block_id_t block) {
    return get_block_info(block).recency;
}

segmented_vector_t<repli_timestamp_t> lba_list_t::get_block_recencies(block_id_t first,
                                                                      block_id_t step) {
    guarantee(coro_t::self() != NULL);
    rassert(state == state_ready);
    segmented_vector_t<repli_timestamp_t> ret;
    block_id_t end = in_memory_index.end_block_id();
    block_id_t count = 0;
    for (block_id_t i = first; i < end; i += step) {
        ++count;
        if (count % 1024 == 0) {
            coro_t::yield();
            // Nothing interesting should happen, this is called when the cache is
            // starting up.
            rassert(state == state_ready);
        }
        ret.push_back(in_memory_index.get_block_info(i).recency);
    }
    return ret;
}

void lba_list_t::set_block_info(block_id_t block, repli_timestamp_t recency,
                                flagged_off64_t offset, uint32_t ser_block_size,
                                file_account_t *io_account, extent_transaction_t *txn) {
    rassert(state == state_ready || state == state_gc_shutting_down);

    in_memory_index.set_block_info(block, recency, offset, ser_block_size);

    // If the inline LBA is full, free it up first by moving its entries to
    // the LBA extents
    if (check_inline_lba_full()) {
        move_inline_entries_to_extents(io_account, txn);
        rassert(!check_inline_lba_full());
    }
    // Then store the entry inline
    add_inline_entry(block, recency, offset, ser_block_size);
}

bool lba_list_t::check_inline_lba_full() const {
    rassert(inline_lba_entries_count <= LBA_NUM_INLINE_ENTRIES);
    return inline_lba_entries_count == LBA_NUM_INLINE_ENTRIES;
}

void lba_list_t::move_inline_entries_to_extents(file_account_t *io_account, extent_transaction_t *txn) {
    // Note that there are two slight inefficiencies (w.r.t. i/o) in this code:
    // 1. Regarding garbage collection and inline LBA entries:
    //   The GC will write LBA entries to an extent which are also still in the inline LBA.
    //   When we move those entries over to the extents in this function, we write them again.
    //   This is not necessary, because the GC has already written them.
    //   It's probably not a big deal, but we do sometimes write a few redundant
    //   LBA entries due to this.
    // 2. Also, if an entry which is still in the inline LBA has already
    //   been deprecated by a more recent inlined entry, we are still going to write
    //   the already deprecated entry to the LBA extent first. We could check
    //   whether an entry still matches the in-memory LBA before we move
    //   it to disc_structures. On the other hand that would mean more complicated code
    //   and a little more CPU overhead.

    // Note that the order is important here. The oldest inline entries have to be
    // written first, because they might have been superseded by newer inline entries.
    for (int32_t i = 0; i < inline_lba_entries_count; ++i) {
        const lba_entry_t &e = inline_lba_entries[i];

        /* Strangely enough, this works even with the GC. Here's the reasoning: If the GC is
        waiting for the disk structure lock, then sync() will never be called again on the
        current disk_structure, so it's meaningless but harmless to call add_entry(). However,
        since our changes are also being put into the in_memory_index, they will be
        incorporated into the new disk_structure that the GC creates, so they won't get lost. */
        disk_structures[e.block_id % LBA_SHARD_FACTOR]->add_entry(
                e.block_id,
                e.recency,
                e.offset,
                e.ser_block_size,
                io_account,
                txn);
    }

    inline_lba_entries_count = 0;
}

void lba_list_t::add_inline_entry(block_id_t block, repli_timestamp_t recency,
                                flagged_off64_t offset, uint32_t ser_block_size) {

    rassert(!check_inline_lba_full());
    inline_lba_entries[inline_lba_entries_count++] =
            lba_entry_t::make(block, recency, offset, ser_block_size);
}

class lba_syncer_t :
    public lba_disk_structure_t::sync_callback_t
{
public:
    lba_list_t *owner;
    bool done, should_delete_self;
    int structures_unsynced;
    lba_list_t::sync_callback_t *callback;

    lba_syncer_t(lba_list_t *_owner, file_account_t *io_account)
        : owner(_owner), done(false), should_delete_self(false), callback(NULL)
    {
        structures_unsynced = LBA_SHARD_FACTOR;
        for (int i = 0; i < LBA_SHARD_FACTOR; i++) {
            owner->disk_structures[i]->sync(io_account, this);
        }
    }

    void on_lba_sync() {
        rassert(structures_unsynced > 0);
        structures_unsynced--;
        if (structures_unsynced == 0) {
            done = true;
            if (callback) callback->on_lba_sync();
            if (should_delete_self) delete this;
        }
    }
};

void lba_list_t::sync(file_account_t *io_account, sync_callback_t *cb) {
    rassert(state == state_ready || state == state_gc_shutting_down);

    lba_syncer_t *syncer = new lba_syncer_t(this, io_account);
    if (syncer->done) {
        delete syncer;
        cb->on_lba_sync();
    } else {
        syncer->should_delete_self = true;
        syncer->callback = cb;
    }
}

void lba_list_t::consider_gc() {
    for (int i = 0; i < LBA_SHARD_FACTOR; i++) {
        if (we_want_to_gc(i)) {
            rassert(!gc_active[i]);
            gc_active[i] = true;
            coro_t *gc_coro = coro_t::spawn_sometime(std::bind(&lba_list_t::gc,
                    this, i, auto_drainer_t::lock_t(gc_drainer.get())));
            gc_coro->set_priority(CORO_PRIORITY_LBA_GC);
        }
    }
}

void lba_list_t::gc(int lba_shard, auto_drainer_t::lock_t) {
    ++extent_manager->stats->pm_serializer_lba_gcs;

    // Start a transaction
    std::vector<scoped_ptr_t<extent_transaction_t> > txns;
    txns.push_back(make_scoped<extent_transaction_t>());
    extent_manager->begin_transaction(txns.back().get());

    // Fetch a list of current LBA extents, minus the active one
    const std::set<lba_disk_extent_t *> gced_extents =
        disk_structures[lba_shard]->get_inactive_extents();

    // Begin rewriting all LBA entries, one batch of entries at a time.
    int num_written_in_batch = 0;
    bool aborted = false;
    const block_id_t end_id = end_block_id();
    for (block_id_t id = lba_shard; id < end_id; id += LBA_SHARD_FACTOR) {
        flagged_off64_t off = get_block_offset(id);
        if (off.has_value()) {
            uint32_t ser_block_size = get_ser_block_size(id);
            disk_structures[lba_shard]->add_entry(id,
                                                  get_block_recency(id),
                                                  off,
                                                  ser_block_size,
                                                  gc_io_account.get(),
                                                  txns.back().get());
        }

        ++num_written_in_batch;
        if (num_written_in_batch >= LBA_GC_BATCH_SIZE) {
            num_written_in_batch = 0;
            // End the transaction. We must not commit it though until after
            // we have written a new metablock (see below).
            extent_manager->end_transaction(txns.back().get());

            // Sync the LBA. This is simply to make the final sync at the end
            // (as well as other LBA syncs done from inside the serializer) finish
            // faster.
            // If those took too long, it would block out serializer index writes,
            // because those have to get in line for writing the metablock,
            // and `write_metablock()` has to wait for the LBA sync to complete.
            struct : public cond_t, public lba_disk_structure_t::sync_callback_t {
                void on_lba_sync() { pulse(); }
            } on_lba_sync;
            disk_structures[lba_shard]->sync(gc_io_account.get(), &on_lba_sync);

            on_lba_sync.wait();

            // Start a new transaction for the next batch of entries
            txns.push_back(make_scoped<extent_transaction_t>());
            extent_manager->begin_transaction(txns.back().get());

            // Check if we are shutting down. If yes, we simply abort garbage
            // collection.
            if (state == lba_list_t::state_gc_shutting_down) {
                aborted = true;
                break;
            }
        }
    }

    // Discard the old LBA extents
    if (!aborted) {
        disk_structures[lba_shard]->destroy_extents(gced_extents, gc_io_account.get(),
                                                    txns.back().get());
    }

    // Sync the changed LBA for a final time
    struct : public cond_t, public lba_disk_structure_t::sync_callback_t {
        void on_lba_sync() { pulse(); }
    } on_lba_sync;
    disk_structures[lba_shard]->sync(gc_io_account.get(), &on_lba_sync);

    // End the last extent manager transaction
    extent_manager->end_transaction(txns.back().get());

    // Write a new metablock once the LBA has synced. We have to do this before
    // we can commit the extent_manager transactions.
    write_metablock_fun(&on_lba_sync, gc_io_account.get());

    // Commit all extent transactions. From that point on the data of extents
    // we have deleted can be overwritten.
    for (auto txn = txns.begin(); txn != txns.end(); ++txn) {
        extent_manager->commit_transaction(txn->get());
    }

    gc_active[lba_shard] = false;
}

bool lba_list_t::is_any_gc_active() const {
    for (int i = 0; i < LBA_SHARD_FACTOR; ++i) {
        if (gc_active[i]) {
            return true;
        }
    }
    return false;
}

// Decides, based on the number of unused entries.
bool lba_list_t::we_want_to_gc(int i) {

    // Don't garbage collect if we are already garbage collecting
    if (gc_active[i]) {
        return false;
    }

    // Never start garbage collecting if we are shutting down the GC
    if (state == lba_list_t::state_gc_shutting_down) {
        return false;
    }

    // Don't count the extent we're currently writing to. If there is no superblock, then that
    // extent is the only one, so we don't want to GC obviously.
    if (disk_structures[i]->superblock_extent == NULL) {
        return false;
    }

    // If the LBA is under the threshold, then don't GC regardless of how much is garbage
    if (disk_structures[i]->extents_in_superblock.size() * extent_manager->extent_size <
            LBA_MIN_SIZE_FOR_GC / LBA_SHARD_FACTOR) {
        return false;
    }

    // How much space are we using on disk? How much of that space is absolutely necessary?
    // If we are not using more than N times the amount of space that we need, don't GC
    int entries_per_extent = disk_structures[i]->num_entries_that_can_fit_in_an_extent();
    int64_t entries_total = disk_structures[i]->extents_in_superblock.size() * entries_per_extent;
    int64_t entries_live = end_block_id() / LBA_SHARD_FACTOR;
    if ((entries_live / static_cast<double>(entries_total)) > LBA_MIN_UNGARBAGE_FRACTION) {  // TODO: multiply both sides by common denominator
        return false;
    }

    // Looks like we're GCing
    return true;
}

void lba_list_t::shutdown_gc() {
    guarantee(state == state_ready);
    guarantee(coro_t::self() != NULL);

    state = state_gc_shutting_down;

    // Wait for active GC coroutines to finish
    gc_drainer.reset();
}

void lba_list_t::shutdown() {
    guarantee(state == state_gc_shutting_down);
    guarantee(!is_any_gc_active());

    for (int i = 0; i < LBA_SHARD_FACTOR; i++) {
        disk_structures[i]->shutdown();   // Also deletes it
        disk_structures[i] = NULL;
    }

    gc_io_account.reset();

    state = state_shut_down;
}

lba_list_t::~lba_list_t() {
    rassert(state == state_unstarted || state == state_shut_down);
    for (int i = 0; i < LBA_SHARD_FACTOR; i++) rassert(disk_structures[i] == NULL);
}
