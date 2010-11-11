#ifndef __FSCK_RAW_BLOCK_HPP__
#define __FSCK_RAW_BLOCK_HPP__

#include "serializer/log/data_block_manager.hpp"

namespace fsck {

typedef data_block_manager_t::buf_data_t buf_data_t;

class raw_block {
public:
    enum { none = 0, block_id_mismatch, raw_block_err_count };
    typedef uint8_t error;

    error err;

    // buf is a fake!  buf is sizeof(buf_data_t) greater than realbuf, which is below.
    const void *buf;

protected:
    raw_block();
    ~raw_block();
    void init(size_t block_size, direct_file_t *file, off64_t offset);
    bool init(size_t block_size, direct_file_t *file, off64_t offset, ser_block_id_t ser_block_id);

    buf_data_t *realbuf;
private:
    DISABLE_COPYING(raw_block);
};

}  // namespace fsck


#endif  // __FSCK_RAW_BLOCK_HPP__
