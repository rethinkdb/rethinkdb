
#ifndef __BUFFER_CACHE_PAGE_MAP_ARRAY_HPP__
#define __BUFFER_CACHE_PAGE_MAP_ARRAY_HPP__

#include "utils.hpp"

#define BUFFERS_PER_CHUNK (1 << 16)

// Nobody will have more than 500 GB of memory. (Famous last words...)
#define MAX_SANE_MEMORY_SIZE (500L * (1 << 30))

#define MAX_CHUNKS (MAX_SANE_MEMORY_SIZE / BTREE_BLOCK_SIZE / BUFFERS_PER_CHUNK)

template <class config_t>
struct array_map_t {

public:
    typedef typename config_t::serializer_t serializer_t;
    typedef typename config_t::buf_t buf_t;
    
public:
    array_map_t() : count(0) {
        bzero(chunks, sizeof(chunk_t *)*MAX_CHUNKS);
    }
    ~array_map_t() {
        assert(count == 0);
        for (int i = 0; i<MAX_CHUNKS; i++) {
            assert(!chunks[i]);
        }
    }

    buf_t* find(block_id_t block_id) {
        unsigned int chunk_id = block_id / BUFFERS_PER_CHUNK;
        assert(chunk_id < MAX_CHUNKS);
        if (chunks[chunk_id]) {
            return chunks[chunk_id]->find(block_id);
        } else {
            return NULL;
        }
    }
    
    void insert(buf_t *block) {
        unsigned int chunk_id = block->get_block_id() / BUFFERS_PER_CHUNK;
        assert(chunk_id < MAX_CHUNKS);
        if (!chunks[chunk_id]) {
            chunks[chunk_id] = new chunk_t;
        }
        count ++;
        chunks[chunk_id]->insert(block);
    }
    
    void erase(buf_t *buf) {
        unsigned int chunk_id = buf->get_block_id() / BUFFERS_PER_CHUNK;
        assert(chunk_id < MAX_CHUNKS);
        assert(chunks[chunk_id]);
        count --;
        chunks[chunk_id]->erase(buf);
        if (chunks[chunk_id]->count == 0) {
            delete chunks[chunk_id];
            chunks[chunk_id] = NULL;
        }
    }

private:
    class chunk_t : public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, array_map_t<config_t> > {
    public:
        chunk_t() : count(0) {
            bzero(buffers, sizeof(buf_t *)*BUFFERS_PER_CHUNK);
        }
        int count;
        buf_t *buffers[BUFFERS_PER_CHUNK];
        buf_t *find(block_id_t block_id) {
            return buffers[block_id % BUFFERS_PER_CHUNK];
        }
        void insert(buf_t *block) {
            block_id_t block_id = block->get_block_id();
            assert(block);
            assert(!buffers[block_id % BUFFERS_PER_CHUNK]);
            count ++;
            buffers[block_id % BUFFERS_PER_CHUNK] = block;
        }
        void erase(buf_t *buf) {
            block_id_t block_id = buf->get_block_id();
            assert(count > 0);
            assert(buffers[block_id % BUFFERS_PER_CHUNK] == buf);
            count --;
            buffers[block_id % BUFFERS_PER_CHUNK] = NULL;
        }
    };
    
    int count;
    chunk_t *chunks[MAX_CHUNKS];
};

#endif // __BUFFER_CACHE_PAGE_MAP_ARRAY_HPP__

