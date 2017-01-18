#ifndef CONTAINERS_OPTIONAL_HPP_
#define CONTAINERS_OPTIONAL_HPP_

#include <type_traits>

#include "errors.hpp"
#include "containers/printf_buffer.hpp"

struct r_nullopt_t { };
static constexpr r_nullopt_t r_nullopt{};

template <class T>
class optional {
public:
    optional() noexcept : has_value_(false) { }
    optional(r_nullopt_t) noexcept : has_value_(false) { } // NOLINT(runtime/explicit)
    explicit optional(const T &x) noexcept(noexcept(T(std::declval<const T &>())))
        : has_value_(true) {
        new (&value_) T(x);
    }
    explicit optional(T &&x) noexcept(noexcept(T(std::move(std::declval<T>()))))
        : has_value_(true) {
        new (&value_) T(std::move(x));
    }

    ~optional() {
        if (has_value_) {
            value_.~T();
        }
    }

    optional(const optional &c) noexcept(std::is_nothrow_copy_constructible<T>::value)
        : has_value_(c.has_value_) {
        if (c.has_value_) {
            new (&value_) T(c.value_);
        }
    }

    optional(optional &&c) noexcept(std::is_nothrow_move_constructible<T>::value)
        : has_value_(c.has_value_) {
        if (c.has_value_) {
            new (&value_) T(std::move(c.value_));
        }
    }

    void operator=(const optional &rhs) noexcept(std::is_nothrow_copy_assignable<T>::value && std::is_nothrow_copy_constructible<T>::value) {
        if (rhs.has_value_) {
            set(rhs.value_);
        } else {
            reset();
        }
    }

    void operator=(optional &&rhs) noexcept(std::is_nothrow_move_assignable<T>::value && std::is_nothrow_move_constructible<T>::value) {
        if (rhs.has_value_) {
            set(std::move(rhs.value_));
        } else {
            reset();
        }
    }

    bool has_value() const noexcept {
        return has_value_;
    }

    template <class U>
    void set(U &&rhs) {
        if (has_value_) {
            value_ = std::forward<U>(rhs);
        } else {
            new (&value_) T(std::forward<U>(rhs));
            has_value_ = true;
        }
    }

    // These are called get(), like boost's optional, unlike C++17's
    // optional, which uses value().  The reason is, these check with
    // a guarantee and abort, instead of throwing an exception.
    T &get() {
        guarantee(has_value_);
        return value_;
    }

    const T &get() const {
        guarantee(has_value_);
        return value_;
    }

    template <class U>
    T value_or(U &&default_value) const {
        if (has_value_) {
            return value_;
        } else {
            return std::forward<U>(default_value);
        }
    }

    T &operator*() noexcept {
        return value_;
    }

    const T &operator*() const noexcept {
        return value_;
    }

    const T *operator->() const noexcept {
        return &value_;
    }
    T *operator->() noexcept {
        return &value_;
    }

    void reset() noexcept {
        if (has_value_) {
            has_value_ = false;
            value_.~T();
        }
    }

    bool operator<(const optional &rhs) const {
        if (has_value_) {
            if (rhs.has_value_) {
                return value_ < rhs.value_;
            } else {
                return false;
            }
        } else {
            return rhs.has_value_;
        }
    }

    bool operator==(const optional &rhs) const {
        if (has_value_) {
            if (rhs.has_value_) {
                return value_ == rhs.value_;
            } else {
                return false;
            }
        } else {
            return !rhs.has_value_;
        }
    }

    // We can't write !(*this == rhs) because this is a generic type
    // that could wrap a floating point comparison.
    bool operator!=(const optional &rhs) const {
        if (has_value_) {
            if (rhs.has_value_) {
                return value_ != rhs.value_;
            } else {
                return true;
            }
        } else {
            return rhs.has_value_;
        }
    }

    explicit operator bool() const noexcept {
        return has_value();
    }

private:
    bool has_value_;
    union { T value_; };
};

template <class T>
optional<typename std::decay<T>::type> make_optional(T &&x) {
    return optional<typename std::decay<T>::type>(std::forward<T>(x));
}

template <class T>
void debug_print(printf_buffer_t *buf, const optional<T> &x) {
    if (x.has_value()) {
        buf->appendf("opt{");
        debug_print(buf, *x);
        buf->appendf("}");
    } else {
        buf->appendf("none");
    }
}

#endif  // CONTAINERS_OPTIONAL_HPP_
