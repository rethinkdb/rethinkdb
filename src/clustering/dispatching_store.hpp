#ifndef __CLUSTERING_DISPATCHING_STORE_HPP__
#define __CLUSTERING_DISPATCHING_STORE_HPP__

#include "store.hpp"

struct dispatching_store_t : public set_store_t {

    dispatching_store_t();
    ~dispatching_store_t();

    mutation_result_t change(const mutation_t &mut, castime_t castime);

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
