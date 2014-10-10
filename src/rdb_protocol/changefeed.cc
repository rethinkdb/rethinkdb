#include "rdb_protocol/changefeed.hpp"

#include <queue>

#include "boost_utils.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/interruptor.hpp"
#include "containers/archive/boost_types.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/val.hpp"
#include "rpc/mailbox/typed.hpp"

#include "debug.hpp"

namespace ql {

datum_t key_to_datum(std::string key) {
    // We use the escaped key to construct a `datum_t` that sorts
    // correctly.  An alternative, equally valid strategy would be to
    // keep track of the primary key and reconstruct it from
    // `item.data`.
    std::string escaped_key;
    escaped_key.reserve(key.size() + 10);
    for (const auto &c : key) {
        if (c == 0 || c == 1) {
            escaped_key.push_back(1);
            escaped_key.push_back(c+1);
        } else {
            escaped_key.push_back(c);
        }
    }
    return datum_t(datum_string_t(escaped_key));
}

namespace changefeed {

template<class M, class T>
size_t erase1(M *ms, const T &t) {
    auto it = ms->find(t);
    if (it != ms->end()) {
        ms->erase(it);
        return 1;
    } else {
        return 0;
    }
}

server_t::client_info_t::client_info_t()
    : limit_clients(&opt_lt<std::string>) { }

server_t::server_t(mailbox_manager_t *_manager)
    : uuid(generate_uuid()),
      manager(_manager),
      stop_mailbox(manager, std::bind(&server_t::stop_mailbox_cb, this, ph::_1)) { }

server_t::~server_t() { }

void server_t::stop_mailbox_cb(client_t::addr_t addr) {
    auto_drainer_t::lock_t lock(&drainer);
    rwlock_in_line_t spot(&clients_lock, access_t::read);
    spot.read_signal()->wait_lazily_unordered();
    auto it = clients.find(addr);
    // The client might have already been removed from e.g. a peer disconnect or
    // drainer destruction.  (Also, if we have multiple shards per btree this
    // will be called twice, and the second time it should be a no-op.)
    if (it != clients.end()) {
        it->second.cond->pulse_if_not_already_pulsed();
    }
}

void server_t::add_client(const client_t::addr_t &addr, region_t region) {
    auto_drainer_t::lock_t lock(&drainer);
    rwlock_in_line_t spot(&clients_lock, access_t::write);
    spot.write_signal()->wait_lazily_unordered();
    auto info = &clients[addr];

    // We do this regardless of whether there's already an entry for this
    // address, because we might be subscribed to multiple regions if we're
    // oversharded.  This will have to become smarter once you can unsubscribe
    // at finer granularity (i.e. when we support changefeeds on selections).
    info->regions.push_back(std::move(region));

    // The entry might already exist if we have multiple shards per btree, but
    // that's fine.
    if (!info->cond.has()) {
        info->stamp = 0;
        cond_t *stopped = new cond_t();
        info->cond.init(stopped);
        // We spawn now so the auto drainer lock is acquired immediately.
        // Passing the raw pointer `stopped` is safe because `add_client_cb` is
        // the only function which can remove an entry from the map.
        coro_t::spawn_now_dangerously(
            std::bind(&server_t::add_client_cb, this, stopped, addr));
    }
}

void server_t::add_limit_client(
    const client_t::addr_t &addr,
    const region_t &region,
    const std::string &table,
    const uuid_u &client_uuid,
    const keyspec_t::limit_t &spec,
    limit_order_t lt,
    stream_t &&stream) {

    auto_drainer_t::lock_t lock(&drainer);
    rwlock_in_line_t spot(&clients_lock, access_t::write);
    spot.read_signal()->wait_lazily_unordered();
    auto it = clients.find(addr);

    // It's entirely possible the peer disconnected by the time we got here.
    if (it != clients.end()) {
        auto *vec = &it->second.limit_clients[spec.range.sindex];
        auto lm = make_scoped<limit_manager_t>(
            region,
            table,
            this,
            it->first,
            client_uuid,
            spec,
            std::move(lt),
            std::move(stream));
        spot.write_signal()->wait_lazily_unordered();
        vec->push_back(std::move(lm));
    }
}

void server_t::add_client_cb(signal_t *stopped, client_t::addr_t addr) {
    auto_drainer_t::lock_t coro_lock(&drainer);
    {
        disconnect_watcher_t disconnect(manager, addr.get_peer());
        wait_any_t wait_any(
            &disconnect, stopped, coro_lock.get_drain_signal());
        wait_any.wait_lazily_unordered();
    }
    rwlock_in_line_t coro_spot(&clients_lock, access_t::write);
    coro_spot.read_signal()->wait_lazily_unordered();
    auto it = clients.find(addr);
    if (it != clients.end()) {
        // We can be removed more than once safely (e.g. in the case of oversharding).
        send_one_with_lock(coro_lock, &*it, msg_t(msg_t::stop_t()));
    }
    coro_spot.write_signal()->wait_lazily_unordered();
    size_t erased = clients.erase(addr);
    // This is true even if we have multiple shards per btree because
    // `add_client` only spawns one of us.
    guarantee(erased == 1);
}

struct stamped_msg_t {
    stamped_msg_t() { }
    stamped_msg_t(uuid_u _server_uuid, uint64_t _stamp, msg_t _submsg)
        : server_uuid(std::move(_server_uuid)),
          stamp(_stamp),
          submsg(std::move(_submsg)) { }
    uuid_u server_uuid;
    uint64_t stamp;
    msg_t submsg;
};

RDB_MAKE_SERIALIZABLE_3(stamped_msg_t, server_uuid, stamp, submsg);

// This function takes a `lock_t` to make sure you have one.  (We can't just
// always ackquire a drainer lock before sending because we sometimes send a
// `stop_t` during destruction, and you can't acquire a drain lock on a draining
// `auto_drainer_t`.)
void server_t::send_one_with_lock(
    const auto_drainer_t::lock_t &,
    std::pair<const client_t::addr_t, client_info_t> *client,
    msg_t msg) {
    uint64_t stamp;
    {
        // We don't need a write lock as long as we make sure the coroutine
        // doesn't block between reading and updating the stamp.
        ASSERT_NO_CORO_WAITING;
        stamp = client->second.stamp++;
    }
    send(manager, client->first, stamped_msg_t(uuid, stamp, std::move(msg)));
}

void server_t::send_all(const msg_t &msg, const store_key_t &key) {
    auto_drainer_t::lock_t lock(&drainer);
    rwlock_in_line_t spot(&clients_lock, access_t::read);
    spot.read_signal()->wait_lazily_unordered();
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (std::any_of(it->second.regions.begin(),
                        it->second.regions.end(),
                        std::bind(&region_contains_key, ph::_1, std::cref(key)))) {
            send_one_with_lock(lock, &*it, msg);
        }
    }
}

void server_t::stop_all() {
    auto_drainer_t::lock_t lock(&drainer);
    rwlock_in_line_t spot(&clients_lock, access_t::read);
    spot.read_signal()->wait_lazily_unordered();
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        it->second.cond->pulse_if_not_already_pulsed();
    }
}

server_t::addr_t server_t::get_stop_addr() {
    return stop_mailbox.get_address();
}

uint64_t server_t::get_stamp(const client_t::addr_t &addr) {
    auto_drainer_t::lock_t lock(&drainer);
    rwlock_in_line_t spot(&clients_lock, access_t::read);
    spot.read_signal()->wait_lazily_unordered();
    auto it = clients.find(addr);
    if (it == clients.end()) {
        // The client was removed, so no future messages are coming.
        return std::numeric_limits<uint64_t>::max();
    } else {
        return it->second.stamp;
    }
}

uuid_u server_t::get_uuid() {
    return uuid;
}

void server_t::foreach_limit(const boost::optional<std::string> &sindex,
                             std::function<void(limit_manager_t *)> f) {
    auto_drainer_t::lock_t lock(&drainer);
    rwlock_in_line_t spot(&clients_lock, access_t::read);
    spot.read_signal()->wait_lazily_unordered();
    for (auto &&client : clients) {
        auto it = client.second.limit_clients.find(sindex);
        if (it != client.second.limit_clients.end()) {
            for (auto &&lc : it->second) {
                auto_drainer_t::lock_t lc_lock(&lc->drainer);
                rwlock_in_line_t lc_spot(&lc->lock, access_t::write);
                lc_spot.write_signal()->wait_lazily_unordered();
                f(lc.get());
            }
        }
    }
}

limit_order_t::limit_order_t(sorting_t _sorting)
    : sorting(std::move(_sorting)) {
    rcheck_toplevel(
        sorting != sorting_t::UNORDERED, base_exc_t::GENERIC,
        "Cannot get changes on the first elements of an unordered stream.");
}

bool limit_order_t::operator()(const datum_t &a, const datum_t &b) const {
    int cmp = a.cmp(reql_version_t::LATEST, b);
    switch (sorting) {
    case sorting_t::ASCENDING: return cmp > 0;
    case sorting_t::DESCENDING: return cmp < 0;
    case sorting_t::UNORDERED: // fallthru
    default: unreachable();
    }
    unreachable();
}

bool limit_order_t::operator()(const std::string &a, const std::string &b) const {
    switch (sorting) {
    case sorting_t::ASCENDING: return a > b;
    case sorting_t::DESCENDING: return a < b;
    case sorting_t::UNORDERED: // fallthru
    default: unreachable();
    }
    unreachable();
}

void limit_manager_t::send(msg_t &&msg) {
    auto_drainer_t::lock_t drain_lock(&parent->drainer);
    auto it = parent->clients.find(parent_client);
    guarantee(it != parent->clients.end());
    parent->send_one_with_lock(drain_lock, &*it, std::move(msg));
}

limit_manager_t::limit_manager_t(
    region_t _region,
    std::string _table,
    server_t *_parent,
    client_t::addr_t _parent_client,
    uuid_u _uuid,
    keyspec_t::limit_t _spec,
    limit_order_t _lt,
    stream_t &&stream)
    : region(std::move(_region)),
      table(std::move(_table)),
      parent(_parent),
      parent_client(std::move(_parent_client)),
      uuid(std::move(_uuid)),
      spec(std::move(_spec)),
      lt(std::move(_lt)),
      lqueue(lt) {
    for (auto &&item : stream) {
        auto keystr = key_to_unescaped_str(item.key);
        bool inserted;
        if (spec.range.sindex) {
            inserted = lqueue.insert(
                datum_t::extract_primary(keystr),
                std::move(item.sindex_key),
                std::move(item.data)).second;
        } else {
            inserted = lqueue.insert(
                keystr, key_to_datum(keystr), std::move(item.data)).second;
        }
        guarantee(inserted);
    }

    std::vector<std::pair<std::string, std::pair<datum_t, datum_t> > > v;
    for (const auto &pair : lqueue) {
        guarantee(pair->second.first.has());
        guarantee(pair->second.second.has());
        v.push_back(*pair);
    }
    send(msg_t(msg_t::limit_start_t(uuid, std::move(v))));
}


void limit_manager_t::add(std::string id, datum_t key, datum_t val) {
    debugf("%p add %s <%s,%s>\n",
           this,
           id.c_str(),
           key.print().c_str(),
           val.print().c_str());
    added.push_back(
        std::make_pair(std::move(id), std::make_pair(std::move(key), std::move(val))));
}

void limit_manager_t::del(std::string id) {
    debugf("%p add %s\n", this, id.c_str());
    deleted.push_back(std::move(id));
}

class ref_visitor_t : public boost::static_visitor<lvec_t> {
public:
    ref_visitor_t(sorting_t _sorting,
                  boost::optional<lqueue_t::iterator> _start,
                  size_t _n)
        : sorting(_sorting), start(std::move(_start)), n(_n) { }

    lvec_t operator()(const primary_ref_t &ref) {
        rget_read_response_t resp;
        key_range_t range;
        auto open = key_range_t::open;
        switch (sorting) {
        case sorting_t::ASCENDING: {
            auto kstart = start ? store_key_t((**start)->first) : store_key_t::min();
            range = key_range_t(open, kstart, open, store_key_t::max());
        } break;
        case sorting_t::DESCENDING: {
            auto kstart = start ? store_key_t((**start)->first) : store_key_t::max();
            range = key_range_t(open, store_key_t::min(), open, kstart);
        } break;
        case sorting_t::UNORDERED: // fallthru
        default: unreachable();
        }
        rdb_rget_slice(
            ref.btree,
            range,
            ref.superblock,
            ref.env,
            batchspec_t::all().with_at_most(n),
            std::vector<transform_variant_t>(),
            boost::optional<terminal_variant_t>(),
            sorting,
            &resp,
            release_superblock_t::RELEASE);
        auto *gs = boost::get<ql::grouped_t<ql::stream_t> >(&resp.result);
        if (gs == NULL) {
            auto *exc = boost::get<ql::exc_t>(&resp.result);
            guarantee(exc != NULL);
            rassert(exc == NULL); // RSI: remove
            return lvec_t();
        }
        stream_t stream = groups_to_batch(
            gs->get_underlying_map(ql::grouped::order_doesnt_matter_t()));
        guarantee(stream.size() <= n);
        lvec_t lvec;
        for (auto &&item : stream) {
            std::string keystr = key_to_unescaped_str(item.key);
            lvec.push_back(
                std::make_pair(
                    keystr,
                    std::make_pair(key_to_datum(keystr), std::move(item.data))));
        }
        return lvec;
    }

    lvec_t operator()(const sindex_ref_t &ref) {
        rget_read_response_t resp;
        datum_range_t srange;
        auto open = key_range_t::bound_t::open;
        datum_t dstart = start ? (**start)->second.first : datum_t();
        switch (sorting) {
        case sorting_t::ASCENDING:
            srange = datum_range_t(dstart, open, datum_t(), open);
            break;
        case sorting_t::DESCENDING:
            srange = datum_range_t(datum_t(), open, dstart, open);
            break;
        case sorting_t::UNORDERED: // fallthru
        default: unreachable();
        }
        rdb_rget_secondary_slice(
            ref.btree,
            srange,
            region_t(srange.to_sindex_keyrange()), // RSI: does this work?
            ref.superblock,
            ref.env,
            batchspec_t::all().with_at_most(n), // RSI: ACC_TERMINAL
            std::vector<transform_variant_t>(),
            boost::optional<terminal_variant_t>(), // RSI: ACC_TERMINAL,
            datum_range_t::universe().to_primary_keyrange(), // RSI: use restricted range
            sorting,
            *ref.sindex_info,
            &resp,
            release_superblock_t::KEEP);
        auto *gs = boost::get<ql::grouped_t<ql::stream_t> >(&resp.result);
        if (gs == NULL) {
            auto *exc = boost::get<ql::exc_t>(&resp.result);
            guarantee(exc != NULL);
            rassert(exc == NULL); // RSI: remove
            return lvec_t();
        }
        stream_t stream = groups_to_batch(
            gs->get_underlying_map(ql::grouped::order_doesnt_matter_t()));
        std::sort(stream.begin(), stream.end(),
                  [](const rget_item_t &a, const rget_item_t &b) {
                      return a.sindex_key < b.sindex_key;
                  });
        if (stream.size() > n) {
            stream.resize(n);
        }
        lvec_t lvec;
        for (auto &&item : stream) {
            lvec.push_back(
                std::make_pair(
                    datum_t::extract_primary(key_to_unescaped_str(item.key)),
                    std::make_pair(std::move(item.sindex_key), std::move(item.data))));
        }
        return lvec;
    }

private:
    sorting_t sorting;
    boost::optional<lqueue_t::iterator> start;
    size_t n;
};

// RSI: sorting
lvec_t limit_manager_t::read_more(
    const boost::variant<primary_ref_t, sindex_ref_t> &ref,
    sorting_t sorting,
    const boost::optional<lqueue_t::iterator> &start,
    size_t n) {
    debugf("~~ READ_MORE\n");
    ref_visitor_t visitor{sorting, start, n};
    return boost::apply_visitor(visitor, ref);
}

// RSI: pick up here
// * Fix big insert crash (added and then removed again).
// * name `lt` `gt` and switch ordering of PRIMARY KEY ONLY.
// RSI: do we need to include the tag in the conflict resolution in addition to
// the primary key?
void limit_manager_t::commit(
    const boost::variant<primary_ref_t, sindex_ref_t> &sindex_ref) {
    debugf("\n**********************************************************************\n");
    debugf("COMMIT (added %zu, deleted %zu)\n", added.size(), deleted.size());
    lqueue_t real_added(lt);
    std::set<std::string> real_deleted;
    for (auto &&id : deleted) {
        bool data_deleted = lqueue.del_id(id);
        if (data_deleted) {
            bool inserted = real_deleted.insert(std::move(id)).second;
            guarantee(inserted);
        }
    }
    deleted.clear();
    for (const auto &pair : added) {
        bool inserted = lqueue.insert(pair).second;
        guarantee(inserted);
        inserted = real_added.insert(pair).second;
        guarantee(inserted);
    }
    added.clear();

    debugf("real_added %zu, real_deleted %zu\n", real_added.size(), real_deleted.size());
    std::vector<std::string> truncated = lqueue.truncate(spec.limit);
    for (auto &&id : truncated) {
        auto it = real_added.find_id(id);
        if (it != real_added.end()) {
            real_added.erase(it);
        } else {
            bool inserted = real_deleted.insert(std::move(id)).second;
            guarantee(inserted);
        }
    }
    debugf("2 real_added %zu, real_deleted %zu\n",
           real_added.size(), real_deleted.size());
    if (lqueue.size() < spec.limit) {
        debugf("Expanding data...\n");
        auto data_it = lqueue.begin();
        datum_t begin = (data_it == lqueue.end()) ? datum_t() : (*data_it)->second.first;
        // RSI: need a closed bound and duplicate removal to handle truncated
        // sindexes.
        boost::optional<lqueue_t::iterator> start;
        if (data_it != lqueue.end()) {
            start = data_it;
        }
        lvec_t s = read_more(
            sindex_ref,
            spec.range.sorting,
            start,
            spec.limit - lqueue.size());
        debugf("got %zu\n", s.size());
        guarantee(s.size() <= spec.limit - lqueue.size());
        for (auto &&pair : s) {
            bool ins = lqueue.insert(pair).second;
            guarantee(ins);
            size_t erased = real_deleted.erase(pair.first);
            if (erased == 0) {
                ins = real_added.insert(pair).second;
                guarantee(ins);
            }
        }
    }
    if (auto p = boost::get<primary_ref_t>(&sindex_ref)) {
        // RSI: strongly consider changing the functions called by
        // `superblock_queue_t` to use a rwlock rather than a promise for this
        // so that read operations like `ref_visitor_t` don't block out writes.
        // (On the other hand, those read operations are rare, so maybe it isn't
        // worth the refactoring effort.)
        debugf("Releasing superblock.\n");
        p->promise->pulse(p->superblock);
    }
    debugf("3 real_added %zu, real_deleted %zu\n",
           real_added.size(), real_deleted.size());
    for (const auto &id : real_deleted) {
        msg_t::limit_change_t msg;
        msg.sub = uuid;
        msg.old_key = id;
        auto it = real_added.find_id(id);
        if (it != real_added.end()) {
            msg.new_val = std::move(**it);
            real_added.erase(it);
        }
        debugf("DEL send %s\n", msg.print().c_str());
        send(msg_t(std::move(msg)));
    }
    real_deleted.clear();
    debugf("4 real_added %zu, real_deleted %zu\n",
           real_added.size(), real_deleted.size());
    for (auto &&it : real_added) {
        msg_t::limit_change_t msg;
        msg.sub = uuid;
        msg.new_val = std::move(*it);
        debugf("ADD send %s\n", msg.print().c_str());
        send(msg_t(std::move(msg)));
    }
    real_added.clear();
    debugf("5 real_added %zu, real_deleted %zu\n",
           real_added.size(), real_deleted.size());
    debugf("\n**********************************************************************\n");
}


RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(msg_t, op);
RDB_IMPL_ME_SERIALIZABLE_2(msg_t::change_t, empty_ok(old_val), empty_ok(new_val));
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(msg_t::change_t);
RDB_IMPL_ME_SERIALIZABLE_2(msg_t::limit_start_t, sub, start_data);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(msg_t::limit_start_t);
RDB_IMPL_ME_SERIALIZABLE_3(msg_t::limit_change_t, sub, old_key, new_val);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(msg_t::limit_change_t);
RDB_IMPL_SERIALIZABLE_0_SINCE_v1_13(msg_t::stop_t);

enum class detach_t { NO, YES };

// Uses the home thread of the subscriber, not the client.
class subscription_t : public home_thread_mixin_t {
public:
    virtual ~subscription_t();
    std::vector<datum_t>
    get_els(batcher_t *batcher, const signal_t *interruptor);
    virtual void start(env_t *env,
                       std::string table,
                       namespace_interface_t *nif,
                       client_t::addr_t *addr) = 0;
    void stop(const std::string &msg, detach_t should_detach);
protected:
    explicit subscription_t(feed_t *_feed);
    void maybe_signal_cond() THROWS_NOTHING;
    void destructor_cleanup(std::function<void()> del_sub) THROWS_NOTHING;
    // If an error occurs, we're detached and `exc` is set to an exception to rethrow.
    std::exception_ptr exc;
    // If we exceed the array size limit, elements are evicted from `els` and
    // `skipped` is incremented appropriately.  If `skipped` is non-0, we send
    // an error object to the user with the number of skipped elements before
    // continuing.
    size_t skipped;
    // The feed we're subscribed to.
    feed_t *feed;
private:
    virtual bool has_el() = 0;
    virtual datum_t pop_el() = 0;
    // Used to block on more changes.  NULL unless we're waiting.
    cond_t *cond;
    auto_drainer_t drainer;
    DISABLE_COPYING(subscription_t);
};

class flat_sub_t : public subscription_t {
public:
    template<class... Args>
    flat_sub_t(Args &&... args)
        : subscription_t(std::forward<Args>(args)...) { }
    virtual void add_el(
        const uuid_u &uuid,
        uint64_t stamp,
        const datum_t &pkey_val,
        datum_t d,
        const configured_limits_t &limits) = 0;
};

class limit_sub_t;
class feed_t : public home_thread_mixin_t, public slow_atomic_countable_t<feed_t> {
public:
    feed_t(client_t *client,
           mailbox_manager_t *_manager,
           namespace_interface_t *ns_if,
           uuid_u uuid,
           std::string pkey,
           signal_t *interruptor);
    ~feed_t();

    void add_point_sub(flat_sub_t *sub, const datum_t &key) THROWS_NOTHING;
    void del_point_sub(flat_sub_t *sub, const datum_t &key) THROWS_NOTHING;

    void add_range_sub(flat_sub_t *sub) THROWS_NOTHING;
    void del_range_sub(flat_sub_t *sub) THROWS_NOTHING;

    void add_limit_sub(limit_sub_t *sub, const uuid_u &uuid) THROWS_NOTHING;
    void del_limit_sub(limit_sub_t *sub, const uuid_u &uuid) THROWS_NOTHING;

    void each_range_sub(const std::function<void(flat_sub_t *)> &f) THROWS_NOTHING;
    void each_point_sub(const std::function<void(flat_sub_t *)> &f) THROWS_NOTHING;
    void each_sub(const std::function<void(flat_sub_t *)> &f) THROWS_NOTHING;
    void on_point_sub(
        datum_t key, const std::function<void(flat_sub_t *)> &f) THROWS_NOTHING;
    void on_limit_sub(
        const uuid_u &uuid, const std::function<void(limit_sub_t *)> &f) THROWS_NOTHING;


    bool can_be_removed();
    client_t::addr_t get_addr() const;

    const std::string pkey;
private:
    void add_sub_with_lock(
        rwlock_t *rwlock, const std::function<void()> &f) THROWS_NOTHING;
    void del_sub_with_lock(
        rwlock_t *rwlock, const std::function<size_t()> &f) THROWS_NOTHING;

    template<class Sub>
    void each_sub_in_vec(
        const std::vector<std::set<Sub *> > &vec,
        rwlock_in_line_t *spot,
        const std::function<void(Sub *)> &f) THROWS_NOTHING;
    template<class Sub>
    void each_sub_in_vec_cb(const std::function<void(Sub *)> &f,
                            const std::vector<std::set<Sub *> > &vec,
                            const std::vector<int> &sub_threads,
                            int i);
    void each_point_sub_cb(const std::function<void(flat_sub_t *)> &f, int i);
    void mailbox_cb(stamped_msg_t msg);
    void constructor_cb();

    client_t *client;
    uuid_u uuid;
    mailbox_manager_t *manager;
    mailbox_t<void(stamped_msg_t)> mailbox;
    std::vector<server_t::addr_t> stop_addrs;

    std::vector<scoped_ptr_t<disconnect_watcher_t> > disconnect_watchers;

    struct queue_t {
        rwlock_t lock;
        uint64_t next;
        struct lt_t {
            bool operator()(const stamped_msg_t &left, const stamped_msg_t &right) {
                return left.stamp > right.stamp; // We want the min val to be on top.
            }
        };
        std::priority_queue<stamped_msg_t, std::vector<stamped_msg_t>, lt_t> map;
    };
    // Maps from a `server_t`'s uuid_u.  We don't need a lock for this because
    // the set of `uuid_u`s never changes after it's initialized.
    std::map<uuid_u, scoped_ptr_t<queue_t> > queues;
    cond_t queues_ready;

    std::map<datum_t,
             std::vector<std::set<flat_sub_t *> >,
             optional_datum_less_t> point_subs;
    rwlock_t point_subs_lock;

    std::vector<std::set<flat_sub_t *> > range_subs;
    rwlock_t range_subs_lock;

    std::map<uuid_u, std::vector<std::set<limit_sub_t *> > > limit_subs;
    rwlock_t limit_subs_lock;

    int64_t num_subs;
    bool detached;
    auto_drainer_t drainer;
};

class point_sub_t : public flat_sub_t {
public:
    // Throws QL exceptions.
    point_sub_t(feed_t *feed, datum_t _key)
        : flat_sub_t(feed), key(std::move(_key)), stamp(0) {
        feed->add_point_sub(this, key);
    }
    virtual ~point_sub_t() {
        destructor_cleanup(std::bind(&feed_t::del_point_sub, feed, this, key));
    }
    virtual void start(env_t *env,
                       std::string,
                       namespace_interface_t *nif,
                       client_t::addr_t *addr) {
        assert_thread();
        read_response_t read_resp;
        nif->read(
            read_t(
                changefeed_point_stamp_t(
                    *addr, store_key_t(key->print_primary())),
                profile_bool_t::DONT_PROFILE),
            &read_resp,
            order_token_t::ignore,
            env->interruptor);
        auto resp = boost::get<changefeed_point_stamp_response_t>(
            &read_resp.response);
        guarantee(resp != NULL);
        uint64_t start_stamp = resp->stamp.second;
        // It's OK to check `el.has()` because we never return the subscription
        // to anything that calls `get_els` unless the subscription has been
        // started.  We use `>` because a normal stamp that's equal to the start
        // stamp wins (the semantics are that the start stamp is the first
        // "legal" stamp).
        if (!el.has() || start_stamp > stamp) {
            stamp = start_stamp;
            el = std::move(resp->initial_val);
        }
    }
private:
    virtual void add_el(const uuid_u &,
                        uint64_t d_stamp,
                        const datum_t &pkey_val,
                        datum_t d,
                        const configured_limits_t &) {
        assert_thread();
        rassert(*pkey_val == *key);
        // We use `>=` because we might have the same stamp as the start stamp
        // (the start stamp reads the stamp non-destructively on the shards).
        // We do ordering and duplicate checking in the layer above, so apart
        // from that we have a strict ordering.
        if (d_stamp >= stamp) {
            stamp = d_stamp;
            el = std::move(d);
            maybe_signal_cond();
        }
    }
    virtual bool has_el() { return el.has(); }
    virtual datum_t pop_el() {
        guarantee(has_el());
        datum_t ret = el;
        el.reset();
        return ret;
    }
    datum_t key;
    uint64_t stamp;
    datum_t el;
};

class range_sub_t : public flat_sub_t {
public:
    // Throws QL exceptions.
    range_sub_t(feed_t *feed, keyspec_t::range_t _spec)
        : flat_sub_t(feed), spec(std::move(_spec)) {
        feed->add_range_sub(this);
    }
    virtual ~range_sub_t() {
        destructor_cleanup(std::bind(&feed_t::del_range_sub, feed, this));
    }
    virtual void start(env_t *env,
                       std::string,
                       namespace_interface_t *nif,
                       client_t::addr_t *addr) {
        assert_thread();
        read_response_t read_resp;
        nif->read(
            read_t(changefeed_stamp_t(*addr), profile_bool_t::DONT_PROFILE),
            &read_resp, order_token_t::ignore, env->interruptor);
        auto resp = boost::get<changefeed_stamp_response_t>(&read_resp.response);
        guarantee(resp != NULL);
        start_stamps = std::move(resp->stamps);
        guarantee(start_stamps.size() != 0);
    }
private:
    virtual void add_el(const uuid_u &uuid,
                        uint64_t stamp,
                        const datum_t &pkey_val,
                        datum_t d,
                        const configured_limits_t &limits) {
        if (spec.range.contains(reql_version_t::LATEST, pkey_val)) {
            // If we don't have start timestamps, we haven't started, and if we have
            // exc, we've stopped.
            if (start_stamps.size() != 0 && !exc) {
                auto it = start_stamps.find(uuid);
                guarantee(it != start_stamps.end());
                if (stamp >= it->second) {
                    els.push_back(std::move(d));
                    if (els.size() > limits.array_size_limit()) {
                        skipped += els.size();
                        els.clear();
                    }
                    maybe_signal_cond();
                }
            }
        }
    }
    virtual bool has_el() { return els.size() != 0; }
    virtual datum_t pop_el() {
        guarantee(has_el());
        datum_t ret = std::move(els.front());
        els.pop_front();
        return ret;
    }
    // The stamp (see `stamped_msg_t`) associated with our `changefeed_stamp_t`
    // read.  We use these to make sure we don't see changes from writes before
    // our subscription.
    std::map<uuid_u, uint64_t> start_stamps;
    // The queue of changes we've accumulated since the last time we were read from.
    std::deque<datum_t> els;
    keyspec_t::range_t spec;
};

class limit_sub_t : public subscription_t {
public:
    // Throws QL exceptions.
    limit_sub_t(feed_t *feed, keyspec_t::limit_t _spec)
        : subscription_t(feed),
          uuid(generate_uuid()),
          need_init(-1),
          got_init(0),
          spec(std::move(_spec)),
          lt(limit_order_t(spec.range.sorting)),
          lqueue(lt),
          active_data([this](const data_it_t &a, const data_it_t &b) {
                  return lt((*a)->second.first, (*b)->second.first)
                      ? true
                      : (lt((*b)->second.first, (*a)->second.first)
                         ? false
                         : lt((*a)->first, (*b)->first));
              }) {
        feed->add_limit_sub(this, uuid);
    }
    virtual ~limit_sub_t() {
        destructor_cleanup(std::bind(&feed_t::del_limit_sub, feed, this, uuid));
    }

    void maybe_send_start_msg() {
        debugf("need_init: %ld, got_init: %ld\n", need_init, got_init);
        if (need_init == got_init) {
            // When we later support not always returning the initial set, that
            // logic should go here.
            if (need_init == got_init) {
                for (auto &&it : active_data) {
                    els.push_back(
                        datum_t(std::map<datum_string_t, datum_t> {
                                { datum_string_t("old_val"), (*it)->second.second },
                                { datum_string_t("new_val"), (*it)->second.second } }));
                    maybe_signal_cond();
                }
            }
        }
    }

    virtual void start(env_t *env,
                       std::string table,
                       namespace_interface_t *nif,
                       client_t::addr_t *addr) {
        debugf("Starting!\n");
        assert_thread();
        read_response_t read_resp;
        nif->read(
            read_t(changefeed_limit_subscribe_t(*addr, uuid, spec, std::move(table)),
                   profile_bool_t::DONT_PROFILE),
            &read_resp, order_token_t::ignore, env->interruptor);
        auto resp = boost::get<changefeed_limit_subscribe_response_t>(
            &read_resp.response);
        if (resp == NULL) {
            auto err_resp = boost::get<rget_read_response_t>(&read_resp.response);
            guarantee(err_resp != NULL);
            auto err = boost::get<exc_t>(&err_resp->result);
            guarantee(err != NULL);
            throw *err;
        }
        guarantee(need_init == -1);
        debugf("need_init: %zu\n", resp->shards);
        need_init = resp->shards;
        guarantee(need_init > 0);
        maybe_send_start_msg();
    }

    void init(
        const std::vector<std::pair<std::string, std::pair<datum_t, datum_t> > >
            &start_data) {
        got_init += 1;
        debugf("start_data: %zu\n", start_data.size());
        for (const auto &pair : start_data) {
            debugf("%s\n%s\n%s\n",
                   pair.first.c_str(),
                   pair.second.first.print().c_str(),
                   pair.second.second.print().c_str());
            auto it = lqueue.insert(pair).first;
            // RSI: cmp
            active_data.insert(it);
            if (active_data.size() > spec.limit) {
                debugf("\nXXX\nsnapshot:\n");
                auto old_ft = active_data.begin();
                for (auto ft = active_data.begin(); ft != active_data.end(); ++ft) {
                    debugf("lt(%s, %s) = %d\n",
                           (**old_ft)->second.first.print().c_str(),
                           (**ft)->second.first.print().c_str(),
                           lt((**old_ft)->second.first, (**ft)->second.first));
                    debugf("%s\n", (**ft)->second.first.print().c_str());
                    old_ft = ft;
                }
                debugf("erasing %s\n",
                       (**active_data.begin())->second.first.print().c_str());
                size_t erased = erase1(&active_data, *active_data.begin());
                guarantee(erased == 1);
            }
            guarantee(active_data.size() == spec.limit
                      || (active_data.size() < spec.limit
                          && active_data.size() == lqueue.size()));
        }
        maybe_send_start_msg();
    };

    virtual void note_change(
        const boost::optional<std::string> &old_key,
        const boost::optional<std::pair<std::string, std::pair<datum_t, datum_t> > >
            &new_val) {
        debugf("%p note_change:\nold_key: %s\nnew_val: %s\n",
               this,
               old_key ? (*old_key).c_str() : "NONE",
               new_val
               ? strprintf("%s <%s,%s>",
                           (*new_val).first.c_str(),
                           (*new_val).second.first.print().c_str(),
                           (*new_val).second.second.print().c_str()).c_str()
               : "NONE");

        std::string s;
        for (const auto &jt : active_data) {
            s += (*jt)->first + " ";
        }
        debugf("active %s\n", s.c_str());
        s = "";
        for (const auto &jt : lqueue) {
            s += jt->first + " ";
        }
        debugf("lqueue %s\n", s.c_str());

        // RSI: queue if `need_init`.
        datum_t old_send, new_send;
        if (old_key) {
            auto it = lqueue.find_id(*old_key);
            guarantee(it != lqueue.end());
            size_t erased = erase1(&active_data, it);
            debugf("Erased %zu\n", erased);
            if (erased != 0) {
                // The old value was in the set.
                old_send = (*it)->second.second;
            }
            lqueue.erase(it);
        }
        if (new_val) {
            debugf("new_val...\n");
            auto pair = lqueue.insert(*new_val);
            auto it = pair.first;
            guarantee(pair.second);
            // RSI: cmp
            bool insert;
            if (active_data.size() == 0) {
                debugf("no cmp (empty active_data)\n");
                insert = true;
            } else {
                datum_t a = (*it)->second.first;
                guarantee(active_data.size() != 0);
                datum_t b = (**active_data.begin())->second.first;
                insert = !lt(a, b);
                debugf("cmp %s < %s = %d\n",
                       a.print().c_str(), b.print().c_str(), insert);
                debugf("%s %s\n",
                       (**active_data.begin())->second.first.print().c_str(),
                       (**active_data.crbegin())->second.first.print().c_str());
            }
            if (insert) {
                debugf("new_val active!\n");
                active_data.insert(it);
                // The new value is in the old set bounds (and thus in the set).
                new_send = (*it)->second.second;
            }
        }
        if (active_data.size() > spec.limit) {
            // The old value wasn't in the set, but the new value is, and a
            // value has to leave the set to make room.
            auto last = *active_data.begin();
            guarantee(new_send.has() && !old_send.has());
            debugf("___foo___: %s\n", (*last)->second.second.print().c_str());
            old_send = (*last)->second.second;
            erase1(&active_data, last);
        } else if (active_data.size() < spec.limit) {
            // The set is too small.
            if (new_send.has()) {
                // The set is too small because there aren't enough rows in the table.
                guarantee(active_data.size() == lqueue.size());
            } else if (active_data.size() < lqueue.size()) {
                // The set is too small because the new value wasn't in the old
                // set bounds, so we need to add the next best element.
                debugf("Plan omega.\n");
                auto it = *active_data.begin();

                guarantee(it != lqueue.begin());
                --it;
                //                ++it;
                //                guarantee(it != lqueue.end());
                debugf("%zu\n", active_data.size());
                active_data.insert(it);
                debugf("%zu\n", active_data.size());
                new_send = (*it)->second.second;
            } else {
                debugf("Plan what?\n");
            }
        }
        guarantee(active_data.size() == spec.limit
                  || active_data.size() == lqueue.size());

        if (old_send.has() || new_send.has()) {
            datum_t d =
                datum_t(std::map<datum_string_t, datum_t> {
                    { datum_string_t("old_val"),
                      old_send.has() ? old_send : datum_t::null() },
                    { datum_string_t("new_val"),
                      new_send.has() ? new_send : datum_t::null() } });
            debugf("___old_send___: %s\n", old_send.print().c_str());
            debugf("___new_send___: %s\n", new_send.print().c_str());
            debugf("___d___: %s\n", d.print().c_str());
            els.push_back(d);
            maybe_signal_cond();
        }
    }

    virtual bool has_el() { return els.size() != 0; }
    virtual datum_t pop_el() {
        guarantee(has_el());
        datum_t ret = std::move(els.front());
        els.pop_front();
        return ret;
    }


    uuid_u uuid;
    int64_t need_init, got_init;
    keyspec_t::limit_t spec;

    limit_order_t lt;
    lqueue_t lqueue;
    typedef decltype(lqueue)::iterator data_it_t;
    typedef std::function<bool(const data_it_t &, const data_it_t &)> data_it_lt_t;
    std::set<data_it_t, data_it_lt_t> active_data;

    std::deque<datum_t> els;
};

class msg_visitor_t : public boost::static_visitor<void> {
public:
    msg_visitor_t(feed_t *_feed, uuid_u _server_uuid, uint64_t _stamp)
        : feed(_feed), server_uuid(_server_uuid), stamp(_stamp) { }
    void operator()(const msg_t::limit_start_t &msg) const {
        feed->on_limit_sub(
            msg.sub, [&msg](limit_sub_t *sub) { sub->init(msg.start_data); });
    }
    void operator()(const msg_t::limit_change_t &msg) const {
        feed->on_limit_sub(
            msg.sub,
            [&msg](limit_sub_t *sub) { sub->note_change(msg.old_key, msg.new_val); });
    }
    void operator()(const msg_t::change_t &change) const {
        datum_t null = datum_t::null();
        configured_limits_t default_limits;
        std::map<datum_string_t, datum_t> obj{
            {datum_string_t("new_val"), change.new_val.has() ? change.new_val : null},
            {datum_string_t("old_val"), change.old_val.has() ? change.old_val : null}
        };
        auto val = change.new_val.has() ? change.new_val : change.old_val;
        r_sanity_check(val.has());
        auto pkey_val = val->get_field(datum_string_t(feed->pkey), NOTHROW);
        r_sanity_check(pkey_val.has());

        feed->each_range_sub(
            std::bind(&flat_sub_t::add_el,
                      ph::_1,
                      std::cref(server_uuid),
                      stamp,
                      std::cref(pkey_val),
                      datum_t(std::move(obj)),
                      default_limits));
        feed->on_point_sub(
            pkey_val,
            std::bind(&flat_sub_t::add_el,
                      ph::_1,
                      std::cref(server_uuid),
                      stamp,
                      std::cref(pkey_val),
                      change.new_val.has() ? change.new_val : datum_t::null(),
                      default_limits));
    }
    void operator()(const msg_t::stop_t &) const {
        const char *msg = "Changefeed aborted (table unavailable).";
        feed->each_sub(std::bind(&subscription_t::stop, ph::_1, msg, detach_t::NO));
    }
private:
    feed_t *feed;
    uuid_u server_uuid;
    uint64_t stamp;
};

class stream_t : public eager_datum_stream_t {
public:
    template<class... Args>
    stream_t(scoped_ptr_t<subscription_t> &&_sub, Args... args)
        : eager_datum_stream_t(std::forward<Args...>(args...)),
          sub(std::move(_sub)) { }
    virtual bool is_array() { return false; }
    virtual bool is_exhausted() const { return false; }
    virtual bool is_cfeed() const { return true; }
    virtual std::vector<datum_t>
    next_raw_batch(env_t *env, const batchspec_t &bs) {
        rcheck(bs.get_batch_type() == batch_type_t::NORMAL
               || bs.get_batch_type() == batch_type_t::NORMAL_FIRST,
               base_exc_t::GENERIC,
               "Cannot call a terminal (`reduce`, `count`, etc.) on an "
               "infinite stream (such as a changefeed).");
        batcher_t batcher = bs.to_batcher();
        return sub->get_els(&batcher, env->interruptor);
    }
private:
    scoped_ptr_t<subscription_t> sub;
};

subscription_t::subscription_t(feed_t *_feed) : skipped(0), feed(_feed), cond(NULL) {
    guarantee(feed != NULL);
}

subscription_t::~subscription_t() { }

std::vector<datum_t>
subscription_t::get_els(batcher_t *batcher, const signal_t *interruptor) {
    assert_thread();
    guarantee(cond == NULL); // Can't get while blocking.
    auto_drainer_t::lock_t lock(&drainer);
    if (!has_el() && !exc) {
        cond_t wait_for_data;
        cond = &wait_for_data;
        signal_timer_t timer;
        if (batcher->get_batch_type() == batch_type_t::NORMAL
            || batcher->get_batch_type() == batch_type_t::NORMAL_FIRST) {
            timer.start(batcher->microtime_left() / 1000);
        }
        try {
            wait_any_t any_interruptor(interruptor, &timer);
            // We don't need to wait on the drain signal because the interruptor
            // will be pulsed if we're shutting down.
            wait_interruptible(cond, &any_interruptor);
        } catch (const interrupted_exc_t &e) {
            cond = NULL;
            if (timer.is_pulsed()) {
                return std::vector<datum_t>();
            }
            throw e;
        }
        guarantee(cond == NULL);
    }

    std::vector<datum_t> v;
    if (exc) {
        std::rethrow_exception(exc);
    } else if (skipped != 0) {
        v.push_back(
            datum_t(
                std::map<datum_string_t, datum_t>{
                    {datum_string_t("error"), datum_t(
                            datum_string_t(
                                strprintf("Changefeed cache over array size limit, "
                                          "skipped %zu elements.", skipped)))}}));
        skipped = 0;
    } else {
        while (has_el() && !batcher->should_send_batch()) {
            datum_t el = pop_el();
            debugf("___EL___: %s\n", el.print().c_str());
            batcher->note_el(el);
            v.push_back(std::move(el));
        }
    }
    guarantee(v.size() != 0);
    return std::move(v);
}



void subscription_t::stop(const std::string &msg, detach_t detach) {
    assert_thread();
    if (detach == detach_t::YES) {
        feed = NULL;
    }
    exc = std::make_exception_ptr(datum_exc_t(base_exc_t::GENERIC, msg));
    maybe_signal_cond();
}

void subscription_t::maybe_signal_cond() THROWS_NOTHING {
    assert_thread();
    if (cond != NULL) {
        ASSERT_NO_CORO_WAITING;
        cond->pulse();
        cond = NULL;
    }
}

void subscription_t::destructor_cleanup(std::function<void()> del_sub) THROWS_NOTHING {
    // This error is only sent if we're getting destroyed while blocking.
    stop("Subscription destroyed (shutting down?).", detach_t::NO);
    if (feed != NULL) {
        del_sub();
    } else {
        // We only get here if we were detached.
        guarantee(exc);
    }
}

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        sorting_t, int8_t,
        sorting_t::UNORDERED, sorting_t::DESCENDING);

RDB_MAKE_SERIALIZABLE_3(keyspec_t::range_t, sindex, sorting, range);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(keyspec_t::range_t);

RDB_IMPL_ME_SERIALIZABLE_2(keyspec_t::limit_t, range, limit);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(keyspec_t::limit_t);

RDB_MAKE_SERIALIZABLE_1(keyspec_t::point_t, key);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(keyspec_t::point_t);

RDB_MAKE_SERIALIZABLE_1(keyspec_t, spec);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(keyspec_t);

void feed_t::add_sub_with_lock(
    rwlock_t *rwlock, const std::function<void()> &f) THROWS_NOTHING {
    on_thread_t th(home_thread());
    guarantee(!detached);
    num_subs += 1;
    auto_drainer_t::lock_t lock(&drainer);
    rwlock_in_line_t spot(rwlock, access_t::write);
    // TODO: In some cases there's work we could do with just the read signal.
    spot.write_signal()->wait_lazily_unordered();
    f();
}

template<class Map, class Key, class Sub>
void map_add_sub(Map *map, const Key &key, Sub *sub) THROWS_NOTHING {
    auto it = map->find(key);
    if (it == map->end()) {
        auto pair = std::make_pair(key, decltype(it->second)(get_num_threads()));
        it = map->insert(std::move(pair)).first;
    }
    (it->second)[sub->home_thread().threadnum].insert(sub);
}


void feed_t::del_sub_with_lock(
    rwlock_t *rwlock, const std::function<size_t()> &f) THROWS_NOTHING {
    on_thread_t th(home_thread());
    {
        auto_drainer_t::lock_t lock(&drainer);
        rwlock_in_line_t spot(rwlock, access_t::write);
        spot.write_signal()->wait_lazily_unordered();
        size_t erased = f();
        guarantee(erased == 1);
    }
    num_subs -= 1;
    if (num_subs == 0) {
        // It's possible that by the time we get the lock to remove the feed,
        // another subscriber might have already found the feed and subscribed.
        client->maybe_remove_feed(uuid);
    }
}

template<class Map, class Key, class Sub>
size_t map_del_sub(Map *map, const Key &key, Sub *sub) THROWS_NOTHING {
    auto subvec_it = map->find(key);
    size_t erased = (subvec_it->second)[sub->home_thread().threadnum].erase(sub);
    // If there are no more subscribers, remove the key from the map.
    auto it = subvec_it->second.begin();
    for (; it != subvec_it->second.end(); ++it) {
        if (it->size() != 0) {
            break;
        }
    }
    if (it == subvec_it->second.end()) {
        map->erase(subvec_it);
    }
    return erased;
}

// If this throws we might leak the increment to `num_subs`.
void feed_t::add_point_sub(flat_sub_t *sub, const datum_t &key) THROWS_NOTHING {
    add_sub_with_lock(&point_subs_lock, [this, sub, &key]() {
            map_add_sub(&point_subs, key, sub);
        });
}

// Can't throw because it's called in a destructor.
void feed_t::del_point_sub(flat_sub_t *sub, const datum_t &key) THROWS_NOTHING {
    del_sub_with_lock(&point_subs_lock, [this, sub, &key]() {
            return map_del_sub(&point_subs, key, sub);
        });
}

// If this throws we might leak the increment to `num_subs`.
void feed_t::add_range_sub(flat_sub_t *sub) THROWS_NOTHING {
    add_sub_with_lock(&range_subs_lock, [this, sub]() {
            range_subs[sub->home_thread().threadnum].insert(sub);
        });
}

// Can't throw because it's called in a destructor.
void feed_t::del_range_sub(flat_sub_t *sub) THROWS_NOTHING {
    del_sub_with_lock(&range_subs_lock, [this, sub]() {
            return range_subs[sub->home_thread().threadnum].erase(sub);
        });
}

// If this throws we might leak the increment to `num_subs`.
void feed_t::add_limit_sub(limit_sub_t *sub, const uuid_u &sub_uuid) THROWS_NOTHING {
    add_sub_with_lock(&limit_subs_lock, [this, sub, &sub_uuid]() {
            map_add_sub(&limit_subs, sub_uuid, sub);
        });
}

// Can't throw because it's called in a destructor.
void feed_t::del_limit_sub(limit_sub_t *sub, const uuid_u &sub_uuid) THROWS_NOTHING {
    del_sub_with_lock(&limit_subs_lock, [this, sub, &sub_uuid]() {
            return map_del_sub(&limit_subs, sub_uuid, sub);
        });
}

template<class Sub>
void feed_t::each_sub_in_vec(
    const std::vector<std::set<Sub *> > &vec,
    rwlock_in_line_t *spot,
    const std::function<void(Sub *)> &f) THROWS_NOTHING {
    assert_thread();
    auto_drainer_t::lock_t lock(&drainer);
    spot->read_signal()->wait_lazily_unordered();

    std::vector<int> subscription_threads;
    for (int i = 0; i < get_num_threads(); ++i) {
        if (vec[i].size() != 0) {
            subscription_threads.push_back(i);
        }
    }
    pmap(subscription_threads.size(),
         std::bind(&feed_t::each_sub_in_vec_cb<Sub>,
                   this,
                   std::cref(f),
                   std::cref(vec),
                   std::cref(subscription_threads),
                   ph::_1));
}

template<class Sub>
void feed_t::each_sub_in_vec_cb(const std::function<void(Sub *)> &f,
                                const std::vector<std::set<Sub *> > &vec,
                                const std::vector<int> &subscription_threads,
                                int i) {
    guarantee(vec[subscription_threads[i]].size() != 0);
    on_thread_t th((threadnum_t(subscription_threads[i])));
    for (Sub *sub : vec[subscription_threads[i]]) {
        f(sub);
    }
}

void feed_t::each_range_sub(
    const std::function<void(flat_sub_t *)> &f) THROWS_NOTHING {
    assert_thread();
    rwlock_in_line_t spot(&range_subs_lock, access_t::read);
    each_sub_in_vec(range_subs, &spot, f);
}

void feed_t::each_point_sub(
    const std::function<void(flat_sub_t *)> &f) THROWS_NOTHING {
    assert_thread();
    rwlock_in_line_t spot(&point_subs_lock, access_t::read);
    pmap(get_num_threads(),
         std::bind(&feed_t::each_point_sub_cb,
                   this,
                   std::cref(f),
                   ph::_1));
}

void feed_t::each_point_sub_cb(const std::function<void(flat_sub_t *)> &f, int i) {
    on_thread_t th((threadnum_t(i)));
    for (auto const &pair : point_subs) {
        for (flat_sub_t *sub : pair.second[i]) {
            f(sub);
        }
    }
}

void feed_t::each_sub(const std::function<void(flat_sub_t *)> &f) THROWS_NOTHING {
    each_range_sub(f);
    each_point_sub(f);
}

void feed_t::on_point_sub(
    datum_t key,
    const std::function<void(flat_sub_t *)> &f) THROWS_NOTHING {
    assert_thread();
    auto_drainer_t::lock_t lock(&drainer);
    rwlock_in_line_t spot(&point_subs_lock, access_t::read);
    spot.read_signal()->wait_lazily_unordered();

    auto point_sub = point_subs.find(key);
    if (point_sub != point_subs.end()) {
        each_sub_in_vec(point_sub->second, &spot, f);
    }
}

void feed_t::on_limit_sub(
    const uuid_u &sub_uuid, const std::function<void(limit_sub_t *)> &f) THROWS_NOTHING {
    assert_thread();
    auto_drainer_t::lock_t lock(&drainer);
    rwlock_in_line_t spot(&limit_subs_lock, access_t::read);
    spot.read_signal()->wait_lazily_unordered();

    auto limit_sub = limit_subs.find(sub_uuid);
    if (limit_sub != limit_subs.end()) {
        each_sub_in_vec(limit_sub->second, &spot, f);
    }
}

bool feed_t::can_be_removed() {
    return num_subs == 0;
}

client_t::addr_t feed_t::get_addr() const {
    return mailbox.get_address();
}

void feed_t::mailbox_cb(stamped_msg_t msg) {
    // We stop receiving messages when detached (we're only receiving
    // messages because we haven't managed to get a message to the
    // stop mailboxes for some of the masters yet).  This also stops
    // us from trying to handle a message while waiting on the auto
    // drainer.
    if (!detached) {
        auto_drainer_t::lock_t lock(&drainer);

        // We wait for the write to complete and the queues to be ready.
        wait_any_t wait_any(&queues_ready, lock.get_drain_signal());
        wait_any.wait_lazily_unordered();
        if (!lock.get_drain_signal()->is_pulsed()) {
            // We don't need a lock for this because the set of `uuid_u`s never
            // changes after it's initialized.
            auto it = queues.find(msg.server_uuid);
            guarantee(it != queues.end());
            queue_t *queue = it->second.get();
            guarantee(queue != NULL);

            rwlock_in_line_t spot(&queue->lock, access_t::write);
            spot.write_signal()->wait_lazily_unordered();

            // Add us to the queue.
            guarantee(msg.stamp >= queue->next);
            queue->map.push(std::move(msg));

            // Read as much as we can from the queue (this enforces ordering.)
            while (queue->map.size() != 0 && queue->map.top().stamp == queue->next) {
                const stamped_msg_t &curmsg = queue->map.top();
                msg_visitor_t visitor(this, curmsg.server_uuid, curmsg.stamp);
                boost::apply_visitor(visitor, curmsg.submsg.op);
                queue->map.pop();
                queue->next += 1;
            }
        }
    }
}

/* This mustn't hold onto the `namespace_interface_t` after it returns */
feed_t::feed_t(client_t *_client,
               mailbox_manager_t *_manager,
               namespace_interface_t *ns_if,
               uuid_u _uuid,
               std::string _pkey,
               signal_t *interruptor)
    : pkey(std::move(_pkey)),
      client(_client),
      uuid(_uuid),
      manager(_manager),
      mailbox(manager, std::bind(&feed_t::mailbox_cb, this, ph::_1)),
      /* We only use comparison in the point_subs map for equality purposes, not
         ordering -- and this isn't in a secondary index function.  Thus
         reql_version_t::LATEST is appropriate. */
      point_subs(optional_datum_less_t(reql_version_t::LATEST)),
      range_subs(get_num_threads()),
      num_subs(0),
      detached(false) {
    read_t read(changefeed_subscribe_t(mailbox.get_address()),
                profile_bool_t::DONT_PROFILE);
    read_response_t read_resp;
    ns_if->read(read, &read_resp, order_token_t::ignore, interruptor);
    auto resp = boost::get<changefeed_subscribe_response_t>(&read_resp.response);

    guarantee(resp != NULL);
    stop_addrs.reserve(resp->addrs.size());
    for (auto it = resp->addrs.begin(); it != resp->addrs.end(); ++it) {
        stop_addrs.push_back(std::move(*it));
    }

    std::set<peer_id_t> peers;
    for (auto it = stop_addrs.begin(); it != stop_addrs.end(); ++it) {
        peers.insert(it->get_peer());
    }
    for (auto it = peers.begin(); it != peers.end(); ++it) {
        disconnect_watchers.push_back(
            make_scoped<disconnect_watcher_t>(manager, *it));
    }

    for (auto it = resp->server_uuids.begin(); it != resp->server_uuids.end(); ++it) {
        auto res = queues.insert(
            std::make_pair(*it, make_scoped<queue_t>()));
        guarantee(res.second);
        res.first->second->next = 0;

        // In debug mode we put some junk messages in the queues to make sure
        // the queue logic actually does something (since mailboxes are
        // generally ordered right now).
#ifndef NDEBUG
        for (size_t i = 0; i < queues.size()-1; ++i) {
            res.first->second->map.push(
                stamped_msg_t(
                    *it,
                    std::numeric_limits<uint64_t>::max() - i,
                    msg_t()));
        }
#endif
    }
    queues_ready.pulse();

    // We spawn now so that the auto drainer lock is acquired immediately.
    coro_t::spawn_now_dangerously(std::bind(&feed_t::constructor_cb, this));
}

void feed_t::constructor_cb() {
    auto_drainer_t::lock_t lock(&drainer);
    {
        wait_any_t any_disconnect;
        for (size_t i = 0; i < disconnect_watchers.size(); ++i) {
            any_disconnect.add(disconnect_watchers[i].get());
        }
        wait_any_t wait_any(&any_disconnect, lock.get_drain_signal());
        wait_any.wait_lazily_unordered();
    }
    // Clear the disconnect watchers so we don't keep the watched connections open
    // longer than necessary.
    disconnect_watchers.clear();
    if (!detached) {
        scoped_ptr_t<feed_t> self = client->detach_feed(uuid);
        detached = true;
        if (self.has()) {
            const char *msg = "Disconnected from peer.";
            each_sub(std::bind(&subscription_t::stop, ph::_1, msg, detach_t::YES));
            num_subs = 0;
        } else {
            // We only get here if we were removed before we were detached.
            guarantee(num_subs == 0);
        }
    }
}

feed_t::~feed_t() {
    guarantee(num_subs == 0);
    detached = true;
    for (auto it = stop_addrs.begin(); it != stop_addrs.end(); ++it) {
        send(manager, *it, mailbox.get_address());
    }
}

client_t::client_t(
        mailbox_manager_t *_manager,
        const std::function<
            namespace_interface_access_t(
                const namespace_id_t &,
                signal_t *)
            > &_namespace_source) :
    manager(_manager),
    namespace_source(_namespace_source)
{
    guarantee(manager != NULL);
}
client_t::~client_t() { }

counted_t<datum_stream_t>
client_t::new_feed(env_t *env,
                   const namespace_id_t &uuid,
                   const protob_t<const Backtrace> &bt,
                   const std::string &table_name,
                   const std::string &pkey,
                   const keyspec_t &keyspec) {
    try {
        scoped_ptr_t<subscription_t> sub;
        boost::variant<scoped_ptr_t<range_sub_t>, scoped_ptr_t<point_sub_t> > presub;
        addr_t addr;
        {
            threadnum_t old_thread = get_thread_id();
            cross_thread_signal_t interruptor(env->interruptor, home_thread());
            on_thread_t th(home_thread());
            auto_drainer_t::lock_t lock(&drainer);
            rwlock_in_line_t spot(&feeds_lock, access_t::write);
            spot.read_signal()->wait_lazily_unordered();
            auto feed_it = feeds.find(uuid);
            if (feed_it == feeds.end()) {
                spot.write_signal()->wait_lazily_unordered();
                namespace_interface_access_t access =
                        namespace_source(uuid, &interruptor);
                // Even though we have the user's feed here, multiple
                // users may share a feed_t, and this code path will
                // only be run for the first one.  Rather than mess
                // about, just use the defaults.
                auto val = make_scoped<feed_t>(
                    this, manager, access.get(), uuid, pkey, &interruptor);
                feed_it = feeds.insert(std::make_pair(uuid, std::move(val))).first;
            }

            // We need to do this while holding `feeds_lock` to make sure the
            // feed isn't destroyed before we subscribe to it.
            on_thread_t th2(old_thread);
            feed_t *feed = feed_it->second.get();
            addr = feed->get_addr();

            struct keyspec_visitor_t : public boost::static_visitor<subscription_t *> {
                explicit keyspec_visitor_t(feed_t *_feed) : feed(_feed) { }
                subscription_t *operator()(const keyspec_t::range_t &range) const {
                    return new range_sub_t(feed, range);
                }
                subscription_t *operator()(const keyspec_t::limit_t &limit) const {
                    debugf("limit index `%s`\n", opt_or(limit.range.sindex, "").c_str());
                    return new limit_sub_t(feed, limit);
                }
                subscription_t *operator()(const keyspec_t::point_t &point) const {
                    return new point_sub_t(feed, point.key);
                }
                feed_t *feed;
            };
            sub.init(boost::apply_visitor(keyspec_visitor_t(feed), keyspec.spec));
        }
        namespace_interface_access_t access = namespace_source(uuid, env->interruptor);
        sub->start(env, table_name, access.get(), &addr);
        return make_counted<stream_t>(std::move(sub), bt);
    } catch (const cannot_perform_query_exc_t &e) {
        rfail_datum(base_exc_t::GENERIC,
                    "cannot subscribe to table `%s`: %s",
                    table_name.c_str(), e.what());
    }
}

void client_t::maybe_remove_feed(const uuid_u &uuid) {
    assert_thread();
    scoped_ptr_t<feed_t> destroy;
    auto_drainer_t::lock_t lock(&drainer);
    rwlock_in_line_t spot(&feeds_lock, access_t::write);
    spot.write_signal()->wait_lazily_unordered();
    auto feed_it = feeds.find(uuid);
    // The feed might have disappeared because it may have been detached while
    // we held the lock, in which case we don't need to do anything.  The feed
    // might also have gotten a new subscriber, in which case we don't want to
    // remove it yet.
    if (feed_it != feeds.end() && feed_it->second->can_be_removed()) {
        // We want to destroy the feed after the lock is released, because it
        // may be expensive.
        destroy.swap(feed_it->second);
        feeds.erase(feed_it);
    }
}

scoped_ptr_t<feed_t> client_t::detach_feed(const uuid_u &uuid) {
    assert_thread();
    scoped_ptr_t<feed_t> ret;
    auto_drainer_t::lock_t lock(&drainer);
    rwlock_in_line_t spot(&feeds_lock, access_t::write);
    spot.write_signal()->wait_lazily_unordered();
    // The feed might have been removed in `maybe_remove_feed`, in which case
    // there's nothing to detach.
    auto feed_it = feeds.find(uuid);
    if (feed_it != feeds.end()) {
        ret.swap(feed_it->second);
        feeds.erase(feed_it);
    }
    return ret;
}

} // namespace changefeed
} // namespace ql
