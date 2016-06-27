// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/geo/intersection.hpp"

#include <vector>

#include "rdb_protocol/geo/geojson.hpp"
#include "rdb_protocol/geo/geo_visitor.hpp"
#include "rdb_protocol/geo/s2/s2.h"
#include "rdb_protocol/geo/s2/s2polygon.h"
#include "rdb_protocol/geo/s2/s2polyline.h"
#include "rdb_protocol/datum.hpp"

using geo::S2Point;
using geo::S2Polygon;
using geo::S2Polyline;
using geo::S2LatLngRect;
using ql::datum_t;

template<class first_t>
class inner_intersection_tester_t : public s2_geo_visitor_t<bool> {
public:
    explicit inner_intersection_tester_t(const first_t *first) : first_(first) { }

    bool on_point(const S2Point &point) {
        return geo_does_intersect(*first_, point);
    }
    bool on_line(const S2Polyline &line) {
        return geo_does_intersect(*first_, line);
    }
    bool on_polygon(const S2Polygon &polygon) {
        return geo_does_intersect(*first_, polygon);
    }
    bool on_latlngrect(const S2LatLngRect &rect) {
        return geo_does_intersect(*first_, rect);
    }

private:
    const first_t *first_;
};

class intersection_tester_t : public s2_geo_visitor_t<bool> {
public:
    explicit intersection_tester_t(const ql::datum_t *other)
        : other_(other) { }

    bool on_point(const S2Point &point) {
        inner_intersection_tester_t<S2Point> tester(&point);
        return visit_geojson(&tester, *other_);
    }
    bool on_line(const S2Polyline &line) {
        inner_intersection_tester_t<S2Polyline> tester(&line);
        return visit_geojson(&tester, *other_);
    }
    bool on_polygon(const S2Polygon &polygon) {
        inner_intersection_tester_t<S2Polygon> tester(&polygon);
        return visit_geojson(&tester, *other_);
    }
    bool on_latlngrect(const S2LatLngRect &rect) {
        inner_intersection_tester_t<S2LatLngRect> tester(&rect);
        return visit_geojson(&tester, *other_);
    }

private:
    const ql::datum_t *other_;
};

bool geo_does_intersect(const ql::datum_t &g1,
                        const ql::datum_t &g2) {
    intersection_tester_t tester(&g2);
    return visit_geojson(&tester, g1);
}

bool geo_does_intersect(const S2Point &point,
                        const S2Point &other_point) {
    return point == other_point;
}

bool geo_does_intersect(const S2Point &point,
                        const S2Polyline &other_line) {
    // This is probably fragile due to numeric precision limits.
    // On the other hand that should be expected from such an operation.
    int next_vertex;
    S2Point projection = other_line.Project(point, &next_vertex);
    return projection == point;
}

bool geo_does_intersect(const S2Polyline &line,
                        const S2Polyline &other_line) {
    return other_line.Intersects(&line);
}

bool geo_does_intersect(const S2Point &point,
                        const S2Polygon &other_polygon) {
    // First, handle case where we have a polygon with no edges.
    if (other_polygon.num_vertices() == 0) {
        return false;
    }
    // This returns the point itself if it's inside the polygon.
    // In contrast to other_polygon.Contains(), it also works for points that
    // are exactly on the corner of the polygon.
    // That's probably the behavior we want for points (as far as "intersection"
    // on a point makes sense at all).
    S2Point projection = other_polygon.Project(point);
    return projection == point;
}

bool geo_does_intersect(const S2Polyline &line,
                        const S2Polygon &other_polygon) {
    // First, handle case where we have a polygon with no edges.
    if (other_polygon.num_vertices() == 0) {
        return false;
    }
    std::vector<S2Polyline *> intersecting_pieces;
    other_polygon.IntersectWithPolyline(&line, &intersecting_pieces);
    // We're not interested in the actual line pieces that intersect
    for (size_t i = 0; i < intersecting_pieces.size(); ++i) {
        delete intersecting_pieces[i];
    }
    return !intersecting_pieces.empty();
}

bool geo_does_intersect(const S2Polygon &polygon,
                        const S2Polygon &other_polygon) {
    // First, handle case where we have a polygon with no edges.
    if (polygon.num_vertices() == 0) {
        return false;
    }
    if (other_polygon.num_vertices() == 0) {
        return false;
    }
    return other_polygon.Intersects(&polygon);
}

bool geo_does_intersect(const geo::S2LatLngRect &rect,
                        const geo::S2LatLngRect &other_rect) {
    return rect.Intersects(other_rect);
}

bool geo_does_intersect(const geo::S2LatLngRect &rect,
                        const geo::S2Point &other_point) {
    return rect.Contains(other_point);
}

bool geo_does_intersect(const geo::S2LatLngRect &rect,
                        const geo::S2Polyline &other_line) {
    // This can generate false positives. That is ok since we only use LatLngRects as
    // part of our changefeed code where we perform additional post-filtering.
    // Right now ReQL doesn't expose LatLngRects. If that ever changes, we will have
    // to split this into a separate `MayIntersect` term, explicitly disallow
    // `intersects` on LatLngRects, or come up with an exact implementation for this.
    return rect.Intersects(other_line.GetRectBound());
}

bool geo_does_intersect(const geo::S2LatLngRect &rect,
                        const geo::S2Polygon &other_polygon) {
    // This can generate false positives. That is ok since we only use LatLngRects as
    // part of our changefeed code where we perform additional post-filtering.
    // Right now ReQL doesn't expose LatLngRects. If that ever changes, we will have
    // to split this into a separate `MayIntersect` term, explicitly disallow
    // `intersects` on LatLngRects, or come up with an exact implementation for this.
    return rect.Intersects(other_polygon.GetRectBound());
}
