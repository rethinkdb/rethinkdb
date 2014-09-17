// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/pseudo_geometry.hpp"

#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {
namespace pseudo {

const char *const geometry_string = "GEOMETRY";
const char *const geo_latitude_key = "lat";
const char *const geo_longitude_key = "long";

void sanitize_geometry(datum_t *geo) {
    bool has_type = false;
    bool has_coordinates = false;
    for (size_t i = 0; i < geo->obj_size(); ++i) {
        auto pair = geo->get_pair(i);
        if (pair.first == "coordinates") {
            rcheck_target(&pair.second, base_exc_t::GENERIC,
                          pair.second.get_type() == datum_t::R_OBJECT ||
                          pair.second.get_type() == datum_t::R_ARRAY,
                          strprintf("Expected geometry object `coordinates` to be an "
                                    "OBJECT or ARRAY but found %s.",
                                    pair.second.get_type_name().c_str()));
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

datum_t make_geo_coordinate(const lat_lon_point_t &coordinate) {
    datum_object_builder_t builder;
    builder.overwrite(datum_string_t(geo_latitude_key), datum_t(coordinate.first));
    builder.overwrite(datum_string_t(geo_longitude_key), datum_t(coordinate.second));
    return std::move(builder).to_datum();
}

} // namespace pseudo
} // namespace ql

