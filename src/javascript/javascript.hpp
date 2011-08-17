#ifndef __JAVASCRIPT_JAVASCRIPT_HPP__
#define __JAVASCRIPT_JAVASCRIPT_HPP__

#include <JavaScriptCore/JavaScript.h>
#include "API/JSContextRefPrivate.h"
#include "utils.hpp"
#include "arch/runtime/runtime.hpp"
#include <vector>

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

public:
    ctx_t new_ctx();
};

/* ctx_wrapper_t wraps a JSGlobalContextRef and allows it to safely be passed
 * from coroutine to couroutine. It also*/
class ctx_t : public home_thread_mixin_t,
              public home_coro_mixin_t
{
private:
    JSGlobalContextRef m_ctx;
    ctx_group_t *ctx_group;

public:
    ctx_t() { }
    ctx_t(ctx_group_t *);
    ~ctx_t();

    /* Warning, this object cannot be passed between couroutines, in fact it
     * should really never even be copied. Instead you should just use it
     * inline i.e JSFunction(ctx_wrapper.get(), ...); */
    JSGlobalContextRef get();
    DISABLE_COPYING(ctx_t);
};

/* scoped_js_value_t does RAII to protect a JSObjectRef from garbage collection */
class scoped_js_value_t {
private:
    ctx_t *ctx;
    JSValueRef value_ref;
public:
    scoped_js_value_t() : ctx(NULL), value_ref(NULL) { }
    scoped_js_value_t(ctx_t *, JSValueRef);
    scoped_js_value_t(const scoped_js_value_t &);
    ~scoped_js_value_t();
public:
    JSValueRef get();
    
    //This way we can get a pointer with out needing to make a copy, this is
    //needed to pass this value to a function
    JSValueRef *get_ptr();
};

/* annoyingly, for calling javascript functions we need to construct an array
 * of values to pass as arguments. This allows us to do it safely */
class scoped_js_value_array_t {
private:
    ctx_t *ctx;
    std::vector<JSValueRef> value_refs;
public:
    scoped_js_value_array_t(ctx_t *);
    scoped_js_value_array_t(ctx_t *, std::vector<scoped_js_value_t> &);
    ~scoped_js_value_array_t();

public:
    JSValueRef *data();
    int size();
};

} //namespace JS 

#endif
