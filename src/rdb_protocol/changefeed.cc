#include "rdb_protocol/changefeed.hpp"
#include "rdb_protocol/val.hpp"

#include "debug.hpp"

namespace ql {

// RSI: work out threading.

class change_stream_t;

class changefeed_t : public home_thread_mixin_t {
public:
    changefeed_t(mailbox_manager_t *manager)
        : mailbox(manager, [=](counted_t<const datum_t> d) { this->push_datum(d); }),
          subscribers(get_num_threads()) { }
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
    void push_datum(counted_t<const datum_t> d) {
        assert_thread();
        auto f = [&](int i) {
            auto set = subscribers[i];
            if (set.size() != 0) {
                on_thread_t th((threadnum_t(i)));
                for (auto it = set.begin(); it != set.end(); ++it) {
                    (*it)->add_el(d);
                }
            }
        };
        pmap(get_num_threads(), f);
    }
    mailbox_t<void(counted_t<const datum_t>)> mailbox;
    // We use an array rather than a `one_per_thread_t` because it's managed by
    // `changefeed_t`s home thread.
    scoped_array_t<std::set<subscription_t *> > subscribers;
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
    auto ret = feed->subscribers[home_thread().threadnum].insert(this);
    guarantee(ret.second);
}
changefeed_t::subscription_t::~subscription_t() {
    threadnum_t feed_thread = feed->home_thread();
    on_thread_t th(feed_thread);
    size_t erased = feed->subscribers[home_thread().threadnum].erase(this);
    guarantee(erased == 1);
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
        // Do something smart.
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
    const counted_t<table_t> &tbl) {
    uuid_u uuid = tbl->get_uuid();
    changefeed_t *feed;
    {
        on_thread_t th(home_thread());
        auto entry = changefeeds.find(uuid);
        if (entry == changefeeds.end()) {
            auto val = std::make_pair(uuid, make_scoped<changefeed_t>(manager));
            entry = changefeeds.insert(std::move(val)).first;
        }
        feed = &*entry->second;
        assert(feed->home_thread() == home_thread());
    }
    auto ret = counted_t<datum_stream_t>(new change_stream_t(feed, tbl->backtrace()));
    send(manager, feed->addr(), make_counted<const datum_t>(3.14));
    return std::move(ret);
}

}
