#include "btree/coro_wrappers.hpp"

bool co_get_data_provider_value(data_provider_t *data, buffer_group_t *dest) {
    co_data_provider_done_callback_t cb;
    data->get_value(dest, &cb);
    return cb.join();
}

void co_value(store_t::get_callback_t *cb, const_buffer_group_t *value_buffers, mcflags_t flags, cas_t cas) {
    value_done_t done;
    cb->value(value_buffers, &done, flags, cas);
    coro_t::wait();
}

// XXX
//void co_replicant_value(store_t::replicant_t *replicant, const store_key_t *key,
//                        const_buffer_group_t *value, mcflags_t flags,
//                        exptime_t exptime, cas_t cas, repli_timestamp timestamp) {
//    co_replicant_done_callback_t cb;
//    replicant->value(key, value, &cb, flags, exptime, cas, timestamp);
//    cb.join();
//}
