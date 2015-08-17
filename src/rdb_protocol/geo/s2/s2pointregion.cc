// Copyright 2005 Google Inc. All Rights Reserved.

#include "rdb_protocol/geo/s2/s2pointregion.h"
#include "rdb_protocol/geo/s2/base/logging.h"
#include "rdb_protocol/geo/s2/util/coding/coder.h"
#include "rdb_protocol/geo/s2/s2cap.h"
#include "rdb_protocol/geo/s2/s2cell.h"
#include "rdb_protocol/geo/s2/s2latlngrect.h"

namespace geo {

static const unsigned char kCurrentEncodingVersionNumber = 1;

S2PointRegion::~S2PointRegion() {
}

S2PointRegion* S2PointRegion::Clone() const {
  return new S2PointRegion(point_);
}

S2Cap S2PointRegion::GetCapBound() const {
  return S2Cap::FromAxisHeight(point_, 0);
}

S2LatLngRect S2PointRegion::GetRectBound() const {
  S2LatLng ll(point_);
  return S2LatLngRect(ll, ll);
}

bool S2PointRegion::MayIntersect(S2Cell const& cell) const {
  return cell.Contains(point_);
}

void S2PointRegion::Encode(Encoder* encoder) const {
  encoder->Ensure(30);  // sufficient

  encoder->put8(kCurrentEncodingVersionNumber);
  for (int i = 0; i < 3; ++i) {
    encoder->putdouble(point_[i]);
  }
  DCHECK_GE(encoder->avail(), 0);
}

bool S2PointRegion::Decode(Decoder* decoder) {
  unsigned char version = decoder->get8();
  if (version > kCurrentEncodingVersionNumber) return false;

  for (int i = 0; i < 3; ++i) {
    point_[i] = decoder->getdouble();
  }
  DCHECK(S2::IsUnitLength(point_));

  return decoder->avail() >= 0;
}

}  // namespace geo
