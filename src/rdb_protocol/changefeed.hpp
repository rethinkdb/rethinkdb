// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_CHANGEFEED_HPP_
#define RDB_PROTOCOL_CHANGEFEED_HPP_

#include <deque>
#include <exception>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <utility>

#include "errors.hpp"
#include <boost/variant.hpp>

#include "btree/keys.hpp"
#include "clustering/administration/auth/user_context.hpp"
#include "concurrency/new_mutex.hpp"
#include "concurrency/promise.hpp"
#include "concurrency/rwlock.hpp"
#include "containers/counted.hpp"
#include "containers/scoped.hpp"
#include "protocol_api.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/datumspec.hpp"
#include "rdb_protocol/shards.hpp"
#include "region/region.hpp"
#include "repli_timestamp.hpp"
#include "rpc/connectivity/peer_id.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rpc/serialize_macros.hpp"
#include "containers/archive/boost_types.hpp"

class artificial_table_backend_t;
class auto_drainer_t;
class base_table_t;
class btree_slice_t;
class mailbox_manager_t;
class real_superblock_t;
class sindex_superblock_t;
class table_meta_client_t;
struct rdb_modification_report_t;
struct sindex_disk_info_t;

// The string is the btree index key
typedef std::pair<ql::datum_t, std::string> index_pair_t;
typedef std::map<std::string, std::vector<index_pair_t> > index_vals_t;

namespace ql {

class base_exc_t;
class batcher_t;
class datum_stream_t;
class env_t;
class table_t;

namespace changefeed {

// The pairs contain `<Id, <Key, Val> >` where `Id` uniquely identifies an entry
// (basically a primary key + a tag in the case of a multi-index), `Key` is the
// primary or secondary value we're sorting by, and `Val` is the actual row.
typedef std::pair<std::string, std::pair<datum_t, datum_t> > item_t;
typedef std::pair<const std::string, std::pair<datum_t, datum_t> > const_item_t;

std::vector<item_t> mangle_sort_truncate_stream(
    raw_stream_t &&stream, is_primary_t is_primary, sorting_t sorting, size_t n);

boost::optional<datum_t> apply_ops(
    const datum_t &val,
    const std::vector<scoped_ptr_t<op_t> > &ops,
    env_t *env,
    const datum_t &key) THROWS_NOTHING;

struct msg_t {
    struct limit_start_t {
        uuid_u sub;
        std::vector<item_t> start_data;
        limit_start_t() { }
        limit_start_t(uuid_u _sub, decltype(start_data) _start_data)
            : sub(std::move(_sub)), start_data(std::move(_start_data)) { }
        RDB_DECLARE_ME_SERIALIZABLE(limit_start_t);
    };
    struct limit_change_t {
        uuid_u sub;
        boost::optional<std::string> old_key;
        boost::optional<std::pair<std::string, std::pair<datum_t, datum_t> > > new_val;
        RDB_DECLARE_ME_SERIALIZABLE(limit_change_t);
    };
    struct limit_stop_t {
        uuid_u sub;
        exc_t exc;
        RDB_DECLARE_ME_SERIALIZABLE(limit_stop_t);
    };
    struct change_t {
        index_vals_t old_indexes, new_indexes;
        store_key_t pkey;
        /* For a newly-created row, `old_val` is an empty `datum_t`. For a deleted row,
        `new_val` is an empty `datum_t`. */
        datum_t old_val, new_val;
        RDB_DECLARE_ME_SERIALIZABLE(change_t);
    };
    struct stop_t {
        RDB_DECLARE_ME_SERIALIZABLE(stop_t);
    };

    msg_t() { }
    msg_t(msg_t &&msg) : op(std::move(msg.op)) { }
    msg_t(const msg_t &) = default;
    msg_t &operator=(const msg_t &) = default;

    // Starts with STOP to avoid doing work for default initialization.
    typedef boost::variant<stop_t,
                           change_t,
                           limit_start_t,
                           limit_change_t,
                           limit_stop_t> op_t;
    op_t op;

    // Accursed reference collapsing!
    template<class T, class = typename std::enable_if<std::is_object<T>::value>::type>
    explicit msg_t(T &&_op) : op(decltype(op)(std::move(_op))) { }
};

RDB_DECLARE_SERIALIZABLE(msg_t);

class real_feed_t;
struct stamped_msg_t;

typedef mailbox_addr_t<void(stamped_msg_t)> client_addr_t;

struct keyspec_t {
    struct range_t {
        std::vector<transform_variant_t> transforms;
        boost::optional<std::string> sindex;
        sorting_t sorting;
        datumspec_t datumspec;
        boost::optional<datum_t> intersect_geometry;
    };
    struct empty_t { };
    struct limit_t {
        range_t range;
        size_t limit;
    };
    struct point_t {
        datum_t key;
    };

    keyspec_t(keyspec_t &&other) noexcept
        : spec(std::move(other.spec)),
          table(std::move(other.table)),
          table_name(std::move(other.table_name)) { }
    ~keyspec_t();

    // Accursed reference collapsing!
    template<class T, class = typename std::enable_if<std::is_object<T>::value>::type>
    explicit keyspec_t(T &&t,
                       counted_t<base_table_t> &&_table,
                       std::string _table_name)
        : spec(std::move(t)),
          table(std::move(_table)),
          table_name(std::move(_table_name)) { }

    // This needs to be copyable and assignable because it goes inside a
    // `changefeed_stamp_t`, which goes inside a variant.
    keyspec_t(const keyspec_t &) = default;
    keyspec_t &operator=(const keyspec_t &) = default;

    typedef boost::variant<range_t, empty_t, limit_t, point_t> spec_t;
    spec_t spec;
    counted_t<base_table_t> table;
    std::string table_name;
};
region_t keyspec_to_region(const keyspec_t &keyspec);

struct streamspec_t {
    counted_t<datum_stream_t> maybe_src; // Non-null iff `include_initial`.
    std::string table_name;
    bool include_offsets;
    bool include_states;
    bool include_types;
    configured_limits_t limits;
    datum_t squash;
    keyspec_t::spec_t spec;
    streamspec_t(counted_t<datum_stream_t> _maybe_src,
                 std::string _table_name,
                 bool _include_offsets,
                 bool _include_states,
                 bool _include_types,
                 configured_limits_t _limits,
                 datum_t _squash,
                 keyspec_t::spec_t _spec);
};

RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(keyspec_t::range_t);
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(keyspec_t::empty_t);
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(keyspec_t::limit_t);
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(keyspec_t::point_t);

// The `client_t` exists on the server handling the changefeed query, in the
// `rdb_context_t`.  When a query subscribes to the changes on a table, it
// should call `new_stream`.  The `client_t` will give it back a stream of rows.
// The `client_t` does this by maintaining an internal map from table UUIDs to
// `real_feed_t`s.  (It does this so that there is at most one `real_feed_t` per
// <table, client> pair, to prevent redundant cluster messages.)  The actual
// logic for subscribing to a changefeed server and distributing writes to
// streams can be found in the `real_feed_t` class.
class client_t : public home_thread_mixin_t {
public:
    typedef client_addr_t addr_t;
    client_t(
        mailbox_manager_t *_manager,
        const std::function<
            namespace_interface_access_t(
                const namespace_id_t &,
                signal_t *)
            > &_namespace_source
        );
    ~client_t();
    // Throws QL exceptions.
    counted_t<datum_stream_t> new_stream(
        env_t *env,
        const streamspec_t &ss,
        const namespace_id_t &table_id,
        backtrace_id_t bt,
        table_meta_client_t *table_meta_client);
    void maybe_remove_feed(
        const auto_drainer_t::lock_t &lock, const namespace_id_t &uuid);
    scoped_ptr_t<real_feed_t> detach_feed(
        const auto_drainer_t::lock_t &lock,
        real_feed_t *expected_feed);
private:
    friend class subscription_t;
    mailbox_manager_t *const manager;
    std::function<
        namespace_interface_access_t(
            const namespace_id_t &,
            signal_t *)
        > const namespace_source;
    std::map<namespace_id_t, scoped_ptr_t<real_feed_t> > feeds;
    // This lock manages access to the `feeds` map.  The `feeds` map needs to be
    // read whenever `new_stream` is called, and needs to be written to whenever
    // `new_stream` is called with a table not already in the `feeds` map, or
    // whenever `maybe_remove_feed` or `detach_feed` is called.
    //
    // This lock is held for a long time when `new_stream` is called with a table
    // not already in the `feeds` map (in fact, it's held long enough to do a
    // cluster read).  This should only be a problem if the number of tables
    // (*not* the number of feeds) is large relative to read throughput, because
    // otherwise most of the calls to `new_stream` that block will see the table
    // as soon as they're woken up and won't have to do a second read.
    rwlock_t feeds_lock;
    auto_drainer_t drainer;
};

typedef mailbox_addr_t<void(client_addr_t)> server_addr_t;

template<class Id, class Key, class Val, class Gt>
class index_queue_t {
private:
    std::map<Id, std::pair<Key, Val> > data;
    typedef typename std::map<Id, std::pair<Key, Val> >::iterator diterator;
    std::set<diterator,
             std::function<bool(const diterator &, const diterator &)> > index;

    // This can't be passed by reference because we sometimes erase the source
    // of the reference in the body of this function.
    void erase(diterator it) {
        guarantee(it != data.end());
        auto ft = index.find(it);
        guarantee(ft != index.end());
        index.erase(ft);
        data.erase(it);
        guarantee(data.size() == index.size());
    }
public:
    typedef typename
    std::set<diterator, std::function<bool(const diterator &,
                                           const diterator &)> >::iterator iterator;
    typedef typename
    std::set<diterator, std::function<bool(const diterator &,
                                           const diterator &)> >::const_iterator
    const_iterator;

    explicit index_queue_t(Gt gt) : index(gt) { }

    MUST_USE std::pair<iterator, bool>
    insert(std::pair<Id, std::pair<Key, Val> > pair) {
        return insert(std::move(pair.first),
                      std::move(pair.second.first),
                      std::move(pair.second.second));
    }
    MUST_USE std::pair<iterator, bool> insert(Id i, Key k, Val v) {
        std::pair<diterator, bool> p = data.insert(
            std::make_pair(std::move(i), std::make_pair(std::move(k), std::move(v))));
        iterator it;
        if (p.second) { // inserted
            auto pair = index.insert(p.first);
            guarantee(pair.second);
            it = pair.first;
        } else {
            it = index.find(p.first);
            guarantee(it != index.end());
        }
        guarantee(data.size() == index.size());
        return std::make_pair(it, p.second);
    }

    size_t size() const {
        guarantee(data.size() == index.size());
        return data.size();
    }

    iterator begin() { return index.begin(); }
    iterator end() { return index.end(); }
    const_iterator begin() const { return index.begin(); }
    const_iterator end() const { return index.end(); }
    // This is sometimes called after `**raw_it` has been invalidated, so we
    // can't just dispatch to the `erase(diterator)` implementation above.
    void erase(const iterator &raw_it) {
        guarantee(raw_it != index.end());
        data.erase(*raw_it);
        index.erase(raw_it);
        guarantee(data.size() == index.size());
    }
    void clear() {
        data.clear();
        index.clear();
        guarantee(data.size() == index.size());
    }

    iterator find_id(const Id &i) {
        auto dit = data.find(i);
        return dit == data.end() ? index.end() : index.find(dit);
    }
    MUST_USE bool del_id(const Id &i) {
        auto it = data.find(i);
        if (it == data.end()) {
            return false;
        } else {
            erase(it);
            return true;
        }
    }
    iterator top() {
        return size() == 0 ? end() : *index.begin();
    }
    std::vector<Id> truncate_top(size_t n) {
        std::vector<Id> ret;
        while (index.size() > n) {
            ret.push_back((*index.begin())->first);
            data.erase(*index.begin());
            index.erase(index.begin());
        }
        guarantee(data.size() == index.size());
        return ret;
    }
};

class limit_order_t {
public:
    explicit limit_order_t(sorting_t _sorting);
    bool operator()(const item_t &, const item_t &) const;
    bool operator()(const const_item_t &, const const_item_t &) const;
    template<class T>
    bool operator()(const T &a, const T &b) const {
        return (*this)(*a, *b);
    }
private:
    bool operator()(const datum_t &, const datum_t &) const;
    bool operator()(const std::string &, const std::string &) const;
    bool subop(
        const std::string &a_str, const std::pair<datum_t, datum_t> &a_pair,
        const std::string &b_str, const std::pair<datum_t, datum_t> &b_pair) const;
    const sorting_t sorting;
};

typedef index_queue_t<std::string, datum_t, datum_t, limit_order_t> item_queue_t;

struct primary_ref_t {
    btree_slice_t *btree;
    real_superblock_t *superblock;
};

struct sindex_ref_t {
    btree_slice_t *btree;
    sindex_superblock_t *superblock;
    const sindex_disk_info_t *sindex_info;
};

class server_t;
class limit_manager_t {
public:
    // Make sure you have a lock in the `server_t` (e.g. the lock provided by
    // `foreach_limit`) before calling the constructor or any of the member
    // functions.
    limit_manager_t(
        rwlock_in_line_t *clients_lock,
        region_t _region,
        std::string _table,
        rdb_context_t *ctx,
        global_optargs_t optargs,
        auth::user_context_t user_context,
        uuid_u _uuid,
        server_t *_parent,
        client_t::addr_t _parent_client,
        keyspec_t::limit_t _spec,
        limit_order_t _lt,
        std::vector<item_t> &&item_vec);

    void add(rwlock_in_line_t *spot,
             store_key_t sk,
             is_primary_t is_primary,
             datum_t key,
             datum_t val) THROWS_NOTHING;
    void del(rwlock_in_line_t *spot,
             store_key_t sk,
             is_primary_t is_primary) THROWS_NOTHING;
    void commit(rwlock_in_line_t *spot,
                const boost::variant<primary_ref_t, sindex_ref_t> &sindex_ref)
        THROWS_NOTHING;

    void abort(exc_t e);
    bool is_aborted() { return aborted; }

    const region_t region;
    const std::string table;
    const uuid_u uuid;
private:
    // Can throw `exc_t` exceptions if an error occurs while reading from disk.
    std::vector<item_t> read_more(
        const boost::variant<primary_ref_t, sindex_ref_t> &ref,
        const boost::optional<item_t> &start);
    void send(msg_t &&msg);

    scoped_ptr_t<env_t> env;

    server_t *parent;
    client_t::addr_t parent_client;

    keyspec_t::limit_t spec;
    std::vector<scoped_ptr_t<op_t> > ops;

    limit_order_t gt;
    item_queue_t item_queue;

    std::map<std::string, std::pair<datum_t, datum_t> > added;
    std::set<std::string> deleted;

    bool aborted;
public:
    rwlock_t lock;
    auto_drainer_t drainer;
};

// There is one `server_t` per `store_t`, and it is used to send changes that
// occur on that `store_t` to any subscribed `real_feed_t`s contained in a
// `client_t`.
class server_t {
public:
    typedef server_addr_t addr_t;
    typedef mailbox_addr_t<void(client_t::addr_t, boost::optional<std::string>, uuid_u)>
        limit_addr_t;
    explicit server_t(mailbox_manager_t *_manager, store_t *_parent);
    ~server_t();
    void add_client(
        const client_t::addr_t &addr,
        region_t region,
        const auto_drainer_t::lock_t &keepalive);
    void add_limit_client(
        const client_t::addr_t &addr,
        const region_t &region,
        const std::string &table,
        rdb_context_t *ctx,
        global_optargs_t optargs,
        auth::user_context_t user_context,
        const uuid_u &client_uuid,
        const keyspec_t::limit_t &spec,
        limit_order_t lt,
        std::vector<item_t> &&start_data,
        const auto_drainer_t::lock_t &keepalive);
    // `key` should be non-NULL if there is a key associated with the message.
    void send_all(
        const msg_t &msg,
        const store_key_t &key,
        rwlock_in_line_t *stamp_spot,
        const auto_drainer_t::lock_t &keepalive);
    addr_t get_stop_addr();
    limit_addr_t get_limit_stop_addr();
    boost::optional<uint64_t> get_stamp(
        const client_t::addr_t &addr,
        const auto_drainer_t::lock_t &keepalive);
    uuid_u get_uuid();
    // `f` will be called with a read lock on `clients` and a write lock on the
    // limit manager.
    void foreach_limit(
        const boost::optional<std::string> &s,
        const store_key_t *pkey, // NULL if none
        std::function<void(rwlock_in_line_t *,
                           rwlock_in_line_t *,
                           rwlock_in_line_t *,
                           limit_manager_t *)> f,
        const auto_drainer_t::lock_t &keepalive) THROWS_NOTHING;
    bool has_limit(
        const boost::optional<std::string> &s,
        const auto_drainer_t::lock_t &keepalive);
    auto_drainer_t::lock_t get_keepalive();
private:
    friend class limit_manager_t;
    void stop_mailbox_cb(signal_t *interruptor, client_t::addr_t addr);
    void limit_stop_mailbox_cb(signal_t *interruptor,
                               client_t::addr_t addr,
                               boost::optional<std::string> sindex,
                               uuid_u uuid);
    void add_client_cb(
        signal_t *stopped,
        client_t::addr_t addr,
        auto_drainer_t::lock_t keepalive);

    // The UUID of the server, used so that `real_feed_t`s can enforce on ordering on
    // changefeed messages on a per-server basis (and drop changefeed messages
    // from before their own creation timestamp on a per-server basis).
    const uuid_u uuid;
    mailbox_manager_t *const manager;

    struct client_info_t {
        client_info_t();
        scoped_ptr_t<cond_t> cond;
        uint64_t stamp;
        std::vector<region_t> regions;
        std::map<boost::optional<std::string>,
                 std::vector<scoped_ptr_t<limit_manager_t> >,
                 // Be careful not to remove this, since optionals are
                 // convertible to bool.
                 std::function<
                     bool(const boost::optional<std::string> &,
                          const boost::optional<std::string> &)> > limit_clients;
        scoped_ptr_t<rwlock_t> limit_clients_lock;
    };
    std::map<client_t::addr_t, client_info_t> clients;

    void prune_dead_limit(
        auto_drainer_t::lock_t *stealable_lock,
        scoped_ptr_t<rwlock_in_line_t> *stealable_clients_read_lock,
        client_info_t *info,
        boost::optional<std::string> sindex,
        size_t offset);

    void send_one_with_lock(std::pair<const client_t::addr_t, client_info_t> *client,
                            msg_t msg,
                            const auto_drainer_t::lock_t &lock);

    // Controls access to `clients`.  A `server_t` needs to read `clients` when:
    // * `send_all` is called
    // * `get_stamp` is called
    // And needs to write to clients when:
    // * `add_client` is called
    // * `clear` is called
    // * A message is received at `stop_mailbox` unsubscribing a client
    // A lock is needed because e.g. `send_all` calls `send`, which can block,
    // while looping over `clients`, and we need to make sure the map doesn't
    // change under it.
    rwlock_t clients_lock;
    // We need access to the stamp lock that exists on the parent.
    store_t *parent;

    auto_drainer_t drainer;
    // Clients send a message to this mailbox with their address when they want
    // to unsubscribe.  The callback of this mailbox acquires the drainer, so it
    // has to be destroyed first.
    mailbox_t<void(client_t::addr_t)> stop_mailbox;
    // Clients send a message to this mailbox to unsubscribe a particular limit
    // changefeed.
    mailbox_t<void(client_t::addr_t, boost::optional<std::string>, uuid_u)>
        limit_stop_mailbox;
};

class artificial_feed_t;
class artificial_t : public home_thread_mixin_t {
public:
    artificial_t();
    virtual ~artificial_t();

    /* Rules for synchronization between `subscribe()` and `send_all()`:
    - `send_all()` must never be called during a call to `subscribe()`
    - The stream of changes to `send_all()` must not squash the state represented by
        `initial_values` */

    counted_t<datum_stream_t> subscribe(
        env_t *env,
        const streamspec_t &ss,
        const std::string &primary_key_name,
        const std::vector<datum_t> &initial_values,
        backtrace_id_t bt);

    void send_all(const msg_t &msg);

    /* `can_be_removed()` returns `true` if there are no changefeeds currently using the
    `artificial_t`. `maybe_remove()` is called when the last changefeed stops using the
    `artificial_t`, but new changfeeds may be subscribed after `maybe_remove()` is
    called. */
    bool can_be_removed();
    virtual void maybe_remove() = 0;

private:
    new_mutex_t stamp_mutex;
    uint64_t stamp;
    const uuid_u uuid;
    const scoped_ptr_t<artificial_feed_t> feed;
};

} // namespace changefeed
} // namespace ql

#endif // RDB_PROTOCOL_CHANGEFEED_HPP_

