
#ifndef __FALLTHROUGH_CACHE_HPP__
#define __FALLTHROUGH_CACHE_HPP__

#include <map>
#include "utils.hpp"

/* Every operation in this cache fall through to the serializer - no
 * attempt is made to cache anything in memory. This is useful for
 * testing of serializers as well as consumers of caches without
 * depending on complex internal cache behavior. */
template <class config_t>
struct fallthrough_cache_t : public config_t::serializer_t {
public:
    typedef typename config_t::serializer_t serializer_t;
    typedef typename serializer_t::block_id_t block_id_t;
    typedef typename config_t::btree_fsm_t btree_fsm_t;

public:
    fallthrough_cache_t(size_t _block_size) : serializer_t(_block_size) {}

    void* allocate(block_id_t *block_id) {
        *block_id = serializer_t::gen_block_id();
        void *block = malloc_aligned(serializer_t::block_size, serializer_t::block_size);
        ft_map[*block_id] = block;
        return block;
    }
    
    void* acquire(block_id_t block_id, btree_fsm_t *state) {
        typename ft_map_t::iterator block = ft_map.find(block_id);
        if(block == ft_map.end()) {
            void *buf = malloc_aligned(serializer_t::block_size, serializer_t::block_size);
            do_read(block_id, buf, state);
            return NULL;
        } else {
            return (*block).second;
        }
    }

    block_id_t release(block_id_t block_id, void *block, bool dirty, btree_fsm_t *state) {
        if(dirty) {
            return do_write(block_id, (char*)block, state);
        } else {
            ft_map.erase(ft_map.find(block_id));
            free(block);
        }
        return block_id;
    }

    void aio_complete(block_id_t block_id, void *block, bool written) {
        if(written) {
            typename ft_map_t::iterator i = ft_map.find(block_id);
            check("aio_complete: Attempting to erase a non-existing record", i == ft_map.end());
            ft_map.erase(i);
            free(block);
        } else {
            ft_map[block_id] = block;
        }
    }

private:
    typedef std::map<block_id_t, void*> ft_map_t;
    ft_map_t ft_map;
};

#endif // __FALLTHROUGH_CACHE_HPP__

