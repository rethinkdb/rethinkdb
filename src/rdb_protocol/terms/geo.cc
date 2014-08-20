// Copyright 2010-2014 RethinkDB, all rights reserved.

#include <limits>

#include "rdb_protocol/geo/distances.hpp"
#include "rdb_protocol/geo/ellipsoid.hpp"
#include "rdb_protocol/geo/exceptions.hpp"
#include "rdb_protocol/geo/geojson.hpp"
#include "rdb_protocol/geo/inclusion.hpp"
#include "rdb_protocol/geo/intersection.hpp"
#include "rdb_protocol/geo/primitives.hpp"
#include "rdb_protocol/geo/s2/s2polygon.h"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/pseudo_geometry.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/terms/terms.hpp"

namespace ql {

/* A term type for geo queries. The only difference to op_term_t is that it catches
`geo_exception_t`s. */
class geo_term_t : public op_term_t {
public:
    geo_term_t(compile_env_t *env, const protob_t<const Term> &term,
               const argspec_t &argspec, optargspec_t optargspec = optargspec_t({}))
        : op_term_t(env, term, argspec, optargspec) { }
private:
    virtual counted_t<val_t> eval_geo(
            scope_env_t *env, args_t *args, eval_flags_t flags) const = 0;
    counted_t<val_t> eval_impl(
            scope_env_t *env, args_t *args, eval_flags_t flags) const {
        try {
            return eval_geo(env, args, flags);
        } catch (const geo_exception_t &e) {
            rfail(base_exc_t::GENERIC, "%s", e.what());
        }
    }
};

class geojson_term_t : public geo_term_t {
public:
    geojson_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : geo_term_t(env, term, argspec_t(1)) { }
private:
    counted_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<val_t> v = args->arg(env, 0);
        counted_t<const datum_t> geo_json = v->as_datum();
        validate_geojson(geo_json);

        // Store the geo_json object inline, just add a $reql_type$ field
        datum_object_builder_t result(geo_json->as_object());
        bool dup = result.add(datum_t::reql_type_string,
                              make_counted<datum_t>(pseudo::geometry_string));
        rcheck(!dup, base_exc_t::GENERIC, "GeoJSON object already had a "
                                          "$reql_type$ field.");
        // Drop the `bbox` field in case it exists. We don't have any use for it.
        UNUSED bool had_bbox = result.delete_field("bbox");

        return new_val(std::move(result).to_counted());
    }
    virtual const char *name() const { return "geojson"; }
};

class to_geojson_term_t : public geo_term_t {
public:
    to_geojson_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : geo_term_t(env, term, argspec_t(1)) { }
private:
    counted_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<val_t> v = args->arg(env, 0);

        datum_object_builder_t result(v->as_ptype(pseudo::geometry_string)->as_object());
        bool success = result.delete_field(datum_t::reql_type_string);
        r_sanity_check(success);
        return new_val(std::move(result).to_counted());
    }
    virtual const char *name() const { return "to_geojson"; }
};

class point_term_t : public geo_term_t {
public:
    point_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : geo_term_t(env, term, argspec_t(2)) { }
private:
    counted_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        double lat = args->arg(env, 0)->as_num();
        double lon = args->arg(env, 1)->as_num();
        lat_lon_point_t point(lat, lon);

        const counted_t<const datum_t> result =
            construct_geo_point(point, env->env->limits());
        validate_geojson(result);

        return new_val(result);
    }
    virtual const char *name() const { return "point"; }
};

// Accepts either a geometry object of type Point, or an array with two coordinates.
// We often want to support both.
lat_lon_point_t parse_point_argument(const counted_t<const datum_t> &point_datum) {
    if (point_datum->is_ptype(pseudo::geometry_string)) {
        // The argument is a point (should be at least, if not this will throw)
        return extract_lat_lon_point(point_datum);
    } else {
        // The argument must be a coordinate pair
        rcheck_target(point_datum.get(), base_exc_t::GENERIC, point_datum->size() == 2,
            strprintf("Expected point coordinate pair.  "
                      "Got %zu element array instead of a 2 element one.",
                      point_datum->size()));
        double lat = point_datum->get(0)->as_num();
        double lon = point_datum->get(1)->as_num();
        return lat_lon_point_t(lat, lon);
    }
}

// Used by line_term_t and polygon_term_t
lat_lon_line_t parse_line_from_args(scope_env_t *env, args_t *args) {
    lat_lon_line_t line;
    line.reserve(args->num_args());
    for (size_t i = 0; i < args->num_args(); ++i) {
        counted_t<const val_t> point_arg = args->arg(env, i);
        const counted_t<const datum_t> &point_datum = point_arg->as_datum();
        line.push_back(parse_point_argument(point_datum));
    }

    return line;
}

class line_term_t : public geo_term_t {
public:
    line_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : geo_term_t(env, term, argspec_t(2, -1)) { }
private:
    counted_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        const lat_lon_line_t line = parse_line_from_args(env, args);

        const counted_t<const datum_t> result =
            construct_geo_line(line, env->env->limits());
        validate_geojson(result);

        return new_val(result);
    }
    virtual const char *name() const { return "line"; }
};

class polygon_term_t : public geo_term_t {
public:
    polygon_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : geo_term_t(env, term, argspec_t(3, -1)) { }
private:
    counted_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        const lat_lon_line_t shell = parse_line_from_args(env, args);

        const counted_t<const datum_t> result =
            construct_geo_polygon(shell, env->env->limits());
        validate_geojson(result);

        return new_val(result);
    }
    virtual const char *name() const { return "polygon"; }
};

class intersects_term_t : public geo_term_t {
public:
    intersects_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : geo_term_t(env, term, argspec_t(2)) { }
private:
    counted_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<val_t> g1 = args->arg(env, 0);
        counted_t<val_t> g2 = args->arg(env, 1);

        bool result = geo_does_intersect(g1->as_ptype(pseudo::geometry_string),
                                         g2->as_ptype(pseudo::geometry_string));

        return new_val(
            make_counted<const datum_t>(datum_t::construct_boolean_t(), result));
    }
    virtual const char *name() const { return "intersects"; }
};

class includes_term_t : public geo_term_t {
public:
    includes_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : geo_term_t(env, term, argspec_t(2)) { }
private:
    counted_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<val_t> polygon = args->arg(env, 0);
        counted_t<val_t> g = args->arg(env, 1);

        scoped_ptr_t<S2Polygon> s2polygon =
            to_s2polygon(polygon->as_ptype(pseudo::geometry_string));
        bool result = geo_does_include(*s2polygon, g->as_ptype(pseudo::geometry_string));

        return new_val(
            make_counted<const datum_t>(datum_t::construct_boolean_t(), result));
    }
    virtual const char *name() const { return "includes"; }
};

ellipsoid_spec_t pick_reference_ellipsoid(scope_env_t *env, args_t *args) {
    counted_t<val_t> geo_system_arg = args->optarg(env, "geo_system");
    if (geo_system_arg.has()) {
        if (geo_system_arg->as_datum()->get_type() == datum_t::R_OBJECT) {
            // We expect a reference ellipsoid with parameters 'a' and 'f'.
            // (equator radius and the flattening)
            double a = geo_system_arg->as_datum()->get("a")->as_num();
            double f = geo_system_arg->as_datum()->get("f")->as_num();
            rcheck_target(geo_system_arg.get(), base_exc_t::GENERIC,
                          a > 0.0, "The equator radius `a` must be positive.");
            rcheck_target(geo_system_arg.get(), base_exc_t::GENERIC,
                          f >= 0.0 && f < 1.0,
                          "The flattening `f` must be in the range [0, 1).");
            return ellipsoid_spec_t(a, f);
        } else {
            const std::string v = geo_system_arg->as_str().to_std();
            if (v == "WGS84") {
                return WGS84_ELLIPSOID;
            } else if (v == "unit_sphere") {
                return UNIT_SPHERE;
            } else {
                rfail_target(geo_system_arg.get(), base_exc_t::GENERIC,
                             "Unrecognized geo system \"%s\" (valid options: "
                             "\"WGS84\", \"unit_sphere\").", v.c_str());
            }
        }
    } else {
        return WGS84_ELLIPSOID;
    }
}

dist_unit_t pick_dist_unit(scope_env_t *env, args_t *args) {
    counted_t<val_t> geo_system_arg = args->optarg(env, "unit");
    if (geo_system_arg.has()) {
        return parse_dist_unit(geo_system_arg->as_str().to_std());
    } else {
        return dist_unit_t::M;
    }
}

class distance_term_t : public geo_term_t {
public:
    distance_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : geo_term_t(env, term, argspec_t(2), optargspec_t({"geo_system", "unit"})) { }
private:
    counted_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<val_t> g1_arg = args->arg(env, 0);
        counted_t<val_t> g2_arg = args->arg(env, 1);

        ellipsoid_spec_t reference_ellipsoid = pick_reference_ellipsoid(env, args);
        dist_unit_t result_unit = pick_dist_unit(env, args);

        // (At least) one of the arguments must be a point.
        // Find out which one it is.
        scoped_ptr_t<S2Point> p;
        counted_t<const datum_t> g;
        const std::string g1_type =
            g1_arg->as_ptype(pseudo::geometry_string)->get("type")->as_str().to_std();
        if (g1_type == "Point") {
            p = to_s2point(g1_arg->as_ptype(pseudo::geometry_string));
            g = g2_arg->as_ptype(pseudo::geometry_string);
        } else {
            p = to_s2point(g2_arg->as_ptype(pseudo::geometry_string));
            g = g1_arg->as_ptype(pseudo::geometry_string);
        }

        double result = geodesic_distance(*p, g, reference_ellipsoid);
        result = convert_dist_unit(result, dist_unit_t::M, result_unit);

        return new_val(make_counted<const datum_t>(result));
    }
    virtual const char *name() const { return "distance"; }
};

class circle_term_t : public geo_term_t {
public:
    circle_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : geo_term_t(env, term, argspec_t(2),
          optargspec_t({"geo_system", "unit", "fill", "num_vertices"})) { }
private:
    counted_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<val_t> center_arg = args->arg(env, 0);
        counted_t<val_t> radius_arg = args->arg(env, 1);

        counted_t<val_t> fill_arg = args->optarg(env, "fill");
        bool fill = true;
        if (fill_arg.has()) {
            fill = fill_arg->as_bool();
        }
        counted_t<val_t> num_vertices_arg = args->optarg(env, "num_vertices");
        unsigned int num_vertices = 32;
        if (num_vertices_arg.has()) {
            num_vertices = num_vertices_arg->as_int<unsigned int>();
            rcheck_target(num_vertices_arg.get(), base_exc_t::GENERIC, num_vertices > 0,
                          "num_vertices must be positive.");
        }

        ellipsoid_spec_t reference_ellipsoid = pick_reference_ellipsoid(env, args);
        dist_unit_t radius_unit = pick_dist_unit(env, args);

        lat_lon_point_t center = parse_point_argument(center_arg->as_datum());
        double radius = radius_arg->as_num();
        radius = convert_dist_unit(radius, radius_unit, dist_unit_t::M);

        const lat_lon_line_t circle =
            build_circle(center, radius, num_vertices, reference_ellipsoid);

        const counted_t<const datum_t> result =
            fill
            ? construct_geo_polygon(circle, env->env->limits())
            : construct_geo_line(circle, env->env->limits());
        validate_geojson(result);

        return new_val(result);
    }
    virtual const char *name() const { return "circle"; }
};

class get_intersecting_term_t : public geo_term_t {
public:
    get_intersecting_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : geo_term_t(env, term, argspec_t(2), optargspec_t({ "index" })) { }
private:
    counted_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> table = args->arg(env, 0)->as_table();
        counted_t<val_t> query_arg = args->arg(env, 1);
        counted_t<val_t> index = args->optarg(env, "index");
        if (!index.has()) {
            rfail(base_exc_t::GENERIC, "get_intersecting requires an index argument.");
        }
        std::string index_str = index->as_str().to_std();

        counted_t<datum_stream_t> stream = table->get_intersecting(
            env->env, query_arg->as_ptype(pseudo::geometry_string), index_str,
            this);
        return new_val(stream, table);
    }
    virtual const char *name() const { return "get_intersecting"; }
};

class fill_term_t : public geo_term_t {
public:
    fill_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : geo_term_t(env, term, argspec_t(1)) { }
private:
    counted_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<val_t> l_arg = args->arg(env, 0);
        const lat_lon_line_t shell =
            extract_lat_lon_line(l_arg->as_ptype(pseudo::geometry_string));

        const counted_t<const datum_t> result =
            construct_geo_polygon(shell, env->env->limits());
        validate_geojson(result);

        return new_val(result);
    }
    virtual const char *name() const { return "fill"; }
};

class get_nearest_term_t : public geo_term_t {
public:
    get_nearest_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : geo_term_t(env, term, argspec_t(2),
          optargspec_t({ "index", "max_results", "max_dist", "geo_system", "unit" })) { }
private:
    counted_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> table = args->arg(env, 0)->as_table();
        counted_t<val_t> center_arg = args->arg(env, 1);
        counted_t<val_t> index = args->optarg(env, "index");
        if (!index.has()) {
            rfail(base_exc_t::GENERIC, "get_nearest requires an index argument.");
        }
        std::string index_str = index->as_str().to_std();
        lat_lon_point_t center = parse_point_argument(center_arg->as_datum());
        ellipsoid_spec_t reference_ellipsoid = pick_reference_ellipsoid(env, args);
        dist_unit_t dist_unit = pick_dist_unit(env, args);
        counted_t<val_t> max_dist_arg = args->optarg(env, "max_dist");
        double max_dist = 100000; // Default: 100 km
        if (max_dist_arg.has()) {
            max_dist =
                convert_dist_unit(max_dist_arg->as_num(), dist_unit, dist_unit_t::M);
            rcheck_target(max_dist_arg, base_exc_t::GENERIC, max_dist > 0.0,
                          "max_dist must be positive.");
        }
        counted_t<val_t> max_results_arg = args->optarg(env, "max_results");
        int64_t max_results = 100; // Default: 100 results
        if (max_results_arg.has()) {
            max_results = max_results_arg->as_int();
            rcheck_target(max_results_arg, base_exc_t::GENERIC, max_results > 0,
                          "max_results must be positive.");
        }

        counted_t<datum_stream_t> stream = table->get_nearest(
                env->env, center, max_dist, max_results, reference_ellipsoid,
                dist_unit, index_str, this, env->env->limits());
        return new_val(stream, table);
    }
    virtual const char *name() const { return "get_nearest"; }
};

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
counted_t<term_t> make_includes_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<includes_term_t>(env, term);
}
counted_t<term_t> make_distance_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<distance_term_t>(env, term);
}
counted_t<term_t> make_circle_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<circle_term_t>(env, term);
}
counted_t<term_t> make_get_intersecting_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<get_intersecting_term_t>(env, term);
}
counted_t<term_t> make_fill_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<fill_term_t>(env, term);
}
counted_t<term_t> make_get_nearest_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<get_nearest_term_t>(env, term);
}

} // namespace ql

