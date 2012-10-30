// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONTAINERS_TWO_LEVEL_ARRAY_HPP_
#define CONTAINERS_TWO_LEVEL_ARRAY_HPP_

#include "errors.hpp"

/* two_level_array_t is a tree that always has exactly two levels. Its computational complexity is
similar to that of an array, but it neither allocates all of its memory at once nor needs to
realloc() as it grows.

It is parameterized on a value type, 'value_t'. It makes the following assumptions about value_t:
1. value_t has a default constructor value_t() that has no side effects, and its destructor has no
    side effects.
2. value_t supports conversion to bool
3. bool(value_t()) == false
4. bool(any other instance of value_t) == true
Pointer types work well for value_t.

If a get() is called on an index in the array that set() has never been called for, the result will
be value_t().

It is also parameterized at compile-time on the maximum number of elements in the array and on the
size of the chunks to use.
*/

#define DEFAULT_TWO_LEVEL_ARRAY_CHUNK_SIZE (1 << 16)

template <class value_t, int max_size, int chunk_size = DEFAULT_TWO_LEVEL_ARRAY_CHUNK_SIZE>
class two_level_array_t {
public:
    typedef unsigned int key_t;

private:
    static const unsigned int num_chunks = max_size / chunk_size + 1;
    unsigned int count;

    struct chunk_t {
        chunk_t()
            : count(0), values()   // default-initialize each value in values
            { }
        unsigned int count;
        value_t values[chunk_size];
    };
    chunk_t **chunks;

    static unsigned int chunk_for_key(key_t key) {
        unsigned int chunk_id = key / chunk_size;
        rassert(chunk_id < num_chunks, "chunk_id < num_chunks: %u < %u", chunk_id, num_chunks);
        return chunk_id;
    }
    static unsigned int index_for_key(key_t key) {
        return key % chunk_size;
    }

public:
    two_level_array_t() : count(0), chunks(new chunk_t*[num_chunks]) {
        for (unsigned int i = 0; i < num_chunks; i++) {
            chunks[i] = NULL;
        }
    }
    ~two_level_array_t() {
        for (unsigned int i = 0; i < num_chunks; i++) {
            delete chunks[i];
        }
        delete[] chunks;
    }

    value_t& operator[](key_t key) {
        unsigned int chunk_id = chunk_for_key(key);
        if (chunks[chunk_id]) {
            return chunks[chunk_id]->values[index_for_key(key)];
        } else {
            chunk_t *chunk = chunks[chunk_id] = new chunk_t;
            return chunk->values[index_for_key(key)];
        }
    }

    value_t get(key_t key) const {
        unsigned int chunk_id = chunk_for_key(key);
        if (chunks[chunk_id]) {
            return chunks[chunk_id]->values[index_for_key(key)];
        } else {
            return value_t();
        }
    }

    void set(key_t key, value_t value) {
        unsigned int chunk_id = chunk_for_key(key);
        chunk_t *chunk = chunks[chunk_id];

        if (!value && !chunk) {
            /* If the user is inserting a zero value into an already-empty chunk, exit early so we
            don't create a new empty chunk */
            return;
        }

        if (!chunk) chunk = chunks[chunk_id] = new chunk_t;

        if (chunk->values[index_for_key(key)]) {
            --chunk->count;
            --count;
        }
        chunk->values[index_for_key(key)] = value;
        if (value) {
            ++chunk->count;
            ++count;
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

#endif // CONTAINERS_TWO_LEVEL_ARRAY_HPP_
