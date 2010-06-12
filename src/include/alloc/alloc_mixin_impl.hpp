
#ifndef __ALLOC_MIXIN_IMPL_HPP__
#define __ALLOC_MIXIN_IMPL_HPP__

template <class alloc_t>
class alloc_mixin_helper_t {
public:
    static __thread alloc_t *alloc;
};

template <class alloc_t> __thread alloc_t *alloc_mixin_helper_t<alloc_t>::alloc;

/**
 * Allocation mixin using a TLS variable for all new() and delete() calls.
 */
template<class alloc_t>
void *alloc_mixin_t<alloc_t>::operator new(size_t size) {
    return alloc_mixin_helper_t<alloc_t>::alloc->malloc(size);
}

template<class alloc_t>
void alloc_mixin_t<alloc_t>::operator delete(void *ptr) {
    alloc_mixin_helper_t<alloc_t>::alloc->free(ptr);
}

/**
 * Allocation mixin requiring a user-specified allocator at runtime.
 */
template<class alloc_t>
void *alloc_runtime_mixin_t<alloc_t>::operator new(size_t size,
         alloc_t *alloc) {
    alloc_t **ap =
        static_cast<alloc_t **>(alloc->malloc(size + sizeof(alloc_t *)));
    *ap = alloc;
    return &ap[1];
}

/* This function is only called by the C++ runtime if a constructor fails. */
template<class alloc_t>
void alloc_runtime_mixin_t<alloc_t>::operator delete(void *ptr,
        alloc_t *alloc) {
    alloc_t **ap = &static_cast<alloc_t **>(ptr)[-1], *a = *ap;
    assert(a == alloc);
    a->free(ap);
}

template<class alloc_t>
void alloc_runtime_mixin_t<alloc_t>::operator delete(void *ptr) {
    alloc_t **ap = &static_cast<alloc_t **>(ptr)[-1], *a = *ap;
    a->free(ap);
}

#endif /* __ALLOC_MIXIN_IMPL_HPP__ */
