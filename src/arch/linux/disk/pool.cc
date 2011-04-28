#include "arch/linux/disk/pool.hpp"

pool_diskmgr_t::pool_diskmgr_t(linux_event_queue_t *queue) : blocker_pool(MAX_CONCURRENT_IO_REQUESTS, queue) { }

void pool_diskmgr_t::action_t::run() {
    int res;
    if (is_read) res = pread(fd, buf, count, offset);
    else res = pwrite(fd, buf, count, offset);
    guarantee_err((size_t)res == count, "pread()/pwrite() failed.");
}

void pool_diskmgr_t::action_t::done() {
    parent->done_fun(this);
}

void pool_diskmgr_t::submit(action_t *action) {
    action->parent = this;
    blocker_pool.do_job(action);
}

