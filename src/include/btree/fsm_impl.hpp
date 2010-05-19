
#ifndef __BTREE_FSM_IMPL_HPP__
#define __BTREE_FSM_IMPL_HPP__

// TODO: mapping only int->int, allow arbitrary key and value types
// TODO: ignoring duplicates, allow duplicate elements
// TODO: not thread safe, implement concurrency control methods
// TODO: multiple values require cursor/iterator mechanism
// TODO: consider redoing nodes with a visitor pattern to avoid ugly casts

template <class config_t>
typename btree_fsm<config_t>::block_id_t btree_fsm<config_t>::get_root_id(void *superblock_buf) {
    return *((block_id_t*)superblock_buf);
}

#endif // __BTREE_FSM_IMPL_HPP__

