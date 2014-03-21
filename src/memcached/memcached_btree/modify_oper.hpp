// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef MEMCACHED_MEMCACHED_BTREE_MODIFY_OPER_HPP_
#define MEMCACHED_MEMCACHED_BTREE_MODIFY_OPER_HPP_

#include "concurrency/promise.hpp"
#include "containers/scoped.hpp"
#include "memcached/memcached_btree/node.hpp"

class buf_parent_t;

class btree_slice_t;

#define BTREE_MODIFY_OPER_DUMMY_PROPOSED_CAS 0

/* Stats */
class memcached_modify_oper_t {
protected:
    virtual ~memcached_modify_oper_t() { }

public:
    // run_memcached_modify_oper() calls operate() when it reaches the
    // leaf node.  It modifies the value of (or the existence of
    // `value` in some way.  For example, if value contains a NULL
    // pointer, that means no such key-value pair exists.  Setting the
    // value to NULL would mean to delete the key-value pair (but if
    // you do so make sure to wipe out the blob, too).  The return
    // value is true if the leaf node needs to be updated.
    virtual MUST_USE bool operate(buf_parent_t leaf,
                                  scoped_malloc_t<memcached_value_t> *value) = 0;


    virtual MUST_USE int compute_expected_change_count(block_size_t block_size) = 0;
};

class superblock_t;

// Runs a memcached_modify_oper_t.
void run_memcached_modify_oper(memcached_modify_oper_t *oper, btree_slice_t *slice,
                               const store_key_t &key, cas_t proposed_cas,
                               exptime_t effective_time, repli_timestamp_t timestamp,
                               superblock_t *superblock,
                               promise_t<superblock_t *> *pass_back_superblock = NULL);

#endif // MEMCACHED_MEMCACHED_BTREE_MODIFY_OPER_HPP_
