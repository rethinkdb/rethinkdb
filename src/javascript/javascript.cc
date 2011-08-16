#include "javascript/javascript.hpp"
#include "arch/runtime/context_switching.hpp"
#include "arch/runtime/coroutines.hpp"

namespace JS {

ctx_group_t::ctx_group_t() {
    for (int i = 0; i < get_num_threads(); i++) {
        ctx_groups.get_data()[i] = NULL;
        refcounts.get_data()[i] = 0;
    }
}

ctx_group_t::~ctx_group_t() {
    for (int i = 0; i < get_num_threads(); i++) {
        rassert(refcounts.get_data()[i] == 0, "Trying to destroy a context group while an oustanding context exists");
        JSContextGroupRelease(ctx_groups.get_data()[i]);
    }
}

void ctx_group_t::ensure_ctx_group() {
    if (!ctx_groups.get()) {
        ctx_groups.get() = JSContextGroupCreate();
    }
}

JSContextGroupRef ctx_group_t::get_group() {
    ensure_ctx_group();
    return ctx_groups.get();
}

int &ctx_group_t::get_refcount() {
    ensure_ctx_group();
    return refcounts.get();
}

ctx_t ctx_group_t::new_ctx() {
    ensure_ctx_group();
    return ctx_t(this);
}


ctx_t::ctx_t(ctx_group_t *ctx_group) 
    : m_ctx(JSGlobalContextCreateInGroup(ctx_group->get_group(), NULL))
{
    ctx_group->get_refcount()++;
}
ctx_t::~ctx_t() {
    assert_thread();
    JSGlobalContextRelease(m_ctx);
    ctx_group->get_refcount()--;
}

JSGlobalContextRef ctx_t::get() {
    assert_thread();
    JSSetStackBounds(ctx_group->get_group(),
                     coro_t::self()->get_stack()->get_stack_base(),
                     coro_t::self()->get_stack()->get_stack_bound());
    return m_ctx;
}

} //namespace JS 
