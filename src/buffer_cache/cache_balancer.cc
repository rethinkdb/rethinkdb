#include "buffer_cache/cache_balancer.hpp"

#include <limits>

#include "buffer_cache/evicter.hpp"
#include "arch/runtime/runtime.hpp"
#include "concurrency/pmap.hpp"

const uint64_t alt_cache_balancer_t::rebalance_check_interval_ms = 20;
const uint64_t alt_cache_balancer_t::rebalance_access_count_threshold = 100;
const uint64_t alt_cache_balancer_t::rebalance_timeout_ms = 500;

const double alt_cache_balancer_t::read_ahead_proportion = 0.9;

alt_cache_balancer_t::cache_data_t::cache_data_t(alt::evicter_t *_evicter) :
    evicter(_evicter),
    new_size(0),
    old_size(evicter->memory_limit()),
    bytes_loaded(evicter->get_clamped_bytes_loaded()),
    access_count(evicter->access_count()) { }

alt_cache_balancer_t::alt_cache_balancer_t(
        clone_ptr_t<watchable_t<uint64_t> > _total_cache_size_watchable) :
    total_cache_size_watchable(_total_cache_size_watchable),
    rebalance_timer(make_scoped<repeating_timer_t>(rebalance_check_interval_ms, this)),
    rebalance_timer_state(rebalance_timer_state_t::normal),
    last_rebalance_time(0),
    read_ahead_ok(true),
    bytes_toward_read_ahead_limit(0),
    per_thread_data(get_num_threads()),
    rebalance_pool(1, &pool_queue, this),
    cache_size_change_subscription(
        [this]() {
            last_rebalance_time = 0;
            pool_queue.give_value(alt_cache_balancer_dummy_value_t());
        })
{
    watchable_t<uint64_t>::freeze_t freeze(total_cache_size_watchable);
    cache_size_change_subscription.reset(total_cache_size_watchable, &freeze);
}

alt_cache_balancer_t::~alt_cache_balancer_t() {
    assert_thread();
}

bool *alt_cache_balancer_t::notify_activity_boolean(threadnum_t thread) {
    guarantee(thread == get_thread_id());
    return &per_thread_data[thread.threadnum].wake_up_balancer;
}

void alt_cache_balancer_t::wake_up_activity_happened() {
    assert_thread();

    switch (rebalance_timer_state) {
    case rebalance_timer_state_t::normal:
        // Do nothing -- we're already awake.
        break;
    case rebalance_timer_state_t::examining_other_threads:
        rebalance_timer_state = rebalance_timer_state_t::normal;
        break;
    case rebalance_timer_state_t::deactivated:
        rebalance_timer_state = rebalance_timer_state_t::normal;
        rebalance_timer = make_scoped<repeating_timer_t>(rebalance_check_interval_ms, this);
        break;
    default:
        unreachable();
    }
}

void alt_cache_balancer_t::add_evicter(alt::evicter_t *evicter) {
    evicter->assert_thread();
    auto res = per_thread_data[get_thread_id().threadnum].evicters.insert(evicter);
    guarantee(res.second);
}

void alt_cache_balancer_t::remove_evicter(alt::evicter_t *evicter) {
    evicter->assert_thread();
    size_t res = per_thread_data[get_thread_id().threadnum].evicters.erase(evicter);
    guarantee(res == 1);
}

void alt_cache_balancer_t::on_ring() {
    assert_thread();

    // Can't block in this callback, spawn a new coroutine
    // Using this coro_pool, we only have one rebalance going at once
    pool_queue.give_value(alt_cache_balancer_dummy_value_t());
}

void alt_cache_balancer_t::coro_pool_callback(alt_cache_balancer_dummy_value_t, UNUSED signal_t *interruptor) {
    assert_thread();

    uint64_t total_cache_size = total_cache_size_watchable->get();
    // We do some signed arithmetic with cache sizes, so the total cache size
    // must fit into a signed value based on the native pointer type or bad
    // things happen.
    guarantee(total_cache_size_watchable->get() <=
        static_cast<uint64_t>(std::numeric_limits<intptr_t>::max()));

    const size_t num_threads = per_thread_data.size();
    scoped_array_t<std::vector<cache_data_t> > cache_data(num_threads);
    scoped_array_t<bool> zero_access_counts(num_threads);

    // Get cache sizes from shards on each thread
    rebalance_timer_state = rebalance_timer_state_t::examining_other_threads;
    pmap(per_thread_data.size(),
         std::bind(&alt_cache_balancer_t::collect_stats_from_thread,
                   this, ph::_1, &cache_data, &zero_access_counts));

    bool all_zero_access_counts = true;
    // Sum up the number of evicters, bytes loaded, and access counts
    size_t total_evicters = 0;
    uint64_t total_bytes_loaded = 0;
    uint64_t total_access_count = 0;
    for (size_t i = 0; i < num_threads; ++i) {
        total_evicters += cache_data[i].size();
        all_zero_access_counts &= zero_access_counts[i];
        for (size_t j = 0; j < cache_data[i].size(); ++j) {
            total_bytes_loaded += cache_data[i][j].bytes_loaded;
            total_access_count += cache_data[i][j].access_count;
        }
    }

    // Reevaluate if read-ahead should be running
    if (read_ahead_ok) {
        bytes_toward_read_ahead_limit += total_bytes_loaded;
        read_ahead_ok = bytes_toward_read_ahead_limit < (total_cache_size * read_ahead_proportion);
    }

    // Determine if we should do a rebalance, either:
    //  1. At least rebalance_timeout_ms milliseconds have passed
    //  2. At least access_count_threshold accesses have occurred
    // since the last rebalance.
    microtime_t now = current_microtime();

    if (now < last_rebalance_time + (rebalance_timeout_ms * 1000) &&
        total_access_count < rebalance_access_count_threshold) {
        return;
    }

    last_rebalance_time = now;

    // Calculate new cache sizes
    if (total_cache_size > 0 && total_evicters > 0) {
        uint64_t total_new_sizes = 0;

        for (size_t i = 0; i < cache_data.size(); ++i) {
            for (size_t j = 0; j < cache_data[i].size(); ++j) {
                cache_data_t *data = &cache_data[i][j];

                double temp = data->old_size;
                temp /= static_cast<double>(total_cache_size);
                temp *= static_cast<double>(total_bytes_loaded);

                int64_t new_size = data->bytes_loaded;
                new_size -= static_cast<int64_t>(temp);
                new_size += data->old_size;
                new_size = std::max<int64_t>(new_size, 0);

                data->new_size = new_size;
                total_new_sizes += new_size;
            }
        }

        // Distribute any rounding error across shards
        int64_t extra_bytes = total_cache_size - total_new_sizes;
        while (extra_bytes != 0) {
            int64_t delta = extra_bytes / static_cast<int64_t>(total_evicters);
            if (delta == 0) {
                delta = ((extra_bytes < 0) ? -1 : 1);
            }
            for (size_t i = 0; i < cache_data.size() && extra_bytes != 0; ++i) {
                for (size_t j = 0; j < cache_data[i].size() && extra_bytes != 0; ++j) {
                    cache_data_t *data = &cache_data[i][j];

                    // Avoid underflow
                    if (static_cast<int64_t>(data->new_size) + delta >= 0) {
                        data->new_size += delta;
                        extra_bytes -= delta;
                    } else {
                        extra_bytes += data->new_size;
                        data->new_size = 0;
                    }
                }
            }
        }

        // Send new cache sizes to each thread
        pmap(num_threads,
             std::bind(&alt_cache_balancer_t::apply_rebalance_to_thread,
                       this, ph::_1, &cache_data, read_ahead_ok));
    }

    if (all_zero_access_counts
        && rebalance_timer_state == rebalance_timer_state_t::examining_other_threads) {
        rebalance_timer_state = rebalance_timer_state_t::deactivated;
        rebalance_timer.reset();
    } else {
        rebalance_timer_state = rebalance_timer_state_t::normal;
        if (!rebalance_timer.has()) {
            rebalance_timer = make_scoped<repeating_timer_t>(rebalance_check_interval_ms, this);
        }
    }
}

void alt_cache_balancer_t::collect_stats_from_thread(
        int index,
        scoped_array_t<std::vector<cache_data_t> > *data_out,
        scoped_array_t<bool> *zero_access_count_out) {
    on_thread_t rethreader((threadnum_t(index)));

    ASSERT_NO_CORO_WAITING;
    const std::set<alt::evicter_t *> *evicters = &per_thread_data[index].evicters;
    std::vector<cache_data_t> *per_evicter_data = &(*data_out)[index];

    bool all_access_counts_zero = true;

    per_evicter_data->reserve(evicters->size());
    for (auto j = evicters->begin(); j != evicters->end(); ++j) {
        cache_data_t data(*j);
        all_access_counts_zero &= (data.access_count == 0);
        per_evicter_data->push_back(data);
    }

    per_thread_data[index].wake_up_balancer = all_access_counts_zero;
    (*zero_access_count_out)[index] = all_access_counts_zero;
}

void alt_cache_balancer_t::apply_rebalance_to_thread(int index,
        const scoped_array_t<std::vector<cache_data_t> > *new_sizes,
        bool new_read_ahead_ok) {
    on_thread_t rethreader((threadnum_t(index)));

    const std::set<alt::evicter_t *> *evicters = &per_thread_data[index].evicters;
    const std::vector<cache_data_t> *sizes = &(*new_sizes)[index];

    ASSERT_NO_CORO_WAITING;
    for (auto it = sizes->begin(); it != sizes->end(); ++it) {
        // Make sure the evicter still exists
        if (evicters->find(it->evicter) != evicters->end()) {
            it->evicter->update_memory_limit(it->new_size,
                                             it->bytes_loaded,
                                             it->access_count,
                                             new_read_ahead_ok);
        }
    }
}

