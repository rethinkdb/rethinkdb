#include "rdb_protocol/changefeed.hpp"
#include "rdb_protocol/val.hpp"

namespace ql {

// RSI: work out threading.

class change_stream_t;

class changefeed_t {
public:
    // We store these `changefeed_t`s in a `std::map`, but they aren't copyable
    // and our STL doesn't support `map::emplace`, so instead we use
    // `map::operator[]` with a default-constructed `changefeed_t` and
    // explicitly initialize it afterward.
    changefeed_t() { } // Make sure to call `ensure_initialized`!
    void ensure_initialized(mailbox_manager_t *manager) {
        if (!mailbox.has()) {
            auto f = [=](counted_t<const datum_t> d){
                this->push_datum(d);
                this->maybe_notify_waiters();
            };
            mailbox.init(new mailbox_t<void(counted_t<const datum_t>)>(manager, f));
        }
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
        if (!data_cond.has()) {
            data_cond.init(new cond_t());
        }
        data_cond->wait_lazily_unordered();
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
    ~change_stream_t() {
        feed->unsubscribe(this);
    }
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

counted_t<datum_stream_t> changefeed_manager_t::changefeed(table_t *tbl) {
    auto feed = &changefeeds[tbl->get_uuid()];
    feed->ensure_initialized(manager);
    return counted_t<datum_stream_t>(new change_stream_t(feed, tbl->backtrace()));
}

}
