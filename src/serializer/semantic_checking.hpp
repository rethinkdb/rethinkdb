
#ifndef __SERIALIZER_SEMANTIC_CHECKING_HPP__
#define __SERIALIZER_SEMANTIC_CHECKING_HPP__

#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <boost/crc.hpp>
#include "arch/arch.hpp"
#include "config/args.hpp"
#include "serializer/serializer.hpp"
#include "containers/two_level_array.hpp"

/* This is a thin wrapper around the log serializer that makes sure that the
serializer has the correct semantics. It must have exactly the same API as
the log serializer. */

//#define SERIALIZER_DEBUG_PRINT 1

template<class inner_serializer_t>
class semantic_checking_serializer_t :
    public serializer_t,
    private inner_serializer_t::ready_callback_t,
    private inner_serializer_t::shutdown_callback_t
{
private:
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
        // We need to not crc BLOCK_META_DATA_SIZE because it's
        // internal to the serializer.
        crc_computer.process_bytes(buf, get_block_size());
        return crc_computer.checksum();
    }
    
    int last_write_started, last_write_callbacked;

private:
    struct persisted_block_info_t {
        ser_block_id_t block_id;
        block_info_t block_info;
    };
    int semantic_fd;

public:
    typedef typename inner_serializer_t::dynamic_config_t dynamic_config_t;
    typedef typename inner_serializer_t::static_config_t static_config_t;

public:    
    semantic_checking_serializer_t(const char *db_path, dynamic_config_t *config)
        : inner_serializer(db_path, config),
          last_write_started(0), last_write_callbacked(0),
          semantic_fd(-1)
        {
            std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > semantic_path(db_path);
            semantic_path += ".semantic";
            semantic_fd = open(semantic_path.c_str(), O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
            check("Could not open a semantic checking data file", semantic_fd == -1);
        }

    virtual ~semantic_checking_serializer_t() {
        close(semantic_fd);
    }

public:
    struct ready_callback_t {
        virtual void on_serializer_ready(semantic_checking_serializer_t *) = 0;
    };
    
    bool start_new(static_config_t *config, ready_callback_t *cb) {
        ready_callback = cb;
        return inner_serializer.start_new(config, this);
    }
        
    bool start_existing(ready_callback_t *cb) {
        // fill up the blocks from the semantic checking file
        int res = -1;
        do {
            persisted_block_info_t buf;
            res = read(semantic_fd, &buf, sizeof(buf));
            check("Could not read from the semantic checker file", res == -1);
            if(res == sizeof(persisted_block_info_t)) {
                blocks.set(buf.block_id, buf.block_info);
            }
        } while(res == sizeof(persisted_block_info_t));
        
        ready_callback = cb;
        return inner_serializer.start_existing(this);
    }
private:
    ready_callback_t *ready_callback;
    void on_serializer_ready(inner_serializer_t *ser) {
        if (ready_callback) ready_callback->on_serializer_ready(this);
    }

public:
    void *malloc() {
        return inner_serializer.malloc();
    }
    
    void free(void *ptr) {
        inner_serializer.free(ptr);
    }

public:
    /* For reads, we check to make sure that the data we get back in the read is
    consistent with what was last written there. */
    
    struct reader_t :
        public read_callback_t,
        public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, reader_t>
    {
        semantic_checking_serializer_t *parent;
        ser_block_id_t block_id;
        void *buf;
        block_info_t expected_block_state;
        read_callback_t *callback;
        
        reader_t(semantic_checking_serializer_t *parent, ser_block_id_t block_id, void *buf, block_info_t expected_block_state)
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
#ifdef SERIALIZER_DEBUG_PRINT
                    printf("Read %ld: %u\n", block_id, actual_crc);
#endif
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
    
    bool do_read(ser_block_id_t block_id, void *buf, read_callback_t *callback) {
#ifdef SERIALIZER_DEBUG_PRINT
        printf("Reading %ld\n", block_id);
#endif
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
    
    struct writer_t :
        public write_txn_callback_t,
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
    
    bool do_write(write_t *writes, int num_writes, write_txn_callback_t *callback) {
    
        for (int i = 0; i < num_writes; i++) {
            block_info_t b;
            bzero((void*)&b, sizeof(b));  // make valgrind happy
            if (writes[i].buf) {
                b.state = block_info_t::state_have_crc;
                b.crc = compute_crc(writes[i].buf);
#ifdef SERIALIZER_DEBUG_PRINT
                printf("Writing %ld: %u\n", writes[i].block_id, b.crc);
#endif
            } else {
                b.state = block_info_t::state_deleted;
#ifdef SERIALIZER_DEBUG_PRINT
                printf("Deleting %ld\n", writes[i].block_id);
#endif
            }
            blocks.set(writes[i].block_id, b);
            
            // Add the block to the semantic checker file
            persisted_block_info_t buf;
#ifdef VALGRIND
            bzero((void*)&buf, sizeof(buf));  // make valgrind happy
#endif
            buf.block_id = writes[i].block_id;
            buf.block_info = b;
            int res = write(semantic_fd, &buf, sizeof(buf));
            check("Could not write data to semantic checker file", res != sizeof(buf));
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
    size_t get_block_size() {
        return inner_serializer.get_block_size();
    }
    
    ser_block_id_t max_block_id() {
        return inner_serializer.max_block_id();
    }
    
    bool block_in_use(ser_block_id_t id) {
        bool in_use = inner_serializer.block_in_use(id);
        switch (blocks.get(id).state) {
            case block_info_t::state_unknown: break;
            case block_info_t::state_deleted: assert(!in_use); break;
            case block_info_t::state_have_crc: assert(in_use); break;
        }
        return in_use;
    }
    
public:
    struct shutdown_callback_t {
        virtual void on_serializer_shutdown(semantic_checking_serializer_t *) = 0;
    };
    bool shutdown(shutdown_callback_t *cb) {
        shutdown_callback = cb;
        return inner_serializer.shutdown(this);
    }
private:
    shutdown_callback_t *shutdown_callback;
    void on_serializer_shutdown(inner_serializer_t *ser) {
        if (shutdown_callback) shutdown_callback->on_serializer_shutdown(this);
    }

public:
    typedef typename inner_serializer_t::gc_disable_callback_t gc_disable_callback_t;
    bool disable_gc(gc_disable_callback_t *cb) {
        return inner_serializer.disable_gc(cb);
    }
    bool enable_gc() {
        return inner_serializer.enable_gc();
    }

};

#endif /* __SERIALIZER_SEMANTIC_CHECKING_HPP__ */
