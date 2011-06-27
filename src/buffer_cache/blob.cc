#include "buffer_cache/blob.hpp"

#include "buffer_cache/buf_lock.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "serializer/types.hpp"
#include "containers/buffer_group.hpp"

namespace blob {

size_t big_size_offset(size_t maxreflen) {
    return maxreflen <= 255 ? 1 : 2;
}

size_t big_offset_offset(size_t maxreflen) {
    return big_size_offset(maxreflen) + sizeof(int64_t);
}

size_t block_ids_offset(size_t maxreflen) {
    return big_offset_offset(maxreflen) + sizeof(int64_t);
}

temporary_acq_tree_node_t *make_tree_from_block_ids(transaction_t *txn, access_t mode, int levels, int64_t offset, int64_t size, const block_id_t *block_ids);
void expose_tree_from_block_ids(transaction_t *txn, access_t mode, int levels, int64_t offset, int64_t size, temporary_acq_tree_node_t *tree, buffer_group_t *buffer_group_out, blob_acq_t *acq_group_out);


size_t small_size(const char *ref, size_t maxreflen) {
    // small value sizes range from 0 to maxreflen - big_size_offset(maxreflen), and maxreflen indicates large value.
    if (maxreflen <= 255) {
        return *reinterpret_cast<const uint8_t *>(ref);
    } else {
        return *reinterpret_cast<const uint16_t *>(ref);
    }
}

bool size_would_be_small(size_t proposed_size, size_t maxreflen) {
    return proposed_size <= maxreflen - big_size_offset(maxreflen);
}

void set_small_size_field(char *ref, size_t maxreflen, size_t size) {
    if (maxreflen <= 255) {
        *reinterpret_cast<uint8_t *>(ref) = size;
    } else {
        *reinterpret_cast<uint16_t *>(ref) = size;
    }
}

void set_small_size(char *ref, size_t maxreflen, size_t size) {
    rassert(size_would_be_small(size, maxreflen));
    set_small_size_field(ref, maxreflen, size);
}

bool is_small(const char *ref, size_t maxreflen) {
    return small_size(ref, maxreflen) < maxreflen;
}

char *small_buffer(char *ref, size_t maxreflen) {
    return ref + big_size_offset(maxreflen);
}

int64_t big_size(const char *ref, size_t maxreflen) {
    return *reinterpret_cast<const int64_t *>(ref + big_size_offset(maxreflen));
}

void set_big_size(char *ref, size_t maxreflen, int64_t new_size) {
    *reinterpret_cast<int64_t *>(ref + big_size_offset(maxreflen)) = new_size;
}

void set_big_offset(char *ref, size_t maxreflen, int64_t new_offset) {
    *reinterpret_cast<int64_t *>(ref + big_offset_offset(maxreflen)) = new_offset;
}

int64_t big_offset(const char *ref, size_t maxreflen) {
    return *reinterpret_cast<const int64_t *>(ref + big_offset_offset(maxreflen));
}

block_id_t *block_ids(char *ref, size_t maxreflen) {
    return reinterpret_cast<block_id_t *>(ref + block_ids_offset(maxreflen));
}

int64_t leaf_size(block_size_t block_size) {
    return block_size.value() - sizeof(block_magic_t);
}

char *leaf_node_data(void *buf) {
    return reinterpret_cast<char *>(buf) + sizeof(block_magic_t);
}

const char *leaf_node_data(const void *buf) {
    return reinterpret_cast<const char *>(buf) + sizeof(block_magic_t);
}

int64_t internal_node_count(block_size_t block_size) {
    return (block_size.value() - sizeof(block_magic_t)) / sizeof(block_id_t);
}

const block_id_t *internal_node_block_ids(const void *buf) {
    return reinterpret_cast<const block_id_t *>(reinterpret_cast<const char *>(buf) + sizeof(block_magic_t));
}

block_id_t *internal_node_block_ids(void *buf) {
    return reinterpret_cast<block_id_t *>(reinterpret_cast<char *>(buf) + sizeof(block_magic_t));
}

struct ref_info_t {
    size_t refsize;
    int levels;
};

ref_info_t big_ref_info(block_size_t block_size, int64_t offset, int64_t size, size_t maxreflen) {
    rassert(size > int64_t(maxreflen - big_size_offset(maxreflen)));
    int64_t max_blockid_count = (maxreflen - block_ids_offset(maxreflen)) / sizeof(block_id_t);

    int64_t block_count = ceil_divide(size + offset, leaf_size(block_size));

    int levels = 1;
    while (block_count > max_blockid_count) {
        block_count = ceil_divide(block_count, internal_node_count(block_size));
        ++levels;
    }

    ref_info_t info;
    info.refsize = block_ids_offset(maxreflen) + sizeof(block_id_t) * block_count;
    info.levels = levels;
    return info;
}

ref_info_t ref_info(block_size_t block_size, const char *ref, size_t maxreflen) {
    size_t smallsize = small_size(ref, maxreflen);
    if (smallsize <= maxreflen - big_size_offset(maxreflen)) {
        ref_info_t info;
	info.refsize = big_size_offset(maxreflen) + smallsize;
	info.levels = 0;
        return info;
    } else {
        return big_ref_info(block_size, big_offset(ref, maxreflen), big_size(ref, maxreflen), maxreflen);
    }
}

size_t ref_size(block_size_t block_size, const char *ref, size_t maxreflen) {
    return ref_info(block_size, ref, maxreflen).refsize;
}

size_t ref_value_offset(const char *ref, size_t maxreflen) {
    return is_small(ref, maxreflen) ? 0 : big_offset(ref, maxreflen);
}

int64_t stepsize(block_size_t block_size, int levels) {
    rassert(levels > 0);
    int64_t step = leaf_size(block_size);
    for (int i = 0; i < levels - 1; ++i) {
        step *= internal_node_count(block_size);
    }
    return step;
}

int64_t max_end_offset(block_size_t block_size, int levels, size_t maxreflen) {
    if (levels == 0) {
        return maxreflen - big_size_offset(maxreflen);
    } else {
        size_t ref_block_ids = (maxreflen - block_ids_offset(maxreflen)) / sizeof(block_id_t);
        return stepsize(block_size, levels) * ref_block_ids;
    }
}

int64_t clamp(int64_t x, int64_t lo, int64_t hi) {
    return x < lo ? lo : x > hi ? hi : x;
}

void shrink(block_size_t block_size, int levels, int64_t offset, int64_t size, int index, int64_t *suboffset_out, int64_t *subsize_out) {
    int64_t step = stepsize(block_size, levels);

    int64_t clamp_low = index * step;
    int64_t clamp_high = clamp_low + step;

    int64_t suboffset = clamp(offset, clamp_low, clamp_high);
    int64_t subsize = clamp(offset + size, clamp_low, clamp_high) - suboffset;

    *suboffset_out = suboffset - clamp_low;
    *subsize_out = subsize;
}

void compute_acquisition_offsets(block_size_t block_size, int levels, int64_t offset, int64_t size, int *lo_out, int *hi_out) {
    int64_t step = stepsize(block_size, levels);

    *lo_out = offset / step;
    *hi_out = ceil_divide(offset + size, step);
}

}  // namespace blob

blob_t::blob_t(block_size_t block_size, const char *ref, size_t maxreflen)
    : ref_(reinterpret_cast<char *>(malloc(maxreflen))), maxreflen_(maxreflen) {
    rassert(maxreflen >= blob::block_ids_offset(maxreflen) + sizeof(block_id_t));
    memcpy(ref_, ref, blob::ref_size(block_size, ref, maxreflen));
}

void blob_t::dump_ref(block_size_t block_size, char *ref_out, size_t confirm_maxreflen) {
    guarantee(maxreflen_ == confirm_maxreflen);
    memcpy(ref_out, ref_, blob::ref_size(block_size, ref_, maxreflen_));
}

blob_t::~blob_t() {
    free(ref_);
}



size_t blob_t::refsize(block_size_t block_size) const {
    return blob::ref_size(block_size, ref_, maxreflen_);
}

int64_t blob_t::valuesize() const {
    size_t smallsize = blob::small_size(ref_, maxreflen_);
    if (smallsize <= maxreflen_ - 1) {
        return smallsize;
    } else {
        rassert(smallsize == maxreflen_);
        return blob::big_size(ref_, maxreflen_);
    }
    return 0;
}

union temporary_acq_tree_node_t {
    buf_t *buf;
    temporary_acq_tree_node_t *child;
};

void blob_t::expose_region(transaction_t *txn, access_t mode, int64_t offset, int64_t size, buffer_group_t *buffer_group_out, blob_acq_t *acq_group_out) {
    if (size == 0) {
        return;
    }

    if (blob::is_small(ref_, maxreflen_)) {
        char *b = blob::small_buffer(ref_, maxreflen_);
        size_t n = blob::small_size(ref_, maxreflen_);
        rassert(0 <= offset && size_t(offset) <= n);
        rassert(0 <= size && size_t(size) <= n && size_t(offset + size) <= n);
        buffer_group_out->add_buffer(size, b + offset);
    } else {
        // It's large.

        int levels = blob::ref_info(txn->get_cache()->get_block_size(), ref_, maxreflen_).levels;

        int64_t real_offset = blob::big_offset(ref_, maxreflen_) + offset;

        // Acquiring is done recursively in parallel,
        temporary_acq_tree_node_t *tree = blob::make_tree_from_block_ids(txn, mode, levels, real_offset, size, blob::block_ids(ref_, maxreflen_));

        // Exposing and writing to the buffer group is done serially.
        blob::expose_tree_from_block_ids(txn, mode, levels, real_offset, size, tree, buffer_group_out, acq_group_out);
    }
}

namespace blob {

struct region_tree_filler_t {
    transaction_t *txn;
    access_t mode;
    int levels;
    int64_t offset, size;
    const block_id_t *block_ids;
    int lo, hi;
    temporary_acq_tree_node_t *nodes;

    void operator()(int i) const {
        buf_lock_t lock(txn, block_ids[lo + i], mode);
        if (levels > 1) {
            const block_id_t *sub_ids = blob::internal_node_block_ids(lock->get_data_read());

            int64_t suboffset, subsize;
            shrink(txn->get_cache()->get_block_size(), levels, offset, size, lo + i, &suboffset, &subsize);
            nodes[i].child = blob::make_tree_from_block_ids(txn, mode, levels - 1, suboffset, subsize, sub_ids);
        } else {
            nodes[i].buf = lock.give_up_ownership();
        }
    }
};

temporary_acq_tree_node_t *make_tree_from_block_ids(transaction_t *txn, access_t mode, int levels, int64_t offset, int64_t size, const block_id_t *block_ids) {
    rassert(size > 0);

    region_tree_filler_t filler;
    filler.txn = txn;
    filler.mode = mode;
    filler.levels = levels;
    filler.offset = offset;
    filler.size = size;
    filler.block_ids = block_ids;
    compute_acquisition_offsets(txn->get_cache()->get_block_size(), levels, offset, size, &filler.lo, &filler.hi);

    filler.nodes = new temporary_acq_tree_node_t[filler.hi - filler.lo];

    pmap(filler.hi - filler.lo, filler);

    return filler.nodes;
}

void expose_tree_from_block_ids(transaction_t *txn, access_t mode, int levels, int64_t offset, int64_t size, temporary_acq_tree_node_t *tree, buffer_group_t *buffer_group_out, blob_acq_t *acq_group_out) {
    rassert(size > 0);

    int lo, hi;
    blob::compute_acquisition_offsets(txn->get_cache()->get_block_size(), levels, offset, size, &lo, &hi);

    for (int i = 0; i < hi - lo; ++i) {
        int64_t suboffset, subsize;
        blob::shrink(txn->get_cache()->get_block_size(), levels, offset, size, lo + i, &suboffset, &subsize);
        if (levels > 1) {
            expose_tree_from_block_ids(txn, mode, levels - 1, suboffset, subsize, tree[i].child, buffer_group_out, acq_group_out);
        } else {
            rassert(0 < subsize && subsize <= blob::leaf_size(txn->get_cache()->get_block_size()));
            rassert(0 <= suboffset && suboffset + subsize <= blob::leaf_size(txn->get_cache()->get_block_size()));

            buf_t *buf = tree[i].buf;
            void *leaf_buf;
            if (is_read_mode(mode)) {
                leaf_buf = const_cast<void *>(buf->get_data_read());
            } else {
                leaf_buf = buf->get_data_major_write();
            }

            char *data = blob::leaf_node_data(leaf_buf);

            acq_group_out->add_buf(buf);
            buffer_group_out->add_buffer(subsize, data + suboffset);
        }
    }

    delete[] tree;
}

}  // namespace blob

void blob_t::append_region(transaction_t *txn, int64_t size) {
    block_size_t block_size = txn->get_cache()->get_block_size();
    int levels = blob::ref_info(block_size, ref_, maxreflen_).levels;

    // Avoid the empty blob effect.
    if (levels == 0 && blob::small_size(ref_, maxreflen_) == 0 && size > 0) {
        blob::set_small_size(ref_, maxreflen_, 1);
        size -= 1;
    }
    if (size == 0) {
        return;
    }

    while (!allocate_to_dimensions(txn, levels, blob::ref_value_offset(ref_, maxreflen_), valuesize() + size)) {
        levels = add_level(txn, levels);
    }

    if (levels == 0) {
        rassert(blob::is_small(ref_, maxreflen_));
        size_t new_size = size + blob::small_size(ref_, maxreflen_);
        rassert(blob::size_would_be_small(new_size, maxreflen_));
        blob::set_small_size(ref_, maxreflen_, new_size);
    } else {
        blob::set_big_size(ref_, maxreflen_, blob::big_size(ref_, maxreflen_) + size);
    }

    rassert(blob::ref_info(block_size, ref_, maxreflen_).levels == levels);
}

void blob_t::prepend_region(transaction_t *txn, int64_t size) {
    block_size_t block_size = txn->get_cache()->get_block_size();
    int levels = blob::ref_info(block_size, ref_, maxreflen_).levels;

    if (levels == 0) {
        size_t small_size = blob::small_size(ref_, maxreflen_);
        if (small_size + size <= maxreflen_ - blob::big_size_offset(maxreflen_)) {
            char *buf = blob::small_buffer(ref_, maxreflen_);
            memmove(buf + size, buf, small_size);
            blob::set_small_size(ref_, maxreflen_, small_size + size);
            return;
        }
    }

    // Avoid the empty blob effect.
    if (levels == 0 && blob::small_size(ref_, maxreflen_) == 0 && size > 0) {
        blob::set_small_size(ref_, maxreflen_, 1);
        size -= 1;
    }

    for (;;) {
        if (!shift_at_least(txn, levels, std::max<int64_t>(0, - (blob::ref_value_offset(ref_, maxreflen_) - size)))) {
            levels = add_level(txn, levels);
        } else if (!allocate_to_dimensions(txn, levels, blob::ref_value_offset(ref_, maxreflen_) - size, valuesize() + size)) {
            levels = add_level(txn, levels);
        } else {
            break;
        }
    }

    rassert(levels > 0);
    int64_t final_offset = blob::big_offset(ref_, maxreflen_) - size;
    rassert(final_offset >= 0);
    blob::set_big_offset(ref_, maxreflen_, final_offset);
    blob::set_big_size(ref_, maxreflen_, blob::big_size(ref_, maxreflen_) + size);

    rassert(blob::ref_info(block_size, ref_, maxreflen_).levels == levels);
}

void blob_t::unappend_region(transaction_t *txn, int64_t size) {
    block_size_t block_size = txn->get_cache()->get_block_size();
    int levels = blob::ref_info(block_size, ref_, maxreflen_).levels;

    bool emptying = false;
    if (valuesize() == size && size > 0) {
        emptying = true;
        size -= 1;
    }

    if (size != 0) {
        deallocate_to_dimensions(txn, levels, blob::ref_value_offset(ref_, maxreflen_), valuesize() - size);
        for (;;) {
            shift_at_least(txn, levels, - blob::ref_value_offset(ref_, maxreflen_));

            if (!remove_level(txn, &levels)) {
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

void blob_t::unprepend_region(transaction_t *txn, int64_t size) {
    block_size_t block_size = txn->get_cache()->get_block_size();
    int levels = blob::ref_info(block_size, ref_, maxreflen_).levels;

    bool emptying = false;
    if (valuesize() == size && size > 0) {
        emptying = true;
        size -= 1;
    }

    if (size != 0) {
        deallocate_to_dimensions(txn, levels, blob::ref_value_offset(ref_, maxreflen_) + size, valuesize() - size);
        for (;;) {
            shift_at_least(txn, levels, - blob::ref_value_offset(ref_, maxreflen_));
            if (!remove_level(txn, &levels)) {
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

namespace blob {

struct traverse_helper_t {
    virtual void preprocess(transaction_t *txn, int levels, buf_lock_t& lock, block_id_t *block_id) = 0;
    virtual void postprocess(buf_lock_t& lock) = 0;
    virtual ~traverse_helper_t() { }
};

void traverse_recursively(transaction_t *txn, int levels, block_id_t *block_ids, int64_t old_offset, int64_t old_size, int64_t new_offset, int64_t new_size, traverse_helper_t *helper);

void traverse_index(transaction_t *txn, int levels, block_id_t *block_ids, int index, int64_t old_offset, int64_t old_size, int64_t new_offset, int64_t new_size, traverse_helper_t *helper) {
    block_size_t block_size = txn->get_cache()->get_block_size();
    int64_t sub_old_offset, sub_old_size, sub_new_offset, sub_new_size;
    blob::shrink(block_size, levels, old_offset, old_size, index, &sub_old_offset, &sub_old_size);
    blob::shrink(block_size, levels, new_offset, new_size, index, &sub_new_offset, &sub_new_size);

    if (sub_old_size > 0) {
        if (levels > 1) {
            buf_lock_t lock(txn, block_ids[index], rwi_write);
            void *b = lock->get_data_major_write();

            block_id_t *subids = blob::internal_node_block_ids(b);
            traverse_recursively(txn, levels - 1, subids, sub_old_offset, sub_old_size, sub_new_offset, sub_new_size, helper);
        }

    } else {
        buf_lock_t lock;
        //        debugf("preprocess levels = %d, index = %d\n", levels, index);
        helper->preprocess(txn, levels, lock, &block_ids[index]);

        if (levels > 1) {
            void *b = lock->get_data_major_write();
            block_id_t *subids = blob::internal_node_block_ids(b);
            traverse_recursively(txn, levels - 1, subids, sub_old_offset, sub_old_size, sub_new_offset, sub_new_size, helper);
        }

        helper->postprocess(lock);
    }
}

void traverse_recursively(transaction_t *txn, int levels, block_id_t *block_ids, int64_t old_offset, int64_t old_size, int64_t new_offset, int64_t new_size, traverse_helper_t *helper) {
    block_size_t block_size = txn->get_cache()->get_block_size();

    int old_lo, old_hi, new_lo, new_hi;
    compute_acquisition_offsets(block_size, levels, old_offset, old_size, &old_lo, &old_hi);
    compute_acquisition_offsets(block_size, levels, new_offset, new_size, &new_lo, &new_hi);

    int64_t leafsize = leaf_size(block_size);

    // This highest_i variable makes sure, rigorously, that the two
    // for loops don't trace the same subtree twice.
    int highest_i = -1;

    //    debugf("at levels=%d, old_lo=%d, old_hi=%d, new_lo=%d, new_hi=%d\n", levels, old_lo, old_hi, new_lo, new_hi);
    if (new_offset / leafsize < old_offset / leafsize) {
        // See comment [1] below.
        for (int i = new_lo; i <= old_lo && i < new_hi; ++i) {
            //            debugf("calling traverse_index (lo) levels=%d, i=%d, %ld %ld %ld %ld\n", levels, i, old_offset, old_size, new_offset, new_size);
            traverse_index(txn, levels, block_ids, i, old_offset, old_size, new_offset, new_size, helper);
            highest_i = i;
        }
    }
    if (ceil_divide(new_offset + new_size, leafsize) > ceil_divide(old_offset + old_size, leafsize)) {
        for (int i = std::max(highest_i + 1, old_hi - 1); i < new_hi; ++i) {
            //            debugf("calling traverse_index (hi) levels=%d, i=%d, %ld %ld %ld %ld\n", levels, i, old_offset, old_size, new_offset, new_size);
            traverse_index(txn, levels, block_ids, i, old_offset, old_size, new_offset, new_size, helper);
        }
    }

    // [1] We have a problem that if old_offset is on a multiple of
    // the step size, we'll visit the left edge of the completely
    // acquired branch.  However, when old_lo = 1020, we'd actually be
    // walking past the end of the value.  We make the mistake in the
    // design of this function of allowing old_size to be zero and
    // trying to handle such cases.  The check that i < new_hi
    // prevents us from walking off the edge when old_lo = 1020,
    // because new_hi cannot be more than 1020.
    //
    // Really, we should require that old_size be greater than zero.
}

}  // namespace

bool blob_t::traverse_to_dimensions(transaction_t *txn, int levels, int64_t old_offset, int64_t old_size, int64_t new_offset, int64_t new_size, blob::traverse_helper_t *helper) {
    int64_t old_end = old_offset + old_size;
    int64_t new_end = new_offset + new_size;
    rassert(new_offset <= old_offset && new_end >= old_end);
    block_size_t block_size = txn->get_cache()->get_block_size();
    if (new_offset >= 0 && new_end <= blob::max_end_offset(block_size, levels, maxreflen_)) {
        if (levels != 0) {
            blob::traverse_recursively(txn, levels, blob::block_ids(ref_, maxreflen_), old_offset, old_size, new_offset, new_size, helper);
        }
        return true;
    } else {
        return false;
    }
}

struct allocate_helper_t : public blob::traverse_helper_t {
    void preprocess(transaction_t *txn, int levels, buf_lock_t& lock, block_id_t *block_id) {
        lock.allocate(txn);
        *block_id = lock->get_block_id();
        void *b = lock->get_data_major_write();
        if (levels == 1) {
            block_magic_t leafmagic = { { 'l', 'a', 'r', 'l' } };
            *reinterpret_cast<block_magic_t *>(b) = leafmagic;
        } else {
            block_magic_t internalmagic = { { 'l', 'a', 'r', 'i' } };
            *reinterpret_cast<block_magic_t *>(b) = internalmagic;
        }
    }
    void postprocess(UNUSED buf_lock_t& lock) { }
};

bool blob_t::allocate_to_dimensions(transaction_t *txn, int levels, int64_t new_offset, int64_t new_size) {
    allocate_helper_t helper;
    return traverse_to_dimensions(txn, levels, blob::ref_value_offset(ref_, maxreflen_), valuesize(), new_offset, new_size, &helper);
}

struct deallocate_helper_t : public blob::traverse_helper_t {
    void preprocess(transaction_t *txn, UNUSED int levels, buf_lock_t& lock, block_id_t *block_id) {
        buf_lock_t tmp(txn, *block_id, rwi_write);
        lock.swap(tmp);
    }

    void postprocess(buf_lock_t& lock) {
        lock->mark_deleted();
    }
};

void blob_t::deallocate_to_dimensions(transaction_t *txn, int levels, int64_t new_offset, int64_t new_size) {
    deallocate_helper_t helper;
    if (levels == 0) {
        rassert(0 <= new_offset && 0 <= new_size);
        rassert(new_offset + new_size <= int64_t(blob::small_size(ref_, maxreflen_)));
        char *buf = blob::small_buffer(ref_, maxreflen_);
        memmove(buf, buf + new_offset, new_size);
        blob::set_small_size(ref_, maxreflen_, new_size);
    } else {
        UNUSED bool res = traverse_to_dimensions(txn, levels, new_offset, new_size, blob::ref_value_offset(ref_, maxreflen_), valuesize(), &helper);
        blob::set_big_offset(ref_, maxreflen_, new_offset);
        blob::set_big_size(ref_, maxreflen_, new_size);
        rassert(res);
    }
}


bool blob_t::shift_at_least(transaction_t *txn, int levels, int64_t min_shift) {
    if (levels == 0) {
        // Can't change offset if there are zero levels.  Deal with it.
        return false;
    }

    int64_t shift = min_shift;
    consider_small_shift(txn, levels, &shift);
    consider_big_shift(txn, levels, &shift);
    consider_small_shift(txn, levels, &shift);
    return shift <= 0;
}

void blob_t::consider_big_shift(transaction_t *txn, int levels, int64_t *min_shift) {
    block_size_t block_size = txn->get_cache()->get_block_size();
    int64_t step = blob::stepsize(block_size, levels);

    int64_t offset = blob::big_offset(ref_, maxreflen_);
    int64_t size = blob::big_size(ref_, maxreflen_);

    int64_t practical_offset = floor_aligned(offset, step);
    int64_t practical_end_offset = ceil_aligned(offset + size, step);

    int64_t min_conceivable_shift = std::max(- practical_offset, ceil_modulo(*min_shift, step));
    if (min_conceivable_shift != 0) {
        if (practical_end_offset + min_conceivable_shift <= blob::max_end_offset(block_size, levels, maxreflen_)) {
            block_id_t *ids = blob::block_ids(ref_, maxreflen_);

            size_t steps_to_shift = min_conceivable_shift / step;
            size_t beg = practical_offset / step;
            size_t end = practical_end_offset / step;
            memmove(ids + beg + steps_to_shift, ids + beg, (end - beg) * sizeof(block_id_t));

            blob::set_big_offset(ref_, maxreflen_, offset + min_conceivable_shift);
            *min_shift -= min_conceivable_shift;
        }
    }
}

void blob_t::consider_small_shift(transaction_t *txn, int levels, int64_t *min_shift) {
    block_size_t block_size = txn->get_cache()->get_block_size();
    int64_t step = blob::stepsize(block_size, levels);
    int64_t substep = levels == 1 ? 1 : blob::stepsize(block_size, levels - 1);

    int64_t offset = blob::big_offset(ref_, maxreflen_);
    int64_t size = blob::big_size(ref_, maxreflen_);

    int64_t practical_offset = floor_aligned(offset, substep);
    int64_t practical_end_offset = ceil_aligned(offset + size, substep);

    int64_t min_conceivable_shift = std::max(- practical_offset, ceil_modulo(*min_shift, substep));
    if (min_conceivable_shift != 0) {
        if (practical_end_offset + min_conceivable_shift <= step) {
            if (practical_offset < step && practical_end_offset <= 2 * step) {
                block_id_t *ids = blob::block_ids(ref_, maxreflen_);
                buf_lock_t lobuf(txn, ids[0], rwi_write);

                size_t physical_shift = (min_conceivable_shift / substep) * (levels == 1 ? 1 : sizeof(block_id_t));
                size_t physical_offset = (practical_offset / substep) * (levels == 1 ? 1 : sizeof(block_id_t));
                size_t physical_end_offset = (practical_end_offset / substep) * (levels == 1 ? 1 : sizeof(block_id_t));

                char *data = blob::leaf_node_data(lobuf->get_data_major_write());
                size_t low_copycount = std::min(physical_end_offset, size_t(blob::leaf_size(block_size))) - physical_offset;
                memmove(data + physical_offset + physical_shift, data + physical_offset, low_copycount);

                if (physical_end_offset > size_t(blob::leaf_size(block_size))) {
                    buf_lock_t hibuf(txn, ids[1], rwi_write);
                    const char *data2 = blob::leaf_node_data(hibuf->get_data_read());

                    memmove(data + physical_offset + physical_shift + low_copycount, data2, physical_end_offset - blob::leaf_size(block_size));
                    hibuf->mark_deleted();
                }

                blob::set_big_offset(ref_, maxreflen_, offset + min_conceivable_shift);
                *min_shift -= min_conceivable_shift;
            }
        }
    }
}

// Always returns levels + 1.
int blob_t::add_level(transaction_t *txn, int levels) {
    buf_lock_t lock;
    lock.allocate(txn);
    void *b = lock->get_data_major_write();
    if (levels == 0) {
        block_magic_t leafmagic = { { 'l', 'a', 'r', 'l' } };
        *reinterpret_cast<block_magic_t *>(b) = leafmagic;

        size_t sz = blob::small_size(ref_, maxreflen_);
        rassert(sz < maxreflen_);

        memcpy(blob::leaf_node_data(b), blob::small_buffer(ref_, maxreflen_), sz);

        blob::set_small_size_field(ref_, maxreflen_, maxreflen_);
        blob::set_big_offset(ref_, maxreflen_, 0);
        blob::set_big_size(ref_, maxreflen_, sz);
        blob::block_ids(ref_, maxreflen_)[0] = lock->get_block_id();
    } else {
        block_magic_t internalmagic = { { 'l', 'a', 'r', 'i' } };
        *reinterpret_cast<block_magic_t *>(b) = internalmagic;

        // We don't know how many block ids there could be, so we'll
        // just copy as many as there can be.
        size_t sz = maxreflen_ - blob::block_ids_offset(maxreflen_);

        memcpy(blob::internal_node_block_ids(b), blob::block_ids(ref_, maxreflen_), sz);
        blob::block_ids(ref_, maxreflen_)[0] = lock->get_block_id();
    }

    return levels + 1;
}


bool blob_t::remove_level(transaction_t *txn, int *levels_ref) {
    int levels = *levels_ref;
    if (levels == 0) {
        return false;
    }

    int64_t bigoffset = blob::big_offset(ref_, maxreflen_);
    int64_t bigsize = blob::big_size(ref_, maxreflen_);
    rassert(0 <= bigsize);
    int64_t end_offset = bigoffset + bigsize;
    if (end_offset > blob::max_end_offset(txn->get_cache()->get_block_size(), levels - 1, maxreflen_)) {
        return false;
    }

    // If the size is zero and the offset is zero, the entire tree will have already been deallocated.
    rassert(!(bigoffset == end_offset && end_offset == blob::stepsize(txn->get_cache()->get_block_size(), levels)));
    if (!(bigoffset == end_offset && end_offset == 0)) {

        buf_lock_t lock(txn, blob::block_ids(ref_, maxreflen_)[0], rwi_write);
        if (levels == 1) {
            // We should tried shifting before removing a level.
            rassert(bigoffset == 0);

            const char *b = blob::leaf_node_data(lock->get_data_read());
            memcpy(blob::small_buffer(ref_, maxreflen_), b, maxreflen_ - blob::big_size_offset(maxreflen_));
            blob::set_small_size(ref_, maxreflen_, bigsize);
        } else {
            const block_id_t *b = blob::internal_node_block_ids(lock->get_data_read());
            memcpy(blob::block_ids(ref_, maxreflen_), b, maxreflen_ - blob::block_ids_offset(maxreflen_));
        }

        lock->mark_deleted();
    }

    *levels_ref = levels - 1;
    return true;
}
