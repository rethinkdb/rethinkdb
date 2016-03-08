// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BTREE_SUPERBLOCK_HPP_
#define BTREE_SUPERBLOCK_HPP_

#include "btree/operations.hpp"

class refcount_superblock_t : public superblock_t {
public:
    refcount_superblock_t(superblock_t *sb, int rc) :
        sub_superblock(sb), refcount(rc) { }

    void release() {
        refcount--;
        rassert(refcount >= 0);
        if (refcount == 0) {
            sub_superblock->release();
            sub_superblock = nullptr;
        }
    }

    block_id_t get_root_block_id() {
        return sub_superblock->get_root_block_id();
    }

    void set_root_block_id(block_id_t new_root_block) {
        sub_superblock->set_root_block_id(new_root_block);
    }

    block_id_t get_stat_block_id() {
        return sub_superblock->get_stat_block_id();
    }

    buf_parent_t expose_buf() {
        return sub_superblock->expose_buf();
    }

private:
    superblock_t *sub_superblock;
    int refcount;

    DISABLE_COPYING(refcount_superblock_t);
};


#endif  // BTREE_SUPERBLOCK_HPP_
