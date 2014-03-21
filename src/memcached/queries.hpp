// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef MEMCACHED_QUERIES_HPP_
#define MEMCACHED_QUERIES_HPP_

#include <string.h>
#include <stdio.h>

#include <map>
#include <vector>

#include "protocol_api.hpp"
#include "btree/keys.hpp"
#include "config/args.hpp"
#include "containers/data_buffer.hpp"
#include "containers/counted.hpp"
#include "memcached/region.hpp"
#include "hash_region.hpp"
#include "utils.hpp"

typedef uint32_t mcflags_t;
typedef uint32_t exptime_t;
typedef uint64_t cas_t;

#define INVALID_CAS cas_t(-1)

struct data_buffer_t;


// TODO: Too many of these query objects are publicly mutable objects.


/* `get` */

struct get_query_t {
    store_key_t key;
    get_query_t() { }
    explicit get_query_t(const store_key_t& _key) : key(_key) { }
};

struct get_result_t {
    get_result_t(const counted_t<data_buffer_t>& v, mcflags_t f, cas_t c) :
        value(v), flags(f), cas(c) { }
    get_result_t() :
        value(), flags(0), cas(0) { }

    // NULL means not found.
    counted_t<data_buffer_t> value;

    mcflags_t flags;
    cas_t cas;
};

/* `rget` */

struct rget_query_t {
    hash_region_t<key_range_t> region;
    int maximum;

    rget_query_t() { }
    rget_query_t(const hash_region_t<key_range_t> &_region, int _maximum)
        : region(_region), maximum(_maximum) { }
};

struct key_with_data_buffer_t {
    store_key_t key;
    mcflags_t mcflags;
    counted_t<data_buffer_t> value_provider;

    key_with_data_buffer_t() { }
    key_with_data_buffer_t(const store_key_t &_key,
                           mcflags_t _mcflags,
                           const counted_t<data_buffer_t> &_value_provider)
        : key(_key), mcflags(_mcflags), value_provider(_value_provider) { }
};

struct rget_result_t {
    std::vector<key_with_data_buffer_t> pairs;
    bool truncated;
};

/* `distribution_get` */
struct distribution_get_query_t {
    distribution_get_query_t()
        : max_depth(0), result_limit(0), region(hash_region_t<key_range_t>::universe())
    { }
    distribution_get_query_t(int _max_depth, size_t _result_limit)
        : max_depth(_max_depth), result_limit(_result_limit), region(hash_region_t<key_range_t>::universe())
    { }

    int max_depth;
    size_t result_limit;
    hash_region_t<key_range_t> region;
};

struct distribution_result_t  {
    //Supposing the map has keys:
    //k1, k2 ... kn
    //with k1 < k2 < .. < kn
    //Then k1 == left_key
    //and key_counts[ki] = the number of keys in [ki, ki+1) if i < n
    //key_counts[kn] = the number of keys in [kn, right_key)
    hash_region_t<key_range_t> region;
    std::map<store_key_t, int64_t> key_counts;
};

/* `gets` */

struct get_cas_mutation_t {
    store_key_t key;

    get_cas_mutation_t() { }
    explicit get_cas_mutation_t(const store_key_t &k) : key(k) { }
};

void debug_print(printf_buffer_t *buf, const get_cas_mutation_t& mut);

/* `set`, `add`, `replace`, `cas` */

enum add_policy_t {
    add_policy_yes,
    add_policy_no
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(add_policy_t, int8_t, add_policy_yes, add_policy_no);

enum replace_policy_t {
    replace_policy_yes,
    replace_policy_if_cas_matches,
    replace_policy_no
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(replace_policy_t, int8_t, replace_policy_yes, replace_policy_no);

#define NO_CAS_SUPPLIED 0

struct sarc_mutation_t {
    /* The key to operate on */
    store_key_t key;

    /* The value to give the key; must not be NULL.
    TODO: Should NULL mean a deletion? */
    counted_t<data_buffer_t> data;

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
    sarc_mutation_t(const store_key_t& _key,
                    const counted_t<data_buffer_t>& _data,
                    mcflags_t _flags,
                    exptime_t _exptime,
                    add_policy_t _add_policy,
                    replace_policy_t _replace_policy,
                    cas_t _old_cas) :
        key(_key),
        data(_data),
        flags(_flags),
        exptime(_exptime),
        add_policy(_add_policy),
        replace_policy(_replace_policy),
        old_cas(_old_cas) { }
};

void debug_print(printf_buffer_t *buf, const sarc_mutation_t& mut);

enum set_result_t {
    /* Returned on success */
    sr_stored,
    /* Returned if add_policy is add_policy_no and the key is absent */
    sr_didnt_add,
    /* Returned if replace_policy is replace_policy_no and the key is present or replace_policy
    is replace_policy_if_cas_matches and the CAS does not match */
    sr_didnt_replace,
    /* Returned if the value to be stored is too big */
    sr_too_large
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(set_result_t, int8_t, sr_stored, sr_too_large);

/* `delete` */

struct delete_mutation_t {
    store_key_t key;

    // TODO: wat
    /* This is a hack for replication. If true, the btree will not record the change
    in the delete queue. */
    bool dont_put_in_delete_queue;

    delete_mutation_t() { }
    delete_mutation_t(const store_key_t& _key, bool _dont_put_in_delete_queue) : key(_key), dont_put_in_delete_queue(_dont_put_in_delete_queue) { }
};

void debug_print(printf_buffer_t *buf, const delete_mutation_t& mut);

enum delete_result_t {
    dr_deleted,
    dr_not_found
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(delete_result_t, int8_t, dr_deleted, dr_not_found);

/* `incr`, `decr` */

enum incr_decr_kind_t {
    incr_decr_INCR,
    incr_decr_DECR
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(incr_decr_kind_t, int8_t, incr_decr_INCR, incr_decr_DECR);

struct incr_decr_mutation_t {
    incr_decr_kind_t kind;
    store_key_t key;
    uint64_t amount;

    incr_decr_mutation_t() { }
    incr_decr_mutation_t(incr_decr_kind_t idk, const store_key_t &k, uint64_t a) :
        kind(idk), key(k), amount(a) { }
};

void debug_print(printf_buffer_t *buf, const incr_decr_mutation_t& mut);

struct incr_decr_result_t {
    enum result_t {
        idr_success,
        idr_not_found,
        idr_not_numeric
    } res;
    uint64_t new_value;   // Valid only if idr_success
    incr_decr_result_t() { }
    explicit incr_decr_result_t(result_t r, uint64_t n = 0) : res(r), new_value(n) { }
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(incr_decr_result_t::result_t, int8_t, incr_decr_result_t::idr_success, incr_decr_result_t::idr_not_numeric);

/* `append`, `prepend` */

enum append_prepend_kind_t { append_prepend_APPEND, append_prepend_PREPEND };

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(append_prepend_kind_t, int8_t, append_prepend_APPEND, append_prepend_PREPEND);

struct append_prepend_mutation_t {
    append_prepend_kind_t kind;
    store_key_t key;
    counted_t<data_buffer_t> data;

    append_prepend_mutation_t() { }
    append_prepend_mutation_t(append_prepend_kind_t _kind,
                              const store_key_t &_key,
                              const counted_t<data_buffer_t> &_data)
        : kind(_kind), key(_key), data(_data) { }
};

void debug_print(printf_buffer_t *buf, const append_prepend_mutation_t& mut);

enum append_prepend_result_t {
    apr_success,
    apr_too_large,
    apr_not_found
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(append_prepend_result_t, int8_t, apr_success, apr_not_found);

#endif /* MEMCACHED_QUERIES_HPP_ */
