
#ifndef __MEMCACHED_HANDLER_HPP__
#define __MEMCACHED_HANDLER_HPP__

#include "request_handler/request_handler.hpp"
#include "config/code.hpp"

class memcached_handler_t : public request_handler_t {
public:
    typedef code_config_t::alloc_t alloc_t;
    typedef code_config_t::cache_t cache_t;
    
public:
    memcached_handler_t(cache_t *_cache, alloc_t *_alloc)
        : cache(_cache), alloc(_alloc)
        {}
    
    virtual parse_result_t parse_request(event_t *event);
    virtual void build_response(fsm_t *fsm);

private:
    cache_t *cache;
    alloc_t *alloc;
};

#endif // __MEMCACHED_HANDLER_HPP__

