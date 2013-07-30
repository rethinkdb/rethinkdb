// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONTAINERS_TWO_LEVEL_ARRAY_HPP_
#define CONTAINERS_TWO_LEVEL_ARRAY_HPP_

#include <vector>

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

template <class value_t>
class two_level_array_t {
private:
    static const size_t CHUNK_SIZE = 1 << 16;

    struct chunk_t {
        chunk_t()
            : count(0), values()   // default-initialize each value in values
            { }
        size_t count;
        value_t values[CHUNK_SIZE];
    };
    std::vector<chunk_t *> chunks;

    static size_t chunk_for_key(key_t key) {
        size_t chunk_id = key / CHUNK_SIZE;
        return chunk_id;
    }
    static size_t index_for_key(key_t key) {
        return key % CHUNK_SIZE;
    }

public:
    two_level_array_t() { }
    ~two_level_array_t() {
        for (auto it = chunks.begin(); it != chunks.end(); ++it) {
            delete *it;
        }
    }

    value_t get(size_t key) const {
        size_t chunk_id = chunk_for_key(key);
        if (chunk_id < chunks.size() && chunks[chunk_id]) {
            return chunks[chunk_id]->values[index_for_key(key)];
        } else {
            return value_t();
        }
    }

    void set(size_t key, value_t value) {
        const size_t chunk_id = chunk_for_key(key);
        if (chunk_id >= chunks.size() || chunks[chunk_id] == NULL) {
            if (!value) {
                return;
            } else {
                if (chunk_id >= chunks.size()) {
                    chunks.resize(chunk_id + 1, NULL);
                }
                chunks[chunk_id] = new chunk_t;
            }
        }

        chunk_t *chunk = chunks[chunk_id];
        const size_t index = index_for_key(key);
        if (chunk->values[index]) {
            --chunk->count;
        }
        chunk->values[index] = value;
        if (value) {
            ++chunk->count;
        }

        if (chunk->count == 0) {
            chunks[chunk_id] = NULL;
            delete chunk;

            while (!chunks.empty() && chunks.back() == NULL) {
                chunks.pop_back();
            }
        }
    }
};


template <class value_t>
class two_level_nevershrink_array_t {
private:
    static const size_t CHUNK_SIZE = 1 << 16;

    struct chunk_t {
        chunk_t()
            : values()   // default-initialize each value in values
            { }
        value_t values[CHUNK_SIZE];
    };
    std::vector<chunk_t *> chunks;

    static size_t chunk_for_key(key_t key) {
        size_t chunk_id = key / CHUNK_SIZE;
        return chunk_id;
    }
    static size_t index_for_key(key_t key) {
        return key % CHUNK_SIZE;
    }

public:
    two_level_nevershrink_array_t() { }
    ~two_level_nevershrink_array_t() {
        for (auto it = chunks.begin(); it != chunks.end(); ++it) {
            delete *it;
        }
    }

    value_t &operator[](size_t key) {
        const size_t chunk_id = chunk_for_key(key);
        if (chunk_id > chunks.size()) {
            chunks.resize(chunk_id + 1, NULL);
        }

        if (chunks[chunk_id] == NULL) {
            chunks[chunk_id] = new chunk_t;
        }

        return chunks[chunk_id]->values[index_for_key(key)];
    }
};


#endif // CONTAINERS_TWO_LEVEL_ARRAY_HPP_
