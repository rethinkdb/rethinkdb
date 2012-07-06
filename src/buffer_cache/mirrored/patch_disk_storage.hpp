#ifndef BUFFER_CACHE_MIRRORED_PATCH_DISK_STORAGE_HPP_
#define BUFFER_CACHE_MIRRORED_PATCH_DISK_STORAGE_HPP_

#include "buffer_cache/buf_patch.hpp"
#include "buffer_cache/mirrored/patch_memory_storage.hpp"
#include "buffer_cache/mirrored/config.hpp"

class mc_cache_t;
class mc_buf_lock_t;
class serializer_t;
class coro_t;

extern const block_magic_t log_block_magic;

struct mc_config_block_t {
    block_magic_t magic;

    mirrored_cache_static_config_t cache;

    static const block_magic_t expected_magic;
};

/*
 * patch_disk_storage_t provides an on-disk storage data structure for buffer patches.
 * Before its first use, it has to be initialized, giving it a range of blocks it
 * can use to store the patch log data. The store_patch method allows to store a new
 * patch to the on-disk log. It never blocks, but might fail if it cannot find
 * enough space to store the patch.
 * On the other hand there is clear_n_oldest_blocks(), which completely frees a
 * number of log blocks. For this, it might have to acquire additional blocks
 * from disk, which makes it a blocking operation.
 * compress_n_oldest_blocks can be used as an optimization, but is usually not necessary
 * as store_patch compresses blocks automatically while it tries to find space to
 * store a patch.
 */

/* WARNING: Most of the functions in here are *not* reentrant-safe and rely on concurrency control happening
 *  elsewhere (specifically in mc_cache_t and writeback_t) */

// TODO: This is a huge mess because it goes through the same mc_inner_buf_t / mc_buf_lock_t interface
// that the cache client goes through, except that it cheats on some things. Instead it should just
// directly manage its own buffers in a different block-id-space than the public buffers.

class patch_disk_storage_t {
public:
    static void create(serializer_t *serializer, block_id_t start_id, mirrored_cache_static_config_t *config);
    patch_disk_storage_t(mc_cache_t *cache, block_id_t start_id);
    ~patch_disk_storage_t();

    void shutdown();

    // Loads on-disk data into memory
    void load_patches(patch_memory_storage_t &in_memory_storage);

    // Returns true on success, false if patch could not be stored (e.g. because of insufficient free space in log)
    // This function never blocks and must only be called while the flush_lock is held.
    bool store_patch(buf_patch_t &patch, const block_sequence_id_t current_block_block_sequence_id);

    // This function might block while it acquires old blocks from disk.
    void clear_n_oldest_blocks(unsigned int n);

    void compress_n_oldest_blocks(unsigned int n);

    unsigned int get_number_of_log_blocks() const;

private:

    void reclaim_space(const size_t space_required); // Calls compress_block for select_log_block_for_compression()
    block_id_t select_log_block_for_compression(); // For now: just always select the oldest (=next) block
    void compress_block(const block_id_t log_block_id);

    void clear_block(const block_id_t log_block_id, coro_t* notify_coro);
    void set_active_log_block(const block_id_t log_block_id);

    void init_log_block(const block_id_t log_block_id);

    // We use our own acquire function which does not care about locks
    // (we are the only one ever acessing the log blocks, except for writeback_t
    //      which takes care of synchronizing with us by itself)
    mc_buf_lock_t * acquire_block_no_locking(const block_id_t log_block_id);

    block_id_t active_log_block;
    uint16_t next_patch_offset;

    unsigned int waiting_for_clear;

    mc_cache_t *cache;
    block_id_t first_block;
    block_id_t number_of_blocks;
    std::vector<bool> block_is_empty;
    std::vector<mc_buf_lock_t *> log_block_bufs;
};

#endif /* BUFFER_CACHE_MIRRORED_PATCH_DISK_STORAGE_HPP_ */

