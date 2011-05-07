#ifndef __CLUSTERING_DISPATCHING_STORE_HPP__
#define __CLUSTERING_DISPATCHING_STORE_HPP__

#include "store.hpp"
#include "clustering/master_map.hpp"

struct clustered_store_t : public set_store_t, public get_store_t {

    clustered_store_t();
    ~clustered_store_t();

    mutation_result_t change(const mutation_t &mut, castime_t castime);

    get_result_t get(const store_key_t &key);

    rget_result_t rget(UNUSED rget_bound_mode_t left_mode, const UNUSED store_key_t &left_key,
        UNUSED rget_bound_mode_t right_mode, const UNUSED store_key_t &right_key) { 
        not_implemented(); 
        rget_result_t res;
        return res;
    }

    struct dispatchee_t {
    public:
        dispatchee_t(int bucket, clustered_store_t*, std::pair<set_store_t*, get_store_t*>);
        ~dispatchee_t();
        std::pair<set_store_t *, get_store_t *> client;
    private:
        clustered_store_t *parent;
        int bucket;
        DISABLE_COPYING(dispatchee_t);
    };

private:
    friend class dispatchee_t;
    storage_map_t dispatchees;
    DISABLE_COPYING(clustered_store_t);
};

#endif /* __CLUSTERING_DISPATCHING_STORE_HPP__ */
