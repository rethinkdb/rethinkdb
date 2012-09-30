#include "mock/serializer_filestream.hpp"

#include "buffer_cache/buffer_cache.hpp"

namespace mock {

// TODO: Uh, maybe we should just use a blob.

serializer_file_read_stream_t::serializer_file_read_stream_t(serializer_t *serializer)
    : serializer_(serializer), known_size_(-1), position_(0) {
    mirrored_cache_config_t config;
    cache_.init(new cache_t(serializer, &config, &get_global_perfmon_collection()));
    if (cache_->contains_block(0)) {
        transaction_t txn(cache_.get(), rwi_read, 0, repli_timestamp_t::invalid, order_token_t::ignore);
        buf_lock_t bufzero(&txn, 0, rwi_read);
        const void *data = bufzero.get_data_read();
        known_size_ = *static_cast<const int64_t *>(data);
        guarantee(known_size_ >= 0);
    }
}

serializer_file_read_stream_t::~serializer_file_read_stream_t() { }

MUST_USE int64_t serializer_file_read_stream_t::read(void *p, int64_t n) {
    guarantee(n >= 0);
    const int64_t real_size = known_size_ + sizeof(int64_t);
    const int64_t real_position = position_ + sizeof(int64_t);

    if (real_position == real_size || n == 0) {
        return 0;
    }

    const block_size_t block_size = cache_->get_block_size();
    const int64_t block_number = real_position / block_size.value();
    const int64_t block_offset = real_position % block_size.value();

    const int64_t s = std::min(real_position + n, real_size);
    const int64_t end_block_offset = s < block_size.value() * (block_number + 1) ? s % block_size.value() : block_size.value();
    const int64_t num_copied = end_block_offset - block_offset;
    rassert(num_copied > 0);

    if (block_number >= MAX_BLOCK_ID) {
        return -1;
    }

    transaction_t txn(cache_.get(), rwi_read, 0, repli_timestamp_t::invalid, order_token_t::ignore);
    buf_lock_t block(&txn, block_number, rwi_read);
    const char *data = static_cast<const char *>(block.get_data_read());
    memcpy(p, data + block_offset, num_copied);
    return num_copied;
}

serializer_file_write_stream_t::serializer_file_write_stream_t(serializer_t *serializer) : serializer_(serializer), size_(0) {
    mirrored_cache_config_t config;
    cache_.init(new cache_t(serializer, &config, &get_global_perfmon_collection()));
    {
        transaction_t txn(cache_.get(), rwi_write, 1, repli_timestamp_t::invalid, order_token_t::ignore);
        // Hold the size block during writes, to lock out other writers.
        buf_lock_t z(&txn, 0, rwi_write);
        int64_t *p = static_cast<int64_t *>(z.get_data_major_write());
        *p = 0;
        for (block_id_t i = 1; i < MAX_BLOCK_ID && cache_->contains_block(i); ++i) {
            buf_lock_t b(&txn, i, rwi_write);
            b.mark_deleted();
        }
    }
}

serializer_file_write_stream_t::~serializer_file_write_stream_t() { }

MUST_USE int64_t serializer_file_write_stream_t::write(const void *p, int64_t n) {
    const char *chp = static_cast<const char *>(p);
    const int block_size = cache_->get_block_size().value();
    transaction_t txn(cache_.get(), rwi_write, 2 + n / block_size, repli_timestamp_t::invalid, order_token_t::ignore);
    // Hold the size block during writes, to lock out other writers.
    buf_lock_t z(&txn, 0, rwi_write);
    int64_t *const size_ptr = static_cast<int64_t *>(z.get_data_major_write());
    guarantee(*size_ptr == size_);
    int64_t offset = size_ + sizeof(int64_t);
    const int64_t end_offset = offset + n;
    while (offset < end_offset) {
        int64_t block_id = offset / block_size;
        guarantee(block_id <= MAX_BLOCK_ID);
        if (block_id >= MAX_BLOCK_ID) {
            return -1;
        }

        buf_lock_t b(&txn, block_id, rwi_write);
        const int64_t block_offset = offset % block_size;
        const int64_t end_block_offset = end_offset / block_size == block_id ? end_offset % block_size : block_size;
        char *const buf = static_cast<char *>(b.get_data_major_write());
        const int64_t num_written = end_block_offset - block_offset;
        guarantee(num_written > 0);
        memcpy(buf + block_offset, chp, num_written);
        offset += num_written;
    }
    size_ += n;
    *size_ptr = size_;
    return n;
}

}  // namespace mock
