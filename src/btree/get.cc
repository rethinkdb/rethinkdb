#include "btree/get.hpp"

#include "utils.hpp"
#include "btree/delete_expired.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "buffer_cache/co_functions.hpp"
#include "perfmon.hpp"
#include "store.hpp"
#include "concurrency/cond_var.hpp"

void co_btree_get(const btree_key *key, btree_slice_t *slice,
                  promise_t<store_t::get_result_t, threadsafe_cond_t> *res) {

    block_pm_duration get_time(&pm_cmd_get);

    /* In theory moving back might not be necessary, but not doing it causes problems right now. */
    on_thread_t mover(slice->home_thread);

    block_pm_duration get_time_2(&pm_cmd_get_without_threads);

    transactor_t transactor(&slice->cache, rwi_read);

    /* Construct a space to hold the result */
    boost::shared_ptr<value_data_provider_t> dp(new value_data_provider_t());

    // Acquire the superblock

    buf_lock_t buf_lock(transactor, SUPERBLOCK_ID, rwi_read);
    block_id_t node_id = ptr_cast<btree_superblock_t>(buf_lock.buf()->get_data_read())->root_block;
    rassert(node_id != SUPERBLOCK_ID);

    if (node_id == NULL_BLOCK_ID) {
        /* No root, so no keys in this entire shard */
        res->pulse(store_t::get_result_t());
        return;
    }

    // Acquire the root and work down the tree to the leaf node

    while (true) {
        {
            buf_lock_t tmp(transactor, node_id, rwi_read);
            buf_lock.swap(tmp);
        }

#ifndef NDEBUG
        node::validate(slice->cache.get_block_size(), ptr_cast<node_t>(buf_lock.buf()->get_data_read()));
#endif

        const node_t *node = ptr_cast<node_t>(buf_lock.buf()->get_data_read());
        if (!node::is_internal(node)) {
            break;
        }

        block_id_t next_node_id = internal_node::lookup(ptr_cast<internal_node_t>(node), key);
        rassert(next_node_id != NULL_BLOCK_ID);
        rassert(next_node_id != SUPERBLOCK_ID);

        node_id = next_node_id;
    }

    // Got down to the leaf, now examine it

    /* Load the value directly into the value_data_provider_t so we don't end up making an
    extra copy */
    btree_value &value = dp->small_part;
    bool found = leaf::lookup(ptr_cast<leaf_node_t>(buf_lock.buf()->get_data_read()), key, &value);
    buf_lock.release();

    if (found && value.expired()) {
        btree_delete_expired(key, slice);
        found = false;
    }

    if (!found) {
        /* Key not found. */
        res->pulse(store_t::get_result_t());
        return;
    }

    /* If we have a large value, we will hold on to it until the caller is done with it to avoid
    making a copy. If we have a small value, we just make the copy. That's why this has two paths.
    */

    if (value.is_large()) {
        /* Acquire large value */
        large_buf_t *large_value = new large_buf_t(transactor.transaction());
        co_acquire_large_value(large_value, value.lb_ref(), rwi_read);
        rassert(large_value->state == large_buf_t::loaded);
        rassert(large_value->get_root_ref().block_id == value.lb_ref().block_id);

        /* Pass value to caller */
        dp->large_part = large_value;
        threadsafe_cond_t to_signal_when_done;
        dp->to_signal_when_done = &to_signal_when_done;
        res->pulse(store_t::get_result_t(dp, value.mcflags(), 0));
        dp.reset();   // So that the destructor can eventually get called
        to_signal_when_done.wait();   // Wait until caller is finished

        /* Clean up large value */
        large_value->release();
        delete large_value;

    } else {
        /* Simple small-value delivery */
        res->pulse(store_t::get_result_t(dp, value.mcflags(), 0));
    }
}

store_t::get_result_t btree_get(const btree_key *key, btree_slice_t *slice) {
    promise_t<store_t::get_result_t, threadsafe_cond_t> res;
    coro_t::spawn(boost::bind(co_btree_get, key, slice, &res));
    return res.wait();
}

/* value_data_provider_t */

value_data_provider_t::value_data_provider_t() :
    large_part(NULL), to_signal_when_done(NULL)
{ }

value_data_provider_t::~value_data_provider_t() {
    if (to_signal_when_done) to_signal_when_done->pulse();
}

size_t value_data_provider_t::get_size() const {
    return small_part.value_size();
}

const const_buffer_group_t *value_data_provider_t::get_data_as_buffers() throw (data_provider_failed_exc_t) {
    rassert(bg.buffers.size() == 0);   // This should be our first time here
    if (small_part.is_large()) {
        for (int i = 0; i < large_part->get_num_segments(); i++) {
            uint16_t size;
            const void *data = large_part->get_segment(i, &size);
            bg.add_buffer(size, data);
        }
    } else {
        bg.add_buffer(small_part.value_size(), small_part.value());
    }
    return &bg;
}

