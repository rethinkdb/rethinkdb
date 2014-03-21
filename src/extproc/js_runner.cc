// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "extproc/js_runner.hpp"

#include <map>

#include "extproc/js_job.hpp"
#include "time.hpp"

const size_t js_runner_t::CACHE_SIZE = 100;

// This class allows us to manage timeouts in a cleaner manner
class js_timeout_t {
public:
    // Construct a sentry on the stack when performing a timed js call, and it
    //  will run the timer for the duration it is in scope
    class sentry_t {
    public:
        sentry_t(js_timeout_t *_parent, uint64_t timeout_ms) :
            parent(_parent) {
            if (parent->timer.is_pulsed()) {
                throw interrupted_exc_t();
            }
            parent->timer.start(timeout_ms);
        }
        ~sentry_t() {
            if (!parent->timer.is_pulsed()) {
                parent->timer.cancel();
            }
        }
    private:
        js_timeout_t *parent;
    };

    signal_t *get_signal() {
        return &timer;
    }

private:
    signal_timer_t timer;
};

// Contains all the data relevant to a single worker process, so we can
//  easily clear it all and replace it
class js_runner_t::job_data_t {
public:
    job_data_t(extproc_pool_t *pool, signal_t *interruptor) :
        combined_interruptor(interruptor, js_timeout.get_signal()),
        js_job(pool, &combined_interruptor) { }

    explicit job_data_t(extproc_pool_t *pool) :
        js_job(pool, js_timeout.get_signal()) { }

    struct func_info_t {
        explicit func_info_t(js_id_t _id) :
            id(_id), timestamp(current_microtime()) { }

        js_id_t id;
        microtime_t timestamp;
    };

    std::map<std::string, func_info_t> id_cache;
    js_timeout_t js_timeout;
    wait_any_t combined_interruptor;
    js_job_t js_job;
};

js_runner_t::js_runner_t() { }

js_runner_t::~js_runner_t() {
    assert_thread();
    if (job_data.has()) {
        // Have the worker job exit its loop - if anything fails,
        //  don't worry, the worker will be cleaned up
        try {
            for (auto it = job_data->id_cache.begin();
                 it != job_data->id_cache.end(); ++it) {
                job_data->js_job.release(it->second.id);
            }
            job_data->js_job.exit();
        } catch (...) {
            // Do nothing
        }
    }
}

bool js_runner_t::connected() const {
    assert_thread();
    return job_data.has();
}

// Starts the javascript function in the worker process
void js_runner_t::begin(extproc_pool_t *pool, signal_t *interruptor) {
    assert_thread();
    if (interruptor == NULL) {
        job_data.init(new job_data_t(pool));
    } else {
        job_data.init(new job_data_t(pool, interruptor));
    }
}

js_result_t js_runner_t::eval(const std::string &source,
                              const req_config_t &config) {
    assert_thread();
    guarantee(job_data.has());

    js_result_t result;

    // Check if this is a function we already have cached
    auto it = job_data->id_cache.find(source);
    if (it != job_data->id_cache.end()) {
        result = it->second.id;
        return result;
    }

    object_buffer_t<js_timeout_t::sentry_t> sentry;
    sentry.create(&job_data->js_timeout, config.timeout_ms);

    try {
        result = job_data->js_job.eval(source);
    } catch (...) {
        // Sentry must be destroyed before the js_timeout
        sentry.reset();
        // This will mark the worker as errored so we don't try to re-sync with it
        //  on the next line (since we're in a catch statement, we aren't allowed)
        job_data->js_job.worker_error();
        job_data.reset();
        throw;
    }

    // If the eval returned a function, cache it
    js_id_t *any_id = boost::get<js_id_t>(&result);
    if (any_id != NULL) {
        cache_id(*any_id, source);
    }

    return result;
}

js_result_t js_runner_t::call(const std::string &source,
                              const std::vector<counted_t<const ql::datum_t> > &args,
                              const req_config_t &config) {
    assert_thread();
    guarantee(job_data.has());

    // This will retrieve the function from the cache if it's there, or re-eval it
    js_result_t result = eval(source, config);
    js_id_t *fn_id = boost::get<js_id_t>(&result);
    guarantee(fn_id != NULL);

    object_buffer_t<js_timeout_t::sentry_t> sentry;
    sentry.create(&job_data->js_timeout, config.timeout_ms);

    try {
        result = job_data->js_job.call(*fn_id, args);
    } catch (...) {
        // Sentry must be destroyed before the js_timeout
        sentry.reset();
        // This will mark the worker as errored so we don't try to re-sync with it
        //  on the next line (since we're in a catch statement, we aren't allowed)
        job_data->js_job.worker_error();
        job_data.reset();
        throw;
    }

    // If the call returned a function, cache it
    js_id_t *any_id = boost::get<js_id_t>(&result);
    if (any_id != NULL) {
        cache_id(*any_id, source);
    }

    return result;
}

void js_runner_t::cache_id(js_id_t id, const std::string &source) {
    guarantee(job_data.has());
    guarantee(id != INVALID_ID);

    trim_cache();

    auto cache_res = job_data->id_cache.insert(std::make_pair(source,
                                                              job_data_t::func_info_t(id)));
    guarantee(cache_res.second, "js_runner_t cached two functions with the same source");
}

void js_runner_t::release_id(js_id_t id) {
    guarantee(job_data.has());
    job_data->js_job.release(id);
}

void js_runner_t::trim_cache() {
    guarantee(job_data.has());

    if (job_data->id_cache.size() < CACHE_SIZE) {
        return;
    }

    auto oldest_func = job_data->id_cache.begin();
    for (auto it = ++job_data->id_cache.begin(); it != job_data->id_cache.end(); ++it) {
        if (it->second.timestamp < oldest_func->second.timestamp) {
            oldest_func = it;
        }
    }

    try {
        release_id(oldest_func->second.id);
        job_data->id_cache.erase(oldest_func);
    } catch (...) {
        // This will mark the worker as errored so we don't try to re-sync with it
        //  on the next line (since we're in a catch statement, we aren't allowed)
        job_data->js_job.worker_error();
        job_data.reset();
        throw;
    }
}
