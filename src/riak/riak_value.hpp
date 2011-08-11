#ifndef __RIAK_RIAK_VALUE__
#define __RIAK_RIAK_VALUE__

#include "buffer_cache/blob.hpp"
#include "btree/node.hpp"


typedef uint32_t etag_t;

/* TODO there are a 3 ways that links could be put on disk
 *    Method                         Pros                Cons
 * 1: links just go in a blob        Space efficient     Queries take O(n)
 * 2: links go in a btree            Space inefficient   Queiries take Log(n) + m
 * 3: blob for few, btree for many   Best of both worlds We don't have it implemented
 *
 * The right way to do this is 3, for now we're doing 1. 2 will be easier to
 * write if the support operations throught riak like appending to values.
 * Riak's api does not support appending. */
#define MAX_LINK_SIZE 256
struct link_hdr_t {
    uint8_t bucket_len, key_len, tag_len;
} __attribute__((__packed__));

//notice this class does nothing to maintain its consistency
class riak_value_t {
public:
    time_t mod_time;
    etag_t etag;
    uint16_t content_type_len;
    uint32_t value_len;
    uint16_t n_links;
    uint32_t links_length;

    //contents
    char contents[];

    //contents is blob_t which has the following structure:
    // contents = content_type content (link_hdr link)* 

    void print(block_size_t block_size) {
        print_hd(this, 0, offsetof(riak_value_t, contents) + blob::ref_size(block_size, contents, blob::btree_maxreflen));
    }
};

#define MAX_RIAK_VALUE_SIZE (offsetof(riak_value_t, contents) + blob::btree_maxreflen)

template <>
class value_sizer_t<riak_value_t> {
public:
    value_sizer_t(block_size_t bs) : block_size_(bs) { }

    int size(const riak_value_t *value) {
        return offsetof(riak_value_t, contents) + blob::ref_size(block_size_, value->contents, blob::btree_maxreflen);
    }

    // True if size(value) would return no more than length_available.
    // Does not read any bytes outside of [value, value +
    // length_available).
    bool fits(const riak_value_t *value, int length_available) {
        if (length_available < 0) { return false; }
        if ((unsigned) length_available < offsetof(riak_value_t, contents)) { return false; }
        else { return blob::ref_fits(block_size_, length_available - offsetof(riak_value_t, contents), value->contents, blob::btree_maxreflen); }
    }

    int max_possible_size() {
        return offsetof(riak_value_t, contents) + blob::btree_maxreflen;
    }

    // The magic that should be used for btree leaf nodes (or general
    // nodes) with this kind of value.
    static block_magic_t btree_leaf_magic() {
        block_magic_t magic = { { 'r', 'i', 'a', 'k'} };
        return magic;
    }

    block_size_t block_size() const { return block_size_; }

protected:
    // The block size.  It's convenient for leaf node code and for
    // some subclasses, too.
    block_size_t block_size_;
};

#endif
