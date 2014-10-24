#include "allocation/unusual_allocator.hpp"

#include "rdb_protocol/error.hpp"
#include "containers/data_buffer.hpp"
#include "containers/shared_buffer.hpp"

template <class T>
T *unusual_size_allocator_t<T>::allocate(size_t n) {
    if (auto p = parent.lock()) {
        if (!(p->debit(full_size(n)))) {
            rfail_toplevel(ql::base_exc_t::GENERIC,
                           "Failed to allocate memory; exceeded allocation threshold");
        }
    }
    return static_cast<T*>(::rmalloc(full_size(n)));
}

template <class T>
void unusual_size_allocator_t<T>::deallocate(T *region, size_t n) {
    if (auto p = parent.lock()) p->credit(full_size(n));
    ::free(region);
}

template <class T>
size_t unusual_size_allocator_t<T>::max_size() const {
    if (auto p = parent.lock()) return p->left();
    return std::numeric_limits<size_t>::max();
}

template class unusual_size_allocator_t<data_buffer_t>;
template class unusual_size_allocator_t<shared_buf_t>;
