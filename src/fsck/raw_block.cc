#include "fsck/raw_block.hpp"

namespace fsck {

const char *raw_block_t::error_name(error code) {
    static const char *codes[] = {"none", "block id mismatch"};
    return codes[code];
}

raw_block_t::raw_block_t() : err(none), buf(NULL), realbuf(NULL) { }

void raw_block_t::init(int64_t size, nondirect_file_t *file, off64_t offset) {
    rassert(!realbuf);
    realbuf = (buf_data_t *)malloc_aligned(size, DEVICE_BLOCK_SIZE);
    file->read_blocking(offset, size, realbuf);
    buf = (void *)(realbuf + 1);
}

bool raw_block_t::init(block_size_t size, nondirect_file_t *file, off64_t offset, block_id_t ser_block_id) {
    init(size.ser_value(), file, offset);

    if (realbuf->block_id != ser_block_id) {
        err = block_id_mismatch;
        return false;
    }

    return true;
}

raw_block_t::~raw_block_t() {
    free(realbuf);
}

}  // namespace fsck
