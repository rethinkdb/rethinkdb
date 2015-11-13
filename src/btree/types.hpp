// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BTREE_TYPES_HPP_
#define BTREE_TYPES_HPP_

#include "errors.hpp"

class buf_parent_t;

enum class continue_bool_t { CONTINUE = 0, ABORT = 1 };

class value_deleter_t {
public:
    value_deleter_t() { }
    virtual void delete_value(buf_parent_t leaf_node, const void *value) const = 0;

protected:
    virtual ~value_deleter_t() { }

    DISABLE_COPYING(value_deleter_t);
};

enum class release_superblock_t {RELEASE, KEEP};

#endif /* BTREE_TYPES_HPP_ */
