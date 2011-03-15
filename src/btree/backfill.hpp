#ifndef __BTREE_BACKFILL_HPP__
#define __BTREE_BACKFILL_HPP__

#include "utils2.hpp"

class btree_slice_t;
class backfill_callback_t;

void spawn_btree_backfill(btree_slice_t *slice, repli_timestamp since_when, backfill_callback_t *callback);



#endif  // __BTREE_BACKFILL_HPP__
