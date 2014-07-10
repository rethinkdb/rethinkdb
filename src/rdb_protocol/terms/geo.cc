// Copyright 2010-2014 RethinkDB, all rights reserved.

#include "geo/distances.hpp"
#include "geo/ellipsoid.hpp"
#include "geo/exceptions.hpp"
#include "geo/geojson.hpp"
#include "geo/intersection.hpp"
#include "geo/primitives.hpp"
#include "geo/s2/s2polygon.h"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/terms/terms.hpp"

namespace ql {

class geojson_term_t : public op_term_t {
public:
    geojson_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<val_t> v = args->arg(env, 0);
        counted_t<const datum_t> geo_json = v->as_datum();
        try {
            validate_geojson(geo_json);
        } catch (const geo_exception_t &e) {
            rfail(base_exc_t::GENERIC, "%s", e.what());
        }

        // Store the geo_json object inline, just add a $reql_type$ field
        datum_ptr_t type_obj((datum_t::R_OBJECT));
        bool dup = type_obj.add("$reql_type$", datum_ptr_t("geometry").to_counted());
        rcheck(!dup, base_exc_t::GENERIC, "GeoJSON object contained a $reql_type$ field");
        counted_t<const datum_t> result = type_obj->merge(geo_json);

        return new_val(result);
    }
    virtual const char *name() const { return "geo_json"; }
};

class to_geojson_term_t : public op_term_t {
public:
    to_geojson_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<val_t> v = args->arg(env, 0);
        // TODO! Make sure that the object is a geometry-typed object

        datum_ptr_t result(v->as_datum()->as_object());
        bool success = result.delete_field("$reql_type$");
        r_sanity_check(success); // TODO! Assumes we verify the type beforehand...
        return new_val(result.to_counted());
    }
    virtual const char *name() const { return "to_geojson"; }
};

class point_term_t : public op_term_t {
public:
    point_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        double lat = args->arg(env, 0)->as_num();
        double lon = args->arg(env, 1)->as_num();
        lat_lon_point_t point(lat, lon);

        try {
            const counted_t<const datum_t> result = construct_geo_point(point);
            validate_geojson(result);

            return new_val(result);
        } catch (const geo_exception_t &e) {
            rfail(base_exc_t::GENERIC, "%s", e.what());
        }
    }
    virtual const char *name() const { return "point"; }
};

// Used by line_term_t and polygon_term_t
lat_lon_line_t parse_line_from_args(scope_env_t *env, args_t *args) {
    // TODO! Validation (range etc.)
    lat_lon_line_t line;
    line.reserve(args->num_args());
    for (size_t i = 0; i < args->num_args(); ++i) {
        const counted_t<const datum_t> &point_arg = args->arg(env, i)->as_datum();
        if (point_arg->is_ptype("geometry")) {
            // The argument is a point
            // TODO! get its coordinate vector and append it
        } else {
            // The argument must be a coordinate pair
            const std::vector<counted_t<const datum_t> > &point_arr =
                point_arg->as_array();
            if (point_arr.size() != 2) {
                // TODO!
                crash("Expected point coordinate pair in line constructor. "
                      "Got TODO element array instead of a 2 element one.");
            }
            double lat = point_arr[0]->as_num();
            double lon = point_arr[1]->as_num();
            line.push_back(lat_lon_point_t(lat, lon));
        }
    }

    return line;
}

class line_term_t : public op_term_t {
public:
    line_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2, -1)) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        const lat_lon_line_t line = parse_line_from_args(env, args);

        try {
            const counted_t<const datum_t> result = construct_geo_line(line);
            validate_geojson(result);

            return new_val(result);
        } catch (const geo_exception_t &e) {
            rfail(base_exc_t::GENERIC, "%s", e.what());
        }
    }
    virtual const char *name() const { return "line"; }
};

class polygon_term_t : public op_term_t {
public:
    polygon_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(3, -1)) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        const lat_lon_line_t shell = parse_line_from_args(env, args);

        try {
            const counted_t<const datum_t> result = construct_geo_polygon(shell);
            validate_geojson(result);

            return new_val(result);
        } catch (const geo_exception_t &e) {
            rfail(base_exc_t::GENERIC, "%s", e.what());
        }
    }
    virtual const char *name() const { return "polygon"; }
};

class intersects_term_t : public op_term_t {
public:
    intersects_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<val_t> poly = args->arg(env, 0);
        counted_t<val_t> g = args->arg(env, 1);
        // TODO! Type validation

        try {
            // TODO! Support intersection tests with lines
            scoped_ptr_t<S2Polygon> s2poly = to_s2polygon(poly->as_datum());

            bool result = geo_does_intersect(*s2poly, g->as_datum());

            return new_val(make_counted<const datum_t>(datum_t::R_BOOL, result));
        } catch (const geo_exception_t &e) {
            rfail(base_exc_t::GENERIC, "%s", e.what());
        }
    }
    virtual const char *name() const { return "intersects"; }
};

class distance_term_t : public op_term_t {
public:
    distance_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        // TODO! Support opt-args
        // TODO! Support polygons / lines maybe?
        counted_t<val_t> p1 = args->arg(env, 0);
        counted_t<val_t> p2 = args->arg(env, 1);
        // TODO! Type validation

        try {
            // TODO! This is just a quick hack for testing.
            lat_lon_point_t llp1(p1->as_datum()->get("coordinates")->get(1)->as_num(),
                                 p1->as_datum()->get("coordinates")->get(0)->as_num());
            lat_lon_point_t llp2(p2->as_datum()->get("coordinates")->get(1)->as_num(),
                                 p2->as_datum()->get("coordinates")->get(0)->as_num());

            double result = karney_distance(llp1, llp2, WGS84_ELLIPSOID);

            return new_val(make_counted<const datum_t>(result));
        } catch (const geo_exception_t &e) {
            rfail(base_exc_t::GENERIC, "%s", e.what());
        }
    }
    virtual const char *name() const { return "distance"; }
};

class circle_term_t : public op_term_t {
public:
    circle_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        // TODO! Support opt-args (distance things, num_vertices, fill)
        // TODO! Support polygons / lines maybe?
        counted_t<val_t> center = args->arg(env, 0);
        counted_t<val_t> radius = args->arg(env, 1);
        // TODO! Type validation

        // TODO! Opt-args
        bool fill = true;
        unsigned int num_vertices = 20;

        try {
            // TODO! This is just a quick hack for testing.
            lat_lon_point_t llcenter(center->as_datum()->get("coordinates")->get(1)->as_num(),
                                     center->as_datum()->get("coordinates")->get(0)->as_num());

            const lat_lon_line_t circle =
                build_circle(llcenter, radius->as_num(), num_vertices, WGS84_ELLIPSOID);

            const counted_t<const datum_t> result =
                fill
                ? construct_geo_polygon(circle)
                : construct_geo_line(circle);
            validate_geojson(result);

            return new_val(result);
        } catch (const geo_exception_t &e) {
            rfail(base_exc_t::GENERIC, "%s", e.what());
        }
    }
    virtual const char *name() const { return "circle"; }
};

/*
class includes_term_t : public op_term_t {
public:
    includes_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<val_t> poly = args->arg(env, 0);
        counted_t<val_t> g = args->arg(env, 1);
        // TODO! Type validation

        // TODO! Use visitor
        S2Polygon poly_s2 = polygon_to_s2(poly);
        bool result;
        if (g_type == "Polygon") {
            S2Polygon g_poly = polygon_to_s2(g);
            result = poly_s2.Contains(g_poly);
        } else if (g_type == "LineString") {
            S2Polyline g_line = line_to_s2(g);
            // poly_s2 contains g_line, iff
            // - Any of the points of poly_s2 is in g_line,
            // - and g_line does not intersect with any of the outlines of g_poly
            // TODO!
            r_sanity_check(false);
            result = false;
        } else if (g_type == "Point") {
            S2Point g_point = point_to_s2(g);
            result = poly_s2.Contains(g_point);
        } else {
            // TODO! Handle properly
            r_sanity_check(false);
            result = false;
        }

        return new_val(result.to_counted());
    }
    virtual const char *name() const { return "includes"; }
};*/

counted_t<term_t> make_geojson_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<geojson_term_t>(env, term);
}
counted_t<term_t> make_to_geojson_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<to_geojson_term_t>(env, term);
}
counted_t<term_t> make_point_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<point_term_t>(env, term);
}
counted_t<term_t> make_line_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<line_term_t>(env, term);
}
counted_t<term_t> make_polygon_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<polygon_term_t>(env, term);
}
counted_t<term_t> make_intersects_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<intersects_term_t>(env, term);
}
counted_t<term_t> make_distance_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<distance_term_t>(env, term);
}
counted_t<term_t> make_circle_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<circle_term_t>(env, term);
}

} // namespace ql

