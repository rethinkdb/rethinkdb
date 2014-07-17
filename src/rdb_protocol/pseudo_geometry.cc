// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/pseudo_geometry.hpp"

#include "geo/exceptions.hpp"
#include "geo/geojson.hpp"
#include "geo/inclusion.hpp"
#include "geo/lat_lon_types.hpp"
#include "geo/s2/s2polygon.h"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {
namespace pseudo {

const char *const geometry_string = "GEOMETRY";

counted_t<const datum_t> geo_sub(counted_t<const datum_t> lhs,
                                 counted_t<const datum_t> rhs) {
    rcheck_target(lhs.get(), base_exc_t::GENERIC, lhs->is_ptype(geometry_string),
                  "Value must be of geometry type.");
    rcheck_target(rhs.get(), base_exc_t::GENERIC, rhs->is_ptype(geometry_string),
                  "Value must be of geometry type.");

    const lat_lon_line_t shell = extract_lat_lon_shell(rhs);
    rcheck_target(rhs.get(), base_exc_t::GENERIC,
                  rhs->get("coordinates")->size() <= 1,
                  "The subtrahend must be a Polygon with only an outer shell. "
                  "This one has holes.");

    // Construct a polygon from lhs with rhs cut out
    rcheck_target(lhs.get(), base_exc_t::GENERIC,
                  lhs->get("type")->as_str().to_std() == "Polygon",
                  strprintf("The minuend must be a Polygon. Got %s",
                            lhs->get("type")->as_str().c_str()));
    rcheck_target(lhs.get(), base_exc_t::GENERIC,
                  lhs->get("coordinates")->size() >= 1,
                  "The minuend is an empty polygon. It must at least have "
                  "an outer shell.");

    {
        scoped_ptr_t<S2Polygon> lhs_poly = to_s2polygon(lhs);
        if (!geo_does_include(*lhs_poly, rhs)) {
            throw geo_exception_t("The subtrahend is not contained in the minuend.");
        }
    }

    datum_ptr_t result(datum_t::R_OBJECT);
    bool dup;
    dup = result.add(datum_t::reql_type_string, make_counted<datum_t>(geometry_string));
    r_sanity_check(!dup);
    dup = result.add("type", make_counted<datum_t>("Polygon"));
    r_sanity_check(!dup);
    std::vector<counted_t <const datum_t> > coordinates =
        lhs->get("coordinates")->as_array();
    coordinates.push_back(rhs->get("coordinates")->get(0));
    dup = result.add("coordinates", make_counted<datum_t>(std::move(coordinates)));
    r_sanity_check(!dup);
    counted_t<const datum_t> result_counted = result.to_counted();
    validate_geojson(result_counted);

    return result_counted;
}

} // namespace pseudo
} // namespace ql

