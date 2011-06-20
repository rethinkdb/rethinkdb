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
    // small value sizes range from 0 to maxreflen - 1, and maxreflen indicates large value.
    if (maxreflen <= 255) {
        return *reinterpret_cast<const uint8_t *>(ref);
    } else {
        return *reinterpret_cast<const uint16_t *>(ref);
    }
}

bool is_small(const char *ref, size_t maxreflen) {
    if (maxreflen <= 255) {
        return *reinterpret_cast<const uint8_t *>(ref) < maxreflen;
    } else {
        return *reinterpret_cast<const uint16_t *>(ref) < maxreflen;
    }
}

char *small_buffer(char *ref, size_t maxreflen) {
    return ref + (maxreflen <= 255 ? 1 : 2);
}

int64_t big_size(const char *ref, size_t maxreflen) {
    return *reinterpret_cast<const int64_t *>(ref + BIG_SIZE_OFFSET(maxreflen));
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

std::pair<size_t, int> big_ref_info(block_size_t block_size, int64_t size, int64_t offset, size_t maxreflen) {
    rassert(size > int64_t(maxreflen - 1));
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
    if (smallsize <= maxreflen - 1) {
        return std::pair<size_t, int>(1 + smallsize, 0);
    } else {
        return big_ref_info(block_size, big_size(ref, maxreflen), big_offset(ref, maxreflen), maxreflen);
    }
}

size_t ref_size(block_size_t block_size, const char *ref, size_t maxreflen) {
    return ref_info(block_size, ref, maxreflen).first;
}

int64_t stepsize(block_size_t block_size, int levels) {
    int64_t step = leaf_size(block_size);
    for (int i = 0; i < levels - 1; ++i) {
        step *= internal_node_count(block_size);
    }
    return step;
}

void shrink(block_size_t block_size, int levels, int64_t offset, int64_t size, int index, int64_t *suboffset_out, int64_t *subsize_out) {
    int64_t step = stepsize(block_size, levels);

    int64_t clamp_low = index * step;
    int64_t clamp_high = clamp_low + step;
    int64_t suboffset = (offset < clamp_low ? clamp_low : offset) - clamp_low;
    int64_t subsize = (offset + size > clamp_high ? clamp_high : offset + size) - suboffset;

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
    crash("not yet implemented.");
}

void blob_t::prepend_region(UNUSED transaction_t *txn, UNUSED int64_t size) {
    crash("not yet implemented.");
}

void blob_t::unappend_region(UNUSED transaction_t *txn, UNUSED int64_t size) {
    crash("not yet implemented.");
}

void blob_t::unprepend_region(UNUSED transaction_t *txn, UNUSED int64_t size) {
    crash("not yet implemented.");
}

