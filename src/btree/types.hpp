// Copyright 2010-2014 RethinkDB, all rights reserved.

/// @file types.hpp
/// @brief Common type definitions for B+ tree operations
///
/// Provides fundamental type definitions, enums, and interfaces used
/// throughout the B+ tree implementation for database indexing and storage.
///
/// @defgroup BTreeCore B+ Tree Core Components
/// Core types and interfaces for B+ tree operations
/// @{

#ifndef BTREE_TYPES_HPP_
#define BTREE_TYPES_HPP_

#include "errors.hpp"

class buf_parent_t;

/// @brief Boolean type for traversal control
/// Used to indicate whether a traversal operation should continue or stop.
enum class continue_bool_t {
    CONTINUE = 0,  ///< Continue traversing the tree
    ABORT = 1      ///< Stop traversal (abort operation)
};

/// @brief Interface for custom value deletion in B+ tree nodes
///
/// `value_deleter_t` defines a callback interface for deleting values
/// from B+ tree leaf nodes. Different value types (strings, blobs, documents)
/// may require different cleanup logic, which this interface allows to customize.
///
/// Implementations should inherit from value_deleter_t and override delete_value()
/// to provide custom cleanup for their value type.
///
/// @example
/// @code
/// class my_value_deleter : public value_deleter_t {
/// public:
///     void delete_value(buf_parent_t leaf_node, const void *value) const override {
///         // Custom cleanup for my_value_type
///         my_value_type *v = const_cast<my_value_type *>(
///             static_cast<const my_value_type *>(value));
///         // Cleanup logic...
///         delete v;
///     }
/// };
/// @endcode
///
/// @note The leaf_node parameter allows access to the buffer context
/// @see buf_parent_t for leaf node context
class value_deleter_t {
public:
    /// @brief Default constructor
    value_deleter_t() { }

    /// @brief Deletes a value from a B+ tree leaf node
    /// Called by the tree when a value needs to be removed or replaced.
    /// Implementations should perform any necessary cleanup for the value type.
    /// @param leaf_node The parent buffer context of the leaf node containing the value
    /// @param value Pointer to the value to delete
    virtual void delete_value(buf_parent_t leaf_node, const void *value) const = 0;

protected:
    /// @brief Virtual destructor for proper cleanup
    virtual ~value_deleter_t() { }

    DISABLE_COPYING(value_deleter_t);
};

/// @brief Control flag for superblock release behavior
/// Indicates whether a superblock should be released after an operation.
enum class release_superblock_t {
    RELEASE,  ///< Release/unlock the superblock after operation
    KEEP      ///< Keep/maintain the superblock lock
};

/// @brief Boolean type for stamp read status
/// Indicates whether a read operation included timestamp/stamp information.
enum class is_stamp_read_t {
    NO,   ///< Read did not include stamp information
    YES   ///< Read included stamp information
};

/// @}

#endif /* BTREE_TYPES_HPP_ */
