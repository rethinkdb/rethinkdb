// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef BUFFER_CACHE_MIRRORED_PATCH_DISK_STORAGE_HPP_
#define BUFFER_CACHE_MIRRORED_PATCH_DISK_STORAGE_HPP_

#include <vector>

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
};

#endif /* BUFFER_CACHE_MIRRORED_PATCH_DISK_STORAGE_HPP_ */

