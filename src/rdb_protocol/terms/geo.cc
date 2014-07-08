// Copyright 2010-2014 RethinkDB, all rights reserved.

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/terms/terms.hpp"
//#include "rdb_protocol/pseudo_geo.hpp" // TODO!

namespace ql {

class geojson_term_t : public op_term_t {
public:
    geojson_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<val_t> v = args->arg(env, 0);
        counted_t<const datum_t> geo_json = v->as_datum();

        // Store the geo_json object inline, just add a $reql_type$ field
        datum_ptr_t type_obj((datum_t::R_OBJECT));
        bool success = type_obj.add("$reql_type$", datum_ptr_t("geometry").to_counted());
        r_sanity_check(success);
        counted_t<const datum_t> result = type_obj->merge(geo_json);

        // TODO! Validate
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
        bool success = result.delete_field("$reql_type");
        r_sanity_check(success); // TODO! Assumes we verify the type beforehand...
        return new_val(result.to_counted());
    }
    virtual const char *name() const { return "to_geojson"; }
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
    virtual const char *name() const { return "to_geojson"; }
};*/

counted_t<term_t> make_geojson_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<geojson_term_t>(env, term);
}
counted_t<term_t> make_to_geojson_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<to_geojson_term_t>(env, term);
}

} // namespace ql

