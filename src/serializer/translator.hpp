
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
    static ser_block_id_t translate_block_id(ser_block_id_t id, int mod_count, int mod_id, ser_block_id_t min);

    // "Inverts" translate_block_id, converting inner_id back to mod_id (not back to id).
    static ser_block_id_t untranslate_block_id(ser_block_id_t inner_id, int mod_count, ser_block_id_t min);

private:
    ser_block_id_t xlate(ser_block_id_t id);
    
public:
    /* The translator serializer will only use block IDs on the inner serializer that
    are greater than or equal to 'min' and such that ((id - min) % mod_count) == mod_id. */ 
    translator_serializer_t(serializer_t *inner, int mod_count, int mod_id, ser_block_id_t min);
    
    void *malloc();
    void free(void *ptr);
    bool do_read(ser_block_id_t block_id, void *buf, read_callback_t *callback);
    bool do_write(write_t *writes, int num_writes, write_txn_callback_t *callback);
    size_t get_block_size();
    ser_block_id_t max_block_id();
    bool block_in_use(ser_block_id_t id);
};

#endif /* __SERIALIZER_TRANSLATOR_HPP__ */
