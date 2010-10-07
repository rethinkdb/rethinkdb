#include "disk_structure.hpp"

void lba_disk_structure_t::create(extent_manager_t *em, direct_file_t *file, lba_disk_structure_t **out) {

    lba_disk_structure_t *s = new lba_disk_structure_t();
    s->em = em;
    s->file = file;
    s->superblock = NULL;
    s->last_extent = NULL;
    
    *out = s;
}

struct lba_load_fsm_t :
    lba_disk_extent_t::load_callback_t,
    lba_disk_superblock_t::load_callback_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, lba_load_fsm_t>
{
    lba_disk_structure_t *owner;
    lba_load_fsm_t(lba_disk_structure_t *owner) : owner(owner) {
    }
    
    lba_disk_structure_t::load_callback_t *callback;
    bool waiting_for_superblock;
    bool waiting_for_last_extent;
    
    bool run(lba_metablock_mixin_t *metablock, lba_disk_structure_t::load_callback_t *cb) {
        
        if (metablock->last_lba_extent_offset != NULL_OFFSET) {
            waiting_for_last_extent = !lba_disk_extent_t::load(
                owner->em, owner->file,
                metablock->last_lba_extent_offset,
                metablock->last_lba_extent_entries_count,
                &owner->last_extent, this);
        } else {
            owner->last_extent = NULL;
            waiting_for_last_extent = false;
        }
        
        if (metablock->lba_superblock_offset != NULL_OFFSET) {
            waiting_for_superblock = !lba_disk_superblock_t::load(
                owner->em, owner->file,
                metablock->lba_superblock_offset,
                metablock->lba_superblock_entries_count,
                &owner->superblock, this);
        } else {
            owner->superblock = NULL;
            waiting_for_superblock = false;
        }
        
        if (!waiting_for_superblock && !waiting_for_last_extent) {
            callback = NULL;
            finish();
            return true;
        } else {
            callback = cb;
            return false;
        }
    }
    
    void on_load_lba_superblock() {
        assert(waiting_for_superblock);
        waiting_for_superblock = false;
        if (!waiting_for_superblock && !waiting_for_last_extent) finish();
    }
    
    void on_load_lba_extent() {
        assert(waiting_for_last_extent);
        waiting_for_last_extent = false;
        if (!waiting_for_superblock && !waiting_for_last_extent) finish();
    }
    
    void finish() {
        if (callback) callback->on_load_lba();
        delete this;
    }
};

bool lba_disk_structure_t::load(extent_manager_t *em, direct_file_t *file, lba_metablock_mixin_t *metablock,
    lba_disk_structure_t **out, lba_disk_structure_t::load_callback_t *cb) {
    
    lba_disk_structure_t *s = new lba_disk_structure_t();
    s->em = em;
    s->file = file;
    
    *out = s;
    
    lba_load_fsm_t *loader = new lba_load_fsm_t(s);
    return loader->run(metablock, cb);
}

struct lba_writer_t :
    public lba_disk_extent_t::sync_callback_t,
    public lba_disk_superblock_t::sync_callback_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, lba_writer_t>
{
    
    int outstanding_cbs;
    lba_disk_structure_t::sync_callback_t *callback;
    
    lba_writer_t(lba_disk_structure_t::sync_callback_t *cb) {
        outstanding_cbs = 0;
        callback = cb;
    }
    
    void on_sync_lba_superblock() {
        outstanding_cbs --;
        if (outstanding_cbs == 0) finish();
    }
    
    void on_sync_lba_extent() {
        outstanding_cbs --;
        if (outstanding_cbs == 0) finish();
    }
    
    void finish() {
        if (callback) callback->on_sync_lba();
        delete this;
    }
};

void lba_disk_structure_t::add_entry(block_id_t block_id, off64_t offset) {
    
    if (last_extent && last_extent->full()) {
        
        if (!superblock) {
            lba_disk_superblock_t::create(em, file, &superblock);
        }
        
        superblock->extents.push_back(last_extent);
        superblock->modified = true;
        
        last_extent = NULL;
    }
    
    if (!last_extent) {
        lba_disk_extent_t::create(em, file, &last_extent);
    }
    
    assert(!last_extent->full());
    last_extent->add_entry(block_id, offset);
}

bool lba_disk_structure_t::sync(sync_callback_t *cb) {
    
    lba_writer_t *writer = new lba_writer_t(cb);
    
    if (last_extent) {
        printf("sync::last_extent exists\n");
        if (!last_extent->sync(writer)) writer->outstanding_cbs++;
    } else {
        printf("sync::last_extent does not exist\n");
    }
    
    if (superblock) {
        
        /* Sync each extent hanging off the superblock. */
        /* TODO: It might make more sense for superblock->sync() to also sync all the extents
        attached to the superblock, since superblock->load() loads all the extents attached to the
        superblock. */
        for (lba_disk_extent_t *e = superblock->extents.head(); e; e = superblock->extents.next(e)) {
            if (!e->sync(writer)) writer->outstanding_cbs++;
        }
        
        /* Sync the superblock itself */
        if (!superblock->sync(writer)) writer->outstanding_cbs++;
    }
    
    /* Figure out if we need to block or not */
    
    if (writer->outstanding_cbs == 0) {
        delete writer;
        return true;
    } else {
        return false;
    }
}

void lba_disk_structure_t::prepare_metablock(lba_metablock_mixin_t *mb_out) {
    
    if (last_extent) {
        mb_out->last_lba_extent_offset = last_extent->offset;
        mb_out->last_lba_extent_entries_count = last_extent->count;

        assert((offsetof(lba_extent_t, entries[0]) + sizeof(lba_entry_t) * mb_out->last_lba_extent_entries_count)
               % DEVICE_BLOCK_SIZE == 0);
    } else {
        mb_out->last_lba_extent_offset = NULL_OFFSET;
        mb_out->last_lba_extent_entries_count = 0;
    }
    
    if (superblock) {
        mb_out->lba_superblock_offset = superblock->offset;
        mb_out->lba_superblock_entries_count = superblock->extents.size();
    } else {
        mb_out->lba_superblock_offset = NULL_OFFSET;
        mb_out->lba_superblock_entries_count = 0;
    }
}

void lba_disk_structure_t::destroy() {
    if (superblock) superblock->destroy();
    if (last_extent) last_extent->destroy();
    delete this;
}

void lba_disk_structure_t::shutdown() {
    if (superblock) superblock->shutdown();
    if (last_extent) last_extent->shutdown();
    delete this;
}
