#ifndef __DIFF_IN_CORE_STORAGE_HPP__
#define	__DIFF_IN_CORE_STORAGE_HPP__

// TODO: Maybe move this file out of mirrored?

class diff_core_storage_t {
public:
    diff_core_storage_t(); // Initialize an empty diff storage

    // This assumes that patches is properly sorted. It will initialize a block_patch_list_t and store it.
    void load_block_patch_list(const block_id_t block_id, const std::list<buf_patch_t*>& patches);

    // Returns true iff any changes have been made to the buf
    bool flush_patches(const block_id_t block_id, char* buf_data);

    void store_patch(const block_id_t, buf_patch_t* patch);

private:
    struct block_patch_list_t  : public std::list<buf_patch_t*> {
        void truncate(); // Clears the patch list and deletes all stored patches
    };

    typedef std::map<block_id_t, block_patch_list_t> patch_map_t;
    patch_map_t patch_map;

    // TODO! Implement the reclaiming priority list thing
};


#endif	/* __DIFF_IN_CORE_STORAGE_HPP__ */

