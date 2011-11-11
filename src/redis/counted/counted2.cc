#include "redis/counted/counted2.hpp"

#include <float.h>
#include "buffer_cache/buffer_cache.hpp"

const counted2_value_t *counted_btree2_t::at(unsigned index) {
    buf_lock_t blk(txn, root->node_id, rwi_read);
    return at_recur(blk, index);
}

void counted_btree2_t::insert(float score, std::string &value) {
    if(root->count == 0) {
        // Allocate the root block
        buf_lock_t new_block;
        new_block.allocate(txn);
        leaf_counted2_node_t *new_node = reinterpret_cast<leaf_counted2_node_t *>(new_block->get_data_major_write());
        new_node->magic = leaf_counted2_node_t::expected_magic();
        new_node->n_refs = 0;
        root->node_id = new_block->get_block_id();
        root->greatest_score = FLT_MAX;
        new_block.release();
    }

    // Pre-emptively increment count
    root->count++;

    buf_lock_t blk(txn, root->node_id, rwi_write);
    block_id_t new_block;
    unsigned new_sub_size;
    float split_score;

    if(insert_recur(blk, score, value, &new_block, &new_sub_size, &split_score)) {
        // The root block was split, we need to creat a new root and insert both the old root and new block

        buf_lock_t new_root;
        new_root.allocate(txn);
        internal_counted2_node_t *root_node = reinterpret_cast<internal_counted2_node_t *>(new_root->get_data_major_write());
        root_node->magic = internal_counted2_node_t::expected_magic();
        root_node->n_refs = 2;

        // Insert old root
        root_node->refs[0].count = root->count - new_sub_size;
        root_node->refs[0].node_id = root->node_id;
        root_node->refs[0].greatest_score = split_score;

        // And new block
        root_node->refs[1].count = new_sub_size;
        root_node->refs[1].node_id = new_block;
        root_node->refs[1].greatest_score = root->greatest_score;

        // And reset root block id (count and score are already correct)
        root->node_id = new_root->get_block_id();
    }
}

void counted_btree2_t::remove(unsigned index) {
    rassert(index <= root->count);

    // remove from root
    buf_lock_t blk(txn, root->node_id, rwi_write);
    remove_recur(blk, index);

    root->count--;
    if(root->count == 0) {
        // Deallocate root block
        blk->mark_deleted();
        root->node_id = NULL_BLOCK_ID;
    }
}

void counted_btree2_t::clear() {
    buf_lock_t blk(txn, root->node_id, rwi_write);
    clear_recur(blk);
    
    // Deallocate root block
    blk->mark_deleted();
    root->node_id = NULL_BLOCK_ID;
    root->count = 0;
}

void counted_btree2_t::clear_recur(buf_lock_t &blk) {
    const counted2_node_t *node = reinterpret_cast<const counted2_node_t *>(blk->get_data_read());
    if(node->magic == internal_counted2_node_t::expected_magic()) {
        internal_clear(blk);    
    } else if(node->magic == leaf_counted2_node_t::expected_magic()) {
        leaf_clear(blk);
    }
}

void counted_btree2_t::internal_clear(buf_lock_t &blk) {
    const internal_counted2_node_t *i_node = reinterpret_cast<const internal_counted2_node_t *>(blk->get_data_read());
    for(int i = 0; i < i_node->n_refs; i++) {
        sub_ref2_t sub_ref = i_node->refs[i];
        buf_lock_t sub_tree(txn, sub_ref.node_id, rwi_write);

        // Clear first
        clear_recur(sub_tree);

        // And deallocate
        sub_tree->mark_deleted();
    }
}

void counted_btree2_t::leaf_clear(buf_lock_t &blk) {
    const leaf_counted2_node_t *l_node = reinterpret_cast<const leaf_counted2_node_t *>(blk->get_data_read());

    unsigned offset = 0;
    for(unsigned i = 0; i < l_node->n_refs; i++) {
        blob_t b(const_cast<char *>(l_node->refs + offset), blob::btree_maxreflen);
        offset += b.refsize(blksize);
        b.clear(txn);
    }
}

// TODO FIX THIS FOR THE NEW FORMAT
const counted2_value_t *counted_btree2_t::at_recur(buf_lock_t &buf, unsigned index) {
    if(index >= root->count) return NULL;

    const counted2_node_t *node = reinterpret_cast<const counted2_node_t *>(buf->get_data_read());

    if(node->magic == internal_counted2_node_t::expected_magic()) {
        const internal_counted2_node_t *i_node = reinterpret_cast<const internal_counted2_node_t *>(node);

        // Find the appropriate sub tree
        unsigned count = 0;
        for(int i = 0; i < i_node->n_refs; i++) {
            sub_ref2_t sub_ref = i_node->refs[i];

            count += sub_ref.count;
            if(count > index) {
                buf_lock_t tmp(txn, sub_ref.node_id, rwi_read);
                buf.swap(tmp);
                tmp.release();
                return at_recur(buf, index - (count - sub_ref.count));
            }
        }
        
        // TODO proper error
        crash("Index not found though it should have been");

    } else if(node->magic == leaf_counted2_node_t::expected_magic()) {
        const leaf_counted2_node_t *l_node = reinterpret_cast<const leaf_counted2_node_t *>(node);

        // Find the value, this involves a linear search through the blobs in this node
        unsigned offset = 0;
        for(unsigned i = 0; i < index; i++) {
            rassert(i != l_node->n_refs);

            blob_t b(const_cast<char *>(l_node->refs + offset), blob::btree_maxreflen);
            offset += b.refsize(blksize);
        }

        return reinterpret_cast<const counted2_value_t *>(l_node->refs + offset);
    }
    
    unreachable();
}

bool counted_btree2_t::insert_recur(buf_lock_t &blk, float score, std::string &value,
        block_id_t *new_blk_out, unsigned *new_size_out, float *split_score_out) {
    const counted2_node_t *node = reinterpret_cast<const counted2_node_t *>(blk->get_data_read());
    if(node->magic == internal_counted2_node_t::expected_magic()) {
        return internal_insert(blk, score, value, new_blk_out, new_size_out, split_score_out);
    } else if(node->magic == leaf_counted2_node_t::expected_magic()) {
        return leaf_insert(blk, score, value, new_blk_out, new_size_out, split_score_out); 
    }

    unreachable();
}

bool counted_btree2_t::internal_insert(buf_lock_t &blk, float score, std::string &value, block_id_t *new_blk_out, unsigned *new_size_out, float *split_score_out) {
    bool split = false;
    const internal_counted2_node_t *node = reinterpret_cast<const internal_counted2_node_t *>(blk->get_data_read());

    // Find the appropriate sub tree
    unsigned insert_index = 0;
    const sub_ref2_t *sub_ref = node->refs;

    while(sub_ref->greatest_score < score) {
        insert_index++;
        sub_ref = node->refs + insert_index;
        rassert(insert_index < node->n_refs);
    }

    // Pre-emptively increment the count of this sub-tree
    uint32_t new_count = sub_ref->count + 1;
    blk->set_data(const_cast<uint32_t *>(&(sub_ref->count)), &new_count, sizeof(new_count));

    // Why didn't we just simply increment sub_ref->count? Beause that's a const reference obtained through get_data_read.
    // We could have used get_data_major write to get a non-const reference instead, but using blk->set_data instead
    // invokes the buffer patch system to save us on IO costs. You'll see this a few more times.

    buf_lock_t sub_tree(txn, sub_ref->node_id, rwi_write);
    block_id_t new_block_id;
    unsigned new_sub_size;
    float split_score;

    if(insert_recur(sub_tree, score, value, &new_block_id, &new_sub_size, &split_score)) {
        // This sub block was split, insert the new reference after this one

        buf_lock_t *insertee = &blk;
        
        // Check to see if we have to split *this* block
        buf_lock_t new_block; // This needs to be here because of scoping issues
        if(sizeof(*node) + (node->n_refs + 1) * sizeof(sub_ref2_t) > blksize.value()) {
            // Split this node!

            unsigned refs_left = node->n_refs / 2;
            unsigned refs_right = node->n_refs - refs_left;

            float split_score = node->refs[refs_left].greatest_score;

            // Allocate the new node
            new_block.allocate(txn);

            internal_counted2_node_t *new_node = reinterpret_cast<internal_counted2_node_t *>(new_block->get_data_major_write());
            new_node->magic = internal_counted2_node_t::expected_magic();
            new_node->n_refs = refs_right;

            // Move the data over
            memcpy(new_node->refs, node->refs + refs_left, refs_right * sizeof(sub_ref2_t));

            // Resize old block
            blk->set_data(const_cast<uint16_t *>(&(node->n_refs)), &refs_left, sizeof(refs_left));

            // Count size of new internal node
            unsigned right_size = 0;
            for(int i = 0; i < new_node->n_refs; i++) {
                right_size += new_node->refs[i].count;
            }

            *new_blk_out = new_block->get_block_id();
            *new_size_out = right_size;
            *split_score_out = split_score;

            if(insert_index > refs_left) {
                insert_index -= refs_left;
                node = new_node;
                insertee = &new_block;
            }

            split = true;
        }

        // Insert the new ref (new ref goes to the *right* of insert_index)
        
        // Shift to make space
        unsigned refs_to_shift = node->n_refs - insert_index - 1;
        if(refs_to_shift) {
            (*insertee)->move_data(const_cast<sub_ref2_t *>(node->refs + insert_index + 2), node->refs + insert_index + 1, refs_to_shift * sizeof(sub_ref2_t));
        }

        // And change the ref count on this node to reflect the insert
        uint16_t new_n_refs = node->n_refs + 1;
        (*insertee)->set_data(const_cast<uint16_t *>(&(node->n_refs)), &new_n_refs, sizeof(new_n_refs));

        // Set new reference
        (*insertee)->set_data(const_cast<block_id_t *>(&(node->refs[insert_index + 1].node_id)), &new_block_id, sizeof(new_block_id));
        (*insertee)->set_data(const_cast<uint32_t *>(&(node->refs[insert_index + 1].count)), &new_sub_size, sizeof(new_sub_size));
        (*insertee)->set_data(const_cast<float *>(&(node->refs[insert_index + 1].greatest_score)), &(sub_ref->greatest_score), sizeof(sub_ref->greatest_score));

        // Fix count and score on old reference
        uint32_t modified_old_sub_size = node->refs[insert_index].count - new_sub_size;
        (*insertee)->set_data(const_cast<uint32_t *>(&(node->refs[insert_index].count)), &modified_old_sub_size, sizeof(modified_old_sub_size));
        (*insertee)->set_data(const_cast<float *>(&(node->refs[insert_index].greatest_score)), &split_score, sizeof(split_score));

    } // else nothing to do at this level

    return split;
}

bool counted_btree2_t::leaf_insert(buf_lock_t &blk, float score, std::string &value, block_id_t *new_blk_out, unsigned *new_size_out, float *split_score_out) {

    bool split = false;

    // Write the ref to local storage first to see how large it will be
    char blob_ref[blob::btree_maxreflen];
    memset(blob_ref, 0, blob::btree_maxreflen);
    blob_t b(blob_ref, blob::btree_maxreflen);
    b.append_region(txn, value.size());
    b.write_from_string(value, txn, 0);
    unsigned refsize = b.refsize(blksize);

    const leaf_counted2_node_t *node = reinterpret_cast<const leaf_counted2_node_t *>(blk->get_data_read());

    // Find the correct blob, this involves a linear search through the blobs in this node
    unsigned offset = 0;
    unsigned i = 0;
    const counted2_value_t *val_ref = reinterpret_cast<const counted2_value_t*>(node->refs);
    while(score < val_ref->score) {
        offset += val_ref->size(blksize);
        val_ref = reinterpret_cast<const counted2_value_t *>(node->refs + offset);
        i++;

        rassert(i < node->n_refs);
    }

    // Count the additional allocated space in the block
    int toshift = 0;
    while(i < node->n_refs) {
        const counted2_value_t *ref = reinterpret_cast<const counted2_value_t*>(node->refs + offset + toshift);
        toshift += ref->size(blksize);
        i++;
    }

    buf_lock_t *insertee = &blk;

    // Before we shift we need to check if this node needs to split
    unsigned total_size = offset + toshift;
    buf_lock_t new_block; // It has to be out here for scoping reasons
    if(sizeof(leaf_counted2_node_t) + total_size + refsize + sizeof(counted2_value_t) > blksize.value()) {
        // split!

        // Find the place to split
        unsigned bytes_left = 0;
        uint16_t refs_left = 0;
        while(bytes_left < total_size / 2) {
            const counted2_value_t *ref = reinterpret_cast<const counted2_value_t*>(node->refs + bytes_left);
            bytes_left += ref->size(blksize); 
            refs_left++;
        }
        unsigned bytes_right = total_size - bytes_left;
        uint16_t refs_right = node->n_refs - refs_left;

        // The split score is the score of the first value in the new right block
        float split_score = reinterpret_cast<const counted2_value_t*>(node->refs + bytes_left)->score;

        // Allocate the new block
        new_block.allocate(txn);

        leaf_counted2_node_t *new_node = reinterpret_cast<leaf_counted2_node_t *>(new_block->get_data_major_write());
        new_node->magic = leaf_counted2_node_t::expected_magic();
        new_node->n_refs = refs_right;

        // Move the data over
        memcpy(new_node->refs, node->refs + bytes_left, bytes_right);
        
        // De-allocate space in old block
        blk->set_data(const_cast<uint16_t *>(&(node->n_refs)), &refs_left, sizeof(refs_left));

        *new_blk_out = new_block->get_block_id();
        *new_size_out = refs_right;
        *split_score_out = split_score;

        // Do we insert into the new block?
        if(offset > bytes_left) {
            offset -= bytes_left;
            node = new_node;
            insertee = &new_block;
            (*new_size_out)++;
        } else {
            toshift -= bytes_right;
        }
        
        split = true;
    }

    const char *place = node->refs + offset;

    // Shift
    (*insertee)->move_data(const_cast<char *>(place + refsize + sizeof(counted2_value_t)), place, toshift);
    uint16_t new_refs = node->n_refs + 1;
    (*insertee)->set_data(const_cast<uint16_t *>(&(node->n_refs)), &new_refs, sizeof(new_refs));

    // And write
    (*insertee)->set_data(const_cast<char *>(place), &score, sizeof(score));
    (*insertee)->set_data(const_cast<char *>(place + sizeof(score)), blob_ref, refsize);

    return split;
}

void counted_btree2_t::remove_recur(buf_lock_t &blk, unsigned index) {
    const counted2_node_t *node = reinterpret_cast<const counted2_node_t *>(blk->get_data_read());
    if(node->magic == internal_counted2_node_t::expected_magic()) {
        internal_remove(blk, index);
    } else if(node->magic == leaf_counted2_node_t::expected_magic()) {
        leaf_remove(blk, index);
    }
}

void counted_btree2_t::internal_remove(buf_lock_t &blk, unsigned index) {
    const internal_counted2_node_t *node = reinterpret_cast<const internal_counted2_node_t *>(blk->get_data_read());

    // Find appropriate sub tree
    unsigned count = 0;
    int remove_index = 0;
    const sub_ref2_t *sub_ref = node->refs;
    while(count + sub_ref->count < index) {
        count += sub_ref->count;
        remove_index++;
        sub_ref = node->refs + index;

        rassert(remove_index != node->n_refs);
    }

    // Remove from sub-tree
    buf_lock_t sub_blk(txn, sub_ref->node_id, rwi_write);
    remove_recur(sub_blk, index - count);

    uint32_t new_count = sub_ref->count - 1;
    blk->set_data(const_cast<uint32_t *>(&(sub_ref->count)), &new_count, sizeof(new_count));

    // Check and handle merge
    // For now, our under-fill condition is empty

    if(sub_ref->count == 0) {
        // Merge (in this case just delete this sub-tree)
        sub_blk->mark_deleted();
        sub_blk.release();

        // Modify ref-count
        uint16_t new_ref_count = node->n_refs - 1;
        blk->set_data(const_cast<uint16_t *>(&(node->n_refs)), &new_ref_count, sizeof(new_ref_count));

        // Shift down other references
        unsigned bytes_to_shift = (node->n_refs - remove_index) * sizeof(sub_ref2_t);
        blk->move_data(const_cast<sub_ref2_t *>(node->refs + remove_index), node->refs + remove_index + 1, bytes_to_shift);
    }
}

void counted_btree2_t::leaf_remove(buf_lock_t &blk, unsigned index) {
    const leaf_counted2_node_t *node = reinterpret_cast<const leaf_counted2_node_t *>(blk->get_data_read());

    // Find appropriate place in this node
    unsigned offset = 0;
    unsigned remove_index = 0;
    while(remove_index < index) {
        rassert(remove_index < node->n_refs);
        blob_t b(const_cast<char *>(node->refs + offset), blob::btree_maxreflen);
        offset += b.refsize(blksize);
        remove_index++;
    }

    const char *place = node->refs + offset;

    // Clear blob
    blob_t b(const_cast<char *>(place), blob::btree_maxreflen);
    unsigned ref_size = b.refsize(blksize);
    b.clear(txn);

    // Shift down data over this old ref

    // src pointer for the memmove
    const char *next_place = node->refs + (offset + ref_size);

    // Count data to shift

    // Skip this blob
    remove_index++;
    offset += ref_size;

    unsigned to_shift = 0;
    while(remove_index < node->n_refs) {
        blob_t b(const_cast<char *>(node->refs + offset), blob::btree_maxreflen);
        offset += b.refsize(blksize);
        remove_index++;
    }

    // Shift down
    blk->move_data(const_cast<char *>(place), next_place, to_shift);
    uint16_t new_ref_count = node->n_refs - 1;
    blk->set_data(const_cast<uint16_t *>(&(node->n_refs)), &new_ref_count, sizeof(new_ref_count));
}

counted_btree2_t::iterator_t counted_btree2_t::score_iterator(float score_min, float score_max) {
    return iterator_t(root->node_id, txn, blksize, score_min, score_max);
}

counted_btree2_t::iterator_t::iterator_t(block_id_t root, transaction_t *txn_, block_size_t& blksize_, float score_min, float score_max) :
    txn(txn_),
    blksize(blksize_),
    max_score(score_max),
    current_rank(0),
    at_end(false)
{
    current_frame = new stack_frame_t();
    buf_lock_t root_blk(txn, root, rwi_read);
    current_frame->blk.swap(root_blk);

    // We can't use a recursive pattern to do the look up so we'll use a loop and
    // maintain a stack of blocks we've traversed
    while(true) {
        const counted2_node_t *node = reinterpret_cast<const counted2_node_t *>(current_frame->blk->get_data_read());
        if(node->magic == internal_counted2_node_t::expected_magic()) {
            const internal_counted2_node_t *internal_node = reinterpret_cast<const internal_counted2_node_t *>(node);
            
            // Find sub tree where the range starts
            const sub_ref2_t *ref = internal_node->refs;
            while(score_min < ref->greatest_score) {
                current_frame->index++;
                rassert(current_frame->index < internal_node->n_refs);
                ref = internal_node->refs + current_frame->index;
                
                // We keep track of where we are rank wise
                current_rank += ref->count;
            }

            stack.push(current_frame);
            current_frame = new stack_frame_t();

            buf_lock_t next_block(txn, ref->node_id, rwi_read);
            current_frame->blk.swap(next_block);

            // And continue to next iteration
        } else if(node->magic == leaf_counted2_node_t::expected_magic()) {
            // We've found the correct leaf node, find the first value in our range

            const leaf_counted2_node_t *leaf_node = reinterpret_cast<const leaf_counted2_node_t *>(node);
            
            // Find our starting point
            current_offset = 0;
            current_val = reinterpret_cast<const counted2_value_t *>(leaf_node->refs);
            while(score_min < current_val->score) {
                rassert(current_frame->index < leaf_node->n_refs);
                current_offset += current_val->size(blksize);
                current_val = reinterpret_cast<const counted2_value_t *>(leaf_node->refs + current_offset);
                current_rank++;
                current_frame->index++;
            }

            // We've found it
            break;
        } else {
            unreachable();
        }
    }

}

counted_btree2_t::iterator_t::~iterator_t() {
    delete current_frame;
    while(!stack.empty()) {
        current_frame = stack.top();
        stack.pop();
        delete current_frame;
    }
}

bool counted_btree2_t::iterator_t::is_valid() {
    return (!at_end) || (score() <= max_score);
}

void counted_btree2_t::iterator_t::next() {
    if(!is_valid()) return;
    
    const leaf_counted2_node_t *leaf_node =
            reinterpret_cast<const leaf_counted2_node_t *>(current_frame->blk->get_data_read());
    rassert(leaf_node->magic == leaf_counted2_node_t::expected_magic());

    current_frame->index++;
    if(current_frame->index >= leaf_node->n_refs) {
        // Move on to the next leaf node by backtracking through the internal nodes we've saved

        const internal_counted2_node_t *internal_node;
        do {
            if(stack.empty()) {
                // We've hit the end of the line
                at_end = true;
                return;
            }
            
            delete current_frame;

            current_frame = stack.top();
            stack.pop();
            current_frame->index++;

            internal_node = reinterpret_cast<const internal_counted2_node_t *>(current_frame->blk->get_data_read());

        } while(current_frame->index < internal_node->n_refs);

        // And back down to the next leaf
        do {
            const sub_ref2_t *ref = internal_node->refs + current_frame->index;
            buf_lock_t next_node(txn, ref->node_id, rwi_read);

            stack.push(current_frame);
            current_frame = new stack_frame_t();
            current_frame->blk.swap(next_node);

            internal_node = reinterpret_cast<const internal_counted2_node_t *>(current_frame->blk->get_data_read());
        } while(internal_node->magic == internal_counted2_node_t::expected_magic());

        // We're back at the leaf level (internal_node is actually a leaf node)
        current_offset = 0;
        leaf_node = reinterpret_cast<const leaf_counted2_node_t *>(internal_node);
    } else {
        current_offset += current_val->size(blksize);
    }

    current_val = reinterpret_cast<const counted2_value_t *>(leaf_node->refs + current_offset);
    current_rank++;
}

float counted_btree2_t::iterator_t::score() {
    return current_val->score;
}

unsigned counted_btree2_t::iterator_t::rank() {
    return current_rank;
}

std::string counted_btree2_t::iterator_t::member() {
    std::string to_return;
    blob_t b(const_cast<char *>(current_val->blb), blob::btree_maxreflen);
    b.read_to_string(to_return, txn, 0, b.valuesize());
    return to_return;
}
