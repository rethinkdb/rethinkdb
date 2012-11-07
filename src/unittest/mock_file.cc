// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "unittest/mock_file.hpp"

#include "arch/io/disk.hpp"

namespace unittest {

mock_file_t::mock_file_t(mode_t mode, std::vector<char> *data) : mode_(mode), data_(data) {
    guarantee(mode != 0);
    guarantee(data_ != NULL);
}
mock_file_t::~mock_file_t() { }

bool mock_file_t::exists() { return true; }
bool mock_file_t::is_block_device() { return false; }
uint64_t mock_file_t::get_size() { return data_->size(); }
void mock_file_t::set_size(size_t size) { data_->resize(size, 0); }
void mock_file_t::set_size_at_least(size_t size) {
    if (data_->size() < size) {
        data_->resize(size, 0);
    }
}

void mock_file_t::read_async(size_t offset, size_t length, void *buf,
                             UNUSED file_account_t *account, linux_iocallback_t *cb) {
    read_blocking(offset, length, buf);
    cb->on_io_complete();
}

void mock_file_t::write_async(size_t offset, size_t length, const void *buf,
                              UNUSED file_account_t *account, linux_iocallback_t *cb) {
    write_blocking(offset, length, buf);
    cb->on_io_complete();
}

void mock_file_t::read_blocking(size_t offset, size_t length, void *buf) {
    guarantee(mode_ & mode_read);
    verify_aligned_file_access(data_->size(), offset, length, buf);
    guarantee(!(offset > SIZE_MAX - length || offset + length > data_->size()));
    memcpy(buf, data_->data() + offset, length);
}

void mock_file_t::write_blocking(size_t offset, size_t length, const void *buf) {
    guarantee(mode_ & mode_write);
    verify_aligned_file_access(data_->size(), offset, length, buf);
    guarantee(!(offset > SIZE_MAX - length || offset + length > data_->size()));
    memcpy(data_->data() + offset, buf, length);
}

bool mock_file_t::coop_lock_and_check() {
    // We don't actually implement the locking behavior.
    return true;
}

std::string mock_file_opener_t::file_name() const {
    return "<mock file>";
}

MUST_USE bool mock_file_opener_t::open_serializer_file_create(scoped_ptr_t<file_t> *file_out) {
    file_out->init(new mock_file_t(mock_file_t::mode_rw, &file_));
    file_exists_ = true;
    return true;
}

MUST_USE bool mock_file_opener_t::open_serializer_file_existing(scoped_ptr_t<file_t> *file_out) {
    if (!file_exists_) {
        return false;
    } else {
        file_out->init(new mock_file_t(mock_file_t::mode_rw, &file_));
        return true;
    }
}

#ifdef SEMANTIC_SERIALIZER_CHECK
MUST_USE bool mock_file_opener_t::open_semantic_checking_file(UNUSED int *fd_out) {
    crash("mock_file_t cannot be used with semantic serializer checker for now");
}
#endif


}  // namespace unittest
