#ifndef __JAVASCRIPT_JAVASCRIPT_HPP__
#define __JAVASCRIPT_JAVASCRIPT_HPP__

#include <JavaScriptCore/JavaScript.h>
#include "API/JSContextRefPrivate.h"
#include "utils.hpp"
#include "arch/runtime/runtime.hpp"
#include <vector>
#include <exception>
#include "javascript/javascript_pool.hpp"

namespace JS {

/* The context group is an object that creates a context group per thread and
 * facilitates its use and cleanup. Note because JSContextGroupRefs are pretty
 * heavy weight objects this structure is lazy in its creation of them (this
 * can be turned off) */
class ctx_t;

namespace details {
template <class T>
class scoped_js_t;
} //namespace details

typedef details::scoped_js_t< ::JSValueRef> scoped_js_value_t;
typedef details::scoped_js_t< ::JSObjectRef> scoped_js_object_t;

class scoped_js_value_array_t;
class scoped_js_string_t;

/* ctx_t is a wrapper for an actual JavaScript context object. Any operations
 * that touch a context must be run in the ctx_group_t's associated
 * blocker thread. */

class ctx_t {
public:
    explicit ctx_t(javascript_pool_t *js_pool);
    ~ctx_t();

    /* Run a function in the ctx_group's blocker thread. See
     * ctx_group_t::run_blocking for more information. */
    template <typename res_t>
    res_t run_blocking(boost::function<res_t()> f) {
        return ctx_group->run_blocking<res_t>(f);
    }

    inline void incr_refcount() { refcount++; }
    inline void decr_refcount() { refcount--; }

private:
    ctx_group_t *ctx_group;
    JSGlobalContextRef jsc_context;

    int refcount; /* the number of scoped_js_values that reference this ctx */

public:
    /* All of the following functions must be called only on the blocker thread. */
    scoped_js_value_t JSValueMakeFromJSONString(scoped_js_string_t);
    scoped_js_string_t JSValueCreateJSONString(scoped_js_value_t, unsigned);
    scoped_js_value_t JSEvaluateScript(scoped_js_string_t, scoped_js_object_t, scoped_js_string_t, int);
    scoped_js_object_t JSValueToObject(scoped_js_value_t);
    scoped_js_value_t JSObjectCallAsFunction(scoped_js_object_t, scoped_js_object_t, scoped_js_value_array_t);
    scoped_js_value_t JSObjectCallAsFunction(scoped_js_object_t, scoped_js_object_t, scoped_js_value_t);
    scoped_js_object_t JSObjectMakeArray(scoped_js_value_array_t);
    scoped_js_string_t JSValueToStringCopy(scoped_js_value_t);

    /* These functions *might* be safe to call in the main thread pool, but
     * verify for yourself before using them. */
    scoped_js_string_t JSContextCreateBacktrace(unsigned);
    std::string make_string(scoped_js_value_t);

private:
    friend class scoped_js_value_array_t;
    template <class T> friend class details::scoped_js_t;

    void JSValueProtect(JSValueRef);
    void JSValueUnprotect(JSValueRef);

private:
    /* get() gets the actual JSC context; it's for internal use, and must be
     * called on the blocker thread. */
    JSGlobalContextRef get();

private:
    DISABLE_COPYING(ctx_t);
};

namespace details {
/* scoped_js_t does RAII to protect a JSValueRef or JSObjectRef from
 * garbage collection */
template <class T>
class scoped_js_t {
    //This is need for conversions between types
    template <class S> friend class scoped_js_t;
private:
    ctx_t *ctx;
    T value_ref;
public:
    scoped_js_t() : ctx(NULL), value_ref(NULL) { protect(); }

    scoped_js_t(void *) : ctx(NULL), value_ref(NULL) { protect(); }

    scoped_js_t(ctx_t *ctx, T value_ref)
        : ctx(ctx), value_ref(value_ref)
    {
        protect();
    }

    scoped_js_t(const scoped_js_t<T> &other)
        : ctx(other.ctx), value_ref(other.value_ref)
    {
        protect();
    }

    template <class S>
    scoped_js_t(const scoped_js_t<S> &other)
        : ctx(other.ctx), value_ref(other.value_ref)
    {
        protect();
    }

    scoped_js_t<T> operator=(const scoped_js_t<T> &other) {
        unprotect();
        ctx = other.ctx;
        value_ref = other.value_ref;
        protect();
        return *this;
    }

    template <class S>
    scoped_js_t<T> operator=(const scoped_js_t<S> &other) {
        unprotect();
        ctx = other.ctx;
        value_ref = other.value_ref;
        protect();
        return *this;
    }

    ~scoped_js_t() {
        unprotect();
    }

private:
    void protect() {
        if (value_ref) {
            rassert(ctx);
            ctx->JSValueProtect(value_ref);
            ctx->incr_refcount();
        }
    }

    void unprotect() {
        if (value_ref) {
            rassert(ctx);
            ctx->JSValueUnprotect(value_ref);
            ctx->decr_refcount();
        }
    }

public:
    //warning, this value is not safe to copy.
    T get() {
        return value_ref;
    }

    //This way we can get a pointer with out needing to make a copy, this is
    //needed to pass this value to a function
    const T *get_ptr() {
        return &value_ref;
    }
};
} //namespace details

/* annoyingly, for calling javascript functions we need to construct an array
 * of values to pass as arguments. This allows us to do it safely */
class scoped_js_value_array_t {
private:
    ctx_t *ctx;
    std::vector<JSValueRef> value_refs;
public:
    scoped_js_value_array_t(ctx_t *);
    scoped_js_value_array_t(ctx_t *, std::vector<scoped_js_value_t> &);
    scoped_js_value_array_t(const scoped_js_value_array_t &);
    ~scoped_js_value_array_t();

public:
    JSValueRef *data();
    int size();
};

// E X C E P T I O N
class engine_exception : public std::exception {
public:
    const char *what() const throw() {
        return value.c_str();
    }
    engine_exception(ctx_t &, scoped_js_value_t &);
    engine_exception(std::string);
    ~engine_exception() throw() { }
private:
    std::string value;
};

class scoped_js_string_t {
private:
    JSStringRef str;
public:
    scoped_js_string_t(JSStringRef _str)
        : str(_str)
    { }

    scoped_js_string_t(std::string _str) 
        : str(::JSStringCreateWithUTF8CString(_str.c_str()))
    { }

    scoped_js_string_t(const scoped_js_string_t &other)
        : str(other.str)
    {
        if (str) {
            ::JSStringRetain(str);
        }
    }
    ~scoped_js_string_t() {
        if (str) {
            ::JSStringRelease(str);
        }
    }
public:
    JSStringRef get() { return str; }
};

std::string js_obj_to_string(scoped_js_string_t);

} //namespace JS 

#endif
