// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "mock/serializer_filestream.hpp"

#include "buffer_cache/alt/alt.hpp"
#include "perfmon/core.hpp"
#include "serializer/serializer.hpp"

namespace mock {

// Maybe we should have just used a blob for this.

serializer_file_read_stream_t::serializer_file_read_stream_t(serializer_t *serializer)
    : known_size_(-1), position_(0) {
    bool has_block_zero;
    {
        on_thread_t th(serializer->home_thread());
        has_block_zero = !serializer->get_delete_bit(0);
    }
    cache_.init(new alt_cache_t(serializer, alt_cache_config_t(),
                                &get_global_perfmon_collection()));
    if (has_block_zero) {
        alt_txn_t txn(cache_.get(), alt_read_access_t::read);
        buf_lock_t bufzero(alt_buf_parent_t(&txn), 0, alt_access_t::read);
        alt_buf_read_t bufzero_read(&bufzero);
        const void *data = bufzero_read.get_data_read();
        known_size_ = *static_cast<const int64_t *>(data);
        guarantee(known_size_ >= 0);
    }
}

serializer_file_read_stream_t::~serializer_file_read_stream_t() { }

MUST_USE int64_t serializer_file_read_stream_t::read(void *p, int64_t n) {
    if (known_size_ == -1) {
        return -1;
    }

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
    const int64_t end_block_offset = (s < block_size.value() * (block_number + 1)) ? s % block_size.value() : block_size.value();
    const int64_t num_copied = end_block_offset - block_offset;
    rassert(num_copied > 0);

    alt_txn_t txn(cache_.get(), alt_read_access_t::read);
    buf_lock_t block(alt_buf_parent_t(&txn), block_number, alt_access_t::read);
    alt_buf_read_t block_read(&block);
    const char *data = static_cast<const char *>(block_read.get_data_read());
    memcpy(p, data + block_offset, num_copied);
    return num_copied;
}

void delete_contiguous_blocks_from_0(serializer_t *serializer) {
    on_thread_t th(serializer->home_thread());
    scoped_ptr_t<file_account_t>
        account(serializer->make_io_account(CACHE_WRITES_IO_PRIORITY));
    block_id_t num_blocks = 0;
    while (!serializer->get_delete_bit(num_blocks)) {
        ++num_blocks;
    }
    if (num_blocks > 0) {
        std::vector<index_write_op_t> deletes;
        for (block_id_t i = 0; i < num_blocks; ++i) {
            deletes.push_back(index_write_op_t(i,
                                               counted_t<standard_block_token_t>()));
        }
        serializer->index_write(deletes, account.get());
    }
}

serializer_file_write_stream_t::serializer_file_write_stream_t(serializer_t *serializer) : size_(0) {
    // The serializer might be reused for the sake of a write stream.  We delete all
    // of its blocks.  (Having the cache support testing whether a block is deleted
    // (given a particular parent block) would be possible and nice but somewhat
    // problematic.)
    delete_contiguous_blocks_from_0(serializer);

    cache_.init(new alt_cache_t(serializer, alt_cache_config_t(),
                                &get_global_perfmon_collection()));

    alt_txn_t txn(cache_.get(),
                  write_durability_t::HARD,
                  repli_timestamp_t::invalid,
                  1);

    buf_lock_t z(&txn, 0, alt_create_t::create);
    alt_buf_write_t z_write(&z);
    int64_t *p = static_cast<int64_t *>(z_write.get_data_write());
    *p = 0;
}

serializer_file_write_stream_t::~serializer_file_write_stream_t() { }

MUST_USE int64_t serializer_file_write_stream_t::write(const void *p, int64_t n) {
    const char *chp = static_cast<const char *>(p);
    const int block_size = cache_->get_block_size().value();

    alt_txn_t txn(cache_.get(), write_durability_t::HARD, repli_timestamp_t::invalid,
                  2 + n / block_size);
    // Hold the size block during writes, to lock out other writers.
    buf_lock_t z(alt_buf_parent_t(&txn), 0, alt_access_t::write);
    {
        alt_buf_read_t z_read(&z);
        const int64_t *size_ptr = static_cast<const int64_t *>(z_read.get_data_read());
        guarantee(*size_ptr == size_);
    }
    int64_t offset = size_ + sizeof(int64_t);
    const int64_t end_offset = offset + n;
    while (offset < end_offset) {
        int64_t block_id = offset / block_size;

        buf_lock_t block;
        buf_lock_t *b = &z;
        if (block_id > 0) {
            if (offset % block_size == 0) {
                block = buf_lock_t(&z, block_id, alt_access_t::write);
            } else {
                block = buf_lock_t(alt_buf_parent_t(&z), block_id,
                                   alt_create_t::create);
            }
            b = &block;
        }

        const int64_t block_offset = offset % block_size;
        const int64_t end_block_offset = end_offset / block_size == block_id ? end_offset % block_size : block_size;
        const int64_t num_written = end_block_offset - block_offset;
        guarantee(num_written > 0);
        {
            alt_buf_write_t b_read(b);
            char *buf = static_cast<char *>(b_read.get_data_write());
            memcpy(buf + block_offset, chp, num_written);
        }
        offset += num_written;
    }
    size_ += n;
    {
        alt_buf_write_t z_write(&z);
        int64_t *size_ptr = static_cast<int64_t *>(z_write.get_data_write());
        *size_ptr = size_;
    }
    return n;
}

}  // namespace mock
