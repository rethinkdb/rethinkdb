
#ifndef __MEMCACHED_OPERATIONS_HPP__
#define __MEMCACHED_OPERATIONS_HPP__

#include "operations.hpp"
#include "common.hpp"

class memcached_operations_t : public operations_t {
public:
    memcached_operations_t(rethink_tree_t *_btree)
        : btree(_btree)
        {}
    
    virtual result_t process_command(event_t *event);

private:
    rethink_tree_t *btree;
};

#endif // __MEMCACHED_OPERATIONS_HPP__

