#include "disk_structure.hpp"

lba_disk_structure_t::lba_disk_structure_t(extent_manager_t *em, direct_file_t *file)
    : em(em), file(file), superblock_extent(NULL), last_extent(NULL)
{
}

lba_disk_structure_t::lba_disk_structure_t(extent_manager_t *em, direct_file_t *file, lba_shard_metablock_t *metablock)
    : em(em), file(file)
{
    if (metablock->last_lba_extent_offset != NULL_OFFSET) {
        last_extent = new lba_disk_extent_t(em, file, metablock->last_lba_extent_offset, metablock->last_lba_extent_entries_count);
    } else {
        last_extent = NULL;
    }
    
    if (metablock->lba_superblock_offset != NULL_OFFSET) {
        superblock_offset = metablock->lba_superblock_offset;
        startup_superblock_count = metablock->lba_superblock_entries_count;

        off64_t superblock_extent_offset = superblock_offset - (superblock_offset % em->extent_size);
        size_t superblock_size = ceil_aligned(
            offsetof(lba_superblock_t, entries[0]) + sizeof(lba_superblock_entry_t) * startup_superblock_count,
            DEVICE_BLOCK_SIZE);
        superblock_extent = new extent_t(em, file, superblock_extent_offset,
            superblock_offset + superblock_size - superblock_extent_offset);
        
        startup_superblock_buffer = (lba_superblock_t *)malloc_aligned(superblock_size, DEVICE_BLOCK_SIZE);
        superblock_extent->read(
            superblock_offset - superblock_extent_offset,
            superblock_size,
            startup_superblock_buffer,
            this);
    } else {
        superblock_extent = NULL;
    }
}

void lba_disk_structure_t::set_load_callback(load_callback_t *cb) {
    if (superblock_extent) {
        start_callback = cb;
    } else {
        cb->on_lba_load();
    }
}

void lba_disk_structure_t::on_extent_read() {
    for (int i = 0; i < startup_superblock_count; i++) {
        extents_in_superblock.push_back(
            new lba_disk_extent_t(em, file, 
                startup_superblock_buffer->entries[i].offset,
                startup_superblock_buffer->entries[i].lba_entries_count));
    }
    
    free(startup_superblock_buffer);
    
    start_callback->on_lba_load();
}

void lba_disk_structure_t::add_entry(ser_block_id_t block_id, repli_timestamp recency, flagged_off64_t offset) {
    if (last_extent && last_extent->full()) {
        /* We have filled up an extent. Transfer it to the superblock. */

        extents_in_superblock.push_back(last_extent);
        last_extent = NULL;

        /* Since there is a new extent on the superblock, we need to rewrite the superblock. Make
        sure that the superblock extent has enough room for a new superblock. */

        size_t superblock_size = sizeof(lba_superblock_t) + sizeof(lba_superblock_entry_t) * extents_in_superblock.size();
        assert(superblock_size <= em->extent_size);

        if (superblock_extent && superblock_extent->amount_filled + superblock_size > em->extent_size) {
            superblock_extent->destroy();
            superblock_extent = NULL;
        }

        if (!superblock_extent) {
            superblock_extent = new extent_t(em, file);
        }

        /* Prepare the new superblock. */

        char buffer[ceil_aligned(superblock_size, DEVICE_BLOCK_SIZE)];
        bzero(buffer, ceil_aligned(superblock_size, DEVICE_BLOCK_SIZE));

        lba_superblock_t *new_superblock = (lba_superblock_t *)buffer;
        memcpy(new_superblock->magic, lba_super_magic, LBA_SUPER_MAGIC_SIZE);
        int i = 0;
        for (lba_disk_extent_t *e = extents_in_superblock.head(); e; e = extents_in_superblock.next(e)) {
            new_superblock->entries[i].offset = e->offset;
            new_superblock->entries[i].lba_entries_count = e->count;
            i++;
        }

        /* Write the new superblock */

        superblock_offset = superblock_extent->offset + superblock_extent->amount_filled;
        superblock_extent->append(buffer, ceil_aligned(superblock_size, DEVICE_BLOCK_SIZE));
    }

    if (!last_extent) {
        last_extent = new lba_disk_extent_t(em, file);
    }

    assert(!last_extent->full());

    // TODO: timestamp
    last_extent->add_entry(lba_entry_t::make(block_id, recency, offset));
}

struct lba_writer_t :
    public extent_t::sync_callback_t
{
    int outstanding_cbs;
    lba_disk_structure_t::sync_callback_t *callback;
    
    explicit lba_writer_t(lba_disk_structure_t::sync_callback_t *cb) {
        outstanding_cbs = 0;
        callback = cb;
    }
    
    void on_extent_sync() {
        outstanding_cbs--;
        if (outstanding_cbs == 0) {
            if (callback) callback->on_lba_sync();
            delete this;
        }
    }
};

void lba_disk_structure_t::sync(sync_callback_t *cb) {
    lba_writer_t *writer = new lba_writer_t(cb);
    
    /* Count how many things need to be synced */
    if (last_extent) writer->outstanding_cbs++;
    if (superblock_extent) writer->outstanding_cbs++;
    writer->outstanding_cbs += extents_in_superblock.size();
    
    /* Sync the things that need to be synced */
    if (writer->outstanding_cbs == 0) {
        cb->on_lba_sync();
        delete writer;
    } else {
        if (last_extent) last_extent->sync(writer);
        if (superblock_extent) superblock_extent->sync(writer);
        for (lba_disk_extent_t *e = extents_in_superblock.head(); e; e = extents_in_superblock.next(e)) {
            e->sync(writer);
        }
    }
}

struct reader_t :
    public extent_t::read_callback_t
{
    lba_disk_structure_t *ds;
    in_memory_index_t *index;
    lba_disk_structure_t::read_callback_t *rcb;
    
    struct extent_info_t {
        lba_disk_extent_t *extent;
        lba_disk_extent_t::read_info_t read_info;
        explicit extent_info_t(lba_disk_extent_t *e) : extent(e) { }
    };
    std::vector< extent_info_t > extents;
    int cbs_out;
    
    reader_t(lba_disk_structure_t *ds, in_memory_index_t *index, lba_disk_structure_t::read_callback_t *cb)
        : ds(ds), index(index), rcb(cb)
    {
        for (lba_disk_extent_t *e = ds->extents_in_superblock.head(); e; e = ds->extents_in_superblock.next(e)) {
            extents.push_back(extent_info_t(e));
        }
        if (ds->last_extent) extents.push_back(extent_info_t(ds->last_extent));
        
        if (extents.size() == 0) {
            rcb->on_lba_read();
            delete this;
        } else {
            cbs_out = extents.size();
            
            // In case all the reads complete immediately and we get deleted, store a copy
            // of extents.size() on the stack
            int size = extents.size();
            
            for (int i = 0; i < size; i++) {
                extents[i].extent->read_step_1(&extents[i].read_info, this);
            }
        }
    }
    
    void on_extent_read() {
        cbs_out--;
        if (cbs_out == 0) {
            for (unsigned i = 0; i < extents.size(); i++) {
                extents[i].extent->read_step_2(&extents[i].read_info, index);
            }
            rcb->on_lba_read();
            delete this;
        }
    }
};

void lba_disk_structure_t::read(in_memory_index_t *index, read_callback_t *cb) {
    new reader_t(this, index, cb);
}

void lba_disk_structure_t::prepare_metablock(lba_shard_metablock_t *mb_out) {
    if (last_extent) {
        mb_out->last_lba_extent_offset = last_extent->offset;
        mb_out->last_lba_extent_entries_count = last_extent->count;

        assert((offsetof(lba_extent_t, entries[0]) + sizeof(lba_entry_t) * mb_out->last_lba_extent_entries_count)
               % DEVICE_BLOCK_SIZE == 0);
    } else {
        mb_out->last_lba_extent_offset = NULL_OFFSET;
        mb_out->last_lba_extent_entries_count = 0;
    }
    
    if (extents_in_superblock.size()) {
        mb_out->lba_superblock_offset = superblock_offset;
        mb_out->lba_superblock_entries_count = extents_in_superblock.size();
    } else {
        mb_out->lba_superblock_offset = NULL_OFFSET;
        mb_out->lba_superblock_entries_count = 0;
    }
}

int lba_disk_structure_t::num_entries_that_can_fit_in_an_extent() const {
    return (em->extent_size - offsetof(lba_extent_t, entries[0])) / sizeof(lba_entry_t);
}

void lba_disk_structure_t::destroy() {
    if (superblock_extent) superblock_extent->destroy();
    while (lba_disk_extent_t *e = extents_in_superblock.head()) {
        extents_in_superblock.remove(e);
        e->destroy();
    }
    if (last_extent) last_extent->destroy();
    delete this;
}

void lba_disk_structure_t::shutdown() {
    if (superblock_extent) superblock_extent->shutdown();
    while (lba_disk_extent_t *e = extents_in_superblock.head()) {
        extents_in_superblock.remove(e);
        e->shutdown();
    }
    if (last_extent) last_extent->shutdown();
    delete this;
}
