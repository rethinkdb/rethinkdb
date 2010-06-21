
#ifndef __RWI_CONC_HPP__
#define __RWI_CONC_HPP__

#include "concurrency/rwi_lock.hpp"

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
    
    struct buf_t {
        buf_t()
            : lock(NULL, 0) // TODO: we gotta pass real values here
            {}
        
        rwi_lock_t lock;
    };

    /* Returns true if acquired successfully */
    bool acquire(buf_t *buf, access_t mode) {
        // TODO: pass real state instead of NULL
        if(buf->lock.lock(mode, NULL))
            return true;
        else
            return false;
    }

    void release(buf_t *buf) {
        buf->lock.unlock();
    }
};

#endif // __RWI_CONC_HPP__

