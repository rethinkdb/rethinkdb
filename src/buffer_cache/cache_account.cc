#include "buffer_cache/cache_account.hpp"

#include "arch/types.hpp"

cache_account_t::cache_account_t()
    : thread_(-1), io_account_(NULL) { }

cache_account_t::cache_account_t(cache_account_t &&movee)
    : thread_(movee.thread_), io_account_(movee.io_account_) {
    movee.thread_ = threadnum_t(-1);
    movee.io_account_ = NULL;
}

cache_account_t &cache_account_t::operator=(cache_account_t &&movee) {
    cache_account_t tmp(std::move(movee));
    std::swap(thread_, tmp.thread_);
    std::swap(io_account_, tmp.io_account_);
    return *this;
}

void cache_account_t::init(threadnum_t thread, file_account_t *io_account) {
    rassert(io_account_ == NULL);
    rassert(io_account != NULL);
    io_account_ = io_account;
    thread_ = thread;
}


cache_account_t::cache_account_t(threadnum_t thread, file_account_t *io_account)
    : thread_(thread), io_account_(io_account) {
    rassert(io_account != NULL);
}

void cache_account_t::reset() {
    if (io_account_ != NULL) {
        threadnum_t local_thread = thread_;
        file_account_t *local_account = io_account_;
        thread_ = threadnum_t(-1);
        io_account_ = NULL;
        {
            on_thread_t th(local_thread);
            delete local_account;
        }
    }
}

cache_account_t::~cache_account_t() {
    reset();
}
