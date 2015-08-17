// Copyright 2005 Google Inc. All Rights Reserved.

#include "rdb_protocol/geo/s2/s2region.h"

namespace geo {

S2Region::~S2Region() {
}

bool S2Region::DecodeWithinScope(Decoder* const decoder) {
  return Decode(decoder);
}

}  // namespace geo
