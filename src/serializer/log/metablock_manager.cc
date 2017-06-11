// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "serializer/log/metablock_manager.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "errors.hpp"
#include <boost/crc.hpp>

#include "arch/arch.hpp"
#include "arch/runtime/coroutines.hpp"
#include "concurrency/cond_var.hpp"
#include "serializer/log/log_serializer.hpp"
#include "serializer/log/metablock.hpp"
#include "version.hpp"

namespace crc_metablock {
uint32_t compute_metablock_crc(const crc_metablock_t *crc_mb) {
    boost::crc_32_type crc;
    crc.process_bytes(&crc_mb->disk_format_version, sizeof(crc_mb->disk_format_version));
    crc.process_bytes(&crc_mb->version, sizeof(crc_mb->version));
    crc.process_bytes(&crc_mb->metablock, sizeof(crc_mb->metablock));
    return crc.checksum();
}

// crc_mb->metablock is already initialized, the rest of the metablock is zero-filled.
void prepare(crc_metablock_t *crc_mb, uint32_t _disk_format_version,
             metablock_version_t vers) {
    crc_mb->disk_format_version = _disk_format_version;
    memcpy(crc_mb->magic_marker, MB_MARKER_MAGIC, sizeof(MB_MARKER_MAGIC));
    crc_mb->version = vers;
    crc_mb->_crc = compute_metablock_crc(crc_mb);
}

bool check_crc(const crc_metablock_t *crc_mb) {
    return (crc_mb->_crc == compute_metablock_crc(crc_mb));
}
}  // namespace crc_metablock

namespace metablock_offsets {
// How many distinct metablock offsets there are.
size_t count(int64_t extent_size) {
    return std::min<int64_t>(extent_size / METABLOCK_SIZE, MB_BLOCKS_PER_EXTENT) - 1;
}

int64_t get(int64_t extent_size, size_t index) {
    size_t nmetablocks = metablock_offsets::count(extent_size);
    guarantee(index < nmetablocks);
    // The first DEVICE_BLOCK_SIZE of the file is used for the static header.
    return (index + 1) * METABLOCK_SIZE;
}

// How big the file has to be, to contain all metablocks.
int64_t min_filesize(int64_t extent_size) {
    return (1 + metablock_offsets::count(extent_size)) * METABLOCK_SIZE;
}

int64_t next(int64_t extent_size, size_t index) {
    return (1 + index) % metablock_offsets::count(extent_size);
}
}  // namespace metablock_offsets.


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

metablock_manager_t::metablock_manager_t(extent_manager_t *em)
    : next_mb_slot(SIZE_MAX),
      extent_manager(em),
      state(state_unstarted),
      dbfile(nullptr) {
    static_assert(sizeof(crc_metablock_t) <= METABLOCK_SIZE,
                  "crc_metablock_t too big");

    // We don't try to reserve any metablock extents because the only
    // extent we use is extent 0.  Extent 0 is already reserved by the
    // static header, so we don't need to reserve it.
}

metablock_manager_t::~metablock_manager_t() {
    rassert(state == state_unstarted || state == state_shut_down);
}

void metablock_manager_t::create(
        file_t *dbfile, int64_t extent_size,
        scoped_device_block_aligned_ptr_t<crc_metablock_t> &&initial) {
    dbfile->set_file_size_at_least(metablock_offsets::min_filesize(extent_size),
                                       extent_size);

    /* Allocate a buffer for doing our writes */
    scoped_device_block_aligned_ptr_t<crc_metablock_t> buffer(METABLOCK_SIZE);
    memset(buffer.get(), 0, METABLOCK_SIZE);

    /* Wipe the metablock slots so we don't mistake something left by a previous
    database for a valid metablock. */
    struct : public iocallback_t, public cond_t {
        size_t refcount;
        void on_io_complete() {
            refcount--;
            if (refcount == 0) pulse();
        }
    } callback;
    callback.refcount = metablock_offsets::count(extent_size);
    for (size_t i = 0; i < metablock_offsets::count(extent_size); i++) {
        // We don't datasync here -- we can datasync when we write the first real
        // metablock.
        dbfile->write_async(metablock_offsets::get(extent_size, i),
                            METABLOCK_SIZE, buffer.get(),
                            DEFAULT_DISK_ACCOUNT, &callback, file_t::NO_DATASYNCS);
    }
    callback.wait();

    buffer = std::move(initial);

    /* Write the first metablock */
    // We use cluster_version_t::LATEST_DISK.  Maybe we'd want to decouple cluster
    // versions from disk format versions?  We can do that later if we want.
    crc_metablock::prepare(buffer.get(),
                           static_cast<uint32_t>(cluster_version_t::LATEST_DISK),
                           MB_START_VERSION);
    co_write(dbfile, metablock_offsets::get(extent_size, 0), METABLOCK_SIZE, buffer.get(),
             DEFAULT_DISK_ACCOUNT, file_t::WRAP_IN_DATASYNCS);
}

bool disk_format_version_is_recognized(uint32_t disk_format_version) {
    // Someday, we might have to do more than recognize a disk format version to be
    // valid -- the block structure of LBAs or extents might require us to look at
    // the current metablock's version number more closely.  If you have a new
    // cluster_version_t value and such changes have not happened, it is correct to
    // add the new cluster_version_t value to this list of recognized ones.
    if (disk_format_version
        == static_cast<uint32_t>(obsolete_cluster_version_t::v1_13)
     || disk_format_version
        == static_cast<uint32_t>(obsolete_cluster_version_t::v1_13_2_is_latest)) {
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
        || disk_format_version == static_cast<uint32_t>(cluster_version_t::v2_3)
        || disk_format_version == static_cast<uint32_t>(cluster_version_t::v2_4)
        || disk_format_version ==
            static_cast<uint32_t>(cluster_version_t::v2_5_is_latest_disk);
}


void metablock_manager_t::co_start_existing(file_t *file, bool *mb_found_out,
                                            log_serializer_metablock_t *mb_out) {
    rassert(state == state_unstarted);
    dbfile = file;
    rassert(dbfile != nullptr);

    const int64_t extent_size = extent_manager->extent_size;
    const size_t num_metablocks = metablock_offsets::count(extent_size);

    dbfile->set_file_size_at_least(metablock_offsets::min_filesize(extent_size),
                                   extent_size);

    // Reading metablocks by issuing one I/O request at a time is
    // slow. Read all of them in one batch, and check them later.
    state = state_reading;

    scoped_device_block_aligned_ptr_t<char> lbm(METABLOCK_SIZE * num_metablocks);

    struct : public iocallback_t, public cond_t {
        size_t refcount;
        void on_io_complete() {
            refcount--;
            if (refcount == 0) { pulse(); }
        }
    } callback;
    callback.refcount = num_metablocks;
    for (size_t i = 0; i < num_metablocks; ++i) {
        dbfile->read_async(metablock_offsets::get(extent_size, i), METABLOCK_SIZE,
                           lbm.get() + i * METABLOCK_SIZE,
                           DEFAULT_DISK_ACCOUNT, &callback);
    }
    callback.wait();
    extent_manager->stats->bytes_read(METABLOCK_SIZE * metablock_offsets::count(extent_size));

    static_assert(MB_BAD_VERSION < 0, "MB_BAD_VERSION is not -1, not signed int64_t");
    metablock_version_t max_version = MB_BAD_VERSION;
    size_t max_index = SIZE_MAX;

    for (size_t i = 0; i < num_metablocks; ++i) {
        crc_metablock_t *mb = reinterpret_cast<crc_metablock_t *>(lbm.get() + i * METABLOCK_SIZE);
        if (crc_metablock::check_crc(mb)) {
            guarantee(mb->version != max_version);
            if (mb->version > max_version) {
                max_version = mb->version;
                max_index = i;
            }
        }
    }

    if (max_index == SIZE_MAX) {
        /* no metablock found anywhere -- the DB is toast */
        next_version_number = MB_START_VERSION;
        *mb_found_out = false;

        /* The log serializer will catastrophically fail when it sees that mb_found is
           false. */
    } else {
        crc_metablock_t *latest_crc_mb = reinterpret_cast<crc_metablock_t *>(lbm.get() + max_index * METABLOCK_SIZE);
        if (!disk_format_version_is_recognized(latest_crc_mb->disk_format_version)) {
            fail_due_to_user_error(
                    "Data version not recognized. Is the data "
                    "directory from a newer version of RethinkDB? "
                    "(version on disk: %" PRIu32 ")",
                    latest_crc_mb->disk_format_version);
        }

        // We found a metablock.
        next_version_number = max_version + 1;
        next_mb_slot = metablock_offsets::next(extent_size, max_index);
        *mb_found_out = true;
        memcpy(mb_out, &latest_crc_mb->metablock, sizeof(log_serializer_metablock_t));
    }

    state = state_ready;
}

//The following two functions will go away in favor of the preceding one
void metablock_manager_t::start_existing_callback(
        file_t *file, bool *mb_found, log_serializer_metablock_t *mb_out,
        metablock_read_callback_t *cb) {
    co_start_existing(file, mb_found, mb_out);
    cb->on_metablock_read();
}

bool metablock_manager_t::start_existing(
        file_t *file, bool *mb_found, log_serializer_metablock_t *mb_out,
        metablock_read_callback_t *cb) {
    coro_t::spawn_later_ordered(std::bind(&metablock_manager_t::start_existing_callback,
                                          this, file, mb_found, mb_out, cb));
    return false;
}

// crc_mb is zero-initialized.
void metablock_manager_t::co_write_metablock(
        const scoped_device_block_aligned_ptr_t<crc_metablock_t> &crc_mb,
        file_account_t *io_account) {
    mutex_t::acq_t hold(&write_lock);

    rassert(state == state_ready);

    crc_metablock::prepare(crc_mb.get(),
                           static_cast<uint32_t>(cluster_version_t::LATEST_DISK),
                           next_version_number++);
    rassert(crc_metablock::check_crc(crc_mb.get()));

    state = state_writing;
    int64_t offset = metablock_offsets::get(extent_manager->extent_size, next_mb_slot);
    next_mb_slot = metablock_offsets::next(extent_manager->extent_size, next_mb_slot);
    co_write(dbfile, offset, METABLOCK_SIZE, crc_mb.get(), io_account,
             file_t::WRAP_IN_DATASYNCS);

    state = state_ready;
    extent_manager->stats->bytes_written(METABLOCK_SIZE);
}

void metablock_manager_t::write_metablock_callback(
        const scoped_device_block_aligned_ptr_t<crc_metablock_t> *mb,
        file_account_t *io_account,
        metablock_write_callback_t *cb) {
    co_write_metablock(*mb, io_account);
    cb->on_metablock_write();
}

void metablock_manager_t::write_metablock(
        const scoped_device_block_aligned_ptr_t<crc_metablock_t> &crc_mb,
        file_account_t *io_account,
        metablock_write_callback_t *cb) {
    coro_t::spawn_later_ordered(std::bind(&metablock_manager_t::write_metablock_callback,
                                          this, &crc_mb, io_account, cb));
}

void metablock_manager_t::shutdown() {

    rassert(state == state_ready);
    state = state_shut_down;
}
