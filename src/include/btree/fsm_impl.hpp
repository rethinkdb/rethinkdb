
#ifndef __BTREE_FSM_IMPL_HPP__
#define __BTREE_FSM_IMPL_HPP__

// TODO: mapping only int->int, allow arbitrary key and value types
// TODO: ignoring duplicates, allow duplicate elements
// TODO: not thread safe, implement concurrency control methods
// TODO: multiple values require cursor/iterator mechanism
// TODO: consider redoing nodes with a visitor pattern to avoid ugly casts

template <class config_t>
int btree_fsm<config_t>::get_root_id(block_id_t *root_id) {
    block_id_t superblock_id = btree_fsm_t::cache->get_superblock_id();
    void *buf = btree_fsm_t::cache->acquire(superblock_id, btree_fsm_t::netfsm);
    if(buf == NULL) {
        return 0;
    }
    memcpy((void*)root_id, buf, sizeof(*root_id));
    btree_fsm_t::cache->release(superblock_id, buf, false, btree_fsm_t::netfsm);
    return 1;
}

#endif // __BTREE_FSM_IMPL_HPP__

