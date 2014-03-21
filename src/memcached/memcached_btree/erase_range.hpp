// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef MEMCACHED_MEMCACHED_BTREE_ERASE_RANGE_HPP_
#define MEMCACHED_MEMCACHED_BTREE_ERASE_RANGE_HPP_

#include "btree/erase_range.hpp"

void memcached_erase_range(key_tester_t *tester,
                           bool left_key_supplied,
                           const store_key_t& left_key_exclusive,
                           bool right_key_supplied,
                           const store_key_t& right_key_inclusive,
                           superblock_t *superblock, signal_t *interruptor);

void memcached_erase_range(key_tester_t *tester,
                           const key_range_t &keys,
                           superblock_t *superblock, signal_t *interruptor);

#endif /* MEMCACHED_MEMCACHED_BTREE_ERASE_RANGE_HPP_ */
