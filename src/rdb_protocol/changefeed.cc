#include "rdb_protocol/changefeed.hpp"

#include <queue>

#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/interruptor.hpp"
#include "containers/archive/boost_types.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/val.hpp"

#include "debug.hpp"

namespace ql {

namespace changefeed {

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

void server_t::add_client(const client_t::addr_t &addr) {
    auto_drainer_t::lock_t lock(&drainer);
    rwlock_in_line_t spot(&clients_lock, access_t::write);
    spot.write_signal()->wait_lazily_unordered();
    auto info = &clients[addr];
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

void server_t::add_client_cb(signal_t *stopped, client_t::addr_t addr) {
    auto_drainer_t::lock_t coro_lock(&drainer);
    disconnect_watcher_t disconnect(
        manager->get_connectivity_service(), addr.get_peer());
    {
        wait_any_t wait_any(
            &disconnect, stopped, coro_lock.get_drain_signal());
        wait_any.wait_lazily_unordered();
    }
    send_all_with_lock(coro_lock, msg_t(msg_t::stop_t()));
    rwlock_in_line_t coro_spot(&clients_lock, access_t::write);
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
void server_t::send_all_with_lock(const auto_drainer_t::lock_t &, msg_t msg) {
    rwlock_in_line_t spot(&clients_lock, access_t::read);
    spot.read_signal()->wait_lazily_unordered();
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        uint64_t stamp;
        {
            // We don't need a write lock as long as we make sure the coroutine
            // doesn't block between reading and updating the stamp.
            ASSERT_NO_CORO_WAITING;
            stamp = it->second.stamp++;
        }
        send(manager, it->first, stamped_msg_t(uuid, stamp, msg));
    }
}

void server_t::send_all(msg_t msg) {
    auto_drainer_t::lock_t lock(&drainer);
    send_all_with_lock(lock, std::move(msg));
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

msg_t::msg_t(msg_t &&msg) : op(std::move(msg.op)) { }
msg_t::msg_t(stop_t &&_op) : op(std::move(_op)) { }
msg_t::msg_t(change_t &&_op) : op(std::move(_op)) { }

msg_t::change_t::change_t() { }
msg_t::change_t::change_t(counted_t<const datum_t> _old_val,
                          counted_t<const datum_t> _new_val)
    : old_val(std::move(_old_val)), new_val(std::move(_new_val)) { }
msg_t::change_t::~change_t() { }

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(msg_t, op);
RDB_IMPL_ME_SERIALIZABLE_2_SINCE_v1_13(msg_t::change_t, empty_ok(old_val), empty_ok(new_val));
RDB_IMPL_SERIALIZABLE_0_SINCE_v1_13(msg_t::stop_t);

enum class detach_t { NO, YES };

// Uses the home thread of the subscriber, not the client.
class subscription_t : public home_thread_mixin_t {
public:
    // Throws QL exceptions.
    explicit subscription_t(feed_t *_feed);
    ~subscription_t();
    std::vector<counted_t<const datum_t> >
    get_els(batcher_t *batcher, const signal_t *interruptor);
    void add_el(const uuid_u &uuid, uint64_t stamp, counted_t<const datum_t> d);
    void start(std::map<uuid_u, uint64_t> &&_start_stamps);
    void stop(const std::string &msg, detach_t should_detach);
private:
    void maybe_signal_cond() THROWS_NOTHING;
    // If an error occurs, we're detached and `exc` is set to an exception to rethrow.
    std::exception_ptr exc;
    // If we exceed the array size limit, elements are evicted from `els` and
    // `skipped` is incremented appropriately.  If `skipped` is non-0, we send
    // an error object to the user with the number of skipped elements before
    // continuing.
    size_t skipped;
    // Used to block on more changes.  NULL unless we're waiting.
    cond_t *cond;
    // The queue of changes we've accumulated since the last time we were read from.
    std::deque<counted_t<const datum_t> > els;
    // The feed we're subscribed to.
    feed_t *feed;
    // The stamp (see `stamped_msg_t`) associated with our `changefeed_stamp_t`
    // read.  We use these to make sure we don't see changes from writes before
    // our subscription.
    std::map<uuid_u, uint64_t> start_stamps;
    auto_drainer_t drainer;
    DISABLE_COPYING(subscription_t);
};

class feed_t : public home_thread_mixin_t, public slow_atomic_countable_t<feed_t> {
public:
    feed_t(client_t *client,
           mailbox_manager_t *manager,
           base_namespace_repo_t *ns_repo,
           uuid_u uuid,
           signal_t *interruptor);
    ~feed_t();
    void add_sub(subscription_t *sub) THROWS_NOTHING;
    void del_sub(subscription_t *sub) THROWS_NOTHING;
    void each_sub(const std::function<void(subscription_t *)> &f) THROWS_NOTHING;
    bool can_be_removed();
    client_t::addr_t get_addr() const;
private:
    void each_subscription_cb(const std::function<void(subscription_t *)> &f,
                     const std::vector<int> &subscription_threads,
                     int i);
    void mailbox_cb(stamped_msg_t msg);
    void constructor_cb();

    client_t *client;
    uuid_u uuid;
    mailbox_manager_t *manager;
    mailbox_t<void(stamped_msg_t)> mailbox;
    std::vector<server_t::addr_t> stop_addrs;

    std::vector<scoped_ptr_t<disconnect_watcher_t> > disconnect_watchers;
    wait_any_t any_disconnect;

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

    std::vector<std::set<subscription_t *> > subs;
    rwlock_t subs_lock;
    int64_t num_subs;

    bool detached;

    auto_drainer_t drainer;
};

class msg_visitor_t : public boost::static_visitor<void> {
public:
    msg_visitor_t(feed_t *_feed, uuid_u _server_uuid, uint64_t _stamp)
        : feed(_feed), server_uuid(_server_uuid), stamp(_stamp) { }
    void operator()(const msg_t::change_t &change) const {
        auto null = make_counted<const datum_t>(datum_t::R_NULL);
        std::map<std::string, counted_t<const datum_t> > obj{
            {"new_val", change.new_val.has() ? change.new_val : null},
            {"old_val", change.old_val.has() ? change.old_val : null}
        };
        auto d = make_counted<const datum_t>(std::move(obj));
        feed->each_sub(
            std::bind(&subscription_t::add_el,
                      ph::_1,
                      std::cref(server_uuid),
                      stamp,
                      d));
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
    virtual std::vector<counted_t<const datum_t> >
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

subscription_t::subscription_t(feed_t *_feed) : skipped(0), cond(NULL), feed(_feed) {
    guarantee(feed != NULL);
    feed->add_sub(this);
}

subscription_t::~subscription_t() {
    // This error is only sent if we're getting destroyed while blocking.
    stop("Subscription destroyed (shutting down?).", detach_t::NO);
    if (feed != NULL) {
        feed->del_sub(this);
    } else {
        // We only get here if we were detached.
        guarantee(exc);
    }
}

std::vector<counted_t<const datum_t> >
subscription_t::get_els(batcher_t *batcher, const signal_t *interruptor) {
    assert_thread();
    guarantee(cond == NULL); // Can't get while blocking.
    auto_drainer_t::lock_t lock(&drainer);
    if (els.size() == 0 && !exc) {
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
                return std::vector<counted_t<const datum_t> >();
            }
            throw e;
        }
        guarantee(cond == NULL);
    }

    std::vector<counted_t<const datum_t> > v;
    if (exc) {
        std::rethrow_exception(exc);
    } else if (skipped != 0) {
        v.push_back(
            make_counted<const datum_t>(
                std::map<std::string, counted_t<const datum_t> >{
                    {"error", make_counted<const datum_t>(
                            strprintf("Changefeed cache over array size limit, "
                                      "skipped %zu elements.", skipped))}}));
        skipped = 0;
    } else {
        while (els.size() > 0 && !batcher->should_send_batch()) {
            batcher->note_el(els.front());
            v.push_back(std::move(els.front()));
            els.pop_front();
        }
    }
    guarantee(v.size() != 0);
    return std::move(v);
}

void subscription_t::add_el(
    const uuid_u &uuid, uint64_t stamp, counted_t<const datum_t> d) {
    assert_thread();
    // If we don't have start timestamps, we haven't started, and if we have
    // exc, we've stopped.
    if (start_stamps.size() != 0 && !exc) {
        auto it = start_stamps.find(uuid);
        guarantee(it != start_stamps.end());
        if (stamp >= it->second) {
            els.push_back(d);
            if (els.size() > array_size_limit()) {
                skipped += els.size();
                els.clear();
            }
            maybe_signal_cond();
        }
    }
}

void subscription_t::start(std::map<uuid_u, uint64_t> &&_start_stamps) {
    assert_thread();
    start_stamps = std::move(_start_stamps);
    guarantee(start_stamps.size() != 0);
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

// If this throws we might leak the increment to `num_subs`.
void feed_t::add_sub(subscription_t *sub) THROWS_NOTHING {
    on_thread_t th(home_thread());
    guarantee(!detached);
    num_subs += 1;
    auto_drainer_t::lock_t lock(&drainer);
    rwlock_in_line_t spot(&subs_lock, access_t::write);
    spot.write_signal()->wait_lazily_unordered();
    subs[sub->home_thread().threadnum].insert(sub);
}
// Can't throw because it's called in a destructor.
void feed_t::del_sub(subscription_t *sub) THROWS_NOTHING {
    on_thread_t th(home_thread());
    {
        auto_drainer_t::lock_t lock(&drainer);
        rwlock_in_line_t spot(&subs_lock, access_t::write);
        spot.write_signal()->wait_lazily_unordered();
        size_t erased = subs[sub->home_thread().threadnum].erase(sub);
        guarantee(erased == 1);
    }
    num_subs -= 1;
    if (num_subs == 0) {
        // It's possible that by the time we get the lock to remove the feed,
        // another subscriber might have already found the feed and subscribed.
        client->maybe_remove_feed(uuid);
    }
}

void feed_t::each_sub(const std::function<void(subscription_t *)> &f) THROWS_NOTHING {
    assert_thread();
    auto_drainer_t::lock_t lock(&drainer);
    rwlock_in_line_t spot(&subs_lock, access_t::read);
    spot.read_signal()->wait_lazily_unordered();

    std::vector<int> subscription_threads;
    for (int i = 0; i < get_num_threads(); ++i) {
        if (subs[i].size() != 0) {
            subscription_threads.push_back(i);
        }
    }
    pmap(subscription_threads.size(),
         std::bind(&feed_t::each_subscription_cb,
                   this,
                   std::cref(f),
                   std::cref(subscription_threads),
                   ph::_1));
}
void feed_t::each_subscription_cb(const std::function<void(subscription_t *)> &f,
                         const std::vector<int> &subscription_threads,
                         int i) {
    std::set<subscription_t *> *set = &subs[subscription_threads[i]];
    guarantee(set->size() != 0);
    on_thread_t th((threadnum_t(subscription_threads[i])));
    for (auto it = set->begin(); it != set->end(); ++it) {
        f(*it);
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

feed_t::feed_t(client_t *_client,
               mailbox_manager_t *_manager,
               base_namespace_repo_t *ns_repo,
               uuid_u _uuid,
               signal_t *interruptor)
    : client(_client),
      uuid(_uuid),
      manager(_manager),
      mailbox(manager, std::bind(&feed_t::mailbox_cb, this, ph::_1)),
      subs(get_num_threads()),
      num_subs(0),
      detached(false) {
    base_namespace_repo_t::access_t access(ns_repo, uuid, interruptor);
    namespace_interface_t *nif = access.get_namespace_if();
    read_t read(changefeed_subscribe_t(mailbox.get_address()),
                profile_bool_t::DONT_PROFILE);
    read_response_t read_resp;
    nif->read(read, &read_resp, order_token_t::ignore, interruptor);
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
            make_scoped<disconnect_watcher_t>(
                manager->get_connectivity_service(), *it));
        any_disconnect.add(&*disconnect_watchers.back());
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
    wait_any_t wait_any(&any_disconnect, lock.get_drain_signal());
    wait_any.wait_lazily_unordered();
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

client_t::client_t(mailbox_manager_t *_manager)
  : manager(_manager) {
    guarantee(manager != NULL);
}
client_t::~client_t() { }

counted_t<datum_stream_t>
client_t::new_feed(const counted_t<table_t> &tbl, env_t *env) {
    try {
        uuid_u uuid = tbl->get_uuid();
        scoped_ptr_t<subscription_t> sub;
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
                auto val = make_scoped<feed_t>(
                        this, manager, env->ns_repo(), uuid, &interruptor);
                feed_it = feeds.insert(std::make_pair(uuid, std::move(val))).first;
            }

            // We need to do this while holding `feeds_lock` to make sure the
            // feed isn't destroyed before we subscribe to it.
            on_thread_t th2(old_thread);
            feed_t *feed = feed_it->second.get();
            addr = feed->get_addr();
            sub.init(new subscription_t(feed));
        }
        base_namespace_repo_t::access_t access(env->ns_repo(), uuid, env->interruptor);
        namespace_interface_t *nif = access.get_namespace_if();
        read_t read(changefeed_stamp_t(addr), profile_bool_t::DONT_PROFILE);
        read_response_t read_resp;
        nif->read(read, &read_resp, order_token_t::ignore, env->interruptor);
        auto resp = boost::get<changefeed_stamp_response_t>(&read_resp.response);
        guarantee(resp != NULL);
        sub->start(std::move(resp->stamps));
        return make_counted<stream_t>(std::move(sub), tbl->backtrace());
    } catch (const cannot_perform_query_exc_t &e) {
        rfail_datum(ql::base_exc_t::GENERIC,
                    "cannot subscribe to table `%s`: %s",
                    tbl->name.c_str(), e.what());
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
