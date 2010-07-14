
#ifndef __BUFFER_CACHE_UNLOCKED_HASH_MAP_HPP__
#define __BUFFER_CACHE_UNLOCKED_HASH_MAP_HPP__

#include <map>
#include "utils.hpp"

template <class config_t>
struct unlocked_hash_map_t
{
public:
    typedef typename config_t::serializer_t serializer_t;
    typedef typename config_t::buf_t buf_t;
    
public:
    buf_t* find(block_id_t block_id) {
        typename ft_map_t::iterator block = ft_map.find(block_id);
        if(block == ft_map.end()) {
            return NULL;
        } else {
            return (*block).second;
        }
    }
    
    void insert(buf_t *block) {
        ft_map[block->get_block_id()] = block;
    }
    
    void erase(buf_t *buf) {
        typename ft_map_t::iterator i = ft_map.find(buf->get_block_id());
        ft_map.erase(i);
    }
    
private:
    typedef std::map<block_id_t, buf_t*, std::less<block_id_t>,
                     gnew_alloc<std::pair<block_id_t, buf_t*> > > ft_map_t;
    ft_map_t ft_map;
};

#endif // __BUFFER_CACHE_UNLOCKED_HASH_MAP_HPP__

