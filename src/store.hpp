#ifndef __STORE_HPP__
#define __STORE_HPP__

#include "errors.hpp"
#include <boost/scoped_ptr.hpp>
#include "utils.hpp"
#include "arch/arch.hpp"
#include "data_provider.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/fifo_checker.hpp"
#include "containers/iterators.hpp"
#include "containers/unique_ptr.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>

class sequence_group_t;

typedef uint32_t mcflags_t;
typedef uint32_t exptime_t;
typedef uint64_t cas_t;

struct store_key_t {
    uint8_t size;
    char contents[MAX_KEY_SIZE];

    store_key_t() { }
    store_key_t(int sz, const char *buf) {
        assign(sz, buf);
    }

    void assign(int sz, const char *buf) {
        rassert(sz <= MAX_KEY_SIZE);
        size = sz;
        memcpy(contents, buf, sz);
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

inline std::string key_to_str(const store_key_t &key) {
    return std::string(key.contents, key.size);
}

enum rget_bound_mode_t {
    rget_bound_open,   // Don't include boundary key
    rget_bound_closed,   // Include boundary key
    rget_bound_none   // Ignore boundary key and go all the way to the left/right side of the tree
};

struct key_with_data_provider_t {
    std::string key;
    mcflags_t mcflags;
    boost::shared_ptr<data_provider_t> value_provider;

    key_with_data_provider_t(const std::string &key, mcflags_t mcflags, const boost::shared_ptr<data_provider_t>& value_provider) :
        key(key), mcflags(mcflags), value_provider(value_provider) { }

    struct less {
        bool operator()(const key_with_data_provider_t &pair1, const key_with_data_provider_t &pair2) {
            return pair1.key < pair2.key;
        }
    };
};

// A NULL unique pointer means not allowed
typedef unique_ptr_t<one_way_iterator_t<key_with_data_provider_t> > rget_result_t;

struct get_result_t {
    get_result_t(const boost::shared_ptr<data_provider_t>& v, mcflags_t f, cas_t c) :
        is_not_allowed(false), value_provider(v), flags(f), cas(c) { }
    get_result_t() :
        is_not_allowed(false), value_provider(), flags(0), cas(0) { }

    /* If true, then all other fields should be ignored. */
    bool is_not_allowed;

    // NULL means not found. Parts of the store may wait for the data_provider_t's destructor,
    // so don't hold on to it forever.
    boost::shared_ptr<data_provider_t> value_provider;

    mcflags_t flags;
    cas_t cas;
};

class get_store_t {
public:
    virtual get_result_t get(const store_key_t &key, sequence_group_t *seq_group, order_token_t token) = 0;
    virtual rget_result_t rget(sequence_group_t *seq_group, rget_bound_mode_t left_mode, const store_key_t &left_key,
                               rget_bound_mode_t right_mode, const store_key_t &right_key, order_token_t token) = 0;
    virtual ~get_store_t() {}
};

// A castime_t contains proposed cas information (if it's needed) and
// timestamp information.  By deciding these now and passing them in
// at the top, the precise same information gets sent to the replicas.
struct castime_t {
    cas_t proposed_cas;
    repli_timestamp timestamp;

    castime_t(cas_t proposed_cas_, repli_timestamp timestamp_)
        : proposed_cas(proposed_cas_), timestamp(timestamp_) { }
};

/* get_cas is a mutation instead of another method on get_store_t because it may need to
put a CAS-unique on a key that didn't have one before */

struct get_cas_mutation_t {
    store_key_t key;
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

#define NO_CAS_SUPPLIED 0

struct sarc_mutation_t {

    /* The key to operate on */
    store_key_t key;

    /* The value to give the key; must not be NULL.
    TODO: Should NULL mean a deletion? */
    unique_ptr_t<data_provider_t> data;

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
    /* Returned if the data_provider_t that you gave returned have_failed(). */
    sr_data_provider_failed,
    /* Returned if the store doesn't want you to do what you're doing. */
    sr_not_allowed,
};

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

enum append_prepend_kind_t { append_prepend_APPEND, append_prepend_PREPEND };

struct append_prepend_mutation_t {
    append_prepend_kind_t kind;
    store_key_t key;
    unique_ptr_t<data_provider_t> data;
};

enum append_prepend_result_t {
    apr_success,
    apr_too_large,
    apr_not_found,
    apr_data_provider_failed,
    apr_not_allowed,
};

struct mutation_t {

    boost::variant<get_cas_mutation_t, sarc_mutation_t, delete_mutation_t, incr_decr_mutation_t, append_prepend_mutation_t> mutation;

    // implicit
    template<class T>
    mutation_t(const T &m) : mutation(m) { }

    /* get_key() extracts the "key" field from whichever sub-mutation we actually are */
    store_key_t get_key() const;
};

struct mutation_result_t {

    boost::variant<get_result_t, set_result_t, delete_result_t, incr_decr_result_t, append_prepend_result_t> result;

    // implicit
    template<class T>
    mutation_result_t(const T &r) : result(r) { }
};

class set_store_interface_t {

public:
    /* These NON-VIRTUAL methods all construct a mutation_t and then call change(). */
    get_result_t get_cas(sequence_group_t *seq_group, const store_key_t& key, order_token_t token);
    set_result_t sarc(sequence_group_t *seq_group, const store_key_t& key, unique_ptr_t<data_provider_t> data, mcflags_t flags, exptime_t exptime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas, order_token_t token);
    incr_decr_result_t incr_decr(sequence_group_t *seq_group, incr_decr_kind_t kind, const store_key_t& key, uint64_t amount, order_token_t token);
    append_prepend_result_t append_prepend(sequence_group_t *seq_group, append_prepend_kind_t kind, const store_key_t& key, unique_ptr_t<data_provider_t> data, order_token_t token);
    delete_result_t delete_key(sequence_group_t *seq_group, const store_key_t& key, order_token_t token, bool dont_store_in_delete_queue = false);

    virtual mutation_result_t change(sequence_group_t *seq_group, const mutation_t&, order_token_t) = 0;

    virtual ~set_store_interface_t() {}
};

/* set_store_t is different from set_store_interface_t in that it is completely deterministic. If
you have two identical set_store_ts and you send exactly the same data to them, you will put them
into exactly the same state. This is important for keeping replicas in sync.

Usually, all of the changes for a given key must arrive at a set_store_t from the same thread;
otherwise this would defeat the purpose of being deterministic because they would arrive in an
arbitrary order.*/

class set_store_t {

public:
    virtual mutation_result_t change(sequence_group_t *seq_group, const mutation_t&, castime_t, order_token_t) = 0;

    virtual ~set_store_t() {}
};

/* timestamping_store_interface_t timestamps any operations that are given to it and then passes
them on to the underlying store_t. It also linearizes operations; it routes all operations to its
home thread before passing them on, so they happen in a well-defined order. In this way it acts as
a translator between set_store_interface_t and set_store_t. */

class timestamping_set_store_interface_t :
    public set_store_interface_t,
    public home_thread_mixin_t
{

public:
    timestamping_set_store_interface_t(set_store_t *target);

    mutation_result_t change(sequence_group_t *seq_group, const mutation_t&, order_token_t);

    /* When we pass a mutation through, we give it a timestamp determined by the last call to
    set_timestamp(). */
    void set_timestamp(repli_timestamp_t ts);
#ifndef NDEBUG
    repli_timestamp_t get_timestamp() const { return timestamp; }
#endif

private:
    castime_t make_castime();
    set_store_t *target;
    uint32_t cas_counter;
    repli_timestamp_t timestamp;
};

#endif /* __STORE_HPP__ */
