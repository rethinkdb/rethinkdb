#include "arch/io/disk/pool.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "config/args.hpp"

#define BLOCKER_POOL_QUEUE_DEPTH (MAX_CONCURRENT_IO_REQUESTS * 2)

pool_diskmgr_t::pool_diskmgr_t(
        linux_event_queue_t *queue, passive_producer_t<action_t *> *_source)
    : source(_source),
      blocker_pool(MAX_CONCURRENT_IO_REQUESTS, queue),
      n_pending(0) {
    if (source->available->get()) pump();
    source->available->set_callback(this);
}

pool_diskmgr_t::~pool_diskmgr_t() {
    assert_thread();
    source->available->unset_callback();
}

void pool_diskmgr_t::action_t::run() {
    int res;
    if (is_read) {
        res = pread(fd, buf, count, offset);
    } else {
        res = pwrite(fd, buf, count, offset);
    }
    guarantee_err((size_t)res == count, "pread()/pwrite() failed.");
}

void pool_diskmgr_t::action_t::done() {
    parent->assert_thread();
    parent->n_pending--;
    parent->pump();
    parent->done_fun(this);
}

void pool_diskmgr_t::on_source_availability_changed() {
    assert_thread();
    /* This is called when the queue used to be empty but now has requests on
    it, and also when the queue's last request is consumed. */
    if (source->available->get()) pump();
}

void pool_diskmgr_t::pump() {
    assert_thread();
    while (source->available->get() && n_pending < BLOCKER_POOL_QUEUE_DEPTH) {
        action_t *a = source->pop();
        a->parent = this;
        n_pending++;
        blocker_pool.do_job(a);
    }
}

