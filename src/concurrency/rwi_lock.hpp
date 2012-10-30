// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_RWI_LOCK_HPP_
#define CONCURRENCY_RWI_LOCK_HPP_

#include "containers/intrusive_list.hpp"
#include "concurrency/access.hpp"

// Forward declarations
struct rwi_lock_t;
struct lock_request_t;

/**
 * Callback class used to notify lock clients that they now have the
 * lock.
 */
struct lock_available_callback_t {
public:
    virtual ~lock_available_callback_t() {}
    virtual void on_lock_available() = 0;
};

struct lock_in_line_callback_t {
public:
    lock_in_line_callback_t() { }
    virtual void on_in_line() = 0;
protected:
    virtual ~lock_in_line_callback_t() { }
private:
    DISABLE_COPYING(lock_in_line_callback_t);
};

/**
 * Read/write/intent lock allows locking a resource for reading,
 * reading with the intent to potentially upgrade to write, and
 * writing. This is useful in a btree setting, where something might
 * be locked with.
 */
struct rwi_lock_t {
public:
    // Note, the receiver of lock_request_t completion notifications
    // is responsible for freeing associated memory by calling delete.

    rwi_lock_t()
        : state(rwis_unlocked), nreaders(0)
        {}

    // Call to lock for read, write, intent, or upgrade intent to write
    bool lock(access_t access, lock_available_callback_t *callback);

    // Like `lock()` but blocks; only legal in a coroutine. If `call_when_in_line` is not zero,
    // it will be called as soon as `co_lock()` has gotten in line for the lock but before
    // `co_lock()` actually returns.

    // You are encouraged to use `read_acq_t` and `write_acq_t` instead of
    // `co_lock()`.
    void co_lock(access_t access, lock_in_line_callback_t *call_when_in_line = 0);

    // Call if you've locked for read or write, or upgraded to write,
    // and are now unlocking.
    void unlock();
    // Call if you've locked for intent before, didn't upgrade to
    // write, and are now unlocking.
    void unlock_intent();

    // Returns true if the lock is locked in any form, but doesn't acquire the lock. (In the buffer
    // cache, this is used by the page replacement algorithm to see whether the buffer is in use.)
    bool locked();

    struct read_acq_t {
        read_acq_t() : lock(NULL) { }
        explicit read_acq_t(rwi_lock_t *l) : lock(l) {
            lock->co_lock(rwi_read);
        }
        void reset() {
            if (lock) {
                lock->unlock();
                lock = NULL;
            }
        }
        void assert_is_holding(DEBUG_VAR rwi_lock_t *l) {
            rassert(lock == l);
        }
        ~read_acq_t() {
            reset();
        }
    private:
        friend void swap(read_acq_t &, read_acq_t &);
        rwi_lock_t *lock;
        DISABLE_COPYING(read_acq_t);
    };

    struct write_acq_t {
        write_acq_t() : lock(NULL) { }
        explicit write_acq_t(rwi_lock_t *l) : lock(l) {
            lock->co_lock(rwi_write);
        }
        void reset() {
            if (lock) {
                lock->unlock();
                lock = NULL;
            }
        }
        void assert_is_holding(DEBUG_VAR rwi_lock_t *l) {
            rassert(lock == l);
        }
        ~write_acq_t() {
            reset();
        }
    private:
        friend void swap(write_acq_t &, write_acq_t &);
        rwi_lock_t *lock;
        DISABLE_COPYING(write_acq_t);
    };

private:
    enum rwi_state {
        rwis_unlocked,
        rwis_reading,
        rwis_writing,
        rwis_reading_with_intent
    };

    bool try_lock(access_t access, bool from_queue);
    bool try_lock_read(bool from_queue);
    bool try_lock_write(bool from_queue);
    bool try_lock_intent(bool from_queue);
    bool try_lock_upgrade(bool from_queue);
    void enqueue_request(access_t access, lock_available_callback_t *callback);
    void process_queue();

    rwi_state state;
    int nreaders; // not counting reader with intent

    intrusive_list_t<lock_request_t> queue;

    DISABLE_COPYING(rwi_lock_t);
};

inline void swap(rwi_lock_t::read_acq_t &a1, rwi_lock_t::read_acq_t &a2) {  // NOLINT
    rwi_lock_t *tmp = a1.lock;
    a1.lock = a2.lock;
    a2.lock = tmp;
}

inline void swap(rwi_lock_t::write_acq_t &a1, rwi_lock_t::write_acq_t &a2) {  // NOLINT
    rwi_lock_t *tmp = a1.lock;
    a1.lock = a2.lock;
    a2.lock = tmp;
}

#endif // CONCURRENCY_RWI_LOCK_HPP_

