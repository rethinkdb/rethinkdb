#ifndef __BUFFER_CACHE_BLOB_HPP__
#define __BUFFER_CACHE_BLOB_HPP__

#include <stdint.h>
#include <stddef.h>

#include "buffer_cache/buffer_cache.hpp"

typedef uint32_t block_id_t;

class buffer_group_t;

class blob_acq_t {
public:
    blob_acq_t() { }
    ~blob_acq_t() {
        for (int i = 0, e = bufs_.size(); i < e; ++i) {
            bufs_[i]->release();
        }
    }

    void add_buf(buf_t *buf) {
        bufs_.push_back(buf);
    }

private:
    std::vector<buf_t *> bufs_;

    // disable copying
    blob_acq_t(const blob_acq_t&);
    void operator=(const blob_acq_t&);
};


union temporary_acq_tree_node_t;

namespace blob {
struct traverse_helper_t;
}  // namespace blob

class blob_t {
public:
    blob_t(block_size_t block_size, const char *ref, size_t maxreflen);
    void dump_ref(block_size_t block_size, char *ref_out, size_t confirm_maxreflen);
    ~blob_t();


    size_t refsize(block_size_t block_size) const;
    int64_t valuesize() const;

    void expose_region(transaction_t *txn, access_t mode, int64_t offset, int64_t size, buffer_group_t *buffer_group_out, blob_acq_t *acq_group_out);
    void append_region(transaction_t *txn, int64_t size);
    void prepend_region(transaction_t *txn, int64_t size);

    void unappend_region(transaction_t *txn, int64_t size);
    void unprepend_region(transaction_t *txn, int64_t size);

private:
    bool traverse_to_dimensions(transaction_t *txn, int levels, int64_t old_offset, int64_t old_size, int64_t new_offset, int64_t new_size, blob::traverse_helper_t *helper);
    bool allocate_to_dimensions(transaction_t *txn, int levels, int64_t new_offset, int64_t new_size);
    bool shift_at_least(transaction_t *txn, int levels, int64_t min_shift);
    void consider_big_shift(transaction_t *txn, int levels, int64_t *min_shift);
    void consider_small_shift(transaction_t *txn, int levels, int64_t *min_shift);
    void deallocate_to_dimensions(transaction_t *txn, int levels, int64_t new_offset, int64_t new_size);
    int add_level(transaction_t *txn, int levels);
    bool remove_level(transaction_t *txn, int *levels_ref);

    char *ref_;
    size_t maxreflen_;

    // disable copying
    blob_t(const blob_t&);
    void operator=(const blob_t&);
};




#endif  // __BUFFER_CACHE_BLOB_HPP__
