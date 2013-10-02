// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "serializer/log/lba/lba_list.hpp"

#include "utils.hpp"
#include "serializer/log/lba/disk_format.hpp"
#include "arch/arch.hpp"
#include "perfmon/perfmon.hpp"
#include "serializer/log/stats.hpp"

// TODO: Some of the code in this file is bullshit disgusting shit.

lba_list_t::lba_list_t(extent_manager_t *em)
    : shutdown_callback(NULL), gc_count(0), extent_manager(em),
      state(state_unstarted),
      inline_lba_entries_count(0)
{
    for (int i = 0; i < LBA_SHARD_FACTOR; i++) disk_structures[i] = NULL;
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

bool lba_list_t::start_existing(file_t *file, metablock_mixin_t *last_metablock, ready_callback_t *cb) {
    rassert(state == state_unstarted);

    dbfile = file;

    lba_start_fsm_t *starter = new lba_start_fsm_t(this, last_metablock);
    if (state == state_ready) {
        return true;
    } else {
        starter->callback = cb;
        return false;
    }
}

block_id_t lba_list_t::end_block_id() {
    rassert(state == state_ready);

    return in_memory_index.end_block_id();
}

index_block_info_t lba_list_t::get_block_info(block_id_t block) {
    rassert(state == state_ready);
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

void lba_list_t::set_block_info(block_id_t block, repli_timestamp_t recency,
                                flagged_off64_t offset, uint32_t ser_block_size,
                                file_account_t *io_account, extent_transaction_t *txn) {
    rassert(state == state_ready);

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
    // TODO (daniel): Regarding garbage collection and inline LBA entries:
    //   The GC will write LBA entries to an extent which are also still in the inline LBA.
    //   When we move those entries over to the extents in this function, we write them again.
    //   This is not necessary, because the GC has already written them.
    //   It's probably not a big deal, but we do sometimes write a few redundant
    //   LBA entries due to this.
    
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

bool lba_list_t::sync(file_account_t *io_account, sync_callback_t *cb) {
    rassert(state == state_ready);

    lba_syncer_t *syncer = new lba_syncer_t(this, io_account);
    if (syncer->done) {
        delete syncer;
        return true;
    } else {
        syncer->should_delete_self = true;
        syncer->callback = cb;
        return false;
    }
}

void lba_list_t::consider_gc(file_account_t *io_account, extent_transaction_t *txn) {
    for (int i = 0; i < LBA_SHARD_FACTOR; i++) {
        if (we_want_to_gc(i)) {
            gc(i, io_account, txn);
        }
    }
}

class gc_fsm_t :
    public lba_disk_structure_t::sync_callback_t
{
public:
    lba_list_t *owner;
    int i;

    gc_fsm_t(lba_list_t *_owner, int _i, file_account_t *io_account, extent_transaction_t *txn) : owner(_owner), i(_i) {
        ++owner->extent_manager->stats->pm_serializer_lba_gcs; //This is kinda bad that we go through the extent manager... but whatever
        ++owner->gc_count;
        do_replace_disk_structure(io_account, txn);
    }

    void do_replace_disk_structure(file_account_t *io_account, extent_transaction_t *txn) {
        /* Replace the LBA with a new empty LBA */

        owner->disk_structures[i]->destroy(txn);
        owner->disk_structures[i] = new lba_disk_structure_t(owner->extent_manager, owner->dbfile);

        /* Put entries in the new empty LBA */

        for (block_id_t id = i, end_id = owner->end_block_id();
             id < end_id;
             id += LBA_SHARD_FACTOR) {
            block_id_t block_id = id;
            flagged_off64_t off = owner->get_block_offset(block_id);
            if (off.has_value()) {
                uint32_t ser_block_size = owner->get_ser_block_size(block_id);
                owner->disk_structures[i]->add_entry(block_id,
                                                     owner->get_block_recency(block_id),
                                                     off, ser_block_size,
                                                     io_account, txn);
            }
        }

        /* Sync the new LBA */

        owner->disk_structures[i]->sync(io_account, this);
    }

    void on_lba_sync() {
        rassert(owner->gc_count > 0);
        owner->gc_count--;

        if (owner->state == lba_list_t::state_shutting_down && owner->gc_count == 0)
            owner->shutdown_now();

        delete this;
    }
};

// Decides, based on the number of unused entries.
bool lba_list_t::we_want_to_gc(int i) {

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

void lba_list_t::gc(int i, file_account_t *io_account, extent_transaction_t *txn) {
    new gc_fsm_t(this, i, io_account, txn);
}

bool lba_list_t::shutdown(shutdown_callback_t *cb) {
    rassert(state == state_ready);
    rassert(cb);

    if (gc_count > 0) {
        // We're gc'ing, can't shut down just yet...
        state = state_shutting_down;
        shutdown_callback = cb;
        return false;
    } else {
        shutdown_callback = NULL;
        return shutdown_now();
    }
}

bool lba_list_t::shutdown_now() {
    for (int i = 0; i < LBA_SHARD_FACTOR; i++) {
        disk_structures[i]->shutdown();   // Also deletes it
        disk_structures[i] = NULL;
    }

    state = state_shut_down;

    if (shutdown_callback)
        shutdown_callback->on_lba_shutdown();

    return true;
}

lba_list_t::~lba_list_t() {
    rassert(state == state_unstarted || state == state_shut_down);
    for (int i = 0; i < LBA_SHARD_FACTOR; i++) rassert(disk_structures[i] == NULL);
}
