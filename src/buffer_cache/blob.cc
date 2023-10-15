// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "buffer_cache/blob.hpp"

#include <stdint.h>
#include <string.h>

#include <algorithm>
#include <limits>

#include "buffer_cache/alt.hpp"
#include "concurrency/pmap.hpp"
#include "containers/buffer_group.hpp"
#include "containers/unaligned.hpp"
#include "containers/scoped.hpp"
#include "math.hpp"
#include "serializer/types.hpp"
#include "utils.hpp"

// The maximal number of blocks to traverse concurrently.
// Roughly equivalent to the maximal number of coroutines that loading a blob
// can allocate.
const int64_t BLOB_TRAVERSAL_CONCURRENCY = 8;


template <class T>
void clear_and_delete(std::vector<T *> *vec) {
    while (!vec->empty()) {
        T *back = vec->back();
        vec->pop_back();
        delete back;
    }
}

blob_acq_t::~blob_acq_t() {
    clear_and_delete(&reads_);
    clear_and_delete(&writes_);
    clear_and_delete(&bufs_);
}

namespace blob {

// The internal node block ids of an internal node.
const unaligned<block_id_t> *internal_node_block_ids(const void *buf);

// Returns the internal block ids of a non-inlined blob ref.
const unaligned<block_id_t> *block_ids(const char *ref, int maxreflen);

int big_size_offset(int maxreflen) {
    return maxreflen <= 255 ? 1 : 2;
}

int block_ids_offset(int maxreflen) {
    return big_size_offset(maxreflen) + sizeof(int64_t);
}

temporary_acq_tree_node_t *
make_tree_from_block_ids(buf_parent_t parent, access_t mode, int levels,
                         int64_t offset, int64_t size, const unaligned<block_id_t> *block_ids);

// touches_end_t specifies whether the [offset, offset + size) region given to
// expose_tree_from_block_ids touches the end of the data in the blob.
enum class touches_end_t { yes, no };
void expose_tree_from_block_ids(buf_parent_t parent, access_t mode,
                                int levels, int64_t offset, int64_t size,
                                temporary_acq_tree_node_t *tree,
                                touches_end_t touches_end,
                                buffer_group_t *buffer_group_out,
                                blob_acq_t *acq_group_out);


int small_size(const char *ref, int maxreflen) {
    // small value sizes range from 0 to maxreflen - big_size_offset(maxreflen), and
    // maxreflen indicates large value.
    if (maxreflen <= 255) {
        return *reinterpret_cast<const uint8_t *>(ref);
    } else {
        return reinterpret_cast<const unaligned<uint16_t> *>(ref)->value;
    }
}

bool size_would_be_small(int64_t proposed_size, int maxreflen) {
    return proposed_size <= maxreflen - big_size_offset(maxreflen);
}

void set_small_size_field(char *ref, int maxreflen, int64_t size) {
    if (maxreflen <= 255) {
        *reinterpret_cast<uint8_t *>(ref) = size;
    } else {
        reinterpret_cast<unaligned<uint16_t> *>(ref)->value = size;
    }
}

void set_small_size(char *ref, int maxreflen, int64_t size) {
    rassert(size_would_be_small(size, maxreflen));
    set_small_size_field(ref, maxreflen, size);
}

bool is_small(const char *ref, int maxreflen) {
    return small_size(ref, maxreflen) < maxreflen;
}

char *small_buffer(char *ref, int maxreflen) {
    return ref + big_size_offset(maxreflen);
}

int64_t big_size(const char *ref, int maxreflen) {
    return reinterpret_cast<const unaligned<int64_t> *>(ref + big_size_offset(maxreflen))->value;
}

void set_big_size(char *ref, int maxreflen, int64_t new_size) {
    reinterpret_cast<unaligned<int64_t> *>(ref + big_size_offset(maxreflen))->value = new_size;
}

const unaligned<block_id_t> *block_ids(const char *ref, int maxreflen) {
    return reinterpret_cast<const unaligned<block_id_t> *>(ref + block_ids_offset(maxreflen));
}

unaligned<block_id_t> *block_ids(char *ref, int maxreflen) {
    return reinterpret_cast<unaligned<block_id_t> *>(ref + block_ids_offset(maxreflen));
}

int64_t leaf_size(max_block_size_t block_size) {
    return block_size.value() - sizeof(block_magic_t);
}

char *leaf_node_data(void *buf) {
    return reinterpret_cast<char *>(buf) + sizeof(block_magic_t);
}

const char *leaf_node_data(const void *buf) {
    return reinterpret_cast<const char *>(buf) + sizeof(block_magic_t);
}

const size_t LEAF_NODE_DATA_OFFSET = sizeof(block_magic_t);

int64_t internal_node_bytesize(max_block_size_t block_size) {
    return block_size.value() - sizeof(block_magic_t);
}

int64_t internal_node_count(max_block_size_t block_size) {
    return internal_node_bytesize(block_size) / sizeof(block_id_t);
}

// These actually are unaligned -- block_magic_t is 4 bytes, and
// blob internal node block ids are 64 bits wide.
const unaligned<block_id_t> *internal_node_block_ids(const void *buf) {
    return reinterpret_cast<const unaligned<block_id_t> *>(reinterpret_cast<const char *>(buf) + sizeof(block_magic_t));
}

unaligned<block_id_t> *internal_node_block_ids(void *buf) {
    return reinterpret_cast<unaligned<block_id_t> *>(reinterpret_cast<char *>(buf) + sizeof(block_magic_t));
}

ref_info_t big_ref_info(max_block_size_t block_size, int64_t size,
                        int maxreflen, int64_t *blockid_count_out) {
    rassert(size > int64_t(maxreflen - big_size_offset(maxreflen)));
    int64_t max_blockid_count = (maxreflen - block_ids_offset(maxreflen)) / sizeof(block_id_t);

    int64_t block_count = ceil_divide(size, leaf_size(block_size));

    int levels = 1;
    while (block_count > max_blockid_count) {
        block_count = ceil_divide(block_count, internal_node_count(block_size));
        ++levels;
    }

    ref_info_t info;
    info.refsize = block_ids_offset(maxreflen) + sizeof(block_id_t) * block_count;
    info.levels = levels;
    *blockid_count_out = block_count;
    return info;
}

ref_info_t big_ref_info(max_block_size_t block_size, int64_t size,
                        int maxreflen) {
    int64_t blockid_count;
    return big_ref_info(block_size, size, maxreflen, &blockid_count);
}

ref_info_t ref_info(max_block_size_t block_size, const char *ref, int maxreflen) {
    int smallsize = small_size(ref, maxreflen);
    if (smallsize <= maxreflen - big_size_offset(maxreflen)) {
        ref_info_t info;
        info.refsize = big_size_offset(maxreflen) + smallsize;
        info.levels = 0;
        return info;
    } else {
        return big_ref_info(block_size, big_size(ref, maxreflen), maxreflen);
    }
}

int ref_size(max_block_size_t block_size, const char *ref, int maxreflen) {
    return ref_info(block_size, ref, maxreflen).refsize;
}

bool ref_fits(max_block_size_t block_size, int data_length, const char *ref, int maxreflen) {
    if (data_length <= 0) {
        return false;
    }
    int smallsize = small_size(ref, maxreflen);
    if (smallsize <= maxreflen - big_size_offset(maxreflen)) {
        return big_size_offset(maxreflen) + smallsize <= data_length;
    } else {
        if (data_length < block_ids_offset(maxreflen)) {
            return false;
        }

        return data_length >= ref_size(block_size, ref, maxreflen);
    }
}

int maxreflen_from_blockid_count(int count) {
    // The block ids offset is either 17 or 18 depending on whether
    // the return value would be <= 255.
    int sz = block_ids_offset(1) + count * sizeof(block_id_t);
    int diff = block_ids_offset(sz) - block_ids_offset(1);
    return diff + sz;
}

int64_t value_size(const char *ref, int maxreflen) {
    int smallsize = blob::small_size(ref, maxreflen);
    if (smallsize <= maxreflen - big_size_offset(maxreflen)) {
        return smallsize;
    } else {
        rassert(smallsize == maxreflen);
        return blob::big_size(ref, maxreflen);
    }
}


int btree_maxreflen = 251;
block_magic_t internal_node_magic = { { 'l', 'a', 'r', 'i' } };
block_magic_t leaf_node_magic = { { 'l', 'a', 'r', 'l' } };


int64_t stepsize(max_block_size_t block_size, int levels) {
    rassert(levels > 0);
    int64_t step = leaf_size(block_size);
    for (int i = 0; i < levels - 1; ++i) {
        step *= internal_node_count(block_size);
    }
    return step;
}

int64_t max_end_offset(max_block_size_t block_size, int levels, int maxreflen) {
    if (levels == 0) {
        return maxreflen - big_size_offset(maxreflen);
    } else {
        int ref_block_ids = (maxreflen - block_ids_offset(maxreflen)) / sizeof(block_id_t);
        return stepsize(block_size, levels) * ref_block_ids;
    }
}

int64_t clamp(int64_t x, int64_t lo, int64_t hi) {
    return x < lo ? lo : x > hi ? hi : x;
}


void shrink(max_block_size_t block_size, int levels, int64_t offset, int64_t size, int index, int64_t *suboffset_out, int64_t *subsize_out) {
    int64_t step = stepsize(block_size, levels);

    int64_t clamp_low = index * step;
    int64_t clamp_high = clamp_low + step;

    int64_t suboffset = clamp(offset, clamp_low, clamp_high);
    int64_t subsize = clamp(offset + size, clamp_low, clamp_high) - suboffset;

    *suboffset_out = suboffset - clamp_low;
    *subsize_out = subsize;
}

// Exists so callers passing a 0 offset don't have to analyze that the suboffset will
// be 0.
void shrink(max_block_size_t block_size, int levels,
            int64_t size, int index, int64_t *subsize_out) {
    int64_t suboffset;
    shrink(block_size, levels, 0, size, index, &suboffset, subsize_out);
    guarantee(suboffset == 0);
}

void compute_acquisition_offsets(max_block_size_t block_size, int levels, int64_t offset, int64_t size, int *lo_out, int *hi_out) {
    int64_t step = stepsize(block_size, levels);

    *lo_out = offset / step;
    *hi_out = ceil_divide(offset + size, step);
}

}  // namespace blob

blob_t::blob_t(max_block_size_t block_size, char *ref, int maxreflen)
    : ref_(ref), maxreflen_(maxreflen) {
    guarantee(maxreflen <= blob::leaf_size(block_size));
    guarantee(maxreflen - blob::block_ids_offset(maxreflen) <= blob::internal_node_bytesize(block_size));
    guarantee(maxreflen >= blob::block_ids_offset(maxreflen) + static_cast<int>(sizeof(block_id_t)));
}



int blob_t::refsize(max_block_size_t block_size) const {
    return blob::ref_size(block_size, ref_, maxreflen_);
}

int64_t blob_t::valuesize() const {
    return blob::value_size(ref_, maxreflen_);
}

void blob_t::detach_subtrees(buf_parent_t root) {
    if (blob::is_small(ref_, maxreflen_)) {
        return;
    }

    int64_t blockid_count;
    blob::big_ref_info(root.cache()->max_block_size(),
                       blob::big_size(ref_, maxreflen_),
                       maxreflen_,
                       &blockid_count);

    const unaligned<block_id_t> *ids = blob::block_ids(ref_, maxreflen_);

    for (int64_t i = 0; i < blockid_count; ++i) {
        root.detach_child(ids[i].copy());
    }
}

union temporary_acq_tree_node_t {
    buf_lock_t *buf;
    temporary_acq_tree_node_t *child;
};

void blob_t::expose_region(buf_parent_t parent, access_t mode,
                           int64_t offset, int64_t size,
                           buffer_group_t *buffer_group_out,
                           blob_acq_t *acq_group_out) {
    if (size == 0) {
        return;
    }

    if (blob::is_small(ref_, maxreflen_)) {
        char *b = blob::small_buffer(ref_, maxreflen_);
#ifndef NDEBUG
        int n = blob::small_size(ref_, maxreflen_);
        rassert(0 <= offset && offset <= n);
        rassert(0 <= size && size <= n && offset + size <= n);
#endif
        buffer_group_out->add_buffer(size, b + offset);
    } else {
        // It's large.

        const int levels = blob::ref_info(parent.cache()->max_block_size(),
                                          ref_, maxreflen_).levels;

        // Acquiring is done recursively in parallel,
        temporary_acq_tree_node_t *tree
            = blob::make_tree_from_block_ids(parent, mode, levels,
                                             offset, size,
                                             blob::block_ids(ref_, maxreflen_));

        // Exposing and writing to the buffer group is done serially.
        blob::expose_tree_from_block_ids(parent, mode, levels,
                                         offset, size,
                                         tree,
                                         size == valuesize()
                                         ? blob::touches_end_t::yes
                                         : blob::touches_end_t::no,
                                         buffer_group_out,
                                         acq_group_out);
    }
}

void blob_t::expose_all(buf_parent_t parent, access_t mode,
                        buffer_group_t *buffer_group_out,
                        blob_acq_t *acq_group_out) {
    expose_region(parent, mode, 0, valuesize(), buffer_group_out, acq_group_out);
}

namespace blob {

struct region_tree_filler_t {
    explicit region_tree_filler_t(buf_parent_t _parent)
        : parent(_parent) { }

    buf_parent_t parent;
    access_t mode;
    int levels;
    int64_t offset, size;
    const unaligned<block_id_t> *block_ids;
    int lo, hi;
    temporary_acq_tree_node_t *nodes;

    void operator()(int i) const {
	block_id_t block_i = block_ids[lo + i].value;
        if (levels > 1) {
            buf_lock_t lock(parent, block_i, mode);
            buf_read_t buf_read(&lock);
            const unaligned<block_id_t> *sub_ids
                = blob::internal_node_block_ids(buf_read.get_data_read());

            int64_t suboffset, subsize;
            shrink(parent.cache()->max_block_size(), levels, offset, size, lo + i, &suboffset, &subsize);
            nodes[i].child = blob::make_tree_from_block_ids(buf_parent_t(&lock),
                                                            mode, levels - 1,
                                                            suboffset, subsize,
                                                            sub_ids);
        } else {
            nodes[i].buf = new buf_lock_t(parent, block_i, mode);
        }
    }
};

int64_t choose_concurrency(int levels) {
    // Parallelize on the lowest level. That way we get a higher chance
    // of sequential disk reads, assuming that the blob is laid out on disk
    // from left to right (which might or might not be true, depending the details
    // of how it was written and how the cache and serializer operate).
    if (levels > 1) {
        return 1;
    } else {
        rassert(levels == 1);
        return BLOB_TRAVERSAL_CONCURRENCY;
    }
}

temporary_acq_tree_node_t *
make_tree_from_block_ids(buf_parent_t parent, access_t mode, int levels,
                         int64_t offset, int64_t size, const unaligned<block_id_t> *block_ids) {
    rassert(size > 0);

    region_tree_filler_t filler(parent);
    filler.mode = mode;
    filler.levels = levels;
    filler.offset = offset;
    filler.size = size;
    filler.block_ids = block_ids;
    compute_acquisition_offsets(parent.cache()->max_block_size(), levels, offset,
                                size, &filler.lo, &filler.hi);

    filler.nodes = new temporary_acq_tree_node_t[filler.hi - filler.lo];

    throttled_pmap(filler.hi - filler.lo, filler, choose_concurrency(levels));

    return filler.nodes;
}

void expose_tree_from_block_ids(buf_parent_t parent, access_t mode,
                                int levels, int64_t offset, int64_t size,
                                temporary_acq_tree_node_t *tree,
                                touches_end_t touches_end,
                                buffer_group_t *buffer_group_out,
                                blob_acq_t *acq_group_out) {
    rassert(size > 0);

    int lo, hi;
    blob::compute_acquisition_offsets(parent.cache()->max_block_size(), levels,
                                      offset, size, &lo, &hi);

    for (int i = 0; i < hi - lo; ++i) {
        int64_t suboffset, subsize;
        blob::shrink(parent.cache()->max_block_size(), levels, offset, size,
                     lo + i, &suboffset, &subsize);
        if (levels > 1) {
            expose_tree_from_block_ids(parent, mode, levels - 1, suboffset,
                                       subsize, tree[i].child, touches_end,
                                       buffer_group_out,
                                       acq_group_out);
        } else {
            rassert(0 < subsize && subsize <= blob::leaf_size(parent.cache()->max_block_size()));
            rassert(0 <= suboffset && suboffset + subsize <= blob::leaf_size(parent.cache()->max_block_size()));

            buf_lock_t *buf = tree[i].buf;
            void *leaf_buf;
            if (mode == access_t::read) {
                buf_read_t *buf_read = new buf_read_t(buf);
                // We can't assert a specific block size here (without undesirably
                // intricate logic), because immediately after creation, the blob has
                // size max_value_size, but after we've written to the block, its
                // size could be shrunken.
                uint32_t block_size;
                leaf_buf = const_cast<void *>(buf_read->get_data_read(&block_size));
                acq_group_out->add_buf(buf, buf_read);
            } else {
                buf_write_t *buf_write = new buf_write_t(buf);
                if (touches_end == touches_end_t::yes) {
                    // Using suboffset + subsize is valid because we know that, when
                    // appending, there are no bytes in this block past suboffset +
                    // subsize that are a valid part of the blob.
                    leaf_buf = buf_write->get_data_write(suboffset + subsize
                                                         + blob::LEAF_NODE_DATA_OFFSET);
                } else {
                    leaf_buf = buf_write->get_data_write();
                }

                acq_group_out->add_buf(buf, buf_write);
            }

            char *data = blob::leaf_node_data(leaf_buf);
            buffer_group_out->add_buffer(subsize, data + suboffset);
        }
    }

    delete[] tree;
}

}  // namespace blob

void blob_t::append_region(buf_parent_t parent, int64_t size) {
    const max_block_size_t block_size = parent.cache()->max_block_size();
    int levels = blob::ref_info(block_size, ref_, maxreflen_).levels;

    // Avoid the empty blob effect.
    if (levels == 0 && blob::small_size(ref_, maxreflen_) == 0 && size > 0) {
        blob::set_small_size(ref_, maxreflen_, 1);
        size -= 1;
    }
    if (size == 0) {
        return;
    }

    while (!allocate_to_dimensions(parent, levels,
                                   valuesize() + size)) {
        levels = add_level(parent, levels);
    }

    if (levels == 0) {
        rassert(blob::is_small(ref_, maxreflen_));
        int64_t new_size = size + blob::small_size(ref_, maxreflen_);
        rassert(blob::size_would_be_small(new_size, maxreflen_));
        blob::set_small_size(ref_, maxreflen_, new_size);
    } else {
        blob::set_big_size(ref_, maxreflen_, blob::big_size(ref_, maxreflen_) + size);
    }

    rassert(blob::ref_info(block_size, ref_, maxreflen_).levels == levels);
}

void blob_t::unappend_region(buf_parent_t parent, int64_t size) {
    const max_block_size_t block_size = parent.cache()->max_block_size();
    int levels = blob::ref_info(block_size, ref_, maxreflen_).levels;

    bool emptying = false;
    if (valuesize() == size && size > 0) {
        emptying = true;
        size -= 1;
    }

    if (size != 0) {
        deallocate_to_dimensions(parent, levels,
                                 valuesize() - size);
        for (;;) {
            if (!remove_level(parent, &levels)) {
                break;
            }
        }
    }

    if (emptying) {
        rassert(blob::is_small(ref_, maxreflen_));
        rassert(blob::small_size(ref_, maxreflen_) == 1);
        blob::set_small_size(ref_, maxreflen_, 0);
    }
}

void blob_t::clear(buf_parent_t parent) {
    unappend_region(parent, valuesize());
}

void blob_t::write_from_string(const std::string &val,
                               buf_parent_t parent, int64_t offset) {
    buffer_group_t dest;
    blob_acq_t acq;
    expose_region(parent, access_t::write, offset, val.size(), &dest, &acq);

    buffer_group_t src;
    src.add_buffer(val.size(), val.data());
    buffer_group_copy_data(&dest, const_view(&src));
}

namespace blob {

struct traverse_helper_t {
    virtual buf_lock_t preprocess(buf_parent_t parent, int levels,
                                  unaligned<block_id_t> *block_id) = 0;
    virtual void postprocess(buf_lock_t *lock) = 0;
    virtual ~traverse_helper_t() { }
};

void traverse_recursively(buf_parent_t parent, int levels,
                          unaligned<block_id_t> *block_ids,
                          int64_t smaller_size, int64_t bigger_size,
                          traverse_helper_t *helper);

void traverse_index(buf_parent_t parent, int levels, unaligned<block_id_t> *block_ids,
                    int index, int64_t smaller_size, int64_t bigger_size,
                    traverse_helper_t *helper) {
    const max_block_size_t block_size = parent.cache()->max_block_size();
    int64_t sub_smaller_size, sub_bigger_size;
    blob::shrink(block_size, levels, smaller_size, index, &sub_smaller_size);
    blob::shrink(block_size, levels, bigger_size, index, &sub_bigger_size);

    if (sub_smaller_size > 0) {
        if (levels > 1) {
            buf_lock_t lock(parent, block_ids[index].copy(), access_t::write);
            buf_write_t write(&lock);
            void *b = write.get_data_write();

            unaligned<block_id_t> *subids = blob::internal_node_block_ids(b);
            traverse_recursively(buf_parent_t(&lock), levels - 1, subids,
                                 sub_smaller_size, sub_bigger_size,
                                 helper);
        }

    } else {
        buf_lock_t lock = helper->preprocess(parent, levels, &block_ids[index]);

        if (levels > 1) {
            buf_write_t write(&lock);
            void *b = write.get_data_write();
            unaligned<block_id_t> *subids = blob::internal_node_block_ids(b);
            traverse_recursively(buf_parent_t(&lock), levels - 1, subids,
                                 sub_smaller_size, sub_bigger_size,
                                 helper);
        }

        helper->postprocess(&lock);
    }
}

void traverse_recursively(buf_parent_t parent, int levels, unaligned<block_id_t> *block_ids,
                          int64_t smaller_size, int64_t bigger_size,
                          traverse_helper_t *helper) {
    const max_block_size_t block_size = parent.cache()->max_block_size();

    int old_lo, old_hi, new_lo, new_hi;
    compute_acquisition_offsets(block_size, levels, 0, smaller_size,
                                &old_lo, &old_hi);
    compute_acquisition_offsets(block_size, levels, 0, bigger_size,
                                &new_lo, &new_hi);

    int64_t leafsize = leaf_size(block_size);

    if (ceil_divide(bigger_size, leafsize) > ceil_divide(smaller_size, leafsize)) {
        throttled_pmap(std::max(0, old_hi - 1), new_hi,
            std::bind(&traverse_index, parent, levels, block_ids, ph::_1,
                      smaller_size, bigger_size, helper),
            choose_concurrency(levels));
    }
}


}  // namespace blob

bool blob_t::traverse_to_dimensions(buf_parent_t parent, int levels,
                                    int64_t smaller_size, int64_t bigger_size,
                                    blob::traverse_helper_t *helper) {
    rassert(bigger_size >= smaller_size);
    const max_block_size_t block_size = parent.cache()->max_block_size();
    if (bigger_size <= blob::max_end_offset(block_size, levels, maxreflen_)) {
        if (levels != 0) {
            blob::traverse_recursively(parent, levels,
                                       blob::block_ids(ref_, maxreflen_),
                                       smaller_size, bigger_size,
                                       helper);
        }
        return true;
    } else {
        return false;
    }
}

struct allocate_helper_t : public blob::traverse_helper_t {
    buf_lock_t preprocess(buf_parent_t parent, int levels,
                          unaligned<block_id_t> *block_id) {
        buf_lock_t temp_lock(parent, alt_create_t::create, block_type_t::aux);
        block_id->value = temp_lock.block_id();
        {
            buf_write_t lock_write(&temp_lock);
            void *b = lock_write.get_data_write();
            if (levels == 1) {
                *static_cast<block_magic_t *>(b) = blob::leaf_node_magic;
            } else {
                *static_cast<block_magic_t *>(b) = blob::internal_node_magic;
            }
        }
        return temp_lock;
    }
    void postprocess(UNUSED buf_lock_t *lock) { }
};

bool blob_t::allocate_to_dimensions(buf_parent_t parent, int levels,
                                    int64_t new_size) {
    allocate_helper_t helper;
    return traverse_to_dimensions(parent, levels,
                                  valuesize(), new_size, &helper);
}

struct deallocate_helper_t : public blob::traverse_helper_t {
    buf_lock_t preprocess(buf_parent_t parent, UNUSED int levels,
                          unaligned<block_id_t> *block_id) {
        return buf_lock_t(parent, block_id->copy(), access_t::write);
    }

    void postprocess(buf_lock_t *lock) {
        // mark_deleted() requires that we have already got the write lock.
        // Make sure that's the case before we call it...
        lock->write_acq_signal()->wait_lazily_unordered();
        lock->mark_deleted();
    }
};

void blob_t::deallocate_to_dimensions(buf_parent_t parent, int levels,
                                      int64_t new_size) {
    deallocate_helper_t helper;
    if (levels == 0) {
        rassert(0 <= new_size);
        rassert(new_size <= blob::small_size(ref_, maxreflen_));
        blob::set_small_size(ref_, maxreflen_, new_size);
    } else {
        DEBUG_VAR bool res = traverse_to_dimensions(parent, levels, new_size, valuesize(), &helper);
        blob::set_big_size(ref_, maxreflen_, new_size);
        rassert(res);
    }
}

// Always returns levels + 1.
int blob_t::add_level(buf_parent_t parent, int levels) {
    buf_lock_t lock(parent, alt_create_t::create, block_type_t::aux);
    buf_write_t lock_write(&lock);
    void *b = lock_write.get_data_write();
    if (levels == 0) {
        *reinterpret_cast<block_magic_t *>(b) = blob::leaf_node_magic;

        int sz = blob::small_size(ref_, maxreflen_);
        rassert(sz < maxreflen_);

        memcpy(blob::leaf_node_data(b), blob::small_buffer(ref_, maxreflen_), sz);

        blob::set_small_size_field(ref_, maxreflen_, maxreflen_);
        blob::set_big_size(ref_, maxreflen_, sz);
        blob::block_ids(ref_, maxreflen_)[0].value = lock.block_id();
    } else {
        *reinterpret_cast<block_magic_t *>(b) = blob::internal_node_magic;

        // We don't know how many block ids there could be, so we'll
        // just copy as many as there can be.
        int sz = maxreflen_ - blob::block_ids_offset(maxreflen_);

        memcpy(blob::internal_node_block_ids(b), blob::block_ids(ref_, maxreflen_), sz);
        blob::block_ids(ref_, maxreflen_)[0].value = lock.block_id();
    }

    return levels + 1;
}


bool blob_t::remove_level(buf_parent_t parent, int *levels_ref) {
    const max_block_size_t block_size = parent.cache()->max_block_size();
    int levels = *levels_ref;
    if (levels == 0) {
        return false;
    }

    int64_t bigsize = blob::big_size(ref_, maxreflen_);
    rassert(0 <= bigsize);
    if (bigsize > blob::max_end_offset(parent.cache()->max_block_size(),
                                       levels - 1, maxreflen_)) {
        return false;
    }

    // If the size is zero, the entire tree will have already been deallocated.  (But
    // we don't do this, at this precise moment..)
    rassert(bigsize != 0);
    if (bigsize != 0) {

        buf_lock_t lock(parent, blob::block_ids(ref_, maxreflen_)[0].copy(),
                        access_t::write);
        if (levels == 1) {
            buf_read_t lock_read(&lock);
            uint32_t unused_block_size;
            const char *b = blob::leaf_node_data(lock_read.get_data_read(&unused_block_size));
            char *small = blob::small_buffer(ref_, maxreflen_);
            memcpy(small, b, bigsize);
            blob::set_small_size(ref_, maxreflen_, bigsize);
        } else {
            buf_read_t lock_read(&lock);
            const unaligned<block_id_t> *b = blob::internal_node_block_ids(lock_read.get_data_read());

            // Detach children: they're getting reattached to `parent`.
            int lo;
            int hi;
            blob::compute_acquisition_offsets(block_size, levels - 1,
                                              0, bigsize,
                                              &lo, &hi);

            // We must overwrite at least the first block ID, or we might end up
            // processing it a second time when `remove_level` gets called again.
            guarantee(lo == 0 && hi > 0);

            unaligned<block_id_t> *block_ids = blob::block_ids(ref_, maxreflen_);
            for (int i = lo; i < hi; ++i) {
                lock.detach_child(b[i].copy());
                block_ids[i] = b[i];
            }
        }

        lock.write_acq_signal()->wait_lazily_unordered();
        lock.mark_deleted();
    }

    *levels_ref = levels - 1;
    return true;
}


