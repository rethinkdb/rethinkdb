// Copyright 2005 Google Inc. All Rights Reserved.

#include <math.h>
#include <stdio.h>
#include <iostream>

#include "rdb_protocol/geo/s2/s1angle.h"
#include "rdb_protocol/geo/s2/s2latlng.h"

namespace geo {
using std::ostream;
using std::cout;
using std::endl;

S1Angle::S1Angle(S2Point const& x, S2Point const& y)
    : radians_(x.Angle(y)) {
}

S1Angle::S1Angle(S2LatLng const& x, S2LatLng const& y)
    : radians_(x.GetDistance(y).radians()) {
}

S1Angle S1Angle::Normalized() const {
  S1Angle a(radians_);
  a.Normalize();
  return a;
}

void S1Angle::Normalize() {
  radians_ = remainder(radians_, 2.0 * M_PI);
  if (radians_ <= -M_PI) radians_ = M_PI;
}

ostream& operator<<(ostream& os, S1Angle const& a) {
  double degrees = a.degrees();
  char buffer[13];
  int sz = snprintf(buffer, sizeof(buffer), "%.7f", degrees);
  if (sz >= 0 && static_cast<size_t>(sz) < sizeof(buffer)) {
    return os << buffer;
  } else {
    return os << degrees;
  }
}
}  // namespace geo
