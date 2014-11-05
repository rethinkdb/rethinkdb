// Copyright 2010-2014 RethinkDB, all rights reserved.
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
#include "concurrency/promise.hpp"
#include "concurrency/rwlock.hpp"
#include "containers/counted.hpp"
#include "containers/scoped.hpp"
#include "protocol_api.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/shards.hpp"
#include "region/region.hpp"
#include "repli_timestamp.hpp"
#include "rpc/connectivity/peer_id.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rpc/serialize_macros.hpp"

class auto_drainer_t;
class btree_slice_t;
class mailbox_manager_t;
class namespace_interface_access_t;
class superblock_t;
struct sindex_disk_info_t;
struct rdb_modification_report_t;

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
typedef std::vector<item_t> item_vec_t;

item_vec_t mangle_sort_truncate_stream(
    stream_t &&stream, is_primary_t is_primary, sorting_t sorting, size_t n);

struct msg_t {
    struct limit_start_t {
        uuid_u sub;
        item_vec_t start_data;
        limit_start_t() { }
        limit_start_t(uuid_u _sub, decltype(start_data) _start_data)
            : sub(std::move(_sub)), start_data(std::move(_start_data)) { }
        RDB_DECLARE_ME_SERIALIZABLE;
    };
    struct limit_change_t {
        uuid_u sub;
        boost::optional<std::string> old_key;
        boost::optional<std::pair<std::string, std::pair<datum_t, datum_t> > > new_val;
        RDB_DECLARE_ME_SERIALIZABLE;
    };
    struct limit_stop_t {
        uuid_u sub;
        exc_t exc;
    };
    struct change_t {
        change_t() { }
        change_t(datum_t _old_val, datum_t _new_val)
            : old_val(std::move(_old_val)), new_val(std::move(_new_val)) { }
        datum_t old_val, new_val;
        RDB_DECLARE_ME_SERIALIZABLE;
    };
    struct stop_t { };

    msg_t() { }
    msg_t(msg_t &&msg) : op(std::move(msg.op)) { }
    template<class T>
    explicit msg_t(T &&_op) : op(std::move(_op)) { }

    // We need to define the copy constructor.  GCC 4.4 doesn't let us use `=
    // default`, and SRH is uncomfortable violating the rule of 3, so we define
    // the destructor and assignment operator as well.
    msg_t(const msg_t &msg) : op(msg.op) { }
    ~msg_t() { }
    const msg_t &operator=(const msg_t &msg) {
        op = msg.op;
        return *this;
    }

    // Starts with STOP to avoid doing work for default initialization.
    boost::variant<stop_t, change_t, limit_start_t, limit_change_t, limit_stop_t> op;
};

RDB_SERIALIZE_OUTSIDE(msg_t::limit_start_t);
RDB_SERIALIZE_OUTSIDE(msg_t::limit_change_t);
RDB_DECLARE_SERIALIZABLE(msg_t::limit_stop_t);
RDB_SERIALIZE_OUTSIDE(msg_t::change_t);
RDB_DECLARE_SERIALIZABLE(msg_t::stop_t);
RDB_DECLARE_SERIALIZABLE(msg_t);

class feed_t;
struct stamped_msg_t;

typedef mailbox_addr_t<void(stamped_msg_t)> client_addr_t;

struct keyspec_t {
    struct range_t {
        range_t() { }
        range_t(boost::optional<std::string> _sindex,
                sorting_t _sorting,
                datum_range_t _range)
            : sindex(std::move(_sindex)),
              sorting(_sorting),
              range(std::move(_range)) { }
        boost::optional<std::string> sindex;
        sorting_t sorting;
        datum_range_t range;
    };
    struct limit_t {
        limit_t() { }
        limit_t(range_t _range, size_t _limit)
            : range(std::move(_range)), limit(_limit) { }
        range_t range;
        size_t limit;
    };
    struct point_t {
        point_t() { }
        explicit point_t(datum_t _key) : key(std::move(_key)) { }
        datum_t key;
    };

    keyspec_t(keyspec_t &&keyspec) : spec(std::move(keyspec.spec)) { }
    template<class T>
    explicit keyspec_t(T &&t) : spec(std::move(t)) { }

    // This needs to be copyable and assignable because it goes inside a
    // `changefeed_stamp_t`, which goes inside a variant.
    keyspec_t(const keyspec_t &keyspec) = default;
    keyspec_t &operator=(const keyspec_t &) = default;

    boost::variant<range_t, limit_t, point_t> spec;
private:
    keyspec_t() { }
};
region_t keyspec_to_region(const keyspec_t &keyspec);

RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(keyspec_t::range_t);
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(keyspec_t::limit_t);
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(keyspec_t::point_t);
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(keyspec_t);

// The `client_t` exists on the machine handling the changefeed query, in the
// `rdb_context_t`.  When a query subscribes to the changes on a table, it
// should call `new_feed`.  The `client_t` will give it back a stream of rows.
// The `client_t` does this by maintaining an internal map from table UUIDs to
// `feed_t`s.  (It does this so that there is at most one `feed_t` per <table,
// client> pair, to prevent redundant cluster messages.)  The actual logic for
// subscribing to a changefeed server and distributing writes to streams can be
// found in the `feed_t` class.
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
    counted_t<datum_stream_t> new_feed(
        env_t *env,
        const namespace_id_t &table,
        const protob_t<const Backtrace> &bt,
        const std::string &table_name,
        const std::string &pkey,
        const keyspec_t &keyspec);
    void maybe_remove_feed(const namespace_id_t &uuid);
    scoped_ptr_t<feed_t> detach_feed(const namespace_id_t &uuid);
private:
    friend class subscription_t;
    mailbox_manager_t *const manager;
    std::function<
        namespace_interface_access_t(
            const namespace_id_t &,
            signal_t *)
        > const namespace_source;
    std::map<namespace_id_t, scoped_ptr_t<feed_t> > feeds;
    // This lock manages access to the `feeds` map.  The `feeds` map needs to be
    // read whenever `new_feed` is called, and needs to be written to whenever
    // `new_feed` is called with a table not already in the `feeds` map, or
    // whenever `maybe_remove_feed` or `detach_feed` is called.
    //
    // This lock is held for a long time when `new_feed` is called with a table
    // not already in the `feeds` map (in fact, it's held long enough to do a
    // cluster read).  This should only be a problem if the number of tables
    // (*not* the number of feeds) is large relative to read throughput, because
    // otherwise most of the calls to `new_feed` that block will see the table
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

    void erase(const diterator &it) {
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
            guarantee(k == p.first->second.first);
            it = index.find(p.first);
            guarantee(it != index.end());
        }
        guarantee(data.size() == index.size());
        return std::make_pair(it, p.second);
    }

    size_t size() {
        guarantee(data.size() == index.size());
        return data.size();
    }

    iterator begin() { return index.begin(); }
    iterator end() { return index.end(); }
    void erase(const iterator &raw_it) {
        guarantee(raw_it != index.end());
        erase(*raw_it);
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
    bool operator()(const datum_t &, const datum_t &) const;
    bool operator()(const std::string &, const std::string &) const;
    template<class T>
    bool operator()(const T &a, const T &b) const {
        return (*this)(*a, *b);
    }
private:
    bool subop(
        const std::string &a_str, const std::pair<datum_t, datum_t> &a_pair,
        const std::string &b_str, const std::pair<datum_t, datum_t> &b_pair) const;
    const sorting_t sorting;
};

typedef index_queue_t<std::string, datum_t, datum_t, limit_order_t> item_queue_t;

struct primary_ref_t {
    btree_slice_t *btree;
    superblock_t *superblock;
};

struct sindex_ref_t {
    btree_slice_t *btree;
    superblock_t *superblock;
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
        std::map<std::string, wire_func_t> optargs,
        uuid_u _uuid,
        server_t *_parent,
        client_t::addr_t _parent_client,
        keyspec_t::limit_t _spec,
        limit_order_t _lt,
        item_vec_t &&item_vec);

    void add(rwlock_in_line_t *spot,
             store_key_t sk,
             is_primary_t is_primary,
             datum_t key,
             datum_t val);
    void del(rwlock_in_line_t *spot,
             store_key_t sk,
             is_primary_t is_primary);
    void commit(rwlock_in_line_t *spot,
                const boost::variant<primary_ref_t, sindex_ref_t> &sindex_ref);

    void abort(exc_t e);
    bool is_aborted() { return aborted; }

    const region_t region; // TODO: use this when ranges are supported.
    const std::string table;
    const uuid_u uuid;
private:
    // Can throw `exc_t` exceptions if an error occurs while reading from disk.
    item_vec_t read_more(const boost::variant<primary_ref_t, sindex_ref_t> &ref,
                         sorting_t sorting,
                         const boost::optional<item_queue_t::iterator> &start,
                         size_t n);
    void send(msg_t &&msg);

    scoped_ptr_t<env_t> env;

    server_t *parent;
    client_t::addr_t parent_client;

    keyspec_t::limit_t spec;
    limit_order_t gt;
    item_queue_t item_queue;

    std::vector<std::pair<std::string, std::pair<datum_t, datum_t> > > added;
    std::vector<std::string> deleted;

    bool aborted;
public:
    rwlock_t lock;
    auto_drainer_t drainer;
};

// There is one `server_t` per `store_t`, and it is used to send changes that
// occur on that `store_t` to any subscribed `feed_t`s contained in a
// `client_t`.
class server_t {
public:
    typedef server_addr_t addr_t;
    typedef mailbox_addr_t<void(client_t::addr_t, boost::optional<std::string>, uuid_u)>
        limit_addr_t;
    explicit server_t(mailbox_manager_t *_manager);
    ~server_t();
    void add_client(const client_t::addr_t &addr, region_t region);
    void add_limit_client(
        const client_t::addr_t &addr,
        const region_t &region,
        const std::string &table,
        rdb_context_t *ctx,
        std::map<std::string, wire_func_t> optargs,
        const uuid_u &client_uuid,
        const keyspec_t::limit_t &spec,
        limit_order_t lt,
        item_vec_t &&start_data);
    // `key` should be non-NULL if there is a key associated with the message.
    void send_all(const msg_t &msg, const store_key_t &key);
    void stop_all();
    addr_t get_stop_addr();
    limit_addr_t get_limit_stop_addr();
    uint64_t get_stamp(const client_t::addr_t &addr);
    uuid_u get_uuid();
    // `f` will be called with a read lock on `clients` and a write lock on the
    // limit manager.
    void foreach_limit(const boost::optional<std::string> &s,
                       const store_key_t *pkey, // NULL if none
                       std::function<void(rwlock_in_line_t *,
                                          rwlock_in_line_t *,
                                          rwlock_in_line_t *,
                                          limit_manager_t *)> f);
    bool has_limit(const boost::optional<std::string> &s);
private:
    friend class limit_manager_t;
    void stop_mailbox_cb(signal_t *interruptor, client_t::addr_t addr);
    void limit_stop_mailbox_cb(signal_t *interruptor,
                               client_t::addr_t addr,
                               boost::optional<std::string> sindex,
                               uuid_u uuid);
    void add_client_cb(signal_t *stopped, client_t::addr_t addr);

    // The UUID of the server, used so that `feed_t`s can enforce on ordering on
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

    void send_one_with_lock(const auto_drainer_t::lock_t &lock,
                            std::pair<const client_t::addr_t, client_info_t> *client,
                            msg_t msg);

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

} // namespace changefeed
} // namespace ql

#endif // RDB_PROTOCOL_CHANGEFEED_HPP_

