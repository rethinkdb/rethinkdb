// Copyright 2010-2014 RethinkDB, all rights reserved.

/// @file object_buffer.hpp
/// @brief In-place object construction and storage buffer
///
/// Provides a memory buffer for constructing and destructing objects in-place
/// without dynamic allocation. Useful for managing object lifetime in fixed-size
/// contexts like stack allocation or embedded structures.
///
/// @defgroup ObjectBuffering Object Construction and Buffering
/// In-place object construction with lifetime management
/// @{

#ifndef CONTAINERS_OBJECT_BUFFER_HPP_
#define CONTAINERS_OBJECT_BUFFER_HPP_

#include <new>
#include <utility>

#include "errors.hpp"
#include "arch/compiler.hpp"

/// @brief In-place object storage and construction buffer
///
/// Provides a buffer for constructing a single object of type T in-place
/// without using dynamic allocation. The buffer manages the object's lifetime
/// through create(), reset(), and other methods.
///
/// @tparam T The type of object to construct (should have non-blocking destructor)
/// @warning Do not use this with objects that have blocking destructors
/// if you plan to allocate multiple times with the same buffer.
///
/// @example
/// @code
/// object_buffer_t<MyClass> buffer;
///
/// // Construct object with arguments
/// MyClass *obj = buffer.create("arg1", 42);
/// obj->do_work();
///
/// // Destroy and reset
/// buffer.reset();  // or automatic on destruction
/// @endcode
template <class T>
class object_buffer_t {
public:
    /// @brief RAII helper that ensures object is destroyed
    /// Automatically calls reset() in destructor if object exists
    class destruction_sentinel_t {
    public:
        /// @brief Creates a sentinel guarding an object_buffer_t
        /// @param _parent The buffer to guard
        explicit destruction_sentinel_t(object_buffer_t<T> *_parent) : parent(_parent) { }

        /// @brief Destructor ensures the parent buffer is reset
        ~destruction_sentinel_t() {
            if (parent->has()) {
                parent->reset();
            }
        }
    private:
        /// @internal The guarded buffer
        object_buffer_t<T> *parent;

        DISABLE_COPYING(destruction_sentinel_t);
    };

    /// @brief Constructs an empty buffer
    object_buffer_t() : state(EMPTY) { }

    /// @brief Destructor ensures proper cleanup of managed object
    /// The buffer cannot be destroyed while an object is being constructed
    ~object_buffer_t() {
        if (state == INSTANTIATED) {
            reset();
        } else {
            rassert(state == EMPTY);
        }
    }

    /// @brief Constructs an object in-place with the given arguments
    /// @tparam Args Argument types (deduced)
    /// @param args Constructor arguments for the object
    /// @return Pointer to the constructed object
    /// @throws Any exception thrown by the T constructor
    /// @example
    /// @code
    /// MyClass *obj = buffer.create(10, "name");
    /// @endcode
    template <class... Args>
    T *create(Args &&... args) {
        rassert(state == EMPTY, "state is %s", state_string());
        state = CONSTRUCTING;
        try {
            new (&object_data[0]) T(std::forward<Args>(args)...);
        } catch (...) {
            state = EMPTY;
            throw;
        }
        state = INSTANTIATED;
        return get();
    }

    /// @brief Returns a pointer to the managed object
    /// @return Pointer to the object (must have called create() first)
    T *get() {
        rassert(state == INSTANTIATED);
        return reinterpret_cast<T *>(&object_data[0]);
    }

    /// @brief Arrow operator for convenient member access
    /// @return Pointer to the object
    T *operator->() {
        return get();
    }

    /// @brief Returns a const pointer to the managed object
    /// @return Const pointer to the object
    const T *get() const {
        rassert(state == INSTANTIATED);
        return reinterpret_cast<const T *>(&object_data[0]);
    }

    const T *operator->() const {
        return get();
    }

    void reset() {
        guarantee(state == INSTANTIATED || state == EMPTY);
        if (state == INSTANTIATED) {
            T *obj_ptr = get();
            state = DESTRUCTING;
            obj_ptr->~T();
            state = EMPTY;
        }
    }

    bool has() const {
        return (state == INSTANTIATED);
    }

private:
    // Force alignment of the data to the alignment of the templatized type,
    // this avoids some optimization errors, see github issue #3300 for an example.
    ATTR_ALIGNED(alignof(T)) char object_data[sizeof(T)];

    enum buffer_state_t {
        EMPTY,
        CONSTRUCTING,
        INSTANTIATED,
        DESTRUCTING
    } state;

    const char *state_string() {
        switch (state) {
        case buffer_state_t::EMPTY: return "EMPTY";
        case buffer_state_t::CONSTRUCTING: return "CONSTRUCTING";
        case buffer_state_t::INSTANTIATED: return "INSTANTIATED";
        case buffer_state_t::DESTRUCTING: return "DESTRUCTING";
        default: return "corrupted!";
        }
    }

    DISABLE_COPYING(object_buffer_t);
};

#endif  // CONTAINERS_OBJECT_BUFFER_HPP_
