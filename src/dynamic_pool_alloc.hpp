
#ifndef __DYNAMIC_POOL_ALLOC_HPP__
#define __DYNAMIC_POOL_ALLOC_HPP__

template <class super_alloc_t>
struct dynamic_pool_alloc_t : public super_alloc_t {
    dynamic_pool_alloc_t(size_t _object_size)
        : super_alloc_t(100, _object_size)
        {}
};

#endif // __DYNAMIC_POOL_ALLOC_HPP__

