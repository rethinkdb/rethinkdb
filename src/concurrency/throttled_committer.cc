// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "concurrency/throttled_committer.hpp"

#include "errors.hpp"
#include "arch/runtime/coroutines.hpp"


throttled_committer_t::throttled_committer_t(const std::function<void()> &_commit_cb,
                                             int _max_active_commits) :
    on_next_commit_complete(new counted_cond_t()),
    unhandled_commit_waiter_exists(false),
    num_active_commits(0),
    max_active_commits(_max_active_commits),
    commit_cb(_commit_cb) { }

throttled_committer_t::~throttled_committer_t() {
    assert_thread();
    rassert(num_active_commits == 0);
}

void throttled_committer_t::sync() {
    rassert(coro_t::self() != NULL);
    assert_thread();

    // We keep a pointer to the on_next_commit_complete signal
    // so we get notified exactly when the commit that includes everything
    // up to this call to `sync()` has been completed.
    counted_t<counted_cond_t> commit_complete = on_next_commit_complete;
    unhandled_commit_waiter_exists = true;

    // Check if we can initiate a new commit
    if (num_active_commits < max_active_commits) {
        ++num_active_commits;
        do_commit();
    }

    // Wait for the commit to complete
    commit_complete->wait_lazily_unordered();
}

void throttled_committer_t::do_commit() {
    assert_thread();
    rassert(num_active_commits <= max_active_commits);

    // Swap out the on_next_commit_complete signal so subsequent syncs
    // can be captured by the next round of do_commit().
    counted_t<counted_cond_t> sync_complete(new counted_cond_t());
    {
        ASSERT_NO_CORO_WAITING;
        sync_complete.swap(on_next_commit_complete);
        unhandled_commit_waiter_exists = false;
    }

    // Actually perform the commit
    commit_cb();

    // Tell the sync waiters that the commit is done
    sync_complete->pulse();
    --num_active_commits;

    // Check if we should start another commit
    rassert(num_active_commits < max_active_commits);
    if (unhandled_commit_waiter_exists) {
        ++num_active_commits;
        coro_t::spawn_sometime(std::bind(&throttled_committer_t::do_commit, this));
    }
}
