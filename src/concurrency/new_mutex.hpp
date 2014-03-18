#ifndef CONCURRENCY_NEW_MUTEX_HPP_
#define CONCURRENCY_NEW_MUTEX_HPP_

#include "concurrency/rwlock.hpp"

class new_mutex_t {
public:
    new_mutex_t() { }
    ~new_mutex_t() { }

private:
    friend class new_mutex_in_line_t;

    rwlock_t rwlock_;

    DISABLE_COPYING(new_mutex_t);
};

class new_mutex_in_line_t {
public:
    explicit new_mutex_in_line_t(new_mutex_t *new_mutex)
        : rwlock_in_line_(&new_mutex->rwlock_, access_t::write) { }

    const signal_t *acq_signal() const {
        return rwlock_in_line_.write_signal();
    }

    void reset() {
        rwlock_in_line_.reset();
    }

private:
    rwlock_in_line_t rwlock_in_line_;

    DISABLE_COPYING(new_mutex_in_line_t);
};


#endif  // CONCURRENCY_NEW_MUTEX_HPP_
