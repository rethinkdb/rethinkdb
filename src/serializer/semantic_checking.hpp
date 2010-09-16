
#ifndef __SERIALIZER_SEMANTIC_CHECKING_HPP__
#define __SERIALIZER_SEMANTIC_CHECKING_HPP__

#include <boost/crc.hpp>
#include "config/args.hpp"

/* This is a thin wrapper around the log serializer that makes sure that the
serializer has the correct semantics. It must have exactly the same API as
the log serializer. */

template<class inner_serializer_t>
class semantic_checking_serializer_t
{
    inner_serializer_t inner_serializer;
    
    typedef uint32_t crc_t;
    
    struct block_info_t {
        
        enum state_t {
            state_unknown,
            state_deleted,
            state_have_crc
        } state;
        crc_t crc;
        
        // For compatibility with two_level_array_t
        block_info_t() : state(state_unknown) {}
        operator bool() { return state != state_unknown; }
    };
    
    two_level_array_t<block_info_t, MAX_BLOCK_ID> blocks;
    
    crc_t compute_crc(void *buf) {
        boost::crc_optimal<32, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true> crc_computer;
        crc_computer.process_bytes(buf, block_size);
        return crc_computer.checksum();
    }
    
    int last_write_started, last_write_callbacked;
    
public:
    size_t block_size;
    
    semantic_checking_serializer_t(char *db_path, size_t block_size)
        : inner_serializer(db_path, block_size),
          last_write_started(0), last_write_callbacked(0),
          block_size(block_size) {}

public:
    typedef typename inner_serializer_t::ready_callback_t ready_callback_t;
    bool start(ready_callback_t *ready_cb) {
        return inner_serializer.start(ready_cb);
    }
    
public:
    /* For reads, we check to make sure that the data we get back in the read is
    consistent with what was last written there. */
    
    struct read_callback_t {
        virtual void on_serializer_read() = 0;
    };
    
    struct reader_t :
        public inner_serializer_t::read_callback_t,
        public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, reader_t>
    {
        semantic_checking_serializer_t *parent;
        block_id_t block_id;
        void *buf;
        block_info_t expected_block_state;
        read_callback_t *callback;
        
        reader_t(semantic_checking_serializer_t *parent, block_id_t block_id, void *buf, block_info_t expected_block_state)
            : parent(parent), block_id(block_id), buf(buf), expected_block_state(expected_block_state) {}
        
        void on_serializer_read() {
            
            /* Make sure that the value that the serializer returned was valid */
            
            crc_t actual_crc = parent->compute_crc(buf);
            
            switch (expected_block_state.state) {
                
                case block_info_t::state_unknown: {
                    /* We don't know what this block was supposed to contain, so we can't do any
                    verification */
                    break;
                }
                
                case block_info_t::state_deleted: {
                    fail("Cache asked for a deleted block, and serializer didn't complain.");
                }
                
                case block_info_t::state_have_crc: {
                    if (expected_block_state.crc != actual_crc) {
                        fail("Serializer returned bad value for block ID %d", (int)block_id);
                    }
                    break;
                }
            }
            
            if (callback) callback->on_serializer_read();
            delete this;
        }
    };
    
    bool do_read(block_id_t block_id, void *buf, read_callback_t *callback) {
        reader_t *reader = new reader_t(this, block_id, buf, blocks.get(block_id));
        reader->callback = NULL;
        if (inner_serializer.do_read(block_id, buf, reader)) {
            reader->on_serializer_read();
            return true;
        } else {
            reader->callback = callback;
            return false;
        }
    }
    
public:
    /* For writes, we make sure that the writes come back in the same order that
    we send them to the serializer. */
    
    typedef typename inner_serializer_t::write_block_callback_t write_block_callback_t;
    
    struct write_txn_callback_t {
        virtual void on_serializer_write_txn() = 0;
    };
    
    struct writer_t :
        public inner_serializer_t::write_txn_callback_t,
        public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, writer_t>
    {
        semantic_checking_serializer_t *parent;
        int version;
        write_txn_callback_t *callback;
        
        writer_t(semantic_checking_serializer_t *parent, int version)
            : parent(parent), version(version) {}
            
        void on_serializer_write_txn() {
            if (parent->last_write_callbacked != version-1) {
                fail("Serializer completed writes in the wrong order.");
            }
            parent->last_write_callbacked = version;
            if (callback) callback->on_serializer_write_txn();
            delete this;
       }
    };
    
    typedef typename inner_serializer_t::write_t write_t;
    
    bool do_write(write_t *writes, int num_writes, write_txn_callback_t *callback) {
    
        for (int i = 0; i < num_writes; i++) {
            block_info_t b;
            if (writes[i].buf) {
                b.state = block_info_t::state_have_crc;
                b.crc = compute_crc(writes[i].buf);
            } else {
                b.state = block_info_t::state_deleted;
            }
            blocks.set(writes[i].block_id, b);
        }
        
        writer_t *writer = new writer_t(this, ++last_write_started);
        writer->callback = NULL;
        if (inner_serializer.do_write(writes, num_writes, writer)) {
            writer->on_serializer_write_txn();
            return true;
        } else {
            writer->callback = callback;
            return false;
        }
    }
    
public:
    block_id_t gen_block_id() {
        return inner_serializer.gen_block_id();
    }
    
public:
    typedef typename inner_serializer_t::shutdown_callback_t shutdown_callback_t;
    bool shutdown(shutdown_callback_t *cb) {
        return inner_serializer.shutdown(cb);
    }
};

#endif /* __SERIALIZER_SEMANTIC_CHECKING_HPP__ */
