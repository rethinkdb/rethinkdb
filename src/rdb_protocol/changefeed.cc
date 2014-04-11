#include "rdb_protocol/changefeed.hpp"

#include "rpc/mailbox/typed.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/val.hpp"

#include "debug.hpp"

namespace ql {

typedef changefeed_manager_t::subscription_t subscription_t;

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
RDB_IMPL_ME_SERIALIZABLE_3(
    changefeed_msg_t, type, empty_ok(old_val), empty_ok(new_val));

class changefeed_updater_t : public home_thread_mixin_t {
public:
    changefeed_updater_t(mailbox_addr_t<void(changefeed_msg_t)> _addr,
                         base_namespace_repo_t<rdb_protocol_t> *ns_repo,
                         uuid_u uuid)
        : addr(_addr) {
        auto_drainer_t::lock_t lock(&drainer);
        access.init(new base_namespace_repo_t<rdb_protocol_t>::access_t(
                        ns_repo, uuid, lock.get_drain_signal()));
        start_changefeed();
    }
    ~changefeed_updater_t() {
        stop_changefeed();
    }
private:
    void start_changefeed() THROWS_ONLY(cannot_perform_query_exc_t) {
        update_changefeed(rdb_protocol_t::changefeed_update_t::SUBSCRIBE);
    }
    void stop_changefeed() THROWS_NOTHING { // This is called in a destructor.
        try {
            update_changefeed(rdb_protocol_t::changefeed_update_t::UNSUBSCRIBE);
        } catch (const cannot_perform_query_exc_t &e) {
            // RSI(CR): We can't access the table to unsubscribe.  Not sure what to
            // do here; how should we handle the dangling mailbox on the other
            // end?
        }
    }
    void update_changefeed(rdb_protocol_t::changefeed_update_t::action_t action)
        THROWS_ONLY(cannot_perform_query_exc_t) {
        assert_thread();
        try {
            auto_drainer_t::lock_t lock(&drainer);
            auto nif = access->get_namespace_if();
            rdb_protocol_t::write_t write(
                rdb_protocol_t::changefeed_update_t(addr, action),
                profile_bool_t::DONT_PROFILE);
            rdb_protocol_t::write_response_t resp;
            nif->write(write, &resp, order_token_t::ignore, lock.get_drain_signal());
        } catch (const interrupted_exc_t &e) {
            // RSI(CR): We're being destroyed.  Not sure what to do here; how should
            // we handle the dangling mailbox on the other end?
        }
    }
    mailbox_addr_t<void(changefeed_msg_t)> addr;
    scoped_ptr_t<base_namespace_repo_t<rdb_protocol_t>::access_t> access;
    auto_drainer_t drainer;
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
        // If we have subscribers left, they have a dangling pointer.  This
        // assertion should be true because we're only destroyed when shutting
        // down, and the `change_stream_t`s holding `subscription_t`s should be
        // destroyed before us.

        // RSI(test): test that!
        guarantee(total_subscribers == 0);
    }

private:
    friend class changefeed_manager_t::subscription_t;
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
    change_stream_t(uuid_u uuid,
                    base_namespace_repo_t<rdb_protocol_t> *ns_repo,
                    changefeed_manager_t *manager,
                    Args... args)
        THROWS_ONLY(cannot_perform_query_exc_t, exc_t, datum_exc_t)
        : eager_datum_stream_t(std::forward<Args...>(args...)),
          subscription(uuid, ns_repo, manager) { }
    virtual bool is_array() { return false; }
    virtual bool is_exhausted() const { return false; }
    virtual std::vector<counted_t<const datum_t> >
    next_raw_batch(env_t *env, const batchspec_t &bs) {
        batcher_t batcher = bs.to_batcher();
        return subscription.get_els(&batcher, env->interruptor);
    }
private:
    subscription_t subscription;
};

std::vector<counted_t<const datum_t> >
subscription_t::get_els(batcher_t *batcher, const signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t) {
    assert_thread();
    guarantee(cond == NULL); // Can't call `get_els` while already blocking.
    auto_drainer_t::lock_t lock(&*drainer);
    if (els.size() == 0 && !finished) {
        cond_t wait_for_data;
        cond = &wait_for_data;
        try {
            wait_interruptible(cond, interruptor);
        } catch (const interrupted_exc_t &e) {
            cond = NULL;
            throw e;
        }
        guarantee(cond == NULL);
    }
    if (finished) {
        if (exc.has()) {
            throw *exc;
        } else {
            return std::vector<counted_t<const datum_t> >();
        }
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

void subscription_t::maybe_signal_cond() THROWS_NOTHING {
    assert_thread();
    if (cond != NULL) {
        auto tmp = cond;
        cond = NULL;
        tmp->pulse();
    }
}

void subscription_t::add_el(counted_t<const datum_t> d) {
    assert_thread();
    if (!finished) {
        els.push_back(d);
        if (els.size() > array_size_limit()) {
            els.clear();
            finished = true;
            exc.init(new datum_exc_t(base_exc_t::GENERIC,
                                     "Changefeed buffer over array size limit.  "
                                     "If you're making a lot of changes to your data, "
                                     "make sure your client can keep up."));

        }
        maybe_signal_cond();
    }
}

// We don't remove ourselves from our feed, because the `change_stream_t`
// reading from us needs us to still exist.  We'll cease to exist when it reads
// the end-of-stream empty batch from us and is removed from the stream cache.
// (In the meantime, `add_el` becomes a noop.)
void subscription_t::finish() {
    assert_thread();
    finished = true;
    maybe_signal_cond();
}

subscription_t::subscription_t(
    uuid_u uuid,
    base_namespace_repo_t<rdb_protocol_t> *ns_repo,
    changefeed_manager_t *_manager)
    THROWS_ONLY(cannot_perform_query_exc_t)
    : finished(false), cond(NULL), manager(_manager), drainer(new auto_drainer_t()) {
    on_thread_t th(manager->home_thread());
    changefeed = manager->changefeeds.find(uuid);
    if (changefeed == manager->changefeeds.end()) {
        auto cfeed = make_scoped<changefeed_t>(manager->manager, ns_repo, uuid);
        changefeed = manager->changefeeds.insert(
            std::make_pair(uuid, std::move(cfeed))).first;
    }
    guarantee(changefeed->second->home_thread() == manager->home_thread());
    changefeed->second->total_subscribers += 1;
    changefeed->second->add_subscriber(home_thread().threadnum, this);
}
subscription_t::~subscription_t() {
    finished = true;
    // This will only actually get thrown if `maybe_signal_cond` wakes a
    // coroutine, in which case we were deleted while a `change_stream_t` was
    // waiting on us, which means we should wake it up and give it some
    // reasonable error before shutting down (which is enforced by the drainer).
    exc.init(new datum_exc_t(base_exc_t::GENERIC,
                             "Subscription destroyed (shutting down?)"));
    maybe_signal_cond();
    on_thread_t th(changefeed->second->home_thread());
    changefeed->second->total_subscribers -= 1;
    changefeed->second->del_subscriber(home_thread().threadnum, this);
    if (changefeed->second->total_subscribers == 0) {
        // It's safe to do this because the `subscription_t` constructor doesn't
        // block between finding the changefeed in the map and incrementing its
        // `total_subcribers` count, so right now we're 100% sure that no other
        // subscription has a reference to this changefeed.
        guarantee(changefeed->second->home_thread() == manager->home_thread());
        manager->changefeeds.erase(changefeed);
    }
}

void changefeed_t::add_subscriber(int threadnum, subscription_t *subscriber) {
    assert_thread();
    guarantee(total_subscribers != 0);
    rwlock_in_line_t spot(&subscribers_lock, access_t::write);
    spot.write_signal()->wait_lazily_unordered();
    auto set = &subscribers[threadnum];
    auto ret = set->insert(subscriber);
    guarantee(ret.second);
}

void changefeed_t::del_subscriber(int threadnum, subscription_t *subscriber)
    THROWS_NOTHING {
    assert_thread();
    rwlock_in_line_t spot(&subscribers_lock, access_t::write);
    spot.write_signal()->wait_lazily_unordered();
    auto set = &subscribers[threadnum];
    size_t erased = set->erase(subscriber);
    guarantee(erased == 1);
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
    try {
        return counted_t<datum_stream_t>(
            new change_stream_t(
                tbl->get_uuid(),
                env->cluster_access.ns_repo,
                this,
                tbl->backtrace()));
    } catch (const cannot_perform_query_exc_t &e) {
        rfail_datum(ql::base_exc_t::GENERIC,
                    "cannot subscribe to table `%s`: %s",
                    tbl->name.c_str(), e.what());
    }
}

}
