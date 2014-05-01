#include "rdb_protocol/changefeed.hpp"

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
      stop_mailbox(
          manager,
          [this](client_t::addr_t addr) {
              auto_drainer_t::lock_t lock(&drainer);
              rwlock_in_line_t spot(&clients_lock, access_t::write);
              spot.read_signal()->wait_lazily_unordered();
              auto it = clients.find(addr);
              // The client might have already been removed from e.g. a peer
              // disconnect or drainer destruction.
              if (it != clients.end()) {
                  auto pair = &it->second;
                  spot.write_signal()->wait_lazily_unordered();
                  pair->first -= 1;
                  if (pair->first == 0) {
                      // This won't get pulsed twice because the clients can't
                      // send to this mailbox until the original subscribing write
                      // has completed, which means `add_client` has been called
                      // with `addr` as many times as it's ever going to be, so
                      // we'll never hit `0` more than once.
                      pair->second->pulse();
                  }
              }
          }
      ) { }

void server_t::add_client(const client_t::addr_t &addr) {
    auto_drainer_t::lock_t lock(&drainer);
    rwlock_in_line_t spot(&clients_lock, access_t::write);
    spot.write_signal()->wait_lazily_unordered();
    auto pair = &clients[addr];
    if (pair->second.has()) {
        pair->first += 1;
    } else {
        pair->first = 1;
        pair->second.init(new cond_t());
        signal_t *stopped = &*pair->second;
        // We spawn now so the auto drainer lock is acquired immediately.
        coro_t::spawn_now_dangerously(
            [this, stopped, addr]() {
                auto_drainer_t::lock_t coro_lock(&drainer);
                disconnect_watcher_t disconnect(
                    manager->get_connectivity_service(), addr.get_peer());
                {
                    wait_any_t wait_any(
                        &disconnect, stopped, coro_lock.get_drain_signal());
                    wait_any.wait_lazily_unordered();
                }
                debugf("Stopping...\n");
                rwlock_in_line_t coro_spot(&clients_lock, access_t::write);
                coro_spot.write_signal()->wait_lazily_unordered();
                size_t erased = clients.erase(addr);
                guarantee(erased == 1);
                debugf("Stopped!\n");
            }
        );
    }
}

struct stamped_msg_t {
    stamped_msg_t() { }
    stamped_msg_t(uuid_u _server_uuid, repli_timestamp_t _timestamp, msg_t _submsg)
        : server_uuid(std::move(_server_uuid)),
          timestamp(std::move(_timestamp)),
          submsg(std::move(_submsg)) { }
    uuid_u server_uuid;
    repli_timestamp_t timestamp;
    msg_t submsg;
    RDB_DECLARE_ME_SERIALIZABLE;
};

void server_t::send_all(repli_timestamp_t timestamp, msg_t msg) {
    auto_drainer_t::lock_t lock(&drainer);
    rwlock_in_line_t spot(&clients_lock, access_t::read);
    spot.read_signal()->wait_lazily_unordered();
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        send(manager, it->first,
             stamped_msg_t(uuid, std::move(timestamp), std::move(msg)));
    }
}

server_t::addr_t server_t::get_stop_addr() {
    return stop_mailbox.get_address();
}

uuid_u server_t::get_uuid() {
    return uuid;
}

msg_t::msg_t(msg_t &&msg) : op(std::move(msg.op)) { }
msg_t::msg_t(const msg_t &msg) : op(msg.op) { }
msg_t::msg_t(stop_t &&_op) : op(std::move(_op)) { }
msg_t::msg_t(change_t &&_op) : op(std::move(_op)) { }

msg_t::change_t::change_t() { }
msg_t::change_t::change_t(const rdb_modification_report_t *report)
  : old_val(report->info.deleted.first), new_val(report->info.added.first) { }
msg_t::change_t::~change_t() { }

RDB_IMPL_ME_SERIALIZABLE_1(msg_t, op);
RDB_IMPL_ME_SERIALIZABLE_2(msg_t::change_t, empty_ok(old_val), empty_ok(new_val));
RDB_IMPL_ME_SERIALIZABLE_0(msg_t::stop_t);
RDB_IMPL_ME_SERIALIZABLE_3(stamped_msg_t, server_uuid, timestamp, submsg);

enum class detach_t { NO, YES };

// Uses the home thread of the subscriber, not the client.
class sub_t : public home_thread_mixin_t {
public:
    // Throws QL exceptions.
    sub_t(feed_t *_feed);
    ~sub_t();
    std::vector<counted_t<const datum_t> >
    get_els(batcher_t *batcher, const signal_t *interruptor);
    void add_el(const uuid_u &uuid, const repli_timestamp_t &timestamp,
                counted_t<const datum_t> d);
    void start(std::map<uuid_u, repli_timestamp_t> &&_start_timestamps);
    void abort(const std::string &msg, detach_t detach = detach_t::NO);
private:
    void maybe_signal_cond() THROWS_NOTHING;
    std::exception_ptr exc;
    cond_t *cond; // NULL unless we're waiting.
    std::deque<counted_t<const datum_t> > els;
    feed_t *feed;
    std::map<uuid_u, repli_timestamp_t> start_timestamps;
    auto_drainer_t drainer;
    DISABLE_COPYING(sub_t);
};

class feed_t : public home_thread_mixin_t, public slow_atomic_countable_t<feed_t> {
public:
    feed_t(client_t *client,
           base_namespace_repo_t *ns_repo,
           uuid_u uuid,
           signal_t *interruptor);
    ~feed_t();
    void add_sub(sub_t *sub) THROWS_NOTHING;
    void del_sub(sub_t *sub) THROWS_NOTHING;
    template<class T> void each_sub(const T &t) THROWS_NOTHING;
    bool can_be_removed();
private:
    void check_stamp(uuid_u uuid, repli_timestamp_t timestamp);
    std::map<uuid_u, repli_timestamp_t> last_timestamps;

    client_t *client;
    uuid_u uuid;
    mailbox_manager_t *manager;
    mailbox_t<void(stamped_msg_t)> mailbox;
    std::vector<server_t::addr_t> stop_addrs;
    std::vector<scoped_ptr_t<disconnect_watcher_t> > disconnect_watchers;
    wait_any_t any_disconnect;
    std::vector<std::set<sub_t *> > subs;
    rwlock_t subs_lock;
    int64_t num_subs;
    bool detached;
    auto_drainer_t drainer;
};

class msg_visitor_t : public boost::static_visitor<void> {
public:
    msg_visitor_t(feed_t *_feed, uuid_u _server_uuid, repli_timestamp_t _timestamp)
        : feed(_feed), server_uuid(_server_uuid), timestamp(_timestamp) { }
    void operator()(const msg_t::change_t &change) const {
        auto null = make_counted<const datum_t>(datum_t::R_NULL);
        std::map<std::string, counted_t<const datum_t> > obj{
            {"new_val", change.new_val.has() ? change.new_val : null},
            {"old_val", change.old_val.has() ? change.old_val : null}
        };
        auto d = make_counted<const datum_t>(std::move(obj));
        feed->each_sub(
            [this, d](sub_t *sub) {
                sub->add_el(server_uuid, timestamp, d);
            }
        );
    }
    void operator()(const msg_t::stop_t &) const {
        const char *msg = "Changefeed aborted (table dropped).";
        feed->each_sub([msg](sub_t *sub) { sub->abort(msg); });
    }
private:
    feed_t *feed;
    uuid_u server_uuid;
    repli_timestamp_t timestamp;
};
void handle_msg(feed_t *feed, const stamped_msg_t &msg) {
    msg_visitor_t visitor(feed, msg.server_uuid, msg.timestamp);
    boost::apply_visitor(visitor, msg.submsg.op);
}

class stream_t : public eager_datum_stream_t {
public:
    template<class... Args>
    stream_t(scoped_ptr_t<sub_t> &&_sub, Args... args)
        : eager_datum_stream_t(std::forward<Args...>(args...)),
          sub(std::move(_sub)) { }
    virtual bool is_array() { return false; }
    virtual bool is_exhausted() const { return false; }
    virtual std::vector<counted_t<const datum_t> >
    next_raw_batch(env_t *env, const batchspec_t &bs) {
        batcher_t batcher = bs.to_batcher();
        return sub->get_els(&batcher, env->interruptor);
    }
private:
    scoped_ptr_t<sub_t> sub;
};

sub_t::sub_t(feed_t *_feed) : cond(NULL), feed(_feed) {
    guarantee(feed != NULL);
    feed->add_sub(this);
}

sub_t::~sub_t() {
    debugf("destroy %p\n", this);
    // This error is only sent if we're getting destroyed while blocking.
    abort("Subscription destroyed (shutting down?).");
    debugf("del_sub %p\n", this);
    if (feed != NULL) {
        feed->del_sub(this);
    } else {
        // We only get here if we were detached.
        guarantee(exc);
    }
    debugf("destroyed %p\n", this);
}

std::vector<counted_t<const datum_t> >
sub_t::get_els(batcher_t *batcher, const signal_t *interruptor) {
    assert_thread();
    guarantee(cond == NULL); // Can't get while blocking.
    auto_drainer_t::lock_t lock(&drainer);
    if (els.size() == 0 && !exc) {
        cond_t wait_for_data;
        cond = &wait_for_data;
        try {
            // We don't need to wait on the drain signal because the interruptor
            // will be pulsed if we're shutting down.
            wait_interruptible(cond, interruptor);
        } catch (const interrupted_exc_t &e) {
            cond = NULL;
            throw e;
        }
        guarantee(cond == NULL);
    }
    if (exc) {
        std::rethrow_exception(exc);
    }
    std::vector<counted_t<const datum_t> > v;
    while (els.size() > 0 && !batcher->should_send_batch()) {
        batcher->note_el(els.front());
        v.push_back(std::move(els.front()));
        els.pop_front();
    }
    guarantee(v.size() != 0);
    return std::move(v);
}

void sub_t::add_el(const uuid_u &uuid, const repli_timestamp_t &timestamp,
                   counted_t<const datum_t> d) {
    assert_thread();
    // If we don't have start timestamps, we haven't started, and if we have
    // exc, we've stopped.
    if (start_timestamps.size() != 0 && !exc) {
        auto it = start_timestamps.find(uuid);
        guarantee(it != start_timestamps.end());
        if (timestamp > it->second) {
            els.push_back(d);
            if (els.size() > array_size_limit()) {
                els.clear();
                abort("Changefeed buffer over array size limit.  "
                      "If you're making a lot of changes to your data, "
                      "make sure your client can keep up.");
            } else {
                maybe_signal_cond();
            }
        }
    }
}

void sub_t::start(std::map<uuid_u, repli_timestamp_t> &&_start_timestamps) {
    assert_thread();
    start_timestamps = std::move(_start_timestamps);
    guarantee(start_timestamps.size() != 0);
}

void sub_t::abort(const std::string &msg, detach_t detach) {
    assert_thread();
    if (detach == detach_t::YES) {
        feed = NULL;
    }
    exc = std::make_exception_ptr(datum_exc_t(base_exc_t::GENERIC, msg));
    maybe_signal_cond();
}

void sub_t::maybe_signal_cond() THROWS_NOTHING {
    assert_thread();
    if (cond != NULL) {
        ASSERT_NO_CORO_WAITING;
        cond->pulse();
        cond = NULL;
    }
}

// If this throws we might leak the increment to `num_subs`.
void feed_t::add_sub(sub_t *sub) THROWS_NOTHING {
    on_thread_t th(home_thread());
    guarantee(!detached);
    num_subs += 1;
    auto_drainer_t::lock_t lock(&drainer);
    rwlock_in_line_t spot(&subs_lock, access_t::write);
    spot.write_signal()->wait_lazily_unordered();
    subs[sub->home_thread().threadnum].insert(sub);
}
// Can't throw because it's called in a destructor.
void feed_t::del_sub(sub_t *sub) THROWS_NOTHING {
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

template<class T>
void feed_t::each_sub(const T &t) THROWS_NOTHING {
    assert_thread();
    auto_drainer_t::lock_t lock(&drainer);
    rwlock_in_line_t spot(&subs_lock, access_t::read);
    spot.read_signal()->wait_lazily_unordered();

    std::vector<int> sub_threads;
    for (int i = 0; i < get_num_threads(); ++i) {
        if (subs[i].size() != 0) {
            sub_threads.push_back(i);
        }
    }
    pmap(sub_threads.size(),
         [&t, &sub_threads, this](int i) {
             auto set = &subs[sub_threads[i]];
             guarantee(set->size() != 0);
             on_thread_t th((threadnum_t(sub_threads[i])));
             for (auto it = set->begin(); it != set->end(); ++it) {
                 // debugf("%p: %d\n", (*it), (*it)->home_thread().threadnum);
                 t(*it);
             }
         });
}

bool feed_t::can_be_removed() {
    return num_subs == 0;
}

void feed_t::check_stamp(uuid_u server_uuid, repli_timestamp_t timestamp) {
    auto it = last_timestamps.find(server_uuid);
    if (it != last_timestamps.end()) {
        if (timestamp < it->second) {
            debugf("%s: %" PRIu64 " < %" PRIu64 "\n",
                   uuid_to_str(server_uuid).c_str(),
                   timestamp.longtime,
                   it->second.longtime);
            guarantee(false);
        }
    }
    last_timestamps[server_uuid] = timestamp;
}

feed_t::feed_t(client_t *_client,
               base_namespace_repo_t *ns_repo,
               uuid_u _uuid,
               signal_t *interruptor)
    : client(_client),
      uuid(_uuid),
      manager(client->get_manager()),
      mailbox(
          manager,
          [this](stamped_msg_t msg) {
              // We stop receiving messages when detached (we're only receiving
              // messages because we haven't managed to get a message to the
              // stop mailboxes for some of the masters yet).  This also stops
              // us from trying to handle a message while waiting on the auto
              // drainer.
              if (!detached) {
                  check_stamp(msg.server_uuid, msg.timestamp);
                  handle_msg(this, msg);
              }
          }
      ),
      subs(get_num_threads()),
      num_subs(0),
      detached(false) {
    base_namespace_repo_t::access_t access(ns_repo, uuid, interruptor);
    auto nif = access.get_namespace_if();
    write_t write(changefeed_subscribe_t(mailbox.get_address()),
                  profile_bool_t::DONT_PROFILE);
    write_response_t write_resp;
    nif->write(write, &write_resp, order_token_t::ignore, interruptor);
    auto resp = boost::get<changefeed_subscribe_response_t>(&write_resp.response);
    guarantee(resp);
    stop_addrs = std::move(resp->addrs);
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
    // We spawn now to make sure `this` is valid when it runs.
    coro_t::spawn_now_dangerously(
        [this]() {
            auto_drainer_t::lock_t lock(&drainer);
            wait_any_t wait_any(&any_disconnect, lock.get_drain_signal());
            wait_any.wait_lazily_unordered();
            if (!detached) {
                scoped_ptr_t<feed_t> self = client->detach_feed(uuid);
                detached = true;
                if (self.has()) {
                    each_sub(
                        [](sub_t *sub) {
                            // RSI: make sure feed pointer doesn't survive past a lock.
                            sub->abort("Disconnected from peer.", detach_t::YES);
                        }
                    );
                    num_subs = 0;
                } else {
                    // We only get here if we were removed before we were detached.
                    guarantee(num_subs == 0);
                }
            }
        }
    );
}

feed_t::~feed_t() {
    debugf("~feed_t()\n");
    guarantee(num_subs == 0);
    detached = true;
    // RSI: Are these sends expensive enough that we should avoid holding up
    // destruction for them?  Also, can they throw?
    for (auto it = stop_addrs.begin(); it != stop_addrs.end(); ++it) {
        send(manager, *it, mailbox.get_address());
    }
    debugf("~feed_t() DONE\n");
}

client_t::client_t(mailbox_manager_t *_manager) : manager(_manager) { }
client_t::~client_t() { }

counted_t<datum_stream_t>
client_t::new_feed(const counted_t<table_t> &tbl, env_t *env) {
    debugf("CLIENT: calling `new_feed`...\n");
    try {
        uuid_u uuid = tbl->get_uuid();
        scoped_ptr_t<sub_t> sub;
        {
            threadnum_t old_thread = get_thread_id();
            cross_thread_signal_t interruptor(env->interruptor, home_thread());
            on_thread_t th(home_thread());
            debugf("CLIENT: On home thread...\n");
            auto_drainer_t::lock_t lock(&drainer);
            rwlock_in_line_t spot(&feeds_lock, access_t::write);
            spot.read_signal()->wait_lazily_unordered();
            debugf("CLIENT: getting feed...\n");
            auto feed_it = feeds.find(uuid);
            if (feed_it == feeds.end()) {
                debugf("CLIENT: making feed...\n");
                spot.write_signal()->wait_lazily_unordered();
                auto val = make_scoped<feed_t>(
                    this, env->cluster_access.ns_repo, uuid, &interruptor);
                feed_it = feeds.insert(std::make_pair(uuid, std::move(val))).first;
            }

            // We need to do this while holding `feeds_lock` to make sure the
            // feed isn't destroyed before we subscribe to it.
            on_thread_t th2(old_thread);
            sub.init(new sub_t(feed_it->second.get()));
        }
        base_namespace_repo_t::access_t access(
            env->cluster_access.ns_repo, uuid, env->interruptor);
        auto nif = access.get_namespace_if();
        write_t write(changefeed_timestamp_t(), profile_bool_t::DONT_PROFILE);
        write_response_t write_resp;
        nif->write(write, &write_resp, order_token_t::ignore, env->interruptor);
        auto resp = boost::get<changefeed_timestamp_response_t>(
            &write_resp.response);
        guarantee(resp);
        sub->start(std::move(resp->timestamps));
        return counted_t<datum_stream_t>(
            new stream_t(std::move(sub), tbl->backtrace()));
    } catch (const cannot_perform_query_exc_t &e) {
        rfail_datum(ql::base_exc_t::GENERIC,
                    "cannot subscribe to table `%s`: %s",
                    tbl->name.c_str(), e.what());
    }
}

void client_t::maybe_remove_feed(const uuid_u &uuid) {
    debugf("CLIENT: maybe_remove_feed...\n");
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
        debugf("CLIENT: removing feed...\n");
        // We want to destroy the feed after the lock is released, because it
        // may be expensive.
        destroy.swap(feed_it->second);
        feeds.erase(feed_it);
    }
    debugf("CLIENT: maybe remove feed DONE!\n");
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
    return std::move(ret);
}

} // namespace changefeed
} // namespace ql
