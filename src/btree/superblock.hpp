// Copyright 2010-2014 RethinkDB, all rights reserved.

/// @file superblock.hpp
/// @brief B+ tree superblock management with reference counting
///
/// Provides wrapper types for managing B+ tree superblocks with automatic
/// reference counting and lifecycle management.
///
/// @defgroup BTreeCore
/// @{

#ifndef BTREE_SUPERBLOCK_HPP_
#define BTREE_SUPERBLOCK_HPP_

#include "btree/operations.hpp"

/// @brief Reference-counted wrapper for B+ tree superblocks
///
/// `refcount_superblock_t` wraps a base superblock_t with reference counting.
/// This allows multiple operations to safely share a superblock reference,
/// with automatic cleanup when the reference count reaches zero.
///
/// The superblock holds metadata about the B+ tree, including the root block ID
/// and statistics block ID. Reference counting ensures proper resource management
/// when multiple concurrent or sequential operations need access to the tree structure.
///
/// @example
/// @code
/// superblock_t *base_sb = get_base_superblock();
/// refcount_superblock_t ref1(base_sb, 2);  // Initial refcount of 2
/// refcount_superblock_t ref2 = ref1;       // Share the reference
///
/// // Get tree metadata
/// block_id_t root = ref1.get_root_block_id();
///
/// // Release when done (decrements refcount)
/// ref1.release();
/// ref2.release();  // Final release destroys the underlying superblock
/// @endcode
///
/// @note Reference counting is manual - callers must call release() explicitly
/// @see superblock_t for base interface
class refcount_superblock_t : public superblock_t {
public:
    /// @brief Constructs a reference-counted superblock wrapper
    /// @param sb The underlying superblock to wrap
    /// @param rc The initial reference count
    refcount_superblock_t(superblock_t *sb, int rc) :
        sub_superblock(sb), refcount(rc) { }

    /// @brief Decrements the reference count and releases if zero
    /// When the reference count reaches zero, the underlying superblock is released.
    /// @post If refcount reaches 0, sub_superblock is set to nullptr
    void release() {
        refcount--;
        rassert(refcount >= 0);
        if (refcount == 0) {
            sub_superblock->release();
            sub_superblock = nullptr;
        }
    }

    /// @brief Gets the root block ID of the B+ tree
    /// @return The block ID of the root node
    block_id_t get_root_block_id() {
        return sub_superblock->get_root_block_id();
    }

    /// @brief Sets the root block ID when the tree structure changes
    /// @param new_root_block The new root block ID
    void set_root_block_id(block_id_t new_root_block) {
        sub_superblock->set_root_block_id(new_root_block);
    }

    /// @brief Gets the statistics block ID
    /// @return The block ID where tree statistics are stored
    block_id_t get_stat_block_id() {
        return sub_superblock->get_stat_block_id();
    }

    /// @brief Exposes the underlying buffer parent context
    /// @return A buf_parent_t for accessing the superblock buffer
    buf_parent_t expose_buf() {
        return sub_superblock->expose_buf();
    }

private:
    superblock_t *sub_superblock;  ///< The underlying wrapped superblock
    int refcount;                  ///< Reference count (decremented on release())

    DISABLE_COPYING(refcount_superblock_t);
};

/// @}

#endif  // BTREE_SUPERBLOCK_HPP_
