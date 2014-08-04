// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_PSEUDO_TIME_HPP_
#define RDB_PROTOCOL_PSEUDO_TIME_HPP_

#include <string>

#include "version.hpp"

template <class> class counted_t;

namespace ql {
class rcheckable_t;
class datum_t;

namespace pseudo {
extern const char *const time_string;

counted_t<const datum_t> iso8601_to_time(
    const std::string &s, const std::string &default_tz, const rcheckable_t *t);
std::string time_to_iso8601(counted_t<const datum_t> d);
double time_to_epoch_time(counted_t<const datum_t> d);

counted_t<const datum_t> time_now();
counted_t<const datum_t> time_tz(counted_t<const datum_t> time);
counted_t<const datum_t> time_in_tz(counted_t<const datum_t> t,
                                    counted_t<const datum_t> tz);

int time_cmp(reql_version_t reql_version, const datum_t &x, const datum_t &y);
void sanitize_time(datum_t *time);
counted_t<const datum_t> make_time(double epoch_time, std::string tz);
counted_t<const datum_t> make_time(
    int year, int month, int day, int hours, int minutes, double seconds,
    std::string tz, const rcheckable_t *target);
counted_t<const datum_t> time_add(
    counted_t<const datum_t> x, counted_t<const datum_t> y);
counted_t<const datum_t> time_sub(
    counted_t<const datum_t> x, counted_t<const datum_t> y);

enum time_component_t {
    YEAR,
    MONTH,
    DAY,
    DAY_OF_WEEK,
    DAY_OF_YEAR,
    HOURS,
    MINUTES,
    SECONDS
};
double time_portion(counted_t<const datum_t> time, time_component_t c);
counted_t<const datum_t> time_date(counted_t<const datum_t> time,
                                   const rcheckable_t *target);
counted_t<const datum_t> time_of_day(counted_t<const datum_t> time);

void time_to_str_key(const datum_t &d, std::string *str_out);

} // namespace pseudo
} // namespace ql

#endif // RDB_PROTOCOL_PSEUDO_TIME_HPP_
