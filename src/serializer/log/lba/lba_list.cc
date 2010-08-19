#include "lba_list.hpp"

#include "cpu_context.hpp"
#include "event_queue.hpp"

// END_OF_LBA_CHAIN indicates the end of a linked list of LBA extents.
#define END_OF_LBA_CHAIN off64_t(-1)

// PADDING_BLOCK_ID and PADDING_OFFSET indicate that an entry in the LBA list only exists to fill
// out a DEVICE_BLOCK_SIZE-sized chunk of the extent.
#define PADDING_BLOCK_ID block_id_t(-1)
#define PADDING_OFFSET off64_t(-1)

// This special offset value indicates that a block is being deleted
#define DELETE_BLOCK off64_t(-1)

static const char lba_magic[LBA_MAGIC_SIZE] = {'l', 'b', 'a', 'm', 'a', 'g', 'i', 'c'};

lba_list_t::lba_list_t(extent_manager_t *em)
    : extent_manager(em), state(state_unstarted), last_write(NULL), next_free_id(NULL_BLOCK_ID)
    {}

lba_list_t::~lba_list_t() {
    assert(state == state_unstarted || state == state_shut_down);
    assert(last_write == NULL);
    assert(current_extent == NULL);
}

/* This form of start() is called when we are creating a new database */
void lba_list_t::start(fd_t fd) {
    
    assert(state == state_unstarted);
    
    dbfd = fd;
    
    current_extent = NULL;
    offset_of_last_extent = END_OF_LBA_CHAIN;
    entries_in_last_extent = 0;

    // Create an initial superblock (because set_block_offset() only accepts block IDs created by
    // gen_block_id(), so we have to manually help the serializer bypass gen_block_id() to get the
    // correct ID for the superblock)
    assert(SUPERBLOCK_ID == 0);
    blocks.set_size(1);
    blocks[SUPERBLOCK_ID].set_found(true);
    blocks[SUPERBLOCK_ID].set_state(block_in_limbo);
    
    state = state_ready;
}

struct lba_start_fsm_t :
    public iocallback_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, lba_start_fsm_t >
{
    typedef lba_list_t::lba_extent_t lba_extent_t;
    typedef lba_list_t::lba_entry_t lba_entry_t;
    typedef lba_list_t::block_info_t block_info_t;
    
    lba_start_fsm_t(lba_list_t *l)
        : owner(l), state(state_start) {
        // TODO: Allocation
        buffer = (lba_extent_t *)malloc_aligned(owner->extent_manager->extent_size, DEVICE_BLOCK_SIZE);
        assert(buffer);
    }
    
    ~lba_start_fsm_t() {
        assert(state == state_start || state == state_done);
        free(buffer);
    }
    
    bool run(lba_list_t::metablock_mixin_t *last_metablock, lba_list_t::ready_callback_t *cb) {
        
        assert(state == state_start);
        
        assert(owner->state == lba_list_t::state_unstarted);
        owner->state = lba_list_t::state_starting_up;
        
        /* The rest of the extent that we were writing to when we last shut down is considered
        taboo, because if we crashed during a write there might be some data on the end of it that
        we don't know is there. If we write as though nothing happened, we might force the SSD
        controller to copy the extent, leading to fragmentation. So instead of making it the
        current extent we make it the last extent. */
        owner->current_extent = NULL;
        owner->offset_of_last_extent = last_metablock->lba_chain_head;
        owner->entries_in_last_extent = last_metablock->entries_in_lba_chain_head;
        
        owner->blocks.set_size(last_metablock->highest_block_id_plus_one);
        for (block_id_t i = 0; i < owner->blocks.get_size(); i ++) {
            owner->blocks[i].set_found(false);
        }
        num_blocks_yet_to_be_found = owner->blocks.get_size();
        
        lba_chain_next = last_metablock->lba_chain_head;
        entries_in_lba_chain_next = last_metablock->entries_in_lba_chain_head;
        num_blocks_yet_to_be_found = last_metablock->highest_block_id_plus_one;
        
        state = state_load_link;
        callback = NULL;
        if (process_next_link_in_lba_chain()) {
            return true;
        } else {
            callback = cb;
            return false;
        }
    }
    
    bool process_next_link_in_lba_chain() {
        
        if (state == state_load_link) {
        
            if (num_blocks_yet_to_be_found == 0) {
                state = state_finish;
                
            } else {
                // Don't expect to reach end of LBA chain before all blocks are found
                assert(lba_chain_next != END_OF_LBA_CHAIN);
                
                event_queue_t *queue = get_cpu_context()->event_queue;
                queue->iosys.schedule_aio_read(
                    owner->dbfd,
                    lba_chain_next,
                    owner->extent_manager->extent_size,
                    buffer,
                    queue,
                    this);
                
                state = state_loading_link;
                return false;
            }
        }
        
        if (state == state_process_link) {
            
            // Make sure it's a real LBA extent
            assert(memcmp(buffer->header.magic, lba_magic, LBA_MAGIC_SIZE) == 0);
            
            // Record each entry, as long as we haven't already found a newer entry. We start from the
            // end of the extent and work backwards so that we always read the newest entries first.
            for (int i = entries_in_lba_chain_next - 1; i >= 0; i --) {
                
                lba_entry_t *entry = &buffer->entries[i];
                
                // Skip it if it's padding
                if (entry->block_id == PADDING_BLOCK_ID && entry->offset == PADDING_OFFSET)
                    continue;
                assert(entry->block_id != PADDING_BLOCK_ID);
                assert(entry->offset != PADDING_OFFSET);
                
                // Sanity check. If this assertion fails, it probably means that the file was
                // corrupted, or was created with a different btree block size.
                assert(entry->offset == DELETE_BLOCK || entry->offset % DEVICE_BLOCK_SIZE == 0);
                
                // If it's an entry for a block we haven't seen before, then record its information
                block_info_t *binfo = &owner->blocks[entry->block_id];
                if (!binfo->is_found()) {
                    binfo->set_found(true);
                    if (entry->offset == DELETE_BLOCK) {
                        binfo->set_state(lba_list_t::block_unused);
                    } else {
                        binfo->set_state(lba_list_t::block_used);
                        binfo->set_offset(entry->offset);
                    }
                    num_blocks_yet_to_be_found --;
                    assert(num_blocks_yet_to_be_found >= 0);
                }
            }
            
            lba_chain_next = buffer->header.next_extent_in_chain;
            entries_in_lba_chain_next = buffer->header.entries_in_next_extent_in_chain;
            state = state_load_link;
            return process_next_link_in_lba_chain();
        }
        
        if (state == state_finish) {
            state = state_done;

            assert(owner->state == lba_list_t::state_starting_up);
            owner->state = lba_list_t::state_ready;

            // Now that we're finished loading the lba blocks, go
            // through the ids and create a freelist of unused ids so
            // that we can recycle and reuse them (this is necessary
            // because the system depends on the id space being
            // dense). We're doing another pass here, and it sucks,
            // but will do for first release. TODO: fix the lba system
            // to avoid O(n) algorithms on startup.
            for (block_id_t id = 0; id < owner->blocks.get_size(); id ++) {
                if (owner->blocks[id].get_state() == lba_list_t::block_unused) {
                    // Add the unused block to the freelist
                    owner->blocks[id].set_next_free_id(owner->next_free_id);
                    owner->next_free_id = id;
                }
            }
            
            
            if (callback) callback->on_lba_ready();
            
            delete this;
            return true;
        }
        
        fail("Invalid state.");
    }
    
    void on_io_complete(event_t *e) {
        assert(state == state_loading_link);
        state = state_process_link;
        process_next_link_in_lba_chain();
    }
    
    lba_list_t *owner;
    
    enum state_t {
        state_start,
        state_load_link,
        state_loading_link,
        state_process_link,
        state_finish,
        state_done
    } state;
    
    lba_list_t::ready_callback_t *callback;
    
    lba_extent_t *buffer;
    int num_blocks_yet_to_be_found;
    off64_t lba_chain_next;
    int entries_in_lba_chain_next;
};

/* This form of start() is called when we are loading an existing database */
bool lba_list_t::start(fd_t fd, metablock_mixin_t *last_metablock, ready_callback_t *cb) {
    
    assert(state == state_unstarted);
    
    dbfd = fd;
    
    lba_start_fsm_t *starter = new lba_start_fsm_t(this);
    return starter->run(last_metablock, cb);
}

block_id_t lba_list_t::gen_block_id() {
    
    assert(state == state_ready);
    
    if(next_free_id == NULL_BLOCK_ID) {
        // There is no recyclable block id
        block_id_t id = blocks.get_size();
        blocks.set_size(blocks.get_size() + 1);
        blocks[id].set_found(true);
        blocks[id].set_state(block_in_limbo);
        return id;
    } else {
        // Grab a recyclable id from the freelist
        block_id_t id = next_free_id;
        assert(blocks[id].get_state() == block_unused);
        next_free_id = blocks[id].get_next_free_id();
        blocks[id].set_state(block_in_limbo);
        return id;
    }
}

off64_t lba_list_t::get_block_offset(block_id_t block) {
    
    assert(state == state_ready);
    
    assert(blocks[block].get_state() == block_used);
    return blocks[block].get_offset();
}

/* We allocate temporary storage for the LBA blocks we are writing in the form of lba_extent_buf_t.
Each lba_extent_buf_t represents an LBA extent on disk, and holds a buffer for the extent in
memory. The extent we are currently writing to lives in the lba_list_t's 'current_extent' field.
When we fill up an lba_extent_buf_t, we remove it from 'current_extent', but the lba_extent_buf_t
is responsible for deallocating itself only after all of the writes in the IO queue that refer to it
have completed. */

struct lba_extent_buf_t :
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, lba_extent_buf_t >
{

public:
    lba_extent_buf_t(lba_list_t *owner, off64_t offset_of_last_extent, int entries_in_last_extent)
        : owner(owner), num_entries(0), amount_synced(0), writes_out(0) {
        
        offset = owner->extent_manager->gen_extent();
        
        data = (lba_list_t::lba_extent_t *)malloc_aligned(owner->extent_manager->extent_size, DEVICE_BLOCK_SIZE);
        assert(data);

#ifndef NDEBUG
        memset(data, 0xBD, owner->extent_manager->extent_size);   // Make Valgrind happy
#endif
        
        // Make sure that the size of the header is a multiple of the size of one entry, so that the
        // header doesn't prevent the entries from aligning with DEVICE_BLOCK_SIZE.
        assert(offsetof(lba_list_t::lba_extent_t, entries[0]) % sizeof(lba_list_t::lba_entry_t) == 0);
        
        memcpy(data->header.magic, lba_magic, LBA_MAGIC_SIZE);
        data->header.next_extent_in_chain = owner->offset_of_last_extent;
        data->header.entries_in_next_extent_in_chain = owner->entries_in_last_extent;
    }
    
    ~lba_extent_buf_t() {
        assert(writes_out == 0);
        free(data);
    }
    
    size_t amount_filled() {
        
        return offsetof(lba_list_t::lba_extent_t, entries[0]) +
            sizeof(lba_list_t::lba_entry_t)*num_entries;
    }
    
    void add_entry(block_id_t id, off64_t offset) {
        
        // Make sure that entries will align with DEVICE_BLOCK_SIZE
        assert(DEVICE_BLOCK_SIZE % sizeof(lba_list_t::lba_entry_t) == 0);
        
        // Make sure that there is room
        assert(amount_filled() + sizeof(lba_list_t::lba_entry_t) <= owner->extent_manager->extent_size);
        
        data->entries[num_entries].block_id = id;
        data->entries[num_entries].offset = offset;
        num_entries ++;
    }
    
    // Declared later due to circular dependency with lba_write_t
    void sync();
    
    void on_write_complete() {
        assert(writes_out > 0);
        writes_out --;
        
        if (writes_out == 0 && amount_synced == owner->extent_manager->extent_size)
            delete this;
    }
    
    lba_list_t::lba_extent_t *data;
    
    lba_list_t *owner;   // The LBA list that this extent is a part of
    off64_t offset;   // The offset into the file of the start of the extent
    int num_entries;   // How many entries are currently in the extent in memory
    size_t amount_synced;   // How many bytes of the extent have been scheduled to be written
    int writes_out;   // How many lba_write_ts exist that point to this buf
};

/* When writing offsets to a file, we do not want to confirm to the user that the sync is complete
until not only the write that they initiated is complete, but all prior writes are complete. This is
handled by lba_write_t, which is in effect a linked list of writes. The lba_write_t's done() method
will only run when the write and all prior writes are complete. */

struct lba_write_t :
    public iocallback_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, lba_write_t >
{

    lba_write_t(lba_list_t *owner, lba_extent_buf_t *buf)
        : owner(owner), buf(buf), next_write(NULL)
    {
        waiting_for_this_write = true;
        if (owner->last_write) {
            owner->last_write->next_write = this;
            waiting_for_last_write = true;
        } else {
            owner->last_write = this;
            waiting_for_last_write = false;
        }
    }
    
    ~lba_write_t() {
        assert(!waiting_for_this_write);
        assert(!waiting_for_last_write);
    }
    
    void on_io_complete(event_t *e) {
        assert(waiting_for_this_write);
        waiting_for_this_write = false;
        if (buf) buf->on_write_complete();
        if (!waiting_for_last_write) done();
    }
    
    void on_last_write_complete() {
        assert(waiting_for_last_write);
        waiting_for_last_write = false;
        if (!waiting_for_this_write) done();
    }
    
    void done() {
        while (lba_list_t::sync_callback_t *cb = callbacks.head()) {
            callbacks.remove(cb);
            cb->on_lba_sync();
        }
        if (next_write) next_write->on_last_write_complete();
        if (owner->last_write == this) owner->last_write = NULL;
        delete this;
    }
        
    bool waiting_for_last_write;   // true unless the entire LBA up to this point is safely on disk
    bool waiting_for_this_write;   // true unless our chunk of the LBA is safely on disk
    lba_list_t *owner;
    
    // We notify 'buf', 'callbacks', and/or 'next_write' when we are done
    lba_extent_buf_t *buf;
    intrusive_list_t<lba_list_t::sync_callback_t> callbacks;
    lba_write_t *next_write;
};

// Declared here due to circular dependency with lba_write_t
void lba_extent_buf_t::sync() {
    
    if (amount_synced < amount_filled()) {
    
        // The lba_write_t will register itself with the lba_list_t
        lba_write_t *w = new lba_write_t(owner, this);
        
        event_queue_t *queue = get_cpu_context()->event_queue;
        queue->iosys.schedule_aio_write(
            owner->dbfd,
            offset + amount_synced,            // Offset write begins at
            amount_filled() - amount_synced,   // Length of write
            (byte*)data + amount_synced,
            queue,
            w);
        
        amount_synced = amount_filled();
        writes_out ++;
    }
}

void lba_list_t::make_entry_in_extent(block_id_t block, off64_t offset) {
    
    if (!current_extent) {
    
        current_extent = new lba_extent_buf_t(this,
            offset_of_last_extent,
            entries_in_last_extent);
    }

    current_extent->add_entry(block, offset);
        
    if (current_extent->amount_filled() == extent_manager->extent_size) {
        
        // Sync proactively so we doesn't have to keep track of which lba_extent_buf_ts are unsynced
        current_extent->sync();
        
        offset_of_last_extent = current_extent->offset;
        entries_in_last_extent = current_extent->num_entries;
        current_extent = NULL;
    }
}

void lba_list_t::set_block_offset(block_id_t block, off64_t offset) {
        
    assert(offset != DELETE_BLOCK);   // Watch out for collisions with the magic value
    
    // Blocks must be returned by gen_block_id() before they are legal to use
    assert(blocks[block].get_state() != block_unused);
    
    blocks[block].set_state(block_used);
    blocks[block].set_offset(offset);
    
    make_entry_in_extent(block, offset);
}

void lba_list_t::delete_block(block_id_t block) {
    
    assert(blocks[block].get_state() != block_unused);
    
    if (blocks[block].get_state() == block_in_limbo) {
        // Block ID allocated, but deleted before first write. We don't need to go to disk.
        blocks[block].set_state(block_unused);
    } else {
        blocks[block].set_state(block_unused);
        make_entry_in_extent(block, DELETE_BLOCK);
    }

    // Add the unused block to the freelist
    blocks[block].set_next_free_id(next_free_id);
    next_free_id = block;
}

bool lba_list_t::sync(sync_callback_t *cb) {
    
    assert(state == state_ready);
    
    if (current_extent) {
        
        // This is the most common case. We have appended entries to an existing extent but not
        // filled it, or we filled one or more extents and then started a new one. If we filled more
        // than one extent and are now syncing the final one, that is okay, because the lba_write_t
        // will not call us back until the lba_write_ts from the previous extents are also done
        // writing.
        
        while (current_extent->amount_filled() % DEVICE_BLOCK_SIZE != 0) {
            current_extent->add_entry(PADDING_BLOCK_ID, PADDING_OFFSET);
        }
        
        current_extent->sync();
        
        // If our padding filled the remaining empty space in the extent, flush it
        if (current_extent->amount_filled() == extent_manager->extent_size) {
            offset_of_last_extent = current_extent->offset;
            entries_in_last_extent = current_extent->num_entries;
            current_extent = NULL;
        }
    }
    
    if (last_write) {
        // Attach our callback to the write that is in progress
        if (cb) last_write->callbacks.push_back(cb);
        return false;
    } else {
        return true;
    }
}

void lba_list_t::prepare_metablock(metablock_mixin_t *metablock) {
    
    if (current_extent) {
        metablock->lba_chain_head = current_extent->offset;
        metablock->entries_in_lba_chain_head = current_extent->num_entries;
    } else {
        metablock->lba_chain_head = offset_of_last_extent;
        metablock->entries_in_lba_chain_head = entries_in_last_extent;
    }
    metablock->highest_block_id_plus_one = blocks.get_size();
}

void lba_list_t::shutdown() {
    assert(state == state_ready);    
    state = state_shut_down;
    
    assert(!last_write);
    if (current_extent) {
        delete current_extent;
        current_extent = NULL;
    }
}
