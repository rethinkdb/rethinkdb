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

class intersection_tester_t : public geo_visitor_t {
public:
    intersection_tester_t(const S2Polygon *polygon) : polygon_(polygon) { }

    void on_point(const S2Point &point) {
        // TODO! This doesn't catch points that are *exactly* on the boundary.
        // Is that a problem? Maybe not. It just should be consistent.
        result_ = polygon_->Contains(point);
    }
    void on_line(const S2Polyline &line) {
        std::vector<S2Polyline *> intersecting_pieces;
        polygon_->IntersectWithPolyline(&line, &intersecting_pieces);
        result_ = !intersecting_pieces.empty();
        for (size_t i = 0; i < intersecting_pieces.size(); ++i) {
            delete intersecting_pieces[i];
        }
    }
    void on_polygon(const S2Polygon &polygon) {
        result_ = polygon_->Intersects(&polygon);
    }


    bool get_result() {
        guarantee(result_);
        return result_.get();
    }

private:
    const S2Polygon *polygon_;
    boost::optional<bool> result_;
};

bool geo_does_intersect(const S2Polygon &polygon,
                        const counted_t<const ql::datum_t> &other) {
    intersection_tester_t tester(&polygon);
    visit_geojson(&tester, other);
    return tester.get_result();
}
