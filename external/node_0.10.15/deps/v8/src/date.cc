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

#include "date.h"

#include "v8.h"

#include "objects.h"
#include "objects-inl.h"

namespace v8 {
namespace internal {


static const int kDays4Years[] = {0, 365, 2 * 365, 3 * 365 + 1};
static const int kDaysIn4Years = 4 * 365 + 1;
static const int kDaysIn100Years = 25 * kDaysIn4Years - 1;
static const int kDaysIn400Years = 4 * kDaysIn100Years + 1;
static const int kDays1970to2000 = 30 * 365 + 7;
static const int kDaysOffset = 1000 * kDaysIn400Years + 5 * kDaysIn400Years -
                               kDays1970to2000;
static const int kYearsOffset = 400000;
static const char kDaysInMonths[] =
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};


void DateCache::ResetDateCache() {
  static const int kMaxStamp = Smi::kMaxValue;
  stamp_ = Smi::FromInt(stamp_->value() + 1);
  if (stamp_->value() > kMaxStamp) {
    stamp_ = Smi::FromInt(0);
  }
  ASSERT(stamp_ != Smi::FromInt(kInvalidStamp));
  for (int i = 0; i < kDSTSize; ++i) {
    ClearSegment(&dst_[i]);
  }
  dst_usage_counter_ = 0;
  before_ = &dst_[0];
  after_ = &dst_[1];
  local_offset_ms_ = kInvalidLocalOffsetInMs;
  ymd_valid_ = false;
}


void DateCache::ClearSegment(DST* segment) {
  segment->start_sec = kMaxEpochTimeInSec;
  segment->end_sec = -kMaxEpochTimeInSec;
  segment->offset_ms = 0;
  segment->last_used = 0;
}


void DateCache::YearMonthDayFromDays(
    int days, int* year, int* month, int* day) {
  if (ymd_valid_) {
    // Check conservatively if the given 'days' has
    // the same year and month as the cached 'days'.
    int new_day = ymd_day_ + (days - ymd_days_);
    if (new_day >= 1 && new_day <= 28) {
      ymd_day_ = new_day;
      ymd_days_ = days;
      *year = ymd_year_;
      *month = ymd_month_;
      *day = new_day;
      return;
    }
  }
  int save_days = days;

  days += kDaysOffset;
  *year = 400 * (days / kDaysIn400Years) - kYearsOffset;
  days %= kDaysIn400Years;

  ASSERT(DaysFromYearMonth(*year, 0) + days == save_days);

  days--;
  int yd1 = days / kDaysIn100Years;
  days %= kDaysIn100Years;
  *year += 100 * yd1;

  days++;
  int yd2 = days / kDaysIn4Years;
  days %= kDaysIn4Years;
  *year += 4 * yd2;

  days--;
  int yd3 = days / 365;
  days %= 365;
  *year += yd3;


  bool is_leap = (!yd1 || yd2) && !yd3;

  ASSERT(days >= -1);
  ASSERT(is_leap || (days >= 0));
  ASSERT((days < 365) || (is_leap && (days < 366)));
  ASSERT(is_leap == ((*year % 4 == 0) && (*year % 100 || (*year % 400 == 0))));
  ASSERT(is_leap || ((DaysFromYearMonth(*year, 0) + days) == save_days));
  ASSERT(!is_leap || ((DaysFromYearMonth(*year, 0) + days + 1) == save_days));

  days += is_leap;

  // Check if the date is after February.
  if (days >= 31 + 28 + is_leap) {
    days -= 31 + 28 + is_leap;
    // Find the date starting from March.
    for (int i = 2; i < 12; i++) {
      if (days < kDaysInMonths[i]) {
        *month = i;
        *day = days + 1;
        break;
      }
      days -= kDaysInMonths[i];
    }
  } else {
    // Check January and February.
    if (days < 31) {
      *month = 0;
      *day = days + 1;
    } else {
      *month = 1;
      *day = days - 31 + 1;
    }
  }
  ASSERT(DaysFromYearMonth(*year, *month) + *day - 1 == save_days);
  ymd_valid_ = true;
  ymd_year_ = *year;
  ymd_month_ = *month;
  ymd_day_ = *day;
  ymd_days_ = save_days;
}


int DateCache::DaysFromYearMonth(int year, int month) {
  static const int day_from_month[] = {0, 31, 59, 90, 120, 151,
                                       181, 212, 243, 273, 304, 334};
  static const int day_from_month_leap[] = {0, 31, 60, 91, 121, 152,
                                            182, 213, 244, 274, 305, 335};

  year += month / 12;
  month %= 12;
  if (month < 0) {
    year--;
    month += 12;
  }

  ASSERT(month >= 0);
  ASSERT(month < 12);

  // year_delta is an arbitrary number such that:
  // a) year_delta = -1 (mod 400)
  // b) year + year_delta > 0 for years in the range defined by
  //    ECMA 262 - 15.9.1.1, i.e. upto 100,000,000 days on either side of
  //    Jan 1 1970. This is required so that we don't run into integer
  //    division of negative numbers.
  // c) there shouldn't be an overflow for 32-bit integers in the following
  //    operations.
  static const int year_delta = 399999;
  static const int base_day = 365 * (1970 + year_delta) +
                              (1970 + year_delta) / 4 -
                              (1970 + year_delta) / 100 +
                              (1970 + year_delta) / 400;

  int year1 = year + year_delta;
  int day_from_year = 365 * year1 +
                      year1 / 4 -
                      year1 / 100 +
                      year1 / 400 -
                      base_day;

  if ((year % 4 != 0) || (year % 100 == 0 && year % 400 != 0)) {
    return day_from_year + day_from_month[month];
  }
  return day_from_year + day_from_month_leap[month];
}


void DateCache::ExtendTheAfterSegment(int time_sec, int offset_ms) {
  if (after_->offset_ms == offset_ms &&
      after_->start_sec <= time_sec + kDefaultDSTDeltaInSec &&
      time_sec <= after_->end_sec) {
    // Extend the after_ segment.
    after_->start_sec = time_sec;
  } else {
    // The after_ segment is either invalid or starts too late.
    if (after_->start_sec <= after_->end_sec) {
      // If the after_ segment is valid, replace it with a new segment.
      after_ = LeastRecentlyUsedDST(before_);
    }
    after_->start_sec = time_sec;
    after_->end_sec = time_sec;
    after_->offset_ms = offset_ms;
    after_->last_used = ++dst_usage_counter_;
  }
}


int DateCache::DaylightSavingsOffsetInMs(int64_t time_ms) {
  int time_sec = (time_ms >= 0 && time_ms <= kMaxEpochTimeInMs)
      ? static_cast<int>(time_ms / 1000)
      : static_cast<int>(EquivalentTime(time_ms) / 1000);

  // Invalidate cache if the usage counter is close to overflow.
  // Note that dst_usage_counter is incremented less than ten times
  // in this function.
  if (dst_usage_counter_ >= kMaxInt - 10) {
    dst_usage_counter_ = 0;
    for (int i = 0; i < kDSTSize; ++i) {
      ClearSegment(&dst_[i]);
    }
  }

  // Optimistic fast check.
  if (before_->start_sec <= time_sec &&
      time_sec <= before_->end_sec) {
    // Cache hit.
    before_->last_used = ++dst_usage_counter_;
    return before_->offset_ms;
  }

  ProbeDST(time_sec);

  ASSERT(InvalidSegment(before_) || before_->start_sec <= time_sec);
  ASSERT(InvalidSegment(after_) || time_sec < after_->start_sec);

  if (InvalidSegment(before_)) {
    // Cache miss.
    before_->start_sec = time_sec;
    before_->end_sec = time_sec;
    before_->offset_ms = GetDaylightSavingsOffsetFromOS(time_sec);
    before_->last_used = ++dst_usage_counter_;
    return before_->offset_ms;
  }

  if (time_sec <= before_->end_sec) {
    // Cache hit.
    before_->last_used = ++dst_usage_counter_;
    return before_->offset_ms;
  }

  if (time_sec > before_->end_sec + kDefaultDSTDeltaInSec) {
    // If the before_ segment ends too early, then just
    // query for the offset of the time_sec
    int offset_ms = GetDaylightSavingsOffsetFromOS(time_sec);
    ExtendTheAfterSegment(time_sec, offset_ms);
    // This swap helps the optimistic fast check in subsequent invocations.
    DST* temp = before_;
    before_ = after_;
    after_ = temp;
    return offset_ms;
  }

  // Now the time_sec is between
  // before_->end_sec and before_->end_sec + default DST delta.
  // Update the usage counter of before_ since it is going to be used.
  before_->last_used = ++dst_usage_counter_;

  // Check if after_ segment is invalid or starts too late.
  // Note that start_sec of invalid segments is kMaxEpochTimeInSec.
  if (before_->end_sec + kDefaultDSTDeltaInSec <= after_->start_sec) {
    int new_after_start_sec = before_->end_sec + kDefaultDSTDeltaInSec;
    int new_offset_ms = GetDaylightSavingsOffsetFromOS(new_after_start_sec);
    ExtendTheAfterSegment(new_after_start_sec, new_offset_ms);
  } else {
    ASSERT(!InvalidSegment(after_));
    // Update the usage counter of after_ since it is going to be used.
    after_->last_used = ++dst_usage_counter_;
  }

  // Now the time_sec is between before_->end_sec and after_->start_sec.
  // Only one daylight savings offset change can occur in this interval.

  if (before_->offset_ms == after_->offset_ms) {
    // Merge two segments if they have the same offset.
    before_->end_sec = after_->end_sec;
    ClearSegment(after_);
    return before_->offset_ms;
  }

  // Binary search for daylight savings offset change point,
  // but give up if we don't find it in four iterations.
  for (int i = 4; i >= 0; --i) {
    int delta = after_->start_sec - before_->end_sec;
    int middle_sec = (i == 0) ? time_sec : before_->end_sec + delta / 2;
    int offset_ms = GetDaylightSavingsOffsetFromOS(middle_sec);
    if (before_->offset_ms == offset_ms) {
      before_->end_sec = middle_sec;
      if (time_sec <= before_->end_sec) {
        return offset_ms;
      }
    } else {
      ASSERT(after_->offset_ms == offset_ms);
      after_->start_sec = middle_sec;
      if (time_sec >= after_->start_sec) {
        // This swap helps the optimistic fast check in subsequent invocations.
        DST* temp = before_;
        before_ = after_;
        after_ = temp;
        return offset_ms;
      }
    }
  }
  UNREACHABLE();
  return 0;
}


void DateCache::ProbeDST(int time_sec) {
  DST* before = NULL;
  DST* after = NULL;
  ASSERT(before_ != after_);

  for (int i = 0; i < kDSTSize; ++i) {
    if (dst_[i].start_sec <= time_sec) {
      if (before == NULL || before->start_sec < dst_[i].start_sec) {
        before = &dst_[i];
      }
    } else if (time_sec < dst_[i].end_sec) {
      if (after == NULL || after->end_sec > dst_[i].end_sec) {
        after = &dst_[i];
      }
    }
  }

  // If before or after segments were not found,
  // then set them to any invalid segment.
  if (before == NULL) {
    before = InvalidSegment(before_) ? before_ : LeastRecentlyUsedDST(after);
  }
  if (after == NULL) {
    after = InvalidSegment(after_) && before != after_
            ? after_ : LeastRecentlyUsedDST(before);
  }

  ASSERT(before != NULL);
  ASSERT(after != NULL);
  ASSERT(before != after);
  ASSERT(InvalidSegment(before) || before->start_sec <= time_sec);
  ASSERT(InvalidSegment(after) || time_sec < after->start_sec);
  ASSERT(InvalidSegment(before) || InvalidSegment(after) ||
         before->end_sec < after->start_sec);

  before_ = before;
  after_ = after;
}


DateCache::DST* DateCache::LeastRecentlyUsedDST(DST* skip) {
  DST* result = NULL;
  for (int i = 0; i < kDSTSize; ++i) {
    if (&dst_[i] == skip) continue;
    if (result == NULL || result->last_used > dst_[i].last_used) {
      result = &dst_[i];
    }
  }
  ClearSegment(result);
  return result;
}

} }  // namespace v8::internal
