#ifndef CONTAINERS_COW_PTR_TCC_
#define CONTAINERS_COW_PTR_TCC_

#include "containers/cow_ptr.hpp"

#include "errors.hpp"
#include <boost/make_shared.hpp>

template <class T>
cow_ptr_t<T>::cow_ptr_t() : value(boost::make_shared<T>()), change_count(0) { }

template <class T>
cow_ptr_t<T>::cow_ptr_t(const T &t) : value(boost::make_shared<T>(t)), change_count(0) { }

template <class T>
cow_ptr_t<T>::cow_ptr_t(const cow_ptr_t &other) : change_count(0) {
    if (other.change_count == 0) {
        value = other.value;
    } else {
        /* We can't just copy `ref.value` because then we would share a buffer
        with it, but its buffer is currently being modified. We have to allocate
        our own buffer. */
        value = boost::make_shared<T>(*other);
    }
}

template <class T>
cow_ptr_t<T>::~cow_ptr_t() {
    rassert(change_count == 0);
}

template <class T>
cow_ptr_t<T> &cow_ptr_t<T>::operator=(const cow_ptr_t &other) {
    if (change_count == 0 && other.change_count == 0) {
        value = other.value;
    } else {
        set(*other);
    }
    return *this;
}

template <class T>
const T &cow_ptr_t<T>::operator*() const {
    return *value;
}

template <class T>
const T *cow_ptr_t<T>::operator->() const {
    return value.get();
}

template <class T>
const T *cow_ptr_t<T>::get() const {
    return value.get();
}

template <class T>
void cow_ptr_t<T>::set(const T &new_value) {
    if (value.unique()) {
        *value = new_value;
    } else {
        rassert(change_count == 0);
        value = boost::make_shared<T>(new_value);
    }
}

template <class T>
cow_ptr_t<T>::change_t::change_t(cow_ptr_t *p) : parent(p) {
    if (!parent->value.unique()) {
        rassert(parent->change_count == 0);
        parent->value = boost::make_shared<T>(*parent->value);
    }
    ++parent->change_count;
}

template <class T>
cow_ptr_t<T>::change_t::~change_t() {
    --parent->change_count;
}

template <class T>
T *cow_ptr_t<T>::change_t::get() {
    return parent->value.get();
}

#endif   /* CONTAINERS_COW_PTR_TCC_ */
