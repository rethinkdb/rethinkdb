#include "javascript/javascript.hpp"
#include "arch/runtime/context_switching.hpp"
#include "arch/runtime/coroutines.hpp"

#include <boost/bind.hpp>

namespace JS {

ctx_t::ctx_t(javascript_pool_t *js_pool) 
    : ctx_group(js_pool->get_a_ctx_group()),
      jsc_context(ctx_group->create_JSGlobalContext()),
      refcount(0) { }

ctx_t::~ctx_t() {
    rassert(refcount == 0);
    ctx_group->release_JSGlobalContext(jsc_context);
}

JSGlobalContextRef ctx_t::get() {
    rassert(ctx_group->am_on_blocker_thread());
    DEBUG_ONLY(::JSGarbageCollect(jsc_context));
    return jsc_context;
}

scoped_js_value_t ctx_t::JSValueMakeFromJSONString(scoped_js_string_t json_str) {
    scoped_js_value_t res(this, ::JSValueMakeFromJSONString(get(), json_str.get()));
    if (res.get() == NULL) {
        throw engine_exception("Failed to parse JSON");
    }
    return res;
}

scoped_js_string_t ctx_t::JSValueCreateJSONString(scoped_js_value_t val, unsigned indent) {
    JSValueRef exc;
    scoped_js_string_t res(::JSValueCreateJSONString(get(), val.get(), indent, &exc));
    if (res.get() == NULL) {
        scoped_js_value_t exception_val(this, exc);
        throw engine_exception(*this, exception_val);
    }
    return res;
}

scoped_js_value_t ctx_t::JSEvaluateScript(scoped_js_string_t script, scoped_js_object_t thisObject, scoped_js_string_t sourceURL, int startingLineNumber) {
    JSValueRef exc;
    scoped_js_value_t res(this, ::JSEvaluateScript(get(), script.get(), thisObject.get(), sourceURL.get(), startingLineNumber, &exc));
    if (res.get() == NULL) {
        scoped_js_value_t exception_val(this, exc);
        throw engine_exception(*this, exception_val);
    }
    return res;
}

scoped_js_object_t ctx_t::JSValueToObject(scoped_js_value_t val) {
    JSValueRef exc;
    scoped_js_object_t res(this, ::JSValueToObject(get(), val.get(), &exc));
    if (res.get() == NULL) {
        scoped_js_value_t exception_val(this, exc);
        throw engine_exception(*this, exception_val);
    }
    return res;
}

scoped_js_value_t ctx_t::JSObjectCallAsFunction(scoped_js_object_t object, scoped_js_object_t thisObject, scoped_js_value_array_t args) {
    JSValueRef exc;
    scoped_js_value_t res(this, ::JSObjectCallAsFunction(get(), object.get(), thisObject.get(), args.size(), args.data(), &exc));
    if (res.get() == NULL) {
        scoped_js_value_t exception_val(this, exc);
        throw engine_exception(*this, exception_val);
    }
    return res;
}

scoped_js_value_t ctx_t::JSObjectCallAsFunction(scoped_js_object_t object, scoped_js_object_t thisObject, scoped_js_value_t arg) {
    JSValueRef exc;
    scoped_js_value_t res(this, ::JSObjectCallAsFunction(get(), object.get(), thisObject.get(), 1, arg.get_ptr(), &exc));
    if (res.get() == NULL) {
        scoped_js_value_t exception_val(this, exc);
        throw engine_exception(*this, exception_val);
    }
    return res;
}

scoped_js_object_t ctx_t::JSObjectMakeArray(scoped_js_value_array_t vals) {
    JSValueRef exc;
    scoped_js_object_t res(this, ::JSObjectMakeArray(get(), vals.size(), vals.data(), &exc));
    if (res.get() == NULL) {
        scoped_js_value_t exception_val(this, exc);
        throw engine_exception(*this, exception_val);
    }
    return res;
}


scoped_js_string_t ctx_t::JSValueToStringCopy(scoped_js_value_t val) {
    JSValueRef exc;
    scoped_js_string_t res(::JSValueToStringCopy(get(), val.get(), &exc));
    if (res.get() == NULL) {
        scoped_js_value_t exception_val(this, exc);
        throw engine_exception(*this, exception_val);
    }
    return res;
}

scoped_js_string_t ctx_t::JSContextCreateBacktrace(unsigned maxStackSize) {
    return ::JSContextCreateBacktrace(get(), maxStackSize);
}

std::string ctx_t::make_string(scoped_js_value_t value) {
    return js_obj_to_string(JSValueToStringCopy(value));
}


void ctx_t::JSValueProtect(JSValueRef val) {
    ::JSValueProtect(get(), val);
}

void ctx_t::JSValueUnprotect(JSValueRef val) {
    ::JSValueUnprotect(get(), val);
}

scoped_js_value_array_t::scoped_js_value_array_t(ctx_t *ctx)
    : ctx(ctx)
{
    ctx->incr_refcount();
}

scoped_js_value_array_t::scoped_js_value_array_t(ctx_t *ctx, std::vector<scoped_js_value_t> &values)
    : ctx(ctx)
{
    value_refs.reserve(values.size());
    for (std::vector<scoped_js_value_t>::iterator it = values.begin(); it != values.end(); it++) {
        value_refs.push_back(it->get());
        ctx->JSValueProtect(it->get());
    }

    ctx->incr_refcount();
}

scoped_js_value_array_t::scoped_js_value_array_t(const scoped_js_value_array_t &other)
    : ctx(other.ctx), value_refs(other.value_refs)
{
    for (std::vector<JSValueRef>::iterator it = value_refs.begin(); it != value_refs.end(); it++) {
        ctx->JSValueProtect(*it);
    }
    
    ctx->incr_refcount();
}

scoped_js_value_array_t::~scoped_js_value_array_t() {
    for (std::vector<JSValueRef>::iterator it = value_refs.begin(); it != value_refs.end(); it++) {
        ctx->JSValueUnprotect(*it);
    }
    ctx->decr_refcount();
}

JSValueRef *scoped_js_value_array_t::data() { return value_refs.data(); }
int scoped_js_value_array_t::size() { return (int) value_refs.size(); }

engine_exception::engine_exception(ctx_t &ctx, scoped_js_value_t &js_exception) {
    if (!js_exception.get()) {
        value = "Unknown js exception";
    } else {
        scoped_js_string_t str = ctx.JSValueToStringCopy(js_exception);
        if (!str.get()) {
            value = "Unable to convert js value to string.";
        } else {
            value = js_obj_to_string(str);
        }
    }
}

engine_exception::engine_exception(std::string custom_val) 
    : value(custom_val)
{ }

std::string js_obj_to_string(scoped_js_string_t str) {
    size_t size = JSStringGetMaximumUTF8CStringSize(str.get());
    char *result_buf = new char[size];
    JSStringGetUTF8CString(str.get(), result_buf, size);
    return std::string(result_buf);
}

} //namespace JS 
