#ifndef __MEMCACHED_STORE_HPP__
#define __MEMCACHED_STORE_HPP__

#include "memcached/queries.hpp"

class order_token_t;

class sequence_group_t;

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
    repli_timestamp_t timestamp;

    castime_t(cas_t proposed_cas_, repli_timestamp_t timestamp_)
        : proposed_cas(proposed_cas_), timestamp(timestamp_) { }

    castime_t() : proposed_cas(0), timestamp(repli_timestamp_t::invalid) { }
};

struct mutation_t {
    typedef boost::variant<get_cas_mutation_t, sarc_mutation_t, delete_mutation_t, incr_decr_mutation_t, append_prepend_mutation_t> mutation_variant_t;
    mutation_variant_t mutation;

    mutation_t() {
    }

    template<class T>
    mutation_t(const T &m) : mutation(m) { }  // NOLINT (runtime/explicit)

    /* get_key() extracts the "key" field from whichever sub-mutation we actually are */
    store_key_t get_key() const;
};

struct mutation_result_t {
    typedef boost::variant<get_result_t, set_result_t, delete_result_t, incr_decr_result_t, append_prepend_result_t> result_variant_t;
    result_variant_t result;

    mutation_result_t() { }
    explicit mutation_result_t(const result_variant_t& r) : result(r) { }
};

class set_store_interface_t {

public:
    /* These NON-VIRTUAL methods all construct a mutation_t and then call change(). */
    get_result_t get_cas(sequence_group_t *seq_group, const store_key_t &key, order_token_t token);
    set_result_t sarc(sequence_group_t *seq_group, const store_key_t &key, boost::intrusive_ptr<data_buffer_t> data, mcflags_t flags, exptime_t exptime, add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas, order_token_t token);
    incr_decr_result_t incr_decr(sequence_group_t *seq_group, incr_decr_kind_t kind, const store_key_t &key, uint64_t amount, order_token_t token);
    append_prepend_result_t append_prepend(sequence_group_t *seq_group, append_prepend_kind_t kind, const store_key_t &key, boost::intrusive_ptr<data_buffer_t> data, order_token_t token);
    delete_result_t delete_key(sequence_group_t *seq_group, const store_key_t &key, order_token_t token, bool dont_store_in_delete_queue = false);

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
    public home_thread_mixin_t {
public:
    explicit timestamping_set_store_interface_t(set_store_t *target);

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
