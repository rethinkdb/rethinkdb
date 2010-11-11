#include "fsck/raw_block.hpp"

namespace fsck {

raw_block::raw_block() : err(none), buf(NULL), realbuf(NULL) { }

void raw_block::init(size_t block_size, direct_file_t *file, off64_t offset) {
    assert(!realbuf);
    realbuf = (buf_data_t *)malloc_aligned(block_size, DEVICE_BLOCK_SIZE);
    file->read_blocking(offset, block_size, realbuf);
    buf = (void *)(realbuf + 1);
}

bool raw_block::init(size_t block_size, direct_file_t *file, off64_t offset, ser_block_id_t ser_block_id) {
    init(block_size, file, offset);

    if (realbuf->block_id != ser_block_id) {
        err = block_id_mismatch;
        return false;
    }

    return true;
}

raw_block::~raw_block() {
    free(realbuf);
}

}  // namespace fsck
