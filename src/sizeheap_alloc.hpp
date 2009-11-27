
#ifndef __SIZEHEAP_ALLOC_HPP__
#define __SIZEHEAP_ALLOC_HPP__

#include <ext/hash_map>

// TODO: the hashmap lookup makes the allocator roughly three times
// slower than the parent allocator - we should do this statically.

template <class super_alloc_t>
struct sizeheap_alloc_t {

    ~sizeheap_alloc_t() {
        // Delete all parent allocators
        for(typename heap_map_t::iterator i = heaps.begin(); i != heaps.end(); i++) {
            delete (*i).second;
        }
    }

    void* malloc(size_t size) {
        size_t new_size = size + sizeof(void*);
        // Look up super alloc in hashtable
        typename heap_map_t::iterator parent_alloc = heaps.find(size);
        if(parent_alloc == heaps.end()) {
            // If doesn't exist, create and insert it
            super_alloc_t *alloc = new super_alloc_t(10, new_size);
            parent_alloc = heaps.insert(std::pair<size_t, super_alloc_t*>(size, alloc))
                .first;
        }
        
        // Allocate memory
        void *ptr = (*parent_alloc).second->malloc(new_size);
        if(ptr) {
            *(super_alloc_t**)ptr = (*parent_alloc).second;
            return (char*)ptr + sizeof(void*);
        } else {
            return NULL;
        }
    }

    void free(void *ptr) {
        // Look up the parent allocator
        char *_ptr = (char*)ptr - sizeof(void*);
        super_alloc_t *alloc = *(super_alloc_t**)_ptr;
        // Free the memory in the appropriate parent allocator
        alloc->free(_ptr);
    }

private:
    // TODO: replace with unordered_map
    typedef __gnu_cxx::hash_map<size_t, super_alloc_t*> heap_map_t;
    heap_map_t heaps;
};

#endif // __SIZEHEAP_ALLOC_HPP__

