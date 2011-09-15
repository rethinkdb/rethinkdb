#ifndef __COUNTED_HPP__
#define __COUNTED_HPP__

#include "buffer_cache/blob.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "protocol/redis/redis_types.hpp"
#include "containers/iterators.hpp"

/*
struct sub_ref_t {
    uint32_t count; // the size of this sub tree
    block_id_t node_id; // The root block of the sub tree
} __attribute__((__packed__));
*/

struct counted_node_t {
    block_magic_t magic;
} __attribute__((__packed__));

struct internal_counted_node_t : counted_node_t {
    // The number of fixed size sub-tree references in this internal node
    uint16_t n_refs;
    
    // The array of sub-tree references
    sub_ref_t refs[0]; 

    static block_magic_t expected_magic() {
        block_magic_t bm = { { 'i', 'n', 't', 'r' } };
        return bm;
    }
} __attribute__((__packed__));

struct leaf_counted_node_t : counted_node_t {
    // The number of blob references stored in this leaf
    uint16_t n_refs;
    char refs[0];

    static block_magic_t expected_magic() {
        block_magic_t bm = { { 'l', 'e', 'a', 'f' } };
        return bm;
    }
} __attribute__((__packed__));


struct counted_btree_t {
    // Give the constructor a pointer to the root block id of the tree. Operations that split the root
    // can then update this root block within the value field of the parent btree
    counted_btree_t(sub_ref_t *root_, block_size_t blocksize, transaction_t *transaction) :
        root(root_), blksize(blocksize), txn(transaction) {;}

    counted_btree_t(block_size_t blocksize) : blksize(blocksize) {;}

    // This pointer actually points to a blob. Use it to initialize one
    const char *at(unsigned index);
    void insert(unsigned index, std::string &value);
    void remove(unsigned index);
    void clear();

protected:
    const char *at_recur(buf_lock_t &buf, unsigned index);
    bool insert_recur(buf_lock_t &blk, unsigned index, std::string &value, block_id_t *new_blk_out, unsigned *new_size_out);
    bool internal_insert(buf_lock_t &blk, unsigned index, std::string &value, block_id_t *new_blk_out, unsigned *new_size_out);
    bool leaf_insert(buf_lock_t &blk, unsigned index, std::string &value, block_id_t *new_blk_out, unsigned *new_size_out);
    void remove_recur(buf_lock_t &blk, unsigned index);
    void internal_remove(buf_lock_t &blk, unsigned index);
    void leaf_remove(buf_lock_t &blk, unsigned index);
    void clear_recur(buf_lock_t &blk);
    void internal_clear(buf_lock_t &blk);
    void leaf_clear(buf_lock_t &blk);

    sub_ref_t *root;
    block_size_t blksize;
    transaction_t *txn;
};

#endif /* __COUNTED_HPP__ */
