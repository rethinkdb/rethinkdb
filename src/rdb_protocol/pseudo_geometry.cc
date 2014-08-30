// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/pseudo_geometry.hpp"

#include <map>
#include <string>

#include "rdb_protocol/geo/exceptions.hpp"
#include "rdb_protocol/geo/geojson.hpp"
#include "rdb_protocol/geo/inclusion.hpp"
#include "rdb_protocol/geo/lat_lon_types.hpp"
#include "rdb_protocol/geo/s2/s2polygon.h"
#include "rdb_protocol/configured_limits.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/error.hpp"

using geo::S2Polygon;

namespace ql {
namespace pseudo {

const char *const geometry_string = "GEOMETRY";

datum_t geo_sub(datum_t lhs,
                datum_t rhs,
                const configured_limits_t &limits) {
    rcheck_target(&lhs, base_exc_t::GENERIC, lhs->is_ptype(geometry_string),
                  "Value must be of geometry type.");
    rcheck_target(&rhs, base_exc_t::GENERIC, rhs->is_ptype(geometry_string),
                  "Value must be of geometry type.");

    rcheck_target(&rhs, base_exc_t::GENERIC,
                  rhs->get_field("coordinates")->arr_size() <= 1,
                  "The second argument to `sub` must be a Polygon with only an outer "
                  "shell.  This one has holes.");

    // Construct a polygon from lhs with rhs cut out
    rcheck_target(&lhs, base_exc_t::GENERIC,
                  lhs->get_field("type")->as_str() == "Polygon",
                  strprintf("The first argument to `sub` must be a Polygon.  Found `%s`.",
                            lhs->get_field("type")->as_str().to_std().c_str()));
    rcheck_target(&lhs, base_exc_t::GENERIC,
                  lhs->get_field("coordinates")->arr_size() >= 1,
                  "The first argument to `sub` is an empty polygon.  It must at least "
                  "have an outer shell.");

    {
        scoped_ptr_t<S2Polygon> lhs_poly = to_s2polygon(lhs);
        if (!geo_does_include(*lhs_poly, rhs)) {
            throw geo_exception_t("The second argument to `sub` is not contained "
                                  "in the first one.");
        }
    }

    datum_object_builder_t result;
    bool dup;
    dup = result.add(datum_t::reql_type_string, datum_t(geometry_string));
    r_sanity_check(!dup);
    dup = result.add("type", datum_t("Polygon"));
    r_sanity_check(!dup);

    datum_array_builder_t coordinates_builder(lhs.get_field("coordinates"), limits);
    coordinates_builder.add(rhs.get_field("coordinates").get(0));
    dup = result.add("coordinates", std::move(coordinates_builder).to_datum());
    r_sanity_check(!dup);
    datum_t result_datum = std::move(result).to_datum();
    validate_geojson(result_datum);

    return result_datum;
}

void sanitize_geometry(datum_t *geo) {
    bool has_type = false;
    bool has_coordinates = false;
    for (size_t i = 0; i < geo->obj_size(); ++i) {
        auto pair = geo->get_pair(i);
        if (pair.first == "coordinates") {
            pair.second.check_type(datum_t::R_ARRAY);
            has_coordinates = true;
        } else if (pair.first == "type") {
            pair.second.check_type(datum_t::R_STR);
            has_type = true;
        } else if (pair.first == "crs") {
        } else if (pair.first == datum_t::reql_type_string) {
        } else {
            rfail_target(&pair.second, base_exc_t::GENERIC,
                         "Unrecognized field `%s` found in geometry object.",
                         pair.first.to_std().c_str());
        }
    }
    rcheck_target(geo, base_exc_t::NON_EXISTENCE, has_type,
                  "Field `type` not found in geometry object.");
    rcheck_target(geo, base_exc_t::NON_EXISTENCE, has_coordinates,
                  "Field `coordinates` not found in geometry object.");
}

} // namespace pseudo
} // namespace ql

