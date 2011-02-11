#ifndef __BUF_HELPERS_HPP__
#define	__BUF_HELPERS_HPP__

#include <vector>

// This file contains a mock buffer implementation and can be used
// in other tests which rely on buf_t

#include "serializer/types.hpp"
#include "buffer_cache/buf_patch.hpp"

namespace unittest {

class test_buf_t
{
public:
    test_buf_t(const block_size_t bs, const block_id_t block_id) :
            block_id(block_id) {
        data.resize(bs.value(), '\0');
        dirty = false;
        next_patch_counter = 1;
    }

    block_id_t get_block_id() {
        return block_id;
    }

    const void *get_data_read() {
        return &data[0];
    }

    void *get_data_major_write() {
        dirty = true;
        return &data[0];
    }

    void set_data(const void* dest, const void* src, const size_t n) {
        memcpy((char*)dest, (char*)src, n);
    }

    void move_data(const void* dest, const void* src, const size_t n) {
        memmove((char*)dest, (char*)src, n);
    }

    void apply_patch(buf_patch_t *patch) {
        patch->apply_to_buf((char*)get_data_major_write());
        delete patch;
    }

    patch_counter_t get_next_patch_counter() {
        return next_patch_counter++;
    }

    void mark_deleted() {
    }

    void release() {
        delete this;
    }

    bool is_dirty() {
        return dirty;
    }

private:
    block_id_t block_id;
    patch_counter_t next_patch_counter;
    bool dirty;
    std::vector<char> data;
};

}  // namespace unittest

#define CUSTOM_BUF_TYPE
typedef unittest::test_buf_t buf_t;


#endif	/* __BUF_HELPERS_HPP__ */

