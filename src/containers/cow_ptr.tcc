#ifndef CONTAINERS_COW_PTR_TCC_
#define CONTAINERS_COW_PTR_TCC_

#include "containers/cow_ptr.hpp"

template <class T>
class cow_pointee_t : public slow_shared_mixin_t<cow_pointee_t<T> > {
    friend class cow_ptr_t<T>;

    cow_pointee_t() { }
    cow_pointee_t(const T &copyee) : value(copyee) { }

    T value;
};

template <class T>
cow_ptr_t<T>::cow_ptr_t() : ptr(new cow_pointee_t<T>), change_count(0) { }

template <class T>
cow_ptr_t<T>::cow_ptr_t(const T &t) : ptr(new cow_pointee_t<T>(t)), change_count(0) { }

template <class T>
cow_ptr_t<T>::cow_ptr_t(const cow_ptr_t &other) : change_count(0) {
    if (other.change_count == 0) {
        ptr = other.ptr;
    } else {
        /* We can't just copy `ref.ptr` because then we would share a buffer
        with it, but its buffer is currently being modified. We have to allocate
        our own buffer. */
        ptr.reset(new cow_pointee_t<T>(*other));
    }
}

template <class T>
cow_ptr_t<T>::~cow_ptr_t() {
    rassert(change_count == 0);
}

template <class T>
cow_ptr_t<T> &cow_ptr_t<T>::operator=(const cow_ptr_t &other) {
    if (change_count == 0 && other.change_count == 0) {
        ptr = other.ptr;
    } else {
        set(*other);
    }
    return *this;
}

template <class T>
const T &cow_ptr_t<T>::operator*() const {
    return ptr->value;
}

template <class T>
const T *cow_ptr_t<T>::operator->() const {
    return &ptr->value;
}

template <class T>
const T *cow_ptr_t<T>::get() const {
    return &ptr->value;
}

template <class T>
void cow_ptr_t<T>::set(const T &new_value) {
    if (ptr.unique()) {
        ptr->value = new_value;
    } else {
        rassert(change_count == 0);
        ptr.reset(new cow_pointee_t<T>(new_value));
    }
}

template <class T>
cow_ptr_t<T>::change_t::change_t(cow_ptr_t *p) : parent(p) {
    if (!parent->ptr.unique()) {
        rassert(parent->change_count == 0);
        parent->ptr.reset(new cow_pointee_t<T>(parent->ptr->value));
    }
    ++parent->change_count;
}

template <class T>
cow_ptr_t<T>::change_t::~change_t() {
    --parent->change_count;
}

template <class T>
T *cow_ptr_t<T>::change_t::get() {
    return &parent->ptr->value;
}

#endif   /* CONTAINERS_COW_PTR_TCC_ */
