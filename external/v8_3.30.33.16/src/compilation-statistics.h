// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_COMPILATION_STATISTICS_H_
#define V8_COMPILATION_STATISTICS_H_

#include <map>
#include <string>

#include "src/allocation.h"
#include "src/base/platform/time.h"

namespace v8 {
namespace internal {

class CompilationInfo;

class CompilationStatistics FINAL : public Malloced {
 public:
  CompilationStatistics() {}

  class BasicStats {
   public:
    BasicStats()
        : total_allocated_bytes_(0),
          max_allocated_bytes_(0),
          absolute_max_allocated_bytes_(0) {}

    void Accumulate(const BasicStats& stats);

    base::TimeDelta delta_;
    size_t total_allocated_bytes_;
    size_t max_allocated_bytes_;
    size_t absolute_max_allocated_bytes_;
    std::string function_name_;
  };

  void RecordPhaseStats(const char* phase_kind_name, const char* phase_name,
                        const BasicStats& stats);

  void RecordPhaseKindStats(const char* phase_kind_name,
                            const BasicStats& stats);

  void RecordTotalStats(size_t source_size, const BasicStats& stats);

 private:
  class TotalStats : public BasicStats {
   public:
    TotalStats() : source_size_(0) {}
    uint64_t source_size_;
  };

  class OrderedStats : public BasicStats {
   public:
    explicit OrderedStats(size_t insert_order) : insert_order_(insert_order) {}
    size_t insert_order_;
  };

  class PhaseStats : public OrderedStats {
   public:
    PhaseStats(size_t insert_order, const char* phase_kind_name)
        : OrderedStats(insert_order), phase_kind_name_(phase_kind_name) {}
    std::string phase_kind_name_;
  };

  friend std::ostream& operator<<(std::ostream& os,
                                  const CompilationStatistics& s);

  typedef OrderedStats PhaseKindStats;
  typedef std::map<std::string, PhaseKindStats> PhaseKindMap;
  typedef std::map<std::string, PhaseStats> PhaseMap;

  TotalStats total_stats_;
  PhaseKindMap phase_kind_map_;
  PhaseMap phase_map_;

  DISALLOW_COPY_AND_ASSIGN(CompilationStatistics);
};

std::ostream& operator<<(std::ostream& os, const CompilationStatistics& s);

}  // namespace internal
}  // namespace v8

#endif
