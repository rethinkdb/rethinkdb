// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef GEO_GEOJSON_HPP_
#define GEO_GEOJSON_HPP_

#include "containers/counted.hpp"

class geo_visitor_t;
namespace ql {
class datum_t;
}


void visit_geojson(geo_visitor_t *visitor, const counted_t<const ql::datum_t> &geojson);

#endif  // GEO_GEO_VISITOR_HPP_
