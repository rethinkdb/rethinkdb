#include "protocol/redis/counted/counted.hpp"

const char *counted_btree_t::at(unsigned index) {
    buf_lock_t blk(txn, root->node_id, rwi_read);
    return at_recur(blk, index);
}

void counted_btree_t::insert(unsigned index, std::string &value) {
    assert(index <= root->count);

    if(root->count == 0) {
        // Allocate the root block
        buf_lock_t new_block;
        new_block.allocate(txn);
        leaf_counted_node_t *new_node = reinterpret_cast<leaf_counted_node_t *>(new_block->get_data_major_write());
        new_node->magic = leaf_counted_node_t::expected_magic();
        new_node->n_refs = 0;
        root->node_id = new_block->get_block_id();
        new_block.release();
    }

    // Pre-emptively increment count
    root->count++;

    buf_lock_t blk(txn, root->node_id, rwi_write);
    block_id_t new_block;
    unsigned new_sub_size;

    if(insert_recur(blk, index, value, &new_block, &new_sub_size)) {
        // The root block was split, we need to creat a new root and insert both the old root and new block

        buf_lock_t new_root;
        new_root.allocate(txn);
        internal_counted_node_t *root_node = reinterpret_cast<internal_counted_node_t *>(new_root->get_data_major_write());
        root_node->magic = internal_counted_node_t::expected_magic();
        root_node->n_refs = 2;

        // Insert old root
        root_node->refs[0].count = root->count - new_sub_size;
        root_node->refs[0].node_id = root->node_id;

        // And new block
        root_node->refs[1].count = new_sub_size;
        root_node->refs[1].node_id = new_block;

        // And reset root block id (count is already correct)
        root->node_id = new_root->get_block_id();
    }
}

void counted_btree_t::remove(unsigned index) {
    assert(index <= root->count);

    // remove from root
    buf_lock_t blk(txn, root->node_id, rwi_write);
    remove_recur(blk, index);

    root->count--;
    if(root->count == 0) {
        // Deallocate root block
        blk->mark_deleted();
        blk->release();
        root->node_id = NULL_BLOCK_ID;
    }
}

const char *counted_btree_t::at_recur(buf_lock_t &buf, unsigned index) {
    if(index >= root->count) return NULL;

    const counted_node_t *node = reinterpret_cast<const counted_node_t *>(buf->get_data_read());

    if(node->magic == internal_counted_node_t::expected_magic()) {
        const internal_counted_node_t *i_node = reinterpret_cast<const internal_counted_node_t *>(node);

        // Find the appropriate sub tree
        unsigned count = 0;
        for(int i = 0; i < i_node->n_refs; i++) {
            sub_ref_t sub_ref = i_node->refs[i];

            count += sub_ref.count;
            if(count > index) {
                buf_lock_t tmp(txn, sub_ref.node_id, rwi_read);
                buf.swap(tmp);
                tmp.release();
                return at_recur(buf, index - (count - sub_ref.count));
            }
        }
        
        // TODO proper error
        assert(0);

    } else if(node->magic == leaf_counted_node_t::expected_magic()) {
        const leaf_counted_node_t *l_node = reinterpret_cast<const leaf_counted_node_t *>(node);

        // Find the value, this involves a linear search through the blobs in this node
        unsigned offset = 0;
        for(unsigned i = 0; i < index; i++) {
            assert(i != l_node->n_refs);

            blob_t b(const_cast<char *>(l_node->refs + offset), blob::btree_maxreflen);
            offset += b.refsize(blksize);
        }

        return l_node->refs + offset;
    }
    
    assert(0);

}

bool counted_btree_t::insert_recur(buf_lock_t &blk, unsigned index, std::string &value, block_id_t *new_blk_out, unsigned *new_size_out) {
    const counted_node_t *node = reinterpret_cast<const counted_node_t *>(blk->get_data_read());
    if(node->magic == internal_counted_node_t::expected_magic()) {
        return internal_insert(blk, index, value, new_blk_out, new_size_out);
    } else if(node->magic == leaf_counted_node_t::expected_magic()) {
        return leaf_insert(blk, index, value, new_blk_out, new_size_out); 
    }

    assert(0);
}

bool counted_btree_t::internal_insert(buf_lock_t &blk, unsigned index, std::string &value, block_id_t *new_blk_out, unsigned *new_size_out) {
    bool split = false;
    const internal_counted_node_t *node = reinterpret_cast<const internal_counted_node_t *>(blk->get_data_read());

    // Find the appropriate sub tree
    unsigned count = 0;
    unsigned insert_index = 0;
    const sub_ref_t *sub_ref = node->refs;

    while(sub_ref->count + count < index) {
        count += sub_ref->count;
        insert_index++;
        sub_ref = node->refs + insert_index;

        assert(insert_index != node->n_refs); // We must be inserting to an index passed the end of the list
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

    if(insert_recur(sub_tree, index - count, value, &new_block_id, &new_sub_size)) {
        // This sub block was split, insert the new reference after this one

        buf_lock_t *insertee = &blk;
        
        // Check to see if we have to split *this* block
        buf_lock_t new_block; // This needs to be here because of scoping issues
        if(sizeof(*node) + (node->n_refs + 1) * sizeof(sub_ref_t) > blksize.value()) {
            // Split this node!

            unsigned refs_left = node->n_refs / 2;
            unsigned refs_right = node->n_refs - refs_left;

            // Allocate the new node
            new_block.allocate(txn);

            internal_counted_node_t *new_node = reinterpret_cast<internal_counted_node_t *>(new_block->get_data_major_write());
            new_node->magic = internal_counted_node_t::expected_magic();
            new_node->n_refs = refs_right;

            // Move the data over
            memcpy(new_node->refs, node->refs + refs_left, refs_right * sizeof(sub_ref_t));

            // Resize old block
            blk->set_data(const_cast<uint16_t *>(&(node->n_refs)), &refs_left, sizeof(refs_left));

            // Count size of new internal node
            unsigned right_size = 0;
            for(int i = 0; i < new_node->n_refs; i++) {
                right_size += new_node->refs[i].count;
            }

            *new_blk_out = new_block->get_block_id();
            *new_size_out = right_size;

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
            (*insertee)->move_data(const_cast<sub_ref_t *>(node->refs + insert_index + 2), node->refs + insert_index + 1, refs_to_shift * sizeof(sub_ref_t));
        }

        // And change the ref count on this node to reflect the insert
        uint16_t new_n_refs = node->n_refs + 1;
        (*insertee)->set_data(const_cast<uint16_t *>(&(node->n_refs)), &new_n_refs, sizeof(new_n_refs));

        // Set new reference
        (*insertee)->set_data(const_cast<block_id_t *>(&(node->refs[insert_index + 1].node_id)), &new_block_id, sizeof(new_block_id));
        (*insertee)->set_data(const_cast<block_id_t *>(&(node->refs[insert_index + 1].count)), &new_sub_size, sizeof(new_sub_size));

        // Fix count on old reference
        uint32_t modified_old_sub_size = node->refs[insert_index].count - new_sub_size;
        (*insertee)->set_data(const_cast<uint32_t *>(&(node->refs[insert_index].count)), &modified_old_sub_size, sizeof(modified_old_sub_size));

    } // else nothing to do at this level

    return split;
}

bool counted_btree_t::leaf_insert(buf_lock_t &blk, unsigned index, std::string &value, block_id_t *new_blk_out, unsigned *new_size_out) {

    bool split = false;

    // Write the ref to local storage first to see how large it will be
    char blob_ref[blob::btree_maxreflen];
    memset(blob_ref, 0, blob::btree_maxreflen);
    blob_t b(blob_ref, blob::btree_maxreflen);
    b.append_region(txn, value.size());
    b.write_from_string(value, txn, 0);
    unsigned refsize = b.refsize(blksize);

    const leaf_counted_node_t *node = reinterpret_cast<const leaf_counted_node_t *>(blk->get_data_read());

    // Find the correct blob, this involves a linear search through the blobs in this node
    unsigned offset = 0;
    unsigned i = 0;
    while(i < index) {
        assert(i <= node->n_refs);
        blob_t b(const_cast<char *>(node->refs + offset), blob::btree_maxreflen);
        offset += b.refsize(blksize);
        i++;
    }

    // Count the additional allocated space in the block
    int toshift = 0;
    while(i < node->n_refs) {
        blob_t b(const_cast<char *>(node->refs + offset), blob::btree_maxreflen);
        toshift += b.refsize(blksize);
        i++;
    }

    buf_lock_t *insertee = &blk;

    // Before we shift we need to check if this node needs to split
    unsigned total_size = offset + toshift;
    buf_lock_t new_block; // It has to be out here for scoping reasons
    if(sizeof(leaf_counted_node_t) + total_size + refsize > blksize.value()) {
        // split!

        // Find the place to split
        unsigned bytes_left = 0;
        uint16_t refs_left = 0;
        while(bytes_left < total_size / 2) {
            blob_t b(const_cast<char *>(node->refs + bytes_left), blob::btree_maxreflen);
            bytes_left += b.refsize(blksize);
            refs_left++;
        }
        unsigned bytes_right = total_size - bytes_left;
        uint16_t refs_right = node->n_refs - refs_left;

        // Allocate the new block
        new_block.allocate(txn);

        leaf_counted_node_t *new_node = reinterpret_cast<leaf_counted_node_t *>(new_block->get_data_major_write());
        new_node->magic = leaf_counted_node_t::expected_magic();
        new_node->n_refs = refs_right;

        // Move the data over
        memcpy(new_node->refs, node->refs + bytes_left, bytes_right);
        
        // De-allocate space in old block
        blk->set_data(const_cast<uint16_t *>(&(node->n_refs)), &refs_left, sizeof(refs_left));

        *new_blk_out = new_block->get_block_id();
        *new_size_out = refs_right;

        // Do we insert into the new block?
        if(index > refs_left) {
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
    (*insertee)->move_data(const_cast<char *>(place + refsize), place, toshift);
    uint16_t new_refs = node->n_refs + 1;
    (*insertee)->set_data(const_cast<uint16_t *>(&(node->n_refs)), &new_refs, sizeof(new_refs));

    // And write
    (*insertee)->set_data(const_cast<char *>(place), blob_ref, refsize);

    return split;
}

void counted_btree_t::remove_recur(buf_lock_t &blk, unsigned index) {
    const counted_node_t *node = reinterpret_cast<const counted_node_t *>(blk->get_data_read());
    if(node->magic == internal_counted_node_t::expected_magic()) {
        internal_remove(blk, index);
    } else if(node->magic == leaf_counted_node_t::expected_magic()) {
        internal_remove(blk, index);
    }
}

void counted_btree_t::internal_remove(buf_lock_t &blk, unsigned index) {
    const internal_counted_node_t *node = reinterpret_cast<const internal_counted_node_t *>(blk->get_data_read());

    // Find appropriate sub tree
    unsigned count = 0;
    int remove_index = 0;
    const sub_ref_t *sub_ref = node->refs;
    while(count + sub_ref->count < index) {
        count += sub_ref->count;
        remove_index++;
        sub_ref = node->refs + index;

        assert(remove_index != node->n_refs);
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
        unsigned bytes_to_shift = (node->n_refs - remove_index) * sizeof(sub_ref_t);
        blk->move_data(const_cast<sub_ref_t *>(node->refs + remove_index), node->refs + remove_index + 1, bytes_to_shift);
    }
}

void counted_btree_t::leaf_remove(buf_lock_t &blk, unsigned index) {
    const leaf_counted_node_t *node = reinterpret_cast<const leaf_counted_node_t *>(blk->get_data_read());

    // Find appropriate place in this node
    unsigned offset = 0;
    unsigned remove_index = 0;
    while(remove_index < index) {
        assert(remove_index < node->n_refs);
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
