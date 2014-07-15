// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BTREE_DEPTH_FIRST_TRAVERSAL_HPP_
#define BTREE_DEPTH_FIRST_TRAVERSAL_HPP_

#include "btree/keys.hpp"
#include "btree/types.hpp"
#include "containers/archive/archive.hpp"
#include "containers/counted.hpp"

namespace profile { class trace_t; }

class superblock_t;

class depth_first_traversal_callback_t {
public:
    /* Return value of `NO` indicates to keep going; `YES` indicates to stop
    traversing the tree. */
    virtual done_traversing_t handle_pair(scoped_key_value_t &&keyvalue) = 0;
    virtual profile::trace_t *get_trace() THROWS_NOTHING { return NULL; }
protected:
    virtual ~depth_first_traversal_callback_t() { }
};

enum direction_t {
    FORWARD,
    BACKWARD
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(direction_t, int8_t, FORWARD, BACKWARD);

/* Returns `true` if we reached the end of the btree or range, and `false` if
`cb->handle_value()` returned `false`. */
bool btree_depth_first_traversal(superblock_t *superblock,
                                 const key_range_t &range,
                                 depth_first_traversal_callback_t *cb,
                                 direction_t direction,
                                 release_superblock_t release_superblock
                                     = release_superblock_t::RELEASE);

#endif /* BTREE_DEPTH_FIRST_TRAVERSAL_HPP_ */
