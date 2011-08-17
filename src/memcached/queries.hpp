#ifndef __MEMCACHED_QUERIES_HPP__
#define __MEMCACHED_QUERIES_HPP__

#include <string.h>
#include <stdio.h>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>

#include "utils.hpp"

typedef uint32_t mcflags_t;
typedef uint32_t exptime_t;
typedef uint64_t cas_t;

class data_provider_t;

template <typename T> struct one_way_iterator_t;

/* `store_key_t` */

struct store_key_t {
    uint8_t size;
    char contents[MAX_KEY_SIZE];

    store_key_t() { }

    store_key_t(int sz, const char *buf) {
        assign(sz, buf);
    }

    store_key_t(const std::string& s) {
        assign(s.size(), s.data());
    }

    void assign(int sz, const char *buf) {
        rassert(sz <= MAX_KEY_SIZE);
        size = sz;
        memcpy(contents, buf, sz);
    }

    void print() const {
        printf("%*.*s", size, size, contents);
    }

    static store_key_t min() {
        return store_key_t(0, NULL);
    }

    static store_key_t max() {
        uint8_t buf[MAX_KEY_SIZE];
        for (int i = 0; i < MAX_KEY_SIZE; i++) buf[i] = 255;
        return store_key_t(MAX_KEY_SIZE, (char *)buf);
    }

    bool increment() {
        if (size < MAX_KEY_SIZE) {
            contents[size] = 0;
            size++;
            return true;
        }
        while (size > 0 && (reinterpret_cast<uint8_t *>(contents))[size-1] == 255) {
            size--;
        }
        if (size == 0) {
            /* We were the largest possible key. Oops. Restore our previous
            state and return `false`. */
            *this = store_key_t::max();
            return false;
        }
        (reinterpret_cast<uint8_t *>(contents))[size-1]++;
        return true;
    }

    bool decrement() {
        if (size == 0) {
            return false;
        }
        if ((reinterpret_cast<uint8_t *>(contents))[size-1] > 0) {
            (reinterpret_cast<uint8_t *>(contents))[size-1]--;
            return true;
        }
        size--;
        return true;
    }
};

inline bool operator==(const store_key_t &k1, const store_key_t &k2) {
    return k1.size == k2.size && memcmp(k1.contents, k2.contents, k1.size) == 0;
}

inline bool operator!=(const store_key_t &k1, const store_key_t &k2) {
    return !(k1 == k2);
}

inline bool operator<(const store_key_t &k1, const store_key_t &k2) {
    return sized_strcmp(k1.contents, k1.size, k2.contents, k2.size) < 0;
}

inline bool operator>(const store_key_t &k1, const store_key_t &k2) {
    return k2 < k1;
}

inline bool operator<=(const store_key_t &k1, const store_key_t &k2) {
    return sized_strcmp(k1.contents, k1.size, k2.contents, k2.size) <= 0;
}

inline bool operator>=(const store_key_t &k1, const store_key_t &k2) {
    return k2 <= k1;
}

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

inline std::string key_to_str(const store_key_t &key) {
    return std::string(key.contents, key.size);
}

/* `get` */

struct get_query_t {
    store_key_t key;
};

struct get_result_t {
    get_result_t(boost::shared_ptr<data_provider_t> v, mcflags_t f, cas_t c) :
        is_not_allowed(false), value(v), flags(f), cas(c) { }
    get_result_t() :
        is_not_allowed(false), value(), flags(0), cas(0) { }

    /* If true, then all other fields should be ignored. */
    bool is_not_allowed;

    // NULL means not found. Parts of the store may wait for the data_provider_t's destructor,
    // so don't hold on to it forever.
    boost::shared_ptr<data_provider_t> value;

    mcflags_t flags;
    cas_t cas;
};

/* `rget` */

enum rget_bound_mode_t {
    rget_bound_open,   // Don't include boundary key
    rget_bound_closed,   // Include boundary key
    rget_bound_none   // Ignore boundary key and go all the way to the left/right side of the tree
};

struct rget_query_t {
    rget_bound_mode_t left_mode;
    store_key_t left_key;
    rget_bound_mode_t right_mode;
    store_key_t right_key;
};

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

// A NULL unique pointer means not allowed
typedef boost::shared_ptr<one_way_iterator_t<key_with_data_provider_t> > rget_result_t;

/* `gets` */

struct get_cas_mutation_t {
    store_key_t key;
};

/* `set`, `add`, `replace`, `cas` */

enum add_policy_t {
    add_policy_yes,
    add_policy_no
};

enum replace_policy_t {
    replace_policy_yes,
    replace_policy_if_cas_matches,
    replace_policy_no
};

#define NO_CAS_SUPPLIED 0

struct sarc_mutation_t {

    /* The key to operate on */
    store_key_t key;

    /* The value to give the key; must not be NULL.
    TODO: Should NULL mean a deletion? */
    boost::shared_ptr<data_provider_t> data;

    /* The flags to store with the value */
    mcflags_t flags;

    /* When to make the value expire */
    exptime_t exptime;

    /* If add_policy is add_policy_no and the key doesn't already exist, then the operation
    will be cancelled and the return value will be sr_didnt_add */
    add_policy_t add_policy;

    /* If replace_policy is replace_policy_no and the key already exists, or if
    replace_policy is replace_policy_if_cas_matches and the key is either missing a
    CAS or has a CAS different from old cas, then the operation will be cancelled and
    the return value will be sr_didnt_replace. */
    replace_policy_t replace_policy;
    cas_t old_cas;

    //sarc_mutation_t() { BREAKPOINT; }
};

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
    /* Returned if the store doesn't want you to do what you're doing. */
    sr_not_allowed,
};

/* `delete` */

struct delete_mutation_t {
    store_key_t key;

    /* This is a hack for replication. If true, the btree will not record the change
    in the delete queue. */
    bool dont_put_in_delete_queue;
};

enum delete_result_t {
    dr_deleted,
    dr_not_found,
    dr_not_allowed
};

/* `incr`, `decr` */

enum incr_decr_kind_t {
    incr_decr_INCR,
    incr_decr_DECR
};

struct incr_decr_mutation_t {
    incr_decr_kind_t kind;
    store_key_t key;
    uint64_t amount;
};

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

/* `append`, `prepend` */

enum append_prepend_kind_t { append_prepend_APPEND, append_prepend_PREPEND };

struct append_prepend_mutation_t {
    append_prepend_kind_t kind;
    store_key_t key;
    boost::shared_ptr<data_provider_t> data;
};

enum append_prepend_result_t {
    apr_success,
    apr_too_large,
    apr_not_found,
    apr_not_allowed,
};

#endif /* __MEMCACHED_QUERIES_HPP__ */
