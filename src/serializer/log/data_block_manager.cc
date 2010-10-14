#include "data_block_manager.hpp"
#include "log_serializer.hpp"
#include "utils.hpp"
#include "arch/arch.hpp"

void data_block_manager_t::start(direct_file_t *file) {
    
    assert(state == state_unstarted);
    dbfile = file;
    last_data_extent = NULL_OFFSET;
    blocks_in_last_data_extent = 0;

    state = state_ready;
}

void data_block_manager_t::start(direct_file_t *file, metablock_mixin_t *last_metablock) {

    assert(state == state_unstarted);
    dbfile = file;
    last_data_extent = last_metablock->last_data_extent;
    blocks_in_last_data_extent = last_metablock->blocks_in_last_data_extent;
    state = state_ready;
}

bool data_block_manager_t::read(off64_t off_in, void *buf_out, iocallback_t *cb) {

    assert(state == state_ready);
    
    dbfile->read_async(off_in, block_size, buf_out, cb);
    
    return false;
}

bool data_block_manager_t::write(void *buf_in, off64_t *off_out, iocallback_t *cb) {
    // Either we're ready to write, or we're shutting down and just
    // finished reading blocks for gc and called do_write.
    assert(state == state_ready
           || (state == state_shutting_down && gc_state.step == gc_read));
    
    off64_t offset = *off_out = gimme_a_new_offset();
    
    dbfile->write_async(offset, block_size, buf_in, cb);
    
    return false;
}

void data_block_manager_t::mark_garbage(off64_t offset) {

    unsigned int extent_id = (offset / extent_manager->extent_size);
    unsigned int block_id = (offset % extent_manager->extent_size) / block_size;
    
    gc_entry *entry = entries.get(extent_id);
    assert(entry->g_array[block_id] == 0);
    entry->g_array.set(block_id, 1);
    
    if (entry->g_array.count() == entry->g_array.size()) {
    
        /* Every block in the extent is now garbage. */
        extent_manager->release_extent(entry->offset);
        if (entry->our_pq_entry) gc_pq.remove(entry->our_pq_entry);
        delete entry;
        entries.set(extent_id, NULL);
        if (gc_state.current_entry == entry) gc_state.current_entry = NULL;
        
    } else if (entry->our_pq_entry) {
        entry->our_pq_entry->update();
    }

    gc_stats.garbage_blocks++;
}

void data_block_manager_t::start_reconstruct() {
    gc_state.step = gc_reconstruct;
}

void data_block_manager_t::mark_live(off64_t offset) {
    assert(gc_state.step == gc_reconstruct);
    unsigned int extent_id = (offset / extent_manager->extent_size);
    unsigned int block_id = (offset % extent_manager->extent_size) / block_size;

    if (entries.get(extent_id) == NULL) {
    
        gc_entry *entry = new gc_entry;
        entry->offset = extent_id * extent_manager->extent_size;
        entry->active = false;
        entry->g_array.set(); //set everything to garbage
        entry->our_pq_entry = gc_pq.push(entry);
        entries.set(extent_id, entry);
        
        extent_manager->reserve_extent(entry->offset);
    }

    /* mark the block as alive */
    assert(entries.get(extent_id)->g_array[block_id] == 1);
    entries.get(extent_id)->g_array.set(block_id, 0);
}

void data_block_manager_t::end_reconstruct() {
    for (unsigned int i = blocks_in_last_data_extent; i < extent_manager->extent_size / BTREE_BLOCK_SIZE; i++)
        mark_live(last_data_extent + (BTREE_BLOCK_SIZE * i));

    gc_state.step = gc_ready;
}

void data_block_manager_t::start_gc() {
    if (gc_state.step == gc_ready)
        run_gc();
}

/* TODO this currently cleans extent by extent, we should tune it to always have a certain number of outstanding blocks
 */
void data_block_manager_t::run_gc() {

    switch (gc_state.step) {
    
        case gc_ready:
            //TODO, need to make sure we don't gc the extent we're writing to
            if (gc_pq.empty() || !gc_criterion()(*gc_pq.peak())) return;

            /* grab the entry */
            gc_state.current_entry = gc_pq.pop();
            gc_state.current_entry->our_pq_entry = NULL;

            /* read all the live data into buffers */

            /* make sure the read callback knows who we are */
            gc_state.gc_read_callback.parent = this;

            for (unsigned int i = 0; i < extent_manager->extent_size / BTREE_BLOCK_SIZE; i++) {
                if (!gc_state.current_entry->g_array[i]) {
                    dbfile->read_async(
                        gc_state.current_entry->offset + (i * block_size),
                        block_size,
                        gc_state.gc_blocks + (i * block_size),
                        &(gc_state.gc_read_callback));
                    gc_state.refcount++;
                }
            }
            assert(gc_state.refcount > 0);
            gc_state.step = gc_read;
            break;
            
        case gc_read:
            gc_state.refcount--;
            if (gc_state.refcount > 0) {
                /* We got a block, but there are still more to go */
                break;
                
            } else {
                /* If other forces cause all of the blocks in the extent to become garbage
                before we even finish GCing it, they will set current_extent to NULL. */
                if (gc_state.current_entry == NULL) {
                    gc_state.step = gc_ready;
                    break;
                }
                
                /* an array to put our writes in */
                int num_writes = gc_state.current_entry->g_array.size() - gc_state.current_entry->g_array.count();
                log_serializer_t::write_t writes[num_writes];
                int current_write = 0;

                for (unsigned int i = 0; i < extent_manager->extent_size / BTREE_BLOCK_SIZE; i++) {
                    /* We re-check the bit array here in case a write came in for one of the
                    blocks we are GCing. We wouldn't want to overwrite the new valid data with
                    out-of-date data. */
                    if (!gc_state.current_entry->g_array[i]) {
                        writes[current_write].block_id = *((ser_block_id_t *) (gc_state.gc_blocks + (i * block_size)));
                        writes[current_write].buf = gc_state.gc_blocks + (i * block_size);
                        writes[current_write].callback = NULL;
                        current_write++;
                    }
                }
                
                assert(current_write == num_writes);
                
                /* make sure the callback knows who we are */
                gc_state.gc_write_callback.parent = this;
                
                gc_state.step = gc_write;

                /* schedule the write */
                bool done = serializer->do_write(writes, num_writes, &gc_state.gc_write_callback);
                if (!done) break;
            }
            
        case gc_write:
            /* Our write should have forced all of the blocks in the extent to become garbage,
            which should have caused the extent to be released and gc_state.current_offset to
            become NULL. */
            assert(gc_state.current_entry == NULL);
            
            assert(gc_state.refcount == 0);

            gc_state.step = gc_ready;

            /* update stats */
            gc_stats.total_blocks -= EXTENT_SIZE / BTREE_BLOCK_SIZE;
            gc_stats.garbage_blocks -= EXTENT_SIZE / BTREE_BLOCK_SIZE;

            if(state == state_shutting_down) {
                if(shutdown_callback)
                    shutdown_callback->on_datablock_manager_shutdown();
                state = state_shut_down;
                return;
            }
            
            run_gc();   // We might want to start another GC round
            break;
            
        default: fail("Unknown gc_step");
    }
}

void data_block_manager_t::prepare_metablock(metablock_mixin_t *metablock) {
    
    assert(state == state_ready
           || (state == state_shutting_down && gc_state.step == gc_read));
    
    metablock->last_data_extent = last_data_extent;
    metablock->blocks_in_last_data_extent = blocks_in_last_data_extent;
}

bool data_block_manager_t::shutdown(shutdown_callback_t *cb) {
    assert(cb);
    assert(state == state_ready);

    if(gc_state.step != gc_ready) {
        state = state_shutting_down;
        shutdown_callback = cb;
        return false;
    }
    
    state = state_shut_down;
    return true;
}

off64_t data_block_manager_t::gimme_a_new_offset() {
    
    if (last_data_extent == NULL_OFFSET) {
        
        last_data_extent = extent_manager->gen_extent();
        blocks_in_last_data_extent = 0;
        add_gc_entry();
    }
    
    off64_t offset = last_data_extent + blocks_in_last_data_extent * block_size;
    blocks_in_last_data_extent ++;
    
    if (blocks_in_last_data_extent == extent_manager->extent_size / block_size) {
       
        /* deactivate the last extent */
        entries.get(last_data_extent / EXTENT_SIZE)->active = false;
        entries.get(last_data_extent / EXTENT_SIZE)->our_pq_entry->update();

        last_data_extent = NULL_OFFSET;
    }
    
    return offset;
}

void data_block_manager_t::add_gc_entry() {
    
    gc_entry *entry = new gc_entry;
    entry->offset = last_data_extent;
    entry->active = true;
    entry->our_pq_entry = gc_pq.push(entry);
    
    unsigned int extent_id = last_data_extent / extent_manager->extent_size;
    assert(entries.get(extent_id) == NULL);
    entries.set(extent_id, entry);

    /* update stats */
    gc_stats.total_blocks += EXTENT_SIZE / BTREE_BLOCK_SIZE;
}

/* functions for gc structures */

bool data_block_manager_t::gc_criterion::operator() (const gc_entry entry) {
    return entry.g_array.count() >= ((EXTENT_SIZE / BTREE_BLOCK_SIZE) * 3) / 4 && !entry.active; // 3/4 garbage
}

/* !< is x less than y */
bool data_block_manager_t::Less::operator() (const data_block_manager_t::gc_entry *x, const data_block_manager_t::gc_entry *y) {
    if (x->active)
        return true;
    else if (y->active)
        return false;
    else
        return x->g_array.count() < y->g_array.count();
}

/****************
 *Stat functions*
 ****************/
float  data_block_manager_t::garbage_ratio() {
    return (float) gc_stats.garbage_blocks / (float) gc_stats.total_blocks;
}
