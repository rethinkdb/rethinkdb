#include "buffer_cache/alt/cache_balancer.hpp"

#include "buffer_cache/alt/evicter.hpp"
#include "arch/runtime/runtime.hpp"
#include "concurrency/pmap.hpp"

#include "debug.hpp"
#include "logger.hpp"

alt_cache_balancer_t::cache_data_t::cache_data_t(alt::evicter_t *_evicter) :
    evicter(_evicter),
    new_size(0),
    old_size(evicter->get_memory_limit()),
    used_size(evicter->in_memory_size()),
    evictions(evicter->get_cache_evictions()) { }

alt_cache_balancer_t::alt_cache_balancer_t(uint64_t _total_cache_size,
                                           uint64_t _base_mem_per_store,
                                           uint64_t _damping_factor,
                                           uint64_t interval_ms) :
    total_cache_size(_total_cache_size),
    base_mem_per_store(_base_mem_per_store),
    damping_factor(_damping_factor),
    rebalance_timer(interval_ms, this),
    thread_info(get_num_threads()),
    rebalance_pool(1, &pool_queue, this) { }

alt_cache_balancer_t::~alt_cache_balancer_t() {
    assert_thread();
}

void alt_cache_balancer_t::add_evicter(alt::evicter_t *evicter) {
    thread_info_t *info = &thread_info[get_thread_id().threadnum];
    cross_thread_mutex_t::acq_t mutex_acq(&info->mutex);
    auto res = info->evicters.insert(evicter);
    guarantee(res.second);
}

void alt_cache_balancer_t::remove_evicter(alt::evicter_t *evicter) {
    thread_info_t *info = &thread_info[get_thread_id().threadnum];
    cross_thread_mutex_t::acq_t mutex_acq(&info->mutex);
    size_t res = info->evicters.erase(evicter);
    guarantee(res == 1);
}

void alt_cache_balancer_t::on_ring() {
    assert_thread();
    // Can't block in this callback, spawn a new coroutine
    // Using this coro_pool, we only have one rebalance going at once
    pool_queue.give_value(NULL);
}

void alt_cache_balancer_t::coro_pool_callback(void *, UNUSED signal_t *interruptor) {
    assert_thread();
    scoped_array_t<std::vector<cache_data_t> > per_thread_data;
    per_thread_data.init(thread_info.size());

    // Get cache sizes from shards on each thread
    size_t total_evicters = 0;
    uint64_t total_evictions = 0;

    for (size_t i = 0; i < thread_info.size(); ++i) {
        std::set<alt::evicter_t*> *current_evicters = &thread_info[i].evicters;

        cross_thread_mutex_t::acq_t mutex_acq(&thread_info[i].mutex);

        per_thread_data[i].reserve(current_evicters->size());
        total_evicters += current_evicters->size();

        for (auto j = current_evicters->begin(); j != current_evicters->end(); ++j) {
            cache_data_t data(*j);
            per_thread_data[i].push_back(data);

            total_evictions += data.evictions;
        }
    }

    // Calculate new cache sizes if there were evictions
    if (total_evictions > 0) {
        for (size_t i = 0; i < per_thread_data.size(); ++i) {
            for (size_t j = 0; j < per_thread_data[i].size(); ++j) {
                cache_data_t *data = &per_thread_data[i][j];

                int64_t temp = total_cache_size;
                temp -= total_evicters * base_mem_per_store;
                temp = std::max(temp, static_cast<int64_t>(0));
                temp *= data->evictions;
                temp /= total_evictions;
                temp += base_mem_per_store;

                // Apply damping to keep cache from changing size too quickly
                temp -= data->old_size;
                temp /= static_cast<int64_t>(damping_factor);
                temp += data->old_size;
                data->new_size = temp;
            }
        }

        // Send new cache sizes to each thread
        pmap(per_thread_data.size(),
             std::bind(&alt_cache_balancer_t::apply_rebalance_to_thread,
                       this, ph::_1, &per_thread_data));
    }
}

void alt_cache_balancer_t::apply_rebalance_to_thread(int index,
        scoped_array_t<std::vector<cache_data_t> > *new_sizes) {
    on_thread_t rethreader((threadnum_t(index)));
    // No need to lock the thread_info since we're blocking rebalance and can count
    // on coroutine safety for other cases
    std::set<alt::evicter_t *> *evicters = &thread_info[index].evicters;
    std::vector<cache_data_t> *sizes = &(*new_sizes)[index];

    for (auto it = sizes->begin(); it != sizes->end(); ++it) {
        // Make sure the evicter still exists
        if (evicters->find(it->evicter) != evicters->end()) {
            it->evicter->update_memory_limit(it->new_size);
        }
    }
}
