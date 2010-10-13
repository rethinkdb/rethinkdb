
#ifndef __BUFFER_CACHE_PAGE_MAP_ARRAY_HPP__
#define __BUFFER_CACHE_PAGE_MAP_ARRAY_HPP__

#include "containers/two_level_array.hpp"

template<class mc_config_t>
struct array_map_t {
    
    typedef mc_buf_t<mc_config_t> buf_t;
    
public:
    struct local_buf_t {
    
        explicit local_buf_t(buf_t *gbuf) : gbuf(gbuf) {
            assert(!gbuf->cache->page_map.array.get(gbuf->get_block_id()));
            gbuf->cache->page_map.array.set(gbuf->get_block_id(), gbuf);
        }
        
        ~local_buf_t() {
            assert(gbuf->cache->page_map.array.get(gbuf->get_block_id()));
            gbuf->cache->page_map.array.set(gbuf->get_block_id(), NULL);
        }
        
    private:
        buf_t *gbuf;
    };
    friend class local_buf_t;
    
    array_map_t() { }
    
    ~array_map_t() {
        assert(array.size() == 0);
    }

    buf_t* find(block_id_t block_id) {
        return array.get(block_id);
    }

private:
    two_level_array_t<buf_t*, MAX_BLOCK_ID> array;
};

#endif // __BUFFER_CACHE_PAGE_MAP_ARRAY_HPP__

