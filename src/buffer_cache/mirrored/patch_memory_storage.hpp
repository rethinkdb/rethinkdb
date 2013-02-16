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
class patch_memory_storage_t {
    class block_patch_list_t {
    public:
        typedef std::vector<buf_patch_t *>::const_iterator const_iterator;

        block_patch_list_t();

        // Deletes all stored patches
        ~block_patch_list_t();

        // Deletes all patches that apply to transactions lower than
        // the given transaction id
        void filter_before_block_sequence(const block_sequence_id_t block_sequence_id);

        // Grabs ownership of the patch.
        void add_patch(buf_patch_t *patch);

        int32_t patches_serialized_size() const { return patches_serialized_size_; }

        bool empty() const { return patches_.empty(); }

        const_iterator patches_begin() const { return patches_.begin(); }
        const_iterator patches_end() const { return patches_.end(); }

#ifndef NDEBUG
        void verify_patches_list(block_sequence_id_t) const;
#endif
    private:
        int32_t patches_serialized_size_;

        // This owns the pointers it contains and they get deleted when we're done.
        std::vector<buf_patch_t *> patches_;
    };

public:
    typedef block_patch_list_t::const_iterator const_patch_iterator;

    patch_memory_storage_t(); // Initialize an empty diff storage

private:

    typedef std::map<block_id_t, block_patch_list_t> patch_map_t;
    const patch_map_t patch_map;  // Always empty.
};


#endif /* BUFFER_CACHE_MIRRORED_PATCH_MEMORY_STORAGE_HPP_ */

