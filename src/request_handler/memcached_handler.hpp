
#ifndef __MEMCACHED_HANDLER_HPP__
#define __MEMCACHED_HANDLER_HPP__

#include "request_handler/request_handler.hpp"
#include "config/code.hpp"
#include "alloc/alloc_mixin.hpp"

/*! memcached_handler_t
 *  \brief Implements a wrapper for bin_memcached_handler_t and txt_memcached_handler_t
 *         which parses incoming packets to determine which type they are
 */
template<class config_t>
class memcached_handler_t : public request_handler_t<config_t>,
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, memcached_handler_t<config_t> > {
public:
    typedef typename config_t::cache_t cache_t;
    typedef typename config_t::conn_fsm_t conn_fsm_t;
    typedef typename config_t::request_t request_t;
    typedef typename config_t::btree_fsm_t btree_fsm_t;
    typedef typename config_t::btree_set_fsm_t btree_set_fsm_t;
    typedef typename config_t::btree_get_fsm_t btree_get_fsm_t;
    typedef typename config_t::btree_delete_fsm_t btree_delete_fsm_t;
    typedef typename config_t::req_handler_t req_handler_t;
    typedef typename req_handler_t::parse_result_t parse_result_t;
    typedef typename config_t::t_memcached_handler_t txt_memcached_handler_t;
    typedef typename config_t::b_memcached_handler_t bin_memcached_handler_t;
    typedef typename config_t::b_memcached_handler_t::bin_magic_t bin_magic_t;
    
public:
    memcached_handler_t(cache_t *_cache, event_queue_t *eq)
        : req_handler_t(eq), cache(_cache), memcached_protocol(memcached_unknown_protocol), req_handler(NULL)
        {}
    
    virtual parse_result_t parse_request(event_t *event);
    virtual void build_response(request_t *request);

private:
    cache_t *cache;
    typedef enum {
        memcached_unknown_protocol = -1,
        memcached_txt_protocol,
        memcached_bin_protocol,
        num_memcached_protocol
    } memcached_protocol_t;

    //! memcached_protocol: which protocol we've decided we're using (if we've decided)
    memcached_protocol_t memcached_protocol;

    //! the correct handler for the packet (or NULL if we haven't decided)
    req_handler_t *req_handler;

    //! determine which protocol the packet is using
    void determine_protocol(const event_t *event);
};

#include "request_handler/memcached_handler.tcc"

#endif // __MEMCACHED_HANDLER_HPP__
