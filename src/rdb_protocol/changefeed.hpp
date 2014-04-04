#ifndef RDB_PROTOCOL_CHANGEFEED_HPP_
#define RDB_PROTOCOL_CHANGEFEED_HPP_

#include "rdb_protocol/datum_stream.hpp"
#include "rpc/mailbox/typed.hpp"

class mailbox_manager_t;

namespace ql {

class table_t;
class changefeed_t;

class changefeed_manager_t : public home_thread_mixin_t {
public:
    changefeed_manager_t(mailbox_manager_t *_manager);
    ~changefeed_manager_t();
    counted_t<datum_stream_t> changefeed(const counted_t<table_t> &tbl, env_t *env);
private:
    mailbox_manager_t *manager;
    std::map<uuid_u, scoped_ptr_t<changefeed_t> > changefeeds;
};

} // namespace ql

#endif // RDB_PROTOCOL_CHANGEFEED_HPP_
