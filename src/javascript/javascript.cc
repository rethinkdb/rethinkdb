#include "javascript/javascript.hpp"
#include "arch/runtime/context_switching.hpp"
#include "arch/runtime/coroutines.hpp"

namespace JS {

ctx_group_t::ctx_group_t() 
    : ctx_groups(NULL), refcounts(0)
{ }

ctx_group_t::~ctx_group_t() {
    for (one_per_thread_t<int>::iterator it = refcounts.begin(); it != refcounts.end(); it++) {
        rassert(*it == 0, "Trying to destroy a context group while an oustanding context exists");
    }
    for (one_per_thread_t<JSContextGroupRef>::iterator it = ctx_groups.begin(); it != ctx_groups.end(); it++) {
        JSContextGroupRelease(*it);
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
    : m_ctx(JSGlobalContextCreateInGroup(ctx_group->get_group(), NULL)), ctx_group(ctx_group)
{
    ctx_group->get_refcount()++;
}
ctx_t::~ctx_t() {
    assert_thread();
    assert_coro();

    JSGlobalContextRelease(m_ctx);
    ctx_group->get_refcount()--;
}

JSGlobalContextRef ctx_t::get() {
    assert_thread();
    assert_coro();

    JSSetStackBounds(ctx_group->get_group(),
                     coro_t::self()->get_stack()->get_stack_base(),
                     coro_t::self()->get_stack()->get_stack_bound());
    return m_ctx;
}

scoped_js_value_t::scoped_js_value_t(ctx_t *ctx, JSValueRef value_ref) 
    : ctx(ctx), value_ref(value_ref)
{
    JSValueProtect(ctx->get(), value_ref);
}

//Notice this works because you can protect and unprotect multiple times
scoped_js_value_t::scoped_js_value_t(const scoped_js_value_t &other) 
    : ctx(other.ctx), value_ref(other.value_ref)
{ }

scoped_js_value_t::~scoped_js_value_t() {
    JSValueUnprotect(ctx->get(), value_ref);
}

//warning, this value is not safe to copy.
JSValueRef scoped_js_value_t::get() {
    return value_ref;
}

JSValueRef *scoped_js_value_t::get_ptr() {
    return &value_ref;
}

scoped_js_value_array_t::scoped_js_value_array_t(ctx_t *ctx)
    : ctx(ctx)
{ }

scoped_js_value_array_t::scoped_js_value_array_t(ctx_t *ctx, std::vector<scoped_js_value_t> &values)
    : ctx(ctx)
{
    value_refs.reserve(values.size());
    for (std::vector<scoped_js_value_t>::iterator it = values.begin(); it != values.end(); it++) {
        value_refs.push_back(it->get());

        JSValueProtect(ctx->get(), it->get());
    }
}

scoped_js_value_array_t::~scoped_js_value_array_t() {
    for (std::vector<JSValueRef>::iterator it = value_refs.begin(); it != value_refs.end(); it++) {
        JSValueUnprotect(ctx->get(), *it);
    }
}

JSValueRef *scoped_js_value_array_t::data() { return value_refs.data(); }
int scoped_js_value_array_t::size() { return (int) value_refs.size(); }

} //namespace JS 
