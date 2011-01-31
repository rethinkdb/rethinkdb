#ifndef __DIFF_OUT_OF_CORE_STORAGE_HPP__
#define	__DIFF_OUT_OF_CORE_STORAGE_HPP__

#include "buffer_cache/buf_patch.hpp"
#include "buffer_cache/mirrored/diff_in_core_storage.hpp"

class mc_cache_t;
class mc_buf_t;

static char LOG_BLOCK_MAGIC[] __attribute__((unused)) = {'L','O','G','B'};

/* WARNING: Most of the functions in here are not reentrant-safe and rely on concurrency control happening */
/*  elsewhere (specifically in mc_cache_t and writeback_t) */
class diff_oocore_storage_t {
public:
    diff_oocore_storage_t(mc_cache_t &cache);

    void init(const block_id_t first_block, const block_id_t number_of_blocks);

    // Loads on-disk data into memory
    void load_patches(diff_core_storage_t &in_core_storage);

    // Returns true on success, false if patch could not be stored (e.g. because of insufficient free space in log)
    // This function never blocks and must only be called while the flush_lock is held.
    bool store_patch(buf_patch_t &patch);

    // This function might block while it acquires old blocks from disk.
    void flush_n_oldest_blocks(unsigned int n);
    
private:
    void reclaim_space(const size_t space_required); // TODO! Calls compress_block for select_log_block_for_compression()
    block_id_t select_log_block_for_compression(); // TODO! For now: just always select the oldest (=next) block
    void compress_block(const block_id_t log_block_id); // TODO! (checks in-core storage to see if there are unapplied patches)

    void flush_block(const block_id_t log_block_id); // TODO! Interacts with cache (cache needs an additional function, which acquires a block without locking it, then sets needs_flush and truncates etc.)
    void set_active_log_block(const block_id_t log_block_id);

    void init_log_block(const block_id_t log_block_id);

    // We use our own acquire function which does not care about locks
    // (we are the only one ever acessing the log blocks, except for writeback_t
    //      which takes care of synchronizing with us by itself)
    mc_buf_t* acquire_block_no_locking(const block_id_t log_block_id);

    block_id_t active_log_block;
    uint16_t next_patch_offset;

    mc_cache_t &cache;
    block_id_t first_block;
    block_id_t number_of_blocks;
};

#endif	/* __DIFF_OUT_OF_CORE_STORAGE_HPP__ */

