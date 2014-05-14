#ifndef RDB_PROTOCOL_CHANGEFEED_HPP_
#define RDB_PROTOCOL_CHANGEFEED_HPP_

#include <deque>
#include <exception>
#include <map>

#include "errors.hpp"

#include <boost/variant.hpp>

#include "concurrency/rwlock.hpp"
#include "containers/counted.hpp"
#include "containers/scoped.hpp"
#include "protocol_api.hpp"
#include "repli_timestamp.hpp"
#include "rpc/connectivity/connectivity.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rpc/serialize_macros.hpp"

class auto_drainer_t;
class base_namespace_repo_t;
class mailbox_manager_t;
struct rdb_modification_report_t;

namespace ql {

class base_exc_t;
class batcher_t;
class changefeed_t;
class datum_stream_t;
class datum_t;
class env_t;
class table_t;

namespace changefeed {

struct msg_t {
    struct change_t {
        change_t();
        explicit change_t(counted_t<const datum_t> _old_val,
                          counted_t<const datum_t> _new_val);
        ~change_t();
        counted_t<const datum_t> old_val, new_val;
        RDB_DECLARE_ME_SERIALIZABLE;
    };
    struct stop_t { RDB_DECLARE_ME_SERIALIZABLE; };

    msg_t() { }
    msg_t(msg_t &&msg);
    msg_t(const msg_t &msg) = default;
    explicit msg_t(stop_t &&op);
    explicit msg_t(change_t &&op);

    // Starts with STOP to avoid doing work for default initialization.
    boost::variant<stop_t, change_t> op;

    RDB_DECLARE_ME_SERIALIZABLE;
};

class feed_t;
struct stamped_msg_t;

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
    typedef mailbox_addr_t<void(stamped_msg_t)> addr_t;
    client_t(mailbox_manager_t *_manager);
    ~client_t();
    // Throws QL exceptions.
    counted_t<datum_stream_t> new_feed(const counted_t<table_t> &tbl, env_t *env);
    void maybe_remove_feed(const uuid_u &uuid);
    scoped_ptr_t<feed_t> detach_feed(const uuid_u &uuid);
private:
    friend class sub_t;
    mailbox_manager_t *manager;
    std::map<uuid_u, scoped_ptr_t<feed_t> > feeds;
    // This lock manages access to the `feeds` map.  The `feeds` map needs to be
    // read whenever `new_feed` is called, and needs to be written to whenever
    // `new_feed` is called with a table not already in the `feeds` map, or
    // whenever `maybe_remove_feed` or `detach_feed` is called.
    rwlock_t feeds_lock;
    auto_drainer_t drainer;
};

// There is one `server_t` per `store_t`, and it is used to send changes that
// occur on that `store_t` to any subscribed `feed_t`s contained in a
// `client_t`.
class server_t {
public:
    typedef mailbox_addr_t<void(client_t::addr_t)> addr_t;
    server_t(mailbox_manager_t *_manager);
    void add_client(const client_t::addr_t &addr);
    void send_all(msg_t msg);
    addr_t get_stop_addr();
    uint64_t get_stamp(const client_t::addr_t &addr);
    uuid_u get_uuid();
private:
    void stop_mailbox_cb(client_t::addr_t addr);
    void add_client_cb(signal_t *stopped, client_t::addr_t addr);

    uuid_u uuid;
    mailbox_manager_t *manager;

    struct client_info_t {
        scoped_ptr_t<cond_t> cond;
        uint64_t stamp;
    };
    std::map<client_t::addr_t, client_info_t> clients;
    rwlock_t clients_lock;

    mailbox_t<void(client_t::addr_t)> stop_mailbox;
    auto_drainer_t drainer;
};

} // namespace changefeed
} // namespace ql

#endif // RDB_PROTOCOL_CHANGEFEED_HPP_
