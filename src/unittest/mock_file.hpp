#ifndef UNITTEST_MOCK_FILE_HPP_
#define UNITTEST_MOCK_FILE_HPP_

#include "arch/types.hpp"

#include "errors.hpp"

namespace unittest {

class mock_file_t : public file_t {
public:
    enum mode_t { mode_read = 1 << 0, mode_write = 1 << 1 };

    mock_file_t(mode_t mode);
    ~mock_file_t();

    bool exists();
    bool is_block_device();
    uint64_t get_size();
    void set_size(size_t size);
    void set_size_at_least(size_t size);

    void read_async(size_t offset, size_t length, void *buf, linux_file_account_t *account, linux_iocallback_t *cb);
    void write_async(size_t offset, size_t length, const void *buf, linux_file_account_t *account, linux_iocallback_t *cb);

    void read_blocking(size_t offset, size_t length, void *buf);
    void write_blocking(size_t offset, size_t length, const void *buf);

    bool coop_lock_and_check();

private:
    mode_t mode_;
    std::vector<char> data_;

    DISABLE_COPYING(mock_file_t);
};

}  // namespace unittest


#endif  // UNITTEST_MOCK_FILE_HPP_
