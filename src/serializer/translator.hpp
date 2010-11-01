
#ifndef __SERIALIZER_TRANSLATOR_HPP__
#define __SERIALIZER_TRANSLATOR_HPP__

#include "serializer/serializer.hpp"

/* The translator serializer is a wrapper around another serializer. It uses some subset
of the block IDs available on the inner serializer, but presents the illusion of a complete
serializer. It is used for splitting one serializer among several buffer caches on different
threads. */



class translator_serializer_t :
    public serializer_t
{
private:
    serializer_t *inner;
    int mod_count, mod_id;
    ser_block_id_t min;
    
public:
    // Translates a block id given the particular parameters.  This
    // needs to be exposed in some way because recovery tools depend
    // on it.
    static ser_block_id_t translate_block_id(ser_block_id_t id, int mod_count, int mod_id, ser_block_id_t min) {
        return id * mod_count + mod_id + min;
    }

    // "Inverts" translate_block_id, converting inner_id back to mod_id (not back to id).
    static ser_block_id_t untranslate_block_id(ser_block_id_t inner_id, int mod_count, ser_block_id_t min) {
        // We know that inner_id == id * mod_count + mod_id + min.
        // Thus inner_id - min == id * mod_count + mod_id.
        // It follows that inner_id - min === mod_id (modulo mod_count).
        // So (inner_id - min) % mod_count == mod_id (since 0 <= mod_id < mod_count).
        // (And inner_id - min >= 0, so '%' works as expected.)
        return (inner_id - min) % mod_count;
    }

private:
    ser_block_id_t xlate(ser_block_id_t id) {
        return translate_block_id(id, mod_count, mod_id, min);
    }
    
public:
    /* The translator serializer will only use block IDs on the inner serializer that
    are greater than or equal to 'min' and such that ((id - min) % mod_count) == mod_id. */ 
    translator_serializer_t(serializer_t *inner, int mod_count, int mod_id, ser_block_id_t min)
        : inner(inner), mod_count(mod_count), mod_id(mod_id), min(min)
    {
        assert(mod_count > 0);
        assert(mod_id >= 0);
        assert(mod_id < mod_count);
    }
    
    void *malloc() {
        return inner->malloc();
    }
    
    void free(void *ptr) {
        inner->free(ptr);
    }
    
    bool do_read(ser_block_id_t block_id, void *buf, read_callback_t *callback) {
        return inner->do_read(xlate(block_id), buf, callback);
    }
    
    bool do_write(write_t *writes, int num_writes, write_txn_callback_t *callback) {
        write_t writes2[num_writes];
        for (int i = 0; i < num_writes; i++) {
            writes2[i].block_id = xlate(writes[i].block_id);
            writes2[i].buf = writes[i].buf;
            writes2[i].callback = writes[i].callback;
        }
        return inner->do_write(writes2, num_writes, callback);
    }
    
    size_t get_block_size() {
        return inner->get_block_size();
    }
    
    ser_block_id_t max_block_id() {
        int64_t x = inner->max_block_id() - min;
        if (x <= 0) {
            x = 0;
        } else {
            while (x % mod_count != mod_id) x++;
            x /= mod_count;
        }
        assert(xlate(x) >= inner->max_block_id());
        return x;
    }
    
    bool block_in_use(ser_block_id_t id) {
        return inner->block_in_use(xlate(id));
    }
};

#endif /* __SERIALIZER_TRANSLATOR_HPP__ */
