#ifndef __PATCH_MEMORY_STORAGE_HPP__
#define	__PATCH_MEMORY_STORAGE_HPP__

#include <list>
#include <map>
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
public:
    patch_memory_storage_t(); // Initialize an empty diff storage

    // This assumes that patches is properly sorted. It will initialize a block_patch_list_t and store it.
    void load_block_patch_list(const block_id_t block_id, const std::list<buf_patch_t*>& patches);

    // Removes all patches which are obsolete w.r.t. the given block_sequence_id
    void filter_applied_patches(const block_id_t block_id, const ser_block_sequence_id_t block_sequence_id);

    // Returns true iff any changes have been made to the buf
    bool apply_patches(const block_id_t block_id, char *buf_data) const;

    inline void store_patch(buf_patch_t &patch) {
        const block_id_t block_id = patch.get_block_id();
        patch_map[block_id].push_back(&patch);
        patch_map[block_id].affected_data_size += patch.get_affected_data_size();
    }

    // Return NULL if no patches exist for that block
    const std::vector<buf_patch_t*>* get_patches(const block_id_t block_id) const;

    inline size_t get_affected_data_size(const block_id_t block_id) const  {
        patch_map_t::const_iterator map_entry = patch_map.find(block_id);
        if (map_entry == patch_map.end())
            return 0;
        else
            return map_entry->second.affected_data_size;
    }

    // Remove all patches for that block (e.g. after patches have been applied and the block gets flushed to disk)
    void drop_patches(const block_id_t block_id);

private:
    struct block_patch_list_t : public std::vector<buf_patch_t*> {
        block_patch_list_t();
        ~block_patch_list_t(); // Deletes all stored patches
        void filter_before_block_sequence(const ser_block_sequence_id_t block_sequence_id); // Deletes all patches that apply to blocks versions lower than the given block sequence id

        size_t affected_data_size;
    };

    typedef std::map<block_id_t, block_patch_list_t> patch_map_t;
    patch_map_t patch_map;
};


#endif	/* __PATCH_MEMORY_STORAGE_HPP__ */

