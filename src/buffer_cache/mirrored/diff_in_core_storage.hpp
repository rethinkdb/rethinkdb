#ifndef __DIFF_IN_CORE_STORAGE_HPP__
#define	__DIFF_IN_CORE_STORAGE_HPP__

#include <list>
#include <map>
#include "buffer_cache/buf_patch.hpp"

// TODO: Maybe move this file out of mirrored?

class diff_core_storage_t {
public:
    diff_core_storage_t(); // Initialize an empty diff storage

    // This assumes that patches is properly sorted. It will initialize a block_patch_list_t and store it.
    void load_block_patch_list(const block_id_t block_id, const std::list<buf_patch_t*>& patches);

    // Removes all patches which are obsolete w.r.t. the given transaction_id
    void filter_applied_patches(const block_id_t block_id, const ser_transaction_id_t transaction_id);

    // Returns true iff any changes have been made to the buf
    bool apply_patches(const block_id_t block_id, char *buf_data) const;

    void store_patch(buf_patch_t &patch);

    // Return NULL if no patches exist for that block
    const std::list<buf_patch_t*>* get_patches(const block_id_t block_id) const;

    // Remove all patches for that block (e.g. after patches have been applied and the block gets flushed to disk)
    void drop_patches(const block_id_t block_id);

private:
    struct block_patch_list_t : public std::list<buf_patch_t*> {
        ~block_patch_list_t(); // Deletes all stored patches
        void filter_before_transaction(const ser_transaction_id_t transaction_id); // Deletes all patches that apply to transactions lower than the given transaction id
    };

    typedef std::map<block_id_t, block_patch_list_t> patch_map_t;
    patch_map_t patch_map;
};


#endif	/* __DIFF_IN_CORE_STORAGE_HPP__ */

