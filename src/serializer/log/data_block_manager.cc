#include "data_block_manager.hpp"
#include "log_serializer.hpp"
#include "utils.hpp"
#include "arch/arch.hpp"

void data_block_manager_t::start(direct_file_t *file) {
    
    assert(state == state_unstarted);
    dbfile = file;
    last_data_extent = extent_manager->gen_extent();
    blocks_in_last_data_extent = 0;

    add_gc_entry();

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

    if ((gc_state.step == gc_read || gc_state.step == gc_write)
        && gc_state.current_entry.offset / extent_manager->extent_size == extent_id)
    {
        gc_state.current_entry.g_array.set(block_id, 1);
    } else {
        assert(entries.get(extent_id));
        entries.get(extent_id)->data.g_array.set(block_id, 1);
        entries.get(extent_id)->update();
    }


    gc_stats.garbage_blocks++;
}

void data_block_manager_t::start_reconstruct() {
    gc_state.step = gc_reconstruct;
}

void data_block_manager_t::mark_live(off64_t offset) {
    // This is called at startup.
    assert(gc_state.step == gc_reconstruct);

    unsigned int extent_id = (offset / extent_manager->extent_size);
    unsigned int block_id = (offset % extent_manager->extent_size) / block_size;

    if (entries.get(extent_id) == NULL) {
        gc_entry entry;
        entry.offset = extent_id * extent_manager->extent_size;
        entry.active = false;
        entry.g_array.set(); //set everything to garbage

        entries.set(extent_id, gc_pq.push(entry));
    }

    /* mark the block as alive */
    entries.get(extent_id)->data.g_array.set(block_id, 0);
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
    do {
        bool fallthrough;
        switch (gc_state.step) {
            case gc_ready:
                fallthrough = false;
                //TODO, need to make sure we don't gc the extent we're writing to
                if (should_we_keep_gcing(gc_pq.peak())) {
                    /* grab the entry */
                    gc_state.current_entry = gc_pq.pop();

                    // The reason why the GC deletes the entries instead of the PQ deleting them
                    // is in case we get a write to the block we are GCing.
                    gdelete(entries.get(gc_state.current_entry.offset / extent_manager->extent_size));
                    entries.set(gc_state.current_entry.offset / extent_manager->extent_size, NULL);

                    /* read all the live data into buffers */

                    /* make sure the read callback knows who we are */
                    gc_state.gc_read_callback.parent = this;

                    for (unsigned int i = 0; i < extent_manager->extent_size / BTREE_BLOCK_SIZE; i++) {
                        if (!gc_state.current_entry.g_array[i]) {
                            dbfile->read_async(
                                gc_state.current_entry.offset + (i * block_size),
                                block_size,
                                gc_state.gc_blocks + (i * block_size),
                                &(gc_state.gc_read_callback));
                            gc_state.refcount++;
                            gc_state.blocks_copying++;
                        }
                    }
                    gc_state.step = gc_read;

                    if (gc_state.refcount == 0)
                        fallthrough = true;
                } else {
                    return;
                }
                if (!fallthrough)
                    break;
            case gc_read:
                fallthrough = false;
                /* refcount can == 0 here if we fell through */
                gc_state.refcount--;
                if (gc_state.refcount <= 0) {
                    /* we can get here with a refcount of 0 */
                    gc_state.refcount = 0;

                    /* check if blocks need to be copied */
                    if (gc_state.current_entry.g_array.count() < gc_state.current_entry.g_array.size()) {
                        /* an array to put our writes in */
                        log_serializer_t::write_t writes[gc_state.current_entry.g_array.size() - gc_state.current_entry.g_array.count()];
                        int nwrites = 0;

                        for (unsigned int i = 0; i < extent_manager->extent_size / BTREE_BLOCK_SIZE; i++) {
                            if (!gc_state.current_entry.g_array[i]) {
                                writes[nwrites].block_id = *((ser_block_id_t *) (gc_state.gc_blocks + (i * block_size)));
                                writes[nwrites].buf = gc_state.gc_blocks + (i * block_size);
                                writes[nwrites].callback = NULL;
                                nwrites++;
                            }
                        }
                        /* make sure the callback knows who we are */
                        gc_state.gc_write_callback.parent = this;

                        /* release the gc'ed extent (it's safe because
                         * the extent manager will ensure it won't be
                         * given away until the metablock is
                         * written) */
                        extent_manager->release_extent(gc_state.current_entry.offset);
                        
                        /* schedule the write */
                        fallthrough = serializer->do_write(writes, gc_state.current_entry.g_array.size() - gc_state.current_entry.g_array.count() , (log_serializer_t::write_txn_callback_t *) &gc_state.gc_write_callback);
                    } else {
                        fallthrough = true;
                    }

                    gc_state.step = gc_write;
                }
                if (!fallthrough)
                    break;
            case gc_write:
                //not valid when writes are asynchronous (it would be nice if we could have this)
                assert(gc_state.current_entry.g_array.count() == gc_state.current_entry.g_array.size()); //Checks if everything in the array has been marked as garbage
                assert(entries.get(gc_state.current_entry.offset / extent_manager->extent_size) == NULL);

                assert(gc_state.refcount == 0);
                gc_state.blocks_copying = 0;

                gc_state.step = gc_ready;

                /* update stats */
                gc_stats.total_blocks -= extent_manager->extent_size / BTREE_BLOCK_SIZE;
                gc_stats.garbage_blocks -= extent_manager->extent_size / BTREE_BLOCK_SIZE;

                if(state == state_shutting_down) {
                    if(shutdown_callback)
                        shutdown_callback->on_datablock_manager_shutdown();
                    state = state_shut_down;
                    return;
                }
                
                break;
            default:
                fail("Unknown gc_step");
                break;
        }
    } while (gc_state.step == gc_ready); 
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

    if (blocks_in_last_data_extent == extent_manager->extent_size / block_size) {
        /* deactivate the last extent */
        entries.get(last_data_extent / extent_manager->extent_size)->data.active = false;
        entries.get(last_data_extent / extent_manager->extent_size)->update();

        last_data_extent = extent_manager->gen_extent();
        blocks_in_last_data_extent = 0;

        add_gc_entry();
    }
    off64_t offset = last_data_extent + blocks_in_last_data_extent * block_size;
    blocks_in_last_data_extent ++;
    return offset;
}

void data_block_manager_t::add_gc_entry() {
    gc_entry entry;
    entry.offset = last_data_extent;
    entry.active = true;
    unsigned int extent_id = last_data_extent / extent_manager->extent_size;

    assert(entries.get(extent_id) == NULL);

    entries.set(extent_id, gc_pq.push(entry));

    /* update stats */
    gc_stats.total_blocks += extent_manager->extent_size / BTREE_BLOCK_SIZE;
}

/* functions for gc structures */

// Right now we start gc'ing when the garbage ratio is >= 0.75 (75%
// garbage) and we gc all blocks with that ratio or higher.

bool data_block_manager_t::should_we_keep_gcing(const gc_entry entry) {
    return entry.g_array.count() >= ((extent_manager->extent_size / BTREE_BLOCK_SIZE) * GC_THRESHOLD_RATIO_NUMERATOR) / GC_THRESHOLD_RATIO_DENOMINATOR && !entry.active; // 3/4 garbage
}

bool data_block_manager_t::do_we_want_to_start_gcing() {
    return gc_stats.garbage_blocks * GC_THRESHOLD_RATIO_DENOMINATOR >= GC_THRESHOLD_RATIO_NUMERATOR * gc_stats.total_blocks;
}

/* !< is x less than y */
bool data_block_manager_t::Less::operator() (const data_block_manager_t::gc_entry x, const data_block_manager_t::gc_entry y) {
    if (x.active)
        return true;
    else if (y.active)
        return false;
    else
        return x.g_array.count() < y.g_array.count();
}

/****************
 *Stat functions*
 ****************/

float  data_block_manager_t::garbage_ratio() {
    // TODO: not divide by zero?
    return (float) gc_stats.garbage_blocks / (float) gc_stats.total_blocks;
}
