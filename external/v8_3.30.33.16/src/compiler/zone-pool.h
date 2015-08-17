// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_COMPILER_ZONE_POOL_H_
#define V8_COMPILER_ZONE_POOL_H_

#include <map>
#include <set>
#include <vector>

#include "src/v8.h"

namespace v8 {
namespace internal {
namespace compiler {

class ZonePool FINAL {
 public:
  class Scope FINAL {
   public:
    explicit Scope(ZonePool* zone_pool) : zone_pool_(zone_pool), zone_(NULL) {}
    ~Scope() { Destroy(); }

    Zone* zone() {
      if (zone_ == NULL) zone_ = zone_pool_->NewEmptyZone();
      return zone_;
    }
    void Destroy() {
      if (zone_ != NULL) zone_pool_->ReturnZone(zone_);
      zone_ = NULL;
    }

   private:
    ZonePool* const zone_pool_;
    Zone* zone_;
    DISALLOW_COPY_AND_ASSIGN(Scope);
  };

  class StatsScope FINAL {
   public:
    explicit StatsScope(ZonePool* zone_pool);
    ~StatsScope();

    size_t GetMaxAllocatedBytes();
    size_t GetCurrentAllocatedBytes();
    size_t GetTotalAllocatedBytes();

   private:
    friend class ZonePool;
    void ZoneReturned(Zone* zone);

    typedef std::map<Zone*, size_t> InitialValues;

    ZonePool* const zone_pool_;
    InitialValues initial_values_;
    size_t total_allocated_bytes_at_start_;
    size_t max_allocated_bytes_;

    DISALLOW_COPY_AND_ASSIGN(StatsScope);
  };

  explicit ZonePool(Isolate* isolate);
  ~ZonePool();

  size_t GetMaxAllocatedBytes();
  size_t GetTotalAllocatedBytes();
  size_t GetCurrentAllocatedBytes();

 private:
  Zone* NewEmptyZone();
  void ReturnZone(Zone* zone);

  static const size_t kMaxUnusedSize = 3;
  typedef std::vector<Zone*> Unused;
  typedef std::vector<Zone*> Used;
  typedef std::vector<StatsScope*> Stats;

  Isolate* const isolate_;
  Unused unused_;
  Used used_;
  Stats stats_;
  size_t max_allocated_bytes_;
  size_t total_deleted_bytes_;

  DISALLOW_COPY_AND_ASSIGN(ZonePool);
};

}  // namespace compiler
}  // namespace internal
}  // namespace v8

#endif
