
#ifndef __MEMCACHED_OPERATIONS_HPP__
#define __MEMCACHED_OPERATIONS_HPP__

#include "operations.hpp"
#include "config/code.hpp"

class memcached_operations_t : public operations_t {
public:
    typedef code_config_t::alloc_t alloc_t;
    typedef code_config_t::cache_t cache_t;
    
public:
    memcached_operations_t(cache_t *_cache, alloc_t *_alloc)
        : cache(_cache), alloc(_alloc)
        {}
    
    virtual result_t process_command(event_t *event);
    virtual void complete_op(btree_fsm_t *btree_fsm, event_t *event);

private:
    cache_t *cache;
    alloc_t *alloc;
};

#endif // __MEMCACHED_OPERATIONS_HPP__

