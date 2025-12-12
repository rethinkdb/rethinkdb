// Copyright 2010-2014 RethinkDB, all rights reserved.

/// @file shared_buffer.hpp
/// @brief Reference-counted binary buffer for shared data ownership
///
/// Provides a reference-counted binary buffer allowing multiple references
/// to different offsets within the same underlying buffer. Useful for sharing
/// large binary data across multiple components with automatic cleanup.
///
/// @defgroup SharedBuffers Reference-Counted Binary Buffers
/// Memory management for shared binary data
/// @{

#ifndef CONTAINERS_SHARED_BUFFER_HPP_
#define CONTAINERS_SHARED_BUFFER_HPP_

#include <atomic>

#include "containers/counted.hpp"
#include "errors.hpp"

/// @brief Reference-counted binary buffer for shared ownership
///
/// `shared_buf_t` is a reference-counted buffer that can be shared across
/// multiple components. Unlike simple pointers, multiple shared_buf_ref_t instances
/// can safely hold references to the same buffer even at different offsets.
/// The buffer is automatically deleted when the last reference is released.
///
/// Memory layout is carefully controlled to allow custom allocation and placement
/// of the data field after the header.
///
/// @note This class is non-copyable and uses custom new/delete operators
/// @example
/// @code
/// // Create a shared buffer
/// auto buffer = shared_buf_t::create(1024);
/// 
/// // Create references at different offsets
/// shared_buf_ref_t<char> ref1(buffer, 0);
/// shared_buf_ref_t<char> ref2(buffer, 512);
/// 
/// // Both references share the same underlying buffer
/// memcpy(ref1.get(), "data1", 5);
/// memcpy(ref2.get(), "data2", 5);
/// @endcode
class shared_buf_t {
public:
    /// @brief Deleted default constructor
    /// Buffers must be created via create()
    shared_buf_t() = delete;

    /// @brief Factory function to create a reference-counted buffer
    /// Allocates a buffer of the specified size with reference counting overhead.
    /// @param _size The size of the buffer in bytes
    /// @return A counted_t reference to the newly created buffer
    static counted_t<shared_buf_t> create(size_t _size);

    /// @brief Custom delete operator for reference-counted cleanup
    /// Called automatically by reference counting framework
    /// @param p Pointer to buffer being deleted
    static void operator delete(void *p);

    /// @brief Gets mutable pointer to buffer data at optional offset
    /// @param offset Offset within the buffer (default: 0)
    /// @return Mutable pointer to data at the given offset
    char *data(size_t offset = 0);

    /// @brief Gets const pointer to buffer data at optional offset
    /// @param offset Offset within the buffer (default: 0)
    /// @return Const pointer to data at the given offset
    const char *data(size_t offset = 0) const;

    /// @brief Gets the total size of the buffer
    /// @return Size in bytes
    size_t size() const;

private:
    // We duplicate the implementation of slow_atomic_countable_t here for the
    // sole purpose of having full control over the layout of fields. This
    // is required because we manually allocate memory for the data field,
    // and C++ doesn't guarantee any specific field memory layout under inheritance
    // (as far as I know).
    friend void counted_add_ref(const shared_buf_t *p);
    friend void counted_release(const shared_buf_t *p);
    friend intptr_t counted_use_count(const shared_buf_t *p);

    mutable std::atomic<intptr_t> refcount_;  ///< Atomic reference count

    // The size of data_, for boundary checking.
    size_t size_;

    // We actually allocate more memory than this.
    // It's crucial that this field is the last one in this class.
    char data_[1];  ///< Variable-length data array (actually larger)

    DISABLE_COPYING(shared_buf_t);
};

/// @brief Reference to a type within a shared buffer at a specific offset
///
/// `shared_buf_ref_t<T>` provides a typed reference to data within a shared_buf_t
/// at a specific byte offset. Multiple references can point to the same buffer
/// at different offsets without copying.
///
/// The template parameter T allows typed access (e.g., shared_buf_ref_t<uint32_t>).
/// Boundary checking ensures reads don't exceed buffer bounds.
///
/// @tparam T The type to interpret the buffer contents as
/// @note Memory layout is optimized for size
/// @example
/// @code
/// shared_buf_ref_t<uint32_t> numbers(buffer, 0);
/// uint32_t first_number = *numbers.get();
///
/// shared_buf_ref_t<uint32_t> next_number = 
///     numbers.make_child(sizeof(uint32_t));
/// @endcode
template <class T>
class shared_buf_ref_t {
public:
    /// @brief Default constructor creates null reference
    shared_buf_ref_t() : offset(0) { }

    /// @brief Constructs a reference to data at a specific offset
    /// @param _buf The shared buffer to reference
    /// @param _offset The byte offset within the buffer
    /// @pre _buf must be valid and _offset must be in bounds
    shared_buf_ref_t(const counted_t<const shared_buf_t> &_buf, size_t _offset)
        : buf(_buf), offset(_offset) {
        rassert(buf.has());
    }

    /// @brief Gets the pointer to the referenced data
    /// @return Const pointer to T at the specified offset
    /// @pre The reference must be valid
    const T *get() const {
        rassert(buf.has());
        rassert(buf->size() >= offset);
        return reinterpret_cast<const T *>(buf->data(offset));
    }

    /// @brief Creates a child reference at a relative offset
    /// Useful for creating references to nested structures or array elements.
    /// @param relative_offset The offset relative to this reference
    /// @return A new reference at this offset + relative_offset
    /// @pre relative_offset must be within buffer bounds
    shared_buf_ref_t make_child(size_t relative_offset) const {
        guarantee_in_boundary(relative_offset);
        return shared_buf_ref_t(buf, offset + relative_offset);
    }

    /// @brief Validates that buffer has space for N elements of type T
    /// Ensures that reading num_elements elements won't exceed buffer bounds.
    /// @param num_elements Number of elements to validate space for
    /// @pre num_elements * sizeof(T) <= remaining buffer space
    void guarantee_in_boundary(size_t num_elements) const {
        guarantee(get_safety_boundary() >= num_elements);
    }

    /// @brief Gets safe upper bound on readable elements from this position
    /// Calculates the maximum number of complete T elements that can be safely
    /// read from this reference's offset to the end of the buffer.
    /// @return Number of elements of type T that can be safely read
    size_t get_safety_boundary() const {
        rassert(buf.has());
        rassert(buf->size() >= offset);
        return (buf->size() - offset) / sizeof(T);
    }

private:
    counted_t<const shared_buf_t> buf;  ///< Reference to the underlying shared buffer
    size_t offset;                      ///< Byte offset within the buffer
};

/// @brief Increments the reference count for a shared buffer
/// Called automatically by counted_t
/// @param p Pointer to the shared buffer
inline void counted_add_ref(const shared_buf_t *p) {
    DEBUG_VAR intptr_t res = ++(p->refcount_);
    rassert(res > 0);
}

/// @brief Decrements the reference count and deletes if zero
/// Called automatically by counted_t when references are released
/// @param p Pointer to the shared buffer
inline void counted_release(const shared_buf_t *p) {
    int64_t res = --(p->refcount_);
    rassert(res >= 0);
    if (res == 0) {
        delete const_cast<shared_buf_t *>(p);
    }
}

/// @brief Gets the current reference count
/// Used for debugging and validation
/// @param p Pointer to the shared buffer
/// @return Current reference count (must be > 0)
inline intptr_t counted_use_count(const shared_buf_t *p) {
    // Finally a practical use for volatile.
    intptr_t tmp = p->refcount_.load();
    rassert(tmp > 0);
    return tmp;
}

/// @}

#endif  // CONTAINERS_SHARED_BUFFER_HPP_
