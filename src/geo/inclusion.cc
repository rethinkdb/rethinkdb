// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "geo/inclusion.hpp"

#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "geo/geojson.hpp"
#include "geo/geo_visitor.hpp"
#include "geo/intersection.hpp"
#include "geo/s2/s2.h"
#include "geo/s2/s2polygon.h"
#include "geo/s2/s2polyline.h"
#include "rdb_protocol/datum.hpp"

using ql::datum_t;

class inclusion_tester_t : public s2_geo_visitor_t<bool> {
public:
    explicit inclusion_tester_t(const S2Polygon *polygon) : polygon_(polygon) { }

    bool on_point(const S2Point &g) {
        return geo_does_include(*polygon_, g);
    }
    bool on_line(const S2Polyline &g) {
        return geo_does_include(*polygon_, g);
    }
    bool on_polygon(const S2Polygon &g) {
        return geo_does_include(*polygon_, g);
    }
private:
    const S2Polygon *polygon_;
};

bool geo_does_include(const S2Polygon &polygon,
                      const counted_t<const ql::datum_t> &g) {
    inclusion_tester_t tester(&polygon);
    return visit_geojson(&tester, g);
}

bool geo_does_include(const S2Polygon &polygon,
                      const S2Point &g) {
    // polygon.Contains(g) doesn't work for points that are exactly on a corner
    // of the polygon, so we use geo_does_intersect instead.
    return geo_does_intersect(polygon, g);
}

bool geo_does_include(const S2Polygon &polygon,
                      const S2Polyline &g) {
    std::vector<S2Polyline *> remaining_pieces;
    polygon.SubtractFromPolyline(&g, &remaining_pieces);
    // We're not interested in the actual line pieces that are not contained
    for (size_t i = 0; i < remaining_pieces.size(); ++i) {
        delete remaining_pieces[i];
    }
    return remaining_pieces.empty();
}

bool geo_does_include(const S2Polygon &polygon,
                      const S2Polygon &g) {
    return polygon.Contains(&g);
}
