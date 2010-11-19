
#ifndef __BUFFER_CACHE_PAGE_MAP_ARRAY_HPP__
#define __BUFFER_CACHE_PAGE_MAP_ARRAY_HPP__

#include "containers/two_level_array.hpp"
#include "config/args.hpp"
#include "buffer_cache/types.hpp"

struct mc_inner_buf_t;

struct array_map_t {
    
    typedef mc_inner_buf_t inner_buf_t;
    
public:
    struct local_buf_t {
        explicit local_buf_t(inner_buf_t *gbuf);
        ~local_buf_t();
    private:
        inner_buf_t *gbuf;
    };
    friend class local_buf_t;
    
    array_map_t() { }
    
    ~array_map_t() {
        assert(array.size() == 0);
    }

    inner_buf_t* find(block_id_t block_id) {
        return array.get(block_id);
    }

private:
    two_level_array_t<inner_buf_t*, MAX_BLOCK_ID> array;
};

#endif // __BUFFER_CACHE_PAGE_MAP_ARRAY_HPP__

