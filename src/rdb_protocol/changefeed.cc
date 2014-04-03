#include "rdb_protocol/changefeed.hpp"
#include "rdb_protocol/val.hpp"

#include "debug.hpp"

namespace ql {

// RSI: work out threading.

class change_stream_t;

class changefeed_t : public home_thread_mixin_t {
public:
    changefeed_t(mailbox_manager_t *manager)
        : mailbox(manager, [=](counted_t<const datum_t> d) { this->push_datum(d); }) { }
    ~changefeed_t() {
        debugf("~changefeed_t");
        // If we have subscribers left, they have a dangling pointer.
        for (int i = 0; i < get_num_threads(); ++i) {
            guarantee(subscribers.get_thread(threadnum_t(i))->size() == 0);
        }
    }

    void subscribe(change_stream_t *s);
    // `unsubscribe` can't throw because it's called in a destructor.
    void unsubscribe(change_stream_t *s) THROWS_NOTHING;

    mailbox_addr_t<void(counted_t<const datum_t>)> addr() {
        on_thread_t th(home_thread());
        return mailbox.get_address();
    }
private:
    void push_datum(counted_t<const datum_t> d);
    mailbox_t<void(counted_t<const datum_t>)> mailbox;
    one_per_thread_t<std::set<change_stream_t *> > subscribers;
};

class change_stream_t : public eager_datum_stream_t, public home_thread_mixin_t {
public:
    template<class... Args>
    change_stream_t(changefeed_t *_feed, Args... args)
        : eager_datum_stream_t(std::forward<Args...>(args...)),
          feed(_feed), wake_coro(NULL) {
        feed->subscribe(this);
    }
    ~change_stream_t() { feed->unsubscribe(this); }
    void add_el(counted_t<const datum_t> d) {
        home_thread_mixin_t::assert_thread();
        els.push_back(d);
        if (wake_coro != NULL) {
            wake_coro->notify_sometime();
            wake_coro = NULL;
        }
        if (els.size() > array_size_limit()) {
            // RSI: do something smart.
        }
    }
    virtual bool is_array() { return false; }
    virtual bool is_exhausted() const { return false; }
    virtual std::vector<counted_t<const datum_t> >
    next_raw_batch(env_t *, const batchspec_t &bs) {
        home_thread_mixin_t::assert_thread();
        batcher_t batcher = bs.to_batcher();
        std::vector<counted_t<const datum_t> > v;
        while (els.size() == 0) {
            wake_coro = coro_t::self();
            coro_t::wait();
        }
        while (els.size() > 0 && !batcher.should_send_batch()) {
            batcher.note_el(els.front());
            v.push_back(std::move(els.front()));
            els.pop_front();
        }
        r_sanity_check(v.size() != 0);
        return std::move(v);
    }
private:
    changefeed_t *feed;
    coro_t *wake_coro;
    std::deque<counted_t<const datum_t> > els;
};

void changefeed_t::subscribe(change_stream_t *s) {
    on_thread_t th(home_thread());
    subscribers.get_thread(s->home_thread())->insert(s);
    // RSI: subscribe remotely
}
void changefeed_t::unsubscribe(change_stream_t *s) THROWS_NOTHING {
    on_thread_t th(home_thread());
    subscribers.get_thread(s->home_thread())->erase(s);
    // RSI: unsubscribe remotely.
}
void changefeed_t::push_datum(counted_t<const datum_t> d) {
    assert_thread();
    auto f = [&](int i) {
        if (subscribers.get_thread(threadnum_t(i))->size() != 0) {
            on_thread_t th((threadnum_t(i)));
            auto set = subscribers.get();
            for (auto it = set->begin(); it != set->end(); ++it) {
                (*it)->add_el(d);
            }
        }
    };
    pmap(get_num_threads(), f);
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
