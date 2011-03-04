#ifndef __CLUSTERING_DISPATCHING_STORE_HPP__
#define __CLUSTERING_DISPATCHING_STORE_HPP__

#include "store.hpp"

struct dispatching_store_t : public set_store_t {

    dispatching_store_t();
    ~dispatching_store_t();

    get_result_t get_cas(const store_key_t &key, castime_t castime);

    set_result_t sarc(const store_key_t &key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas);
    delete_result_t delete_key(const store_key_t &key, repli_timestamp timestamp);

    incr_decr_result_t incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount, castime_t castime);
    append_prepend_result_t append_prepend(append_prepend_kind_t kind, const store_key_t &key, data_provider_t *data, castime_t castime);

    struct dispatchee_t : public intrusive_list_node_t<dispatchee_t> {
    public:
        dispatchee_t(dispatching_store_t*, set_store_t*);
        ~dispatchee_t();
        set_store_t *client;
    private:
        dispatching_store_t *parent;
        DISABLE_COPYING(dispatchee_t);
    };

private:
    friend class dispatchee_t;
    intrusive_list_t<dispatchee_t> dispatchees;
    DISABLE_COPYING(dispatching_store_t);
};

#endif /* __CLUSTERING_DISPATCHING_STORE_HPP__ */
