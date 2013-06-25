// Copyright 2010-2013 RethinkDB, all rights reserved.
#define __STDC_FORMAT_MACROS
#include "serializer/log/data_block_manager.hpp"

#include "utils.hpp"
#include <boost/bind.hpp>

#include "arch/arch.hpp"
#include "arch/runtime/coroutines.hpp"
#include "concurrency/mutex.hpp"
#include "perfmon/perfmon.hpp"
#include "serializer/log/log_serializer.hpp"
#include "stl_utils.hpp"

// Max amount of bytes which can be read ahead in one i/o transaction (if enabled)
const int64_t APPROXIMATE_READ_AHEAD_SIZE = 32 * DEFAULT_BTREE_BLOCK_SIZE;

// TODO: Right now we perform garbage collection via the do_write() interface on the
// log_serializer_t. This leads to bugs in a couple of ways:
//
// 1. We have to be sure to get the metadata (repli timestamp, delete bit) right. The
//    data block manager shouldn't have to know about that stuff.
//
// 2. We have to special-case the serializer so that it allows us to submit
//    do_write()s during shutdown. If there were an alternative interface, it could
//    ignore or refuse our GC requests when it is shutting down.

// Later, rewrite this so that we have a special interface through which to order
// garbage collection.

data_block_manager_t::data_block_manager_t(const log_serializer_dynamic_config_t *_dynamic_config, extent_manager_t *em, log_serializer_t *_serializer, const log_serializer_on_disk_static_config_t *_static_config, log_serializer_stats_t *_stats)
    : stats(_stats), shutdown_callback(NULL), state(state_unstarted), dynamic_config(_dynamic_config),
      static_config(_static_config), extent_manager(em), serializer(_serializer),
      next_active_extent(0), gc_state(), gc_stats(stats)
{
    rassert(dynamic_config != NULL);
    rassert(static_config != NULL);
    rassert(extent_manager != NULL);
    rassert(serializer != NULL);
}

data_block_manager_t::~data_block_manager_t() {
    guarantee(state == state_unstarted || state == state_shut_down);
}

void data_block_manager_t::prepare_initial_metablock(metablock_mixin_t *mb) {
    for (int i = 0; i < MAX_ACTIVE_DATA_EXTENTS; i++) {
        mb->active_extents[i] = NULL_OFFSET;
        mb->blocks_in_active_extent[i] = 0;
    }
}

void data_block_manager_t::start_reconstruct() {
    guarantee(state == state_unstarted);
    gc_state.set_step(gc_reconstruct);
}

// Marks the block at the given offset as alive, in the appropriate
// gc_entry_t in the entries table.  (This is used when we start up, when
// everything is presumed to be garbage, until we mark it as
// non-garbage.)
void data_block_manager_t::mark_live(int64_t offset, uint32_t ser_block_size) {
    int extent_id = static_config->extent_index(offset);

    if (entries.get(extent_id) == NULL) {
        guarantee(gc_state.step() == gc_reconstruct);  // This is called at startup.

        gc_entry_t *entry = new gc_entry_t(this, extent_id * extent_manager->extent_size);
        entry->state = gc_entry_t::state_reconstructing;
        reconstructed_extents.push_back(entry);
    }

    gc_entry_t *entry = entries.get(extent_id);
    entry->mark_live_indexwise_with_offset(offset, ser_block_size);
}

void data_block_manager_t::end_reconstruct() {
    guarantee(state == state_unstarted);
    guarantee(gc_state.step() == gc_reconstruct);
    gc_state.set_step(gc_ready);
}

void data_block_manager_t::start_existing(file_t *file, metablock_mixin_t *last_metablock) {
    guarantee(state == state_unstarted);
    dbfile = file;
    gc_io_account_nice.init(new file_account_t(file, GC_IO_PRIORITY_NICE));
    gc_io_account_high.init(new file_account_t(file, GC_IO_PRIORITY_HIGH));

    /* Reconstruct the active data block extents from the metablock. */
    for (unsigned int i = 0; i < MAX_ACTIVE_DATA_EXTENTS; i++) {
        int64_t offset = last_metablock->active_extents[i];

        if (offset != NULL_OFFSET) {
            /* It is possible to have an active data block extent with no actual data
            blocks in it. In this case we would not have created a gc_entry_t for the
            extent yet. */
            if (entries.get(offset / extent_manager->extent_size) == NULL) {
                gc_entry_t *e = new gc_entry_t(this, offset);
                e->state = gc_entry_t::state_reconstructing;
                reconstructed_extents.push_back(e);
            }

            active_extents[i] = entries.get(offset / extent_manager->extent_size);
            rassert(active_extents[i]);

            /* Turn the extent from a reconstructing extent into an active extent */
            guarantee(active_extents[i]->state == gc_entry_t::state_reconstructing);
            active_extents[i]->state = gc_entry_t::state_active;
            reconstructed_extents.remove(active_extents[i]);

            blocks_in_active_extent[i] = last_metablock->blocks_in_active_extent[i];
        } else {
            active_extents[i] = NULL;
        }
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
                                   file_account_t *const io_account) {
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

        scoped_malloc_t<char> read_ahead_buf(
                malloc_aligned(read_ahead_size, DEVICE_BLOCK_SIZE));

        // Do the disk read!
        co_read(parent->dbfile, read_ahead_offset, read_ahead_size,
                read_ahead_buf.get(), io_account);

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
                // RSI: Remove block id from block metadata, just put a
                // reverse-lookup table in the gc entries, initialized from the LBA.
                const block_id_t block_id
                    = reinterpret_cast<const ls_buf_data_t *>(current_buf)->block_id;

                const index_block_info_t info
                    = parent->serializer->lba_index->get_block_info(block_id);

                const bool block_is_live = info.offset.has_value() &&
                    current_offset == info.offset.get_value();

                if (!block_is_live) {
                    continue;
                }

                scoped_malloc_t<ser_buffer_t> data = parent->serializer->malloc();
                memcpy(data.get(), current_buf, info.ser_block_size);
                guarantee(info.ser_block_size <= *(lower_it + 1) - *lower_it);

                counted_t<ls_block_token_pointee_t> ls_token
                    = parent->serializer->generate_block_token(current_offset,
                                                               info.ser_block_size);

                counted_t<standard_block_token_t> token
                    = to_standard_block_token(block_id, ls_token);

                parent->serializer->offer_buf_to_read_ahead_callbacks(
                        block_id,
                        std::move(data),
                        block_size_t::unsafe_make(info.ser_block_size),
                        token,
                        info.recency);
            }
        }

        guarantee(handled_required_block);
    }
};


bool data_block_manager_t::should_perform_read_ahead(int64_t offset) {
    unsigned int extent_id = static_config->extent_index(offset);

    gc_entry_t *entry = entries.get(extent_id);

    // If the extent was written, we don't perform read ahead because it would
    // a) be potentially useless and b) has an elevated risk of conflicting with
    // active writes on the io queue.
    return !entry->was_written && serializer->should_perform_read_ahead();
}

void data_block_manager_t::read(int64_t off_in, uint32_t ser_block_size_in,
                                void *buf_out, file_account_t *io_account) {
    guarantee(state == state_ready);
    if (should_perform_read_ahead(off_in)) {
        dbm_read_ahead_t::perform_read_ahead(this, off_in, ser_block_size_in,
                                             buf_out, io_account);
    } else {
        if (divides(DEVICE_BLOCK_SIZE, reinterpret_cast<intptr_t>(buf_out)) &&
            divides(DEVICE_BLOCK_SIZE, off_in) &&
            divides(DEVICE_BLOCK_SIZE, ser_block_size_in)) {
            co_read(dbfile, off_in, ser_block_size_in, buf_out, io_account);
        } else {
            int64_t floor_off_in = floor_aligned(off_in, DEVICE_BLOCK_SIZE);
            int64_t ceil_off_end = ceil_aligned(off_in + ser_block_size_in,
                                                DEVICE_BLOCK_SIZE);
            scoped_malloc_t<char> buf(malloc_aligned(ceil_off_end - floor_off_in,
                                                     DEVICE_BLOCK_SIZE));
            co_read(dbfile, floor_off_in, ceil_off_end - floor_off_in,
                    buf.get(), io_account);

            memcpy(buf_out, buf.get() + (off_in - floor_off_in), ser_block_size_in);
        }
    }
}

/*
 Instead of wrapping this into a coroutine, we are still using a callback as we want
 to be able to spawn a lot of writes in parallel. Having to spawn a coroutine for
 each of them would involve a major memory overhead for all the stacks.  Also this
 makes stuff easier in other places, as we get the offset as well as a freshly set
 block_sequence_id immediately.
 */
counted_t<ls_block_token_pointee_t>
data_block_manager_t::write(const void *buf_in, block_id_t block_id,
                            bool assign_new_block_sequence_id,
                            file_account_t *io_account, iocallback_t *cb) {
    // Either we're ready to write, or we're shutting down and just
    // finished reading blocks for gc and called do_write.
    guarantee(state == state_ready
           || (state == state_shutting_down && gc_state.step() == gc_write));

    counted_t<ls_block_token_pointee_t> token = gimme_a_new_offset();

    ++stats->pm_serializer_data_blocks_written;

    ls_buf_data_t *data = const_cast<ls_buf_data_t *>(reinterpret_cast<const ls_buf_data_t *>(buf_in) - 1);
    data->block_id = block_id;
    if (assign_new_block_sequence_id) {
        data->block_sequence_id = ++serializer->latest_block_sequence_id;
    }

    dbfile->write_async(token->offset(), static_config->block_size().ser_value(), data, io_account, cb, file_t::NO_DATASYNCS);

    return token;
}

void data_block_manager_t::check_and_handle_empty_extent(unsigned int extent_id) {
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
            case gc_entry_t::state_in_gc:
                guarantee(gc_state.current_entry == entry);
                gc_state.current_entry = NULL;
                break;
            default:
                unreachable();
        }

        ++stats->pm_serializer_data_extents_reclaimed;
        entry->destroy();
        rassert(entries.get(extent_id) == NULL);

    } else if (entry->state == gc_entry_t::state_old) {
        entry->our_pq_entry->update();
    }
}

file_account_t *data_block_manager_t::choose_gc_io_account() {
    // Start going into high priority as soon as the garbage ratio is more than
    // 2% above the configured goal.
    // The idea is that we use the nice i/o account whenever possible, except
    // if it proves insufficient to maintain an acceptable garbage ratio, in
    // which case we switch over to the high priority account until the situation
    // has improved.

    // This means that we can end up oscillating between both accounts, which
    // is probably fine. TODO: Make sure it actually is in practice!
    if (garbage_ratio() > dynamic_config->gc_high_ratio * 1.02) {
        return gc_io_account_high.get();
    } else {
        return gc_io_account_nice.get();
    }
}

void data_block_manager_t::mark_garbage(int64_t offset, extent_transaction_t *txn) {
    unsigned int extent_id = static_config->extent_index(offset);
    gc_entry_t *entry = entries.get(extent_id);
    unsigned int block_index = entry->block_index(offset);

    // Now we set the i_array entry to zero.  We make an extra reference to the
    // extent which gets held until we commit the transaction.
    txn->push_extent(extent_manager->copy_extent_reference(entry->extent_ref));
    entry->mark_garbage_indexwise(block_index);

    // Add to old garbage count if we have toggled the g_array bit (works because of
    // the g_array[block_index] == 0 assertion above)
    if (entry->state == gc_entry_t::state_old && entry->block_is_garbage(block_index)) {
        gc_stats.old_garbage_block_bytes += entry->block_size(block_index);
    }

    check_and_handle_empty_extent(extent_id);
}

void data_block_manager_t::mark_live_tokenwise(int64_t offset) {
    unsigned int extent_id = static_config->extent_index(offset);
    gc_entry_t *entry = entries.get(extent_id);
    rassert(entry != NULL);
    unsigned int block_index = entry->block_index(offset);

    entry->mark_live_tokenwise(block_index);
}

void data_block_manager_t::mark_garbage_tokenwise(int64_t offset) {
    unsigned int extent_id = static_config->extent_index(offset);
    gc_entry_t *entry = entries.get(extent_id);

    rassert(entry != NULL);

    unsigned int block_index = entry->block_index(offset);

    guarantee(!entry->block_is_garbage(block_index));

    entry->mark_garbage_tokenwise(block_index);

    // Add to old garbage count if we have toggled the g_array bit (works because of
    // the g_array[block_index] == 0 assertion above)
    if (entry->state == gc_entry_t::state_old && entry->block_is_garbage(block_index)) {
        gc_stats.old_garbage_block_bytes += entry->block_size(block_index);
    }

    check_and_handle_empty_extent(extent_id);
}

void data_block_manager_t::start_gc() {
    if (gc_state.step() == gc_ready) run_gc();
}

data_block_manager_t::gc_writer_t::gc_writer_t(gc_write_t *writes, int num_writes, data_block_manager_t *_parent)
    : done(num_writes == 0), parent(_parent) {
    if (!done) {
        coro_t::spawn(boost::bind(&data_block_manager_t::gc_writer_t::write_gcs, this, writes, num_writes));
    }
}

struct block_write_cond_t : public cond_t, public iocallback_t {
    void on_io_complete() {
        pulse();
    }
};
void data_block_manager_t::gc_writer_t::write_gcs(gc_write_t *writes, int num_writes) {
    if (parent->gc_state.current_entry != NULL) {
        std::vector<block_write_cond_t *> block_write_conds;
        block_write_conds.reserve(num_writes);

        // We acquire block tokens for all the blocks before writing new
        // version.  The point of this is to make sure the _new_ block is
        // correctly "alive" when we write it.

        std::vector<counted_t<ls_block_token_pointee_t> > old_block_tokens;
        old_block_tokens.reserve(num_writes);

        // New block tokens, to hold the return value of
        // data_block_manager_t::write() instead of immediately discarding the
        // created token and causing the extent or block to be collected.
        std::vector<counted_t<ls_block_token_pointee_t> > new_block_tokens;
        new_block_tokens.reserve(num_writes);

        {
            ASSERT_NO_CORO_WAITING;
            // Step 1: Write buffers to disk and assemble index operations
            for (int i = 0; i < num_writes; ++i) {
                block_write_conds.push_back(new block_write_cond_t());
                // ... and save block tokens for the old offset.
                guarantee(parent->gc_state.current_entry != NULL, "i = %d", i);
                old_block_tokens.push_back(parent->serializer->generate_block_token(writes[i].old_offset,
                                                                                    writes[i].ser_block_size));

                const ls_buf_data_t *data = static_cast<const ls_buf_data_t *>(writes[i].buf) - 1;

                // The first "false" argument indicates that we do not with to assign
                // a new block sequence id.
                counted_t<ls_block_token_pointee_t> token
                    = parent->write(writes[i].buf, data->block_id,
                                    false,
                                    parent->choose_gc_io_account(),
                                    block_write_conds.back());

                new_block_tokens.push_back(token);

                writes[i].new_offset = token->offset();
            }
        }

        // Step 2: Wait on all writes to finish
        for (size_t i = 0; i < block_write_conds.size(); ++i) {
            block_write_conds[i]->wait();
            delete block_write_conds[i];
        }

        // We created block tokens for our blocks we're writing, so
        // there's no way the current entry could have become NULL.
        guarantee(parent->gc_state.current_entry != NULL);

        std::vector<index_write_op_t> index_write_ops;

        // Step 3: Figure out index ops.  It's important that we do this
        // now, right before the index_write, so that the updates to the
        // index are done atomically.
        {
            ASSERT_NO_CORO_WAITING;

            for (int i = 0; i < num_writes; ++i) {
                unsigned int block_index
                    = parent->gc_state.current_entry->block_index(writes[i].old_offset);

                if (parent->gc_state.current_entry->block_referenced_by_index(block_index)) {
                    const ls_buf_data_t *data = static_cast<const ls_buf_data_t *>(writes[i].buf) - 1;
                    index_write_ops.push_back(index_write_op_t(data->block_id,
                                                               to_standard_block_token(data->block_id,
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
            for (int i = 0; i < num_writes; ++i) {
                parent->serializer->remap_block_to_new_offset(writes[i].old_offset, writes[i].new_offset);
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
        parent->serializer->index_write(index_write_ops, parent->choose_gc_io_account());

        ASSERT_NO_CORO_WAITING;

        index_write_ops.clear();
    }

    {
        ASSERT_NO_CORO_WAITING;

        // Step 5: Call parent
        done = true;
        parent->on_gc_write_done();
    }

    // Step 6: Delete us
    delete this;
}

void data_block_manager_t::on_gc_write_done() {

    // Continue GC
    run_gc();
}

void data_block_manager_t::run_gc() {
    bool run_again = true;
    while (run_again) {
        run_again = false;
        switch (gc_state.step()) {
            case gc_ready: {
                if (gc_pq.empty() || !should_we_keep_gcing(*gc_pq.peak())) {
                    return;
                }

                ASSERT_NO_CORO_WAITING;

                ++stats->pm_serializer_data_extents_gced;

                /* grab the entry */
                gc_state.current_entry = gc_pq.pop();
                gc_state.current_entry->our_pq_entry = NULL;

                guarantee(gc_state.current_entry->state == gc_entry_t::state_old);
                gc_state.current_entry->state = gc_entry_t::state_in_gc;
                gc_stats.old_garbage_block_bytes -= gc_state.current_entry->garbage_bytes();
                gc_stats.old_total_block_bytes -= static_config->extent_size();

                /* read all the live data into buffers */

                /* make sure the read callback knows who we are */
                gc_state.gc_read_callback.parent = this;

                guarantee(gc_state.refcount == 0);

                // read_async can call the callback immediately, which
                // will then decrement the refcount (and that's all it
                // will do if the refcount is not zero).  So we
                // increment the refcount here and set the step to
                // gc_read, before any calls to read_async.
                gc_state.refcount++;

                guarantee(gc_state.gc_blocks == NULL);
                gc_state.gc_blocks = static_cast<char *>(malloc_aligned(extent_manager->extent_size,
                        DEVICE_BLOCK_SIZE));
                gc_state.set_step(gc_read);
                for (unsigned int i = 0, bpe = gc_state.current_entry->num_blocks(); i < bpe; i++) {
                    if (!gc_state.current_entry->block_is_garbage(i)) {
                        // Increment the refcount before read_async, because
                        // read_async can call its callback immediately, causing the
                        // decrement of the refcount.
                        gc_state.refcount++;
                        dbfile->read_async(gc_state.current_entry->extent_ref.offset() + (i * static_config->block_size().ser_value()),
                                           static_config->block_size().ser_value(),
                                           gc_state.gc_blocks + (i * static_config->block_size().ser_value()),
                                           choose_gc_io_account(),
                                           &(gc_state.gc_read_callback));
                    }
                }

                // Fall through to the gc_read case, where we
                // decrement the refcount we incremented before the
                // for loop.
            }
            case gc_read: {
                gc_state.refcount--;
                if (gc_state.refcount > 0) {
                    /* We got a block, but there are still more to go */
                    break;
                }

                /* If other forces cause all of the blocks in the extent to become
                garbage before we even finish GCing it, they will set current_entry
                to NULL. */
                if (gc_state.current_entry == NULL) {
                    guarantee(gc_state.gc_blocks != NULL);
                    free(gc_state.gc_blocks);
                    gc_state.gc_blocks = NULL;
                    gc_state.set_step(gc_ready);
                    break;
                }

                /* an array to put our writes in */
                int num_writes = gc_state.current_entry->num_live_blocks();

                gc_writes.clear();
                for (unsigned int i = 0, iend = gc_state.current_entry->num_blocks(); i < iend; ++i) {

                    /* We re-check the bit array here in case a write came in for one
                    of the blocks we are GCing. We wouldn't want to overwrite the new
                    valid data with out-of-date data. */
                    if (gc_state.current_entry->block_is_garbage(i)) {
                        continue;
                    }

                    char *block = gc_state.gc_blocks + gc_state.current_entry->relative_offset(i);
                    const int64_t block_offset = gc_state.current_entry->extent_ref.offset()
                        + gc_state.current_entry->relative_offset(i);

                    block_id_t id;
                    // The block is either referenced by an index or by a token (or both)
                    if (gc_state.current_entry->block_referenced_by_index(i)) {
                        id = (reinterpret_cast<ls_buf_data_t *>(block))->block_id;
                        guarantee(id != NULL_BLOCK_ID);
                    } else {
                        id = NULL_BLOCK_ID;
                    }
                    void *data = block + sizeof(ls_buf_data_t);

                    gc_writes.push_back(gc_write_t(id, data, block_offset, gc_state.current_entry->block_size(i)));
                }

                guarantee(gc_writes.size() == static_cast<size_t>(num_writes));

                gc_state.set_step(gc_write);

                /* schedule the write */
                gc_writer_t *gc_writer = new gc_writer_t(gc_writes.data(), gc_writes.size(), this);
                if (!gc_writer->done) {
                    break;
                } else {
                    delete gc_writer;
                }
            }

            case gc_write:
                //We need to do this here so that we don't
                //get stuck on the GC treadmill
                mark_unyoung_entries();
                /* Our write should have forced all of the blocks in the extent to
                become garbage, which should have caused the extent to be released
                and gc_state.current_entry to become NULL. */

                guarantee(gc_state.current_entry == NULL,
                          "%zd garbage bytes left on the extent, %zd index-referenced "
                          "bytes, %zd token-referenced bytes.\n",
                          gc_state.current_entry->garbage_bytes(),
                          gc_state.current_entry->index_bytes(),
                          gc_state.current_entry->token_bytes());

                guarantee(gc_state.refcount == 0);

                guarantee(gc_state.gc_blocks != NULL);
                free(gc_state.gc_blocks);
                gc_state.gc_blocks = NULL;
                gc_state.set_step(gc_ready);

                if (state == state_shutting_down) {
                    actually_shutdown();
                    return;
                }

                run_again = true;   // We might want to start another GC round
                break;

            case gc_reconstruct:
            default:
                unreachable("Unknown gc_step");
        }
    }
}

void data_block_manager_t::prepare_metablock(metablock_mixin_t *metablock) {
    guarantee(state == state_ready || state == state_shutting_down);

    for (int i = 0; i < MAX_ACTIVE_DATA_EXTENTS; i++) {
        if (active_extents[i]) {
            metablock->active_extents[i] = active_extents[i]->extent_ref.offset();
            metablock->blocks_in_active_extent[i] = blocks_in_active_extent[i];
        } else {
            metablock->active_extents[i] = NULL_OFFSET;
            metablock->blocks_in_active_extent[i] = 0;
        }
    }
}

bool data_block_manager_t::shutdown(shutdown_callback_t *cb) {
    rassert(cb != NULL);
    guarantee(state == state_ready);
    state = state_shutting_down;

    if (gc_state.step() != gc_ready) {
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

    for (unsigned int i = 0; i < dynamic_config->num_active_data_extents; i++) {
        if (active_extents[i]) {
            UNUSED int64_t extent = active_extents[i]->extent_ref.release();
            delete active_extents[i];
            active_extents[i] = NULL;
        }
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

counted_t<ls_block_token_pointee_t> data_block_manager_t::gimme_a_new_offset() {
    /* Start a new extent if necessary */

    if (!active_extents[next_active_extent]) {
        active_extents[next_active_extent] = new gc_entry_t(this);
        active_extents[next_active_extent]->state = gc_entry_t::state_active;
        blocks_in_active_extent[next_active_extent] = 0;

        ++stats->pm_serializer_data_extents_allocated;
    }

    /* Put the block into the chosen extent */

    guarantee(active_extents[next_active_extent]->state == gc_entry_t::state_active);
    guarantee(active_extents[next_active_extent]->garbage_bytes() > 0);
    guarantee(blocks_in_active_extent[next_active_extent] < static_config->blocks_per_extent());

    int64_t offset = active_extents[next_active_extent]->extent_ref.offset() + blocks_in_active_extent[next_active_extent] * static_config->block_size().ser_value();
    active_extents[next_active_extent]->was_written = true;

    guarantee(active_extents[next_active_extent]->block_is_garbage(blocks_in_active_extent[next_active_extent]));
    active_extents[next_active_extent]->mark_live_tokenwise(blocks_in_active_extent[next_active_extent]);
    guarantee(!active_extents[next_active_extent]->block_referenced_by_index(blocks_in_active_extent[next_active_extent]));

    blocks_in_active_extent[next_active_extent]++;

    /* Deactivate the extent if necessary */

    if (blocks_in_active_extent[next_active_extent] == static_config->blocks_per_extent()) {
        guarantee(active_extents[next_active_extent]->garbage_bytes() < static_config->extent_size(),
                  "garbage_bytes() == %zu, extent_size=%" PRIu64,
                  active_extents[next_active_extent]->garbage_bytes(), static_config->extent_size());
        active_extents[next_active_extent]->state = gc_entry_t::state_young;
        young_extent_queue.push_back(active_extents[next_active_extent]);
        mark_unyoung_entries();
        active_extents[next_active_extent] = NULL;
    }

    /* Move along to the next extent. This logic is kind of weird because it needs to
    handle the case where we have just started up and we still have active extents
    open from a previous run, but the value of num_active_data_extents was higher on
    that previous run and so there are active data extents that occupy slots in
    active_extents that are higher than our current value of
    num_active_data_extents. The way we handle this case is by continuing to visit
    those slots until the data extents fill up and are deactivated, but then not
    visiting those slots any more. */

    do {
        next_active_extent = (next_active_extent + 1) % MAX_ACTIVE_DATA_EXTENTS;
    } while (next_active_extent >= dynamic_config->num_active_data_extents &&
             !active_extents[next_active_extent]);

    // RSI: don't pass fake block size.
    return serializer->generate_block_token(offset,
                                            serializer->get_block_size().ser_value());
}

// Looks at young_extent_queue and pops things off the queue that are
// no longer deemed young, putting them on the priority queue.
void data_block_manager_t::mark_unyoung_entries() {
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
    gc_entry_t *entry = young_extent_queue.head();
    young_extent_queue.remove(entry);

    guarantee(entry->state == gc_entry_t::state_young);
    entry->state = gc_entry_t::state_old;

    entry->our_pq_entry = gc_pq.push(entry);

    gc_stats.old_total_block_bytes += static_config->extent_size();
    gc_stats.old_garbage_block_bytes += entry->garbage_bytes();
}

void gc_entry_t::add_self_to_parent_entries() {
    unsigned int extent_id = parent->static_config->extent_index(extent_offset);
    guarantee(parent->entries.get(extent_id) == NULL);
    parent->entries.set(extent_id, this);
    g_array.set();

    ++parent->stats->pm_serializer_data_extents;
}

gc_entry_t::gc_entry_t(data_block_manager_t *_parent)
    : parent(_parent),
      extent_ref(parent->extent_manager->gen_extent()),
      g_array(parent->static_config->blocks_per_extent()),
      t_array(parent->static_config->blocks_per_extent()),
      i_array(parent->static_config->blocks_per_extent()),
      timestamp(current_microtime()),
      was_written(false),
      extent_offset(extent_ref.offset()) {
    add_self_to_parent_entries();
}

gc_entry_t::gc_entry_t(data_block_manager_t *_parent, int64_t _offset)
    : parent(_parent),
      extent_ref(parent->extent_manager->reserve_extent(_offset)),
      g_array(parent->static_config->blocks_per_extent()),
      t_array(parent->static_config->blocks_per_extent()),
      i_array(parent->static_config->blocks_per_extent()),
      timestamp(current_microtime()),
      was_written(false),
      extent_offset(extent_ref.offset()) {
    add_self_to_parent_entries();
}

gc_entry_t::~gc_entry_t() {
    unsigned int extent_id = parent->static_config->extent_index(extent_offset);
    guarantee(parent->entries.get(extent_id) == this);
    parent->entries.set(extent_id, NULL);

    --parent->stats->pm_serializer_data_extents;
}

std::vector<uint32_t> gc_entry_t::block_boundaries() const {
    std::vector<uint32_t> ret;
    uint32_t block_size = parent->static_config->block_size().ser_value();
    for (unsigned int i = 0, e = g_array.size(); i <= e; ++i) {
        ret.push_back(i * block_size);
    }
    return ret;
}

uint32_t gc_entry_t::block_size(UNUSED unsigned int block_index) const {
    return parent->static_config->block_size().ser_value();
}

uint32_t gc_entry_t::relative_offset(unsigned int block_index) const {
    return block_index * parent->static_config->block_size().ser_value();
}

unsigned int gc_entry_t::block_index(const int64_t offset) const {
    // RSI: When we have variable sized blocks, we'll need to change this
    // implementation.
    const int64_t relative_offset = offset % parent->static_config->extent_size();
    return relative_offset / parent->static_config->block_size().ser_value();
}

void gc_entry_t::destroy() {
    parent->extent_manager->release_extent(std::move(extent_ref));
    delete this;
}

uint64_t gc_entry_t::garbage_bytes() const {
    uint64_t x = g_array.count();
    return x * parent->static_config->block_size().ser_value();
}

uint64_t gc_entry_t::token_bytes() const {
    uint64_t x = t_array.count();
    return x * parent->static_config->block_size().ser_value();
}

uint64_t gc_entry_t::index_bytes() const {
    uint64_t x = i_array.count();
    return x * parent->static_config->block_size().ser_value();
}

void gc_entry_t::mark_live_indexwise_with_offset(int64_t offset,
                                                 uint32_t ser_block_size) {
    // RSI: actually use ser_block_size, remove this assertion.
    guarantee(ser_block_size == parent->static_config->block_size().ser_value());

    mark_live_indexwise(block_index(offset));
}



/* functions for gc structures */

// Answers the following question: We're in the middle of gc'ing, and
// look, it's the next largest entry.  Should we keep gc'ing?  Returns
// false when the entry is active or young, or when its garbage ratio
// is lower than GC_THRESHOLD_RATIO_*.
bool data_block_manager_t::should_we_keep_gcing(UNUSED const gc_entry_t& entry) const {
    return !gc_state.should_be_stopped && garbage_ratio() > dynamic_config->gc_low_ratio;
}

// Answers the following question: Do we want to bother gc'ing?
// Returns true when our garbage_ratio is greater than
// GC_THRESHOLD_RATIO_*.
bool data_block_manager_t::do_we_want_to_start_gcing() const {
    return !gc_state.should_be_stopped && garbage_ratio() > dynamic_config->gc_high_ratio;
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

bool data_block_manager_t::disable_gc(gc_disable_callback_t *cb) {
    // We _always_ call the callback!

    guarantee(gc_state.gc_disable_callback == NULL);
    gc_state.should_be_stopped = true;

    if (gc_state.step() != gc_ready && gc_state.step() != gc_reconstruct) {
        gc_state.gc_disable_callback = cb;
        return false;
    } else {
        cb->on_gc_disabled();
        return true;
    }
}

void data_block_manager_t::enable_gc() {
    gc_state.should_be_stopped = false;
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
