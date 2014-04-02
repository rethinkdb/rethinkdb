#ifndef RDB_PROTOCOL_CHANGEFEED_HPP_
#define RDB_PROTOCOL_CHANGEFEED_HPP_

#include "rdb_protocol/datum_stream.hpp"
#include "rpc/mailbox/typed.hpp"

namespace ql {

class change_stream_t;
class mailbox_manager_t;
class changefeed_t {
public:
    changefeed_t(mailbox_manager_t *manager);
    void subscribe(change_stream_t *s);
    void unsubscribe(change_stream_t *s);
private:
    mailbox_t<void(counted_t<const datum_t>)> mailbox;
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
private:
    changefeed_t *feed;
};

class table_t;
class changefeed_manager_t {
public:
    changefeed_t *get_feed(table_t *tbl);
private:
    std::map<uuid_u, changefeed_t> changefeeds;
};

} // namespace ql

#endif // RDB_PROTOCOL_CHANGEFEED_HPP_
