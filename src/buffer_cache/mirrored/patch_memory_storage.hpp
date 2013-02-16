// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef BUFFER_CACHE_MIRRORED_PATCH_MEMORY_STORAGE_HPP_
#define BUFFER_CACHE_MIRRORED_PATCH_MEMORY_STORAGE_HPP_

#include <list>
#include <map>
#include <utility>
#include <vector>

#include "buffer_cache/buf_patch.hpp"

/*
 * The patch_memory_storage_t provides a data structure to store and access buffer
 * patches in an efficient way. It provides a few convenience methods to make
 * operating with patches easier. Specifically, there is an apply_patches method
 * which replays all stored patches on a buffer and a filter_applied_patches method
 * which removed outdated patches from the storage (i.e. patches which apply to an older
 * block sequence id than the one provided).
 * When loading a block into the cache, the following steps have to be completed
 * to bring the in-memory buffer up-to-date:
 *  filter_applied_patches() with the blocks on-disk block_sequence_id
 *  apply_patches() to the buffer
 */
class patch_memory_storage_t { };


#endif /* BUFFER_CACHE_MIRRORED_PATCH_MEMORY_STORAGE_HPP_ */

