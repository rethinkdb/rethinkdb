// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/geo/ellipsoid.hpp"

#include "rpc/serialize_macros.hpp"

RDB_IMPL_ME_SERIALIZABLE_2(ellipsoid_spec_t, equator_radius_, flattening_);
INSTANTIATE_SERIALIZABLE_SELF_FOR_CLUSTER(ellipsoid_spec_t);
