#ifndef __STORE_HPP__
#define __STORE_HPP__

#include "utils.hpp"
#include "arch/arch.hpp"

typedef uint32_t mcflags_t;
typedef uint32_t exptime_t;
typedef uint64_t cas_t;

// Note: Changing this struct changes the format of the data stored on disk.
// If you change this struct, previous stored data will be misinterpreted.
struct store_key_t {
    uint8_t size;
    char contents[0];
    uint16_t full_size() const {
        return size + offsetof(store_key_t, contents);
    }
    void print() const {
        printf("%*.*s", size, size, contents);
    }
};

union store_key_and_buffer_t {
    store_key_t key;
    char buffer[sizeof(store_key_t) + MAX_KEY_SIZE];
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
    /* To get a value from the store, call get() or get_cas(), providing the key you want to get.
    The store will return a get_result_t with either the value or NULL. If it returns a value, you
    must call the provided done_callback_t when you are done to release the buffers holding the
    value. If you call get_cas(), the cas will be in the 'cas' member of the get_result_t; if not,
    the value of 'cas' is undefined and should be ignored. */

    struct get_result_t {
        const_buffer_group_t *buffer; //NULL means not found
        struct done_callback_t {
            virtual void have_copied_value() = 0;
            virtual ~done_callback_t() {}
        } *cb;
        mcflags_t flags;
        cas_t cas;
    };
    virtual get_result_t get(store_key_t *key) = 0;
    virtual get_result_t get_cas(store_key_t *key) = 0;

    /* To set a value in the database, call set(), add(), or replace(). Provide a key* for the key
    to be set and a data_provider_t* for the data. Note that the data_provider_t may be called on
    any core, so you must implement core-switching yourself if necessary. The data_provider_t will
    always be called exactly once. */

    enum set_result_t {
        /* Returned on success */
        sr_stored,
        /* Returned if you add() and it already exists or you replace() and it doesn't */
        sr_not_stored,
        /* Returned if you cas() and the key does not exist. */
        sr_not_found,
        /* Returned if you cas() and the key was modified since get_cas(). */
        sr_exists,
        /* Returned if the value to be stored is too big */
        sr_too_large,
        /* Returned if the data_provider_t that you gave returned have_failed(). */
        sr_data_provider_failed
    };

    virtual set_result_t set(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime) = 0;
    virtual set_result_t add(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime) = 0;
    virtual set_result_t replace(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime) = 0;
    virtual set_result_t cas(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique) = 0;

    /* To increment or decrement a value, use incr() or decr(). They're pretty straight-forward. */

    struct incr_decr_result_t {
        enum result_t {
            idr_success,
            idr_not_found,
            idr_not_numeric
        } res;
        unsigned long long new_value;   // Valid only if idr_success
        incr_decr_result_t() { }
        incr_decr_result_t(result_t r, unsigned long long n = 0) : res(r), new_value(n) { }
    };
    virtual incr_decr_result_t incr(store_key_t *key, unsigned long long amount) = 0;
    virtual incr_decr_result_t decr(store_key_t *key, unsigned long long amount) = 0;
    
    /* To append or prepend a value, use append() or prepend(). */
    
    enum append_prepend_result_t {
        apr_success,
        apr_too_large,
        apr_not_found,
        apr_data_provider_failed
    };
    virtual append_prepend_result_t append(store_key_t *key, data_provider_t *data) = 0;
    virtual append_prepend_result_t prepend(store_key_t *key, data_provider_t *data) = 0;
    
    /* To delete a key-value pair, use delete(). */
    
    enum delete_result_t {
        dr_deleted,
        dr_not_found
    };
    virtual delete_result_t delete_key(store_key_t *key) = 0;
    
    /* To start replicating, call replicate() with a replicant_t.
    
    The replicant_t's value() method will be called for every key in the database at the time that
    replicate() is called and for every change that is made after replicate() is called. It may be
    called on any thread(s) and in any order.
    
    To stop replicating, call stop_replicating() with the same replicant. It will be safe to delete
    the replicant only after stopped() is called on it. */
    
    struct replicant_t {
        struct done_callback_t {
            virtual void have_copied_value() = 0;
            virtual ~done_callback_t() {}
        };
        virtual void value(const store_key_t *key, const_buffer_group_t *value, done_callback_t *cb, mcflags_t flags, exptime_t exptime, cas_t cas, repli_timestamp timestamp) = 0;
        virtual void stopped() = 0;
        virtual ~replicant_t() {}
    };
    virtual void replicate(replicant_t *cb, repli_timestamp cutoff) = 0;
    virtual void stop_replicating(replicant_t *cb) = 0;

    virtual ~store_t() {}
};

#endif /* __STORE_HPP__ */
