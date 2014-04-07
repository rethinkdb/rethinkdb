#ifndef RDB_PROTOCOL_CHANGEFEED_HPP_
#define RDB_PROTOCOL_CHANGEFEED_HPP_

#include <map>

#include "containers/counted.hpp"
#include "containers/scoped.hpp"
#include "rpc/serialize_macros.hpp"

class mailbox_manager_t;
struct rdb_modification_report_t;

namespace ql {

class table_t;
class changefeed_t;
class datum_stream_t;
class datum_t;
class env_t;

struct changefeed_msg_t {
    enum type_t { UNINITIALIZED, CHANGE, TABLE_DROP };

    changefeed_msg_t();
    ~changefeed_msg_t();
    static changefeed_msg_t change(const rdb_modification_report_t *report);
    static changefeed_msg_t table_drop();

    type_t type;
    counted_t<const datum_t> old_val;
    counted_t<const datum_t> new_val;
    RDB_DECLARE_ME_SERIALIZABLE;
};

class changefeed_manager_t : public home_thread_mixin_t {
public:
    changefeed_manager_t(mailbox_manager_t *_manager);
    ~changefeed_manager_t();
    counted_t<datum_stream_t> changefeed(const counted_t<table_t> &tbl, env_t *env);
    mailbox_manager_t *get_manager() { return manager; }
private:
    mailbox_manager_t *manager;
    std::map<uuid_u, scoped_ptr_t<changefeed_t> > changefeeds;
};

} // namespace ql

#endif // RDB_PROTOCOL_CHANGEFEED_HPP_
