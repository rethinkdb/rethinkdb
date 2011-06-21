#ifndef __FSCK_RAW_BLOCK_HPP__
#define __FSCK_RAW_BLOCK_HPP__

#include "serializer/log/data_block_manager.hpp"

namespace fsck {

class raw_block_t {
public:
    enum { none = 0, block_id_mismatch, bad_offset, raw_block_err_count };
    typedef uint8_t error;

    static const char *error_name(error code);

    error err;

    // buf is a fake!  buf is sizeof(buf_data_t) greater than realbuf, which is below.
    void *buf;

protected:
    raw_block_t();
    ~raw_block_t();
    bool init(int64_t size, nondirect_file_t *file, off64_t offset) __attribute__ ((warn_unused_result));
    bool init(block_size_t size, nondirect_file_t *file, off64_t offset, block_id_t ser_block_id);

    buf_data_t *realbuf;
private:
    DISABLE_COPYING(raw_block_t);
};

}  // namespace fsck


#endif  // __FSCK_RAW_BLOCK_HPP__
