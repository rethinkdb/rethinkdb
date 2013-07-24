// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_PSEUOD_TIME_HPP_
#define RDB_PROTOCOL_PSEUDO_TIME_HPP_

#include <boost/date_time.hpp>

#include "rdb_protocol/env.hpp"

namespace ql {
namespace pseudo {
extern const char *const time_string;

counted_t<const datum_t> iso8601_to_time(const std::string &s, const rcheckable_t *t);
std::string time_to_iso8601(counted_t<const datum_t> d);
double time_to_epoch_time(counted_t<const datum_t> d);

int time_cmp(const datum_t& x, const datum_t& y);
bool time_valid(const datum_t &time);
counted_t<const datum_t> make_time(double epoch_time, std::string tz="");
counted_t<const datum_t> time_add(
    counted_t<const datum_t> x, counted_t<const datum_t> y);
counted_t<const datum_t> time_sub(
    counted_t<const datum_t> x, counted_t<const datum_t> y);
} //namespace pseudo
} //namespace ql

#endif //RDB_PROTOCOL_PSEUDO_TIME_HPP_
