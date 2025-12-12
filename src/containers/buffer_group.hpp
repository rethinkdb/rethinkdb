// Copyright 2010-2014 RethinkDB, all rights reserved.

/// @file buffer_group.hpp
/// @brief Grouped buffer management for scatter-gather I/O
///
/// Provides containers for managing multiple buffers as a logical group,
/// useful for scatter-gather I/O operations where data is distributed across
/// multiple non-contiguous memory regions.
///
/// @defgroup BufferGroups Buffer Group Management
/// Containers for handling scattered memory buffers as single logical units
/// @{

#ifndef CONTAINERS_BUFFER_GROUP_HPP_
#define CONTAINERS_BUFFER_GROUP_HPP_

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "errors.hpp"

/// @brief Immutable collection of buffer pointers for read operations
///
/// `const_buffer_group_t` represents a collection of read-only buffers that
/// together form a logical data unit. This is useful for scatter-gather read
/// operations or when you need to reference multiple non-contiguous memory regions
/// as a single unit without modifying them.
///
/// @note This class is non-copyable to prevent accidental data duplication
/// @example
/// @code
/// const_buffer_group_t buffers;
/// buffers.add_buffer(100, read_buffer_1);
/// buffers.add_buffer(50, read_buffer_2);
/// 
/// for (size_t i = 0; i < buffers.num_buffers(); ++i) {
///     auto buf = buffers.get_buffer(i);
///     process_data(buf.data, buf.size);
/// }
/// 
/// size_t total = buffers.get_size();  // Returns 150
/// @endcode
class const_buffer_group_t {
public:
    /// @brief Represents a single buffer in the group
    struct buffer_t {
        ssize_t size;        ///< Size of the buffer in bytes
        const void *data;    ///< Pointer to buffer data
    };

    /// @brief Default constructor creates an empty buffer group
    const_buffer_group_t() { }

    /// @brief Adds a read-only buffer to the group
    /// @param s The size of the buffer in bytes
    /// @param d Pointer to the buffer data (must remain valid for group's lifetime)
    void add_buffer(size_t s, const void *d) {
        buffer_t b;
        b.size = s;
        b.data = d;
        buffers_.push_back(b);
    }

    /// @brief Returns the number of buffers in the group
    /// @return The count of buffers added via add_buffer()
    size_t num_buffers() const { return buffers_.size(); }

    /// @brief Retrieves a specific buffer by index
    /// @param i The buffer index (must be < num_buffers())
    /// @return The buffer at index i
    /// @pre i < num_buffers()
    buffer_t get_buffer(size_t i) const {
        rassert(i < buffers_.size());
        return buffers_[i];
    }

    /// @brief Calculates total size of all buffers
    /// @return Sum of all buffer sizes
    size_t get_size() const {
        size_t s = 0;
        for (size_t i = 0; i < buffers_.size(); ++i) {
            s += buffers_[i].size;
        }
        return s;
    }

private:
    std::vector<buffer_t> buffers_;
    DISABLE_COPYING(const_buffer_group_t);
};

/// @brief Mutable collection of buffer pointers for write operations
///
/// `buffer_group_t` is a wrapper around const_buffer_group_t that provides
/// non-const access to buffers. This is used for scatter-gather write operations
/// or when you need to modify data across multiple buffers.
///
/// @note This class is non-copyable to prevent accidental data duplication
/// @example
/// @code
/// buffer_group_t output;
/// output.add_buffer(256, write_buffer_1);
/// output.add_buffer(128, write_buffer_2);
/// 
/// for (size_t i = 0; i < output.num_buffers(); ++i) {
///     auto buf = output.get_buffer(i);
///     fill_with_data(buf.data, buf.size);
/// }
/// @endcode
class buffer_group_t {
public:
    /// @brief Represents a single mutable buffer in the group
    struct buffer_t {
        ssize_t size;    ///< Size of the buffer in bytes
        void *data;      ///< Pointer to buffer data (writable)
    };

    /// @brief Default constructor creates an empty buffer group
    buffer_group_t() { }

    /// @brief Adds a mutable buffer to the group
    /// @param s The size of the buffer in bytes
    /// @param d Pointer to the mutable buffer data
    void add_buffer(size_t s, const void *d) { inner_.add_buffer(s, d); }

    /// @brief Returns the number of buffers in the group
    /// @return The count of buffers added via add_buffer()
    size_t num_buffers() const { return inner_.num_buffers(); }

    /// @brief Retrieves a specific mutable buffer by index
    /// @param i The buffer index (must be < num_buffers())
    /// @return The mutable buffer at index i
    /// @pre i < num_buffers()
    buffer_t get_buffer(size_t i) const {
        rassert(i < inner_.num_buffers());
        buffer_t ret;
        const_buffer_group_t::buffer_t tmp = inner_.get_buffer(i);
        ret.size = tmp.size;
        ret.data = const_cast<void *>(tmp.data);
        return ret;
    }

    /// @brief Calculates total size of all buffers
    /// @return Sum of all buffer sizes
    size_t get_size() const { return inner_.get_size(); }

    /// @brief Gets a const view of this buffer group
    /// @param group Pointer to a buffer_group_t
    /// @return Const view of the same buffer collection
    friend const const_buffer_group_t *const_view(const buffer_group_t *group);

private:
    const_buffer_group_t inner_;
    DISABLE_COPYING(buffer_group_t);
};

/// @brief Gets an immutable view of a mutable buffer group
/// Provides read-only access to a buffer_group_t's buffers without modification.
/// @param group Pointer to the buffer group
/// @return Const pointer to the underlying const_buffer_group_t
inline const const_buffer_group_t *const_view(const buffer_group_t *group) {
    return &group->inner_;
}

/// @brief Copies data from source buffer group to destination buffer group
/// Both groups must have identical total size. Copies bytes sequentially from
/// source buffers into destination buffers.
/// @param out The destination buffer group
/// @param in The source const buffer group
/// @pre Total size of out == total size of in
void buffer_group_copy_data(const buffer_group_t *out, const const_buffer_group_t *in);

/// @brief Copies data from a contiguous buffer to a buffer group
/// Distributes data from a single contiguous source into multiple destination buffers.
/// @param out The destination buffer group to fill
/// @param in Pointer to source data
/// @param size Number of bytes to copy
/// @pre out has sufficient total capacity for size bytes
void buffer_group_copy_data(const buffer_group_t *out, const char *in, int64_t size);

/// @}

#endif  // CONTAINERS_BUFFER_GROUP_HPP_
