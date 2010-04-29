
#ifndef __BUFFER_CACHE_BKL_HPP__
#define __BUFFER_CACHE_BKL_HPP__

#include <pthread.h>

template <class config_t>
struct buffer_cache_bkl_t {
public:
    typedef typename config_t::serializer_t serializer_t;
    typedef typename serializer_t::block_id_t block_id_t;
    typedef typename config_t::fsm_t fsm_t;
    
public:
    buffer_cache_bkl_t() {
        int res = pthread_mutexattr_init(&attr);
        check("Could not initialize bkl mutex attributes", res != 0);

        res = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
        
        res = pthread_mutex_init(&mutex, &attr);
        check("Could not initialize buffer cache bkl mutex", res != 0);
    }

    virtual ~buffer_cache_bkl_t() {
        int res = pthread_mutex_destroy(&mutex);
        check("Could not destroy buffer cache bkl mutex", res != 0);

        res = pthread_mutexattr_destroy(&attr);
        check("Could not destroy bkl mutex attributes", res != 0);
    }

    void* begin_allocate(block_id_t *block_id) {
        pthread_mutex_lock(&mutex);
    }    
    void* end_allocate() {
        pthread_mutex_unlock(&mutex);
    }    
    
    void* begin_acquire(block_id_t block_id, fsm_t *state) {
        pthread_mutex_lock(&mutex);
    }
    void* end_acquire() {
        pthread_mutex_unlock(&mutex);
    }

    block_id_t begin_release(block_id_t block_id, void *block, bool dirty, fsm_t *state) {
        pthread_mutex_lock(&mutex);
    }
    block_id_t end_release() {
        pthread_mutex_unlock(&mutex);
    }

    void begin_aio_complete(block_id_t block_id, void *block, bool written) {
        pthread_mutex_lock(&mutex);
    }
    void end_aio_complete() {
        pthread_mutex_unlock(&mutex);
    }
    
private:
    pthread_mutex_t mutex;
    pthread_mutexattr_t attr;
};

#endif // __BUFFER_CACHE_BKL_HPP__

