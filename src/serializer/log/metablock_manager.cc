// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "serializer/log/metablock_manager.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/arch.hpp"
#include "arch/runtime/coroutines.hpp"
#include "concurrency/cond_var.hpp"
#include "serializer/log/log_serializer.hpp"
#include "version.hpp"

std::vector<int64_t> initial_metablock_offsets(int64_t extent_size) {
    std::vector<int64_t> offsets;

    const int64_t metablocks_per_extent = std::min<int64_t>(extent_size / METABLOCK_SIZE, MB_BLOCKS_PER_EXTENT);

    // The very first DEVICE_BLOCK_SIZE of the file is used for the
    // static header, so we start j at 1.
    for (int64_t j = 1; j < metablocks_per_extent; ++j) {
        int64_t offset = j * METABLOCK_SIZE;

        offsets.push_back(offset);
    }

    return offsets;
}

/* head functions */

template<class metablock_t>
metablock_manager_t<metablock_t>::metablock_manager_t::head_t::head_t(metablock_manager_t *manager)
    : mb_slot(0), saved_mb_slot(static_cast<uint32_t>(-1)), wraparound(false), mgr(manager) { }

template<class metablock_t>
void metablock_manager_t<metablock_t>::metablock_manager_t::head_t::operator++() {
    mb_slot++;
    wraparound = false;

    if (mb_slot >= mgr->metablock_offsets.size()) {
        mb_slot = 0;
        wraparound = true;
    }
}

template<class metablock_t>
int64_t metablock_manager_t<metablock_t>::metablock_manager_t::head_t::offset() {

    return mgr->metablock_offsets[mb_slot];
}

template<class metablock_t>
void metablock_manager_t<metablock_t>::metablock_manager_t::head_t::push() {
    saved_mb_slot = mb_slot;
}

template<class metablock_t>
void metablock_manager_t<metablock_t>::metablock_manager_t::head_t::pop() {
    guarantee(saved_mb_slot != static_cast<uint32_t>(-1), "Popping without a saved state");
    mb_slot = saved_mb_slot;
    saved_mb_slot = static_cast<uint32_t>(-1);
}

template<class metablock_t>
metablock_manager_t<metablock_t>::metablock_manager_t(extent_manager_t *em)
    : head(this), mb_buffer(METABLOCK_SIZE),
      extent_manager(em), metablock_offsets(initial_metablock_offsets(extent_manager->extent_size)),
      state(state_unstarted), dbfile(nullptr) {
    rassert(sizeof(crc_metablock_t) <= METABLOCK_SIZE);
    rassert(mb_buffer.has());
    mb_buffer_in_use = false;

    /* Build the list of metablock locations in the file */

    // We don't try to reserve any metablock extents because the only
    // extent we use is extent 0.  Extent 0 is already reserved by the
    // static header, so we don't need to reserve it.
}

template<class metablock_t>
metablock_manager_t<metablock_t>::~metablock_manager_t() {

    rassert(state == state_unstarted || state == state_shut_down);

    rassert(!mb_buffer_in_use);
}

template<class metablock_t>
void metablock_manager_t<metablock_t>::create(file_t *dbfile, int64_t extent_size, metablock_t *initial) {

    std::vector<int64_t> metablock_offsets = initial_metablock_offsets(extent_size);

    dbfile->set_file_size_at_least(metablock_offsets[metablock_offsets.size() - 1] + METABLOCK_SIZE);

    /* Allocate a buffer for doing our writes */
    scoped_device_block_aligned_ptr_t<crc_metablock_t> buffer(METABLOCK_SIZE);
    memset(buffer.get(), 0, METABLOCK_SIZE);

    /* Wipe the metablock slots so we don't mistake something left by a previous database for a
    valid metablock. */
    struct : public iocallback_t, public cond_t {
        int refcount;
        void on_io_complete() {
            refcount--;
            if (refcount == 0) pulse();
        }
    } callback;
    callback.refcount = metablock_offsets.size();
    for (unsigned i = 0; i < metablock_offsets.size(); i++) {
        // We don't datasync here -- we can datasync when we write the first real
        // metablock.
        dbfile->write_async(metablock_offsets[i], METABLOCK_SIZE, buffer.get(),
                            DEFAULT_DISK_ACCOUNT, &callback, file_t::NO_DATASYNCS);
    }
    callback.wait();

    /* Write the first metablock */
    // We use cluster_version_t::LATEST_DISK.  Maybe we'd want to decouple cluster
    // versions from disk format versions?  We can do that later if we want.
    buffer->prepare(static_cast<uint32_t>(cluster_version_t::LATEST_DISK), initial,
                                          MB_START_VERSION);
    co_write(dbfile, metablock_offsets[0], METABLOCK_SIZE, buffer.get(),
             DEFAULT_DISK_ACCOUNT, file_t::WRAP_IN_DATASYNCS);
}

bool disk_format_version_is_recognized(uint32_t disk_format_version) {
    // Someday, we might have to do more than recognize a disk format version to be
    // valid -- the block structure of LBAs or extents might require us to look at
    // the current metablock's version number more closely.  If you have a new
    // cluster_version_t value and such changes have not happened, it is correct to
    // add the new cluster_version_t value to this list of recognized ones.
    if (disk_format_version
        == static_cast<uint32_t>(obsolete_cluster_version_t::v1_13)) {
        fail_due_to_user_error(
            "Data directory is from version 1.13 of RethinkDB, "
            "which is no longer supported.  "
            "You can migrate by launching RethinkDB 2.0 with this data directory "
            "and rebuilding your secondary indexes.");
    }
    return disk_format_version == static_cast<uint32_t>(cluster_version_t::v1_14)
        || disk_format_version == static_cast<uint32_t>(cluster_version_t::v1_15)
        || disk_format_version == static_cast<uint32_t>(cluster_version_t::v1_16)
        || disk_format_version == static_cast<uint32_t>(cluster_version_t::v2_0)
        || disk_format_version == static_cast<uint32_t>(cluster_version_t::v2_1)
        || disk_format_version == static_cast<uint32_t>(cluster_version_t::v2_2)
        || disk_format_version ==
            static_cast<uint32_t>(cluster_version_t::v2_3_is_latest);
}


template<class metablock_t>
void metablock_manager_t<metablock_t>::co_start_existing(file_t *file, bool *mb_found, metablock_t *mb_out) {
    rassert(state == state_unstarted);
    dbfile = file;
    rassert(dbfile != nullptr);

    rassert(!mb_buffer_in_use);
    mb_buffer_in_use = true;

    metablock_version_t latest_version = MB_BAD_VERSION;

    dbfile->set_file_size_at_least(metablock_offsets[metablock_offsets.size() - 1] + METABLOCK_SIZE);

    // Reading metablocks by issuing one I/O request at a time is
    // slow. Read all of them in one batch, and check them later.
    state = state_reading;
    struct load_buffer_manager_t {
        explicit load_buffer_manager_t(size_t nmetablocks)
            : buffer(METABLOCK_SIZE * nmetablocks) { }
        crc_metablock_t *get_metablock(unsigned i) {
            return reinterpret_cast<crc_metablock_t *>(reinterpret_cast<char *>(buffer.get()) + METABLOCK_SIZE * i);
        }

    private:
        scoped_device_block_aligned_ptr_t<crc_metablock_t> buffer;
    } lbm(metablock_offsets.size());
    struct : public iocallback_t, public cond_t {
        int refcount;
        void on_io_complete() {
            refcount--;
            if (refcount == 0) pulse();
        }
    } callback;
    callback.refcount = metablock_offsets.size();
    for (unsigned i = 0; i < metablock_offsets.size(); i++) {
        dbfile->read_async(metablock_offsets[i], METABLOCK_SIZE, lbm.get_metablock(i),
                           DEFAULT_DISK_ACCOUNT, &callback);
    }
    callback.wait();
    extent_manager->stats->bytes_read(METABLOCK_SIZE * metablock_offsets.size());

    // TODO: we can parallelize this code even further by doing crc
    // checks as soon as a block is ready, as opposed to waiting for
    // all IO operations to complete and then checking. I'm dubious
    // about the benefits though, so leaving this alone.

    // We've read everything from disk. Now find the last good metablock.
    crc_metablock_t *last_good_mb = nullptr;
    for (unsigned i = 0; i < metablock_offsets.size(); i++) {
        crc_metablock_t *mb_temp = lbm.get_metablock(i);
        if (mb_temp->check_crc()) {
            if (mb_temp->version > latest_version) {
                /* this metablock is good, maybe there are more? */
                latest_version = mb_temp->version;
                head.push();
                ++head;
                last_good_mb = mb_temp;
            } else {
                break;
            }
        } else {
            ++head;
        }
    }

    // Cool, hopefully got our metablock. Wrap it up.
    if (latest_version == MB_BAD_VERSION) {

        /* no metablock found anywhere -- the DB is toast */

        next_version_number = MB_START_VERSION;
        *mb_found = false;

        /* The log serializer will catastrophically fail when it sees that mb_found is
           false. We could catastrophically fail here, but it's a bit nicer to have the
           metablock manager as a standalone component that doesn't know how to behave if there
           is no metablock. */

    } else {
        // Right now we don't have anything super-special between disk format
        // versions, just some per-block serialization versioning (which is
        // derived from the 4 bytes block_magic_t at the front of the block), so
        // we don't need to remember the version number -- we just assert it's
        // ok.
        if (!disk_format_version_is_recognized(last_good_mb->disk_format_version)) {
            fail_due_to_user_error(
                    "Data version not recognized. Is the data "
                    "directory from a newer version of RethinkDB? "
                    "(version on disk: %" PRIu32 ")",
                    last_good_mb->disk_format_version);
        }

        /* we found a metablock, set everything up */
        next_version_number = latest_version + 1;
        latest_version = MB_BAD_VERSION; /* version is now useless */
        head.pop();
        *mb_found = true;
        memcpy(mb_buffer.get(), last_good_mb, METABLOCK_SIZE);
        memcpy(mb_out, &(mb_buffer->metablock), sizeof(metablock_t));
    }
    mb_buffer_in_use = false;
    state = state_ready;
}

//The following two functions will go away in favor of the preceding one
template<class metablock_t>
void metablock_manager_t<metablock_t>::start_existing_callback(file_t *file, bool *mb_found, metablock_t *mb_out, metablock_read_callback_t *cb) {
    co_start_existing(file, mb_found, mb_out);
    cb->on_metablock_read();
}

template<class metablock_t>
bool metablock_manager_t<metablock_t>::start_existing(file_t *file, bool *mb_found, metablock_t *mb_out, metablock_read_callback_t *cb) {
    coro_t::spawn_later_ordered(boost::bind(&metablock_manager_t<metablock_t>::start_existing_callback, this, file, mb_found, mb_out, cb));
    return false;
}
template<class metablock_t>
void metablock_manager_t<metablock_t>::co_write_metablock(metablock_t *mb, file_account_t *io_account) {
    mutex_t::acq_t hold(&write_lock);

    rassert(state == state_ready);
    rassert(!mb_buffer_in_use);

    mb_buffer->prepare(static_cast<uint32_t>(cluster_version_t::LATEST_DISK), mb,
                                             next_version_number++);
    rassert(mb_buffer->check_crc());

    mb_buffer_in_use = true;

    state = state_writing;
    co_write(dbfile, head.offset(), METABLOCK_SIZE, mb_buffer.get(), io_account,
             file_t::WRAP_IN_DATASYNCS);

    ++head;

    state = state_ready;
    mb_buffer_in_use = false;
    extent_manager->stats->bytes_written(METABLOCK_SIZE);
}

template<class metablock_t>
void metablock_manager_t<metablock_t>::write_metablock_callback(metablock_t *mb, file_account_t *io_account, metablock_write_callback_t *cb) {
    co_write_metablock(mb, io_account);
    cb->on_metablock_write();
}

template<class metablock_t>
bool metablock_manager_t<metablock_t>::write_metablock(metablock_t *mb, file_account_t *io_account, metablock_write_callback_t *cb) {
    coro_t::spawn_later_ordered(boost::bind(&metablock_manager_t<metablock_t>::write_metablock_callback, this, mb, io_account, cb));
    return false;
}

template<class metablock_t>
void metablock_manager_t<metablock_t>::shutdown() {

    rassert(state == state_ready);
    rassert(!mb_buffer_in_use);
    state = state_shut_down;
}

template class metablock_manager_t<log_serializer_metablock_t>;
