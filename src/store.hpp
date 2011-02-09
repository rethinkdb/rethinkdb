#ifndef __STORE_HPP__
#define __STORE_HPP__

#include "utils.hpp"
#include "arch/arch.hpp"
#include "data_provider.hpp"
#include "concurrency/cond_var.hpp"
#include "containers/iterators.hpp"
#include <boost/shared_ptr.hpp>

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

inline bool str_to_key(const char *str, store_key_t *buf) {
    int len = strlen(str);
    if (len <= MAX_KEY_SIZE) {
        memcpy(buf->contents, str, len);
        buf->size = (uint8_t) len;
        return true;
    } else {
        return false;
    }
}

inline std::string key_to_str(const store_key_t* key) {
    return std::string(key->contents, key->size);
}

struct key_with_data_provider_t {
    std::string key;
    mcflags_t mcflags;
    boost::shared_ptr<data_provider_t> value_provider;

    key_with_data_provider_t(const std::string &key, mcflags_t mcflags, boost::shared_ptr<data_provider_t> value_provider) :
        key(key), mcflags(mcflags), value_provider(value_provider) { }

    struct less {
        bool operator()(const key_with_data_provider_t &pair1, const key_with_data_provider_t &pair2) {
            return pair1.key < pair2.key;
        }
    };
};


union store_key_and_buffer_t {
    store_key_t key;
    char buffer[sizeof(store_key_t) + MAX_KEY_SIZE];
};

// A castime_t contains proposed cas information (if it's needed) and
// timestamp information.  By deciding these now and passing them in
// at the top, the precise same information gets sent to the replicas.
struct castime_t {
    cas_t proposed_cas;
    repli_timestamp timestamp;

    castime_t(cas_t proposed_cas_, repli_timestamp timestamp_)
        : proposed_cas(proposed_cas_), timestamp(timestamp_) { }

    static castime_t dummy() {
        return castime_t(0, repli_timestamp::invalid);
    }
    bool is_dummy() {
        return proposed_cas == 0 && timestamp.time == repli_timestamp::invalid.time;
    }
};

class store_t {
public:
    /* To get a value from the store, call get() or get_cas(), providing the key you want to get.
    The store will return a get_result_t with either the value or NULL. If it returns a value, you
    must call the provided done_callback_t when you are done to release the buffers holding the
    value. If you call get_cas(), the cas will be in the 'cas' member of the get_result_t; if not,
    the value of 'cas' is undefined and should be ignored. */

    struct get_result_t {
        get_result_t(boost::shared_ptr<data_provider_t> v, mcflags_t f, cas_t c, threadsafe_cond_t *s) :
            value(v), flags(f), cas(c), to_signal_when_done(s) { }
        get_result_t() :
            value(), flags(0), cas(0), to_signal_when_done(NULL) { }

        // NULL means not found. If non-NULL you are responsible for calling get_data_as_buffer(),
        // or get_data_into_buffer() on value. Parts of the store may wait for the data_provider_t's
        // destructor, so don't hold on to it forever.
        boost::shared_ptr<data_provider_t> value;
        mcflags_t flags;
        cas_t cas;
        threadsafe_cond_t *to_signal_when_done;
    };
    virtual get_result_t get(store_key_t *key) = 0;
    virtual get_result_t get_cas(store_key_t *key, castime_t castime) = 0;

    typedef one_way_iterator_t<key_with_data_provider_t>* rget_result_t;
    virtual rget_result_t rget(store_key_t *start, store_key_t *end, bool left_open, bool right_open) = 0;

    /* To set a value in the database, call set(), add(), or replace(). Provide a key* for the key
    to be set and a data_provider_t* for the data. Note that the data_provider_t may be called on
    any core, so you must implement core-switching yourself if necessary. The data_provider_t will
    always be called exactly once. */

    enum set_result_t {
        /* Returned on success */
        sr_stored,
        /* Returned if add_policy is add_policy_no and the key is absent */
        sr_didnt_add,
        /* Returned if replace_policy is replace_policy_no and the key is present or replace_policy
        is replace_policy_if_cas_matches and the CAS does not match */
        sr_didnt_replace,
        /* Returned if the value to be stored is too big */
        sr_too_large,
        /* Returned if the data_provider_t that you gave returned have_failed(). */
        sr_data_provider_failed,
        /* Returned if the store doesn't want you to do what you're doing. */
        sr_not_allowed,
    };

    enum add_policy_t {
        add_policy_yes,
        add_policy_no
    };

    enum replace_policy_t {
        replace_policy_yes,
        replace_policy_if_cas_matches,
        replace_policy_no
    };

    static const cas_t NO_CAS_SUPPLIED = 0;

    virtual set_result_t sarc(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas) = 0;

    /* To increment or decrement a value, use incr() or decr(). They're pretty straight-forward. */

    struct incr_decr_result_t {
        enum result_t {
            idr_success,
            idr_not_found,
            idr_not_numeric,
            idr_not_allowed,
        } res;
        uint64_t new_value;   // Valid only if idr_success
        incr_decr_result_t() { }
        incr_decr_result_t(result_t r, uint64_t n = 0) : res(r), new_value(n) { }
    };

    enum incr_decr_kind_t {
        incr_decr_INCR,
        incr_decr_DECR
    };

    virtual incr_decr_result_t incr_decr(incr_decr_kind_t kind, store_key_t *key, uint64_t amount, castime_t castime) = 0;

    /* To append or prepend a value, use append() or prepend(). */

    enum append_prepend_result_t {
        apr_success,
        apr_too_large,
        apr_not_found,
        apr_data_provider_failed,
        apr_not_allowed,
    };

    enum append_prepend_kind_t { append_prepend_APPEND, append_prepend_PREPEND };

    virtual append_prepend_result_t append_prepend(append_prepend_kind_t kind, store_key_t *key, data_provider_t *data, castime_t castime) = 0;

    /* To delete a key-value pair, use delete(). */

    enum delete_result_t {
        dr_deleted,
        dr_not_found,
        dr_not_allowed
    };

    virtual delete_result_t delete_key(store_key_t *key, repli_timestamp timestamp) = 0;

    virtual ~store_t() {}
};

#endif /* __STORE_HPP__ */
