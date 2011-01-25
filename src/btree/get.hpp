#ifndef __BTREE_GET_HPP__
#define __BTREE_GET_HPP__

#include "utils.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "buffer_cache/large_buf.hpp"
#include "btree/key_value_store.hpp"
#include "concurrency/cond_var.hpp"

store_t::get_result_t btree_get(const btree_key *key, btree_slice_t *slice);

/* value_data_provider_t is a data provider that reads from a btree_value and possibly also a
large_buf_t. If you pass it a cond_t, it will signal the cond_t when it's safe to release the
large_buf_t. */
struct value_data_provider_t :
    public auto_copying_data_provider_t
{
public:
    /* Interface used by the thing hooking the value_data_provider_t up with the btree.

    To use: Allocate a value_data_provider_t. Put a btree_value into its 'small_part' member. If
    it's a large value, put a large buf into its large_part member (and, if appropriate, put a
    cond_t in its to_signal_when_done member, so you know when the large_value can be safely
    released). Then pass it to the thing that will consume its value. */
    value_data_provider_t();
    ~value_data_provider_t();
    union {
        char value_memory[MAX_BTREE_VALUE_SIZE];
        btree_value small_part;
    };
    large_buf_t *large_part;
    threadsafe_cond_t *to_signal_when_done;

    /* Interface used by the thing consuming the value */
    size_t get_size() const;
    const const_buffer_group_t *get_data_as_buffers() throw (data_provider_failed_exc_t);

private:
    const_buffer_group_t bg;
};

#endif // __BTREE_GET_HPP__
