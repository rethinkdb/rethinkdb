
#include <vector>
#include "utils.hpp"
#include "disk_format.hpp"
#include "lba_list.hpp"
#include "arch/arch.hpp"
#include "logger.hpp"

perfmon_counter_t pm_serializer_lba_gcs("serializer_lba_gcs");

lba_list_t::lba_list_t(extent_manager_t *em, size_t block_size)
    : shutdown_callback(NULL), gc_count(0), extent_manager(em),
      state(state_unstarted), in_memory_index(block_size)
{
    for (int i = 0; i < LBA_SHARD_FACTOR; i++) disk_structures[i] = NULL;
}

void lba_list_t::prepare_initial_metablock(metablock_mixin_t *mb) {

    for (int i = 0; i < LBA_SHARD_FACTOR; i++) {
        mb->shards[i].lba_superblock_offset = NULL_OFFSET;
        mb->shards[i].lba_superblock_entries_count = 0;
        mb->shards[i].last_lba_extent_offset = NULL_OFFSET;
        mb->shards[i].last_lba_extent_entries_count = 0;
    }
}

struct lba_start_fsm_t :
    private lba_disk_structure_t::load_callback_t,
    private lba_disk_structure_t::read_callback_t
{
    int cbs_out;
    lba_list_t *owner;
    lba_list_t::ready_callback_t *callback;
    
    lba_start_fsm_t(lba_list_t *l, lba_list_t::metablock_mixin_t *last_metablock)
        : owner(l), callback(NULL)
    {
        rassert(owner->state == lba_list_t::state_unstarted);
        owner->state = lba_list_t::state_starting_up;
        
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
    
    void on_lba_read() {
        rassert(cbs_out > 0);
        cbs_out--;
        if (cbs_out == 0) {
            owner->state = lba_list_t::state_ready;
            if (callback) callback->on_lba_ready();
            delete this;
        }
    }
};

bool lba_list_t::start_existing(direct_file_t *file, metablock_mixin_t *last_metablock, ready_callback_t *cb) {
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

ser_block_id_t lba_list_t::end_block_id() {
    rassert(state == state_ready);
    
    return in_memory_index.end_block_id();
}

flagged_off64_t lba_list_t::get_block_offset(ser_block_id_t block) {
    rassert(state == state_ready);

    return in_memory_index.get_block_info(block).offset;
}

repli_timestamp lba_list_t::get_block_recency(ser_block_id_t block) {
    rassert(state == state_ready);

    return in_memory_index.get_block_info(block).recency;
}

bool lba_list_t::is_offset_indexed(off64_t offset) {
    rassert(state == state_ready);

    return in_memory_index.is_offset_indexed(offset);
}

ser_block_id_t lba_list_t::get_block_id(off64_t offset) {
    rassert(state == state_ready);

    return in_memory_index.get_block_id(offset);
}

void lba_list_t::set_block_info(ser_block_id_t block, repli_timestamp recency, flagged_off64_t offset) {
    rassert(state == state_ready);

    in_memory_index.set_block_info(block, recency, offset);

    /* Strangely enough, this works even with the GC. Here's the reasoning: If the GC is
    waiting for the disk structure lock, then sync() will never be called again on the
    current disk_structure, so it's meaningless but harmless to call add_entry(). However,
    since our changes are also being put into the in_memory_index, they will be
    incorporated into the new disk_structure that the GC creates, so they won't get lost. */
    disk_structures[block.value % LBA_SHARD_FACTOR]->add_entry(block, recency, offset);
}

struct lba_syncer_t :
    public lba_disk_structure_t::sync_callback_t
{
    lba_list_t *owner;
    bool done, should_delete_self;
    int structures_unsynced;
    lba_list_t::sync_callback_t *callback;
    
    explicit lba_syncer_t(lba_list_t *owner)
        : owner(owner), done(false), should_delete_self(false), callback(NULL)
    {
        structures_unsynced = LBA_SHARD_FACTOR;
        for (int i = 0; i < LBA_SHARD_FACTOR; i++) {
            owner->disk_structures[i]->sync(this);
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

bool lba_list_t::sync(sync_callback_t *cb) {
    rassert(state == state_ready);
    
    lba_syncer_t *syncer = new lba_syncer_t(this);
    if (syncer->done) {
        delete syncer;
        return true;
    } else {
        syncer->should_delete_self = true;
        syncer->callback = cb;
        return false;
    }
}

void lba_list_t::prepare_metablock(metablock_mixin_t *mb_out) {
    for (int i = 0; i < LBA_SHARD_FACTOR; i++) {
        disk_structures[i]->prepare_metablock(&mb_out->shards[i]);
    }
}

void lba_list_t::consider_gc() {
    for (int i = 0; i < LBA_SHARD_FACTOR; i++) {
        if (we_want_to_gc(i)) gc(i);
    }
}

struct gc_fsm_t :
    public lba_disk_structure_t::sync_callback_t
{
    lba_list_t *owner;
    int i;
    
    gc_fsm_t(lba_list_t *owner, int i)
        : owner(owner), i(i)
        {
            pm_serializer_lba_gcs++;
            owner->gc_count++;
            do_replace_disk_structure();
        }
    
    void do_replace_disk_structure() {
        /* Replace the LBA with a new empty LBA */
        
        owner->disk_structures[i]->destroy();
        owner->disk_structures[i] = new lba_disk_structure_t(owner->extent_manager, owner->dbfile);
        
        /* Put entries in the new empty LBA */

        for (ser_block_id_t::number_t id = i, end_id = owner->end_block_id().value;
             id < end_id;
             id += LBA_SHARD_FACTOR) {
            ser_block_id_t block_id = ser_block_id_t::make(id);
            flagged_off64_t off = owner->get_block_offset(block_id);
            if (flagged_off64_t::has_value(off)) {
                owner->disk_structures[i]->add_entry(block_id, owner->get_block_recency(block_id), off);
            }
        }

        /* Sync the new LBA */
        
        owner->disk_structures[i]->sync(this);
    }
    
    void on_lba_sync() {
        rassert(owner->gc_count > 0);
        owner->gc_count--;

        if(owner->state == lba_list_t::state_shutting_down && owner->gc_count == 0)
            owner->__shutdown();
        
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
    int64_t entries_live = end_block_id().value / LBA_SHARD_FACTOR;
    if ((entries_live / (float)entries_total) > LBA_MIN_UNGARBAGE_FRACTION) {
        return false;
    }

    // Looks like we're GCing
    return true;
}

void lba_list_t::gc(int i) {
    new gc_fsm_t(this, i);
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
        return __shutdown();
    }
}

bool lba_list_t::__shutdown() {
    for (int i = 0; i < LBA_SHARD_FACTOR; i++) {
        disk_structures[i]->shutdown();   // Also deletes it
        disk_structures[i] = NULL;
    }
    
    state = state_shut_down;

    if(shutdown_callback)
        shutdown_callback->on_lba_shutdown();

    return true;
}

lba_list_t::~lba_list_t() {
    rassert(state == state_unstarted || state == state_shut_down);
    for (int i = 0; i < LBA_SHARD_FACTOR; i++) rassert(disk_structures[i] == NULL);
}
