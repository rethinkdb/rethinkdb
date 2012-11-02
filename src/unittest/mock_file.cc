#include "unittest/mock_file.hpp"

#include "arch/io/disk.hpp"

namespace unittest {

mock_file_t::mock_file_t() { }
mock_file_t::~mock_file_t() { }

bool mock_file_t::exists() { return true; }
bool mock_file_t::is_block_device() { return false; }
uint64_t mock_file_t::get_size() { return data_.size(); }
void mock_file_t::set_size(size_t size) { data_.resize(size, 0); }
void mock_file_t::set_size_at_least(size_t size) {
    if (data_.size() < size) {
        data_.resize(size, 0);
    }
}

void mock_file_t::read_async(size_t offset, size_t length, void *buf,
                             UNUSED linux_file_account_t *account, linux_iocallback_t *cb) {
    read_blocking(offset, length, buf);
    cb->on_io_complete();
}

void mock_file_t::write_async(size_t offset, size_t length, const void *buf,
                              UNUSED linux_file_account_t *account, linux_iocallback_t *cb) {
    write_blocking(offset, length, buf);
    cb->on_io_complete();
}

void mock_file_t::read_blocking(size_t offset, size_t length, void *buf) {
    verify_aligned_file_access(data_.size(), offset, length, buf);
    guarantee(!(offset > SIZE_MAX - length || offset + length > data_.size()));
    memcpy(buf, data_.data() + offset, length);
}

void mock_file_t::write_blocking(size_t offset, size_t length, const void *buf) {
    verify_aligned_file_access(data_.size(), offset, length, buf);
    guarantee(!(offset > SIZE_MAX - length || offset + length > data_.size()));
    memcpy(data_.data() + offset, buf, length);
}

bool coop_lock_and_check() { return true; }



}  // namespace unittest
