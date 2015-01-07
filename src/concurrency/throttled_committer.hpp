// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_THROTTLED_COMMITTER_HPP_
#define CONCURRENCY_THROTTLED_COMMITTER_HPP_

#include <functional>

#include "containers/counted.hpp"
#include "concurrency/cond_var.hpp"
#include "errors.hpp"

/*
 * `throttled_committer_t` allows you to control the number of concurrent commits.
 * It provides a `sync()` method to wait until a commit that gets initiated
 * after entering `sync()` has completed.
 *
 * You can use this to combine multiple changes into a single underlying commit,
 * whenever the individual commits take longer than the rate at which new changes
 * are coming in. This assumes that each call of `_commit_cb` always flushes
 * all changes that have accumulated up to that point, not only the latest changes.
 */

class throttled_committer_t : public home_thread_mixin_debug_only_t {
public:
    // Unless _max_active_commits == 1, _commit_cb must be reentrant safe.
    throttled_committer_t(const std::function<void()> &_commit_cb,
                          int _max_active_commits);
    ~throttled_committer_t();

    // Waits until the first commit completes that has been started after
    // entering this function.
    void sync();

private:
    void do_commit();

    // Syncs which are currently waiting for a commit keep a pointer to this condition.
    // It is pulsed once the corresponding commit completes.
    class counted_cond_t : public cond_t,
                           public single_threaded_countable_t<counted_cond_t> {
    };
    counted_t<counted_cond_t> on_next_commit_complete;
    bool unhandled_commit_waiter_exists;

    int num_active_commits;
    int max_active_commits;

    std::function<void()> commit_cb;

    DISABLE_COPYING(throttled_committer_t);
};

#endif /* CONCURRENCY_THROTTLED_COMMITTER_HPP_ */
