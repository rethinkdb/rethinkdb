#include "rdb_protocol/changefeed.hpp"

#include "rpc/mailbox/typed.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/val.hpp"

#include "debug.hpp"

namespace ql {

changefeed_msg_t::changefeed_msg_t() : type(UNINITIALIZED) { }
changefeed_msg_t::~changefeed_msg_t() { }

changefeed_msg_t changefeed_msg_t::change(const rdb_modification_report_t *report) {
    changefeed_msg_t ret;
    ret.type = CHANGE;
    ret.old_val = report->info.deleted.first;
    ret.new_val = report->info.added.first;
    return std::move(ret);
}

changefeed_msg_t changefeed_msg_t::table_drop() {
    changefeed_msg_t ret;
    ret.type = TABLE_DROP;
    return std::move(ret);
}

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
    changefeed_msg_t::type_t,
    int8_t,
    changefeed_msg_t::UNINITIALIZED,
    changefeed_msg_t::TABLE_DROP);
RDB_IMPL_ME_SERIALIZABLE_3(changefeed_msg_t, type, empty_ok(old_val), empty_ok(new_val));

// RSI: make the interruptor make sense.
class changefeed_updater_t : public home_thread_mixin_t {
public:
    changefeed_updater_t(mailbox_addr_t<void(changefeed_msg_t)> _addr,
                         base_namespace_repo_t<rdb_protocol_t> *ns_repo,
                         uuid_u uuid)
        : addr(_addr), access(ns_repo, uuid, &cond) { }
    ~changefeed_updater_t() { cond.pulse(); }
    void start_changefeed() {
        // RSI: handle master unavailable exceptions in the layer above us.
        update_changefeed(rdb_protocol_t::changefeed_update_t::SUBSCRIBE);
    }
    void stop_changefeed() THROWS_NOTHING { // This is called in a destructor.
        try {
            update_changefeed(rdb_protocol_t::changefeed_update_t::UNSUBSCRIBE);
            // RSI: Exceptions: interrupted_exc_t, master unavailable, ?
        } catch (...) {
            // If something goes wrong here (e.g. the master is unavailable), we
            // just continue.  This behavior is correct, but might be
            // inefficient (e.g. if the master is only temporarily down we don't
            // want to stay subscribed forever).
        }
    }
private:
    void update_changefeed(rdb_protocol_t::changefeed_update_t::action_t action) {
        assert_thread();
        auto_drainer_t::lock_t lock(&interrupt_drainer);
        auto nif = access.get_namespace_if();
        rdb_protocol_t::write_t write(
            rdb_protocol_t::changefeed_update_t(addr, action),
            profile_bool_t::DONT_PROFILE);
        rdb_protocol_t::write_response_t resp;
        nif->write(write, &resp, order_token_t::ignore, &cond);
    }
    mailbox_addr_t<void(changefeed_msg_t)> addr;
    cond_t cond;
    base_namespace_repo_t<rdb_protocol_t>::access_t access;
    auto_drainer_t interrupt_drainer;
};

class change_stream_t;

class changefeed_t : public home_thread_mixin_t {
public:
    changefeed_t(mailbox_manager_t *manager,
                 base_namespace_repo_t<rdb_protocol_t> *ns_repo,
                 uuid_u uuid)
        : mailbox(manager, [=](changefeed_msg_t msg) { this->handle(std::move(msg)); }),
          subscribers(get_num_threads()),
          total_subscribers(0),
          updater(mailbox.get_address(), ns_repo, uuid) { }
    ~changefeed_t() {
        // If we have subscribers left, they have a dangling pointer.
        for (int i = 0; i < get_num_threads(); ++i) {
            guarantee(subscribers[i].size() == 0);
        }
    }

    class subscription_t : public home_thread_mixin_t {
    public:
        subscription_t(changefeed_t *_feed);
        ~subscription_t();
        // Will wait for at least one element.
        std::vector<counted_t<const datum_t> > get_els(batcher_t *batcher);
    private:
        friend class changefeed_t;
        void add_el(counted_t<const datum_t> d);
        void finish() { } // RSI: Implement.
        coro_t *wake_coro; // `add_el` notifies `wake_coro` if it's non-NULL
        changefeed_t *feed;
        std::deque<counted_t<const datum_t> > els;
    };

private:
    // Used by `subscription_t`.
    void add_subscriber(int threadnum, subscription_t *subscriber);
    void del_subscriber(int threadnum, subscription_t *subscriber) THROWS_NOTHING;

    void handle(changefeed_msg_t msg) THROWS_NOTHING;
    mailbox_t<void(changefeed_msg_t)> mailbox;
    // We use an array rather than a `one_per_thread_t` because it's managed by
    // `changefeed_t`s home thread.

    scoped_array_t<std::set<subscription_t *> > subscribers;
    int64_t total_subscribers;
    rwlock_t subscribers_lock;
    changefeed_updater_t updater;
};

class change_stream_t : public eager_datum_stream_t {
public:
    template<class... Args>
    change_stream_t(changefeed_t *feed, Args... args)
        : eager_datum_stream_t(std::forward<Args...>(args...)), subscription(feed) { }
    virtual bool is_array() { return false; }
    virtual bool is_exhausted() const { return false; }
    virtual std::vector<counted_t<const datum_t> >
    next_raw_batch(env_t *, const batchspec_t &bs) {
        batcher_t batcher = bs.to_batcher();
        return subscription.get_els(&batcher);
    }
private:
    changefeed_t::subscription_t subscription;
};

std::vector<counted_t<const datum_t> >
changefeed_t::subscription_t::get_els(batcher_t *batcher) {
    assert_thread();
    if (els.size() == 0) {
        wake_coro = coro_t::self();
        coro_t::wait();
    }
    guarantee(els.size() != 0);
    std::vector<counted_t<const datum_t> > v;
    while (els.size() > 0 && !batcher->should_send_batch()) {
        batcher->note_el(els.front());
        v.push_back(std::move(els.front()));
        els.pop_front();
    }
    guarantee(v.size() != 0);
    return std::move(v);
}

void changefeed_t::subscription_t::add_el(counted_t<const datum_t> d) {
    assert_thread();
    els.push_back(d);
    if (els.size() > array_size_limit()) {
        // RSI: Do something smart.
    }
    if (wake_coro) {
        auto coro = wake_coro;
        wake_coro = NULL;
        coro->notify_sometime();
    }
}

changefeed_t::subscription_t::subscription_t(changefeed_t *_feed)
    : wake_coro(NULL), feed(_feed) {
    feed->add_subscriber(home_thread().threadnum, this);
}
changefeed_t::subscription_t::~subscription_t() {
    feed->del_subscriber(home_thread().threadnum, this);
}

void changefeed_t::add_subscriber(int threadnum, subscription_t *subscriber) {
    on_thread_t th(home_thread());
    rwlock_in_line_t spot(&subscribers_lock, access_t::write);
    spot.write_signal()->wait_lazily_unordered();
    auto set = &subscribers[threadnum];
    auto ret = set->insert(subscriber);
    guarantee(ret.second);
    total_subscribers += 1;
    if (total_subscribers == 1) {
        updater.start_changefeed();
    }
}

void changefeed_t::del_subscriber(int threadnum, subscription_t *subscriber)
    THROWS_NOTHING {
    on_thread_t th(home_thread());
    rwlock_in_line_t spot(&subscribers_lock, access_t::write);
    spot.write_signal()->wait_lazily_unordered();
    auto set = &subscribers[threadnum];
    size_t erased = set->erase(subscriber);
    guarantee(erased == 1);
    total_subscribers -= 1;
    if (total_subscribers == 0) {
        updater.stop_changefeed();
    }
}

void changefeed_t::handle(changefeed_msg_t msg) THROWS_NOTHING {
    assert_thread();
    std::function<void(subscription_t *)> action;
    switch (msg.type) {
    case changefeed_msg_t::CHANGE: {
        auto null = make_counted<const datum_t>(datum_t::R_NULL);
        std::map<std::string, counted_t<const datum_t> > obj{
            {"new_val", msg.new_val.has() ? msg.new_val : null},
                {"old_val", msg.old_val.has() ? msg.old_val : null}
        };
        auto d = make_counted<const datum_t>(std::move(obj));
        action = [=](subscription_t *sub) { sub->add_el(d); };
    } break;
    case changefeed_msg_t::TABLE_DROP: {
        action = [](subscription_t *sub) { sub->finish(); };
    } break;
    case changefeed_msg_t::UNINITIALIZED: unreachable();
    default: unreachable();
    }
    // RSI: do we need a drainer here?
    rwlock_in_line_t spot(&subscribers_lock, access_t::read);
    spot.read_signal()->wait_lazily_unordered();

    // We do a bit of extra work to avoid spawning coroutines for threads
    // with no subscribers (since spawning 24 coroutines when we don't need
    // to is wasteful).
    std::vector<int> subscribed_threads;
    auto f = [&](int i) {
        int threadnum = subscribed_threads[i];
        auto set = &subscribers[threadnum];
        rassert(set->size() != 0);
        on_thread_t th((threadnum_t(threadnum)));
        for (auto it = set->begin(); it != set->end(); ++it) {
            action(*it);
        }
    };
    guarantee(subscribers.size() == static_cast<size_t>(get_num_threads()));
    for (int i = 0; i < get_num_threads(); ++i) {
        if (subscribers[i].size() != 0) {
            subscribed_threads.push_back(i);
        }
    }
    pmap(subscribed_threads.size(), f);
}

changefeed_manager_t::changefeed_manager_t(mailbox_manager_t *_manager)
    : manager(_manager) { }
changefeed_manager_t::~changefeed_manager_t() { }

counted_t<datum_stream_t> changefeed_manager_t::changefeed(
    const counted_t<table_t> &tbl, env_t *env) {
    uuid_u uuid = tbl->get_uuid();
    base_namespace_repo_t<rdb_protocol_t> *ns_repo = env->cluster_access.ns_repo;
    changefeed_t *feed;
    {
        on_thread_t th(home_thread());
        auto entry = changefeeds.find(uuid);
        if (entry == changefeeds.end()) {
            auto cfeed = make_scoped<changefeed_t>(manager, ns_repo, uuid);
            entry = changefeeds.insert(std::make_pair(uuid, std::move(cfeed))).first;
        }
        feed = &*entry->second;
        assert(feed->home_thread() == home_thread());
    }
    counted_t<datum_stream_t> ret(new change_stream_t(feed, tbl->backtrace()));
    // send(manager, feed->addr(), make_counted<const datum_t>(3.14));
    return std::move(ret);
}

}
