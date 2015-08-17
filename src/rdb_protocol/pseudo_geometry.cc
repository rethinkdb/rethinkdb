// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/pseudo_geometry.hpp"

#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {
namespace pseudo {

const char *const geometry_string = "GEOMETRY";

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
            rfail_target(&pair.second, base_exc_t::LOGIC,
                         "Unrecognized field `%s` found in geometry object.",
                         pair.first.to_std().c_str());
        }
    }
    rcheck_target(geo,
                  has_type,
                  base_exc_t::NON_EXISTENCE,
                  "Field `type` not found in geometry object.");
    rcheck_target(geo,
                  has_coordinates,
                  base_exc_t::NON_EXISTENCE,
                  "Field `coordinates` not found in geometry object.");
}

} // namespace pseudo
} // namespace ql

