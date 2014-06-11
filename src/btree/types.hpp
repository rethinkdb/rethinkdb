// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BTREE_TYPES_HPP_
#define BTREE_TYPES_HPP_

#include "buffer_cache/alt/alt.hpp" // for buf_parent_t

enum class done_traversing_t { NO, YES };

class value_deleter_t {
public:
    value_deleter_t() { }
    virtual void delete_value(buf_parent_t leaf_node, const void *value) const = 0;

protected:
    virtual ~value_deleter_t() { }

    DISABLE_COPYING(value_deleter_t);
};

/* A deleter that does absolutely nothing. */
class noop_value_deleter_t : public value_deleter_t {
public:
    noop_value_deleter_t() { }
    void delete_value(buf_parent_t, const void *) const { }
};

enum class release_superblock_t {RELEASE, KEEP};

#endif /* BTREE_TYPES_HPP_ */
