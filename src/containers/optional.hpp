#ifndef CONTAINERS_OPTIONAL_HPP_
#define CONTAINERS_OPTIONAL_HPP_

#include <type_traits>

#include "errors.hpp"
#include "containers/printf_buffer.hpp"

/// @brief Sentinel type for empty optional values.
/// 
/// Used to explicitly construct an optional with no value, similar to
/// std::nullopt in C++17.
/// 
/// Example:
/// @code
///   optional<int> value = r_nullopt;
///   assert(!value.has_value());
/// @endcode
struct r_nullopt_t { };
static constexpr r_nullopt_t r_nullopt{};

/// @brief A lightweight optional type wrapper for storing values that may not be present.
/// 
/// This is a custom implementation of optional that provides a type-safe way to
/// represent optional values. Unlike std::optional, this implementation is guaranteed
/// to work consistently across different C++ standard versions.
/// 
/// The template parameter @p T represents the type of value that may be stored.
/// If no value is present, the optional is considered "empty" or "null".
/// 
/// Features:
/// - Move and copy constructors with noexcept guarantees where possible
/// - Safe value access with guarantee-based checking (aborts on invalid access)
/// - Compatible with the RethinkDB type system
/// 
/// Example:
/// @code
///   optional<std::string> name;
///   if (name.has_value()) {
///       std::cout << name.get() << std::endl;
///   }
///   
///   optional<int> count(42);
///   assert(count.has_value());
///   assert(count.get() == 42);
/// @endcode
template <class T>
class optional {
public:
    /// @brief Construct an empty optional.
    /// 
    /// Creates an optional with no contained value. The optional remains empty
    /// until explicitly assigned a value.
    optional() noexcept : has_value_(false) { }
    
    /// @brief Construct an empty optional using the r_nullopt sentinel.
    /// 
    /// Explicitly creates an empty optional. This constructor is implicit to allow
    /// convenient assignment of r_nullopt to optional variables.
    /// 
    /// @param r_nullopt The sentinel value indicating an empty optional
    optional(r_nullopt_t) noexcept : has_value_(false) { } // NOLINT(runtime/explicit)
    
    /// @brief Construct an optional with a copy of the provided value.
    /// 
    /// Creates an optional containing a copy of the given value. The constructor
    /// is explicit to prevent accidental implicit conversions.
    /// 
    /// @param x The value to copy into the optional
    /// @tparam T The type of value being stored
    /// 
    /// Example:
    /// @code
    ///   optional<int> value(42);
    ///   optional<std::string> name(std::string("Alice"));
    /// @endcode
    explicit optional(const T &x) noexcept(noexcept(T(std::declval<const T &>())))
        : has_value_(true) {
        new (&value_) T(x);
    }
    
    /// @brief Construct an optional with a moved value.
    /// 
    /// Creates an optional containing the provided value through move semantics.
    /// This is more efficient than the copy constructor for move-capable types.
    /// 
    /// @param x The value to move into the optional
    /// 
    /// Example:
    /// @code
    ///   std::vector<int> data = {1, 2, 3};
    ///   optional<std::vector<int>> opt(std::move(data));
    /// @endcode
    explicit optional(T &&x) noexcept(noexcept(T(std::move(std::declval<T>()))))
        : has_value_(true) {
        new (&value_) T(std::move(x));
    }

    /// @brief Destroy the optional and its contained value if present.
    /// 
    /// If the optional contains a value, the destructor calls the contained type's
    /// destructor. Otherwise, this is a no-op.
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

    /// @brief Move-construct an optional from another optional.
    /// 
    /// Efficiently transfers ownership of the value from another optional using
    /// move semantics. If the source optional is empty, the new optional is also empty.
    /// 
    /// @param c The source optional to move from
    optional(optional &&c) noexcept(std::is_nothrow_move_constructible<T>::value)
        : has_value_(c.has_value_) {
        if (c.has_value_) {
            new (&value_) T(std::move(c.value_));
        }
    }

    /// @brief Copy-assign from another optional.
    /// 
    /// Assigns the value (or empty state) from another optional. If both contain values,
    /// the assignment operator of the contained type is called. If the source is empty,
    /// the current value (if any) is destroyed.
    /// 
    /// @param rhs The source optional to copy from
    /// @return Reference to this optional
    void operator=(const optional &rhs) noexcept(std::is_nothrow_copy_assignable<T>::value && std::is_nothrow_copy_constructible<T>::value) {
        if (rhs.has_value_) {
            set(rhs.value_);
        } else {
            reset();
        }
    }

    /// @brief Move-assign from another optional.
    /// 
    /// Efficiently transfers ownership of the value from another optional using
    /// move semantics.
    /// 
    /// @param rhs The source optional to move from
    /// @return Reference to this optional
    void operator=(optional &&rhs) noexcept(std::is_nothrow_move_assignable<T>::value && std::is_nothrow_move_constructible<T>::value) {
        if (rhs.has_value_) {
            set(std::move(rhs.value_));
        } else {
            reset();
        }
    }

    /// @brief Check whether the optional contains a value.
    /// 
    /// @return true if the optional contains a value, false otherwise
    /// 
    /// Example:
    /// @code
    ///   optional<int> opt(42);
    ///   if (opt.has_value()) {
    ///       std::cout << "Value is present" << std::endl;
    ///   }
    /// @endcode
    bool has_value() const noexcept {
        return has_value_;
    }

    /// @brief Set the value of the optional.
    /// 
    /// If the optional already contains a value, it is replaced. Otherwise,
    /// the value is constructed in-place. This method supports both copy and move semantics.
    /// 
    /// @param rhs The value to set (can be lvalue or rvalue)
    /// 
    /// Example:
    /// @code
    ///   optional<std::string> name;
    ///   name.set("Alice");
    ///   assert(name.has_value());
    ///   assert(name.get() == "Alice");
    /// @endcode
    template <class U>
    void set(U &&rhs) {
        if (has_value_) {
            value_ = std::forward<U>(rhs);
        } else {
            new (&value_) T(std::forward<U>(rhs));
            has_value_ = true;
        }
    }

    /// @brief Get a mutable reference to the contained value.
    /// 
    /// Retrieves a mutable reference to the value stored in the optional.
    /// If the optional is empty, this method aborts with a guarantee failure
    /// (unlike std::optional which throws an exception).
    /// 
    /// @return A mutable reference to the contained value
    /// @throws Does not throw; instead aborts if no value is present
    /// 
    /// Example:
    /// @code
    ///   optional<int> value(42);
    ///   value.get() = 50;  // Modify the contained value
    ///   assert(value.get() == 50);
    /// @endcode
    T &get() {
        guarantee(has_value_);
        return value_;
    }

    /// @brief Get a const reference to the contained value.
    /// 
    /// Retrieves a const reference to the value stored in the optional.
    /// If the optional is empty, this method aborts with a guarantee failure.
    /// 
    /// @return A const reference to the contained value
    /// 
    /// Example:
    /// @code
    ///   const optional<std::string> name("Alice");
    ///   std::cout << name.get() << std::endl;  // "Alice"
    /// @endcode
    const T &get() const {
        guarantee(has_value_);
        return value_;
    }

    /// @brief Get the contained value or a default if empty.
    /// 
    /// Returns the contained value if the optional has one, otherwise returns
    /// the provided default value. Useful for providing fallback values without
    /// needing to check has_value() explicitly.
    /// 
    /// @param default_value The value to return if the optional is empty
    /// @return The contained value or the provided default
    /// 
    /// Example:
    /// @code
    ///   optional<int> count;
    ///   int value = count.value_or(0);  // value == 0 if count is empty
    ///   
    ///   optional<int> count2(42);
    ///   int value2 = count2.value_or(0);  // value2 == 42
    /// @endcode
    template <class U>
    T value_or(U &&default_value) const {
        if (has_value_) {
            return value_;
        } else {
            return std::forward<U>(default_value);
        }
    }

    /// @brief Dereference operator to get a mutable reference to the value.
    /// 
    /// Returns a mutable reference to the contained value without checking
    /// if the optional is empty. Use only when you are certain a value is present.
    /// 
    /// @return A mutable reference to the contained value
    T &operator*() noexcept {
        return value_;
    }

    /// @brief Dereference operator to get a const reference to the value.
    /// 
    /// @return A const reference to the contained value
    const T &operator*() const noexcept {
        return value_;
    }

    /// @brief Member access operator for const objects.
    /// 
    /// Provides pointer-like syntax to access members of the contained value.
    /// 
    /// @return A const pointer to the contained value
    const T *operator->() const noexcept {
        return &value_;
    }
    
    /// @brief Member access operator for mutable objects.
    /// 
    /// @return A mutable pointer to the contained value
    T *operator->() noexcept {
        return &value_;
    }

    /// @brief Clear the optional, destroying the contained value if present.
    /// 
    /// After calling reset(), the optional will be empty and has_value() will return false.
    /// 
    /// Example:
    /// @code
    ///   optional<std::string> name("Alice");
    ///   assert(name.has_value());
    ///   name.reset();
    ///   assert(!name.has_value());
    /// @endcode
    void reset() noexcept {
        if (has_value_) {
            has_value_ = false;
            value_.~T();
        }
    }

    /// @brief Less-than comparison operator.
    /// 
    /// Compares two optionals. An empty optional is less than any non-empty optional.
    /// If both contain values, the contained values are compared.
    /// 
    /// @param rhs The optional to compare with
    /// @return true if this optional is less than rhs
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
    /// @brief Whether this optional currently contains a value.
    bool has_value_;
    /// @brief Storage for the contained value (uninitialized if has_value_ is false).
    union { T value_; };
};

/// @brief Create an optional containing a decayed copy of the given value.
/// 
/// This is a convenience function that automatically deduces the type and creates
/// an optional. The contained type is decayed (e.g., arrays become pointers),
/// which prevents some subtle type issues.
/// 
/// This function is necessary in C++17 because std::optional exists but returns
/// a different type (std::optional<decay_t<T>>) that is incompatible with our
/// optional<T> class. We always use our own implementation for consistency.
/// 
/// @tparam T The type of value being wrapped (automatically deduced)
/// @param x The value to wrap in an optional
/// @return An optional containing a decayed copy of x
/// 
/// Example:
/// @code
///   auto opt_int = make_optional(42);  // optional<int>
///   auto opt_str = make_optional(std::string("hello"));  // optional<std::string>
///   
///   // Arrays are decayed to pointers:
///   const char* arr = "test";
///   auto opt_ptr = make_optional(arr);  // optional<const char*>
/// @endcode
template <class T>
optional<typename std::decay<T>::type> make_optional(T &&x) {
    return optional<typename std::decay<T>::type>(std::forward<T>(x));
}

/// @brief Debug print an optional value to a printf buffer.
/// 
/// Prints the optional in a readable format: "opt{value}" if it contains a value,
/// or "none" if it is empty. Used for debugging and logging.
/// 
/// @param buf The printf buffer to write to
/// @param x The optional to print
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
