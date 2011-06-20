#include "buffer_cache/blob.hpp"

#include "buffer_cache/types.hpp"
#include "serializer/types.hpp"
#include "containers/buffer_group.hpp"

#define BIG_SIZE_OFFSET(maxreflen) ((maxreflen) <= 255 ? 1 : 2)
#define BIG_OFFSET_OFFSET(maxreflen) (BIG_SIZE_OFFSET(maxreflen) + sizeof(int64_t))
#define BLOCK_IDS_OFFSET(maxreflen) (BIG_OFFSET_OFFSET(maxreflen) + sizeof(int64_t))


size_t short_size(const char *ref, size_t maxreflen) {
    // short value sizes range from 0 to maxreflen - 1, and maxreflen indicates large value.
    if (maxreflen <= 255) {
        return *reinterpret_cast<const uint8_t *>(ref);
    } else {
        return *reinterpret_cast<const uint16_t *>(ref);
    }
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

int64_t internal_node_count(block_size_t block_size) {
    return (block_size.value() - sizeof(block_magic_t)) / sizeof(block_id_t);
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
    size_t shortsize = short_size(ref, maxreflen);
    if (shortsize <= maxreflen - 1) {
        return std::pair<size_t, int>(1 + shortsize, 0);
    } else {
        return big_ref_info(block_size, big_size(ref, maxreflen), big_offset(ref, maxreflen), maxreflen);
    }
}

size_t ref_size(block_size_t block_size, const char *ref, size_t maxreflen) {
    return ref_info(block_size, ref, maxreflen).first;
}

blob_t::blob_t(block_size_t block_size, const char *ref, size_t maxreflen)
    : ref_(reinterpret_cast<char *>(malloc(maxreflen))), maxreflen_(maxreflen) {
    rassert(maxreflen >= BLOCK_IDS_OFFSET(maxreflen) + sizeof(block_id_t));
    memcpy(ref_, ref, ref_size(block_size, ref, maxreflen));
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
    size_t shortsize = short_size(ref_, maxreflen_);
    if (shortsize <= maxreflen_ - 1) {
        return shortsize;
    } else {
        rassert(shortsize == maxreflen_);
        return big_size(ref_, maxreflen_);
    }
    return 0;
}

void blob_t::expose_region(UNUSED transaction_t *txn, UNUSED int64_t offset, UNUSED int64_t size, UNUSED buffer_group_t *buffer_group_out) {
    crash("not yet implemented.");
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

