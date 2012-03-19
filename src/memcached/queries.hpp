#ifndef MEMCACHED_QUERIES_HPP_
#define MEMCACHED_QUERIES_HPP_

#include <string.h>
#include <stdio.h>
#include <vector>

#include "errors.hpp"
#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <boost/serialization/binary_object.hpp>

#include "btree/keys.hpp"
#include "config/args.hpp"
#include "containers/data_buffer.hpp"
#include "protocol_api.hpp"
#include "utils.hpp"

typedef uint32_t mcflags_t;
typedef uint32_t exptime_t;
typedef uint64_t cas_t;

struct data_buffer_t;

template <typename T> struct one_way_iterator_t;

/* `get` */

struct get_query_t {
    store_key_t key;
    get_query_t() { }
    explicit get_query_t(const store_key_t& key_) : key(key_) { }
};

struct get_result_t {
    get_result_t(const boost::intrusive_ptr<data_buffer_t>& v, mcflags_t f, cas_t c) :
        is_not_allowed(false), value(v), flags(f), cas(c) { }
    get_result_t() :
        is_not_allowed(false), value(), flags(0), cas(0) { }

    /* If true, then all other fields should be ignored. */
    bool is_not_allowed;

    // NULL means not found.
    boost::intrusive_ptr<data_buffer_t> value;

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

    rget_query_t() { }
    rget_query_t(const rget_bound_mode_t& left_mode_, const store_key_t left_key_,
                 const rget_bound_mode_t& right_mode_, const store_key_t right_key_)
        : left_mode(left_mode_), left_key(left_key_), right_mode(right_mode_), right_key(right_key_) { }
};

struct key_with_data_buffer_t {
    std::string key;
    mcflags_t mcflags;
    boost::intrusive_ptr<data_buffer_t> value_provider;

    key_with_data_buffer_t(const std::string& _key, mcflags_t _mcflags, const boost::intrusive_ptr<data_buffer_t>& _value_provider)
        : key(_key), mcflags(_mcflags), value_provider(_value_provider) { }

    struct less {
        bool operator()(const key_with_data_buffer_t& pair1, const key_with_data_buffer_t& pair2) {
            return pair1.key < pair2.key;
        }
    };
};

// A NULL unique pointer means not allowed
typedef boost::shared_ptr<one_way_iterator_t<key_with_data_buffer_t> > rget_result_t;

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
    boost::intrusive_ptr<data_buffer_t> data;

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

    sarc_mutation_t() { }
    sarc_mutation_t(const store_key_t& key_, const boost::intrusive_ptr<data_buffer_t>& data_, mcflags_t flags_, exptime_t exptime_, add_policy_t add_policy_, replace_policy_t replace_policy_, cas_t old_cas_) :
        key(key_), data(data_), flags(flags_), exptime(exptime_), add_policy(add_policy_), replace_policy(replace_policy_), old_cas(old_cas_) { }
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

    delete_mutation_t() { }
    delete_mutation_t(const store_key_t& key_, bool dont_put_in_delete_queue_) : key(key_), dont_put_in_delete_queue(dont_put_in_delete_queue_) { }
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
    explicit incr_decr_result_t(result_t r, uint64_t n = 0) : res(r), new_value(n) { }
};

/* `append`, `prepend` */

enum append_prepend_kind_t { append_prepend_APPEND, append_prepend_PREPEND };

struct append_prepend_mutation_t {
    append_prepend_kind_t kind;
    store_key_t key;
    boost::intrusive_ptr<data_buffer_t> data;
};

enum append_prepend_result_t {
    apr_success,
    apr_too_large,
    apr_not_found,
    apr_not_allowed,
};

#endif /* MEMCACHED_QUERIES_HPP_ */
