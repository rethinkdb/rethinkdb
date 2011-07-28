#include "btree/operations.hpp"

#include "btree/slice.hpp"

real_superblock_t::real_superblock_t(buf_lock_t &sb_buf) {
    sb_buf_.swap(sb_buf);
}

void real_superblock_t::release() {
    sb_buf_.release_if_acquired();
}
void real_superblock_t::swap_buf(buf_lock_t &swapee) {
    sb_buf_.swap(swapee);
}

block_id_t real_superblock_t::get_root_block_id() const {
    rassert (sb_buf_.is_acquired());
    return reinterpret_cast<const btree_superblock_t *>(sb_buf_.const_buf()->get_data_read())->root_block;
}
void real_superblock_t::set_root_block_id(const block_id_t new_root_block) {
    rassert (sb_buf_.is_acquired());
    // We have to const_cast, because set_data unfortunately takes void* pointers, but get_data_read()
    // gives us const data. No way around this (except for making set_data take a const void * again, as it used to be).
    sb_buf_->set_data(const_cast<block_id_t *>(&reinterpret_cast<const btree_superblock_t *>(sb_buf_.buf()->get_data_read())->root_block), &new_root_block, sizeof(new_root_block));
}
block_id_t real_superblock_t::get_delete_queue_block() const {
    rassert (sb_buf_.is_acquired());
    return reinterpret_cast<const btree_superblock_t *>(sb_buf_.const_buf()->get_data_read())->delete_queue_block;
}
