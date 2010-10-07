
#include <vector>
#include "utils.hpp"
#include "lba_list.hpp"
#include "cpu_context.hpp"
#include "arch/arch.hpp"

lba_list_t::lba_list_t(data_block_manager_t *dbm, extent_manager_t *em)
    : shutdown_callback(NULL), gc_fsm(NULL), data_block_manager(dbm), extent_manager(em),
      state(state_unstarted), in_memory_index(NULL), disk_structure(NULL)
    {}

/* This form of start() is called when we are creating a new database */
void lba_list_t::start(direct_file_t *file) {
    
    assert(state == state_unstarted);
    
    dbfile = file;
    
    in_memory_index = new in_memory_index_t();
    
    lba_disk_structure_t::create(extent_manager, dbfile, &disk_structure);
    
    state = state_ready;
}

struct lba_start_fsm_t :
    private lba_disk_structure_t::load_callback_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, lba_start_fsm_t>
{
    enum {
        state_start,
        state_loading_lba,
        state_done
    } state;
    
    lba_list_t *owner;
    
    lba_list_t::ready_callback_t *callback;
    
    lba_start_fsm_t(lba_list_t *l)
        : state(state_start), owner(l)
        {}
    
    ~lba_start_fsm_t() {
        assert(state == state_start || state == state_done);
    }
    
    bool run(lba_list_t::metablock_mixin_t *last_metablock, lba_list_t::ready_callback_t *cb) {
        
        assert(state == state_start);
        
        assert(owner->state == lba_list_t::state_unstarted);
        owner->state = lba_list_t::state_starting_up;
        
        bool done = lba_disk_structure_t::load(owner->extent_manager, owner->dbfile, last_metablock,
            &owner->disk_structure, this);
        
        if (done) {
            callback = NULL;
            finish();
            return true;
        } else {
            callback = cb;
            return false;
        }
    }
    
    void on_load_lba() {
        finish();
    }
    
    void finish() {
        owner->in_memory_index = new in_memory_index_t(owner->disk_structure, owner->data_block_manager, owner->extent_manager);
        owner->state = lba_list_t::state_ready;
        
        if (callback) callback->on_lba_ready();
        delete this;
    }
};

/* This form of start() is called when we are loading an existing database */
bool lba_list_t::start(direct_file_t *file, metablock_mixin_t *last_metablock, ready_callback_t *cb) {
    
    assert(state == state_unstarted);
    
    dbfile = file;
    
    lba_start_fsm_t *starter = new lba_start_fsm_t(this);
    return starter->run(last_metablock, cb);
}

block_id_t lba_list_t::gen_block_id() {
    assert(state == state_ready);
    
    return in_memory_index->gen_block_id();
}

block_id_t lba_list_t::max_block_id() {
    assert(state == state_ready);
    
    return in_memory_index->max_block_id();
}

off64_t lba_list_t::get_block_offset(block_id_t block) {
    assert(state == state_ready);
    
    return in_memory_index->get_block_offset(block);
}

#ifndef NDEBUG
bool lba_list_t::is_extent_referenced(off64_t offset) {
    return in_memory_index->is_extent_referenced(offset / EXTENT_SIZE);
}

bool lba_list_t::is_offset_referenced(off64_t offset) {
    return in_memory_index->is_offset_referenced(offset);
}

int lba_list_t::extent_refcount(off64_t offset) {
    return in_memory_index->extent_refcount(offset / EXTENT_SIZE);
}
#endif

void lba_list_t::set_block_offset(block_id_t block, off64_t offset) {
    assert(state == state_ready);
    
    in_memory_index->set_block_offset(block, offset);
    
    /* Strangely enough, this works even with the GC. Here's the reasoning: If the GC is
    waiting for the disk structure lock, then sync() will never be called again on the
    current disk_structure, so it's meaningless but harmless to call add_entry(). However,
    since our changes are also being put into the in_memory_index, they will be
    incorporated into the new disk_structure that the GC creates, so they won't get lost. */
    disk_structure->add_entry(block, offset);
}

void lba_list_t::delete_block(block_id_t block) {
    assert(state == state_ready);
    
    in_memory_index->delete_block(block);
    
    /* See set_block_offset() for why this is OK even when the GC is running */
    disk_structure->add_entry(block, DELETE_BLOCK);
}

struct lba_syncer_t :
    public lba_disk_structure_t::sync_callback_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, lba_syncer_t>
{
    
    lba_list_t *owner;
    lba_syncer_t(lba_list_t *owner) : owner(owner) {}

    lba_list_t::sync_callback_t *callback;
    
    bool run(lba_list_t::sync_callback_t *cb) {
        callback = NULL;
        if (do_write()) {
            return true;
        } else {
            callback = cb;
            return false;
        }
    }
    
    bool do_write() {
        if (owner->disk_structure->sync(this)) return finish();
        else return false;
    }
    
    void on_sync_lba() {
        finish();
    }
    
    bool finish() {
        if (callback) callback->on_lba_sync();
        delete this;
        return true;
    }
};

bool lba_list_t::sync(sync_callback_t *cb) {
    assert(state == state_ready);
    
    // Just to make sure that the LBA GC gets exercised
    if (rand() % 5 == 1) gc();
    
    lba_syncer_t *syncer = new lba_syncer_t(this);
    return syncer->run(cb);
}

void lba_list_t::prepare_metablock(metablock_mixin_t *mb_out) {
    disk_structure->prepare_metablock(mb_out);
}

struct gc_fsm_t :
    public lock_available_callback_t,
    public lba_disk_structure_t::sync_callback_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, gc_fsm_t>
{
    lba_list_t *owner;
    gc_fsm_t(lba_list_t *owner)
        : owner(owner)
        {
            do_replace_disk_structure();
        }
    
    void on_lock_available() {
        do_replace_disk_structure();
    }
    
    void do_replace_disk_structure() {
        
        /* Replace the LBA with a new empty LBA */
        
        owner->disk_structure->destroy();
        lba_disk_structure_t::create(owner->extent_manager, owner->dbfile, &owner->disk_structure);
        
        /* Put entries in the new empty LBA */
        
        for (block_id_t id = 0; id < owner->max_block_id(); id++) {
            owner->disk_structure->add_entry(id, owner->get_block_offset(id));
        }
        
        /* Sync the new LBA */
        
        if (owner->disk_structure->sync(this)) {
            do_cleanup();
        }
    }
    
    void on_sync_lba() {
        do_cleanup();
    }
    
    void do_cleanup() {
        owner->gc_fsm = NULL;

        if(owner->state == lba_list_t::state_shutting_down)
            owner->__shutdown();
        
        delete this;
    }
};

void lba_list_t::gc() {
    if(!gc_fsm) {
        // Only gc if we're not gc'ing already
        gc_fsm = new gc_fsm_t(this);
    }
}

bool lba_list_t::shutdown(shutdown_callback_t *cb) {
    assert(state == state_ready);
    assert(cb);
    
    if(gc_fsm) {
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
    delete in_memory_index;
    in_memory_index = NULL;
    
    disk_structure->shutdown();   // Also deletes it
    disk_structure = NULL;
    
    state = state_shut_down;

    if(shutdown_callback)
        shutdown_callback->on_lba_shutdown();

    return true;
}

lba_list_t::~lba_list_t() {
    assert(state == state_unstarted || state == state_shut_down);
    assert(in_memory_index == NULL);
    assert(disk_structure == NULL);
}
