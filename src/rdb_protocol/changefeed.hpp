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
#include "rpc/serialize_macros.hpp"

class auto_drainer_t;
template<class T> class base_namespace_repo_t;
class mailbox_manager_t;
struct rdb_modification_report_t;
struct rdb_protocol_t;

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
    struct start_t  {
        start_t() { }
        start_t(const peer_id_t &_peer_id) : peer_id(_peer_id) { }
        peer_id_t peer_id;
        RDB_DECLARE_ME_SERIALIZABLE;
    };
    struct change_t {
        change_t();
        change_t(const rdb_modification_report_t *report);
        ~change_t();
        counted_t<const datum_t> old_val, new_val;
        RDB_DECLARE_ME_SERIALIZABLE;
    };
    struct stop_t { RDB_DECLARE_ME_SERIALIZABLE; };

    msg_t() { }
    msg_t(msg_t &&msg);
    msg_t(const msg_t &msg);
    msg_t(stop_t &&op);
    msg_t(start_t &&op);
    msg_t(change_t &&op);

    // Starts with STOP to avoid doing work for default initialization.
    boost::variant<stop_t, start_t, change_t> op;

    RDB_DECLARE_ME_SERIALIZABLE;
};

} // namespace changefeed

class changefeed_manager_t : public home_thread_mixin_t {
public:
    changefeed_manager_t(mailbox_manager_t *_manager);
    ~changefeed_manager_t();

    // Uses the home thread of the subscribing stream, NOT the home thread of
    // the changefeed manager.
    class subscription_t : public home_thread_mixin_t {
    public:
        subscription_t(uuid_u uuid,
                       base_namespace_repo_t<rdb_protocol_t> *ns_repo,
                       changefeed_manager_t *manager)
        THROWS_ONLY(cannot_perform_query_exc_t);
        ~subscription_t();
        std::vector<counted_t<const datum_t> > get_els(
            batcher_t *batcher, const signal_t *interruptor);

        void add_el(counted_t<const datum_t> d);
        void abort(const char *msg);
    private:
        void maybe_signal_cond() THROWS_NOTHING;

        bool finished;
        std::exception_ptr exc;
        cond_t *cond; // NULL unless we're waiting.
        std::deque<counted_t<const datum_t> > els;

        changefeed_manager_t *manager;
        std::map<uuid_u, scoped_ptr_t<changefeed_t> >::iterator changefeed;

        scoped_ptr_t<auto_drainer_t> drainer;
    };

    // Throws query language exceptions.
    counted_t<datum_stream_t> changefeed(const counted_t<table_t> &tbl, env_t *env);
    mailbox_manager_t *get_manager() { return manager; }
private:
    mailbox_manager_t *manager;
    std::map<uuid_u, scoped_ptr_t<changefeed_t> > changefeeds;
};

} // namespace ql

#endif // RDB_PROTOCOL_CHANGEFEED_HPP_
