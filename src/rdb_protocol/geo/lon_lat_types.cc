// Copyright 2010-2016 RethinkDB, all rights reserved.
#include "rdb_protocol/geo/lon_lat_types.hpp"

RDB_IMPL_SERIALIZABLE_2(lon_lat_point_t, longitude, latitude);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(lon_lat_point_t);
