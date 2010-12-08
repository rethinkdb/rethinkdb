#ifndef __STORE_HPP__
#define __STORE_HPP__

#include "utils.hpp"

typedef uint32_t mcflags_t;
typedef uint32_t exptime_t;
typedef uint64_t cas_t;

// Note: Changing this struct changes the format of the data stored on disk.
// If you change this struct, previous stored data will be misinterpreted.
struct store_key_t {
    uint8_t size;
    char contents[0];
    void print() const {
        printf("%*.*s", size, size, contents);
    }
};

struct buffer_group_t {
    struct buffer_t {
        ssize_t size;
        void *data;
    };
    std::vector<buffer_t> buffers;
    void add_buffer(size_t s, void *d) {
        buffer_t b;
        b.size = s;
        b.data = d;
        buffers.push_back(b);
    }
    size_t get_size() {
        size_t s = 0;
        for (int i = 0; i < (int)buffers.size(); i++) {
            s += buffers[i].size;
        }
        return s;
    }
};

struct const_buffer_group_t {
    struct buffer_t {
        ssize_t size;
        const void *data;
    };
    std::vector<buffer_t> buffers;
    void add_buffer(size_t s, const void *d) {
        buffer_t b;
        b.size = s;
        b.data = d;
        buffers.push_back(b);
    }
    size_t get_size() {
        size_t s = 0;
        for (int i = 0; i < (int)buffers.size(); i++) {
            s += buffers[i].size;
        }
        return s;
    }
};

struct data_provider_t {
    
    virtual ~data_provider_t() { }
    virtual size_t get_size() = 0;
    struct done_callback_t {
        virtual void have_provided_value() = 0;
        virtual void have_failed() = 0;
        virtual ~done_callback_t() {}
    };
    virtual void get_value(buffer_group_t *dest, done_callback_t *cb) = 0;
};

struct store_t {
    
    /* To get a value from the store, call get() or get_cas(), providing the key you want to get
    and a get_callback_t. If the store finds the value, it will call value(), giving you a
    buffer_group_t holding the value's contents. You must call the provided done_callback_t when
    you are done copying the value out of the buffer_group_t. If you do not call get_cas(), then
    the parameter 'cas' will be undefined and should be ignored. Note that the callback (value() or
    not_found() may be called on any core, so you are responsible for switching to the request
    handler core. */
    
    struct get_callback_t {
    
        struct done_callback_t {
            virtual void have_copied_value() = 0;
            virtual ~done_callback_t() {}
        };
        virtual void value(const_buffer_group_t *value, done_callback_t *cb, mcflags_t flags, cas_t cas) = 0;
        virtual void not_found() = 0;
        virtual ~get_callback_t() {}
    };
    virtual void get(store_key_t *key, get_callback_t *cb) = 0;
    virtual void get_cas(store_key_t *key, get_callback_t *cb) = 0;
    
    /* To set a value in the database, call set(), add(), or replace(). Provide a key* for the key
    to be set and a data_provider_t* for the data. Note that the data_provider_t may be called on
    any core, so you must implement core-switching yourself if necessary. The data_provider_t will
    always be called exactly once. Also provide a set_callback_t. */
    
    struct set_callback_t {
        
        virtual void stored() = 0;
        
        /* Called if you add() and it already exists or you replace() and it doesn't */
        virtual void not_stored() = 0;
        
        /* Called if you cas() and the key does not exist. */
        virtual void not_found() = 0;
        
        /* Called if you cas() and the key was modified since get_cas(). */
        virtual void exists() = 0;
        
        /* Response if the value to be stored is too big */
        virtual void too_large() = 0;
        
        /* Called if the data_provider_t that you gave returned have_failed(). */
        virtual void data_provider_failed() = 0;

        virtual ~set_callback_t() {}
    };
    virtual void set(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, set_callback_t *cb) = 0;
    virtual void add(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, set_callback_t *cb) = 0;
    virtual void replace(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, set_callback_t *cb) = 0;
    virtual void cas(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique, set_callback_t *cb) = 0;
    
    /* To increment or decrement a value, use incr() or decr(). They're pretty straight-forward. */
    
    struct incr_decr_callback_t {
        
        virtual void success(unsigned long long new_value) = 0;
        virtual void not_found() = 0;
        virtual void not_numeric() = 0;

        virtual ~incr_decr_callback_t() {}
    };
    virtual void incr(store_key_t *key, unsigned long long amount, incr_decr_callback_t *cb) = 0;
    virtual void decr(store_key_t *key, unsigned long long amount, incr_decr_callback_t *cb) = 0;
    
    /* To append or prepend a value, use append() or prepend(). */
    
    struct append_prepend_callback_t {
        
        virtual void success() = 0;
        virtual void too_large() = 0;
        virtual void not_found() = 0;
        virtual void data_provider_failed() = 0;

        virtual ~append_prepend_callback_t() {}
    };
    virtual void append(store_key_t *key, data_provider_t *data, append_prepend_callback_t *cb) = 0;
    virtual void prepend(store_key_t *key, data_provider_t *data, append_prepend_callback_t *cb) = 0;
    
    /* To delete a key-value pair, use delete(). */
    
    struct delete_callback_t {
        
        virtual void deleted() = 0;
        virtual void not_found() = 0;

        virtual ~delete_callback_t() {}
    };
    virtual void delete_key(store_key_t *key, delete_callback_t *cb) = 0;
    
    /* To start replicating, call replicate() with a replicant_t.
    
    The replicant_t's value() method will be called for every key in the database at the time that
    replicate() is called and for every change that is made after replicate() is called. It may be
    called on any thread(s) and in any order.
    
    To stop replicating, call stop_replicating() with the same replicant. Note that it is not safe
    to delete the replicant until its stopped() method is called. */
    
    struct replicant_t {
        
        struct done_callback_t {
            virtual void have_copied_value() = 0;
        };
        virtual void value(const store_key_t *key, const_buffer_group_t *value, done_callback_t *cb, mcflags_t flags, exptime_t exptime, cas_t cas) = 0;
        
        virtual void stopped() = 0;
        
        virtual ~replicant_t() {}
    };
    virtual void replicate(replicant_t *cb) = 0;
    virtual void stop_replicating(replicant_t *cb) = 0;

    virtual ~store_t() {}
};

#endif /* __STORE_HPP__ */
