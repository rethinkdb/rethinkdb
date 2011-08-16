#ifndef __JAVASCRIPT_JAVASCRIPT_HPP__
#define __JAVASCRIPT_JAVASCRIPT_HPP__

#include <JavaScriptCore/JavaScript.h>
#include "API/JSContextRefPrivate.h"
#include "utils.hpp"
#include "arch/runtime/runtime.hpp"

namespace JS {

//static __thread JSContextGroupRef m_ctxGroup;
//static __thread bool initialized = false;
//static __thread int refcount = 0;

template <class T>
class one_per_thread_t {
private:
    T *data;
public:
    one_per_thread_t() 
        : data(new T[get_num_threads()])
    { }
    ~one_per_thread_t() {
        delete data;
    }

    T &get() { return data[get_thread_id()]; }

    // notice you should only use this if you know no one on the other threads
    // can be accessing it
    T *get_data() { return data; }
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
class ctx_t : public home_thread_mixin_t {
private:
    JSGlobalContextRef m_ctx;
    ctx_group_t *ctx_group;

public:
    ctx_t(ctx_group_t *);
    ~ctx_t();

    /* Warning, this object cannot be passed between couroutines, in fact it
     * should really never even be copied. Instead you should just use it
     * inline i.e JSFunction(ctx_wrapper.get(), ...); */
    JSGlobalContextRef get();
    DISABLE_COPYING(ctx_t);
};
} //namespace JS 

#endif
