#ifndef __COUNTED_HPP__
#define __COUNTED_HPP__

#include "buffer_cache/blob.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "redis/redis_types.hpp"
#include "containers/iterators.hpp"
#include <stack>

/*
struct sub_ref_t {
    uint32_t count; // the size of this sub tree
    float greatest_score; // All nodes in this sub-tree are guaranteed to
                          // have a score < this value
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

struct counted_value_t {
    float score;
    char blb[0];

    int size(block_size_t &blksize) const {
        blob_t b(const_cast<char *>(blb), blob::btree_maxreflen);
        return sizeof(counted_value_t) + b.refsize(blksize);
    }
} __attribute__((__packed__));

struct leaf_counted_node_t : counted_node_t {
    // The number of blob references stored in this leaf
    uint16_t n_refs;
    char refs[0]; // refs are to variable sized counted_value_t's

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

    explicit counted_btree_t(block_size_t blocksize) : blksize(blocksize) {;}

    const counted_value_t *at(unsigned index);
    void insert_score(float score, std::string &value);
    void insert_index(unsigned index, std::string &value);
    void remove(unsigned index);
    void clear();

    struct iterator_t {
        iterator_t(block_id_t root, transaction_t *txn, block_size_t &blksize, float score_min, float score_max);
        ~iterator_t();
        bool is_valid();
        void next();
        float score();
        unsigned rank();
        std::string member();

    private:
        struct stack_frame_t {
            stack_frame_t() : index(0) {;}

            unsigned index;
            buf_lock_t blk;
        private:
            DISABLE_COPYING(stack_frame_t);
        };

        transaction_t *txn;
        block_size_t blksize;

        float max_score;
        unsigned current_rank;
        const counted_value_t *current_val;
        unsigned current_offset;
        stack_frame_t *current_frame;
        std::stack<stack_frame_t *> stack;
        float at_end;
    };

    // Returns an iterator over all values with scores between score_min and score_max
    iterator_t score_iterator(float score_min, float score_max);

    // We must support a remove based on score and value

protected:
    const counted_value_t *at_recur(buf_lock_t &buf, unsigned index);
    void insert_root(float score, unsigned index, bool by_score, std::string &value);
    bool insert_recur(buf_lock_t &blk, float score, unsigned index, bool by_score, std::string &value,
            block_id_t *new_blk_out, unsigned *new_size_out, float *split_score_out);
    bool internal_insert(buf_lock_t &blk, float score, unsigned index, bool by_score, std::string &value,
            block_id_t *new_blk_out, unsigned *new_size_out, float *split_score_out);
    bool leaf_insert(buf_lock_t &blk, float score, unsigned index, bool by_score, std::string &value,
            block_id_t *new_blk_out, unsigned *new_size_out, float *split_score_out);
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
