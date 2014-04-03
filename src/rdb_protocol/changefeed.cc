#include "rdb_protocol/changefeed.hpp"
#include "rdb_protocol/val.hpp"

namespace ql {

// RSI: work out threading.

class change_stream_t;

class changefeed_t {
public:
    changefeed_t(mailbox_manager_t *manager) {
        auto f = [=](counted_t<const datum_t> d) {
            this->push_datum(d);
            this->maybe_notify_waiters();
        };
        mailbox.init(new mailbox_t<void(counted_t<const datum_t>)>(manager, f));
    }
    ~changefeed_t() {
        // If we have subscribers left, they have a dangling pointer.
        guarantee(subscribers.size() == 0);
    }
    void subscribe(change_stream_t *s) {
        guarantee(mailbox.has());
        subscribers.insert(s);
    }
    // `unsubscribe` can't throw because it's called in a destructor.
    void unsubscribe(change_stream_t *s) THROWS_NOTHING {
        guarantee(mailbox.has());
        subscribers.erase(s);
        if (subscribers.size() == 0) {
            // RSI: unsubscribe.
        }
    }
    void wait_for_data() {
        guarantee(mailbox.has());
        if (!data_cond.has()) {
            data_cond.init(new cond_t());
        }
        data_cond->wait_lazily_unordered();
    }
    mailbox_addr_t<void(counted_t<const datum_t>)> addr() {
        return mailbox->get_address();
    }
    changefeed_t(changefeed_t &&rhs) {
        mailbox.swap(rhs.mailbox);
        data_cond.swap(rhs.data_cond);
        subscribers.swap(rhs.subscribers);
    }
private:
    void push_datum(counted_t<const datum_t> d);
    void maybe_notify_waiters() {
        if (data_cond.has()) {
            data_cond->pulse();
            data_cond.reset();
        }
    }
    scoped_ptr_t<mailbox_t<void(counted_t<const datum_t>)> > mailbox;
    scoped_ptr_t<cond_t> data_cond;
    std::set<change_stream_t *> subscribers;
};

class change_stream_t : public eager_datum_stream_t {
public:
    template<class... Args>
    change_stream_t(changefeed_t *_feed, Args... args)
        : eager_datum_stream_t(std::forward<Args...>(args...)), feed(_feed) {
        feed->subscribe(this);
    }
    ~change_stream_t() { feed->unsubscribe(this); }
    void add_el(counted_t<const datum_t> d) {
        els.push_back(d);
        if (els.size() > array_size_limit()) {
            // RSI: do something smart.
        }
    }
    virtual bool is_array() { return false; }
    virtual bool is_exhausted() const { return false; }
    virtual std::vector<counted_t<const datum_t> >
    next_raw_batch(env_t *, const batchspec_t &bs) {
        batcher_t batcher = bs.to_batcher();
        std::vector<counted_t<const datum_t> > v;
        while (els.size() == 0) {
            feed->wait_for_data();
        }
        while (els.size() > 0 && !batcher.should_send_batch()) {
            v.push_back(std::move(els.front()));
            batcher.note_el(els.front());
            els.pop_front();
        }
        r_sanity_check(v.size() != 0);
        return std::move(v);
    }
private:
    changefeed_t *feed;
    std::deque<counted_t<const datum_t> > els;
};

void changefeed_t::push_datum(counted_t<const datum_t> d) {
    for (auto it = subscribers.begin(); it != subscribers.end(); ++it) {
        (*it)->add_el(d);
    }
}

changefeed_manager_t::changefeed_manager_t(mailbox_manager_t *_manager)
    : manager(_manager) { }
changefeed_manager_t::~changefeed_manager_t() { }

counted_t<datum_stream_t> changefeed_manager_t::changefeed(
    const counted_t<table_t> &tbl) {
    uuid_u uuid = tbl->get_uuid();
    auto entry = changefeeds.find(uuid);
    if (entry == changefeeds.end()) {
        entry = changefeeds.insert(std::make_pair(uuid, changefeed_t(manager))).first;
    }
    auto feed = &entry->second;
    send(manager, feed->addr(), make_counted<const datum_t>(3.14));
    return counted_t<datum_stream_t>(new change_stream_t(feed, tbl->backtrace()));
}

}
