#ifndef BUFFER_CACHE_CACHE_BALANCER_HPP_
#define BUFFER_CACHE_CACHE_BALANCER_HPP_

#include <stdint.h>
#include <set>
#include <vector>

#include "errors.hpp"
#include "time.hpp"

#include "threading.hpp"
#include "arch/timing.hpp"
#include "concurrency/coro_pool.hpp"
#include "concurrency/queue/single_value_producer.hpp"
#include "concurrency/watchable.hpp"
#include "containers/scoped.hpp"

namespace alt {
class evicter_t;
}

// Base class so we can have a dummy implementation for tests
class cache_balancer_t : public home_thread_mixin_t {
public:
    cache_balancer_t() { }
    virtual ~cache_balancer_t() { }

    // Used to determine the initial size of a cache
    virtual uint64_t base_mem_per_store() const = 0;

    // Tells caches whether to start read ahead initially
    virtual bool read_ahead_ok_at_start() const = 0;

    // Returns a pointer to a boolean for the given thread number (which must be the
    // current thread) which, when set to true, means you should notify the balancer
    // that it should wake up.  Stuff outside the balancer should only set it from
    // true to false.
    //
    // You should only call this once -- that way, you don't need a wasteful virtual
    // function call anytime you want activity.
    virtual bool *notify_activity_boolean(threadnum_t) = 0;

    // Tells the balancer that activity happened, so that maybe it should restart its
    // balancing processes, if necessary (since right now they run on a timer).
    virtual void wake_up_activity_happened() = 0;

protected:
    friend class alt::evicter_t;

    virtual void add_evicter(alt::evicter_t *evicter) = 0;
    virtual void remove_evicter(alt::evicter_t *evicter) = 0;

    DISABLE_COPYING(cache_balancer_t);
};

// Dummy balancer that does nothing but provide the initial size of a cache
class dummy_cache_balancer_t final : public cache_balancer_t {
public:
    explicit dummy_cache_balancer_t(uint64_t _base_mem_per_store)
        : base_mem_per_store_(_base_mem_per_store),
          notify_activity_boolean_(false) { }
    ~dummy_cache_balancer_t() { }

    uint64_t base_mem_per_store() const final {
        return base_mem_per_store_;
    }

    bool read_ahead_ok_at_start() const final {
        return false;
    }

    bool *notify_activity_boolean(threadnum_t) final {
        return &notify_activity_boolean_;
    }

    void wake_up_activity_happened() final { }

private:
    void add_evicter(alt::evicter_t *) { }
    void remove_evicter(alt::evicter_t *) { }

    uint64_t base_mem_per_store_;

    bool notify_activity_boolean_;

    DISABLE_COPYING(dummy_cache_balancer_t);
};

class alt_cache_balancer_dummy_value_t { };

class alt_cache_balancer_t final :
    public cache_balancer_t,
    public coro_pool_callback_t<alt_cache_balancer_dummy_value_t>,
    public repeating_timer_callback_t {
public:
    explicit alt_cache_balancer_t(
        clone_ptr_t<watchable_t<uint64_t> > _total_cache_size_watchable);
    ~alt_cache_balancer_t();

    uint64_t base_mem_per_store() const final {
        return 0;
    }

    bool read_ahead_ok_at_start() const final {
        return true;
    }

    bool *notify_activity_boolean(threadnum_t thread) final;

    void wake_up_activity_happened() final;

private:
    friend class alt::evicter_t;

    // Constants to control how often we rebalance
    static const uint64_t rebalance_access_count_threshold;
    static const uint64_t rebalance_timeout_ms;
    static const uint64_t rebalance_check_interval_ms;

    // Controls how much read ahead is allowed out of total cache size
    static const double read_ahead_proportion;

    // Constants to determine when to stop read-ahead
    static const uint64_t read_ahead_ratio_numerator;
    static const uint64_t read_ahead_ratio_denominator;

    // Called by the evicter on the evicter's thread
    void add_evicter(alt::evicter_t *evicter);
    void remove_evicter(alt::evicter_t *evicter);

    // Print a warning if we can't fit all the tables without using extra memory
    void warn_if_overcommitted(size_t num_shards);

    // Callback for repeating timer
    void on_ring();

    // Callback that handles rebalancing in a coro pool, so we don't block the
    // timer callback or have multiple rebalances happening at once
    void coro_pool_callback(alt_cache_balancer_dummy_value_t,
                            UNUSED signal_t *interruptor);

    // Used when calculating new cache sizes
    struct cache_data_t {
        explicit cache_data_t(alt::evicter_t *_evicter);

        alt::evicter_t *evicter;
        uint64_t new_size;
        uint64_t old_size;
        uint64_t bytes_loaded;
        uint64_t access_count;
    };

    // Helper function to collect stats from each thread so we don't need
    //  atomic variables slowing down normal operations
    void collect_stats_from_thread(int index,
                                   scoped_array_t<std::vector<cache_data_t> > *data_out,
                                   scoped_array_t<bool> *zero_access_counts_out);
    // Helper function that rebalances all the shards on a given thread
    void apply_rebalance_to_thread(int index,
                                   const scoped_array_t<std::vector<cache_data_t> > *new_sizes,
                                   bool new_read_ahead_ok);

    clone_ptr_t<watchable_t<uint64_t> > total_cache_size_watchable;
    scoped_ptr_t<repeating_timer_t> rebalance_timer;
    enum class rebalance_timer_state_t {
        // Normal operating condition: there is a timer, and it'll ping soon.  Can
        // switch to examining_other_threads.
        normal,
        // We're going to other threads to examine their cache activity.  We might
        // ask them to send us a message that puts us back into "normal" state.  Such
        // a request could arrive while we're still in "examining_other_threads"
        // state, or "normal", or "deactivated."
        examining_other_threads,
        // rebalance_timer is null -- we are no longer running timer events.
        deactivated,
    };
    rebalance_timer_state_t rebalance_timer_state;

    microtime_t last_rebalance_time;
    bool read_ahead_ok;
    uint64_t bytes_toward_read_ahead_limit;

    struct per_thread_data_t {
        per_thread_data_t() : wake_up_balancer(false) { }
        std::set<alt::evicter_t *> evicters;
        // true if the balancer should wake up (because there was activity on this
        // thread).
        bool wake_up_balancer;

        DISABLE_COPYING(per_thread_data_t);
    };

    // This contains the extant evicter pointers for each thread, only accessed
    // from each thread
    scoped_array_t<per_thread_data_t> per_thread_data;

    // Coroutine pool to make sure there is only one rebalance happening at a time
    // The single_value_producer_t makes sure we never build up a backlog
    single_value_producer_t<alt_cache_balancer_dummy_value_t> pool_queue;
    coro_pool_t<alt_cache_balancer_dummy_value_t> rebalance_pool;

    watchable_t<uint64_t>::subscription_t cache_size_change_subscription;

    DISABLE_COPYING(alt_cache_balancer_t);
};

#endif  // BUFFER_CACHE_CACHE_BALANCER_HPP_
