
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
#ifdef NDEBUG
    return accessor_t::template get_alloc<type_t>()->malloc(size);
#else
    // In debug mode we need to store the allocator so we can later
    // check that we're deallocating from the same core.
    void *alloc_ptr = accessor_t::template get_alloc<type_t>();
    void **tmp = (void**)accessor_t::template get_alloc<type_t>()->malloc(sizeof(void*) + size);
    tmp[0] = alloc_ptr;
    return &tmp[1];
#endif
}

template<class accessor_t, class type_t>
void alloc_mixin_t<accessor_t, type_t>::operator delete(void *ptr) {
#ifdef NDEBUG
    accessor_t::template get_alloc<type_t>()->free(ptr);
#else
    // In debug mode, check that we use the same allocator to free as
    // we used to create.
    void **tmp = (void**)ptr;
    assert(tmp[-1] == accessor_t::template get_alloc<type_t>());
    accessor_t::template get_alloc<type_t>()->free((void*)&tmp[-1]);
#endif
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
