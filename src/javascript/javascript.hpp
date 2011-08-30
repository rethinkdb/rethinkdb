#ifndef __JAVASCRIPT_JAVASCRIPT_HPP__
#define __JAVASCRIPT_JAVASCRIPT_HPP__

#include <JavaScriptCore/JavaScript.h>
#include "API/JSContextRefPrivate.h"
#include "utils.hpp"
#include "arch/runtime/runtime.hpp"
#include <vector>
#include <exception>

namespace JS {

//static __thread JSContextGroupRef m_ctxGroup;
//static __thread bool initialized = false;
//static __thread int refcount = 0;

template <class T>
class one_per_thread_t {
private:
    std::vector<T> data;
public:
    one_per_thread_t() 
    { 
        data.resize(get_num_threads());
    }
    one_per_thread_t(const T &init)
    {
        data.resize(get_num_threads());
        for (typename std::vector<T>::iterator it = data.begin(); it != data.end(); it++) {
            *it = init;
        }
    }

    ~one_per_thread_t() { }

    T &get() { return data[get_thread_id()]; }

    // notice you should only use this if you know no one on the other threads
    // can be accessing it
    typedef typename std::vector<T>::iterator iterator;

    iterator begin() { return data.begin(); }
    iterator end() { return data.end(); }
};

/* The context group is an object that creates a context group per thread and
 * facilitates its use and cleanup. Note because JSContextGroupRefs are pretty
 * heavy weight objects this structure is lazy in its creation of them (this
 * can be turned off) */
class ctx_t;

namespace details {
template <class T>
class scoped_js_t;
} //namespace details

typedef details::scoped_js_t<JSValueRef> scoped_js_value_t;
typedef details::scoped_js_t<JSObjectRef> scoped_js_object_t;

class scoped_js_value_array_t;
class scoped_js_string_t;

class ctx_group_t {
private:
    one_per_thread_t<JSContextGroupRef> ctx_groups;
    one_per_thread_t<int> refcounts;
    void ensure_ctx_group();

public:
    ctx_group_t();
    ~ctx_group_t();

private:
friend class ctx_t;
    JSContextGroupRef get_group();
    int &get_refcount();

private:
    JSGlobalContextRef make_new_ctx();
};

/* ctx_wrapper_t wraps a JSGlobalContextRef and allows it to safely be passed
 * from coroutine to couroutine. It also*/
class ctx_t : public home_thread_mixin_t,
              public home_coro_mixin_t
{
private:
    JSGlobalContextRef m_ctx;
    ctx_group_t *ctx_group;
    int refcount; //the number of scoped_js_values that reference this ctx_t

public:
    void incr_refcount() { refcount++; }
    void decr_refcount() { refcount--; }

public:
    ctx_t() :refcount(0) { }
    ctx_t(ctx_group_t *); 
    ~ctx_t();

private:
    /* Warning, this object cannot be passed between couroutines, in fact it
     * should really never even be copied. Instead you should just use it
     * inline i.e JSFunction(ctx_wrapper.get(), ...); */
    JSGlobalContextRef get();

private:
    DISABLE_COPYING(ctx_t);

public:
    scoped_js_value_t JSValueMakeFromJSONString(scoped_js_string_t);
    scoped_js_string_t JSValueCreateJSONString(scoped_js_value_t, unsigned);
    scoped_js_value_t JSEvaluateScript(scoped_js_string_t, scoped_js_object_t, scoped_js_string_t, int);
    scoped_js_object_t JSValueToObject(scoped_js_value_t);
    scoped_js_value_t JSObjectCallAsFunction(scoped_js_object_t, scoped_js_object_t, scoped_js_value_array_t);
    scoped_js_value_t JSObjectCallAsFunction(scoped_js_object_t, scoped_js_object_t, scoped_js_value_t);
    scoped_js_object_t JSObjectMakeArray(scoped_js_value_array_t);
    scoped_js_string_t JSValueToStringCopy(scoped_js_value_t);
private:
friend class scoped_js_value_array_t;
template <class T> friend class details::scoped_js_t;

    void JSValueProtect(JSValueRef);
    void JSValueUnprotect(JSValueRef);

public:
    scoped_js_string_t JSContextCreateBacktrace(unsigned);

public:
    std::string make_string(scoped_js_value_t);
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
        : str(JSStringCreateWithUTF8CString(_str.c_str()))
    { }

    scoped_js_string_t(const scoped_js_string_t &other)
        : str(other.str)
    {
        if (str) {
            JSStringRetain(str);
        }
    }
    ~scoped_js_string_t() {
        if (str) {
            JSStringRelease(str);
        }
    }
public:
    JSStringRef get() { return str; }
};

std::string js_obj_to_string(scoped_js_string_t);

} //namespace JS 

#endif
