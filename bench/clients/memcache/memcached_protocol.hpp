
#ifndef __MEMCACHED_PROTOCOL_HPP__
#define __MEMCACHED_PROTOCOL_HPP__

#include <libmemcached/memcached.h>
#include <stdlib.h>
#include <assert.h>
#include "protocol.hpp"

struct memcached_protocol_t {
    memcached_protocol_t() {
        memcached_create(&memcached);
    }

    virtual ~memcached_protocol_t() {
        // Disconnect
        memcached_free(&memcached);
    }
    
    virtual void connect(const char *host, int port) {
        memcached_server_add(&memcached, host, port);
    }
    
    virtual void remove(const char *key, size_t key_size) {
        memcached_return_t _error = memcached_delete(&memcached, key, key_size, 0);
        if(_error != MEMCACHED_SUCCESS) {
            assert(0);
            fprintf(stderr, "Error performing delete operation (%d)\n", _error);
            exit(-1);
        }
    }
    
    virtual void update(const char *key, size_t key_size,
                        const char *value, size_t value_size)
    {
        memcached_return_t _error = memcached_set(&memcached, key, key_size,
                                                  value, value_size, 0, 0);
        if(_error != MEMCACHED_SUCCESS) {
            assert(0);
            fprintf(stderr, "Error performing delete operation (%d)\n", _error);
            exit(-1);
        }
    }
    
    virtual void insert(const char *key, size_t key_size,
                        const char *value, size_t value_size)
    {
        update(key, key_size, value, value_size);
    }
    
    virtual void read(const char *key, size_t key_size) {
        char *_value;
        size_t _value_length;
        uint32_t _flags;
        memcached_return_t _error;
        _value = memcached_get(&memcached, key, key_size,
                               &_value_length, &_flags, &_error);
        if(_error != MEMCACHED_SUCCESS) {
            assert(0);
            fprintf(stderr, "Error performing read operation (%d)\n", _error);
            exit(-1);
        }
        free(_value);
    }

private:
    memcached_st memcached;
};


#endif // __MEMCACHED_PROTOCOL_HPP__

