
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
    ~request() {
        for (unsigned int i = 0; i < nstarted; i++) {
            delete msgs[i];
        }
    }
    
    unsigned int nstarted, ncompleted;

    // TODO: make this dynamic
<<<<<<< HEAD:src/request.hpp
    btree_fsm_t *fsms[MAX_OPS_IN_REQUEST];
    void *handler_data;

=======
    cpu_message_t *msgs[MAX_OPS_IN_REQUEST];
>>>>>>> 1eff101b16b1aad489977c7cad948e3d1f31ca86:src/request.hpp
    conn_fsm_t *netfsm;
};

#endif // __REQUEST_HPP__
