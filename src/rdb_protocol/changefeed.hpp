#ifndef RDB_PROTOCOL_CHANGEFEED_HPP_
#define RDB_PROTOCOL_CHANGEFEED_HPP_

#include <deque>
#include <exception>
#include <map>

#include "errors.hpp"

#include <boost/variant.hpp>

#include "containers/counted.hpp"
#include "containers/scoped.hpp"
#include "protocol_api.hpp"
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
    // RSI: make this client_t::addr_t
    typedef mailbox_addr_t<void(msg_t)> addr_t;
    struct change_t {
        change_t();
        explicit change_t(const rdb_modification_report_t *report);
        ~change_t();
        counted_t<const datum_t> old_val, new_val;
        RDB_DECLARE_ME_SERIALIZABLE;
    };
    struct stop_t { RDB_DECLARE_ME_SERIALIZABLE; };

    msg_t() { }
    msg_t(msg_t &&msg);
    msg_t(const msg_t &msg);
    explicit msg_t(stop_t &&op);
    explicit msg_t(change_t &&op);

    // Starts with STOP to avoid doing work for default initialization.
    boost::variant<stop_t, change_t> op;

    RDB_DECLARE_ME_SERIALIZABLE;
};

class server_t {
public:
    typedef mailbox_addr_t<void(msg_t::addr_t)> addr_t;
    server_t(mailbox_manager_t *_manager);
    void add_client(const msg_t::addr_t &addr);
    void send_all(const msg_t &msg);
    addr_t get_stop_addr();
private:
    mailbox_manager_t *manager;
    std::map<msg_t::addr_t, scoped_ptr_t<cond_t> > clients;
    mailbox_t<void(msg_t::addr_t)> stop_mailbox;
    auto_drainer_t drainer;
};

class feed_t;
class client_t : public home_thread_mixin_t {
public:
    client_t(mailbox_manager_t *_manager);
    ~client_t();
    // Throws QL exceptions.
    counted_t<datum_stream_t> new_feed(const counted_t<table_t> &tbl, env_t *env);
    void remove_feed(const uuid_u &uuid);
    mailbox_manager_t *get_manager() { return manager; }
private:
    friend class sub_t;
    mailbox_manager_t *manager;
    std::map<uuid_u, counted_t<feed_t> > feeds;
};

} // namespace changefeed
} // namespace ql

#endif // RDB_PROTOCOL_CHANGEFEED_HPP_
