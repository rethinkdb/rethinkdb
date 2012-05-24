#include "containers/clone_ptr.hpp"

template<class T>
clone_ptr_t<T>::clone_ptr_t() THROWS_NOTHING : object(NULL) { }

template<class T>
clone_ptr_t<T>::~clone_ptr_t() THROWS_NOTHING {
    if (object) {
        delete object;
    }
}

template<class T>
clone_ptr_t<T>::clone_ptr_t(T *o) THROWS_NOTHING : object(o) { }

template<class T>
clone_ptr_t<T>::clone_ptr_t(const clone_ptr_t &x) THROWS_NOTHING {
    if (x.object) {
        object = x.object->clone();
    } else {
        object = NULL;
    }
}

template<class T> template<class U>
clone_ptr_t<T>::clone_ptr_t(const clone_ptr_t<U> &x) THROWS_NOTHING {
    if (x.object) {
        object = x.object->clone();
    } else {
        object = NULL;
    }
}

template<class T>
clone_ptr_t<T> &clone_ptr_t<T>::operator=(const clone_ptr_t &x) THROWS_NOTHING {
    if (&x != this) {
        if (object) {
            delete object;
        }
        if (x.object) {
            object = x.object->clone();
        } else {
            object = NULL;
        }
    }
    return *this;
}

template<class T> template<class U>
clone_ptr_t<T> &clone_ptr_t<T>::operator=(const clone_ptr_t<U> &x) THROWS_NOTHING {
    if (&x != this) {
        if (object) {
            delete object;
        }
        if (x.object) {
            object = x.object->clone();
        } else {
            object = NULL;
        }
    }
    return *this;
}

template<class T>
T &clone_ptr_t<T>::operator*() const THROWS_NOTHING {
    return *object;
}

template<class T>
T *clone_ptr_t<T>::operator->() const THROWS_NOTHING {
    return object;
}

template<class T>
T *clone_ptr_t<T>::get() const THROWS_NOTHING {
    return object;
}

template<class T>
clone_ptr_t<T>::operator booleanish_t() const THROWS_NOTHING {
    if (object) {
        return &clone_ptr_t::truth_value_method_for_use_in_boolean_conversions;
    } else {
        return 0;
    }
}

template<class T>
void clone_ptr_t<T>::truth_value_method_for_use_in_boolean_conversions() {
    unreachable();
}
