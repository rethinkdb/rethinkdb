// Copyright 2010-2016 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_GEO_LON_LAT_TYPES_HPP_
#define RDB_PROTOCOL_GEO_LON_LAT_TYPES_HPP_

#include <utility>
#include <vector>

#include "rpc/serialize_macros.hpp"

struct lon_lat_point_t {
    double longitude;
    double latitude;

    lon_lat_point_t() { }
    lon_lat_point_t(double lon, double lat) : longitude(lon), latitude(lat) { }

    bool operator==(const lon_lat_point_t &other) const {
        return (longitude == other.longitude) && (latitude == other.latitude);
    }

    bool operator!=(const lon_lat_point_t &other) const {
        return !(*this == other);
    }
};

RDB_DECLARE_SERIALIZABLE(lon_lat_point_t);

typedef std::vector<lon_lat_point_t> lon_lat_line_t;

#endif  // RDB_PROTOCOL_GEO_LON_LAT_TYPES_HPP_
