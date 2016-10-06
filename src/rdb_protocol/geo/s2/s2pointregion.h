// Copyright 2005 Google Inc. All Rights Reserved.

#ifndef UTIL_GEOMETRY_S2POINTREGION_H__
#define UTIL_GEOMETRY_S2POINTREGION_H__

#include "rdb_protocol/geo/s2/base/logging.h"
#include "rdb_protocol/geo/s2/base/macros.h"
#include "rdb_protocol/geo/s2/s2.h"
#include "rdb_protocol/geo/s2/s2region.h"
#include "utils.hpp"

namespace geo {

// An S2PointRegion is a region that contains a single point.  It is more
// expensive than the raw S2Point type and is useful mainly for completeness.
class S2PointRegion : public S2Region {
 public:
  // Create a region containing the given point, which must be unit length.
  inline explicit S2PointRegion(S2Point const& point);

  ~S2PointRegion();

  S2Point const& point() const { return point_; }

  ////////////////////////////////////////////////////////////////////////
  // S2Region interface (see s2region.h for details):

  virtual S2PointRegion* Clone() const;
  virtual S2Cap GetCapBound() const;
  virtual S2LatLngRect GetRectBound() const;
  virtual bool Contains(UNUSED S2Cell const& cell) const { return false; }
  virtual bool MayIntersect(S2Cell const& cell) const;
  virtual bool VirtualContainsPoint(S2Point const& p) const {
    return Contains(p);
  }
  bool Contains(S2Point const& p) const { return (point_ == p); }
  virtual void Encode(Encoder* const encoder) const;
  virtual bool Decode(Decoder* const decoder);

 private:
  S2Point point_;

  DISALLOW_EVIL_CONSTRUCTORS(S2PointRegion);
};

S2PointRegion::S2PointRegion(S2Point const& _point) : point_(_point) {
  DCHECK(S2::IsUnitLength(_point));
}

}  // namespace geo

#endif  // UTIL_GEOMETRY_S2POINTREGION_H__
