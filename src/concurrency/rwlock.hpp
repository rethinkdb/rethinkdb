#ifndef CONCURRENCY_RWLOCK_HPP_
#define CONCURRENCY_RWLOCK_HPP_

#include "concurrency/access.hpp"
#include "concurrency/cond_var.hpp"
#include "containers/intrusive_list.hpp"

class rwlock_in_line_t;

class rwlock_t {
public:
    rwlock_t();
    ~rwlock_t();

private:
    friend class rwlock_in_line_t;
    void add_acq(rwlock_in_line_t *acq);
    void remove_acq(rwlock_in_line_t *acq);

    void pulse_pulsables(rwlock_in_line_t *p);

    // Acquirers, in order by acquisition, with the head containing one of the
    // current acquirer, the tail possibly containing a node that does not yet hold
    // the lock.
    intrusive_list_t<rwlock_in_line_t> acqs_;
    DISABLE_COPYING(rwlock_t);
};

// Construct one of these to get in line for the read/write lock.  Use read_signal()
// and write_signal() to wait for your turn.  (Write accessors might get their read
// signal pulsed before their write signal.)
class rwlock_in_line_t : public intrusive_list_node_t<rwlock_in_line_t> {
public:
    rwlock_in_line_t();

    // The constructor does not block.  Use read_signal() or write_signal() to wait
    // for acquisition.
    rwlock_in_line_t(rwlock_t *lock, access_t access);

    // This can be called whenever: You may destroy the lock, getting "out of line",
    // before the lock has been acquired.
    ~rwlock_in_line_t();

    rwlock_in_line_t(rwlock_in_line_t &&) = default;

    const signal_t *read_signal() const { return &read_cond_; }
    const signal_t *write_signal() const {
        guarantee(access_ == access_t::write);
        return &write_cond_;
    }

    void reset();

    void guarantee_is_for_lock(const rwlock_t *lock) const {
        guarantee(lock_ == lock);
    }

private:
    DISABLE_COPYING(rwlock_in_line_t);
    friend class rwlock_t;
    rwlock_t *lock_;
    access_t access_;
    cond_t read_cond_;
    cond_t write_cond_;
};

class rwlock_acq_t : private rwlock_in_line_t {
public:
    // Acquires the lock.  The constructor blocks the coroutine, it doesn't return
    // until the lock is acquired.
    rwlock_acq_t(rwlock_t *lock, access_t access);
    ~rwlock_acq_t();

private:
    DISABLE_COPYING(rwlock_acq_t);
};

#endif  // CONCURRENCY_RWLOCK_HPP_
