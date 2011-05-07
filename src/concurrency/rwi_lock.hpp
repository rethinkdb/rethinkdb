
#ifndef __RWI_LOCK_HPP__
#define __RWI_LOCK_HPP__

#include "containers/intrusive_list.hpp"
#include "concurrency/access.hpp"
#include "arch/arch.hpp"

// Forward declarations
struct rwi_lock_t;

/**
 * Callback class used to notify lock clients that they now have the
 * lock.
 */
struct lock_available_callback_t {
public:
    virtual ~lock_available_callback_t() {}
    virtual void on_lock_available() = 0;
};

/**
 * Read/write/intent lock allows locking a resource for reading,
 * reading with the intent to potentially upgrade to write, and
 * writing. This is useful in a btree setting, where something might
 * be locked with.
 */
struct rwi_lock_t {
public:
    struct lock_request_t : public thread_message_t,
                            public intrusive_list_node_t<lock_request_t>
    {
        lock_request_t(access_t _op, lock_available_callback_t *_callback)
            : op(_op), callback(_callback)
            {}
        access_t op;
        lock_available_callback_t *callback;
        void on_thread_switch();   // Actually, this is called later on the same thread...
    };

    // Note, the receiver of lock_request_t completion notifications
    // is responsible for freeing associated memory by calling delete.
    
    rwi_lock_t()
        : state(rwis_unlocked), nreaders(0)
        {}
    
    // Call to lock for read, write, intent, or upgrade intent to write
    bool lock(access_t access, lock_available_callback_t *callback);

    // Like lock() but blocks; only legal in a coroutine.
    void co_lock(access_t access);

    // Call if you've locked for read or write, or upgraded to write,
    // and are now unlocking.
    void unlock();

    // Call if you've locked for intent before, didn't upgrade to
    // write, and are now unlocking.
    void unlock_intent();

    // Returns true if the lock is locked in any form, but doesn't acquire the lock. (In the buffer
    // cache, this is used by the page replacement algorithm to see whether the buffer is in use.)
    bool locked();

private:
    enum rwi_state {
        rwis_unlocked,
        rwis_reading,
        rwis_writing,
        rwis_reading_with_intent
    };

    typedef intrusive_list_t<lock_request_t> request_list_t;
    
    bool try_lock(access_t access, bool from_queue);
    bool try_lock_read(bool from_queue);
    bool try_lock_write(bool from_queue);
    bool try_lock_intent(bool from_queue);
    bool try_lock_upgrade(bool from_queue);
    void enqueue_request(access_t access, lock_available_callback_t *callback);
    void process_queue();

    //private:
public:
    rwi_state state;
    int nreaders; // not counting reader with intent
    request_list_t queue;
};



#endif // __RWI_LOCK_HPP__

