#ifndef RPC_SEMILATTICE_JOINS_COW_PTR_HPP_
#define RPC_SEMILATTICE_JOINS_COW_PTR_HPP_

#include "containers/cow_ptr.hpp"

template <class T>
void semilattice_join(cow_ptr_t<T> *a, const cow_ptr_t<T> &b) {
    typename cow_ptr_t<T>::change_t change(a);
    semilattice_join(change.get(), *b);
}

template <class T>
bool operator==(const cow_ptr_t<T> &a, const cow_ptr_t<T> &b) {
    return *a == *b;
}

template <class T>
bool operator!=(const cow_ptr_t<T> &a, const cow_ptr_t<T> &b) {
    return *a != *b;
}

#endif /* RPC_SEMILATTICE_JOINS_COW_PTR_HPP_ */
