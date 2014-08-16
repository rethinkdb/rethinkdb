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

namespace ql {
namespace pseudo {

const char *const geometry_string = "GEOMETRY";

counted_t<const datum_t> geo_sub(counted_t<const datum_t> lhs,
                                 counted_t<const datum_t> rhs,
                                 const configured_limits_t &limits) {
    rcheck_target(lhs.get(), base_exc_t::GENERIC, lhs->is_ptype(geometry_string),
                  "Value must be of geometry type.");
    rcheck_target(rhs.get(), base_exc_t::GENERIC, rhs->is_ptype(geometry_string),
                  "Value must be of geometry type.");

    rcheck_target(rhs.get(), base_exc_t::GENERIC,
                  rhs->get("coordinates")->size() <= 1,
                  "The second argument to `sub` must be a Polygon with only an outer "
                  "shell.  This one has holes.");

    // Construct a polygon from lhs with rhs cut out
    rcheck_target(lhs.get(), base_exc_t::GENERIC,
                  lhs->get("type")->as_str().to_std() == "Polygon",
                  strprintf("The first argument to `sub` must be a Polygon.  Found `%s`.",
                            lhs->get("type")->as_str().c_str()));
    rcheck_target(lhs.get(), base_exc_t::GENERIC,
                  lhs->get("coordinates")->size() >= 1,
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
    dup = result.add(datum_t::reql_type_string, make_counted<datum_t>(geometry_string));
    r_sanity_check(!dup);
    dup = result.add("type", make_counted<datum_t>("Polygon"));
    r_sanity_check(!dup);
    std::vector<counted_t <const datum_t> > coordinates =
        lhs->get("coordinates")->as_array();
    coordinates.push_back(rhs->get("coordinates")->get(0));
    dup = result.add("coordinates",
                     make_counted<datum_t>(std::move(coordinates), limits));
    r_sanity_check(!dup);
    counted_t<const datum_t> result_counted = std::move(result).to_counted();
    validate_geojson(result_counted);

    return result_counted;
}

void sanitize_geometry(datum_t *geo) {
    bool has_type = false;
    bool has_coordinates = false;
    const std::map<std::string, counted_t<const datum_t> > &obj_map =
        geo->as_object();
    for (auto it = obj_map.begin(); it != obj_map.end(); ++it) {
        if (it->first == "coordinates") {
            it->second->check_type(datum_t::R_ARRAY);
            has_coordinates = true;
        } else if (it->first == "type") {
            it->second->check_type(datum_t::R_STR);
            has_type = true;
        } else if (it->first == "crs") {
        } else if (it->first == datum_t::reql_type_string) {
        } else {
            rfail_target(it->second.get(), base_exc_t::GENERIC,
                         "Unrecognized field `%s` found in geometry object.",
                         it->first.c_str());
        }
    }
    rcheck_target(geo, base_exc_t::NON_EXISTENCE, has_type,
                  "Field `type` not found in geometry object.");
    rcheck_target(geo, base_exc_t::NON_EXISTENCE, has_coordinates,
                  "Field `coordinates` not found in geometry object.");
}

} // namespace pseudo
} // namespace ql

