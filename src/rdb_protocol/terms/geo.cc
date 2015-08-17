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
#include "rdb_protocol/terms/obj_or_seq.hpp"
#include "rdb_protocol/terms/terms.hpp"

using geo::S2Point;
using geo::S2Polygon;

namespace ql {

/* A term type for geo queries. The only difference to op_term_t is that it catches
`geo_exception_t`s. */
class geo_term_t : public op_term_t {
public:
    geo_term_t(compile_env_t *env, const protob_t<const Term> &term,
               const argspec_t &argspec, optargspec_t optargspec = optargspec_t({}))
        : op_term_t(env, term, argspec, optargspec) { }
private:
    // With the exception of r.point(), all geo terms are non-deterministic
    // because they typically depend on floating point results that might
    // diverge between machines / compilers / libraries.
    // Even seemingly harmless things such as r.line() are affected because they
    // perform geometric validation.
    bool is_deterministic() const { return false; }
    virtual scoped_ptr_t<val_t> eval_geo(
            scope_env_t *env, args_t *args, eval_flags_t flags) const = 0;
    scoped_ptr_t<val_t> eval_impl(
            scope_env_t *env, args_t *args, eval_flags_t flags) const {
        try {
            return eval_geo(env, args, flags);
        } catch (const geo_exception_t &e) {
            rfail(base_exc_t::LOGIC, "%s", e.what());
        }
    }
};

class geo_obj_or_seq_op_term_t : public obj_or_seq_op_term_t {
public:
    geo_obj_or_seq_op_term_t(compile_env_t *env, protob_t<const Term> term,
                             poly_type_t _poly_type, argspec_t argspec)
        : obj_or_seq_op_term_t(env, term, _poly_type, argspec,
                               std::set<std::string>{"GEOMETRY"}) { }
private:
    // See comment in geo_term_t about non-determinism
    bool is_deterministic() const { return false; }
    virtual scoped_ptr_t<val_t> obj_eval_geo(
            scope_env_t *env, args_t *args, const scoped_ptr_t<val_t> &v0) const = 0;
    scoped_ptr_t<val_t> obj_eval(
            scope_env_t *env, args_t *args, const scoped_ptr_t<val_t> &v0) const {
        try {
            return obj_eval_geo(env, args, v0);
        } catch (const geo_exception_t &e) {
            rfail(base_exc_t::LOGIC, "%s", e.what());
        }
    }
};

class geojson_term_t : public geo_term_t {
public:
    geojson_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : geo_term_t(env, term, argspec_t(1)) { }
private:
    scoped_ptr_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        scoped_ptr_t<val_t> v = args->arg(env, 0);
        datum_t geo_json = v->as_datum();
        validate_geojson(geo_json);

        // Store the geo_json object inline, just add a $reql_type$ field
        datum_object_builder_t result(geo_json);
        bool dup = result.add(datum_t::reql_type_string,
                              datum_t(pseudo::geometry_string));
        rcheck(!dup, base_exc_t::LOGIC, "GeoJSON object already had a "
                                          "$reql_type$ field.");
        // Drop the `bbox` field in case it exists. We don't have any use for it.
        UNUSED bool had_bbox = result.delete_field("bbox");

        return new_val(std::move(result).to_datum());
    }
    virtual const char *name() const { return "geojson"; }
};

// to_geojson doesn't actually perform any geometric calculations, nor does it do
// geometry validation. That's why it's derived from op_term_t rather than geo_term_t.
// It's also deterministic.
class to_geojson_term_t : public op_term_t {
public:
    to_geojson_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        scoped_ptr_t<val_t> v = args->arg(env, 0);

        datum_object_builder_t result(v->as_ptype(pseudo::geometry_string));
        bool success = result.delete_field(datum_t::reql_type_string);
        r_sanity_check(success);
        return new_val(std::move(result).to_datum());
    }
    virtual const char *name() const { return "to_geojson"; }
};

class point_term_t : public geo_term_t {
public:
    point_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : geo_term_t(env, term, argspec_t(2)) { }
private:
    // point_term_t is deterministic because it doesn't perform any complex
    // arithmetics. It only checks the range of the values as part of
    // `validate_geojson()`.
    // Note that this is *only* true because S2 doesn't perform any additional
    // checks on the S2Point constructed in validate_geojson(). The construction
    // of the S2Point itself depends on the M_PI constant which could be system
    // dependent. Therefore if anything in validate_geojson() starts using
    // the constructed S2Point some day, determinism will need to be reconsidered.
    bool is_deterministic() const { return true; }
    scoped_ptr_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        double lon = args->arg(env, 0)->as_num();
        double lat = args->arg(env, 1)->as_num();
        lon_lat_point_t point(lon, lat);

        const datum_t result = construct_geo_point(point, env->env->limits());
        validate_geojson(result);

        return new_val(result);
    }
    virtual const char *name() const { return "point"; }
};

// Accepts either a geometry object of type Point, or an array with two coordinates.
// We often want to support both.
lon_lat_point_t parse_point_argument(const datum_t &point_datum) {
    if (point_datum.is_ptype(pseudo::geometry_string)) {
        // The argument is a point (should be at least, if not this will throw)
        return extract_lon_lat_point(point_datum);
    } else {
        // The argument must be a coordinate pair
        rcheck_target(&point_datum,
                      point_datum.arr_size() == 2,
                      base_exc_t::LOGIC,
                      strprintf("Expected point coordinate pair.  "
                                "Got %zu element array instead of a 2 element one.",
                                point_datum.arr_size()));
        double lon = point_datum.get(0).as_num();
        double lat = point_datum.get(1).as_num();
        return lon_lat_point_t(lon, lat);
    }
}

// Used by line_term_t and polygon_term_t
lon_lat_line_t parse_line_from_args(scope_env_t *env, args_t *args) {
    lon_lat_line_t line;
    line.reserve(args->num_args());
    for (size_t i = 0; i < args->num_args(); ++i) {
        scoped_ptr_t<const val_t> point_arg = args->arg(env, i);
        const datum_t &point_datum = point_arg->as_datum();
        line.push_back(parse_point_argument(point_datum));
    }

    return line;
}

class line_term_t : public geo_term_t {
public:
    line_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : geo_term_t(env, term, argspec_t(2, -1)) { }
private:
    scoped_ptr_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        const lon_lat_line_t line = parse_line_from_args(env, args);

        const datum_t result = construct_geo_line(line, env->env->limits());
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
    scoped_ptr_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        const lon_lat_line_t shell = parse_line_from_args(env, args);

        const datum_t result = construct_geo_polygon(shell, env->env->limits());
        validate_geojson(result);

        return new_val(result);
    }
    virtual const char *name() const { return "polygon"; }
};

class intersects_term_t : public geo_obj_or_seq_op_term_t {
public:
    intersects_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : geo_obj_or_seq_op_term_t(env, term, poly_type_t::FILTER, argspec_t(2)) { }
private:
    scoped_ptr_t<val_t> obj_eval_geo(
            scope_env_t *env, args_t *args, const scoped_ptr_t<val_t> &v0) const {
        scoped_ptr_t<val_t> other = args->arg(env, 1);

        bool result = geo_does_intersect(v0->as_ptype(pseudo::geometry_string),
                                         other->as_ptype(pseudo::geometry_string));

        return new_val_bool(result);
    }
    virtual const char *name() const { return "intersects"; }
};

class includes_term_t : public geo_obj_or_seq_op_term_t {
public:
    includes_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : geo_obj_or_seq_op_term_t(env, term, poly_type_t::FILTER, argspec_t(2)) { }
private:
    scoped_ptr_t<val_t> obj_eval_geo(
            scope_env_t *env, args_t *args, const scoped_ptr_t<val_t> &v0) const {
        scoped_ptr_t<val_t> g = args->arg(env, 1);

        scoped_ptr_t<S2Polygon> s2polygon =
            to_s2polygon(v0->as_ptype(pseudo::geometry_string));
        bool result = geo_does_include(*s2polygon, g->as_ptype(pseudo::geometry_string));

        return new_val_bool(result);
    }
    virtual const char *name() const { return "includes"; }
};

ellipsoid_spec_t pick_reference_ellipsoid(scope_env_t *env, args_t *args) {
    scoped_ptr_t<val_t> geo_system_arg = args->optarg(env, "geo_system");
    if (geo_system_arg.has()) {
        if (geo_system_arg->as_datum().get_type() == datum_t::R_OBJECT) {
            // We expect a reference ellipsoid with parameters 'a' and 'f'.
            // (equator radius and the flattening)
            double a = geo_system_arg->as_datum().get_field("a").as_num();
            double f = geo_system_arg->as_datum().get_field("f").as_num();
            rcheck_target(geo_system_arg.get(),
                          a > 0.0,
                          base_exc_t::LOGIC,
                          "The equator radius `a` must be positive.");
            rcheck_target(geo_system_arg.get(),
                          f >= 0.0 && f < 1.0,
                          base_exc_t::LOGIC,
                          "The flattening `f` must be in the range [0, 1).");
            return ellipsoid_spec_t(a, f);
        } else {
            const std::string v = geo_system_arg->as_str().to_std();
            if (v == "WGS84") {
                return WGS84_ELLIPSOID;
            } else if (v == "unit_sphere") {
                return UNIT_SPHERE;
            } else {
                rfail_target(geo_system_arg.get(), base_exc_t::LOGIC,
                             "Unrecognized geo system \"%s\" (valid options: "
                             "\"WGS84\", \"unit_sphere\").", v.c_str());
            }
        }
    } else {
        return WGS84_ELLIPSOID;
    }
}

dist_unit_t pick_dist_unit(scope_env_t *env, args_t *args) {
    scoped_ptr_t<val_t> geo_system_arg = args->optarg(env, "unit");
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
    scoped_ptr_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        scoped_ptr_t<val_t> g1_arg = args->arg(env, 0);
        scoped_ptr_t<val_t> g2_arg = args->arg(env, 1);

        ellipsoid_spec_t reference_ellipsoid = pick_reference_ellipsoid(env, args);
        dist_unit_t result_unit = pick_dist_unit(env, args);

        // (At least) one of the arguments must be a point.
        // Find out which one it is.
        scoped_ptr_t<S2Point> p;
        datum_t g;
        const std::string g1_type =
            g1_arg->as_ptype(pseudo::geometry_string).get_field("type").as_str().to_std();
        if (g1_type == "Point") {
            p = to_s2point(g1_arg->as_ptype(pseudo::geometry_string));
            g = g2_arg->as_ptype(pseudo::geometry_string);
        } else {
            p = to_s2point(g2_arg->as_ptype(pseudo::geometry_string));
            g = g1_arg->as_ptype(pseudo::geometry_string);
        }

        double result = geodesic_distance(*p, g, reference_ellipsoid);
        result = convert_dist_unit(result, dist_unit_t::M, result_unit);

        return new_val(datum_t(result));
    }
    virtual const char *name() const { return "distance"; }
};

class circle_term_t : public geo_term_t {
public:
    circle_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : geo_term_t(env, term, argspec_t(2),
          optargspec_t({"geo_system", "unit", "fill", "num_vertices"})) { }
private:
    scoped_ptr_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        scoped_ptr_t<val_t> center_arg = args->arg(env, 0);
        scoped_ptr_t<val_t> radius_arg = args->arg(env, 1);

        scoped_ptr_t<val_t> fill_arg = args->optarg(env, "fill");
        bool fill = true;
        if (fill_arg.has()) {
            fill = fill_arg->as_bool();
        }
        scoped_ptr_t<val_t> num_vertices_arg = args->optarg(env, "num_vertices");
        unsigned int num_vertices = 32;
        if (num_vertices_arg.has()) {
            num_vertices = num_vertices_arg->as_int<unsigned int>();
            rcheck_target(num_vertices_arg.get(),
                          num_vertices > 0,
                          base_exc_t::LOGIC,
                          "num_vertices must be positive.");
        }

        ellipsoid_spec_t reference_ellipsoid = pick_reference_ellipsoid(env, args);
        dist_unit_t radius_unit = pick_dist_unit(env, args);

        lon_lat_point_t center = parse_point_argument(center_arg->as_datum());
        double radius = radius_arg->as_num();
        radius = convert_dist_unit(radius, radius_unit, dist_unit_t::M);

        const lon_lat_line_t circle =
            build_circle(center, radius, num_vertices, reference_ellipsoid);

        const datum_t result =
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
    scoped_ptr_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> table = args->arg(env, 0)->as_table();
        scoped_ptr_t<val_t> query_arg = args->arg(env, 1);
        scoped_ptr_t<val_t> index = args->optarg(env, "index");
        rcheck(index.has(), base_exc_t::LOGIC,
               "get_intersecting requires an index argument.");
        std::string index_str = index->as_str().to_std();
        rcheck(index_str != table->get_pkey(), base_exc_t::LOGIC,
               "get_intersecting cannot use the primary index.");
        counted_t<datum_stream_t> stream = table->get_intersecting(
            env->env, query_arg->as_ptype(pseudo::geometry_string), index_str,
            this);
        return new_val(make_counted<selection_t>(table, stream));
    }
    virtual const char *name() const { return "get_intersecting"; }
};

class fill_term_t : public geo_term_t {
public:
    fill_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : geo_term_t(env, term, argspec_t(1)) { }
private:
    scoped_ptr_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        scoped_ptr_t<val_t> l_arg = args->arg(env, 0);
        const lon_lat_line_t shell =
            extract_lon_lat_line(l_arg->as_ptype(pseudo::geometry_string));

        const datum_t result = construct_geo_polygon(shell, env->env->limits());
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
    scoped_ptr_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<table_t> table = args->arg(env, 0)->as_table();
        scoped_ptr_t<val_t> center_arg = args->arg(env, 1);
        scoped_ptr_t<val_t> index = args->optarg(env, "index");
        rcheck(index.has(), base_exc_t::LOGIC,
               "get_nearest requires an index argument.");
        std::string index_str = index->as_str().to_std();
        rcheck(index_str != table->get_pkey(), base_exc_t::LOGIC,
               "get_nearest cannot use the primary index.");
        lon_lat_point_t center = parse_point_argument(center_arg->as_datum());
        ellipsoid_spec_t reference_ellipsoid = pick_reference_ellipsoid(env, args);
        dist_unit_t dist_unit = pick_dist_unit(env, args);
        scoped_ptr_t<val_t> max_dist_arg = args->optarg(env, "max_dist");
        double max_dist = 100000; // Default: 100 km
        if (max_dist_arg.has()) {
            max_dist =
                convert_dist_unit(max_dist_arg->as_num(), dist_unit, dist_unit_t::M);
            rcheck_target(max_dist_arg,
                          max_dist > 0.0,
                          base_exc_t::LOGIC,
                          "max_dist must be positive.");
        }
        scoped_ptr_t<val_t> max_results_arg = args->optarg(env, "max_results");
        int64_t max_results = 100; // Default: 100 results
        if (max_results_arg.has()) {
            max_results = max_results_arg->as_int();
            rcheck_target(max_results_arg,
                          max_results > 0,
                          base_exc_t::LOGIC,
                          "max_results must be positive.");
        }

        datum_t results = table->get_nearest(
                env->env, center, max_dist, max_results, reference_ellipsoid,
                dist_unit, index_str, env->env->limits());
        return new_val(results);
    }
    virtual const char *name() const { return "get_nearest"; }
};

class polygon_sub_term_t : public geo_term_t {
public:
    polygon_sub_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : geo_term_t(env, term, argspec_t(2)) { }
private:
    const datum_t check_arg(scoped_ptr_t<val_t> arg) const {
        const datum_t res = arg->as_ptype(pseudo::geometry_string);

        rcheck_target(arg.get(),
                      res.get_field("type").as_str() == "Polygon",
                      base_exc_t::LOGIC,
                      strprintf("Expected a Polygon but found a %s.",
                                res.get_field("type").as_str().to_std().c_str()));
        rcheck_target(arg.get(),
                      res.get_field("coordinates").arr_size() <= 1,
                      base_exc_t::LOGIC,
                      "Expected a Polygon with only an outer shell.  "
                      "This one has holes.");
        return res;
    }

    scoped_ptr_t<val_t> eval_geo(scope_env_t *env, args_t *args, eval_flags_t) const {
        const datum_t lhs = check_arg(args->arg(env, 0));
        const datum_t rhs = check_arg(args->arg(env, 1));

        {
            scoped_ptr_t<S2Polygon> lhs_poly = to_s2polygon(lhs);
            if (!geo_does_include(*lhs_poly, rhs)) {
                throw geo_exception_t("The second argument to `polygon_sub` is not "
                                      "contained in the first one.");
            }
        }

        // Construct a polygon from lhs with rhs cut out
        datum_object_builder_t result;
        bool dup;
        dup = result.add(datum_t::reql_type_string, datum_t(pseudo::geometry_string));
        r_sanity_check(!dup);
        dup = result.add("type", datum_t("Polygon"));
        r_sanity_check(!dup);

        datum_array_builder_t coordinates_builder(lhs.get_field("coordinates"),
                                                  env->env->limits());
        coordinates_builder.add(rhs.get_field("coordinates").get(0));
        dup = result.add("coordinates", std::move(coordinates_builder).to_datum());
        r_sanity_check(!dup);
        datum_t result_datum = std::move(result).to_datum();
        validate_geojson(result_datum);

        return new_val(result_datum);
    }
    virtual const char *name() const { return "polygon_sub"; }
};


counted_t<term_t> make_geojson_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<geojson_term_t>(env, term);
}
counted_t<term_t> make_to_geojson_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<to_geojson_term_t>(env, term);
}
counted_t<term_t> make_point_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<point_term_t>(env, term);
}
counted_t<term_t> make_line_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<line_term_t>(env, term);
}
counted_t<term_t> make_polygon_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<polygon_term_t>(env, term);
}
counted_t<term_t> make_intersects_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<intersects_term_t>(env, term);
}
counted_t<term_t> make_includes_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<includes_term_t>(env, term);
}
counted_t<term_t> make_distance_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<distance_term_t>(env, term);
}
counted_t<term_t> make_circle_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<circle_term_t>(env, term);
}
counted_t<term_t> make_get_intersecting_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<get_intersecting_term_t>(env, term);
}
counted_t<term_t> make_fill_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<fill_term_t>(env, term);
}
counted_t<term_t> make_get_nearest_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<get_nearest_term_t>(env, term);
}
counted_t<term_t> make_polygon_sub_term(
        compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<polygon_sub_term_t>(env, term);
}

} // namespace ql

