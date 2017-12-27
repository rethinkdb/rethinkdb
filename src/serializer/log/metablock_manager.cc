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
#include "math.hpp"
#include "serializer/log/log_serializer.hpp"
#include "serializer/log/metablock.hpp"
#include "version.hpp"

metablock_filerange_t padding_filerange() {
    metablock_filerange_t ret = { 0, 0 };
    return ret;
}

namespace crc_metablock {
uint32_t compute_metablock_crc(const crc_metablock_t *crc_mb) {
    boost::crc_32_type crc;
    crc.process_bytes(&crc_mb->disk_format_version, sizeof(crc_mb->disk_format_version));
    crc.process_bytes(&crc_mb->version, sizeof(crc_mb->version));
    crc.process_bytes(&crc_mb->metablock, sizeof(crc_mb->metablock));
    return crc.checksum();
}

struct checksum_filerange_less {
    bool operator()(const checksum_filerange &x, const checksum_filerange &y) const {
        return x.offset < y.offset;
    }
};

// Returns true if we should double-datasync (ranges are all padding).
bool prepare_checksums(metablock_fileranges_checksum_t *disk_list,
                       optional<std::vector<checksum_filerange>> &&checksums) {
    if (!checksums.has_value()) {
        goto prepare_padding_ranges;
    }

    if (checksums.has_value()) {
        // Sort and compactify the checksums.
        std::vector<checksum_filerange> ranges = std::move(checksums.get());
        if (ranges.size() == 0) {
            // Technically there's nothing wrong with having no checksum-ranges, but it
            // means we are doing a superfluous metablock write.
            goto prepare_padding_ranges;
        }
        std::sort(ranges.begin(), ranges.end(), checksum_filerange_less());

        size_t merge_ix = 0;

        for (size_t i = 1; i < ranges.size(); ++i) {
            if (ranges[merge_ix].offset + ranges[merge_ix].size == ranges[i].offset) {
                serializer_checksum concat
                    = compute_checksum_concat(ranges[merge_ix].checksum,
                                              ranges[i].checksum,
                                              uint64_t(ranges[i].size) / serializer_checksum::word_size);
                ranges[merge_ix].size += ranges[i].size;
                ranges[merge_ix].checksum = concat;
            } else {
                ++merge_ix;
                ranges[merge_ix] = ranges[i];
            }
        }

        size_t checksum_count = merge_ix + 1;
        if (checksum_count > METABLOCK_NUM_CHECKSUMS) {
            goto prepare_padding_ranges;
        }

        serializer_checksum combined_sum = identity_checksum();
        for (size_t i = 0; i < checksum_count; ++i) {
            combined_sum = compute_checksum_concat(combined_sum, ranges[i].checksum,
                                                   ranges[i].size / serializer_checksum::word_size);
            metablock_filerange_t range = { ranges[i].offset, ranges[i].size };
            disk_list->fileranges[i] = range;
        }
        metablock_filerange_t zero_range = padding_filerange();
        for (size_t i = checksum_count; i < METABLOCK_NUM_CHECKSUMS; ++i) {
            disk_list->fileranges[i] = zero_range;
        }
        disk_list->checksum = combined_sum;
        return false;
    }


 prepare_padding_ranges:
    metablock_filerange_t zero_range = padding_filerange();
    for (size_t i = 0; i < METABLOCK_NUM_CHECKSUMS; ++i) {
        disk_list->fileranges[i] = zero_range;
    }
    disk_list->checksum = identity_checksum();
    return true;
}

// crc_mb->metablock is already initialized, the rest of the metablock is zero-filled.
// Returns true if we should double-datasync.
bool prepare(crc_metablock_t *crc_mb, uint32_t _disk_format_version,
             metablock_version_t vers,
             optional<std::vector<checksum_filerange>> &&checksums) {
    crc_mb->disk_format_version = _disk_format_version;
    memcpy(crc_mb->magic_marker, MB_MARKER_MAGIC, sizeof(MB_MARKER_MAGIC));
    crc_mb->version = vers;

    bool double_datasync = prepare_checksums(&crc_mb->fileranges_checksum_v2_5,
                                             std::move(checksums));

    crc_mb->_crc = compute_metablock_crc(crc_mb);
    return double_datasync;
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
                            DEFAULT_DISK_ACCOUNT, &callback, datasync_op::no_datasyncs);
    }
    callback.wait();

    buffer = std::move(initial);

    optional<std::vector<checksum_filerange>> checksums = r_nullopt;
    /* Write the first metablock */
    // We use cluster_version_t::LATEST_DISK.  Maybe we'd want to decouple cluster
    // versions from disk format versions?  We can do that later if we want.
    crc_metablock::prepare(buffer.get(),
                           static_cast<uint32_t>(cluster_version_t::LATEST_DISK),
                           MB_START_VERSION,
                           std::move(checksums));
    co_write(dbfile, metablock_offsets::get(extent_size, 0), METABLOCK_SIZE, buffer.get(),
             DEFAULT_DISK_ACCOUNT, datasync_op::wrap_in_datasyncs);
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

bool metablock_manager_t::verify_checksum_fileranges(const crc_metablock_t *mb) {
    if (!disk_format_version_is_recognized(mb->disk_format_version)) {
        fail_due_to_user_error(
                "Data version not recognized. Is the data "
                "directory from a newer version of RethinkDB? "
                "(version on disk: %" PRIu32 ")",
                mb->disk_format_version);
    }

    if (mb->disk_format_version != static_cast<uint32_t>(cluster_version_t::v2_5_is_latest_disk)) {
        // There are no checksums.
        return true;
    }

    // (These file ranges are supposed to be a small amount of data that we can load
    // into memory all at once.)

    std::vector<scoped_device_block_aligned_ptr_t<char>> bufs;

    struct : public iocallback_t, public cond_t {
        size_t refcount;
        void on_io_complete() {
            refcount--;
            if (refcount == 0) { pulse(); }
        }
    } callback;
    // Start refcount at 1, call on_io_complete() at end of loop.
    callback.refcount = 1;

    int64_t total_read = 0;
    const metablock_fileranges_checksum_t *disk_list = &mb->fileranges_checksum_v2_5;
    size_t num_fileranges = 0;
    for (; num_fileranges < METABLOCK_NUM_CHECKSUMS; ++num_fileranges) {
        const metablock_filerange_t *range = &disk_list->fileranges[num_fileranges];
        if (range->size == 0) {
            guarantee(range->offset == 0);
            break;
        }
        guarantee(divides(DEVICE_BLOCK_SIZE, range->offset));
        guarantee(divides(DEVICE_BLOCK_SIZE, range->size));
        bufs.push_back(scoped_device_block_aligned_ptr_t<char>(range->size));
        total_read += range->size;
        callback.refcount++;
        dbfile->read_async(range->offset, range->size, bufs.back().get(),
                           DEFAULT_DISK_ACCOUNT, &callback);
    }

    callback.on_io_complete();
    callback.wait();
    extent_manager->stats->bytes_read(total_read);

    serializer_checksum combined_sum = identity_checksum();
    for (size_t i = 0; i < num_fileranges; ++i) {
        serializer_checksum x = compute_checksum(
                bufs[i].get(),
                disk_list->fileranges[i].size / serializer_checksum::word_size);
        combined_sum = compute_checksum_concat(
                combined_sum, x,
                disk_list->fileranges[i].size / serializer_checksum::word_size);
    }
    if (combined_sum.value != disk_list->checksum.value) {
        return false;
    }
    return true;
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

    static_assert(MB_BAD_VERSION < 0, "MB_BAD_VERSION is not -1, not unsigned");
    std::vector<std::pair<metablock_version_t, size_t>> indices_by_version;
    indices_by_version.reserve(num_metablocks);

    for (size_t i = 0; i < num_metablocks; ++i) {
        crc_metablock_t *mb = reinterpret_cast<crc_metablock_t *>(lbm.get() + i * METABLOCK_SIZE);
        if (crc_metablock::check_crc(mb)) {
            guarantee(mb->version != MB_BAD_VERSION);
            // Copy out mb->version to silence complaints about passing reference to
            // packed field to make_pair.
            metablock_version_t version = mb->version;
            indices_by_version.push_back(std::make_pair(version, i));
        }
    }

    if (indices_by_version.empty()) {
        /* no metablock found anywhere -- the DB is toast */
        next_version_number = MB_START_VERSION;
        *mb_found_out = false;

        /* The log serializer will catastrophically fail when it sees that mb_found is
           false. */
    } else {
        std::sort(indices_by_version.begin(), indices_by_version.end());
        size_t n = indices_by_version.size() - 1;
        size_t index = indices_by_version[n].second;

        crc_metablock_t *latest_crc_mb = reinterpret_cast<crc_metablock_t *>(lbm.get() + index * METABLOCK_SIZE);

        if (verify_checksum_fileranges(latest_crc_mb)) {
            // We found a metablock.
            next_version_number = indices_by_version[n].first + 1;
            next_mb_slot = metablock_offsets::next(extent_size, index);
            *mb_found_out = true;
            memcpy(mb_out, &latest_crc_mb->metablock, sizeof(log_serializer_metablock_t));
        } else {
            if (indices_by_version.size() == 1) {
                /* no metablock found anywhere -- the DB is toast */
                next_version_number = MB_START_VERSION;
                *mb_found_out = false;

                /* The log serializer will catastrophically fail when it sees that
                   mb_found is false. */
            } else {
                n -= 1;
                // Yes, we found a metablock.  Since a datasync happened after it was
                // written, we don't need to verify filerange checksums.
                index = indices_by_version[n].second;
                latest_crc_mb = reinterpret_cast<crc_metablock_t *>(lbm.get() + index * METABLOCK_SIZE);

                next_version_number = indices_by_version[n].first + 1;
                next_mb_slot = metablock_offsets::next(extent_size, index);
                *mb_found_out = true;
                memcpy(mb_out, &latest_crc_mb->metablock, sizeof(log_serializer_metablock_t));
            }
        }

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

// crc_mb.get() is zero-initialized, with crc_mb->metablock initialized.
void metablock_manager_t::co_write_metablock(
        const scoped_device_block_aligned_ptr_t<crc_metablock_t> &crc_mb,
        file_account_t *io_account,
        optional<std::vector<checksum_filerange>> &&checksums) {
    mutex_t::acq_t hold(&write_lock);

    rassert(state == state_ready);

    bool double_datasync
        = crc_metablock::prepare(crc_mb.get(),
                                 static_cast<uint32_t>(cluster_version_t::LATEST_DISK),
                                 next_version_number++,
                                 std::move(checksums));
    rassert(crc_metablock::check_crc(crc_mb.get()));

    state = state_writing;
    int64_t offset = metablock_offsets::get(extent_manager->extent_size, next_mb_slot);
    next_mb_slot = metablock_offsets::next(extent_manager->extent_size, next_mb_slot);
    co_write(dbfile, offset, METABLOCK_SIZE, crc_mb.get(), io_account,
             double_datasync
             ? datasync_op::wrap_in_datasyncs
             : datasync_op::datasync_after);

    state = state_ready;
    extent_manager->stats->bytes_written(METABLOCK_SIZE);
}

void metablock_manager_t::write_metablock_callback(
        const scoped_device_block_aligned_ptr_t<crc_metablock_t> *mb,
        file_account_t *io_account,
        optional<std::vector<checksum_filerange>> *checksums,
        metablock_write_callback_t *cb) {
    co_write_metablock(*mb, io_account, std::move(*checksums));
    cb->on_metablock_write();
}

void metablock_manager_t::write_metablock(
        const scoped_device_block_aligned_ptr_t<crc_metablock_t> &crc_mb,
        file_account_t *io_account,
        optional<std::vector<checksum_filerange>> &&checksums,
        metablock_write_callback_t *cb) {
    coro_t::spawn_later_ordered(std::bind(&metablock_manager_t::write_metablock_callback,
                                          this, &crc_mb, io_account, &checksums, cb));
}

void metablock_manager_t::shutdown() {

    rassert(state == state_ready);
    state = state_shut_down;
}
