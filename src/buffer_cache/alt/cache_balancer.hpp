#ifndef BUFFER_CACHE_ALT_CACHE_BALANCER_HPP_
#define BUFFER_CACHE_ALT_CACHE_BALANCER_HPP_

#include <stdint.h>
#include <set>

#include "threading.hpp"
#include "arch/timing.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/cross_thread_mutex.hpp"
#include "concurrency/queue/single_value_producer.hpp"
#include "containers/scoped.hpp"

namespace alt {
    class evicter_t;
}

// Base class so we can have a dummy implementation for tests
class cache_balancer_t {
public:
    cache_balancer_t() { }
    virtual ~cache_balancer_t() { }

    virtual uint64_t get_base_mem_per_store() const = 0;

protected:
    friend class alt::evicter_t;

    virtual void add_evicter(alt::evicter_t *evicter) = 0;
    virtual void remove_evicter(alt::evicter_t *evicter) = 0;
};

// Dummy balancer that does nothing but provide the initial size of a cache
class dummy_cache_balancer_t : public cache_balancer_t {
public:
    dummy_cache_balancer_t(uint64_t _base_mem_per_store) :
        base_mem_per_store(_base_mem_per_store) { }
    ~dummy_cache_balancer_t() { }

    uint64_t get_base_mem_per_store() const {
        return base_mem_per_store;
    }

private:
    void add_evicter(alt::evicter_t *) { }
    void remove_evicter(alt::evicter_t *) { }

    uint64_t base_mem_per_store;
};

class alt_cache_balancer_t :
    public cache_balancer_t,
    public home_thread_mixin_t,
    public coro_pool_callback_t<void *>,
    public repeating_timer_callback_t
{
public:
    alt_cache_balancer_t(uint64_t _total_cache_size,
                         uint64_t _base_mem_per_store,
                         uint64_t _damping_factor,
                         uint64_t interval_ms);
    ~alt_cache_balancer_t();

    uint64_t get_base_mem_per_store() const {
        return base_mem_per_store;
    }

private:
    friend class alt::evicter_t;

    void add_evicter(alt::evicter_t *evicter);
    void remove_evicter(alt::evicter_t *evicter);

    // Callback for repeating timer
    void on_ring();

    // Callback that handles rebalancing in a coro pool, so we don't block the
    // timer callback or have multiple rebalances happening at once
    void coro_pool_callback(void *, UNUSED signal_t *interruptor);

    // Used when calculating new cache sizes
    struct cache_data_t {
        cache_data_t(alt::evicter_t *_evicter);

        alt::evicter_t *evicter;
        uint64_t new_size;
        uint64_t old_size;
        uint64_t used_size;
        uint64_t evictions;
    };

    // Helper function that rebalances all the shards on a given thread
    void apply_rebalance_to_thread(int index,
            scoped_array_t<std::vector<cache_data_t> > *new_sizes);

    struct thread_info_t {
        std::set<alt::evicter_t *> evicters;
        cross_thread_mutex_t mutex;
    };

    uint64_t total_cache_size;
    uint64_t base_mem_per_store;
    uint64_t damping_factor;
    repeating_timer_t rebalance_timer;

    scoped_array_t<thread_info_t> thread_info;

    single_value_producer_t<void *> pool_queue;
    coro_pool_t<void *> rebalance_pool;

    DISABLE_COPYING(alt_cache_balancer_t);
};

#endif  // BUFFER_CACHE_ALT_CACHE_BALANCER_HPP_
