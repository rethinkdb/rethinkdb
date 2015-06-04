// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "serializer/log/data_block_manager.hpp"

#include <inttypes.h>
#include <sys/uio.h>

#include <functional>

#include "arch/arch.hpp"
#include "arch/runtime/coroutines.hpp"
#include "concurrency/mutex.hpp"
#include "concurrency/new_mutex.hpp"
#include "errors.hpp"
#include "perfmon/perfmon.hpp"
#include "serializer/buf_ptr.hpp"
#include "serializer/log/log_serializer.hpp"
#include "stl_utils.hpp"

// Max amount of bytes which can be read ahead in one i/o transaction (if enabled)
const int64_t APPROXIMATE_READ_AHEAD_SIZE = 32 * DEFAULT_BTREE_BLOCK_SIZE;

/*****************
 * GC Parameters *
 *****************/

// How many GC routines to launch in concurrently, at maximum
const size_t MAX_CONCURRENT_GCS = 32;

// Garbage Collection uses its own two IO accounts.
// There is one low-priority account that is meant to guarantee
// (performance-wise) unintrusive garbage collection.
// If the garbage ratio keeps growing,
// GC starts using the high priority account instead, which
// might have a negative influence on database performance
// under i/o heavy workloads but guarantees that the database
// doesn't grow indefinitely.
const int GC_IO_PRIORITY_NICE = 8;
// 4 times the priority of all caches combined
const int GC_IO_PRIORITY_HIGH = 4 * MERGER_BLOCK_WRITE_IO_PRIORITY;

// The ratio at which we start GCing.
const double GC_START_RATIO = 0.15;
// The ratio at which we don't want to keep GC'ing.
const double GC_STOP_RATIO = 0.1;
// The ratio at which we start taking more serious measures to get the garbage
// rate down.
const double GC_HIGH_RATIO = 0.5;

// What's the maximum number of "young" extents we can have?
const size_t GC_YOUNG_EXTENT_MAX_SIZE = 50;
// What's the definition of a "young" extent in microseconds?
const microtime_t GC_YOUNG_EXTENT_TIMELIMIT_MICROS = 50000;


// Identifies an extent, the time we started writing to the
// extent, whether it's the extent we're currently writing to, and
// describes blocks are garbage.
class gc_entry_t : public intrusive_list_node_t<gc_entry_t> {
private:
    struct block_info_t {
        uint32_t relative_offset;
        block_size_t block_size;
        bool token_referenced;
        bool index_referenced;
    };

public:
    /* This constructor is for starting a new active extent. */
    explicit gc_entry_t(data_block_manager_t *_parent)
        : parent(_parent),
          extent_ref(parent->extent_manager->gen_extent()),
          timestamp(current_microtime()),
          was_written(false),
          state(state_active),
          garbage_bytes_stat(_parent->static_config->extent_size()),
          num_live_blocks_stat(0),
          extent_offset(extent_ref.offset()) {
        add_self_to_parent_entries();
    }

    /* This constructor is for reconstructing extents that the LBA tells us contained
       data blocks. */
    gc_entry_t(data_block_manager_t *_parent, int64_t _offset)
        : parent(_parent),
          extent_ref(parent->extent_manager->reserve_extent(_offset)),
          timestamp(current_microtime()),
          was_written(false),
          state(state_reconstructing),
          garbage_bytes_stat(_parent->static_config->extent_size()),
          num_live_blocks_stat(0),
          extent_offset(extent_ref.offset()) {
        add_self_to_parent_entries();
    }

    void destroy() {
        parent->extent_manager->release_extent(std::move(extent_ref));
        delete this;
    }

    ~gc_entry_t() {
        uint64_t extent_id = parent->static_config->extent_index(extent_offset);
        guarantee(parent->entries.get(extent_id) == this);
        parent->entries.set(extent_id, NULL);

        --parent->stats->pm_serializer_data_extents;
    }

    // (To borrow Python syntax: returns [relative_offset(i) for i in range(0,
    // num_blocks())] + [back_relative_offset()].
    std::vector<uint32_t> block_boundaries() const {
        guarantee(state != state_reconstructing);

        std::vector<uint32_t> ret;
        for (auto it = block_infos.begin(); it != block_infos.end(); ++it) {
            ret.push_back(it->relative_offset);
        }

        ret.push_back(back_relative_offset());
        return ret;
    }

    // The offset at the beginning of unused space -- the end of the last block
    // in the extent (rounded up to DEVICE_BLOCK_SIZE).
    uint32_t back_relative_offset() const {
        return block_infos.empty()
            ? 0
            : block_infos.back().relative_offset
            + aligned_value(block_infos.back().block_size);
    }

    // Returns the ostensible size of the block_index'th block.  Note that
    // block_boundaries[i] + block_size(i) <= block_boundaries[i + 1].
    block_size_t block_size(unsigned int block_index) const {
        guarantee(state != state_reconstructing);
        guarantee(block_index < block_infos.size());
        return block_infos[block_index].block_size;
    }

    // Returns block_boundaries()[block_index].
    uint32_t relative_offset(unsigned int block_index) const {
        guarantee(state != state_reconstructing);
        guarantee(block_index < block_infos.size());
        return block_infos[block_index].relative_offset;
    }

    unsigned int block_index(int64_t offset) const {
        guarantee(state != state_reconstructing);
        guarantee(offset >= extent_ref.offset());
        guarantee(offset < extent_ref.offset() + UINT32_MAX);
        const uint32_t relative_offset = offset - extent_ref.offset();

        auto it = find_lower_bound_iter(relative_offset);
        guarantee(it != block_infos.end());
        return it - block_infos.begin();
    }

    bool new_offset(block_size_t block_size,
                    uint32_t *relative_offset_out,
                    unsigned int *block_index_out) {
        // Returns true if there's enough room at the end of the extent for the new
        // block.
        guarantee(state == state_active);
        guarantee(block_size.ser_value() <= parent->static_config->extent_size());

        uint32_t offset = back_relative_offset();
        guarantee(offset <= parent->static_config->extent_size());

        if (offset > parent->static_config->extent_size() - block_size.ser_value()) {
            return false;
        } else {
            *relative_offset_out = offset;
            *block_index_out = block_infos.size();
            block_infos.push_back(block_info_t{offset, block_size, false, false});
            update_stats(NULL, &block_infos.back());
            return true;
        }
    }

    static uint32_t aligned_value(block_size_t bs) {
        return ceil_aligned(bs.ser_value(), DEVICE_BLOCK_SIZE);
    }

    unsigned int num_blocks() const {
        guarantee(state != state_reconstructing);
        return block_infos.size();
    }

    unsigned int num_live_blocks() const {
        return num_live_blocks_stat;
    }

    bool all_garbage() const { return num_live_blocks() == 0; }

    uint32_t garbage_bytes() const {
        return garbage_bytes_stat;
    }

    bool block_is_garbage(unsigned int block_index) const {
        guarantee(state != state_reconstructing);
        guarantee(block_index < block_infos.size());
        return !block_infos[block_index].token_referenced && !block_infos[block_index].index_referenced;
    }

    void mark_live_tokenwise(unsigned int block_index) {
        guarantee(state != state_reconstructing);
        guarantee(block_index < block_infos.size());
        const block_info_t old_info = block_infos[block_index];
        block_infos[block_index].token_referenced = true;
        update_stats(&old_info, &block_infos[block_index]);
    }

    void mark_garbage_tokenwise(unsigned int block_index) {
        guarantee(state != state_reconstructing);
        guarantee(block_index < block_infos.size());
        const block_info_t old_info = block_infos[block_index];
        block_infos[block_index].token_referenced = false;
        update_stats(&old_info, &block_infos[block_index]);
    }

    uint32_t token_bytes() const {
        uint32_t b = 0;
        for (auto it = block_infos.begin(); it < block_infos.end(); ++it) {
            if (it->token_referenced) {
                b += aligned_value(it->block_size);
            }
        }
        return b;
    }

    static bool info_less(const block_info_t &info, uint32_t relative_offset) {
        return info.relative_offset < relative_offset;
    }

    std::vector<block_info_t>::const_iterator find_lower_bound_iter(uint32_t relative_offset) const {
        return std::lower_bound(block_infos.begin(), block_infos.end(), relative_offset, &gc_entry_t::info_less);
    }

    void mark_live_indexwise_with_offset(int64_t offset, block_size_t block_size) {
        guarantee(offset >= extent_ref.offset() && offset < extent_ref.offset() + UINT32_MAX);

        uint32_t relative_offset = offset - extent_ref.offset();

        auto it = find_lower_bound_iter(relative_offset);
        if (it == block_infos.end()) {
            block_infos.push_back(block_info_t{relative_offset, block_size, false, true});
            update_stats(NULL, &block_infos.back());
        } else if (it->relative_offset > relative_offset) {
            guarantee(it->relative_offset >= relative_offset + aligned_value(block_size));
            auto new_block = block_infos.insert(it, block_info_t{relative_offset, block_size, false, true});
            update_stats(NULL, &*new_block);
        } else {
            guarantee(it->relative_offset == relative_offset);
            guarantee(it->block_size == block_size);
            const block_info_t old_info = *it;
            it->index_referenced = true;
            update_stats(&old_info, &*it);
        }
    }

    void mark_garbage_indexwise(unsigned int block_index) {
        guarantee(state != state_reconstructing);
        guarantee(block_infos[block_index].index_referenced);
        const block_info_t old_info = block_infos[block_index];
        block_infos[block_index].index_referenced = false;
        update_stats(&old_info, &block_infos[block_index]);
    }

    bool block_referenced_by_index(unsigned int block_index) const {
        guarantee(block_index < block_infos.size());
        return block_infos[block_index].index_referenced;
    }

    uint32_t index_bytes() const {
        uint32_t b = 0;
        for (auto it = block_infos.begin(); it < block_infos.end(); ++it) {
            if (it->index_referenced) {
                b += aligned_value(it->block_size);
            }
        }
        return b;
    }

    void make_active() {
        guarantee(state == state_reconstructing);
        state = state_active;
    }

    std::string format_block_infos(const char *separator) const {
        const int64_t offset = extent_ref.offset();
        std::string ret;
        for (auto it = block_infos.begin(); it != block_infos.end(); ++it) {
            ret += strprintf("%s[%" PRIi64 "..+%" PRIu32 ") %c%c",
                             it == block_infos.begin() ? "" : separator,
                             offset + it->relative_offset, it->block_size.ser_value(),
                             it->token_referenced ? 'T' : ' ',
                             it->index_referenced ? 'I' : ' ');
        }
        return ret;
    }

private:
    // Private because we cannot guarantee that our stats remain consistent if somebody
    // gets a non-const iterator.
    std::vector<block_info_t>::iterator find_lower_bound_iter(uint32_t relative_offset) {
        return std::lower_bound(block_infos.begin(), block_infos.end(), relative_offset, &gc_entry_t::info_less);
    }

    // old_block can be NULL if a block_info was freshly added
    void update_stats(const block_info_t *old_block, const block_info_t *new_block) {
        rassert(new_block != NULL);
        if (old_block != NULL) {
            // Undo old_block
            if (old_block->token_referenced || old_block->index_referenced) {
                // Block is live
                num_live_blocks_stat -= 1;
                garbage_bytes_stat += aligned_value(old_block->block_size);
            }
        }
        // Apply new_block
        if (new_block->token_referenced || new_block->index_referenced) {
            // Block is live
            num_live_blocks_stat += 1;
            garbage_bytes_stat -= aligned_value(new_block->block_size);
        }
    }

    // Used by constructors.
    void add_self_to_parent_entries() {
        uint64_t extent_id = parent->static_config->extent_index(extent_offset);
        guarantee(parent->entries.get(extent_id) == NULL);
        parent->entries.set(extent_id, this);

        ++parent->stats->pm_serializer_data_extents;
    }


    data_block_manager_t *const parent;

public:
    extent_reference_t extent_ref;

    // When we started writing to the extent (this time).
    const microtime_t timestamp;

    // The PQ entry pointing to us.
    priority_queue_t<gc_entry_t *, gc_entry_less_t>::entry_t *our_pq_entry;

    // True iff the extent has been written to after starting up the serializer.
    bool was_written;

    enum state_t {
        // It has been, or is being, reconstructed from data on disk.
        state_reconstructing,
        // We are currently putting things on this extent. It is equal to
        // last_data_extent.
        state_active,
        // Not active, but not a GC candidate yet. It is in young_extent_queue.
        state_young,
        // Candidate to be GCed. It is in gc_pq.
        state_old,
        // Currently being GCed. It is equal to `current_entry` in one of `active_gcs`.
        state_in_gc
    } state;

private:
    // Block information, ordered by relative offset.
    std::vector<block_info_t> block_infos;

    // Some stats we maintain to make certain operations faster
    uint32_t garbage_bytes_stat;
    unsigned int num_live_blocks_stat;

    // Only to be used by the destructor, used to look up the gc entry in the
    // parent's entries array.
    const int64_t extent_offset;

    DISABLE_COPYING(gc_entry_t);
};

data_block_manager_t::data_block_manager_t(
        extent_manager_t *em, log_serializer_t *_serializer,
        const log_serializer_on_disk_static_config_t *_static_config,
        log_serializer_stats_t *_stats)
    : stats(_stats), shutdown_callback(NULL), state(state_unstarted),
      static_config(_static_config), extent_manager(em), serializer(_serializer),
      gc_stats(stats)
{
    rassert(static_config != NULL);
    rassert(extent_manager != NULL);
    rassert(serializer != NULL);
}

data_block_manager_t::~data_block_manager_t() {
    guarantee(state == state_unstarted || state == state_shut_down);
}

void data_block_manager_t::prepare_initial_metablock(data_block_manager::metablock_mixin_t *mb) {
    mb->active_extent = NULL_OFFSET;
}

void data_block_manager_t::start_reconstruct() {
    guarantee(state == state_unstarted);
}

// Marks the block at the given offset as alive, in the appropriate
// gc_entry_t in the entries table.  (This is used when we start up, when
// everything is presumed to be garbage, until we mark it as
// non-garbage.)
void data_block_manager_t::mark_live(int64_t offset, block_size_t ser_block_size) {
    uint64_t extent_id = static_config->extent_index(offset);

    if (entries.get(extent_id) == NULL) {
        guarantee(state == state_unstarted); // This is called at startup.

        gc_entry_t *entry = new gc_entry_t(this, extent_id * extent_manager->extent_size);
        reconstructed_extents.push_back(entry);
    }

    gc_entry_t *entry = entries.get(extent_id);
    entry->mark_live_indexwise_with_offset(offset, ser_block_size);
}

void data_block_manager_t::end_reconstruct() {
    guarantee(state == state_unstarted);
}

void data_block_manager_t::start_existing(file_t *file,
                                          data_block_manager::metablock_mixin_t *last_metablock) {
    guarantee(state == state_unstarted);
    dbfile = file;
    gc_io_account_nice.init(new file_account_t(file, GC_IO_PRIORITY_NICE));
    gc_io_account_high.init(new file_account_t(file, GC_IO_PRIORITY_HIGH));

    /* Reconstruct the active data block extents from the metablock. */
    const int64_t offset = last_metablock->active_extent;

    if (offset != NULL_OFFSET) {
        /* It is (perhaps) possible to have an active data block extent with no
           actual data blocks in it. In this case we would not have created a
           gc_entry_t for the extent yet. */
        if (entries.get(offset / extent_manager->extent_size) == NULL) {
            gc_entry_t *e = new gc_entry_t(this, offset);
            reconstructed_extents.push_back(e);
        }

        active_extent = entries.get(offset / extent_manager->extent_size);
        guarantee(active_extent != NULL);

        /* Turn the extent from a reconstructing extent into an active extent */
        guarantee(active_extent->state == gc_entry_t::state_reconstructing);
        reconstructed_extents.remove(active_extent);

        active_extent->make_active();
    } else {
        active_extent = NULL;
    }

    /* Convert any extents that we found live blocks in, but that are not active
    extents, into old extents */
    while (gc_entry_t *entry = reconstructed_extents.head()) {
        reconstructed_extents.remove(entry);

        guarantee(entry->state == gc_entry_t::state_reconstructing);
        entry->state = gc_entry_t::state_old;

        entry->our_pq_entry = gc_pq.push(entry);

        gc_stats.old_total_block_bytes += static_config->extent_size();
        gc_stats.old_garbage_block_bytes += entry->garbage_bytes();
    }

    state = state_ready;
}

// Computes an offset and end offset for the purposes of readahead.  Returns an interval
// that contains the [block_offset, block_offset + ser_block_size_in) interval that
// is also contained within a single extent.  boundaries is the extent's gc_entry_t's
// block_boundaries() value.  Outputs an interval that is _NOT_ fit to device block
// size boundaries.  This is used by read_ahead_offset_and_size.
void unaligned_read_ahead_interval(const int64_t block_offset,
                                   const uint32_t ser_block_size,
                                   const int64_t extent_size,
                                   const int64_t read_ahead_size,
                                   const std::vector<uint32_t> &boundaries,
                                   int64_t *const offset_out,
                                   int64_t *const end_offset_out) {
    guarantee(!boundaries.empty());
    guarantee(ser_block_size > 0);
    guarantee(boundaries.back() <= extent_size);

    const int64_t extent = floor_aligned(block_offset, extent_size);

    const int64_t raw_readahead_floor = floor_aligned(block_offset, read_ahead_size);
    const int64_t raw_readahead_ceil = raw_readahead_floor + read_ahead_size;

    // In case the extent size is configured to something weird, we cannot assume
    // that the readahead values are contained within the extent.
    const int64_t readahead_floor = std::max(raw_readahead_floor, extent);
    const int64_t readahead_ceil = std::min(raw_readahead_ceil, extent + extent_size);

    auto beg_it = std::lower_bound(boundaries.begin(), boundaries.end() - 1,
                                   readahead_floor - extent);
    auto end_it = std::lower_bound(boundaries.begin(), boundaries.end() - 1,
                                   readahead_ceil - extent);

    guarantee(beg_it < end_it);

    int64_t beg_ret = *beg_it + extent;
    int64_t end_ret = *end_it + extent;

    guarantee(beg_ret <= block_offset);

    // Since readahead_ceil > block_offset, and since the next block boundary is >=
    // block_offset + ser_block_size, we know end_ret >= block_offset +
    // ser_block_size.
    guarantee(end_ret >= block_offset + ser_block_size);
    guarantee(end_ret <= extent + extent_size);

    *offset_out = beg_ret;
    *end_offset_out = end_ret;
}

// Computes a device block size aligned interval to be read for the purposes of
// readahead.  Returns an interval that contains the [block_offset, block_offset +
// ser_block_size_in) interval.  boundaries is the extent's gc_entry_t's
// block_boundaries() value.
void read_ahead_interval(const int64_t block_offset,
                         const uint32_t ser_block_size,
                         const int64_t extent_size,
                         const int64_t read_ahead_size,
                         const int64_t device_block_size,
                         const std::vector<uint32_t> &boundaries,
                         int64_t *const offset_out,
                         int64_t *const end_offset_out) {
    int64_t unaligned_offset;
    int64_t unaligned_end_offset;
    unaligned_read_ahead_interval(block_offset, ser_block_size, extent_size,
                                  read_ahead_size, boundaries,
                                  &unaligned_offset, &unaligned_end_offset);

    *offset_out = floor_aligned(unaligned_offset, device_block_size);
    *end_offset_out = ceil_aligned(unaligned_end_offset, device_block_size);
}

void read_ahead_offset_and_size(int64_t off_in,
                                int64_t ser_block_size_in,
                                int64_t extent_size,
                                const std::vector<uint32_t> &boundaries,
                                int64_t *offset_out, int64_t *size_out) {
    int64_t offset;
    int64_t end_offset;
    read_ahead_interval(off_in, ser_block_size_in, extent_size,
                        APPROXIMATE_READ_AHEAD_SIZE,
                        DEVICE_BLOCK_SIZE,
                        boundaries,
                        &offset,
                        &end_offset);

    *offset_out = offset;
    *size_out = end_offset - offset;
}

class dbm_read_ahead_t {
public:
    static std::vector<uint32_t> get_boundaries(data_block_manager_t *parent,
                                                int64_t off_in) {
        const gc_entry_t *entry
            = parent->entries.get(off_in / parent->static_config->extent_size());
        guarantee(entry != NULL);

        return entry->block_boundaries();
    }

    static void perform_read_ahead(data_block_manager_t *const parent,
                                   const int64_t off_in,
                                   const uint32_t ser_block_size_in,
                                   void *const buf_out,
                                   file_account_t *const io_account,
                                   log_serializer_stats_t *const stats) {
        const std::vector<uint32_t> boundaries = get_boundaries(parent, off_in);

        int64_t read_ahead_offset;
        int64_t read_ahead_size;

        // Finish initialization.
        read_ahead_offset_and_size(off_in,
                                   ser_block_size_in,
                                   parent->static_config->extent_size(),
                                   boundaries,
                                   &read_ahead_offset,
                                   &read_ahead_size);

        scoped_aligned_malloc_t<char> read_ahead_buf(
                malloc_aligned<char>(read_ahead_size, DEVICE_BLOCK_SIZE));

        // Do the disk read!
        co_read(parent->dbfile, read_ahead_offset, read_ahead_size,
                read_ahead_buf.get(), io_account);

        stats->bytes_read(read_ahead_size);

        // Walk over the read ahead buffer and copy stuff...
        int64_t extent_offset = floor_aligned(read_ahead_offset,
                                              parent->static_config->extent_size());
        uint32_t relative_offset = read_ahead_offset - extent_offset;
        uint32_t relative_end_offset
            = (read_ahead_offset + read_ahead_size) - extent_offset;

        auto lower_it = std::lower_bound(boundaries.begin(), boundaries.end(),
                                         relative_offset);
        auto const upper_it = std::upper_bound(boundaries.begin(), boundaries.end(),
                                               relative_end_offset) - 1;

        bool handled_required_block = false;

        for (; lower_it < upper_it; ++lower_it) {
            const char *current_buf
                = read_ahead_buf.get() + (*lower_it - relative_offset);

            int64_t current_offset = *lower_it + extent_offset;

            if (current_offset == off_in) {
                guarantee(!handled_required_block);

                memcpy(buf_out, current_buf, ser_block_size_in);
                handled_required_block = true;
            } else {
                const block_id_t block_id
                    = reinterpret_cast<const ls_buf_data_t *>(current_buf)->block_id;

                const index_block_info_t info
                    = parent->serializer->lba_index->get_block_info(block_id);

                const bool block_is_live = info.offset.has_value() &&
                    current_offset == info.offset.get_value();

                if (!block_is_live) {
                    continue;
                }

                const block_size_t block_size = block_size_t::unsafe_make(info.ser_block_size);
                buf_ptr_t buf = buf_ptr_t::alloc_uninitialized(block_size);
                memcpy(buf.ser_buffer(), current_buf, info.ser_block_size);
                buf.fill_padding_zero();
                guarantee(info.ser_block_size <= *(lower_it + 1) - *lower_it);

                counted_t<ls_block_token_pointee_t> ls_token
                    = parent->serializer->generate_block_token(current_offset,
                                                               block_size);

                counted_t<standard_block_token_t> token
                    = to_standard_block_token(block_id, std::move(ls_token));

                parent->serializer->offer_buf_to_read_ahead_callbacks(
                        block_id,
                        std::move(buf),
                        token);
            }
        }

        guarantee(handled_required_block);
    }
};

bool data_block_manager_t::should_perform_read_ahead(int64_t offset) {
    uint64_t extent_id = static_config->extent_index(offset);

    gc_entry_t *entry = entries.get(extent_id);

    // If the extent was written, we don't perform read ahead because it would
    // a) be potentially useless and b) has an elevated risk of conflicting with
    // active writes on the io queue.
    return !entry->was_written && serializer->should_perform_read_ahead();
}

buf_ptr_t data_block_manager_t::read(int64_t off_in, block_size_t block_size,
                                   file_account_t *io_account) {
    guarantee(state == state_ready);
    if (should_perform_read_ahead(off_in)) {
        buf_ptr_t ret = buf_ptr_t::alloc_uninitialized(block_size);
        dbm_read_ahead_t::perform_read_ahead(this, off_in, block_size.ser_value(),
                                             ret.ser_buffer(), io_account, stats);
        // We have to fill the padding with zero, since only the first part of the
        // buf got memcpy'd into.
        ret.fill_padding_zero();
        return ret;
    } else {
        if (divides(DEVICE_BLOCK_SIZE, off_in)) {
            buf_ptr_t ret = buf_ptr_t::alloc_uninitialized(block_size);
            co_read(dbfile, off_in, ret.aligned_block_size(),
                    ret.ser_buffer(), io_account);
            stats->bytes_read(ret.aligned_block_size());
            // Blocks are written DEVICE_BLOCK_SIZE-aligned -- so the block on disk
            // should have been written with zero padding.
            ret.assert_padding_zero();
            return ret;
        } else {
            int64_t floor_off_in = floor_aligned(off_in, DEVICE_BLOCK_SIZE);
            int64_t ceil_off_end = ceil_aligned(off_in + block_size.ser_value(),
                                                DEVICE_BLOCK_SIZE);
            scoped_aligned_malloc_t<char> buf(malloc_aligned<char>(ceil_off_end - floor_off_in,
                                                     DEVICE_BLOCK_SIZE));
            co_read(dbfile, floor_off_in, ceil_off_end - floor_off_in,
                    buf.get(), io_account);

            buf_ptr_t ret = buf_ptr_t::alloc_uninitialized(block_size);
            memcpy(ret.ser_buffer(), buf.get() + (off_in - floor_off_in),
                   block_size.ser_value());
            stats->bytes_read(ret.aligned_block_size());
            // We have to fill the padding to zero, in this case.
            ret.fill_padding_zero();
            return ret;
        }
    }
}

std::vector<counted_t<ls_block_token_pointee_t> >
data_block_manager_t::many_writes(const std::vector<buf_write_info_t> &writes,
                                  file_account_t *io_account,
                                  iocallback_t *cb) {
    // These tokens are grouped by extent.  You can do a contiguous write in each
    // extent.
    std::vector<std::vector<counted_t<ls_block_token_pointee_t> > > token_groups
        = gimme_some_new_offsets(writes);

    for (auto it = writes.begin(); it != writes.end(); ++it) {
        it->buf->ser_header.block_id = it->block_id;
    }

    struct intermediate_cb_t : public iocallback_t {
        virtual void on_io_complete() {
            --ops_remaining;
            if (ops_remaining == 0) {
                iocallback_t *local_cb = cb;
                delete this;
                local_cb->on_io_complete();
            }
        }

        size_t ops_remaining;
        iocallback_t *cb;
    };

    intermediate_cb_t *const intermediate_cb = new intermediate_cb_t;
    // We add 1 for degenerate case where token_groups is empty -- we call
    // intermediate_cb->on_io_complete later.
    intermediate_cb->ops_remaining = token_groups.size() + 1;
    intermediate_cb->cb = cb;

    size_t write_number = 0;
    for (size_t i = 0; i < token_groups.size(); ++i) {

        const int64_t front_offset = token_groups[i].front()->offset();
        const int64_t back_offset = token_groups[i].back()->offset()
            + gc_entry_t::aligned_value(token_groups[i].back()->block_size());

        guarantee(divides(DEVICE_BLOCK_SIZE, front_offset));

        const int64_t write_size = back_offset - front_offset;

        scoped_array_t<iovec> iovecs(token_groups[i].size());

        int64_t last_written_offset = front_offset;
        size_t total_aligned_size = 0;

        for (size_t j = 0; j < token_groups[i].size(); ++j) {
            const int64_t j_offset = token_groups[i][j]->offset();
            const block_size_t j_block_size = token_groups[i][j]->block_size();
            guarantee(j_offset == last_written_offset);
            const size_t j_aligned_size = gc_entry_t::aligned_value(j_block_size);
            total_aligned_size += j_aligned_size;

            // The behavior of gimme_some_new_offsets is supposed to retain order, so
            // we expect writes[write_number] to have the currently-relevant write.
            guarantee(writes[write_number].block_size == j_block_size);

            iovecs[j].iov_base = writes[write_number].buf;
            iovecs[j].iov_len = j_aligned_size;
            last_written_offset = j_offset + j_aligned_size;

            ++write_number;
        }

        guarantee(last_written_offset == back_offset);

        dbfile->writev_async(front_offset, write_size,
                             std::move(iovecs), io_account, intermediate_cb);

        stats->bytes_written(total_aligned_size);
    }

    // Call on_io_complete for degenerate case (we added 1 to ops_remaining
    // earlier).
    intermediate_cb->on_io_complete();

    std::vector<counted_t<ls_block_token_pointee_t> > ret;
    ret.reserve(writes.size());
    for (auto it = token_groups.begin(); it != token_groups.end(); ++it) {
        for (auto jt = it->begin(); jt != it->end(); ++jt) {
            ret.push_back(std::move(*jt));
        }
    }

    return ret;
}

void data_block_manager_t::destroy_entry(gc_entry_t *entry) {
    rassert(entry != NULL);
    entry->destroy();
}

void data_block_manager_t::check_and_handle_empty_extent(uint64_t extent_id) {
    gc_entry_t *entry = entries.get(extent_id);
    if (!entry) {
        return; // The extent has already been deleted
    }

    if (entry->all_garbage() && entry->state != gc_entry_t::state_active) {
        /* Every block in the extent is now garbage. */
        switch (entry->state) {
            case gc_entry_t::state_reconstructing:
                unreachable("Marking something as garbage during startup.");

            case gc_entry_t::state_active:
                unreachable("We shouldn't have gotten here.");

            /* Remove from the young extent queue */
            case gc_entry_t::state_young:
                young_extent_queue.remove(entry);
                break;

            /* Remove from the priority queue */
            case gc_entry_t::state_old:
                gc_pq.remove(entry->our_pq_entry);
                gc_stats.old_total_block_bytes -= static_config->extent_size();
                gc_stats.old_garbage_block_bytes -= static_config->extent_size();
                break;

            /* Notify the GC that the extent got released during GC */
            case gc_entry_t::state_in_gc: {
                int num_matched = 0;
                for (gc_state_t *gc_state = active_gcs.head();
                     gc_state != NULL;
                     gc_state = active_gcs.next(gc_state)) {
                    if (gc_state->current_entry == entry) {
                        gc_state->current_entry = NULL;
                        ++num_matched;
#ifdef NDEBUG
                        // In release mode, terminate the loop as soon
                        // as we have found our entry.
                        // In debug, continue to ensure that there is *exactly*
                        // one match.
                        break;
#endif
                    }
                }
                guarantee(num_matched == 1);
            } break;
            default:
                unreachable();
        }

        destroy_entry(entry);

    } else if (entry->state == gc_entry_t::state_old) {
        entry->our_pq_entry->update();
    }
}

size_t data_block_manager_t::compute_gc_concurrency() const {
    // Ok, what we do here is the following:
    // As long as the GC ratio is not increasing, i.e. below GC_START_RATIO,
    // we only start 1 GC coroutine.
    // When it turns out that the GC ratio has increased since we have started
    // GCing, we linearly increase the number of concurrent GCs, until reaching
    // the maximum at GC_HIGH_RATIO.
    // (If the GC ratio still keeps growing at that point, there's probably
    // not much we can do. Unless we would be ok with throttling writes.)
    //
    // Also see `choose_gc_io_account()` for the second component in the automatic
    // GC scaling process.

    CT_ASSERT(GC_HIGH_RATIO > GC_START_RATIO);
    CT_ASSERT(GC_START_RATIO > GC_STOP_RATIO);

    const double gc_ratio = garbage_ratio();
    if (gc_ratio < GC_START_RATIO) {
        return 1;
    } else if (gc_ratio >= GC_HIGH_RATIO) {
        return MAX_CONCURRENT_GCS;
    } else {
        rassert(gc_ratio >= GC_START_RATIO);
        rassert(gc_ratio < GC_HIGH_RATIO);
        rassert(GC_HIGH_RATIO - GC_START_RATIO > 0.0);
        double linear_factor = (gc_ratio - GC_START_RATIO)
            / (GC_HIGH_RATIO - GC_START_RATIO);
        size_t total_concurrency =
            1 + static_cast<size_t>(linear_factor * MAX_CONCURRENT_GCS);
        // std::min to avoid rounding errors leading to illegal return values
        return std::min(total_concurrency, MAX_CONCURRENT_GCS);
    }
}

file_account_t *data_block_manager_t::choose_gc_io_account() {
    // Start going into high priority as soon as the garbage ratio is more than
    // GC_HIGH_RATIO.
    // The idea is that we use the nice i/o account whenever possible, except
    // if it proves insufficient to maintain an acceptable garbage ratio, in
    // which case we switch over to the high priority account until the situation
    // has improved.

    // Note that this means that we can end up oscillating between both accounts,
    // which is fine.
    if (garbage_ratio() > GC_HIGH_RATIO) {
        return gc_io_account_high.get();
    } else {
        return gc_io_account_nice.get();
    }
}

void data_block_manager_t::mark_garbage(int64_t offset, extent_transaction_t *txn) {
    uint64_t extent_id = static_config->extent_index(offset);
    gc_entry_t *entry = entries.get(extent_id);
    unsigned int block_index = entry->block_index(offset);

    // Now we set the i_array entry to zero.  We make an extra reference to the
    // extent which gets held until we commit the transaction.
    txn->push_extent(extent_manager->copy_extent_reference(entry->extent_ref));

    guarantee(!entry->block_is_garbage(block_index));
    entry->mark_garbage_indexwise(block_index);

    // Add to old garbage count if necessary (works because of the
    // !entry->block_is_garbage(block_index) assertion above).
    if (entry->state == gc_entry_t::state_old && entry->block_is_garbage(block_index)) {
        gc_stats.old_garbage_block_bytes += gc_entry_t::aligned_value(entry->block_size(block_index));
    }

    check_and_handle_empty_extent(extent_id);
}

void data_block_manager_t::mark_live_tokenwise_with_offset(int64_t offset) {
    uint64_t extent_id = static_config->extent_index(offset);
    gc_entry_t *entry = entries.get(extent_id);
    rassert(entry != NULL);
    unsigned int block_index = entry->block_index(offset);

    entry->mark_live_tokenwise(block_index);
}

void data_block_manager_t::mark_garbage_tokenwise_with_offset(int64_t offset) {
    uint64_t extent_id = static_config->extent_index(offset);
    gc_entry_t *entry = entries.get(extent_id);

    rassert(entry != NULL);

    unsigned int block_index = entry->block_index(offset);

    guarantee(!entry->block_is_garbage(block_index));

    entry->mark_garbage_tokenwise(block_index);

    // Add to old garbage count if necessary (works because of the
    // !entry->block_is_garbage(block_index) assertion above).
    if (entry->state == gc_entry_t::state_old && entry->block_is_garbage(block_index)) {
        gc_stats.old_garbage_block_bytes += gc_entry_t::aligned_value(entry->block_size(block_index));
    }

    check_and_handle_empty_extent(extent_id);
}

void data_block_manager_t::start_gc() {
    if (state != state_ready) {
        // Don't run GC while we are still starting up. We might still be
        // reconstructing extent information.
        // Also don't run it when we're shutting down, since that would just make
        // the shut down take longer.
        return;
    }

    const size_t goal_num_active_gcs = compute_gc_concurrency();
    while (active_gcs.size() < goal_num_active_gcs) {
        gc_state_t *new_gc_state = new gc_state_t();
        active_gcs.push_back(new_gc_state);
        coro_t::spawn_sometime(std::bind(&data_block_manager_t::run_gc, this,
                                         new_gc_state));
    }
}

struct block_write_cond_t : public cond_t, public iocallback_t {
    void on_io_complete() {
        pulse();
    }
};

void data_block_manager_t::run_gc(gc_state_t *gc_state) {
    while (!gc_pq.empty()
           && should_we_keep_gcing()
           && !should_terminate_one_gc_thread()) {
        gc_one_extent(gc_state);

        if (state == state_shutting_down) {
            active_gcs.remove(gc_state);
            delete gc_state;
            if (active_gcs.empty()) {
                actually_shutdown();
            }
            return;
        }
    }

    active_gcs.remove(gc_state);
    delete gc_state;
}

void data_block_manager_t::gc_one_extent(gc_state_t *gc_state) {
    // A buffer for blocks we're transferring.
    scoped_aligned_malloc_t<char> gc_blocks;
    size_t total_bytes_read = 0;

    // A helper for waiting for all reads to finish
    struct gc_read_cb_t : public cond_t, public iocallback_t {
        gc_read_cb_t() : refcount(0) { }
        void on_io_complete() {
            guarantee(refcount > 0);
            refcount--;
            if (refcount == 0) {
                pulse();
            }
        }
        // Number of outstanding requests
        int refcount;
    };
    gc_read_cb_t read_cb;

    // 1: Grab an entry and read the data
    {
        ASSERT_NO_CORO_WAITING;

        ++stats->pm_serializer_data_extents_gced;

        /* grab the entry */
        guarantee (!gc_pq.empty());
        guarantee(gc_state->current_entry == NULL);
        gc_state->current_entry = gc_pq.pop();
        gc_state->current_entry->our_pq_entry = NULL;

        guarantee(gc_state->current_entry->state == gc_entry_t::state_old);
        gc_state->current_entry->state = gc_entry_t::state_in_gc;
        gc_stats.old_garbage_block_bytes -= gc_state->current_entry->garbage_bytes();
        gc_stats.old_total_block_bytes -= static_config->extent_size();

        /* read all the live data into buffers */

        // read_async can call the callback immediately, which
        // will then decrement the refcount and pulse the condition before
        // we have even issued all the reads we need.
        // We increment the refcount once, and then call `on_io_complete()`
        // once manually when we have issued all reads.
        read_cb.refcount++;

        gc_blocks = malloc_aligned<char>(extent_manager->extent_size, DEVICE_BLOCK_SIZE);

        // We're going to send as few discrete reads as possible, minimizing
        // disk->CPU bandwidth usage, instead of simply reading the entire
        // extent.

        uint32_t current_interval_begin = 0;
        uint32_t current_interval_end = 0;

        const int64_t extent_offset = gc_state->current_entry->extent_ref.offset();

        for (unsigned int i = 0, bpe = gc_state->current_entry->num_blocks();
             i < bpe;
             ++i) {
            if (!gc_state->current_entry->block_is_garbage(i)) {
                const uint32_t beg = gc_state->current_entry->relative_offset(i);
                rassert(divides(DEVICE_BLOCK_SIZE, beg));

                const uint32_t end
                    = gc_state->current_entry->relative_offset(i)
                    + gc_entry_t::aligned_value(gc_state->current_entry->block_size(i));

                if (beg <= current_interval_end) {
                    current_interval_end = end;
                } else {
                    if (current_interval_end > current_interval_begin) {
                        read_cb.refcount++;
                        dbfile->read_async(
                                extent_offset + current_interval_begin,
                                current_interval_end - current_interval_begin,
                                gc_blocks.get() + current_interval_begin,
                                choose_gc_io_account(),
                                &read_cb);
                        total_bytes_read += current_interval_end - current_interval_end;
                    }

                    current_interval_begin = beg;
                    current_interval_end = end;
                }
            }
        }

        guarantee(current_interval_begin < current_interval_end);

        read_cb.refcount++;
        dbfile->read_async(
                extent_offset + current_interval_begin,
                current_interval_end - current_interval_begin,
                gc_blocks.get() + current_interval_begin,
                choose_gc_io_account(),
                &read_cb);
        total_bytes_read += current_interval_end - current_interval_end;

        // Ok, all reads have been issued. Call `on_io_complete()` once to allow
        // `read_cb` to be pulsed (see comment above).
        read_cb.on_io_complete();
    }

    /* Wait for the reads to finish */
    read_cb.wait_lazily_unordered();
    stats->bytes_read(total_bytes_read);

    /* If other forces cause all of the blocks in the extent to become
    garbage before we even finish GCing it, they will set current_entry
    to NULL. */
    if (gc_state->current_entry == NULL) {
        return;
    }

    // 2: Rewrite the blocks that are still live
    std::vector<gc_write_t> gc_writes;
    {
        ASSERT_NO_CORO_WAITING;

        const size_t num_writes = gc_state->current_entry->num_live_blocks();

        gc_writes.reserve(num_writes);
        for (unsigned int i = 0, iend = gc_state->current_entry->num_blocks(); i < iend; ++i) {

            /* We re-check the bit array here in case a write came in for one
            of the blocks we are GCing. We wouldn't want to overwrite the new
            valid data with out-of-date data. */
            if (gc_state->current_entry->block_is_garbage(i)) {
                continue;
            }

            ser_buffer_t *block = reinterpret_cast<ser_buffer_t *>(gc_blocks.get()
                + gc_state->current_entry->relative_offset(i));
            const int64_t block_offset = gc_state->current_entry->extent_ref.offset()
                + gc_state->current_entry->relative_offset(i);

            gc_writes.push_back(gc_write_t(block, block_offset,
                                           gc_state->current_entry->block_size(i)));
        }
        guarantee(gc_writes.size() == num_writes);
    }
    write_gcs(gc_writes, gc_state);

    /* We need to do this here so that we don't
    get stuck on the GC treadmill */
    mark_unyoung_entries();

    /* Our write should have forced all of the blocks in the extent to
    become garbage, which should have caused the extent to be released
    and gc_state.current_entry to become NULL. */
    guarantee(gc_state->current_entry == NULL,
              "%p: %" PRIu32 " garbage bytes left on the extent, %" PRIu32
              " index-referenced bytes, %" PRIu32
              " token-referenced bytes, at offset %" PRIi64
              ".  block dump:\n%s\n",
              this,
              gc_state->current_entry->garbage_bytes(),
              gc_state->current_entry->index_bytes(),
              gc_state->current_entry->token_bytes(),
              gc_state->current_entry->extent_ref.offset(),
              gc_state->current_entry->format_block_infos("\n").c_str());
}

void data_block_manager_t::write_gcs(const std::vector<gc_write_t> &writes,
                                     gc_state_t *gc_state) {
    guarantee(gc_state->current_entry != NULL);

    block_write_cond_t block_write_cond;

    // We acquire block tokens for all the blocks before writing new
    // version.  The point of this is to make sure the _new_ block is
    // correctly "alive" when we write it.

    std::vector<counted_t<ls_block_token_pointee_t> > old_block_tokens;
    old_block_tokens.reserve(writes.size());

    // New block tokens, to hold the return value of
    // data_block_manager_t::write() instead of immediately discarding the
    // created token and causing the extent or block to be collected.
    std::vector<counted_t<ls_block_token_pointee_t> > new_block_tokens;

    {
        // Step 1: Write buffers to disk and assemble index operations
        ASSERT_NO_CORO_WAITING;

        std::vector<buf_write_info_t> the_writes;
        the_writes.reserve(writes.size());
        for (size_t i = 0; i < writes.size(); ++i) {
            old_block_tokens.push_back(serializer->generate_block_token(writes[i].old_offset,
                                                                        writes[i].block_size));

            the_writes.push_back(buf_write_info_t(writes[i].buf,
                                                  writes[i].block_size,
                                                  writes[i].buf->ser_header.block_id));
        }

        new_block_tokens = many_writes(the_writes, choose_gc_io_account(),
                                       &block_write_cond);

        guarantee(new_block_tokens.size() == writes.size());
    }

    // Step 2: Wait on all writes to finish
    block_write_cond.wait();

    // We created block tokens for our blocks we're writing, so
    // there's no way the current entry could have become NULL.
    guarantee(gc_state->current_entry != NULL);

    std::vector<index_write_op_t> index_write_ops;

    // Step 3: Figure out index ops.  It's important that we do this
    // now, right before the index_write, so that the updates to the
    // index are done atomically.
    {
        ASSERT_NO_CORO_WAITING;

        for (size_t i = 0; i < writes.size(); ++i) {
            unsigned int block_index
                = gc_state->current_entry->block_index(writes[i].old_offset);

            if (gc_state->current_entry->block_referenced_by_index(block_index)) {
                block_id_t block_id = writes[i].buf->ser_header.block_id;

                index_write_ops.push_back(
                        index_write_op_t(block_id,
                                         to_standard_block_token(
                                                 block_id,
                                                 new_block_tokens[i])));
            }

            // (If we don't have an i_array entry, the block is referenced
            // by a non-negative number of tokens only.  These get tokens
            // remapped later.)
        }

        // Step 4A: Remap tokens to new offsets.  It is important
        // that we do this _before_ calling index_write.
        // Otherwise, the token_offset map would still point to
        // the extent we're gcing.  Then somebody could do an
        // index_write after our index_write starts but before it
        // returns in Step 4 below, resulting in i_array entries
        // that point to the current entry.  This should empty out
        // all the t_array bits.
        for (size_t i = 0; i < writes.size(); ++i) {
            serializer->remap_block_to_new_offset(writes[i].old_offset,
                                                  new_block_tokens[i]->offset());
        }

        // Step 4A-2: Now that the block tokens have been remapped
        // to a new offset, destroying these tokens will update
        // the bits in the t_array of the new offset (if they're
        // the last token).
        old_block_tokens.clear();
        new_block_tokens.clear();
    }

    // Step 4B: Commit the transaction to the serializer, emptying
    // out all the i_array bits.
    new_mutex_in_line_t dummy_acq;
    serializer->index_write(&dummy_acq, index_write_ops);
}

void data_block_manager_t::prepare_metablock(data_block_manager::metablock_mixin_t *metablock) {
    guarantee(state == state_ready || state == state_shutting_down);

    if (active_extent != NULL) {
        metablock->active_extent = active_extent->extent_ref.offset();
    } else {
        metablock->active_extent = NULL_OFFSET;
    }
}

bool data_block_manager_t::shutdown(data_block_manager::shutdown_callback_t *cb) {
    rassert(cb != NULL);
    guarantee(state == state_ready);
    state = state_shutting_down;

    if (!active_gcs.empty()) {
        shutdown_callback = cb;
        return false;
    } else {
        shutdown_callback = NULL;
        actually_shutdown();
        return true;
    }
}

void data_block_manager_t::actually_shutdown() {
    guarantee(state == state_shutting_down);
    state = state_shut_down;

    guarantee(reconstructed_extents.head() == NULL);

    if (active_extent != NULL) {
        UNUSED int64_t extent = active_extent->extent_ref.release();
        delete active_extent;
        active_extent = NULL;
    }

    while (gc_entry_t *entry = young_extent_queue.head()) {
        young_extent_queue.remove(entry);
        UNUSED int64_t extent = entry->extent_ref.release();
        delete entry;
    }

    while (!gc_pq.empty()) {
        gc_entry_t *entry = gc_pq.pop();
        UNUSED int64_t extent = entry->extent_ref.release();
        delete entry;
    }

    if (shutdown_callback) {
        shutdown_callback->on_datablock_manager_shutdown();
    }
}

std::vector<std::vector<counted_t<ls_block_token_pointee_t> > >
data_block_manager_t::gimme_some_new_offsets(const std::vector<buf_write_info_t> &writes) {
    ASSERT_NO_CORO_WAITING;

    // Start a new extent if necessary.
    if (active_extent == NULL) {
        active_extent = new gc_entry_t(this);
        ++stats->pm_serializer_data_extents_allocated;
    }


    guarantee(active_extent->state == gc_entry_t::state_active);

    std::vector<std::vector<counted_t<ls_block_token_pointee_t> > > ret;

    std::vector<counted_t<ls_block_token_pointee_t> > tokens;
    for (auto it = writes.begin(); it != writes.end(); ++it) {
        uint32_t relative_offset = valgrind_undefined<uint32_t>(UINT32_MAX);
        unsigned int block_index = valgrind_undefined<unsigned int>(UINT_MAX);
        if (!active_extent->new_offset(it->block_size,
                                       &relative_offset, &block_index)) {
            // Move the active_extent gc_entry_t to the young extent queue (if it's
            // not already empty), and make a new gc_entry_t.
            if (active_extent->num_live_blocks() == 0) {
                gc_entry_t *old_active_extent = active_extent;
                active_extent = new gc_entry_t(this);
                destroy_entry(old_active_extent);
            } else {
                active_extent->state = gc_entry_t::state_young;
                young_extent_queue.push_back(active_extent);
                mark_unyoung_entries();
                active_extent = new gc_entry_t(this);
            }

            ++stats->pm_serializer_data_extents_allocated;
            const bool succeeded = active_extent->new_offset(it->block_size,
                                                             &relative_offset,
                                                             &block_index);
            guarantee(succeeded);

            // Push the current group of tokens, if it's nonempty, onto the return vector.
            if (!tokens.empty()) {
                ret.push_back(std::move(tokens));
                tokens.clear();
            }
        }

        const int64_t offset = active_extent->extent_ref.offset() + relative_offset;
        active_extent->was_written = true;
        active_extent->mark_live_tokenwise(block_index);

        tokens.push_back(serializer->generate_block_token(offset, it->block_size));
    }

    if (!tokens.empty()) {
        ret.push_back(std::move(tokens));
    }

    return ret;
}

bool data_block_manager_t::is_gc_active() const {
    return !active_gcs.empty();
}

// Looks at young_extent_queue and pops things off the queue that are
// no longer deemed young, putting them on the priority queue.
void data_block_manager_t::mark_unyoung_entries() {
    ASSERT_NO_CORO_WAITING;
    while (young_extent_queue.size() > GC_YOUNG_EXTENT_MAX_SIZE) {
        remove_last_unyoung_entry();
    }

    uint64_t current_time = current_microtime();

    while (young_extent_queue.head()
           && current_time - young_extent_queue.head()->timestamp > GC_YOUNG_EXTENT_TIMELIMIT_MICROS) {
        remove_last_unyoung_entry();
    }
}

// Pops young_extent_queue and puts it on the priority queue.
// Assumes young_extent_queue is not empty.
void data_block_manager_t::remove_last_unyoung_entry() {
    ASSERT_NO_CORO_WAITING;
    gc_entry_t *entry = young_extent_queue.head();
    young_extent_queue.remove(entry);

    guarantee(entry->state == gc_entry_t::state_young);
    entry->state = gc_entry_t::state_old;

    entry->our_pq_entry = gc_pq.push(entry);

    gc_stats.old_total_block_bytes += static_config->extent_size();
    gc_stats.old_garbage_block_bytes += entry->garbage_bytes();
}

/* functions for gc structures */

// Answers the following question: We're in the middle of gc'ing, and
// look, it's the next largest entry.  Should we keep gc'ing?  Returns
// false when the garbage ratio is lower than GC_STOP_RATIO.
bool data_block_manager_t::should_we_keep_gcing() const {
    return garbage_ratio() > GC_STOP_RATIO;
}

bool data_block_manager_t::should_terminate_one_gc_thread() const {
    const size_t goal_num_active_gcs = compute_gc_concurrency();
    return active_gcs.size() > goal_num_active_gcs;
}

// Answers the following question: Do we want to bother gc'ing?
// Returns true when our garbage_ratio is greater than
// GC_THRESHOLD_RATIO_*.
bool data_block_manager_t::do_we_want_to_start_gcing() const {
    return garbage_ratio() > GC_START_RATIO;
}

bool gc_entry_less_t::operator()(const gc_entry_t *x, const gc_entry_t *y) {
    return x->garbage_bytes() < y->garbage_bytes();
}

/****************
 *Stat functions*
 ****************/

double data_block_manager_t::garbage_ratio() const {
    if (gc_stats.old_total_block_bytes.get() == 0) {
        return 0.0;
    } else {
        double old_garbage = gc_stats.old_garbage_block_bytes.get();
        double old_total = gc_stats.old_total_block_bytes.get();
        return old_garbage / (old_total + extent_manager->held_extents() * static_config->extent_size());
    }
}

void data_block_manager_t::gc_stat_t::operator+=(int64_t num) {
    val += num;
    *perfmon += num;
}

void data_block_manager_t::gc_stat_t::operator-=(int64_t num) {
    val -= num;
    *perfmon -= num;
}

data_block_manager_t::gc_stats_t::gc_stats_t(log_serializer_stats_t *_stats)
    : old_total_block_bytes(&_stats->pm_serializer_old_total_block_bytes),
      old_garbage_block_bytes(&_stats->pm_serializer_old_garbage_block_bytes) { }
