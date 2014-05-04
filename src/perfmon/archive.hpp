// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef PERFMON_ARCHIVE_HPP_
#define PERFMON_ARCHIVE_HPP_

#include <string>
#include <utility>

#include "containers/archive/archive.hpp"
#include "perfmon/core.hpp"

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(perfmon_result_t::perfmon_result_type_t, int8_t, perfmon_result_t::type_value, perfmon_result_t::type_map);

// We have custom-implemented serialization functions because we don't
// want to make a way to support serializing a pointer to
// perfmon_result_t.
void serialize(write_message_t *wm, const perfmon_result_t &x);
archive_result_t deserialize(read_stream_t *s, perfmon_result_t *thing);


#endif /* PERFMON_ARCHIVE_HPP_ */
