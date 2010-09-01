
#include <vector>
#include "utils.hpp"
#include "lba_list.hpp"
#include "cpu_context.hpp"
#include "event_queue.hpp"

// TODO: refactor this monstrosity of a file. Possible rewrite the
// whole goddamn thing all together. (Using a non O(N) algorithm on
// startup would also help - we should be using a tree dagnabit).

/*****************************
 * Some serializer constants *
 *****************************/
#define OFFSET_UNINITIALIZED off64_t(-1)
// PADDING_BLOCK_ID and PADDING_OFFSET indicate that an entry in the LBA list only exists to fill
// out a DEVICE_BLOCK_SIZE-sized chunk of the extent.
#define PADDING_BLOCK_ID block_id_t(-1)
#define PADDING_OFFSET off64_t(-1)

// This special offset value indicates that a block is being deleted
#define DELETE_BLOCK off64_t(-1)

static const char lba_magic[LBA_MAGIC_SIZE] = {'l', 'b', 'a', 'm', 'a', 'g', 'i', 'c'};
static const char lba_super_magic[LBA_SUPER_MAGIC_SIZE] = {'l', 'b', 'a', 's', 'u', 'p', 'e', 'r'};

/**********************
 * SERIALIZATION CODE *
 **********************/

/* lba_buf_t represents a structure that keeps in memory information
 * about the extent to be serialized on disk. It is a base class for
 * lba_extent_buf_t (that represents lba extents), and
 * lba_superblock_buf_t (that represents lba superblock). */
struct lba_buf_t
{
public:
    lba_buf_t(lba_list_t *_owner)
        : owner(_owner), amount_synced(0), writes_out(0), data(NULL)
        {
            // Get us some memory
            data = malloc_aligned(owner->extent_manager->extent_size, DEVICE_BLOCK_SIZE);
            assert(data);
#ifndef NDEBUG
            memset(data, 0xBD, owner->extent_manager->extent_size);   // Make Valgrind happy
#endif
        }

    virtual ~lba_buf_t() {
        assert(writes_out == 0);
        free(data);
    }

    virtual void on_write_complete() = 0;
    
public:
    lba_list_t *owner;      // The LBA list that this extent is a part of
    size_t amount_synced;   // How many bytes of the extent have been scheduled to be written
    int writes_out;         // How many lba_write_ts exist that point to this buf
    void *data;             // Pointer to a chunk of memory to be written to the file
};

/* We allocate temporary storage for the LBA blocks we are writing in the form of lba_extent_buf_t.
Each lba_extent_buf_t represents an LBA extent on disk, and holds a buffer for the extent in
memory. The extent we are currently writing to lives in the lba_list_t's 'current_extent' field.
When we fill up an lba_extent_buf_t, we remove it from 'current_extent', but the lba_extent_buf_t
is responsible for deallocating itself only after all of the writes in the IO queue that refer to it
have completed. */
struct lba_extent_buf_t : public lba_buf_t,
                          public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, lba_extent_buf_t >
{
public:
    typedef lba_list_t::lba_extent_t extent_t;
    
public:
    lba_extent_buf_t(lba_list_t *owner)
        : lba_buf_t(owner), offset(OFFSET_UNINITIALIZED), num_entries(0)
        {
            // Generate ourselves an extent
            offset = owner->extent_manager->gen_extent();

            // Make sure that the size of the header is a multiple of the size of one entry, so that the
            // header doesn't prevent the entries from aligning with DEVICE_BLOCK_SIZE.
            assert(offsetof(extent_t, entries[0]) % sizeof(lba_list_t::lba_entry_t) == 0);
            memcpy(reinterpret_cast<extent_t*>(data)->magic, lba_magic, LBA_MAGIC_SIZE);
        }

    virtual ~lba_extent_buf_t() {}
    
    size_t amount_filled() {
        return offsetof(extent_t, entries[0]) +
            sizeof(lba_list_t::lba_entry_t)*num_entries;
    }
    
    void add_entry(block_id_t id, off64_t offset) {
        
        // Make sure that entries will align with DEVICE_BLOCK_SIZE
        assert(DEVICE_BLOCK_SIZE % sizeof(lba_list_t::lba_entry_t) == 0);
        
        // Make sure that there is room
        assert(amount_filled() + sizeof(lba_list_t::lba_entry_t) <= owner->extent_manager->extent_size);
        
        reinterpret_cast<extent_t*>(data)->entries[num_entries].block_id = id;
        reinterpret_cast<extent_t*>(data)->entries[num_entries].offset = offset;
        num_entries++;
    }
    
    // Declared later due to circular dependency with lba_write_t
    void sync();
    
    virtual void on_write_complete() {
        assert(writes_out > 0);
        writes_out--;
        
        if (writes_out == 0 && amount_synced == owner->extent_manager->extent_size)
            delete this;
    }

public:
    off64_t offset;         // The offset into the file of the start of the extent
    int num_entries;        // How many entries are currently in the extent in memory
};

/* This is the in-memory structure that keeps track of the lba superblock. */
struct lba_superblock_buf_t : public lba_buf_t
{
public:
    typedef lba_list_t::lba_superblock_t extent_t;
    typedef lba_list_t::lba_superblock_entry_t entry_t;
    
public:
    lba_superblock_buf_t(lba_list_t *owner) : lba_buf_t(owner), f_initialized(false), superblock_extent_offset(-1)
        {
            // Make sure that the size of the header is a multiple of the size of one entry, so that the
            // header doesn't prevent the entries from aligning with DEVICE_BLOCK_SIZE.
            assert(offsetof(extent_t, entries[0]) % sizeof(entry_t) == 0);
            memcpy(reinterpret_cast<extent_t*>(data)->magic, lba_super_magic, LBA_SUPER_MAGIC_SIZE);
        }
    
    size_t amount_filled() {
        return offsetof(extent_t, entries[0]) + sizeof(entry_t) * owner->lba_superblock_entry_count;
    }
    
    void add_entry(off64_t offset, int nentries) {
        // Make sure that entries will align with DEVICE_BLOCK_SIZE
        assert(DEVICE_BLOCK_SIZE % sizeof(entry_t) == 0);
        
        // Make sure that there is room
        assert(amount_filled() + sizeof(entry_t) <= owner->extent_manager->extent_size);

        int num_entries = owner->lba_superblock_entry_count;
        reinterpret_cast<extent_t*>(data)->entries[num_entries].offset = offset;
        reinterpret_cast<extent_t*>(data)->entries[num_entries].lba_entries_count = nentries;
        owner->lba_superblock_entry_count++;
    }
    
    // Declared later due to circular dependency with lba_write_t
    void sync();
    
    virtual void on_write_complete() {
        assert(writes_out > 0);
        writes_out--;
    }

private:
    bool f_initialized;
    off64_t superblock_extent_offset;
};
    
/* When writing offsets to a file, we do not want to confirm to the user that the sync is complete
until not only the write that they initiated is complete, but all prior writes are complete. This is
handled by lba_write_t, which is in effect a linked list of writes. The lba_write_t's done() method
will only run when the write and all prior writes are complete. */
struct lba_write_t :
    public iocallback_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, lba_write_t >
{

    lba_write_t(lba_list_t *owner, lba_buf_t *buf)
        : owner(owner), buf(buf), next_write(NULL)
    {
        // The idea here is that every lba_write_t's next_write points
        // to the lba_write_t that's chronologically started after it.
        waiting_for_this_write = true;
        if (owner->last_write) {
            assert(!owner->last_write->next_write);
            owner->last_write->next_write = this;
            waiting_for_last_write = true;
        } else {
            waiting_for_last_write = false;
        }
        owner->last_write = this;
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
    lba_buf_t *buf;
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
        writes_out++;
    }
}

void lba_superblock_buf_t::sync() {
    assert(owner->lba_superblock_entry_count >= 0);
    if(owner->lba_superblock_entry_count == 0)
        return;
    
    lba_write_t *w = new lba_write_t(owner, this);
    event_queue_t *queue = get_cpu_context()->event_queue;
    
    size_t write_count = ceil_aligned(amount_filled(), DEVICE_BLOCK_SIZE);
    
    if(!f_initialized || write_count > owner->extent_manager->extent_size - amount_synced) {
        // We need to generate a new extent
        superblock_extent_offset = owner->extent_manager->gen_extent();
        amount_synced = 0;
        //printf("Getting new extent\n");
    }
    
    //printf("Syncing superblock (%ld) @ %ld\n", write_count, owner->lba_superblock_offset / DEVICE_BLOCK_SIZE);
    queue->iosys.schedule_aio_write(
        owner->dbfd,
        superblock_extent_offset + amount_synced,                // Offset write begins at
        write_count,                                             // Length of write
        (byte*)data,
        queue,
        w);
    
    owner->lba_superblock_offset = superblock_extent_offset + amount_synced;
    amount_synced += write_count;
    writes_out++;
    f_initialized = true;
}

void lba_list_t::make_entry_in_extent(block_id_t block, off64_t offset) {
    
    if (!current_extent) {
        current_extent = new lba_extent_buf_t(this);
    }

    current_extent->add_entry(block, offset);
        
    if (current_extent->amount_filled() == extent_manager->extent_size) {
        // Sync proactively so we don't have to keep track of which lba_extent_buf_ts are unsynced
        current_extent->sync();
        finalize_current_extent();
    }
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
        
        // If our padding filled the remaining empty space in the
        // extent, we're done with it, time to set current_extent to
        // NULL, so we create a new extent object when we need more
        // entries
        if (current_extent->amount_filled() == extent_manager->extent_size) {
            finalize_current_extent();
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
    // TODO: assert that all outstanding LBA disk write requests have
    // completed by this point.
    metablock->lba_superblock_offset = lba_superblock_offset;
    metablock->lba_superblock_entries_count = lba_superblock_entry_count;
    if (current_extent) {
        metablock->last_lba_extent_offset = current_extent->offset;
        metablock->last_lba_extent_entries_count = current_extent->num_entries;
    } else {
        // There is no current extent, which means the entry about the
        // last extent is stored in the lba superblock.
        metablock->last_lba_extent_offset = OFFSET_UNINITIALIZED;
        metablock->last_lba_extent_entries_count = 0;
    }
}

/******************
 * INITIALIZATION *
 ******************/
lba_list_t::lba_list_t(extent_manager_t *em)
    : extent_manager(em), state(state_unstarted), last_write(NULL), next_free_id(NULL_BLOCK_ID)
    {}

struct lba_start_fsm_t :
    public iocallback_t,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, lba_start_fsm_t >
{
    typedef lba_list_t::lba_superblock_t lba_superblock_t;
    typedef lba_list_t::lba_extent_t lba_extent_t;
    typedef lba_list_t::lba_entry_t lba_entry_t;
    typedef lba_list_t::block_info_t block_info_t;
    
    lba_start_fsm_t(lba_list_t *l)
        : owner(l), state(state_start), nextentsloaded(0), nextentsrequested(0)
        {}
    
    ~lba_start_fsm_t() {
        assert(state == state_start || state == state_done);
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
        owner->lba_superblock_offset = last_metablock->lba_superblock_offset;
        owner->lba_superblock_entry_count = last_metablock->lba_superblock_entries_count;
        owner->last_extent_offset = last_metablock->last_lba_extent_offset;
        owner->last_extent_entry_count = last_metablock->last_lba_extent_entries_count;
        
        state = state_load_superblock;
        callback = NULL;
        if(load_lba_fsm()) {
            return true;
        } else {
            callback = cb;
            return false;
        }
    }
    
    bool load_lba_fsm() {
        lba_superblock_t *superblock_buffer = (lba_superblock_t*)owner->superblock_extent->data;
            
        if (state == state_load_superblock) {
            // Yay, we're loading the superblock that points to
            // extents with all the LBA goodness
            if (owner->lba_superblock_offset == OFFSET_UNINITIALIZED) {
                // There is no superblock in the db yet, we only need
                // to take care of the metablock
                assert(owner->lba_superblock_entry_count == 0);
                state = state_load_extents;
            } else {
                // Load the LBA superblock
                event_queue_t *queue = get_cpu_context()->event_queue;
                size_t size_to_read = ceil_aligned(lba_list_t::lba_superblock_t::entry_count_to_file_size(
                                                       owner->lba_superblock_entry_count),
                                                   DEVICE_BLOCK_SIZE);
                queue->iosys.schedule_aio_read(
                    owner->dbfd,
                    owner->lba_superblock_offset,
                    size_to_read,
                    superblock_buffer,
                    queue,
                    this);
                
                state = state_loading_superblock;
                return false;
            }
        }
        
        if (state == state_load_extents) {
            // First, verify that the superblock we loaded is a real
            // superblock
            assert(memcmp(superblock_buffer->magic, lba_super_magic, LBA_SUPER_MAGIC_SIZE) == 0);
            
            event_queue_t *queue = get_cpu_context()->event_queue;
            
            // Load the LBA extents in the superblock, if any, as well
            // as the last LBA extent from the metablock if it exists.
            lba_extent_blocks.reserve(owner->lba_superblock_entry_count + 1);
            for(int i = 0; i < owner->lba_superblock_entry_count; i++) {
                if(superblock_buffer->entries[i].offset != PADDING_OFFSET) {
                    lba_extent_t *extent = (lba_extent_t*)malloc_aligned(owner->extent_manager->extent_size, DEVICE_BLOCK_SIZE);
                    lba_extent_blocks.push_back(lba_extent_info_t(extent, superblock_buffer->entries[i].lba_entries_count));
                    queue->iosys.schedule_aio_read(
                        owner->dbfd,
                        superblock_buffer->entries[i].offset,
                        owner->extent_manager->extent_size,
                        lba_extent_blocks[i].first,
                        queue,
                        this);
                    nextentsrequested++;
                }
            }
            
            // If there is last extent in the metablock, load that too
            if(owner->last_extent_offset != OFFSET_UNINITIALIZED) {
                lba_extent_t *last_extent = (lba_extent_t*)malloc_aligned(owner->extent_manager->extent_size, DEVICE_BLOCK_SIZE);
                lba_extent_blocks.push_back(lba_extent_info_t(last_extent, owner->last_extent_entry_count));
                queue->iosys.schedule_aio_read(
                    owner->dbfd,
                    owner->last_extent_offset,
                    owner->extent_manager->extent_size,
                    last_extent,
                    queue,
                    this);
                nextentsrequested++;
            }

            if(nextentsrequested > 0) {
                // Ok, waiting for the extents to load now...
                state = state_loading_extents;
                return false;
            } else {
                // Ain't nothing to load, G!
                state = state_finish;
            }
        }

        if(state == state_process_extents) {
            // Ok, we've loaded all our LBA extents, process them
            // now. We have to do it back to front as the last entries
            // are at the end.
            for(int j = nextentsloaded - 1; j >=0; j--) {
                lba_extent_info_t extent_info = lba_extent_blocks[j];
                lba_extent_t* buffer = extent_info.first;
                int entries_in_lba_extent = extent_info.second;

                // Make sure it's a real LBA extent
                assert(memcmp(buffer->magic, lba_magic, LBA_MAGIC_SIZE) == 0);
            
                // Record each entry, as long as we haven't already found a newer entry. We start from the
                // end of the extent and work backwards so that we always read the newest entries first.
                for (int i = entries_in_lba_extent - 1; i >= 0; i --) {
                
                    lba_entry_t *entry = &buffer->entries[i];
                
                    // Skip it if it's padding
                    if (entry->block_id == PADDING_BLOCK_ID && entry->offset == PADDING_OFFSET)
                        continue;
                    assert(entry->block_id != PADDING_BLOCK_ID);
                
                    // Sanity check. If this assertion fails, it probably means that the file was
                    // corrupted, or was created with a different btree block size.
                    assert(entry->offset == DELETE_BLOCK || entry->offset % DEVICE_BLOCK_SIZE == 0);
                
                    // If it's an entry for a block we haven't seen before, then record its information
                    if(entry->block_id >= owner->blocks.get_size())
                        owner->blocks.set_size(entry->block_id + 1);
                
                    block_info_t *binfo = &owner->blocks[entry->block_id];
                    if (binfo->get_state() == lba_list_t::block_not_found) {
                        if (entry->offset == DELETE_BLOCK) {
                            binfo->set_state(lba_list_t::block_unused);
                        } else {
                            binfo->set_state(lba_list_t::block_used);
                            binfo->set_offset(entry->offset);
                        }
                    }
                }
            
                // We're done with this extent buffer
                free(buffer);
            }

            state = state_finish;
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
        assert(state == state_loading_superblock ||
               state == state_loading_extents);

        if(state == state_loading_superblock) {
            // Got an event on the superblock
            check("Error loading superblock", e->result < 0);
            state = state_load_extents;
            load_lba_fsm();
            return;
        } else if(state == state_loading_extents) {
            // Got an event on an extent
            check("Error loading extent", e->result < 0);
            nextentsloaded++;
            if(nextentsloaded == nextentsrequested) {
                state = state_process_extents;
                load_lba_fsm();
            }
            return;
        }
        fail("Invalid state.");
    }
    
    lba_list_t *owner;
    
    enum state_t {
        state_start,
        state_load_superblock,
        state_loading_superblock,
        state_load_extents,
        state_loading_extents,
        state_process_extents,
        state_finish,
        state_done
    } state;
    
    lba_list_t::ready_callback_t *callback;
    
    typedef std::pair<lba_extent_t*, int> lba_extent_info_t;
    std::vector<lba_extent_info_t, gnew_alloc<lba_extent_info_t> > lba_extent_blocks;

private:
    int nextentsloaded, nextentsrequested;    
};

/* This form of start() is called when we are creating a new database */
void lba_list_t::start(fd_t fd) {
    
    assert(state == state_unstarted);
    
    dbfd = fd;
    
    current_extent = NULL;
    last_extent_offset = OFFSET_UNINITIALIZED;
    last_extent_entry_count = 0;
    lba_superblock_offset = OFFSET_UNINITIALIZED;
    lba_superblock_entry_count = 0;

    // Create an initial superblock (because set_block_offset() only accepts block IDs created by
    // gen_block_id(), so we have to manually help the serializer bypass gen_block_id() to get the
    // correct ID for the superblock)
    assert(SUPERBLOCK_ID == 0);
    blocks.set_size(1);
    blocks[SUPERBLOCK_ID].set_state(block_in_limbo);
    
    state = state_ready;

    superblock_extent = gnew<lba_superblock_buf_t>(this);
}

/* This form of start() is called when we are loading an existing database */
bool lba_list_t::start(fd_t fd, metablock_mixin_t *last_metablock, ready_callback_t *cb) {
    
    assert(state == state_unstarted);
    
    dbfd = fd;
    
    superblock_extent = gnew<lba_superblock_buf_t>(this);
    lba_start_fsm_t *starter = new lba_start_fsm_t(this);
    return starter->run(last_metablock, cb);
}

void lba_list_t::finalize_current_extent() {
    // Add current extent to the superblock
    superblock_extent->add_entry(current_extent->offset, current_extent->num_entries);
    superblock_extent->sync();

    // Clear it out
    last_extent_offset = current_extent->offset;
    last_extent_entry_count = current_extent->num_entries;
    current_extent = NULL;
}

/************************
 * LBA MANIPULATION OPS *
 ************************/
block_id_t lba_list_t::gen_block_id() {
    
    assert(state == state_ready);
    
    if(next_free_id == NULL_BLOCK_ID) {
        // There is no recyclable block id
        block_id_t id = blocks.get_size();
        blocks.set_size(blocks.get_size() + 1);
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

/***************
 * DESTRUCTION *
 ***************/
void lba_list_t::shutdown() {
    assert(state == state_ready);    
    state = state_shut_down;
    
    assert(!last_write);
    if (current_extent) {
        delete current_extent;
        current_extent = NULL;
    }
    if(superblock_extent) {
        gdelete(superblock_extent);
        superblock_extent = NULL;
    }
}

lba_list_t::~lba_list_t() {
    assert(state == state_unstarted || state == state_shut_down);
    assert(last_write == NULL);
    assert(current_extent == NULL);
}

