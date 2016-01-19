// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef UNITTEST_MOCK_FILE_HPP_
#define UNITTEST_MOCK_FILE_HPP_

#include <string>
#include <vector>

#include "arch/types.hpp"
#include "errors.hpp"
#include "serializer/types.hpp"
#include "utils.hpp"

namespace unittest {

class mock_file_t : public file_t {
public:
    // That mode_rw == (mode_read | mode_write) is no accident.
    enum mode_t { mode_read = 1, mode_write = 2, mode_rw = 3 };

    mock_file_t(mode_t mode, std::vector<char> *data);
    ~mock_file_t();

    int64_t get_file_size();
    void set_file_size(int64_t size);
    void set_file_size_at_least(int64_t size);

    void read_async(int64_t offset, size_t length, void *buf,
                    file_account_t *account, linux_iocallback_t *cb);
    void write_async(int64_t offset, size_t length, const void *buf,
                     file_account_t *account, linux_iocallback_t *cb,
                     wrap_in_datasyncs_t wrap_in_datasyncs);
    void writev_async(int64_t offset, size_t length, scoped_array_t<iovec> &&bufs,
                      file_account_t *account, linux_iocallback_t *cb);

    void *create_account(UNUSED int priority, UNUSED int outstanding_requests_limit) {
        // We don't care about accounts.  Return an arbitrary non-null pointer.
        return this;
    }

    void destroy_account(UNUSED void *account) {
        /* do nothing */
    }

    bool coop_lock_and_check();

private:
    mode_t mode_;
    std::vector<char> *data_;

    DISABLE_COPYING(mock_file_t);
};

class mock_file_opener_t : public serializer_file_opener_t {
public:
    mock_file_opener_t() : file_existence_state_(no_file) { }
    std::string file_name() const;

    void open_serializer_file_create_temporary(scoped_ptr_t<file_t> *file_out);
    void move_serializer_file_to_permanent_location();
    void open_serializer_file_existing(scoped_ptr_t<file_t> *file_out);
    void unlink_serializer_file();

private:
    enum existence_state_t { no_file, temporary_file, permanent_file, unlinked_file };
    existence_state_t file_existence_state_;
    std::vector<char> file_;
};

}  // namespace unittest


#endif  // UNITTEST_MOCK_FILE_HPP_
