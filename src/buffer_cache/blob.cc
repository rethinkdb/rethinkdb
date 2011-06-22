#include "buffer_cache/blob.hpp"

#include "buffer_cache/buf_lock.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "serializer/types.hpp"
#include "containers/buffer_group.hpp"

#define BIG_SIZE_OFFSET(maxreflen) ((maxreflen) <= 255 ? 1 : 2)
#define BIG_OFFSET_OFFSET(maxreflen) (BIG_SIZE_OFFSET(maxreflen) + sizeof(int64_t))
#define BLOCK_IDS_OFFSET(maxreflen) (BIG_OFFSET_OFFSET(maxreflen) + sizeof(int64_t))

temporary_acq_tree_node_t *make_tree_from_block_ids(transaction_t *txn, access_t mode, int levels, int64_t offset, int64_t size, const block_id_t *block_ids);
void expose_tree_from_block_ids(transaction_t *txn, access_t mode, int levels, int64_t offset, int64_t size, temporary_acq_tree_node_t *tree, buffer_group_t *buffer_group_out, blob_acq_t *acq_group_out);


size_t small_size(const char *ref, size_t maxreflen) {
    // small value sizes range from 0 to maxreflen - BIG_SIZE_OFFSET(maxreflen), and maxreflen indicates large value.
    if (maxreflen <= 255) {
        return *reinterpret_cast<const uint8_t *>(ref);
    } else {
        return *reinterpret_cast<const uint16_t *>(ref);
    }
}

bool size_would_be_small(size_t proposed_size, size_t maxreflen) {
    return proposed_size <= maxreflen - BIG_SIZE_OFFSET(maxreflen);
}

void set_small_size(char *ref, size_t size, size_t maxreflen) {
    rassert(size_would_be_small(size, maxreflen));
    if (maxreflen <= 255) {
        *reinterpret_cast<uint8_t *>(ref) = size;
    } else {
        *reinterpret_cast<uint16_t *>(ref) = size;
    }
}

bool is_small(const char *ref, size_t maxreflen) {
    return small_size(ref, maxreflen) < maxreflen;
}

char *small_buffer(char *ref, size_t maxreflen) {
    return ref + BIG_SIZE_OFFSET(maxreflen);
}

int64_t big_size(const char *ref, size_t maxreflen) {
    return *reinterpret_cast<const int64_t *>(ref + BIG_SIZE_OFFSET(maxreflen));
}

void set_big_size(char *ref, size_t maxreflen, int64_t new_size) {
    *reinterpret_cast<int64_t *>(ref + BIG_SIZE_OFFSET(maxreflen)) = new_size;
}

void set_big_offset(char *ref, size_t maxreflen, int64_t new_offset) {
    *reinterpret_cast<int64_t *>(ref + BIG_OFFSET_OFFSET(maxreflen)) = new_offset;
}

int64_t big_offset(const char *ref, size_t maxreflen) {
    return *reinterpret_cast<const int64_t *>(ref + BIG_OFFSET_OFFSET(maxreflen));
}

block_id_t *block_ids(char *ref, size_t maxreflen) {
    return reinterpret_cast<block_id_t *>(ref + BLOCK_IDS_OFFSET(maxreflen));
}

int64_t leaf_size(block_size_t block_size) {
    return block_size.value() - sizeof(block_magic_t);
}

char *leaf_node_data(void *buf) {
    return reinterpret_cast<char *>(buf) + sizeof(block_magic_t);
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

std::pair<size_t, int> big_ref_info(block_size_t block_size, int64_t offset, int64_t size, size_t maxreflen) {
    rassert(size > int64_t(maxreflen - BIG_SIZE_OFFSET(maxreflen)));
    int64_t max_blockid_count = (maxreflen - BLOCK_IDS_OFFSET(maxreflen)) / sizeof(block_id_t);

    int64_t block_count = ceil_divide(size + offset, leaf_size(block_size));

    int levels = 1;
    while (block_count > max_blockid_count) {
        block_count = ceil_divide(block_count, internal_node_count(block_size));
        ++levels;
    }

    return std::pair<size_t, int>(BLOCK_IDS_OFFSET(maxreflen) + sizeof(block_id_t) * block_count, levels);
}

std::pair<size_t, int> ref_info(block_size_t block_size, const char *ref, size_t maxreflen) {
    size_t smallsize = small_size(ref, maxreflen);
    if (smallsize <= maxreflen - BIG_SIZE_OFFSET(maxreflen)) {
        return std::pair<size_t, int>(BIG_SIZE_OFFSET(maxreflen) + smallsize, 0);
    } else {
        return big_ref_info(block_size, big_offset(ref, maxreflen), big_size(ref, maxreflen), maxreflen);
    }
}

size_t ref_size(block_size_t block_size, const char *ref, size_t maxreflen) {
    return ref_info(block_size, ref, maxreflen).first;
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
        return maxreflen - BIG_SIZE_OFFSET(maxreflen);
    } else {
        size_t ref_block_ids = (maxreflen - BLOCK_IDS_OFFSET(maxreflen)) / sizeof(block_id_t);
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

    *suboffset_out = suboffset;
    *subsize_out = subsize;
}

void compute_acquisition_offsets(block_size_t block_size, int levels, int64_t offset, int64_t size, int *lo_out, int *hi_out) {
    int64_t step = stepsize(block_size, levels);

    *lo_out = offset / step;
    *hi_out = ceil_divide(offset + size, step);
}

blob_t::blob_t(block_size_t block_size, const char *ref, size_t maxreflen)
    : ref_(reinterpret_cast<char *>(malloc(maxreflen))), maxreflen_(maxreflen) {
    rassert(maxreflen >= BLOCK_IDS_OFFSET(maxreflen) + sizeof(block_id_t));
    std::pair<size_t, int> pair = ref_info(block_size, ref, maxreflen);
    memcpy(ref_, ref, pair.first);
}

void blob_t::dump_ref(block_size_t block_size, char *ref_out, size_t confirm_maxreflen) {
    guarantee(maxreflen_ == confirm_maxreflen);
    memcpy(ref_out, ref_, ref_size(block_size, ref_, maxreflen_));
}

blob_t::~blob_t() {
    free(ref_);
}



size_t blob_t::refsize(block_size_t block_size) const {
    return ref_size(block_size, ref_, maxreflen_);
}

int64_t blob_t::valuesize() const {
    size_t smallsize = small_size(ref_, maxreflen_);
    if (smallsize <= maxreflen_ - 1) {
        return smallsize;
    } else {
        rassert(smallsize == maxreflen_);
        return big_size(ref_, maxreflen_);
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

    if (is_small(ref_, maxreflen_)) {
        char *b = small_buffer(ref_, maxreflen_);
        size_t n = small_size(ref_, maxreflen_);
        rassert(0 <= offset && size_t(offset) <= n);
        rassert(0 <= size && size_t(size) <= n && size_t(offset + size) <= n);
        buffer_group_out->add_buffer(size, b + offset);
    } else {
        // It's large.

        int levels = ref_info(txn->get_cache()->get_block_size(), ref_, maxreflen_).second;

        // Acquiring is done recursively in parallel,
        temporary_acq_tree_node_t *tree = make_tree_from_block_ids(txn, mode, levels, offset, size, block_ids(ref_, maxreflen_));

        // Exposing and writing to the buffer group is done serially.
        expose_tree_from_block_ids(txn, mode, levels, offset, size, tree, buffer_group_out, acq_group_out);
    }


    crash("not yet implemented.");
}

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
            const block_id_t *sub_ids = internal_node_block_ids(lock->get_data_read());

            int64_t suboffset, subsize;
            shrink(txn->get_cache()->get_block_size(), levels, offset, size, lo + i, &suboffset, &subsize);
            nodes[i].child = make_tree_from_block_ids(txn, mode, levels - 1, suboffset, subsize, sub_ids);
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
    compute_acquisition_offsets(txn->get_cache()->get_block_size(), levels, offset, size, &lo, &hi);

    for (int i = 0; i < hi - lo; ++i) {
        int64_t suboffset, subsize;
        shrink(txn->get_cache()->get_block_size(), levels, offset, size, lo + i, &suboffset, &subsize);
        if (levels > 1) {
            expose_tree_from_block_ids(txn, mode, levels - 1, suboffset, subsize, tree[i].child, buffer_group_out, acq_group_out);
        } else {
            rassert(0 < subsize && subsize <= leaf_size(txn->get_cache()->get_block_size()));
            rassert(0 <= suboffset && suboffset + subsize <= leaf_size(txn->get_cache()->get_block_size()));

            buf_t *buf = tree[i].buf;
            void *leaf_buf;
            if (is_read_mode(mode)) {
                leaf_buf = const_cast<void *>(buf->get_data_read());
            } else {
                leaf_buf = buf->get_data_major_write();
            }

            char *data = leaf_node_data(leaf_buf);

            acq_group_out->add_buf(buf);
            buffer_group_out->add_buffer(subsize, data + suboffset);
        }
    }

    delete[] tree;
}

void blob_t::append_region(UNUSED transaction_t *txn, UNUSED int64_t size) {
    block_size_t block_size = txn->get_cache()->get_block_size();
    int levels = ref_info(block_size, ref_, maxreflen_).second;
    while (!allocate_to_dimensions(txn, levels, ref_value_offset(ref_, maxreflen_), valuesize() + size)) {
        levels = add_level(txn, levels);
    }

    rassert(ref_info(block_size, ref_, maxreflen_).second == levels);
}

void blob_t::prepend_region(UNUSED transaction_t *txn, UNUSED int64_t size) {
    block_size_t block_size = txn->get_cache()->get_block_size();
    int levels = ref_info(block_size, ref_, maxreflen_).second;
    for (;;) {
        if (!shift_at_least(txn, levels, std::max<int64_t>(0, - (ref_value_offset(ref_, maxreflen_) - size)))) {
            levels = add_level(txn, levels);
        } else if (!allocate_to_dimensions(txn, levels, ref_value_offset(ref_, maxreflen_) - size, valuesize() + size)) {
            levels = add_level(txn, levels);
        }
    }

    rassert(ref_info(block_size, ref_, maxreflen_).second == levels);
}

void blob_t::unappend_region(UNUSED transaction_t *txn, UNUSED int64_t size) {
    block_size_t block_size = txn->get_cache()->get_block_size();
    int levels = ref_info(block_size, ref_, maxreflen_).second;
    deallocate_to_dimensions(txn, levels, ref_value_offset(ref_, maxreflen_), valuesize() - size);
    while (remove_level(txn, &levels)) { }
}

void blob_t::unprepend_region(UNUSED transaction_t *txn, UNUSED int64_t size) {
    block_size_t block_size = txn->get_cache()->get_block_size();
    int levels = ref_info(block_size, ref_, maxreflen_).second;
    deallocate_to_dimensions(txn, levels, ref_value_offset(ref_, maxreflen_) + size, valuesize() - size);
    for (;;) {
        if (!remove_level(txn, &levels)) {
            break;
        } else {
            shift_at_least(txn, levels, - ref_value_offset(ref_, maxreflen_));
        }
    }
}

struct traverse_helper_t {
    virtual void preprocess(transaction_t *txn, int levels, buf_lock_t& lock, block_id_t *block_id) = 0;
    virtual void postprocess(buf_lock_t& lock) = 0;
    virtual ~traverse_helper_t() { }
};

void traverse_recursively(transaction_t *txn, int levels, block_id_t *block_ids, int64_t old_offset, int64_t old_size, int64_t new_offset, int64_t new_size, traverse_helper_t *helper);

void traverse_index(transaction_t *txn, int levels, block_id_t *block_ids, int index, int64_t old_offset, int64_t old_size, int64_t new_offset, int64_t new_size, traverse_helper_t *helper) {
    block_size_t block_size = txn->get_cache()->get_block_size();
    int64_t sub_old_offset, sub_old_size, sub_new_offset, sub_new_size;
    shrink(block_size, levels, old_offset, old_size, index, &sub_old_offset, &sub_old_size);
    shrink(block_size, levels, new_offset, new_size, index, &sub_new_offset, &sub_new_size);

    if (sub_old_size > 0) {
        if (levels > 1) {
            buf_lock_t lock(txn, block_ids[index], rwi_write);
            void *b = lock->get_data_major_write();

            block_id_t *subids = internal_node_block_ids(b);
            traverse_recursively(txn, levels - 1, subids, sub_old_offset, sub_old_size, sub_new_offset, sub_new_size, helper);
        }

    } else {
        buf_lock_t lock;
        helper->preprocess(txn, levels, lock, &block_ids[index]);

        if (levels > 1) {
            void *b = lock->get_data_major_write();
            block_id_t *subids = internal_node_block_ids(b);
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
    if (new_offset / leafsize < old_offset / leafsize) {
        for (int i = new_lo; i <= old_lo; ++i) {
            traverse_index(txn, levels, block_ids, i, old_offset, old_size, new_offset, new_size, helper);
        }
    }
    if (ceil_divide(new_offset + new_size, leafsize) > ceil_divide(old_offset + old_size, leafsize)) {
        for (int i = std::max(old_lo + 1, old_hi - 1); i < new_hi; ++i) {
            traverse_index(txn, levels, block_ids, i, old_offset, old_size, new_offset, new_size, helper);
        }
    }
}

bool blob_t::traverse_to_dimensions(transaction_t *txn, int levels, int64_t old_offset, int64_t old_size, int64_t new_offset, int64_t new_size, traverse_helper_t *helper) {
    int64_t old_end = old_offset + old_size;
    int64_t new_end = new_offset + new_size;
    rassert(new_offset <= old_offset && new_end >= old_end);
    block_size_t block_size = txn->get_cache()->get_block_size();
    if (new_offset >= 0 && new_end <= max_end_offset(block_size, levels, maxreflen_)) {
        if (levels != 0) {
            traverse_recursively(txn, levels, block_ids(ref_, maxreflen_), old_offset, old_size, new_offset, new_size, helper);
        }
        return true;
    } else {
        return false;
    }
}

struct allocate_helper_t : public traverse_helper_t {
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
    return traverse_to_dimensions(txn, levels, ref_value_offset(ref_, maxreflen_), valuesize(), new_offset, new_size, &helper);
}

struct deallocate_helper_t : public traverse_helper_t {
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
    UNUSED bool res = traverse_to_dimensions(txn, levels, new_offset, new_size, ref_value_offset(ref_, maxreflen_), valuesize(), &helper);
    rassert(res);
}


// This function changes the value of ref_value_offset(ref_,
// maxreflen_) while keeping valuesize() constant.  It either adds
// min_shift or more to said value and returns true, or it does
// nothing and returns false.  For example, if min_shift is 1, it
// might add 4080 to the offset.  If min_shift is -3, it might do
// nothing and return true (since 0 is greater than -3).  If min_shift
// is -4100, it might add -4080 to the offset.  Or it might add 0 to
// the offset, doing nothing and returning true.  (The number 4080
// pops up because it's a possible return value of stepsize.)
bool blob_t::shift_at_least(transaction_t *txn, int levels, int64_t min_shift) {
    if (levels == 0) {
        // Can't change offset if there are zero levels.  Deal with it.
        return false;
    }

    block_size_t block_size = txn->get_cache()->get_block_size();
    int64_t step = stepsize(block_size, levels);

    int64_t remaining_shift = 0;
    {
        // First consider a "classic" shift.
        int64_t offset = big_offset(ref_, maxreflen_);
        rassert(offset >= 0);
        int64_t max_left_shift_amount = floor_aligned(offset, step);
        int64_t end_offset = offset + big_size(ref_, maxreflen_);
        int64_t max_right_shift_amount = ((maxreflen_ - BLOCK_IDS_OFFSET(maxreflen_)) / sizeof(block_id_t)) * step - ceil_aligned(end_offset, step);

        int64_t appropriate_shift;
        if (min_shift < 0) {
            appropriate_shift = std::max(- max_left_shift_amount, (min_shift / step) * step);
        } else {
            rassert(max_right_shift_amount >= 0 && floor_aligned(max_right_shift_amount, step) == max_right_shift_amount);
            if (max_right_shift_amount < min_shift) {
                return false;
            } else {
                appropriate_shift = ceil_aligned(min_shift, step);
            }
        }

        int64_t block_id_shift = appropriate_shift / step;

        if (block_id_shift < 0) {
            block_id_t *ids = block_ids(ref_, maxreflen_);
            memmove(ids, ids + (- block_id_shift), block_id_shift * sizeof(block_id_t));
            set_big_offset(ref_, maxreflen_, offset + appropriate_shift);
        } else if (block_id_shift > 0) {
            block_id_t *ids = block_ids(ref_, maxreflen_);
            memmove(ids + block_id_shift, ids, block_id_shift * sizeof(block_id_t));
            set_big_offset(ref_, maxreflen_, offset + appropriate_shift);
        }

        if (min_shift >= 0) {
            rassert(big_offset(ref_, maxreflen_) >= 0);
            return true;
        } else {
            remaining_shift = min_shift - appropriate_shift;
            rassert(remaining_shift <= 0);
        }
    }

    // Now consider shifting the first two blocks.
    if (remaining_shift < 0) {
        int64_t offset = big_offset(ref_, maxreflen_);
        rassert(offset >= 0);
        int64_t size = big_size(ref_, maxreflen_);
        int64_t end_offset = offset + size;
        int64_t substep = levels == 1 ? 1 : (step / internal_node_count(block_size));
        int64_t floor_offset = floor_aligned(offset, substep);
        int64_t ceil_end_offset = ceil_aligned(end_offset, substep);
        if (offset < step && ceil_end_offset - floor_offset <= step) {
            if (- remaining_shift >= ceil_end_offset - step) {
                block_id_t *ids = block_ids(ref_, maxreflen_);
                if (levels == 1) {
                    buf_lock_t lobuf(txn, ids[0], rwi_write);
                    char *lodata = leaf_node_data(lobuf->get_data_major_write());
                    int64_t shift_amount = std::min(- remaining_shift, offset);
                    memmove(lodata + offset - shift_amount, lodata + offset, (leaf_size(block_size) - offset) - shift_amount);

                    if (end_offset > step) {
                        buf_lock_t hibuf(txn, ids[1], rwi_write);
                        char *hidata = leaf_node_data(hibuf->get_data_major_write());
                        memmove(lodata + leaf_size(block_size) - shift_amount, hidata, std::min(shift_amount, end_offset - step));
                        hibuf->mark_deleted();
                    }

                    set_big_offset(ref_, maxreflen_, offset - shift_amount);
                } else {
                    buf_lock_t lobuf(txn, ids[0], rwi_write);
                    block_id_t *lodata = internal_node_block_ids(lobuf->get_data_major_write());
                    int64_t shift_amount = std::min(- remaining_shift, offset) / substep;
                    memmove(lodata + (floor_offset / substep - shift_amount), lodata + floor_offset / substep, (internal_node_count(block_size) - floor_offset / substep) * sizeof(block_id_t));

                    if (end_offset > step) {
                        buf_lock_t hibuf(txn, ids[1], rwi_write);
                        block_id_t *hidata = internal_node_block_ids(hibuf->get_data_major_write());

                        memmove(lodata + (internal_node_count(block_size) - shift_amount), hidata, std::min(shift_amount, (end_offset - step) / substep) * sizeof(block_id_t));
                        hibuf->mark_deleted();
                    }

                    set_big_offset(ref_, maxreflen_, offset - shift_amount * substep);
                }
            }
        }
    }

    return true;
}

// Always returns levels + 1.
int blob_t::add_level(transaction_t *txn, int levels) {
    buf_lock_t lock;
    lock.allocate(txn);
    void *b = lock->get_data_major_write();
    if (levels == 0) {
        block_magic_t leafmagic = { { 'l', 'a', 'r', 'l' } };
        *reinterpret_cast<block_magic_t *>(b) = leafmagic;

        size_t sz = small_size(ref_, maxreflen_);
        rassert(sz < maxreflen_);

        memcpy(leaf_node_data(b), small_buffer(ref_, maxreflen_), sz);

        set_small_size(ref_, maxreflen_, maxreflen_);
        set_big_offset(ref_, maxreflen_, 0);
        set_big_size(ref_, maxreflen_, sz);
        block_ids(ref_, maxreflen_)[0] = lock->get_block_id();
    } else {
        block_magic_t internalmagic = { { 'l', 'a', 'r', 'i' } };
        *reinterpret_cast<block_magic_t *>(b) = internalmagic;

        // We don't know how many block ids there could be, so we'll
        // just copy as many as there can be.
        size_t sz = maxreflen_ - BLOCK_IDS_OFFSET(maxreflen_);

        memcpy(internal_node_block_ids(b), block_ids(ref_, maxreflen_), sz);
        block_ids(ref_, maxreflen_)[0] = lock->get_block_id();
    }

    return levels + 1;
}


bool blob_t::remove_level(transaction_t *txn, int *levels_ref) {
    int levels = *levels_ref;
    if (levels == 0) {
        return false;
    }

    int64_t end_offset = big_offset(ref_, maxreflen_) + big_size(ref_, maxreflen_);
    if (end_offset > max_end_offset(txn->get_cache()->get_block_size(), levels - 1, maxreflen_)) {
        return false;
    }

    buf_lock_t lock(txn, block_ids(ref_, maxreflen_)[0], rwi_write);
    const block_id_t *b = internal_node_block_ids(lock->get_data_read());

    memcpy(block_ids(ref_, maxreflen_), b, maxreflen_ - BLOCK_IDS_OFFSET(maxreflen_));
    lock->mark_deleted();

    *levels_ref = levels - 1;
    return true;
}
