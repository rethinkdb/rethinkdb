
#ifndef __MEMCACHED_OPERATIONS_HPP__
#define __MEMCACHED_OPERATIONS_HPP__

#include "operations.hpp"
#include "config/code.hpp"

class memcached_operations_t : public operations_t {
public:
    typedef code_config_t::fsm_t fsm_t;
    typedef code_config_t::btree_t btree_t;
    
public:
    memcached_operations_t(btree_t *_btree)
        : btree(_btree)
        {}
    
    virtual result_t process_command(event_t *event);

private:
    btree_t *btree;
};

#endif // __MEMCACHED_OPERATIONS_HPP__

