// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_BLOB_WRAPPER_HPP_
#define RDB_PROTOCOL_BLOB_WRAPPER_HPP_

#include <string>

#include "btree/btree_store.hpp"
#include "buffer_cache/alt/blob.hpp"

/* This class wraps a blob_t but hides some of its methods. We do this because
 * due to storing multiple references to blobs in btrees there are methods that
 * blob_ts support that are very dangerous to use in the rdb code. Every method
 * that this class does expose is identical to the blob_t method with the same
 * name except that it may add assertions. */
class rdb_blob_wrapper_t {
public:
    rdb_blob_wrapper_t(block_size_t block_size, char *ref, int maxreflen);
    /* The allows you to write some data to the blob as well. This is because
     * the methods to do this are normally not present due to shared
     * references. */
    rdb_blob_wrapper_t(block_size_t block_size, char *ref, int maxreflen,
                       buf_parent_t parent, const std::string &data);

    int refsize(block_size_t block_size) const;

    int64_t valuesize() const;

    /* This function only works in read mode. */
    void expose_all(buf_parent_t parent, access_t mode,
                    buffer_group_t *buffer_group_out,
                    blob_acq_t *acq_group_out);

private:
    friend class rdb_value_deleter_t;

    /* This method is inherently unsafe but also necessary. It's private so
     * that you have to explicitly white list places where it occurs. If you're
     * not 100% sure it's safe to call clear from a certain location don't
     * friend it. */
    void clear(buf_parent_t parent);

    blob_t internal;
};

#endif  // RDB_PROTOCOL_BLOB_WRAPPER_HPP_
