#ifndef __REPLICATION_DEBUG_HPP__
#define __REPLICATION_DEBUG_HPP__

#include "replication/net_structs.hpp"
#include <string>

namespace replication {

std::string debug_format(const net_hello_t *msg);
std::string debug_format(const net_introduce_t *msg);
std::string debug_format(const net_backfill_t *msg);
std::string debug_format(const net_backfill_complete_t *msg);
std::string debug_format(const net_backfill_delete_everything_t *msg);
std::string debug_format(const net_nop_t *msg);
std::string debug_format(const net_get_cas_t *msg);
std::string debug_format(const net_sarc_t *msg);
std::string debug_format(const net_backfill_set_t *msg);
std::string debug_format(const net_incr_t *msg);
std::string debug_format(const net_decr_t *msg);
std::string debug_format(const net_append_t *msg);
std::string debug_format(const net_prepend_t *msg);
std::string debug_format(const net_delete_t *msg);
std::string debug_format(const net_backfill_delete_t *msg);

}   // namespace replication

#endif /* __REPLICATION_DEBUG_HPP__ */
