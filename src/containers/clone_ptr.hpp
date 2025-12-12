// Copyright 2010-2014 RethinkDB, all rights reserved.

/// @file clone_ptr.hpp
/// @brief Smart pointer with value semantics and deep copying
///
/// Provides a smart pointer that automatically clones the managed object
/// when copied. Useful for polymorphic types that should behave like values
/// despite having virtual methods.
///
/// @defgroup ClonePointers Clone Pointer (Value-Semantic Smart Pointers)
/// Smart pointers with automatic deep copying semantics
/// @{

#ifndef CONTAINERS_CLONE_PTR_HPP_
#define CONTAINERS_CLONE_PTR_HPP_

#include "containers/archive/archive.hpp"
#include "containers/scoped.hpp"
#include "rpc/serialize_macros.hpp"

/// @brief Smart pointer with value semantics and deep copying
///
/// `clone_ptr_t` is a smart pointer that calls the `clone()` method on its
/// underlying object whenever the `clone_ptr_t`'s copy constructor is called.
/// This allows polymorphic types (with virtual methods) to behave like value
/// types, automatically creating deep copies on assignment and copy construction.
///
/// This is particularly useful when you have a type that effectively acts like
/// a piece of data (i.e. it can be meaningfully copied) but it also has virtual
/// methods, which would normally prevent proper value semantics.
///
/// @tparam T The type of object being managed (must have a virtual clone() method)
/// @warning Remember to declare `clone()` as a virtual method!
///
/// @example
/// @code
/// class Shape {
/// public:
///     virtual ~Shape() { }
///     virtual Shape *clone() const = 0;
///     virtual void draw() = 0;
/// };
///
/// class Circle : public Shape {
/// public:
///     Circle *clone() const override {
///         return new Circle(*this);
///     }
///     void draw() override { /* ... */ }
/// };
///
/// // Use clone_ptr_t for value semantics
/// clone_ptr_t<Shape> shape1(new Circle());
/// clone_ptr_t<Shape> shape2 = shape1;  // Deep copy via clone()
/// shape2->draw();  // Works! shape2 is an independent copy
/// @endcode
template<class T>
class clone_ptr_t {
public:
    /// @brief Constructs an empty clone pointer
    clone_ptr_t();

    /// @brief Constructs a clone pointer managing the given object
    /// Takes ownership of the argument.
    /// @param p Pointer to object to manage
    explicit clone_ptr_t(T *p);

    /// @brief Move constructor transfers ownership without cloning
    /// @param movee The source pointer (will be emptied)
    clone_ptr_t(clone_ptr_t &&movee) noexcept : object(std::move(movee.object)) { }

    /// @brief Move constructor with type conversion
    /// Allows implicit conversion from derived classes
    /// @tparam U Type convertible to T*
    /// @param movee The source pointer with different type
    template<class U>
    clone_ptr_t(clone_ptr_t<U> &&movee) noexcept : object(std::move(movee.object)) { }

    /// @brief Copy constructor creates a deep copy via clone()
    /// Creates a new object by calling clone() on the source
    /// @param x The source clone pointer to deep copy from
    clone_ptr_t(const clone_ptr_t &x);

    /// @brief Copy constructor with type conversion
    /// Allows implicit conversion from derived classes
    /// @tparam U Type convertible to T*
    /// @param x The source clone pointer with different type
    template<class U>
    clone_ptr_t(const clone_ptr_t<U> &x);  // NOLINT(runtime/explicit)

    /// @brief Copy assignment creates a deep copy
    /// @param x The source clone pointer
    /// @return Reference to this pointer
    clone_ptr_t &operator=(const clone_ptr_t &x);

    /// @brief Move assignment transfers ownership
    /// @param x The source clone pointer
    /// @return Reference to this pointer
    clone_ptr_t &operator=(const clone_ptr_t &&x) noexcept;

    /// @brief Copy assignment with type conversion
    /// @tparam U Type convertible to T*
    /// @param x The source clone pointer with different type
    /// @return Reference to this pointer
    template<class U>
    clone_ptr_t &operator=(const clone_ptr_t<U> &x);

    /// @brief Move assignment with type conversion
    /// @tparam U Type convertible to T*
    /// @param x The source clone pointer with different type
    /// @return Reference to this pointer
    template<class U>
    clone_ptr_t &operator=(const clone_ptr_t<U> &&x) noexcept;

    /// @brief Dereferences the pointer to get a reference to the object
    /// @return Reference to the managed object
    T &operator*() const;

    /// @brief Arrow operator for convenient member access
    /// @return Pointer to the managed object
    T *operator->() const;

    /// @brief Returns the raw pointer to the managed object
    /// @return Pointer to the object, or nullptr if empty
    T *get() const;

    /// @brief Checks if this pointer owns an object
    /// @return true if an object is being managed, false otherwise
    bool has() const {
        return object.has();
    }

private:
    /// @internal Friend declaration for type conversion
    template<class U> friend class clone_ptr_t;

    /// @internal The underlying owned object
    scoped_ptr_t<T> object;
};

#include "containers/clone_ptr.tcc"

/// @}

#endif /* CONTAINERS_CLONE_PTR_HPP_ */
