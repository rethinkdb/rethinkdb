#include "data_block_manager.hpp"
#include "log_serializer.hpp"
#include "utils.hpp"

#include "cpu_context.hpp"
#include "event_queue.hpp"
#include "arch/io.hpp"

void data_block_manager_t::start(fd_t fd) {
    
    assert(state == state_unstarted);
    dbfd = fd;
    last_data_extent = extent_manager->gen_extent();
    blocks_in_last_data_extent = 0;

    add_gc_entry();

    state = state_ready;
}

void data_block_manager_t::start(fd_t fd, metablock_mixin_t *last_metablock) {

    assert(state == state_unstarted);
    dbfd = fd;
    last_data_extent = last_metablock->last_data_extent;
    blocks_in_last_data_extent = last_metablock->blocks_in_last_data_extent;
    state = state_ready;
}

bool data_block_manager_t::read(off64_t off_in, void *buf_out, iocallback_t *cb) {

    assert(state == state_ready);
    
    event_queue_t *queue = get_cpu_context()->event_queue;
    queue->iosys.schedule_aio_read(dbfd, off_in, block_size, buf_out, queue, cb);
    
    return false;
}

bool data_block_manager_t::write(void *buf_in, off64_t *off_out, iocallback_t *cb) {
    assert(state == state_ready);
    
    off64_t offset = *off_out = gimme_a_new_offset();
    
    event_queue_t *queue = get_cpu_context()->event_queue;
    queue->iosys.schedule_aio_write(dbfd, offset, block_size, buf_in, queue, cb);
    
    return false;
}

void data_block_manager_t::mark_garbage(off64_t offset) {
    unsigned int extent_id = (offset / extent_manager->extent_size);
    unsigned int block_id = (offset % extent_manager->extent_size) / block_size;

    if ((gc_state.step == gc_read || gc_state.step == gc_write) && gc_state.current_entry.offset / extent_manager->extent_size == extent_id) {
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
    for (unsigned int extent_id = 0; (extent_id * extent_manager->extent_size) < (unsigned int) extent_manager->max_extent(); extent_id++) {
        if (entries.get(extent_id) == NULL)
            extent_manager->release_extent(extent_id * extent_manager->extent_size);
    }
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
                if (gc_criterion()(gc_pq.peak())) {
                    /* grab the entry */
                    gc_state.current_entry = gc_pq.pop();

                    delete entries.get(gc_state.current_entry.offset / EXTENT_SIZE);
                    entries.set(gc_state.current_entry.offset / EXTENT_SIZE, NULL);

                    /* read all the live data into buffers */
                    event_queue_t *queue = get_cpu_context()->event_queue;

                    /* make sure the read callback knows who we are */
                    gc_state.gc_read_callback.parent = this;

                    for (unsigned int i = 0; i < extent_manager->extent_size / BTREE_BLOCK_SIZE; i++) {
                        if (!gc_state.current_entry.g_array[i]) {
                            queue->iosys.schedule_aio_read(dbfd, gc_state.current_entry.offset + (i * block_size), block_size, gc_state.gc_blocks + (i * block_size), queue, &(gc_state.gc_read_callback));
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
                                writes[nwrites].block_id = *((block_id_t *) (gc_state.gc_blocks + (i * block_size)));
                                writes[nwrites].buf = gc_state.gc_blocks + (i * block_size);
                                writes[nwrites].callback = NULL;
                                nwrites++;
                            }
                        }
                        /* make sure the callback knows who we are */
                        gc_state.gc_write_callback.parent = this;
                        /* schedule the write */
                        serializer->do_write(writes, gc_state.current_entry.g_array.size() - gc_state.current_entry.g_array.count() , (log_serializer_t::write_txn_callback_t *) &gc_state.gc_write_callback);
                    } else {
                        fallthrough = true;
                    }

                    gc_state.step = gc_write;
                }
                if (!fallthrough)
                    break;
            case gc_write:
                //not valid when writes are asynchronous (it would be nice if we could have this)
                assert(gc_state.current_entry.g_array.count() == gc_state.current_entry.g_array.size());
                assert(entries.get(gc_state.current_entry.offset / extent_manager->extent_size) == NULL);

                extent_manager->release_extent(gc_state.current_entry.offset);
                assert(gc_state.refcount == 0);
                gc_state.blocks_copying = 0;


                gc_state.step = gc_ready;

                /* update stats */
                gc_stats.total_blocks -= EXTENT_SIZE / BTREE_BLOCK_SIZE;
                gc_stats.garbage_blocks -= EXTENT_SIZE / BTREE_BLOCK_SIZE;
                break;
            default:
                fail("Unknown gc_step");
                break;
        }
    } while (gc_state.step == gc_ready); 
}

void data_block_manager_t::prepare_metablock(metablock_mixin_t *metablock) {
    
    assert(state == state_ready);
    
    metablock->last_data_extent = last_data_extent;
    metablock->blocks_in_last_data_extent = blocks_in_last_data_extent;
}

void data_block_manager_t::shutdown() {
    assert(state == state_ready);
    state = state_shut_down;
}

off64_t data_block_manager_t::gimme_a_new_offset() {

    if (blocks_in_last_data_extent == extent_manager->extent_size / block_size) {
        /* deactivate the last extent */
        entries.get(last_data_extent / EXTENT_SIZE)->data.active = false;
        entries.get(last_data_extent / EXTENT_SIZE)->update();

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
    gc_stats.total_blocks += EXTENT_SIZE / BTREE_BLOCK_SIZE;
}

/* functions for gc structures */

bool data_block_manager_t::gc_criterion::operator() (const gc_entry entry) {
    return entry.g_array.count() >= ((EXTENT_SIZE / BTREE_BLOCK_SIZE) * 3) / 4 && !entry.active; // 3/4 garbage
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
    return (float) gc_stats.garbage_blocks / (float) gc_stats.total_blocks;
}
