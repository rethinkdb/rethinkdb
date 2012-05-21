#ifndef PERFMON_ARCHIVE_HPP_
#define PERFMON_ARCHIVE_HPP_

#include "perfmon.hpp"
#include "containers/archive/boost_types.hpp"
#include "rpc/serialize_macros.hpp"

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(perfmon_result_t::perfmon_result_type_t, int8_t, perfmon_result_t::type_value, perfmon_result_t::type_map);
RDB_MAKE_SERIALIZABLE_3(perfmon_result_t, type, value_, map_);

#endif /* PERFMON_ARCHIVE_HPP_ */
