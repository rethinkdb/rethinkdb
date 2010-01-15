
#ifndef __FALLTHROUGH_CACHE_HPP__
#define __FALLTHROUGH_CACHE_HPP__

/* Every operation in this cache fall through to the serializer - no
 * attempt is made to cache anything in memory. This is useful for
 * testing of serializers as well as consumers of caches without
 * depending on complex internal cache behavior. */
template <class serializer_t>
struct fallthrough_cache_t : public serializer_t {
public:
    typedef typename serializer_t::block_id_t block_id_t;

public:
    fallthrough_cache_t(size_t _block_size) : block_size(_block_size) {}

    void* allocate(block_id_t *block_id) {
        *block_id = malloc_aligned(block_size, block_size);
        return *block_id;
    }
    
    void* acquire(block_id_t block_id, void *state) {
        void *buf = malloc_aligned(block_size, block_size);
        do_read(block_id, buf, state);
        return NULL;
    }

    block_id_t release(block_id_t block_id, void *block, bool dirty, void *state) {
        if(dirty) {
            // TODO: we need to free the block after do_write completes
            return do_write(block_id, buf, state);
        } else {
            free(block);
        }
        return block_id;
    }

private:
    size_t block_size;
};

#endif // __FALLTHROUGH_CACHE_HPP__

