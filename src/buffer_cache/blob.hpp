// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BUFFER_CACHE_BLOB_HPP_
#define BUFFER_CACHE_BLOB_HPP_

#include <stdint.h>
#include <stddef.h>

#include <string>
#include <vector>
#include <utility>

#include "buffer_cache/types.hpp"
#include "concurrency/access.hpp"
#include "containers/buffer_group.hpp"
#include "errors.hpp"
#include "serializer/types.hpp"
#include "utils.hpp"

class buffer_group_t;

/* An explanation of blobs.

   If we want to store values larger than 250 bytes (or some arbitrary inline value
   size limit), we must split them into large numbers of blocks.  Some kind of tree
   structure is used.  The blob_t type handles both kinds of values.  Here's how it's
   used.

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
    tmp.append_region(leafnode, x.size());

    {
        // group holds pointers to buffers.  acq maintains the buf_lock_t
        // ownership itself.  You cannot use group outside the
        // lifetime of acq.
        blob_acq_t acq;
        buffer_group_t group;

        tmp.expose_region(leafnode, access_t::write, old_size, x.size(),
                          &group, &acq);
        // There are better ways to shovel data into a buffer group if it's not
        // already in a string -- please avoid excessive copying!
        copy_string_to_buffer_group(&group, x);
    }
}

// The ref size changed because we modified the blob.
write_blob_ref_to_something(tmp, blob::ref_size(bs, ref, mrl));

 */

class buf_lock_t;
class buf_parent_t;
class buf_read_t;
class buf_write_t;

// Represents an acquisition of buffers owned by the blob.
class blob_acq_t {
public:
    blob_acq_t() { }
    ~blob_acq_t();

    void add_buf(buf_lock_t *buf, buf_write_t *write) {
        bufs_.push_back(buf);
        writes_.push_back(write);
    }

    void add_buf(buf_lock_t *buf, buf_read_t *read) {
        bufs_.push_back(buf);
        reads_.push_back(read);
    }

private:
    std::vector<buf_lock_t *> bufs_;
    // One of writes_ or reads_ will be empty.
    std::vector<buf_write_t *> writes_;
    std::vector<buf_read_t *> reads_;

    // disable copying
    blob_acq_t(const blob_acq_t&);
    void operator=(const blob_acq_t&);
};


union temporary_acq_tree_node_t;

namespace blob {

struct traverse_helper_t;

// Returns the number of bytes actually used by the blob reference.
// Returns a value in the range [1, maxreflen].
int ref_size(max_block_size_t block_size, const char *ref, int maxreflen);

// Returns true if the size of the blob reference is less than or
// equal to data_length, only reading memory in the range [ref, ref +
// data_length).
bool ref_fits(max_block_size_t block_size, int data_length, const char *ref, int maxreflen);

// Returns what the maxreflen would be, given the desired number of
// block ids in the blob ref.
int maxreflen_from_blockid_count(int count);

// The step size of a blob.
int64_t stepsize(max_block_size_t block_size, int levels);

// Returns offset and size, clamped to and relative to the index'th subtree.
void shrink(max_block_size_t block_size, int levels, int64_t offset, int64_t size, int index, int64_t *suboffset_out, int64_t *subsize_out);

// The maxreflen value (allegedly) appropriate for use with rdb_protocol btrees.
// It's 251.  This should be renamed.
extern int btree_maxreflen;

// The size of a blob, equivalent to blob_t(ref, maxreflen).valuesize().
int64_t value_size(const char *ref, int maxreflen);

struct ref_info_t {
    // The ref_size of a ref.
    int refsize;
    // the number of levels in the underlying tree of buffers.
    int levels;
};
ref_info_t ref_info(max_block_size_t block_size, const char *ref, int maxreflen);

// Returns the char bytes of a leaf node.
const char *leaf_node_data(const void *buf);

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
    blob_t(max_block_size_t block_size, char *ref, int maxreflen);

    // Returns ref_size(block_size, ref, maxreflen), the number of
    // bytes actually used in the blob ref.  A value in the internal
    // [1, maxreflen_].
    int refsize(max_block_size_t block_size) const;

    // Returns the actual size of the value, some number >= 0 and less
    // than one gazillion.
    int64_t valuesize() const;

    // Detaches the blob's subtrees from the root node (see
    // buf_lock_t::detach_child).
    void detach_subtrees(buf_parent_t root);

    // Acquires internal buffers and copies pointers to internal
    // buffers to the buffer_group_t, initializing acq_group_out so
    // that it holds the acquisition of such buffers.  acq_group_out
    // must not be destroyed until the buffers are finished being
    // used.  If you want to expose the region in a _readonly_
    // fashion, use a const_cast!  I am so sorry.
    void expose_region(buf_parent_t root,
                       access_t mode, int64_t offset,
                       int64_t size, buffer_group_t *buffer_group_out,
                       blob_acq_t *acq_group_out);
    void expose_all(buf_parent_t root, access_t mode,
                    buffer_group_t *buffer_group_out,
                    blob_acq_t *acq_group_out);

    // Appends size bytes of garbage data to the blob.
    void append_region(buf_parent_t root, int64_t size);

    // Removes size bytes of data from the end of the blob.  size must
    // be <= valuesize().
    void unappend_region(buf_parent_t root, int64_t size);

    // Empties the blob, making its valuesize() be zero.  Equivalent to
    // unappend_region(txn, valuesize()).  In particular, you can be sure that the
    // blob holds no internal blocks, once it has been cleared.
    void clear(buf_parent_t root);

    // Writes over the portion of the blob, starting at offset, with
    // the contents of the string val. Caller is responsible for making
    // sure this portion of the blob exists
    void write_from_string(const std::string &val, buf_parent_t root,
                           int64_t offset);

private:
    bool traverse_to_dimensions(buf_parent_t parent, int levels,
                                int64_t smaller_size, int64_t bigger_size,
                                blob::traverse_helper_t *helper);
    bool allocate_to_dimensions(buf_parent_t parent, int levels,
                                int64_t new_size);
    bool shift_at_least(buf_parent_t parent, int levels, int64_t min_shift);
    void consider_big_shift(buf_parent_t parent, int levels,
                            int64_t *min_shift);
    void consider_small_shift(buf_parent_t parent, int levels,
                              int64_t *min_shift);
    void deallocate_to_dimensions(buf_parent_t parent, int levels,
                                  int64_t new_size);
    int add_level(buf_parent_t parent, int levels);
    bool remove_level(buf_parent_t parent, int *levels_ref);

    char *ref_;
    int maxreflen_;

    DISABLE_COPYING(blob_t);
};


#endif  // BUFFER_CACHE_BLOB_HPP_
