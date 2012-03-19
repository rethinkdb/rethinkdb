#include "javascript/javascript_pool.hpp"
#include <pthread.h>
#include <stddef.h>
#include <boost/bind.hpp>

#include <API/JSContextRef.h>
#include "javascript/javascript.hpp"
#include "concurrency/cond_var.hpp"

#include <boost/bind.hpp>
#include <boost/bind.hpp>

namespace JS {

/* javascript_pool_t */

javascript_pool_t::javascript_pool_t(int nthreads, linux_event_queue_t *queue)
    : ctx_groups(nthreads),
      current_ctx_group_index(0) {
    for (size_t i = 0; i < ctx_groups.size(); i++) {
        ctx_groups[i] = new ctx_group_t(queue);
    }
}

javascript_pool_t::~javascript_pool_t() {
    for (size_t i = 0; i < ctx_groups.size(); i++) {
        delete ctx_groups[i];
    }
}

ctx_group_t *javascript_pool_t::get_a_ctx_group() {
    // We can use the ctxcounts to use the ctx_group with the least number
    // of contexts or something, but this will work for now.
    // Note: If we don't do that, there's no real reason to have ctxcounts at
    // all in release mode.
    ctx_group_t *ctx_group = ctx_groups[current_ctx_group_index];
    current_ctx_group_index = (current_ctx_group_index + 1) % ctx_groups.size();
    return ctx_group;
}

/* ctx_group_t */

ctx_group_t::ctx_group_t(linux_event_queue_t *queue) :
    blocker(1, queue),
    ctxcount(0),
    jsc_context_group_ref(JSContextGroupCreate()) {
    // XXX: Does JSContextGroupCreate() need to be called in the blocker thread?
}

ctx_group_t::~ctx_group_t() {
    rassert(ctxcount == 0);
    // XXX: Does JSContextGroupRelease() need to be called in the blocker thread?
    JSContextGroupRelease(jsc_context_group_ref);
}

JSGlobalContextRef ctx_group_t::create_JSGlobalContext() {
    incr_ctxcount();
    return run_blocking<JSGlobalContextRef>(boost::bind(::JSGlobalContextCreateInGroup,
                                                          jsc_context_group_ref, (JSClassRef) NULL));
}

void ctx_group_t::release_JSGlobalContext(JSGlobalContextRef jsc_context_ref) {
    run_blocking<void>(boost::bind(::JSGlobalContextRelease, jsc_context_ref));
    decr_ctxcount();
}

bool ctx_group_t::am_on_blocker_thread() {
    return pthread_self() == blocker.threads[0];
}

} // namespace JS
