
#ifndef __PROTOCOL_HPP__
#define __PROTOCOL_HPP__

#include "distr.hpp"

struct protocol_t {
    virtual ~protocol_t() {}
    virtual void connect(const char *host, int port) = 0;
    
    virtual void remove(const char *key, size_t key_size) = 0;
    virtual void update(const char *key, size_t key_size,
                        const char *value, size_t value_size) = 0;
    virtual void insert(const char *key, size_t key_size,
                        const char *value, size_t value_size) = 0;

    virtual void read(payload_t *keys, int count) = 0;
    
};

#endif // __PROTOCOL_HPP__

