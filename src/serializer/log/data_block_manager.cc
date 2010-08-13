#include "data_block_manager.hpp"

#include "cpu_context.hpp"
#include "event_queue.hpp"
#include "arch/io.hpp"

void data_block_manager_t::start(fd_t fd) {
    
    assert(state == state_unstarted);
    dbfd = fd;
    last_data_extent = extent_manager->gen_extent();
    blocks_in_last_data_extent = 0;
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
        last_data_extent = extent_manager->gen_extent();
        blocks_in_last_data_extent = 0;
    }
    off64_t offset = last_data_extent + blocks_in_last_data_extent * block_size;
    blocks_in_last_data_extent ++;
    return offset;
}