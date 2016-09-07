// Copyright 2010-2016 RethinkDB, all rights reserved.
#ifndef CONTAINERS_LIFETIME_HPP_
#define CONTAINERS_LIFETIME_HPP_

#include <type_traits>

template <typename T>
class lifetime_t {
public:
    // `std::add_const<T *>::type` is `T * const` instead of the `T const *` we need, and
    // it will return the original type for a reference. To work around this we strip the
    // type down to it's bare bones, add the const, and re-add the pointer or reference
    // depending on the original type.
    using const_type = typename std::conditional<
        std::is_lvalue_reference<T>::value,
        typename std::add_lvalue_reference<
            typename std::add_const<
                typename std::remove_reference<T>::type
            >::type
        >::type,
        typename std::add_pointer<
            typename std::add_const<
                typename std::remove_pointer<T>::type
            >::type
        >::type
    >::type;

    explicit lifetime_t(T value) : value_(value) {
        static_assert(
            std::is_lvalue_reference<T>::value || std::is_pointer<T>::value,
            "T should be an lvalue reference or pointer");
    }

    // This allows a non-const to const conversion
    operator lifetime_t<typename lifetime_t<T>::const_type>() {
        return lifetime_t<const_type>(value_);
    }

    operator T() {
        return value_;
    }

private:
    T value_;
};

template <typename T>
lifetime_t<T> make_lifetime(T &&value) {
    static_assert(
        std::is_lvalue_reference<T>::value || std::is_pointer<T>::value,
        "T should be an lvalue reference or pointer");

    return lifetime_t<T>(std::forward<T>(value));
}

#endif /* CONTAINERS_LIFETIME_HPP_ */

