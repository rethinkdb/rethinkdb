#ifndef BUFFER_CACHE_CACHE_ACCOUNT_HPP_
#define BUFFER_CACHE_CACHE_ACCOUNT_HPP_

#include "threading.hpp"

class file_account_t;

namespace alt {
class page_cache_t;
}

class cache_account_t {
public:
    cache_account_t();
    ~cache_account_t();
    cache_account_t(cache_account_t &&movee);
    cache_account_t &operator=(cache_account_t &&movee);

    file_account_t *get() const {
        return io_account_;
    }
private:
    friend class alt::page_cache_t;
    // Takes ownership of the file_account_t pointee.
    void init(threadnum_t thread, file_account_t *io_account);
    cache_account_t(threadnum_t thread, file_account_t *io_account);
    void reset();

    // I hate having this thread_ variable.  The file_account_t does need to be
    // destroyed on the right thread, though.
    threadnum_t thread_;
    file_account_t *io_account_;
    DISABLE_COPYING(cache_account_t);
};

#endif  // BUFFER_CACHE_CACHE_ACCOUNT_HPP_
