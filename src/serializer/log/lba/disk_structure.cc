#include "disk_structure.hpp"

void lba_disk_structure_t::create(extent_manager_t *em, fd_t fd, lba_disk_structure_t **out) {

    lba_disk_structure_t *s = new lba_disk_structure_t();
    s->em = em;
    s->fd = fd;
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
                owner->em, owner->fd,
                metablock->last_lba_extent_offset,
                metablock->last_lba_extent_entries_count,
                &owner->last_extent, this);
        } else {
            owner->last_extent = NULL;
            waiting_for_last_extent = false;
        }
        
        if (metablock->lba_superblock_offset != NULL_OFFSET) {
            waiting_for_superblock = !lba_disk_superblock_t::load(
                owner->em, owner->fd,
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

bool lba_disk_structure_t::load(extent_manager_t *em, fd_t fd, lba_metablock_mixin_t *metablock,
    lba_disk_structure_t **out, lba_disk_structure_t::load_callback_t *cb) {
    
    lba_disk_structure_t *s = new lba_disk_structure_t();
    s->em = em;
    s->fd = fd;
    
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

bool lba_disk_structure_t::write(lba_entry_t *entries, int nentries, lba_metablock_mixin_t *mb_out, sync_callback_t *cb) {
    
    /* Record and sync the changes */
    
    lba_writer_t *writer = new lba_writer_t(cb);
    
    for (int i = 0; i < nentries; i ++) {
        
        if (last_extent && last_extent->full()) {
            
            if (!superblock) {
                lba_disk_superblock_t::create(em, fd, &superblock);
            }
            
            superblock->extents.push_back(last_extent);
            superblock->modified = true;
            
            if (!last_extent->sync(writer)) writer->outstanding_cbs++;
            
            last_extent = NULL;
        }
        
        if (!last_extent) {
            lba_disk_extent_t::create(em, fd, &last_extent);
        }
        
        assert(!last_extent->full());
        last_extent->add_entry(entries[i].block_id, entries[i].offset);
    }
    
    if (last_extent) {
        if (!last_extent->sync(writer)) writer->outstanding_cbs++;
    }
    
    if (superblock) {
        if (!superblock->sync(writer)) writer->outstanding_cbs++;
    }
    
    /* Update the metablock */
    
    if (last_extent) {
        mb_out->last_lba_extent_offset = last_extent->offset;
        mb_out->last_lba_extent_entries_count = last_extent->count;
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
    
    /* Figure out if we need to block or not */
    
    if (writer->outstanding_cbs == 0) {
        delete writer;
        return true;
    } else {
        return false;
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