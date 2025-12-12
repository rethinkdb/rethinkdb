/// @file access.hpp
/// @brief Access type specifications for read/write synchronization
///
/// Provides type-safe enumerations for specifying whether an operation
/// requires read or write access to a resource. Used with synchronization
/// primitives like rwlock_t and watchable_t for access control.
///
/// @defgroup ConcurrencyControl Concurrency and Synchronization Primitives
/// Types and utilities for coordinating concurrent access
/// @{

#ifndef CONCURRENCY_ACCESS_HPP_
#define CONCURRENCY_ACCESS_HPP_

/// @brief Enumeration for read or write access specification
///
/// Used to indicate the type of access required for a resource.
/// The only valid values are `read` and `write`.
///
/// This is used by synchronization primitives to distinguish between
/// shared read access and exclusive write access.
///
/// @example
/// @code
/// rwlock_t lock;
/// 
/// // Request read access
/// auto read_token = lock.begin_read();
/// 
/// // Request write access
/// auto write_token = lock.begin_write();
/// @endcode
enum class access_t {
    read,   ///< Indicates read access (shared)
    write   ///< Indicates write access (exclusive)
};

/// @brief Explicit enumeration for read-only access
///
/// A type-safe way to specify read-only access operations.
/// Useful for function overloading and template specialization
/// where read access is required.
///
/// @example
/// @code
/// class data_store {
/// public:
///     const data_t &get(read_access_t) const {
///         return data_;  // Only accessible for reading
///     }
/// };
/// @endcode
enum class read_access_t {
    read  ///< Indicates read access (shared)
};

/// @brief Explicit enumeration for write access
///
/// A type-safe way to specify write access operations.
/// Useful for function overloading and template specialization
/// where write access is required.
///
/// @example
/// @code
/// class data_store {
/// public:
///     void set(write_access_t, const data_t &new_data) {
///         data_ = new_data;  // Only accessible for writing
///     }
/// };
/// @endcode
enum class write_access_t {
    write  ///< Indicates write access (exclusive)
};

/// @}

#endif  // CONCURRENCY_ACCESS_HPP_
