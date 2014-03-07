// Copyright 2012 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef V8_DATE_H_
#define V8_DATE_H_

#include "allocation.h"
#include "globals.h"
#include "platform.h"


namespace v8 {
namespace internal {

class DateCache {
 public:
  static const int kMsPerMin = 60 * 1000;
  static const int kSecPerDay = 24 * 60 * 60;
  static const int64_t kMsPerDay = kSecPerDay * 1000;

  // The largest time that can be passed to OS date-time library functions.
  static const int kMaxEpochTimeInSec = kMaxInt;
  static const int64_t kMaxEpochTimeInMs =
      static_cast<int64_t>(kMaxInt) * 1000;

  // The largest time that can be stored in JSDate.
  static const int64_t kMaxTimeInMs =
      static_cast<int64_t>(864000000) * 10000000;

  // Conservative upper bound on time that can be stored in JSDate
  // before UTC conversion.
  static const int64_t kMaxTimeBeforeUTCInMs =
      kMaxTimeInMs + 10 * kMsPerDay;

  // Sentinel that denotes an invalid local offset.
  static const int kInvalidLocalOffsetInMs = kMaxInt;
  // Sentinel that denotes an invalid cache stamp.
  // It is an invariant of DateCache that cache stamp is non-negative.
  static const int kInvalidStamp = -1;

  DateCache() : stamp_(0) {
    ResetDateCache();
  }

  virtual ~DateCache() {}


  // Clears cached timezone information and increments the cache stamp.
  void ResetDateCache();


  // Computes floor(time_ms / kMsPerDay).
  static int DaysFromTime(int64_t time_ms) {
    if (time_ms < 0) time_ms -= (kMsPerDay - 1);
    return static_cast<int>(time_ms / kMsPerDay);
  }


  // Computes modulo(time_ms, kMsPerDay) given that
  // days = floor(time_ms / kMsPerDay).
  static int TimeInDay(int64_t time_ms, int days) {
    return static_cast<int>(time_ms - days * kMsPerDay);
  }


  // Given the number of days since the epoch, computes the weekday.
  // ECMA 262 - 15.9.1.6.
  int Weekday(int days) {
    int result = (days + 4) % 7;
    return result >= 0 ? result : result + 7;
  }


  bool IsLeap(int year) {
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
  }


  // ECMA 262 - 15.9.1.7.
  int LocalOffsetInMs() {
    if (local_offset_ms_ == kInvalidLocalOffsetInMs)  {
      local_offset_ms_ = GetLocalOffsetFromOS();
    }
    return local_offset_ms_;
  }


  const char* LocalTimezone(int64_t time_ms) {
    if (time_ms < 0 || time_ms > kMaxEpochTimeInMs) {
      time_ms = EquivalentTime(time_ms);
    }
    return OS::LocalTimezone(static_cast<double>(time_ms));
  }

  // ECMA 262 - 15.9.5.26
  int TimezoneOffset(int64_t time_ms) {
    int64_t local_ms = ToLocal(time_ms);
    return static_cast<int>((time_ms - local_ms) / kMsPerMin);
  }

  // ECMA 262 - 15.9.1.9
  int64_t ToLocal(int64_t time_ms) {
    return time_ms + LocalOffsetInMs() + DaylightSavingsOffsetInMs(time_ms);
  }

  // ECMA 262 - 15.9.1.9
  int64_t ToUTC(int64_t time_ms) {
    time_ms -= LocalOffsetInMs();
    return time_ms - DaylightSavingsOffsetInMs(time_ms);
  }


  // Computes a time equivalent to the given time according
  // to ECMA 262 - 15.9.1.9.
  // The issue here is that some library calls don't work right for dates
  // that cannot be represented using a non-negative signed 32 bit integer
  // (measured in whole seconds based on the 1970 epoch).
  // We solve this by mapping the time to a year with same leap-year-ness
  // and same starting day for the year. The ECMAscript specification says
  // we must do this, but for compatibility with other browsers, we use
  // the actual year if it is in the range 1970..2037
  int64_t EquivalentTime(int64_t time_ms) {
    int days = DaysFromTime(time_ms);
    int time_within_day_ms = static_cast<int>(time_ms - days * kMsPerDay);
    int year, month, day;
    YearMonthDayFromDays(days, &year, &month, &day);
    int new_days = DaysFromYearMonth(EquivalentYear(year), month) + day - 1;
    return static_cast<int64_t>(new_days) * kMsPerDay + time_within_day_ms;
  }

  // Returns an equivalent year in the range [2008-2035] matching
  // - leap year,
  // - week day of first day.
  // ECMA 262 - 15.9.1.9.
  int EquivalentYear(int year) {
    int week_day = Weekday(DaysFromYearMonth(year, 0));
    int recent_year = (IsLeap(year) ? 1956 : 1967) + (week_day * 12) % 28;
    // Find the year in the range 2008..2037 that is equivalent mod 28.
    // Add 3*28 to give a positive argument to the modulus operator.
    return 2008 + (recent_year + 3 * 28 - 2008) % 28;
  }

  // Given the number of days since the epoch, computes
  // the corresponding year, month, and day.
  void YearMonthDayFromDays(int days, int* year, int* month, int* day);

  // Computes the number of days since the epoch for
  // the first day of the given month in the given year.
  int DaysFromYearMonth(int year, int month);

  // Cache stamp is used for invalidating caches in JSDate.
  // We increment the stamp each time when the timezone information changes.
  // JSDate objects perform stamp check and invalidate their caches if
  // their saved stamp is not equal to the current stamp.
  Smi* stamp() { return stamp_; }
  void* stamp_address() { return &stamp_; }

  // These functions are virtual so that we can override them when testing.
  virtual int GetDaylightSavingsOffsetFromOS(int64_t time_sec) {
    double time_ms = static_cast<double>(time_sec * 1000);
    return static_cast<int>(OS::DaylightSavingsOffset(time_ms));
  }

  virtual int GetLocalOffsetFromOS() {
    double offset = OS::LocalTimeOffset();
    ASSERT(offset < kInvalidLocalOffsetInMs);
    return static_cast<int>(offset);
  }

 private:
  // The implementation relies on the fact that no time zones have
  // more than one daylight savings offset change per 19 days.
  // In Egypt in 2010 they decided to suspend DST during Ramadan. This
  // led to a short interval where DST is in effect from September 10 to
  // September 30.
  static const int kDefaultDSTDeltaInSec = 19 * kSecPerDay;

  // Size of the Daylight Savings Time cache.
  static const int kDSTSize = 32;

  // Daylight Savings Time segment stores a segment of time where
  // daylight savings offset does not change.
  struct DST {
    int start_sec;
    int end_sec;
    int offset_ms;
    int last_used;
  };

  // Computes the daylight savings offset for the given time.
  // ECMA 262 - 15.9.1.8
  int DaylightSavingsOffsetInMs(int64_t time_ms);

  // Sets the before_ and the after_ segments from the DST cache such that
  // the before_ segment starts earlier than the given time and
  // the after_ segment start later than the given time.
  // Both segments might be invalid.
  // The last_used counters of the before_ and after_ are updated.
  void ProbeDST(int time_sec);

  // Finds the least recently used segment from the DST cache that is not
  // equal to the given 'skip' segment.
  DST* LeastRecentlyUsedDST(DST* skip);

  // Extends the after_ segment with the given point or resets it
  // if it starts later than the given time + kDefaultDSTDeltaInSec.
  inline void ExtendTheAfterSegment(int time_sec, int offset_ms);

  // Makes the given segment invalid.
  inline void ClearSegment(DST* segment);

  bool InvalidSegment(DST* segment) {
    return segment->start_sec > segment->end_sec;
  }

  Smi* stamp_;

  // Daylight Saving Time cache.
  DST dst_[kDSTSize];
  int dst_usage_counter_;
  DST* before_;
  DST* after_;

  int local_offset_ms_;

  // Year/Month/Day cache.
  bool ymd_valid_;
  int ymd_days_;
  int ymd_year_;
  int ymd_month_;
  int ymd_day_;
};

} }   // namespace v8::internal

#endif
