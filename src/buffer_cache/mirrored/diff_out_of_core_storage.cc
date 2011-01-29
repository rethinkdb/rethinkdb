#include "buffer_cache/mirrored/diff_out_of_core_storage.hpp"

diff_oocore_storage_t::diff_oocore_storage_t(mc_cache_t &cache) : cache(cache) {
    first_block = 0;
    number_of_blocks = 0;

    active_log_block = 0;
    next_patch_offset = 0;
}

void diff_oocore_storage_t::init(const block_id_t first_block, const block_id_t number_of_blocks) {
    this->first_block = first_block;
    this->number_of_blocks = number_of_blocks;

    if (number_of_blocks == 0)
        return;

    // TODO! Initialize the set of log blocks
    // TODO! (no transaction required) acquire all blocks, if new: zero and write magic. If existing: check magic

    set_active_log_block(first_block);
}

// Loads on-disk data into memory
void diff_oocore_storage_t::load_patches(diff_core_storage_t &in_core_storage) {
    if (number_of_blocks == 0)
        return;

    // TODO! Start read transaction
    // TODO! Iterate from first_block to first_block + number_of_blocks
    // TODO! Scan through the block, build a map of patch vectors
    // TODO! Finally sort each vector
    // TODO! Then iterate over map and load the lists into the in_core_storage
}

// Returns true on success, false if patch could not be stored (e.g. because of insufficient free space in log)
// This function never blocks and must only be called while the flush_lock is held.
bool diff_oocore_storage_t::store_patch(buf_patch_t &patch) {
    // TODO! assert flush_in_progress

    if (number_of_blocks == 0)
        return false;

    // TODO! (do NOT start a transaction here)
    // TODO! Check if there is sufficient space.
    // TODO! If not: call reclaim_space
    // TODO! Check for sufficient space again
    // TODO! if space sufficient: serialize patch at next_patch_offset, increase offset
    return false;
}

// This function might block while it acquires old blocks from disk.
void diff_oocore_storage_t::flush_n_oldest_blocks(const unsigned int n) {
    // TODO! assert flush_in_progress
    // TODO! assert n <= number_of_blocks
    if (number_of_blocks == 0)
        return;

    // TODO! (do NOT start a transaction here)
    // TODO! Iterate over the next n blocks
    // TODO! Call flush_block on each of them
}

void diff_oocore_storage_t::reclaim_space(const size_t space_required) {
    block_id_t compress_block_id = select_log_block_for_compression();
    compress_block(compress_block_id);
    set_active_log_block(compress_block_id);
}

block_id_t diff_oocore_storage_t::select_log_block_for_compression() {
    block_id_t result = active_log_block + 1;
    if (result >= first_block + number_of_blocks)
        result -= number_of_blocks;
    return result;
}

void diff_oocore_storage_t::compress_block(const block_id_t log_block_id) {
    // TODO! Implement operator< in patch_buf_t, which compares 1) transaction_id 2) patch_counter
    // TODO! Scan over block. For each patch, check if it is >= than the oldest in the in-core storage for the corr. block
    // TODO! If patch is current: load it and attach it to a list
    // TODO! Finally, initialize the block and write back the list
    // TODO! (add an init_log_block(const block_id_t) helper method!)
}

void diff_oocore_storage_t::flush_block(const block_id_t log_block_id) {
    // TODO! Get buf_t from cache (loading it into memory if necessary)
    // TODO! Scan over block. For each patch, check if it is >= than the oldest in the in-core storage for the corr. block
    // TODO! If patch current: set needs_flush in the block and drop_patches from in-core list
    // TODO! Initialize the block
}

void diff_oocore_storage_t::set_active_log_block(const block_id_t log_block_id) {
    // TODO! Set active_log_block
    // TODO! Scan through the block to determine next_patch_offset
    // TODO! Set next_patch_offset to that value.
}
