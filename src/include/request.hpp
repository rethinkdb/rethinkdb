
#ifndef __REQUEST_HPP__
#define __REQUEST_HPP__

#include "config/args.hpp"

template <class config_t>
struct request : public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, request<config_t> >
{
public:
    typedef typename config_t::btree_fsm_t btree_fsm_t;
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    
public:
    explicit request(conn_fsm_t *_netfsm) :
        nstarted(0), ncompleted(0), netfsm(_netfsm) {}
    unsigned int nstarted, ncompleted;

    // TODO: make this dynamic
    btree_fsm_t *fsms[MAX_OPS_IN_REQUEST];

    conn_fsm_t *netfsm;
};

#endif // __REQUEST_HPP__

