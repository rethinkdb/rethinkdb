#include "btree/coro_wrappers.hpp"
#include "concurrency/cond_var.hpp"

void co_deliver_get_result(const_buffer_group_t *bg, mcflags_t flags, cas_t cas,
        value_cond_t<store_t::get_result_t> *dest) {

    store_t::get_result_t res;
    res.buffer = bg;
    res.flags = flags;
    res.cas = cas;
    if (bg) {
        struct : public store_t::get_result_t::done_callback_t, public cond_t {
            void have_copied_value() { pulse(); }
        } cb;
        res.cb = &cb;
        dest->pulse(res);
        cb.wait();
    } else {
        res.cb = NULL;
        dest->pulse(res);
    }
}

// XXX
//void co_replicant_value(store_t::replicant_t *replicant, const store_key_t *key,
//                        const_buffer_group_t *value, mcflags_t flags,
//                        exptime_t exptime, cas_t cas, repli_timestamp timestamp) {
//    co_replicant_done_callback_t cb;
//    replicant->value(key, value, &cb, flags, exptime, cas, timestamp);
//    cb.join();
//}
