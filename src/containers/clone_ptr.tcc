// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "containers/clone_ptr.hpp"

template<class T>
clone_ptr_t<T>::clone_ptr_t() : object(NULL) { }

template<class T>
clone_ptr_t<T>::clone_ptr_t(T *o) : object(o) { }

template<class T>
clone_ptr_t<T>::clone_ptr_t(const clone_ptr_t &x) {
    if (x.object.has()) {
        object.init(x.object->clone());
    }
}

template<class T> template<class U>
clone_ptr_t<T>::clone_ptr_t(const clone_ptr_t<U> &x) {
    if (x.object.has()) {
        object.init(x.object->clone());
    }
}

template<class T>
clone_ptr_t<T> &clone_ptr_t<T>::operator=(const clone_ptr_t &x) {
    if (&x != this) {
        object.reset();
        if (x.object.has()) {
            object.init(x.object->clone());
        }
    }
    return *this;
}

template<class T>
clone_ptr_t<T> &clone_ptr_t<T>::operator=(const clone_ptr_t &&x) noexcept {
    clone_ptr_t tmp(std::move(x));
    object = std::move(tmp.object);
    return *this;
}

template<class T> template<class U>
clone_ptr_t<T> &clone_ptr_t<T>::operator=(const clone_ptr_t<U> &x) {
    if (&x != this) {
        object.reset();
        if (x.object.has()) {
            object.init(x.object->clone());
        }
    }
    return *this;
}

template<class T> template<class U>
clone_ptr_t<T> &clone_ptr_t<T>::operator=(const clone_ptr_t<U> &&x) noexcept {
    clone_ptr_t tmp(std::move(x));
    object = std::move(tmp.object);
    return *this;
}

template<class T>
T &clone_ptr_t<T>::operator*() const {
    return *object.get();
}

template<class T>
T *clone_ptr_t<T>::operator->() const {
    return object.get();
}

template<class T>
T *clone_ptr_t<T>::get() const {
    return object.get();
}
