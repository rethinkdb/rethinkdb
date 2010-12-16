
#ifndef __PROTOCOL_HPP__
#define __PROTOCOL_HPP__

#include <exception>
#include <string>

#include "distr.hpp"

class protocol_error_t : public std::exception, public std::string {
public:
    protocol_error_t(const std::string& message) : std::string(message) { }
    virtual ~protocol_error_t() throw () { }
};

struct protocol_t {
    protocol_t(config_t *config) : config(config) {}
    virtual ~protocol_t() {}
    virtual void connect(server_t *server) = 0;

    virtual void remove(const char *key, size_t key_size) = 0;
    virtual void update(const char *key, size_t key_size,
                        const char *value, size_t value_size) = 0;
    virtual void insert(const char *key, size_t key_size,
                        const char *value, size_t value_size) = 0;

    virtual void read(payload_t *keys, int count, payload_t *values = NULL) = 0;

    virtual void append(const char *key, size_t key_size,
                        const char *value, size_t value_size) = 0;

    virtual void prepend(const char *key, size_t key_size,
                          const char *value, size_t value_size) = 0;

    virtual void shared_init() {}

    config_t *config;

};

#endif // __PROTOCOL_HPP__

