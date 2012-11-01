// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "serializer/log/lba/disk_structure.hpp"
#include "containers/scoped.hpp"

lba_disk_structure_t::lba_disk_structure_t(extent_manager_t *_em, direct_file_t *_file)
    : em(_em), file(_file), superblock_extent(NULL), last_extent(NULL)
{
}

lba_disk_structure_t::lba_disk_structure_t(extent_manager_t *_em, direct_file_t *_file, lba_shard_metablock_t *metablock)
    : em(_em), file(_file)
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

        startup_superblock_buffer = reinterpret_cast<lba_superblock_t *>(malloc_aligned(superblock_size, DEVICE_BLOCK_SIZE));
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

    /* We just read the superblock extent. */

    for (int i = 0; i < startup_superblock_count; i++) {
        extents_in_superblock.push_back(
            new lba_disk_extent_t(em, file,
                startup_superblock_buffer->entries[i].offset,
                startup_superblock_buffer->entries[i].lba_entries_count));
        // debugf("Added extent in superblock %d: %ld\n", i, startup_superblock_buffer->entries[i].offset);
    }

    free(startup_superblock_buffer);

    start_callback->on_lba_load();
}

void lba_disk_structure_t::add_entry(block_id_t block_id, repli_timestamp_t recency, flagged_off64_t offset, file_account_t *io_account, extent_transaction_t *txn) {
    if (last_extent && last_extent->full()) {
        /* We have filled up an extent. Transfer it to the superblock. */

        extents_in_superblock.push_back(last_extent);
        last_extent = NULL;

        /* Since there is a new extent on the superblock, we need to rewrite the superblock. Make
        sure that the superblock extent has enough room for a new superblock. */

        size_t superblock_size = sizeof(lba_superblock_t) + sizeof(lba_superblock_entry_t) * extents_in_superblock.size();
        rassert(superblock_size <= em->extent_size);

        if (superblock_extent && superblock_extent->amount_filled + superblock_size > em->extent_size) {
            superblock_extent->destroy(txn);
            superblock_extent = NULL;
        }

        if (!superblock_extent) {
            superblock_extent = new extent_t(em, file);
        }

        /* Prepare the new superblock. */

        scoped_malloc_t<char> buffer(ceil_aligned(superblock_size, DEVICE_BLOCK_SIZE));
        bzero(buffer.get(), ceil_aligned(superblock_size, DEVICE_BLOCK_SIZE));

        lba_superblock_t *new_superblock = reinterpret_cast<lba_superblock_t *>(buffer.get());
        memcpy(new_superblock->magic, lba_super_magic, LBA_SUPER_MAGIC_SIZE);
        int i = 0;
        for (lba_disk_extent_t *e = extents_in_superblock.head(); e; e = extents_in_superblock.next(e)) {
            new_superblock->entries[i].offset = e->offset;
            new_superblock->entries[i].lba_entries_count = e->count;
            i++;
        }

        /* Write the new superblock */

        superblock_offset = superblock_extent->offset + superblock_extent->amount_filled;
        superblock_extent->append(buffer.get(), ceil_aligned(superblock_size, DEVICE_BLOCK_SIZE), io_account);
    }

    if (!last_extent) {
        last_extent = new lba_disk_extent_t(em, file, io_account);
    }

    rassert(!last_extent->full());

    // TODO: timestamp
    last_extent->add_entry(lba_entry_t::make(block_id, recency, offset), io_account);
}

class lba_writer_t :
    public extent_t::sync_callback_t
{
public:
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

void lba_disk_structure_t::sync(file_account_t *io_account, sync_callback_t *cb) {
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
        if (last_extent) last_extent->sync(io_account, writer);
        if (superblock_extent) superblock_extent->sync(writer);
        for (lba_disk_extent_t *e = extents_in_superblock.head(); e; e = extents_in_superblock.next(e)) {
            e->sync(io_account, writer);
        }
    }
}

struct reader_t
{
    lba_disk_structure_t *ds;   // The disk structure we are reading from
    in_memory_index_t *index;   // The in-memory-index we are reading into
    lba_disk_structure_t::read_callback_t *rcb;   // Who to call back when we finish

    /* extent_reader_t takes care of reading a single extent. */
    struct extent_reader_t :
        public extent_t::read_callback_t
    {
        reader_t *parent;   // Our reader_t that we were created by
        int index;   // parent->readers[index] = this
        lba_disk_extent_t *extent;   // The extent we are supposed to read
        lba_disk_extent_t::read_info_t read_info;   // Opaque data used by extent_t::read()
        bool have_read;   // true if our extent has been loaded from disk

        /* true if the extent before us called read_step_2(). We keep track of this
        because we must be sure to call read_step_2() in the right order at all times;
        otherwise more recent LBA data would be applied after less recent LBA data
        and the LBA would be corrupted. */
        bool prev_done;

        extent_reader_t(reader_t *p, lba_disk_extent_t *e)
            : parent(p), extent(e), have_read(false)
        {
            index = parent->readers.size();
            parent->readers.push_back(this);

            if (index == 0) {
                // We are the first; we don't have to wait for a previous extent
                prev_done = true;
            } else {
                prev_done = false;
            }
        }
        void start_reading() {
            parent->active_readers++;
            extent->read_step_1(&read_info, this);
        }
        void on_extent_read() {   // Called when our extent has been read from disk
            rassert(!have_read);
            have_read = true;
            if (prev_done) done();
        }
        void on_prev_done() {   // Called by the previous extent_reader_t when it finishes
            rassert(!prev_done);
            prev_done = true;
            if (have_read) done();
        }
        void done() {
            extent->read_step_2(&read_info, parent->index);
            parent->active_readers--;
            parent->start_more_readers();
            if (index == static_cast<int>(parent->readers.size()) - 1) {
                parent->done();
            } else {
                parent->readers[index+1]->on_prev_done();
            }
            delete this;
        }
    };
    std::vector< extent_reader_t* > readers;

    int next_reader;   // The index of the next reader that we should call start_reading() on

    // The number of readers that have done start_reading() but not done(). Used to throttle the
    // reading process so that we stay under LBA_READ_BUFFER_SIZE.
    int active_readers;

    reader_t(lba_disk_structure_t *_ds, in_memory_index_t *_index, lba_disk_structure_t::read_callback_t *cb)
        : ds(_ds), index(_index), rcb(cb)
    {
        for (lba_disk_extent_t *e = ds->extents_in_superblock.head(); e; e = ds->extents_in_superblock.next(e)) {
            new extent_reader_t(this, e);
        }
        if (ds->last_extent) new extent_reader_t(this, ds->last_extent);

        /* The constructor for extent_reader_t pushed them onto our 'readers' vector. So now we
        have a vector with an extent_reader_t object for each extent we need to read, but none
        of them have been started yet. */

        if (readers.size() == 0) {
            done();
        } else {
            next_reader = 0;
            active_readers = 0;
            start_more_readers();
        }
    }

    void start_more_readers() {
        int limit = std::max<int>(LBA_READ_BUFFER_SIZE / ds->em->extent_size / LBA_SHARD_FACTOR, 1);
        while (next_reader != static_cast<int>(readers.size()) && active_readers < limit) {
            readers[next_reader++]->start_reading();
        }
    }

    void done() {
        rcb->on_lba_read();
        delete this;
    }
};

void lba_disk_structure_t::read(in_memory_index_t *index, read_callback_t *cb) {
    new reader_t(this, index, cb);
}

void lba_disk_structure_t::prepare_metablock(lba_shard_metablock_t *mb_out) {
    if (last_extent) {
        mb_out->last_lba_extent_offset = last_extent->offset;
        mb_out->last_lba_extent_entries_count = last_extent->count;

        rassert(divides(DEVICE_BLOCK_SIZE, offsetof(lba_extent_t, entries[0]) + sizeof(lba_entry_t) * mb_out->last_lba_extent_entries_count));

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

void lba_disk_structure_t::destroy(extent_transaction_t *txn) {
    if (superblock_extent) {
        superblock_extent->destroy(txn);
    }

    while (lba_disk_extent_t *e = extents_in_superblock.head()) {
        extents_in_superblock.remove(e);
        e->destroy(txn);
    }

    if (last_extent) {
        last_extent->destroy(txn);
    }

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
