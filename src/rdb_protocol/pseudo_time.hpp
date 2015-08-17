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

datum_t iso8601_to_time(
    const std::string &s, const std::string &default_tz, const rcheckable_t *t);
std::string time_to_iso8601(datum_t d);
double time_to_epoch_time(datum_t d);

datum_t time_now();
datum_t time_tz(datum_t time);
datum_t time_in_tz(datum_t t, datum_t tz);

int time_cmp(const datum_t &x, const datum_t &y);
void sanitize_time(datum_t *time);
datum_t make_time(double epoch_time, std::string tz);
datum_t make_time(
    int year, int month, int day, int hours, int minutes, double seconds,
    std::string tz, const rcheckable_t *target);
datum_t time_add(
    datum_t x, datum_t y);
datum_t time_sub(
    datum_t x, datum_t y);

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
double time_portion(datum_t time, time_component_t c);
datum_t time_date(datum_t time,
                                   const rcheckable_t *target);
datum_t time_of_day(datum_t time);

void time_to_str_key(const datum_t &d, std::string *str_out);

} // namespace pseudo
} // namespace ql

#endif // RDB_PROTOCOL_PSEUDO_TIME_HPP_
