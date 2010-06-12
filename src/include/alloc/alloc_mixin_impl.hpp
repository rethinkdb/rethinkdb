
#ifndef __ALLOC_MIXIN_IMPL_HPP__
#define __ALLOC_MIXIN_IMPL_HPP__

/**
 * The small object accessor creates small object allocators per
 * thread per type on the fly.
 */
template <class alloc_t> __thread std::vector<alloc_t*>* tls_small_obj_alloc_accessor<alloc_t>::allocs = NULL;

/**
 * Allocation mixin using a TLS accessor for all new() and delete() calls.
 */
template<class accessor_t, class type_t>
void *alloc_mixin_t<accessor_t, type_t>::operator new(size_t size) {
    return accessor_t::template get_alloc<type_t>()->malloc(size);
}

template<class accessor_t, class type_t>
void alloc_mixin_t<accessor_t, type_t>::operator delete(void *ptr) {
    accessor_t::template get_alloc<type_t>()->free(ptr);
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
