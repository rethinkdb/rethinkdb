// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef BUFFER_CACHE_BLOB_HPP_
#define BUFFER_CACHE_BLOB_HPP_

#include <stdint.h>
#include <stddef.h>

#include <string>
#include <vector>

#include "buffer_cache/buffer_cache.hpp"
#include "concurrency/access.hpp"
#include "containers/buffer_group.hpp"

/* An explanation of blobs.

   If we want to store values larger than 250 bytes, we must split
   them into large numbers of blocks.  Some kind of tree structure is
   used.  The blob_t type handles both kinds of values.  Here's how it's used.

const int mrl = 251;
std::string x = ...;

// value_in_leafnode does not point to a buffer that has 251 bytes you can use.
char *ref = get_blob_ref_from_something();

// Create a buffer of the maxreflen
char tmp[mrl];
memcpy(tmp, ref, blob::ref_size(bs, ref, mrl));
{
    blob_t b(tmp, mrl);
    int64_t old_size = b.valuesize();
    tmp.append_region(txn, x.size());

    {
        // group holds pointers to buffers.  acq maintains the buf_lock_t
        // ownership itself.  You cannot use group outside the
        // lifetime of acq.
        blob_acq_t acq;
        buffer_group_t group;

        tmp.expose_region(txn, rwi_write, old_size, x.size(), &group, &acq);
        copy_string_to_buffer_group(&group, x);
    }
}

// The ref size changed because we modified the blob.
write_blob_ref_to_something(tmp, blob::ref_size(bs, ref, mrl));

 */



class buffer_group_t;

// Represents an acquisition of buffers owned by the blob.
class blob_acq_t {
public:
    blob_acq_t() { }
    ~blob_acq_t();

    void add_buf(buf_lock_t *buf) {
        bufs_.push_back(buf);
    }

private:
    std::vector<buf_lock_t *> bufs_;

    // disable copying
    blob_acq_t(const blob_acq_t&);
    void operator=(const blob_acq_t&);
};


union temporary_acq_tree_node_t;

namespace blob {

struct traverse_helper_t;

// Returns the number of bytes actually used by the blob reference.
// Returns a value in the range [1, maxreflen].
int ref_size(block_size_t block_size, const char *ref, int maxreflen);

// Returns true if the size of the blob reference is less than or
// equal to data_length, only reading memory in the range [ref, ref +
// data_length).
bool ref_fits(block_size_t block_size, int data_length, const char *ref, int maxreflen);

// Returns what the maxreflen would be, given the desired number of
// block ids in the blob ref.
int maxreflen_from_blockid_count(int count);

// The step size of a blob.
int64_t stepsize(block_size_t block_size, int levels);

// The internal node block ids of an internal node.
const block_id_t *internal_node_block_ids(const void *buf);

// Returns offset and size, clamped to and relative to the index'th subtree.
void shrink(block_size_t block_size, int levels, int64_t offset, int64_t size, int index, int64_t *suboffset_out, int64_t *subsize_out);

// The maxreflen value appropriate for use with memcached btrees.  It's 251.  This should be renamed.
extern int btree_maxreflen;

// The size of a blob, equivalent to blob_t(ref, maxreflen).valuesize().
int64_t value_size(const char *ref, int maxreflen);

struct ref_info_t {
    // The ref_size of a ref.
    int refsize;
    // the number of levels in the underlying tree of buffers.
    int levels;
};
ref_info_t ref_info(block_size_t block_size, const char *ref, int maxreflen);

// Returns the internal block ids of a non-inlined blob ref.
const block_id_t *block_ids(const char *ref, int maxreflen);

// Returns the char bytes of a leaf node.
const char *leaf_node_data(const void *buf);

// Returns the internal offset of the ref value, which is especially useful when it's not inlined.
int64_t ref_value_offset(const char *ref, int maxreflen);
extern block_magic_t internal_node_magic;
extern block_magic_t leaf_node_magic;

}  // namespace blob

class blob_t {
public:
    // This is a really confusing api. In order for this to create a
    // new blob you need to manually bzero ref.  Otherwise, it will
    // interpret the garbage data as a valid ref.

    // If you're going to modify the blob, ref must be a pointer to an
    // array of length maxreflen.  If you're going to use the blob
    // in-place, it does not need to be so.  (For example, blob refs
    // in leaf nodes usually take much less space -- the average size
    // for a large blob would be half the space (or less, thanks to
    // Benford's law), and the size for a small blob is 1 plus the
    // size of the blob, or maybe 2 plus the size of the blob.
    blob_t(block_size_t block_size, char *ref, int maxreflen);

    // Returns ref_size(block_size, ref, maxreflen), the number of
    // bytes actually used in the blob ref.  A value in the internal
    // [1, maxreflen_].
    int refsize(block_size_t block_size) const;

    // Returns the actual size of the value, some number >= 0 and less
    // than one gazillion.
    int64_t valuesize() const;

    // Acquires internal buffers and copies pointers to internal
    // buffers to the buffer_group_t, initializing acq_group_out so
    // that it holds the acquisition of such buffers.  acq_group_out
    // must not be destroyed until the buffers are finished being
    // used.  If you want to expose the region in a _readonly_
    // fashion, use a const_cast!  I am so sorry.
    void expose_region(transaction_t *txn, access_t mode, int64_t offset, int64_t size, buffer_group_t *buffer_group_out, blob_acq_t *acq_group_out);
    void expose_all(transaction_t *txn, access_t mode, buffer_group_t *buffer_group_out, blob_acq_t *acq_group_out);

    // Appends size bytes of garbage data to the blob.
    void append_region(transaction_t *txn, int64_t size);

    // Prepends size bytes of garbage data to the blob.
    void prepend_region(transaction_t *txn, int64_t size);

    // Removes size bytes of data from the end of the blob.  size must
    // be <= valuesize().
    void unappend_region(transaction_t *txn, int64_t size);

    // Removes size bytes of data from the beginning of the blob.
    // size must be <= valuesize().
    void unprepend_region(transaction_t *txn, int64_t size);

    // Empties the blob, making its valuesize() be zero.  Equivalent
    // to unappend_region(txn, valuesize()) or unprepend_region(txn,
    // valuesize()).  In particular, you can be sure that the blob
    // holds no internal blocks, once it has been cleared.
    void clear(transaction_t *txn);

    // Writes over the portion of the blob, starting at offset, with
    // the contents of the string val. Caller is responsible for making
    // sure this portion of the blob exists
    void write_from_string(const std::string &val, transaction_t *txn, int64_t offset);

private:
    bool traverse_to_dimensions(transaction_t *txn, int levels, int64_t old_offset, int64_t old_size, int64_t new_offset, int64_t new_size, blob::traverse_helper_t *helper);
    bool allocate_to_dimensions(transaction_t *txn, int levels, int64_t new_offset, int64_t new_size);
    bool shift_at_least(transaction_t *txn, int levels, int64_t min_shift);
    void consider_big_shift(transaction_t *txn, int levels, int64_t *min_shift);
    void consider_small_shift(transaction_t *txn, int levels, int64_t *min_shift);
    void deallocate_to_dimensions(transaction_t *txn, int levels, int64_t new_offset, int64_t new_size);
    int add_level(transaction_t *txn, int levels);
    bool remove_level(transaction_t *txn, int *levels_ref);

    char *ref_;
    int maxreflen_;

    // disable copying
    blob_t(const blob_t&);
    void operator=(const blob_t&);
};




#endif  // BUFFER_CACHE_BLOB_HPP_
