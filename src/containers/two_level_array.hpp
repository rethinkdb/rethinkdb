
#ifndef __TWO_LEVEL_ARRAY_HPP__
#define __TWO_LEVEL_ARRAY_HPP__

/* two_level_array_t is a tree that always has exactly two levels. Its computational complexity is
similar to that of an array, but it neither allocates all of its memory at once nor needs to
realloc() as it grows.

It is parameterized on a value_t; note that it actually stores "value_t*". It is also parameterized
at compile-time on the total number of elements in the array and on the size of the chunks to use.
*/

template <class value_t, int max_size, int chunk_size>
class two_level_array_t {

public:
    typedef unsigned int key_t;

private:
    static const unsigned int num_chunks = max_size / chunk_size + 1;
    unsigned int count;
    
    struct chunk_t : public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, chunk_t > {
        chunk_t() : count(0) {
            bzero(values, sizeof(value_t*)*chunk_size);
        }
        unsigned int count;
        value_t *values[chunk_size];
    };
    chunk_t *chunks[num_chunks];

    static unsigned int chunk_for_key(key_t key) {
        unsigned int chunk_id = key / chunk_size;
        assert(chunk_id < num_chunks);
        return chunk_id;
    }
    static unsigned int index_for_key(key_t key) {
        return key % chunk_size;
    }
    
public:
    two_level_array_t() : count(0) {
        bzero(chunks, sizeof(chunk_t *)*num_chunks);
    }
    ~two_level_array_t() {
        for (unsigned int i=0; i<num_chunks; i++) {
            if (chunks[i]) delete chunks[i];
        }
    }
    
    value_t *get(key_t key) const {
        unsigned int chunk_id = chunk_for_key(key);
        if (chunks[chunk_id]) {
            return chunks[chunk_id]->values[index_for_key(key)];
        } else {
            return NULL;
        }
    }
        
    void set(key_t key, value_t *value) {
        unsigned int chunk_id = chunk_for_key(key);
        chunk_t *chunk = chunks[chunk_id];
        
        if (!chunk) chunk = chunks[chunk_id] = new chunk_t;
        
        if (chunk->values[index_for_key(key)] != NULL) {
            chunk->count--;
            count--;
        }
        chunk->values[index_for_key(key)] = value;
        if (value != NULL) {
            chunk->count++;
            count++;
        }
        
        if (chunk->count == 0) {
            chunks[chunk_id] = NULL;
            delete chunk;
        }
    }
    
    unsigned int size() {
        return count;
    }
};

#endif // __TWO_LEVEL_ARRAY_HPP__
