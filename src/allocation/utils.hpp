#ifndef ALLOCATION_UTILS_HPP
#define ALLOCATION_UTILS_HPP

#include <memory>

template <class T, class Allocator, class... Args>
T* make(std::allocator_arg_t, Allocator &alloc, Args&&... args) {
    T* result = std::allocator_traits<Allocator>::allocate(alloc, 1);
    try {
        std::allocator_traits<Allocator>::construct(alloc, result, std::forward<Args>(args)...);
        return result;
    } catch(...) {
        std::allocator_traits<Allocator>::deallocate(alloc, result, 1);
        throw;
    }
}

#endif // ALLOCATION_UTILS_HPP
