
#ifndef __RWI_LOCK_HPP__
#define __RWI_LOCK_HPP__

#include "alloc/alloc_mixin.hpp"
#include "containers/intrusive_list.hpp"
#include "message_hub.hpp"

/* Read/write/intent lock allows locking a resource for reading,
 * reading with the intent to potentially upgrade to write, and
 * writing. This is useful in a btree setting, where something might
 * be locked with */
template<class config_t>
struct rwi_lock {
public:
    enum access_t {
        rwi_read,
        rwi_write,
        rwi_intent,
        rwi_upgrade
    };

    struct lock_request_t : public cpu_message_t,
                            public alloc_mixin_t<tls_small_obj_alloc_accessor<typename config_t::alloc_t>,
                                                 lock_request_t>,
                            public intrusive_list_node_t<lock_request_t>
    {
        lock_request_t(access_t _op, void *_state)
            : op(_op), state(_state)
            {}
        access_t op;
        void *state;
    };

    // Note, the receiver of lock_request_t completion notifications
    // is responsible for freeing associated memory by calling delete.
    rwi_lock(message_hub_t *_hub, unsigned int _cpu)
        : hub(_hub), cpu(_cpu), state(rwis_unlocked), nreaders(0)
        {}
    
    // Call to lock for read, write, intent, or upgrade intent to write
    bool lock(access_t access, void *state);

    // Call if you've locked for read or write, or upgraded to write,
    // and are now unlocking.
    void unlock();

    // Call if you've locked for intent before, didn't upgrade to
    // write, and are now unlocking.
    void unlock_intent();

private:
    enum rwi_state {
        rwis_unlocked,
        rwis_reading,
        rwis_writing,
        rwis_reading_with_intent
    };

    typedef intrusive_list_t<lock_request_t> request_list_t;
    
    bool try_lock(access_t access);
    bool try_lock_read();
    bool try_lock_write();
    bool try_lock_intent();
    bool try_lock_upgrade();
    void enqueue_request(access_t access, void *state);
    void process_queue();
    void send_notify(lock_request_t *req);

private:
    message_hub_t *hub;
    unsigned int cpu;
    rwi_state state;
    unsigned int nreaders; // not counting reader with intent
    request_list_t queue;
};

#include "rwi_lock_impl.hpp"

#endif // __RWI_LOCK_HPP__

