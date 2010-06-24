
#ifndef __RWI_CONC_HPP__
#define __RWI_CONC_HPP__

#include <queue>
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
    typedef rwi_lock<config_t> rwi_lock_t;
    typedef lock_available_callback<config_t> lock_available_callback_t;
    typedef block_available_callback<config_t> block_available_callback_t;
    typedef typename config_t::buf_t global_buf_t;
    
    struct buf_t : public lock_available_callback_t {
        buf_t(global_buf_t *_gbuf)
            : lock(&get_cpu_context()->event_queue->message_hub,
                   get_cpu_context()->event_queue->queue_id),
              gbuf(_gbuf)
            {}
        
        virtual void on_lock_available() {
            // We're calling back objects that were waiting on a lock. Because
            // of that, we can only call one.
            if(!lock_callbacks.empty()) {
                lock_callbacks.front()->on_block_available(gbuf);
                lock_callbacks.pop();
            }
        }
        
        void add_lock_callback(block_available_callback_t *callback) {
            if(callback)
                lock_callbacks.push(callback);
        }

        rwi_lock_t lock;

    private:
        std::queue<block_available_callback_t*> lock_callbacks;
        global_buf_t *gbuf;
    };

    /* Returns true if acquired successfully */
    bool acquire(typename config_t::buf_t *buf, access_t mode, void *state) {
        if(buf->lock.lock(mode, buf)) {
            return true;
        }
        else {
            return false;
        }
    }

    void release(typename config_t::buf_t *buf) {
        buf->lock.unlock();
    }
};

#endif // __RWI_CONC_HPP__

