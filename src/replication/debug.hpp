#ifndef __REPLICATION_DEBUG_HPP__
#define __REPLICATION_DEBUG_HPP__

#include "replication/net_structs.hpp"
#include <string>

namespace replication {

/* Sometimes the replication code stores the actual value to be set or appended separately
from the rest of the message. That's what the `real_data` parameter is for; if `real_data`
is non-NULL, then the value is read from there instead of from the end of `msg`. */

std::string debug_format(const net_hello_t *msg);
std::string debug_format(const net_introduce_t *msg);
std::string debug_format(const net_backfill_t *msg);
std::string debug_format(const net_backfill_complete_t *msg);
std::string debug_format(const net_backfill_delete_everything_t *msg);
std::string debug_format(const net_nop_t *msg);
std::string debug_format(const net_get_cas_t *msg);
std::string debug_format(const net_sarc_t *msg, const void *real_data = NULL);
std::string debug_format(const net_backfill_set_t *msg, const void *real_data = NULL);
std::string debug_format(const net_incr_t *msg);
std::string debug_format(const net_decr_t *msg);
std::string debug_format(const net_append_t *msg, const void *real_data = NULL);
std::string debug_format(const net_prepend_t *msg, const void *real_data = NULL);
std::string debug_format(const net_delete_t *msg);
std::string debug_format(const net_backfill_delete_t *msg);

}   // namespace replication

#endif /* __REPLICATION_DEBUG_HPP__ */
