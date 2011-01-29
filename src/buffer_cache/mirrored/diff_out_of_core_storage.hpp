#ifndef __DIFF_OUT_OF_CORE_STORAGE_HPP__
#define	__DIFF_OUT_OF_CORE_STORAGE_HPP__

#include "buffer_cache/buf_patch.hpp"
#include "buffer_cache/mirrored/diff_in_core_storage.hpp"
#include "buffer_cache/buffer_cache.hpp"

// TODO: Maybe move this file out of mirrored?

class diff_oocore_storage_t {
public:
    diff_oocore_storage_t(cache_t &cache, const block_id_t first_block, const block_id_t last_block);

    // Loads on-disk data into memory
    void load_patches(diff_core_storage_t &in_core_storage);

    // Returns true on success, false if patch could not be stored (e.g. because of insufficient free space in log)
    // This function never blocks and must only be called while the flush_lock is held.
    bool store_patch(buf_patch_t &patch);

    // This function might block while it acquires old blocks from disk.
    void flush_n_oldest_blocks(const unsigned int n);
    
private:
    // TODO! Handle on-demand initialization of log blocks somewhere
    void reclaim_space(const size_t space_required); // TODO! Calls compress_block for select_log_block_for_compression()
    block_id_t select_log_block_for_compression(); // TODO! For now: just always select the oldest (=next) block
    void compress_block(const block_id_t log_block_id); // TODO! (checks in-core storage to see if there are unapplied patches)

    void flush_block(const block_id_t log_block_id); // TODO! Interacts with cache (cache needs an additional function, which acquires a block without locking it, then sets needs_flush and truncates etc.)

    block_id_t active_log_block;
    // TODO! Write from end of block downwards?
    uint16_t next_patch_offset;

    cache_t &cache;
    block_id_t first_block;
    block_id_t last_block;
};

#endif	/* __DIFF_OUT_OF_CORE_STORAGE_HPP__ */

