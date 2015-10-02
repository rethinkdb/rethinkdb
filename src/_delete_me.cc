#include <string>

#include "boost/optional.hpp"

#include "rpc/mailbox/typed.hpp"
#include "containers/archive/archive.hpp"

// from changefeed.hpp

struct stamped_msg_t;
typedef mailbox_addr_t<void(stamped_msg_t)> client_addr_t;

mailbox_t<void(client_addr_t, boost::optional<std::string>, uuid_u)> limit_stop_mailbox;
