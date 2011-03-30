#include "extent.hpp"

struct extent_block_t :
    public extent_t::sync_callback_t,
    public iocallback_t
{
    char *data;
    extent_t *parent;
    size_t offset;
    std::vector< extent_t::sync_callback_t* > sync_cbs;
    bool waiting_for_prev, have_finished_sync, is_last_block;
    
    extent_block_t(extent_t *parent, size_t offset)
        : parent(parent), offset(offset)
    {
        data = reinterpret_cast<char *>(malloc_aligned(DEVICE_BLOCK_SIZE, DEVICE_BLOCK_SIZE));
    }
    ~extent_block_t() {
        free(data);
    }
    
    void write() {
        waiting_for_prev = true;
        have_finished_sync = false;
        
        parent->sync(this);
        
        if (parent->last_block) parent->last_block->is_last_block = false;
        parent->last_block = this;
        is_last_block = true;
        
        parent->file->write_async(parent->offset + offset, DEVICE_BLOCK_SIZE, data, this);
    }
    
    void on_extent_sync() {
        rassert(waiting_for_prev);
        waiting_for_prev = false;
        if (have_finished_sync) done();
    }
    
    void on_io_complete() {
        rassert(!have_finished_sync);
        have_finished_sync = true;
        if (!waiting_for_prev) done();
    }
    
    void done() {
        for (unsigned i = 0; i < sync_cbs.size(); i++) {
            sync_cbs[i]->on_extent_sync();
        }
        if (is_last_block) {
            rassert(this == parent->last_block);
            parent->last_block = NULL;
        }
        delete this;
    }
};

perfmon_counter_t pm_serializer_lba_extents("serializer_lba_extents");

extent_t::extent_t(extent_manager_t *em, direct_file_t *file)
    : offset(em->gen_extent()), amount_filled(0), em(em), file(file), last_block(NULL), current_block(NULL)
{
    pm_serializer_lba_extents++;
}

extent_t::extent_t(extent_manager_t *em, direct_file_t *file, off64_t loc, size_t size)
    : offset(loc), amount_filled(size), em(em), file(file), last_block(NULL), current_block(NULL)
{
    em->reserve_extent(offset);
    rassert(amount_filled % DEVICE_BLOCK_SIZE == 0);
    pm_serializer_lba_extents++;
}

void extent_t::destroy() {
    em->release_extent(offset);
    shutdown();
}

void extent_t::shutdown() {
    delete this;
}

extent_t::~extent_t() {
    rassert(!current_block);
    if (last_block) last_block->is_last_block = false;
    pm_serializer_lba_extents--;
}

void extent_t::read(size_t pos, size_t length, void *buffer, read_callback_t *cb) {
    rassert(!last_block);
    file->read_async(offset + pos, length, buffer, cb);
}

void extent_t::append(void *buffer, size_t length) {
    rassert(amount_filled + length <= em->extent_size);
    
    while (length > 0) {
        size_t room_in_block;
        if (amount_filled % DEVICE_BLOCK_SIZE == 0) {
            rassert(!current_block);
            current_block = new extent_block_t(this, amount_filled);
            room_in_block = DEVICE_BLOCK_SIZE;
        } else {
            rassert(current_block);
            room_in_block = DEVICE_BLOCK_SIZE - amount_filled % DEVICE_BLOCK_SIZE;
        }
        
        size_t chunk = std::min(length, room_in_block);
        memcpy(current_block->data + (amount_filled % DEVICE_BLOCK_SIZE), buffer, chunk);
        amount_filled += chunk;
        
        if (amount_filled % DEVICE_BLOCK_SIZE == 0) {
            extent_block_t *b = current_block;
            current_block = NULL;
            b->write();
        }
        
        length -= chunk;
        buffer = reinterpret_cast<char *>(buffer) + chunk;
    }
}

void extent_t::sync(sync_callback_t *cb) {
    rassert(amount_filled % DEVICE_BLOCK_SIZE == 0);
    rassert(!current_block);
    if (last_block) {
        last_block->sync_cbs.push_back(cb);
    } else {
        cb->on_extent_sync();
    }
}

