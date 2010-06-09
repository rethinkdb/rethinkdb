
#ifndef __BUFFER_CACHE_BKL_HPP__
#define __BUFFER_CACHE_BKL_HPP__

#include <pthread.h>

// TODO: concurrency should operate on the blocks
// (i.e. acquire_block/release_block, without necessary implication of
// locks). It shouldn't work on internal cache datastructures (they
// ought to be responsible for their own concurrency).

template <class config_t>
struct buffer_cache_bkl_t {
public:
    typedef typename config_t::serializer_t serializer_t;
    typedef typename serializer_t::block_id_t block_id_t;
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef int transaction_t;
    
public:
    buffer_cache_bkl_t()
        : nlock(0), nunlock(0)
    {
        int res = pthread_mutexattr_init(&mutex_attr);
        check("Could not initialize buffer cache bkl mutex attributes", res != 0);

        res = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK_NP);
        check("Could not set bkl mutex type to recursive", res != 0);
        
        res = pthread_mutex_init(&mutex, &mutex_attr);
        check("Could not initialize buffer cache bkl mutex", res != 0);
    }

    virtual ~buffer_cache_bkl_t() {
        assert(nlock == nunlock);
#ifndef NDEBUG
        if(nlock != nunlock) {
            printf("nlocked: %lu, nunlocked: %lu", nlock, nunlock);
        }
#endif
        
        int res = pthread_mutex_destroy(&mutex);
        check("Could not destroy buffer cache bkl mutex", res != 0);

        res = pthread_mutexattr_destroy(&mutex_attr);
        check("Could not destroy mutex attributes", res != 0);
    }

    transaction_t* begin_transaction(conn_fsm_t *state) {
        int res = pthread_mutex_lock(&mutex);
        check("Could not lock mutex", res != 0);
        nlock++;
        return NULL;
    }
    
    void end_transaction(transaction_t *tm) {
        int res = pthread_mutex_unlock(&mutex);
        check("Could not unlock mutex", res != 0);
        nunlock++;
    }

    /* Returns zero if the block has been acquired, -1 is someone else
     * is using the block. In case -1 is returned, the request for the
     * block is queued, and the concurrency strategy should notify the
     * cache when the block is available. */
    int acquire(transaction_t *tm, block_id_t block_id) { return 0; }
    void release(transaction_t *tm, block_id_t block_id, void *block, bool dirty) {}
    
    
private:
    pthread_mutexattr_t mutex_attr;
    pthread_mutex_t mutex;
    unsigned long nlock, nunlock;
};

#endif // __BUFFER_CACHE_BKL_HPP__

