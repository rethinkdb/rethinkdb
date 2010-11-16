#include "data_block_manager.hpp"
#include "log_serializer.hpp"
#include "utils.hpp"
#include "arch/arch.hpp"

perfmon_counter_t
    pm_serializer_data_extents("serializer_data_extents"),
    pm_serializer_data_extents_allocated("serializer_data_extents_allocated[dexts]"),
    pm_serializer_data_extents_reclaimed("serializer_data_extents_reclaimed[dexts]"),
    pm_serializer_data_extents_gced("serializer_data_extents_gced[dexts]"),
    pm_serializer_data_blocks_written("serializer_data_blocks_written"),
    pm_serializer_old_garbage_blocks("serializer_old_garbage_blocks"),
    pm_serializer_old_total_blocks("serializer_old_total_blocks");

void data_block_manager_t::start_new(direct_file_t *file) {

    assert(state == state_unstarted);
    dbfile = file;
    
    for (unsigned int i = 0; i < MAX_ACTIVE_DATA_EXTENTS; i++) {
        active_extents[i] = NULL;
    }

    state = state_ready;
}

void data_block_manager_t::start_reconstruct() {

    assert(state == state_unstarted);
    gc_state.set_step(gc_reconstruct);
}

// Marks the block at the given offset as alive, in the appropriate
// gc_entry in the entries table.  (This is used when we start up, when
// everything is presumed to be garbage, until we mark it as
// non-garbage.)
void data_block_manager_t::mark_live(off64_t offset) {

    assert(gc_state.step() == gc_reconstruct);  // This is called at startup.

    unsigned int extent_id = (offset / extent_manager->extent_size);
    unsigned int block_id = (offset % extent_manager->extent_size) / static_config->block_size;

    if (entries.get(extent_id) == NULL) {
    
        gc_entry *entry = new gc_entry(this, extent_id * extent_manager->extent_size);
        entry->state = gc_entry::state_reconstructing;
        reconstructed_extents.push_back(entry);
    }

    /* mark the block as alive */
    assert(entries.get(extent_id)->g_array[block_id] == 1);
    entries.get(extent_id)->g_array.set(block_id, 0);
}

void data_block_manager_t::end_reconstruct() {

    assert(state == state_unstarted);
    gc_state.set_step(gc_ready);
}

void data_block_manager_t::start_existing(direct_file_t *file, metablock_mixin_t *last_metablock) {

    assert(state == state_unstarted);
    dbfile = file;
    
    /* Reconstruct the active data block extents from the metablock. */
    
    for (unsigned int i = 0; i < MAX_ACTIVE_DATA_EXTENTS; i++) {
        
        off64_t offset = last_metablock->active_extents[i];
        
        if (offset != NULL_OFFSET) {
            
            /* It is possible to have an active data block extent with no actual data
            blocks in it. In this case we would not have created a gc_entry for the extent
            yet. */
            if (entries.get(offset / extent_manager->extent_size) == NULL) {
                gc_entry *e = new gc_entry(this, offset);
                e->state = gc_entry::state_reconstructing;
                reconstructed_extents.push_back(e);
            }
            
            active_extents[i] = entries.get(offset / extent_manager->extent_size);
            assert(active_extents[i]);
            
            /* Turn the extent from a reconstructing extent into an active extent */
            assert(active_extents[i]->state == gc_entry::state_reconstructing);
            active_extents[i]->state = gc_entry::state_active;
            reconstructed_extents.remove(active_extents[i]);
            
            blocks_in_active_extent[i] = last_metablock->blocks_in_active_extent[i];
            
        } else {
            
            active_extents[i] = NULL;
            
        }
    }
    
    /* Convert any extents that we found live blocks in, but that are not active extents,
    into old extents */
    
    while (gc_entry *entry = reconstructed_extents.head()) {
    
        reconstructed_extents.remove(entry);
        
        assert(entry->state == gc_entry::state_reconstructing);
        entry->state = gc_entry::state_old;
        
        entry->our_pq_entry = gc_pq.push(entry);
        
        gc_stats.old_total_blocks += extent_manager->extent_size / static_config->block_size;
        gc_stats.old_garbage_blocks += entry->g_array.count();
    }
    
    state = state_ready;
}

bool data_block_manager_t::read(off64_t off_in, void *buf_out, iocallback_t *cb) {

    assert(state == state_ready);
    
    buf_data_t *data = (buf_data_t*)buf_out;
    data--;
    dbfile->read_async(off_in, static_config->block_size, data, cb);

    return false;
}

bool data_block_manager_t::write(void *buf_in, ser_block_id_t block_id, ser_transaction_id_t transaction_id, off64_t *off_out, iocallback_t *cb) {
    // Either we're ready to write, or we're shutting down and just
    // finished reading blocks for gc and called do_write.
    assert(state == state_ready
           || (state == state_shutting_down && gc_state.step() == gc_write));

    off64_t offset = *off_out = gimme_a_new_offset();
    
    pm_serializer_data_blocks_written++;
    
    buf_data_t *data = (buf_data_t*)buf_in;
    data--;
    *data = make_buf_data_t(block_id, transaction_id);
    
    dbfile->write_async(offset, static_config->block_size, data, cb);

    return false;
}

void data_block_manager_t::mark_garbage(off64_t offset) {

    unsigned int extent_id = (offset / extent_manager->extent_size);
    unsigned int block_id = (offset % extent_manager->extent_size) / static_config->block_size;
    
    gc_entry *entry = entries.get(extent_id);
    assert(entry->g_array[block_id] == 0);
    entry->g_array.set(block_id, 1);
    
    assert(entry->g_array.size() == extent_manager->extent_size / static_config->block_size);
    
    if (entry->state == gc_entry::state_old) {
        gc_stats.old_garbage_blocks++;
    }
    
    if (entry->g_array.count() == extent_manager->extent_size / static_config->block_size && entry->state != gc_entry::state_active) {
    
        /* Every block in the extent is now garbage. */
        
        switch (entry->state) {
            
            case gc_entry::state_reconstructing:
                fail("Marking something as garbage during startup.");
            
            case gc_entry::state_active:
                fail("We shouldn't have gotten here.");
            
            /* Remove from the young extent queue */
            case gc_entry::state_young:
                young_extent_queue.remove(entry);
                break;
            
            /* Remove from the priority queue */
            case gc_entry::state_old:
                gc_pq.remove(entry->our_pq_entry);
                gc_stats.old_total_blocks -= extent_manager->extent_size / static_config->block_size;
                gc_stats.old_garbage_blocks -= extent_manager->extent_size / static_config->block_size;
                break;
            
            /* Notify the GC that the extent got released during GC */
            case gc_entry::state_in_gc:
                assert(gc_state.current_entry == entry);
                gc_state.current_entry = NULL;
                break;
        }
        
        pm_serializer_data_extents_reclaimed++;
        
        entry->destroy();
        
    } else if (entry->state == gc_entry::state_old) {
    
        entry->our_pq_entry->update();
    }
}

void data_block_manager_t::start_gc() {
    if (gc_state.step() == gc_ready )
        run_gc();
}

void data_block_manager_t::run_gc() {
    bool run_again = true;
    while (run_again) {
        run_again = false;
        switch (gc_state.step()) {
    
        case gc_ready:
            if (gc_pq.empty() || !should_we_keep_gcing(*gc_pq.peak())) return;
            
            pm_serializer_data_extents_gced++;
            
            /* grab the entry */
            gc_state.current_entry = gc_pq.pop();
            gc_state.current_entry->our_pq_entry = NULL;
            
            assert(gc_state.current_entry->state == gc_entry::state_old);
            gc_state.current_entry->state = gc_entry::state_in_gc;
            gc_stats.old_garbage_blocks -= gc_state.current_entry->g_array.count();
            gc_stats.old_total_blocks -= extent_manager->extent_size / static_config->block_size;

            /* read all the live data into buffers */

            /* make sure the read callback knows who we are */
            gc_state.gc_read_callback.parent = this;

            for (unsigned int i = 0; i < extent_manager->extent_size / static_config->block_size; i++) {
                if (!gc_state.current_entry->g_array[i]) {
                    dbfile->read_async(
                        gc_state.current_entry->offset + (i * static_config->block_size),
                        static_config->block_size,
                        gc_state.gc_blocks + (i * static_config->block_size),
                        &(gc_state.gc_read_callback));
                    gc_state.refcount++;
                }
            }
            assert(gc_state.refcount > 0);
            gc_state.set_step(gc_read);
            break;
            
        case gc_read: {
            gc_state.refcount--;
            if (gc_state.refcount > 0) {
                /* We got a block, but there are still more to go */
                break;
            }    
            
            /* If other forces cause all of the blocks in the extent to become garbage
            before we even finish GCing it, they will set current_entry to NULL. */
            if (gc_state.current_entry == NULL) {
                gc_state.set_step(gc_ready);
                break;
            }
            
            /* an array to put our writes in */
            int num_writes = extent_manager->extent_size / static_config->block_size - gc_state.current_entry->g_array.count();
            log_serializer_t::write_t writes[num_writes];
            int current_write = 0;

            for (unsigned int i = 0; i < extent_manager->extent_size / static_config->block_size; i++) {
                /* We re-check the bit array here in case a write came in for one of the
                blocks we are GCing. We wouldn't want to overwrite the new valid data with
                out-of-date data. */
                if (!gc_state.current_entry->g_array[i]) {
                    writes[current_write].block_id = *((ser_block_id_t *) (gc_state.gc_blocks + (i * static_config->block_size)));
                    writes[current_write].buf = gc_state.gc_blocks + (i * static_config->block_size) + sizeof(buf_data_t);
                    writes[current_write].callback = NULL;
                    current_write++;
                }
            }
            
            assert(current_write == num_writes);
            
            /* make sure the callback knows who we are */
            gc_state.gc_write_callback.parent = this;
            
            gc_state.set_step(gc_write);

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

            gc_state.set_step(gc_ready);

            if(state == state_shutting_down) {
                actually_shutdown();
                return;
            }

            run_again = true;   // We might want to start another GC round
            break;
            
        default:
            fail("Unknown gc_step");
        }
    }
}

void data_block_manager_t::prepare_metablock(metablock_mixin_t *metablock) {
    
    assert(state == state_ready || state == state_shutting_down);
    
    for (int i = 0; i < MAX_ACTIVE_DATA_EXTENTS; i++) {
        if (active_extents[i]) {
            metablock->active_extents[i] = active_extents[i]->offset;
            metablock->blocks_in_active_extent[i] = blocks_in_active_extent[i];
        } else {
            metablock->active_extents[i] = NULL_OFFSET;
            metablock->blocks_in_active_extent[i] = 0;
        }
    }
}

bool data_block_manager_t::shutdown(shutdown_callback_t *cb) {
    assert(cb);
    assert(state == state_ready);
    state = state_shutting_down;

    if(gc_state.step() != gc_ready) {
        shutdown_callback = cb;
        return false;
    } else {
        shutdown_callback = NULL;
        actually_shutdown();
        return true;
    }
}

void data_block_manager_t::actually_shutdown() {
    
    assert(state == state_shutting_down);
    state = state_shut_down;
    
    assert(!reconstructed_extents.head());
    
    for (unsigned int i = 0; i < dynamic_config->num_active_data_extents; i++) {
        if (active_extents[i]) {
            delete active_extents[i];
            active_extents[i] = NULL;
        }
    }
    
    while (gc_entry *entry = young_extent_queue.head()) {
        young_extent_queue.remove(entry);
        delete entry;
    }
    
    while (!gc_pq.empty()) {
        delete gc_pq.pop();
    }
    
    if (shutdown_callback) shutdown_callback->on_datablock_manager_shutdown();
}

off64_t data_block_manager_t::gimme_a_new_offset() {
    
    /* Start a new extent if necessary */
    
    if (!active_extents[next_active_extent]) {
        
        active_extents[next_active_extent] = new gc_entry(this);
        active_extents[next_active_extent]->state = gc_entry::state_active;
        blocks_in_active_extent[next_active_extent] = 0;
        
        pm_serializer_data_extents_allocated++;
    }
    
    /* Put the block into the chosen extent */
    
    assert(active_extents[next_active_extent]->state == gc_entry::state_active);
    assert(active_extents[next_active_extent]->g_array.count() > 0);
    assert(blocks_in_active_extent[next_active_extent] < extent_manager->extent_size / static_config->block_size);
    
    off64_t offset = active_extents[next_active_extent]->offset + blocks_in_active_extent[next_active_extent] * static_config->block_size;
    
    assert(active_extents[next_active_extent]->g_array[blocks_in_active_extent[next_active_extent]]);
    active_extents[next_active_extent]->g_array.set(blocks_in_active_extent[next_active_extent], 0);
    
    blocks_in_active_extent[next_active_extent]++;
    
    /* Deactivate the extent if necessary */
    
    if (blocks_in_active_extent[next_active_extent] == extent_manager->extent_size / static_config->block_size) {
        
        assert(active_extents[next_active_extent]->g_array.count() < extent_manager->extent_size / static_config->block_size);
        active_extents[next_active_extent]->state = gc_entry::state_young;
        young_extent_queue.push_back(active_extents[next_active_extent]);
        mark_unyoung_entries();
        active_extents[next_active_extent] = NULL;
    }
    
    /* Move along to the next extent. This logic is kind of weird because it needs to handle the
    case where we have just started up and we still have active extents open from a previous run,
    but the value of num_active_data_extents was higher on that previous run and so there are active
    data extents that occupy slots in active_extents that are higher than our current value of
    num_active_data_extents. The way we handle this case is by continuing to visit those slots until
    the data extents fill up and are deactivated, but then not visiting those slots any more. */
    
    do {
        next_active_extent = (next_active_extent + 1) % MAX_ACTIVE_DATA_EXTENTS;
    } while (next_active_extent >= dynamic_config->num_active_data_extents &&
             !active_extents[next_active_extent]);
    
    return offset;
}

// Looks at young_extent_queue and pops things off the queue that are
// no longer deemed young, putting them on the priority queue.
void data_block_manager_t::mark_unyoung_entries() {

    while (young_extent_queue.size() > GC_YOUNG_EXTENT_MAX_SIZE) {
        remove_last_unyoung_entry();
    }

    gc_entry::timestamp_t current_time = gc_entry::current_timestamp();

    while (young_extent_queue.head()
           && current_time - young_extent_queue.head()->timestamp > GC_YOUNG_EXTENT_TIMELIMIT_MICROS) {
        remove_last_unyoung_entry();
    }
}

// Pops young_extent_queue and puts it on the priority queue.
// Assumes young_extent_queue is not empty.
void data_block_manager_t::remove_last_unyoung_entry() {

    gc_entry *entry = young_extent_queue.head();
    young_extent_queue.remove(entry);
    
    assert(entry->state == gc_entry::state_young);
    entry->state = gc_entry::state_old;
    
    entry->our_pq_entry = gc_pq.push(entry);
    
    gc_stats.old_total_blocks += extent_manager->extent_size / static_config->block_size;
    gc_stats.old_garbage_blocks += entry->g_array.count();
}


/* functions for gc structures */

// Answers the following question: We're in the middle of gc'ing, and
// look, it's the next largest entry.  Should we keep gc'ing?  Returns
// false when the entry is active or young, or when its garbage ratio
// is lower than GC_THRESHOLD_RATIO_*.
bool data_block_manager_t::should_we_keep_gcing(const gc_entry& entry) const {
    return !gc_state.should_be_stopped && garbage_ratio() > dynamic_config->gc_low_ratio;
}

// Answers the following question: Do we want to bother gc'ing?
// Returns true when our garbage_ratio is greater than
// GC_THRESHOLD_RATIO_*.
bool data_block_manager_t::do_we_want_to_start_gcing() const {
    return !gc_state.should_be_stopped && garbage_ratio() > dynamic_config->gc_high_ratio;
}

/* !< is x less than y */
bool data_block_manager_t::Less::operator() (const data_block_manager_t::gc_entry *x, const data_block_manager_t::gc_entry *y) {
    return x->g_array.count() < y->g_array.count();
}

/****************
 *Stat functions*
 ****************/

float data_block_manager_t::garbage_ratio() const {
    if (gc_stats.old_total_blocks.get() == 0) {
        return 0.0;
    } else {
        return (float) gc_stats.old_garbage_blocks.get() / (float) gc_stats.old_total_blocks.get();
    }
}

bool data_block_manager_t::disable_gc(gc_disable_callback_t *cb) {
    // We _always_ call the callback!

    assert(gc_state.gc_disable_callback == NULL);
    gc_state.should_be_stopped = true;

    if (gc_state.step() != gc_ready && gc_state.step() != gc_reconstruct) {
        gc_state.gc_disable_callback = cb;
        return false;
    } else {
        cb->on_gc_disabled();
        return true;
    }
}

void data_block_manager_t::enable_gc() {
    gc_state.should_be_stopped = false;
}
