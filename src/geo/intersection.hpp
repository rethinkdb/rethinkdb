// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef GEO_INTERSECTION_HPP_
#define GEO_INTERSECTION_HPP_

#include "containers/counted.hpp"

class S2Polygon;
namespace ql {
class datum_t;
}

// TODO! Should there be a geo namespace?
bool geo_does_intersect(const S2Polygon &polygon,
                        const counted_t<const ql::datum_t> &other);

#endif  // GEO_GEO_VISITOR_HPP_
