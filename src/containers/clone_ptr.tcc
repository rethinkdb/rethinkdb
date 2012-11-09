// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "containers/clone_ptr.hpp"

template<class T>
clone_ptr_t<T>::clone_ptr_t() THROWS_NOTHING : object(NULL) { }

template<class T>
clone_ptr_t<T>::clone_ptr_t(T *o) THROWS_NOTHING : object(o) { }

template<class T>
clone_ptr_t<T>::clone_ptr_t(const clone_ptr_t &x) THROWS_NOTHING {
    if (x.object.has()) {
        object.init(x.object->clone());
    }
}

template<class T> template<class U>
clone_ptr_t<T>::clone_ptr_t(const clone_ptr_t<U> &x) THROWS_NOTHING {
    if (x.object.has()) {
        object.init(x.object->clone());
    }
}

template<class T>
clone_ptr_t<T> &clone_ptr_t<T>::operator=(const clone_ptr_t &x) THROWS_NOTHING {
    if (&x != this) {
        object.reset();
        if (x.object.has()) {
            object.init(x.object->clone());
        }
    }
    return *this;
}

template<class T> template<class U>
clone_ptr_t<T> &clone_ptr_t<T>::operator=(const clone_ptr_t<U> &x) THROWS_NOTHING {
    if (&x != this) {
        object.reset();
        if (x.object.has()) {
            object.init(x.object->clone());
        }
    }
    return *this;
}

template<class T>
T &clone_ptr_t<T>::operator*() const THROWS_NOTHING {
    return *object.get();
}

template<class T>
T *clone_ptr_t<T>::operator->() const THROWS_NOTHING {
    return object.get();
}

template<class T>
T *clone_ptr_t<T>::get() const THROWS_NOTHING {
    return object.get();
}

template<class T>
clone_ptr_t<T>::operator booleanish_t() const THROWS_NOTHING {
    if (object.has()) {
        return &clone_ptr_t::truth_value_method_for_use_in_boolean_conversions;
    } else {
        return 0;
    }
}

template<class T>
void clone_ptr_t<T>::truth_value_method_for_use_in_boolean_conversions() {
    unreachable();
}
