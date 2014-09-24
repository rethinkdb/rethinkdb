// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/geo/distances.hpp"

#include "rdb_protocol/geo/ellipsoid.hpp"
#include "rdb_protocol/geo/exceptions.hpp"
#include "rdb_protocol/geo/geojson.hpp"
#include "rdb_protocol/geo/geo_visitor.hpp"
#include "rdb_protocol/geo/karney/geodesic.h"
#include "rdb_protocol/geo/s2/s2.h"
#include "rdb_protocol/geo/s2/s2latlng.h"
#include "rdb_protocol/geo/s2/s2polygon.h"
#include "rdb_protocol/geo/s2/s2polyline.h"

double geodesic_distance(const lon_lat_point_t &p1,
                         const lon_lat_point_t &p2,
                         const ellipsoid_spec_t &e) {
    // Use Karney's algorithm
    struct geod_geodesic g;
    geod_init(&g, e.equator_radius(), e.flattening());

    double dist;
    geod_inverse(&g, p1.latitude, p1.longitude, p2.latitude, p2.longitude, &dist, NULL, NULL);

    return dist;
}

double geodesic_distance(const geo::S2Point &p,
                         const ql::datum_t &g,
                         const ellipsoid_spec_t &e) {
    class distance_estimator_t : public s2_geo_visitor_t<double> {
    public:
        distance_estimator_t(
                lon_lat_point_t r, const geo::S2Point &r_s2, const ellipsoid_spec_t &_e)
            : ref_(r), ref_s2_(r_s2), e_(_e) { }
        double on_point(const geo::S2Point &point) {
            lon_lat_point_t llpoint =
                lon_lat_point_t(geo::S2LatLng::Longitude(point).degrees(),
                                geo::S2LatLng::Latitude(point).degrees());
            return geodesic_distance(ref_, llpoint, e_);
        }
        double on_line(const geo::S2Polyline &line) {
            // This sometimes over-estimates large distances, because the
            // projection assumes spherical rather than ellipsoid geometry.
            int next_vertex;
            geo::S2Point prj = line.Project(ref_s2_, &next_vertex);
            if (prj == ref_s2_) {
                // ref_ is on the line
                return 0.0;
            } else {
                lon_lat_point_t llprj =
                    lon_lat_point_t(geo::S2LatLng::Longitude(prj).degrees(),
                                    geo::S2LatLng::Latitude(prj).degrees());
                return geodesic_distance(ref_, llprj, e_);
            }
        }
        double on_polygon(const geo::S2Polygon &polygon) {
            // This sometimes over-estimates large distances, because the
            // projection assumes spherical rather than ellipsoid geometry.
            geo::S2Point prj = polygon.Project(ref_s2_);
            if (prj == ref_s2_) {
                // ref_ is inside/on the polygon
                return 0.0;
            } else {
                lon_lat_point_t llprj =
                    lon_lat_point_t(geo::S2LatLng::Longitude(prj).degrees(),
                                    geo::S2LatLng::Latitude(prj).degrees());
                return geodesic_distance(ref_, llprj, e_);
            }
        }
        lon_lat_point_t ref_;
        const geo::S2Point &ref_s2_;
        const ellipsoid_spec_t &e_;
    };
    distance_estimator_t estimator(
            lon_lat_point_t(geo::S2LatLng::Longitude(p).degrees(),
                            geo::S2LatLng::Latitude(p).degrees()),
        p, e);
    return visit_geojson(&estimator, g);
}

lon_lat_point_t geodesic_point_at_dist(const lon_lat_point_t &p,
                                       double dist,
                                       double azimuth,
                                       const ellipsoid_spec_t &e) {
    // Use Karney's algorithm
    struct geod_geodesic g;
    geod_init(&g, e.equator_radius(), e.flattening());

    double lat, lon;
    geod_direct(&g, p.latitude, p.longitude, azimuth, dist, &lat, &lon, NULL);

    return lon_lat_point_t(lon, lat);
}

dist_unit_t parse_dist_unit(const std::string &s) {
    if (s == "m") {
        return dist_unit_t::M;
    } else if (s == "km") {
        return dist_unit_t::KM;
    } else if (s == "mi") {
        return dist_unit_t::MI;
    } else if (s == "nm") {
        return dist_unit_t::NM;
    } else if (s == "ft") {
        return dist_unit_t::FT;
    } else {
        throw geo_exception_t("Unrecognized distance unit `" + s + "`.");
    }
}

double unit_to_meters(dist_unit_t u) {
    switch (u) {
        case dist_unit_t::M: return 1.0;
        case dist_unit_t::KM: return 1000.0;
        case dist_unit_t::MI: return 1609.344;
        case dist_unit_t::NM: return 1852.0;
        case dist_unit_t::FT: return 0.3048;
        default: unreachable();
    }
}

double convert_dist_unit(double d, dist_unit_t from, dist_unit_t to) {
    // First convert to meters
    double conv_factor = unit_to_meters(from);

    // Then to the result unit
    conv_factor /= unit_to_meters(to);

    return d * conv_factor;
}
