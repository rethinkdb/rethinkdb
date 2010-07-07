
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
        explicit local_buf_t(buf_t *_gbuf)
            : lock(&get_cpu_context()->event_queue->message_hub,
                   get_cpu_context()->event_queue->queue_id),
              gbuf(_gbuf)
            {}
        
        virtual void on_lock_available() {
            // We're calling back objects that were waiting on a lock. Because
            // of that, we can only call one.
            // TODO: There's no reason why more than one reader can't go at the same time, but this
            // logic won't allow that. Ideally we would use the existing lock-queue logic in
            // rwi_lock, instead of the buf_t keeping track of its own queue separately from the
            // queue in its lock.
            if(!lock_callbacks.empty()) {
                block_available_callback_t *_callback = lock_callbacks.head();
                _callback->on_block_available(gbuf);
                lock_callbacks.remove(_callback);
            }
        }
        
        void add_lock_callback(block_available_callback_t *callback) {
            if(callback)
                lock_callbacks.push_back(callback);
        }

        rwi_lock_t lock;
        
        // Prints debugging information designed to resolve deadlocks.
        void deadlock_debug() {
			printf("locked = %d\n", (int)lock.locked());
        	printf("waiting for lock(%d) = [\n", (int)lock_callbacks.size());
        	
        	block_available_callback_t *node;
        	for (node = lock_callbacks.head(); node; node = node->next) {
        	    btree_fsm<config_t> *fsm = dynamic_cast<btree_fsm<config_t> *>(node);
        	    if (fsm) {
        	        fsm->deadlock_debug();
        	    } else {
        	        printf("%p (not an FSM)\n", node);
        	    }
        	}
        	printf("]\n");
        }

    private:
        typedef intrusive_list_t<block_available_callback_t> callbacks_t;
        callbacks_t lock_callbacks;
        buf_t *gbuf;
    };

    /* Returns true if acquired successfully */
    bool acquire(typename config_t::buf_t *buf, access_t mode, void *state) {
        if(buf->lock.lock(mode, buf)) {
            return true;
        } else {
            return false;
        }
    }

    void release(typename config_t::buf_t *buf) {
        buf->lock.unlock();
    }
};

#endif // __RWI_CONC_HPP__

