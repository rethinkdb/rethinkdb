#ifndef __STORE_HPP__
#define __STORE_HPP__

typedef uint32_t mcflags_t;
typedef uint32_t exptime_t;
typedef uint64_t cas_t;

struct key_t {
    uint8_t size;
    char contents[0];
    void print() {
        printf("%*.*s", size, size, contents);
    }
};

struct buffer_group_t {
    struct buffer_t {
        size_t size;
        void *data;
    };
    std::vector<buffer_t *> buffers;
    void add_buffer(size_t s, void *d) {
        buffer_t b;
        b.size = s;
        b.data = d;
        buffers.push_back(b);
    }
};

struct data_provider_t {
    
    virtual ~data_provider_t() { }
    virtual size_t get_size() = 0;
    struct done_callback_t {
        virtual void have_provided_value() = 0;
        virtual void have_failed() = 0;
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
        };
        virtual void value(buffer_group_t *value, done_callback_t *cb, mcflags_t flags, cas_t cas) = 0;
        virtual void not_found() = 0;
    };
    virtual void get(key_t *key, get_callback_t *cb);
    virtual void get_cas(key_t *key, get_callback_t *cb);
    
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
        
        /* Called if the data_provider_t that you gave returned have_failed(). */
        virtual void data_provider_failed() = 0;
    };
    virtual void set(key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, set_callback_t *cb) = 0;
    virtual void add(key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, set_callback_t *cb) = 0;
    virtual void replace(key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, set_callback_t *cb) = 0;
    virtual void cas(key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique, set_callback_t *cb) = 0;
    
    /* To increment or decrement a value, use incr() or decr(). They're pretty straight-forward. */
    
    struct incr_decr_callback_t {
        
        virtual void success(unsigned long long new_value) = 0;
        virtual void not_found() = 0;
        virtual void not_numeric() = 0;
    };
    virtual void incr(key_t *key, unsigned long long amount, incr_decr_callback_t *cb) = 0;
    virtual void decr(key_t *key, unsigned long long amount, incr_decr_callback_t *cb) = 0;
    
    /* To append or prepend a value, use append() or prepend(). */
    
    struct append_prepend_callback_t {
        
        virtual void success() = 0;
        virtual void too_large() = 0;
        virtual void not_found() = 0;
        virtual void data_provider_failed() = 0;
    };
    virtual void append(key_t *key, data_provider_t *data, append_prepend_callback_t *cb) = 0;
    virtual void prepend(key_t *key, data_provider_t *data, append_prepend_callback_t *cb) = 0;
    
    /* To delete a key-value pair, use delete(). */
    
    struct delete_callback_t {
        
        virtual void deleted() = 0;
        virtual void not_found() = 0;
    };
    virtual void delete_key(key_t *key, delete_callback_t *cb) = 0;
};

#endif /* __STORE_HPP__ */
