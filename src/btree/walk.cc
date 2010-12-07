#include "btree/walk.hpp"
#include "buffer_cache/large_buf.hpp"
#include "btree/node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/internal_node.hpp"

struct store_walker_t;
struct slice_walker_t;

void walk_slice(store_walker_t *parent, btree_slice_t *slice);
void walk_branch(slice_walker_t *parent, block_id_t node);

struct store_walker_t {
    store_t::replicant_t *callback;
    int active_slice_walkers;
    store_walker_t(btree_key_value_store_t *store, store_t::replicant_t *cb)
        : callback(cb)
    {
        active_slice_walkers = store->btree_static_config.n_slices;
        for (int i = 0; i < store->btree_static_config.n_slices; i++) {
            walk_slice(this, store->slices[i]);
        }
    }
    void done() {
        delete this;
    }
};

void walk_btrees(btree_key_value_store_t *store, store_t::replicant_t *cb) {
    new store_walker_t(store, cb);
}

struct slice_walker_t :
    public home_cpu_mixin_t,
    public block_available_callback_t
{
    btree_slice_t *slice;
    int active_branch_walkers;
    transaction_t *txn;
    store_walker_t *parent;
    slice_walker_t(store_walker_t *parent, btree_slice_t *slice)
        : slice(slice), active_branch_walkers(0), parent(parent)
    {
        do_on_cpu(slice->home_cpu, this, &slice_walker_t::start);
    }
    bool start() {
        txn = slice->cache.begin_transaction(rwi_read, NULL);
        assert(txn);   // Read-only transactions begin right away
        buf_t *buf = txn->acquire(SUPERBLOCK_ID, rwi_read, this);
        if (buf) on_block_available(buf);
        return true;
    }
    void on_block_available(buf_t *buf) {
        btree_superblock_t *superblock = (btree_superblock_t *)buf->get_data_read();
        if (superblock->root_block != NULL_BLOCK_ID) {
            walk_branch(this, superblock->root_block);
        } else {
            done();
        }
        buf->release();
    }
    void done() {
        do_on_cpu(home_cpu, this, &slice_walker_t::report);
    }
    bool report() {
        parent->active_slice_walkers--;
        if (parent->active_slice_walkers == 0) {
            parent->done();
        }
        delete this;
        return true;
    }
};

void walk_slice(store_walker_t *parent, btree_slice_t *slice) {
    new slice_walker_t(parent, slice);
}

struct branch_walker_t :
    public block_available_callback_t,
    public large_buf_available_callback_t,
    public store_t::replicant_t::done_callback_t
{
    slice_walker_t *parent;
    buf_t *buf;
    
    int current_pair;   // Used for iterating over leaf nodes
    store_key_t *current_key;
    btree_value *current_value;
    large_buf_t *large_value;
    const_buffer_group_t buffers;
    
    branch_walker_t(slice_walker_t *parent, block_id_t block_id)
        : parent(parent)
    {
        parent->active_branch_walkers++;
        buf_t *node = parent->txn->acquire(block_id, rwi_read, this);
        if (node) on_block_available(node);
    }
    
    ~branch_walker_t() {
        buf->release();
        parent->active_branch_walkers--;
        if (parent->active_branch_walkers == 0) {
            parent->done();
        }
    }
    
    void on_block_available(buf_t *b) {
        buf = b;
        if (node_handler::is_internal(node_handler::node(buf->get_data_read()))) {
            const btree_internal_node *node = internal_node_handler::internal_node(buf->get_data_read());
            for (int i = 0; i < (int)node->npairs; i++) {
                btree_internal_pair *pair = internal_node_handler::get_pair(node, node->pair_offsets[i]);
                walk_branch(parent, pair->lnode);
            }
            delete this;
        } else {
            current_pair = -1;
            large_value = NULL;
            have_copied_value();
        }
    }
    
    void have_copied_value() {
    
        if (large_value) {
            large_value->release();
            delete large_value;
        }
        
        current_pair++;
        
        const btree_leaf_node *node = leaf_node_handler::leaf_node(buf->get_data_read());
        if (current_pair == node->npairs) {
            delete this;
        
        } else {
            btree_leaf_pair *pair = leaf_node_handler::get_pair(node, node->pair_offsets[current_pair]);
            current_key = &pair->key;
            current_value = pair->value();
            
            if (current_value->is_large()) {
                large_value = new large_buf_t(parent->txn);
                large_value->acquire(current_value->lb_ref(), rwi_read, this);
                
            } else {
                large_value = NULL;
                buffers.buffers.clear();
                buffers.add_buffer(current_value->value_size(), current_value->value());
                deliver_value();
            }
        }
    }
    
    void on_large_buf_available(large_buf_t *) {
        buffers.buffers.clear();
        for (int i = 0; i < large_value->get_num_segments(); i++) {
            uint16_t size;
            const void *data = large_value->get_segment(i, &size);
            buffers.add_buffer(size, data);
        }
        deliver_value();
    }
    
    void deliver_value() {
        parent->parent->callback->value(current_key, &buffers, this,
            current_value->mcflags(), current_value->exptime(),
            current_value->has_cas() ? current_value->cas() : 0);
    }
};

void walk_branch(slice_walker_t *parent, block_id_t node) {
    new branch_walker_t(parent, node);
}
