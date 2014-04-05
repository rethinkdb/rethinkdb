#include "rdb_protocol/changefeed.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/val.hpp"

#include "debug.hpp"

namespace ql {

// RSI: make the interruptor make sense.
class changefeed_updater_t : public home_thread_mixin_t {
public:
    changefeed_updater_t(mailbox_addr_t<void(counted_t<const datum_t>)> _addr,
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
    mailbox_addr_t<void(counted_t<const datum_t>)> addr;
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
        : mailbox(manager, [=](counted_t<const datum_t> d) { this->push_datum(d); }),
          subscribers(get_num_threads()),
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
        coro_t *wake_coro; // `add_el` notifies `wake_coro` if it's non-NULL
        changefeed_t *feed;
        std::deque<counted_t<const datum_t> > els;
    };

    mailbox_addr_t<void(counted_t<const datum_t>)> addr() {
        on_thread_t th(home_thread());
        return mailbox.get_address();
    }
private:
    void push_datum(counted_t<const datum_t> d) THROWS_NOTHING {
        assert_thread();
        // RSI: do we need a drainer here?
        rwlock_in_line_t spot(&subscribers_lock, access_t::read);
        spot.read_signal()->wait_lazily_unordered();
        auto f = [&](int i) {
            auto set = &subscribers[i];
            if (set->size() != 0) {
                on_thread_t th((threadnum_t(i)));
                for (auto it = set->begin(); it != set->end(); ++it) {
                    (*it)->add_el(d);
                }
            }
        };
        // RSI: don't spawn coroutines for unsubscribed threads.
        pmap(get_num_threads(), f);
    }
    mailbox_t<void(counted_t<const datum_t>)> mailbox;
    // We use an array rather than a `one_per_thread_t` because it's managed by
    // `changefeed_t`s home thread.

    scoped_array_t<std::set<subscription_t *> > subscribers;
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

changefeed_t::subscription_t::subscription_t(changefeed_t *_feed)
    : wake_coro(NULL), feed(_feed) {
    threadnum_t feed_thread = feed->home_thread();
    on_thread_t th(feed_thread);
    rwlock_in_line_t spot(&feed->subscribers_lock, access_t::write);
    spot.write_signal()->wait_lazily_unordered();
    auto set = &feed->subscribers[home_thread().threadnum];
    auto ret = set->insert(this);
    guarantee(ret.second);
    debugf("insert (%p: %zu)\n", feed, set->size());
    if (set->size() == 1) {
        feed->updater.start_changefeed();
    }
}
changefeed_t::subscription_t::~subscription_t() {
    threadnum_t feed_thread = feed->home_thread();
    on_thread_t th(feed_thread);
    rwlock_in_line_t spot(&feed->subscribers_lock, access_t::write);
    spot.write_signal()->wait_lazily_unordered();
    auto set = &feed->subscribers[home_thread().threadnum];
    size_t erased = set->erase(this);
    guarantee(erased == 1);
    debugf("erase (%p: %zu)\n", feed, set->size());
    if (set->size() == 0) {
        feed->updater.stop_changefeed();
    }
}

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
    send(manager, feed->addr(), make_counted<const datum_t>(3.14));
    return std::move(ret);
}

}
