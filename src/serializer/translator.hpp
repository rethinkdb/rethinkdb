
#ifndef __SERIALIZER_TRANSLATOR_HPP__
#define __SERIALIZER_TRANSLATOR_HPP__

#include "serializer/serializer.hpp"
#include "buffer_cache/types.hpp"

/* The translator serializer is a wrapper around another serializer. It uses some subset
of the block IDs available on the inner serializer, but presents the illusion of a complete
serializer. It is used for splitting one serializer among several buffer caches on different
threads. */



class translator_serializer_t : public home_thread_mixin_t
{
private:
    serializer_t *inner;
    int mod_count, mod_id;
    config_block_id_t cfgid;

public:
    // Translates a block id given the particular parameters.  This
    // needs to be exposed in some way because recovery tools depend
    // on it.
    static ser_block_id_t translate_block_id(block_id_t id, int mod_count, int mod_id, config_block_id_t cfgid);

    // "Inverts" translate_block_id, converting inner_id back to mod_id (not back to id).
    static int untranslate_block_id(ser_block_id_t inner_id, int mod_count, config_block_id_t cfgid);

private:
    ser_block_id_t xlate(block_id_t id);
    
public:
    /* The translator serializer will only use block IDs on the inner serializer that
    are greater than or equal to 'min' and such that ((id - min) % mod_count) == mod_id. */ 
    translator_serializer_t(serializer_t *inner, int mod_count, int mod_id, config_block_id_t cfgid);
    
    void *malloc();
    virtual void *clone(void*);
    void free(void *ptr);

    bool do_read(block_id_t block_id, void *buf, serializer_t::read_callback_t *callback);
    struct write_t {
        block_id_t block_id;
        bool recency_specified;
        bool buf_specified;
        repli_timestamp recency;
        const void *buf;
        serializer_t::write_block_callback_t *callback;

        static write_t make_touch(block_id_t block_id_, repli_timestamp recency_, serializer_t::write_block_callback_t *callback_) {
            return write_t(block_id_, true, recency_, false, NULL, callback_);
        }

        static write_t make(block_id_t block_id_, repli_timestamp recency_, const void *buf_, serializer_t::write_block_callback_t *callback_) {
            return write_t(block_id_, true, recency_, true, buf_, callback_);
        }

    private:
        static write_t make_internal(block_id_t block_id_, const void *buf_, serializer_t::write_block_callback_t *callback_) {
            return write_t(block_id_, false, repli_timestamp::invalid, true, buf_, callback_);
        }

        write_t(block_id_t block_id_, bool recency_specified_, repli_timestamp recency_,
                bool buf_specified_, const void *buf_, serializer_t::write_block_callback_t *callback_)
            : block_id(block_id_), recency_specified(recency_specified_), buf_specified(buf_specified_), recency(recency_), buf(buf_), callback(callback_) { }
    };
    bool do_write(write_t *writes, int num_writes, serializer_t::write_txn_callback_t *callback);
    block_size_t get_block_size();

    // Returns the first never-used block id.  Every block with id
    // less than this has been created, and possibly deleted.  Every
    // block with id greater than or equal to this has never been
    // created.  As long as you don't skip ahead past max_block_id,
    // block id contiguity will be ensured.
    block_id_t max_block_id();
    bool block_in_use(block_id_t id);
    repli_timestamp get_recency(block_id_t id);
};

#endif /* __SERIALIZER_TRANSLATOR_HPP__ */
