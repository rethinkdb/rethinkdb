
#ifndef __RWI_LOCK_HPP__
#define __RWI_LOCK_HPP__

#include "alloc/alloc_mixin.hpp"
#include "containers/intrusive_list.hpp"
#include "message_hub.hpp"
#include "concurrency/access.hpp"

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
    struct lock_request_t : public cpu_message_t,
                            public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>,
                                                 lock_request_t>,
                            public intrusive_list_node_t<lock_request_t>
    {
        lock_request_t(access_t _op, lock_available_callback_t *_callback)
            : cpu_message_t(cpu_message_t::mt_lock), op(_op), callback(_callback)
            {}
        access_t op;
        lock_available_callback_t *callback;
    };

    // Note, the receiver of lock_request_t completion notifications
    // is responsible for freeing associated memory by calling delete.
    // TODO(NNW): We may want to select our message hub implicitly, rather than
    // making the user pass it in.  Either way, CPU is implicit from the hub.
    rwi_lock_t(message_hub_t *_hub, unsigned int _cpu)
        : hub(_hub), cpu(_cpu), state(rwis_unlocked), nreaders(0)
        {}
    
    // Call to lock for read, write, intent, or upgrade intent to write
    bool lock(access_t access, lock_available_callback_t *callback);

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
    void send_notify(lock_request_t *req);

private:
    message_hub_t *hub;
    unsigned int cpu;
    rwi_state state;
    int nreaders; // not counting reader with intent
    request_list_t queue;
};



#endif // __RWI_LOCK_HPP__

