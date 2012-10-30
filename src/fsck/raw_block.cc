// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "fsck/raw_block.hpp"

#include "arch/arch.hpp"

namespace fsck {

const char *raw_block_t::error_name(error code) {
    static const char *codes[raw_block_err_count] = {"none", "block id mismatch", "bad offset"};
    return codes[code];
}

raw_block_t::raw_block_t() : err(none), buf(NULL), realbuf(NULL) { }

bool raw_block_t::init(int64_t size, nondirect_file_t *file, off64_t offset) {
    rassert(!realbuf);
    realbuf = reinterpret_cast<ls_buf_data_t *>(malloc_aligned(size, DEVICE_BLOCK_SIZE));
    off64_t filesize = file->get_size();
    if (offset > filesize || offset + size > filesize) {
        err = bad_offset;
        return false;
    }
    file->read_blocking(offset, size, realbuf);
    buf = (realbuf + 1);
    return true;
}

bool raw_block_t::init(block_size_t size, nondirect_file_t *file, off64_t offset, block_id_t ser_block_id) {
    if (!init(size.ser_value(), file, offset)) {
        return false;
    }

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
