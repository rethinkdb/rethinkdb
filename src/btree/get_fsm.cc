#include "get_fsm.hpp"
#include "utils.hpp"

#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"

#include "btree/delete_expired_fsm.hpp"

struct value_done_t : public store_t::get_callback_t::done_callback_t {
    coro_t *self;
    value_done_t() : self(coro_t::self()) { }
    void have_copied_value() { self->notify(); }
};

void co_value(store_t::get_callback_t *cb, const_buffer_group_t *value_buffers, mcflags_t flags, cas_t cas) {
    value_done_t done;
    cb->value(value_buffers, &done, flags, cas);
    coro_t::wait();
}

struct large_value_acquired_t : public large_buf_available_callback_t {
    coro_t *self;
    large_value_acquired_t() : self(coro_t::self()) { }
    void on_large_buf_available(large_buf_t *large_value) { self->notify(); }
};

void co_acquire_large_value(large_buf_t *large_value, large_buf_ref root_ref_, access_t access_) {
    large_value_acquired_t acquired;
    large_value->acquire(root_ref_, access_, &acquired);
    coro_t::wait();
}

struct co_block_available_callback_t : public block_available_callback_t {
    coro_t *self;
    buf_t *value;

    virtual void on_block_available(buf_t *block) {
        value = block;
        self->notify();
    }

    buf_t *join() {
        self = coro_t::self();
        coro_t::wait();
        return value;
    }
};

buf_t *co_acquire_transaction(transaction_t *transaction, block_id_t block_id, access_t mode) {
    co_block_available_callback_t cb;
    buf_t *value = transaction->acquire(block_id, mode, &cb);
    if (!value) value = cb.join();
    return value;
}

struct btree_get_t {
    union {
        char key_memory[MAX_KEY_SIZE+sizeof(btree_key)];
        btree_key key;
    };
    union {
        char value_memory[MAX_TOTAL_NODE_CONTENTS_SIZE+sizeof(btree_value)];
        btree_value value;
    };

    transaction_t *transaction;
    cache_t *cache;
    btree_key_value_store_t *store;
    btree_slice_t *slice;
    large_buf_t *large_value;
    const_buffer_group_t value_buffers;
    enum type_t { not_found, got_small_value, got_large_value, invalid_response } response;

    btree_get_t(btree_key *_key, btree_key_value_store_t *store)
        : transaction(NULL), cache(NULL), store(store), response(invalid_response) {
        keycpy(&key, _key);
        slice = store->slice_for_key(&key);
        cache = &slice->cache;
    }

    static void do_get(void *get) { ((btree_get_t*)get)->run(); }

    void run() {
        //Acquire the superblock
        buf_t *buf, *last_buf;
        transaction = cache->begin_transaction(rwi_read, NULL);
        assert(transaction); // Read-only transaction always begins immediately.
        last_buf = co_acquire_transaction(transaction, SUPERBLOCK_ID, rwi_read);
        assert(last_buf);
        block_id_t node_id = ptr_cast<btree_superblock_t>(last_buf->get_data_read())->root_block;
        assert(node_id != SUPERBLOCK_ID);

        //Acquire the root
        if (node_id == NULL_BLOCK_ID) {
            //No root exists
            last_buf->release();
            last_buf = NULL;
            // Commit transaction now because we won't be returning to this core
            bool committed __attribute__((unused)) = transaction->commit(NULL);
            assert(committed);   // Read-only transactions complete immediately
            response = not_found;
            return;
        }

        //Acquire the leaf node
        const node_t *node;
        while (true) {
            buf = co_acquire_transaction(transaction, node_id, rwi_read);
            assert(buf);
            node_handler::validate(cache->get_block_size(), ptr_cast<node_t>(buf->get_data_read()));
            
            // Release the previous buffer
            last_buf->release();
            last_buf = NULL;

            node = ptr_cast<node_t>(buf->get_data_read());
            if(!node_handler::is_internal(node)) break;

            block_id_t next_node_id = internal_node_handler::lookup(ptr_cast<internal_node_t>(node), &key);
            assert(next_node_id != NULL_BLOCK_ID);
            assert(next_node_id != SUPERBLOCK_ID);
            last_buf = buf;
            node_id = next_node_id;
        }

        //Got down to the leaf, now examine it
        bool found = leaf_node_handler::lookup(ptr_cast<leaf_node_t>(node), &key, &value);
        buf->release();
        buf = NULL;
        
        if (found && value.expired()) {
            delete_expired(&key, store);
            found = false;
        }
        
        /* The get_fsm has two paths it takes: one for large values and one for small ones. For large
        values, it holds onto the large value buffer while it goes back to the request handler's core
        and delivers the large value. Then it returns again to the cache's core and frees the value,
        and finally goes to the request handler's core again to free itself. For small values, it
        duplicates the value into an internal buffer and then goes to the request handler's core,
        delivers the value, and frees itself all in one trip. If the value is not found, we take
        a third path that is basically the same as the one for small values. */
        if (!found) {
            // Commit transaction now because we won't be returning to this core
            bool committed __attribute__((unused)) = transaction->commit(NULL);
            assert(committed);   // Read-only transactions complete immediately
            response = not_found;
        } else if (value.is_large()) {
            // Don't commit transaction yet because we need to keep holding onto
            // the large buf until it's been read.
            assert(value.is_large());

            large_value = new large_buf_t(transaction);

            co_acquire_large_value(large_value, value.lb_ref(), rwi_read);
            assert(large_value->state == large_buf_t::loaded);
            assert(large_value->get_root_ref().block_id == value.lb_ref().block_id);

            for (int i = 0; i < large_value->get_num_segments(); i++) {
                uint16_t size;
                const void *data = large_value->get_segment(i, &size);
                value_buffers.add_buffer(size, data);
            }
            response = got_large_value;
        } else {
            // Commit transaction now because we won't be returning to this core
            bool committed __attribute__((unused)) = transaction->commit(NULL);
            assert(committed);   // Read-only transactions complete immediately
            
            value_buffers.add_buffer(value.value_size(), value.value());
            response = got_small_value;
        }
    }

    void deliver(store_t::get_callback_t *cb) {
        switch (response) {
            case not_found:
                cb->not_found();
                return;
            case got_small_value:
            case got_large_value:
                co_value(cb, &value_buffers, value.mcflags(), 0);
                return;
            case invalid_response:
            default:
                crash_or_trap("Invalid response object\n");
        }
    }

    static void finalize(void *arg) {
        btree_get_t *receiver = ptr_cast<btree_get_t>(arg);
        receiver->large_value->release();
        delete receiver->large_value;
        bool committed __attribute__((unused)) = receiver->transaction->commit(NULL);
        assert(committed);   // Read-only transactions complete immediately
    }

    ~btree_get_t() {
        switch(response) {
            case got_large_value:
                run_on_cpu(slice->home_cpu, finalize, this);
                return;
            case got_small_value:
            case not_found:
                return;
            case invalid_response:
            default:
                crash_or_trap("Invalid response object\n");
        }
    }
};

//The rewritten btree get fsm, using coroutines
void _btree_get(btree_key *key, btree_key_value_store_t *store, store_t::get_callback_t *cb) {
    //This must be heap-allocated, otherwise GCC detects that strict aliasing rules
    //are being broken (which they still are in this case anyway)
    btree_get_t *computation = new btree_get_t(key, store);
    run_on_cpu(computation->slice->home_cpu, &btree_get_t::do_get, ptr_cast<void>(computation));
    computation->deliver(cb);
    delete computation;
}

void btree_get(btree_key *key, btree_key_value_store_t *store, store_t::get_callback_t *cb) {
    new coro_t(_btree_get, key, store, cb);
}
