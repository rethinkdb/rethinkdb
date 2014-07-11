// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "geo/intersection.hpp"

#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "geo/geojson.hpp"
#include "geo/geo_visitor.hpp"
#include "geo/s2/s2.h"
#include "geo/s2/s2polygon.h"
#include "geo/s2/s2polyline.h"
#include "rdb_protocol/datum.hpp"

using ql::datum_t;

template<class first_t>
class inner_intersection_tester_t : public s2_geo_visitor_t {
public:
    inner_intersection_tester_t(const first_t *first) : first_(first) { }

    void on_point(const S2Point &point) {
        result_ = geo_does_intersect(*first_, point);
    }
    void on_line(const S2Polyline &line) {
        result_ = geo_does_intersect(*first_, line);
    }
    void on_polygon(const S2Polygon &polygon) {
        result_ = geo_does_intersect(*first_, polygon);
    }

    bool get_result() {
        guarantee(result_);
        return result_.get();
    }

private:
    const first_t *first_;
    boost::optional<bool> result_;
};

class intersection_tester_t : public s2_geo_visitor_t {
public:
    intersection_tester_t(const counted_t<const ql::datum_t> *other) : other_(other) { }

    void on_point(const S2Point &point) {
        inner_intersection_tester_t<S2Point> tester(&point);
        visit_geojson(&tester, *other_);
        result_ = tester.get_result();
    }
    void on_line(const S2Polyline &line) {
        inner_intersection_tester_t<S2Polyline> tester(&line);
        visit_geojson(&tester, *other_);
        result_ = tester.get_result();
    }
    void on_polygon(const S2Polygon &polygon) {
        inner_intersection_tester_t<S2Polygon> tester(&polygon);
        visit_geojson(&tester, *other_);
        result_ = tester.get_result();
    }

    bool get_result() {
        guarantee(result_);
        return result_.get();
    }

private:
    const counted_t<const ql::datum_t> *other_;
    boost::optional<bool> result_;
};

bool geo_does_intersect(const counted_t<const ql::datum_t> &g1,
                        const counted_t<const ql::datum_t> &g2) {
    intersection_tester_t tester(&g2);
    visit_geojson(&tester, g1);
    return tester.get_result();
}

bool geo_does_intersect(const S2Point &point,
                        const S2Point &other_point) {
    return point == other_point;
}

bool geo_does_intersect(const S2Polyline &line,
                        const S2Point &other_point) {
    return geo_does_intersect(other_point, line);
}

bool geo_does_intersect(const S2Polygon &polygon,
                        const S2Point &other_point) {
    return geo_does_intersect(other_point, polygon);
}

// TODO! By the way: Test what happens with empty lines / polygons...
// (i.e. make sure we forbid those)
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

bool geo_does_intersect(const S2Polygon &polygon,
                        const S2Polyline &other_line) {
    return geo_does_intersect(other_line, polygon);
}

bool geo_does_intersect(const S2Point &point,
                        const S2Polygon &other_polygon) {
    // This returns the point itself if it's inside the polygon.
    // In contrast to other_polygon.Contains(), it also works for points that
    // are exactly on the edge of the polygon.
    // That's probably the behavior we want for points (as far as "intersection"
    // on a point makes sense at all).
    S2Point projection = other_polygon.Project(point);
    return projection == point;
}

bool geo_does_intersect(const S2Polyline &line,
                        const S2Polygon &other_polygon) {
    // This does *not* consider the end points of the line.
    // (i.e. if the beginning or end of `line` is exactly on an edge of
    // `other_polygon`)
    // That seems to be consistent with what the term "intersects" means for
    // lines (and polygons).
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
    return other_polygon.Intersects(&polygon);
}