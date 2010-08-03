
#ifndef __RWI_CONC_HPP__
#define __RWI_CONC_HPP__

#include "concurrency/rwi_lock.hpp"
#include "buffer_cache/callbacks.hpp"

/**
 * Since concurrency is handled by slices (one and only one core can
 * ever access a single slice in its lifetime), this class doesn't
 * handle race conditions caused by access by multiple CPUs (as these
 * could never happen), but race conditions caused by the fact that a
 * set of operations necessary to complete a single transaction on a
 * slice can be interleaved by operations from a different transaction
 * on that slice.
 */
template<class config_t>
struct rwi_conc_t {
    typedef block_available_callback<config_t> block_available_callback_t;
    typedef typename config_t::buf_t buf_t;
    
    struct local_buf_t : public lock_available_callback_t {
    
    public:
        explicit local_buf_t(buf_t *_gbuf)
            : lock(&get_cpu_context()->worker->event_queue->message_hub,
                   get_cpu_context()->worker->workerid),
              gbuf(_gbuf)
            {}
        
        bool acquire(access_t mode, block_available_callback_t *callback) {
            if (lock.lock(mode, this)) {
                return true;
            } else {
#ifndef NDEBUG
                gbuf->active_callback_count ++;
#endif
                assert(callback);
                lock_callbacks.push_back(callback);
                return false;
            }
        }
        
        void release() {
            lock.unlock();
        }
        
        bool safe_to_unload() {
            return !lock.locked() &&
                lock_callbacks.empty();
        }

    private:
        
        rwi_lock_t lock;
        
        virtual void on_lock_available() {
            
#ifndef NDEBUG
            gbuf->active_callback_count --;
#endif
            
            // We're calling back objects that were waiting on a lock. Because
            // of that, we can only call one.
            
            assert(!lock_callbacks.empty());
            block_available_callback_t *callback = lock_callbacks.head();
            lock_callbacks.remove(callback);
            
            callback->on_block_available(gbuf);
            // Note that callback may cause block to be unloaded, so we can't safely do anything
            // after callback returns.
        }
        
        typedef intrusive_list_t<block_available_callback_t> callbacks_t;
        
        // lock_callbacks always has the same number of objects as the lock's internal callback
        // queue, but every object on the lock's internal callback queue is the buf itself. When
        // the lock calls back the buf to tell it that the lock is available, then the buf finds the
        // corresponding callback on its lock_callbacks queue and calls that callback back.
        callbacks_t lock_callbacks;
        buf_t *gbuf;
    };
};

#endif // __RWI_CONC_HPP__

